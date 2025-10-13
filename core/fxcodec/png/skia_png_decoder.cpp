// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/skia_png_decoder.h"

#include <limits>
#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/png/png_decoder_delegate.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"
#include "core/fxcrt/unowned_ptr.h"
#include "third_party/skia/include/codec/SkCodec.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "third_party/skia/include/core/SkStream.h"

#ifdef PDF_ENABLE_RUST_PNG
#include "third_party/skia/include/codec/SkPngRustDecoder.h"
#else
#include "third_party/skia/include/codec/SkPngDecoder.h"
#endif

namespace fxcodec {

namespace {

using PngDecoderDelegate = fxcodec::PngDecoderDelegate;
using DecodedColorType = PngDecoderDelegate::DecodedColorType;
using EncodedColorType = PngDecoderDelegate::EncodedColorType;

class CSkiaPngContext;

PngDecoderDelegate::EncodedColorType GetEncodedColorType(
    const SkImageInfo& info) {
  switch (info.colorType()) {
    case kUnknown_SkColorType:
      NOTREACHED();

    // Gray or GrayAlpha
    case kAlpha_8_SkColorType:
    case kGray_8_SkColorType:
    case kA16_float_SkColorType:
    case kA16_unorm_SkColorType:
      switch (info.alphaType()) {
        case kUnknown_SkAlphaType:
          NOTREACHED();
        case kOpaque_SkAlphaType:
          return PngDecoderDelegate::EncodedColorType::kGrayscale;
        case kPremul_SkAlphaType:
        case kUnpremul_SkAlphaType:
          return PngDecoderDelegate::EncodedColorType::kGrayscaleWithAlpha;
      }
      NOTREACHED();  // Undeclared / unrecognized `SkAlphaType` enum value.

    // RGB or RGBA
    // (`SkCodec` layer will expand palette-based PNGs into RGB or RGBA, so
    // "indexed" is not covered by `SkColorType` and we don't need to handle it)
    case kRGB_565_SkColorType:
    case kARGB_4444_SkColorType:
    case kRGBA_8888_SkColorType:
    case kRGB_888x_SkColorType:
    case kBGRA_8888_SkColorType:
    case kRGBA_1010102_SkColorType:
    case kBGRA_1010102_SkColorType:
    case kRGB_101010x_SkColorType:
    case kBGR_101010x_SkColorType:
    case kBGR_101010x_XR_SkColorType:
    case kBGRA_10101010_XR_SkColorType:
    case kRGBA_10x6_SkColorType:
    case kRGBA_F16Norm_SkColorType:
    case kRGBA_F16_SkColorType:
    case kRGB_F16F16F16x_SkColorType:
    case kRGBA_F32_SkColorType:
    case kR8G8_unorm_SkColorType:
    case kR16G16_float_SkColorType:
    case kR16G16_unorm_SkColorType:
    case kR16G16B16A16_unorm_SkColorType:
    case kSRGBA_8888_SkColorType:
    case kR8_unorm_SkColorType:
      switch (info.alphaType()) {
        case kUnknown_SkAlphaType:
          NOTREACHED();
        case kOpaque_SkAlphaType:
          return PngDecoderDelegate::EncodedColorType::kTruecolor;
        case kPremul_SkAlphaType:
        case kUnpremul_SkAlphaType:
          return PngDecoderDelegate::EncodedColorType::kTruecolorWithAlpha;
      }
      NOTREACHED();  // Undeclared / unrecognized `SkAlphaType` enum value.
  }
  NOTREACHED();  // Undeclared / unrecognized `SkColorType` enum value.
}

sk_sp<SkColorSpace> GetTargetColorSpace(double gamma) {
  skcms_Matrix3x3 toXYZD50 = skcms_sRGB_profile()->toXYZD50;

  skcms_TransferFunction fn;
  fn.a = 1.0f;
  fn.b = fn.c = fn.d = fn.e = fn.f = 0.0f;
  fn.g = 1.0f / gamma;

  skcms_ICCProfile profile;
  skcms_Init(&profile);
  skcms_SetTransferFunction(&profile, &fn);
  skcms_SetXYZD50(&profile, &toXYZD50);

  return SkColorSpace::Make(profile);
}

// Implements/exposes `SkStream` API on top of `CFX_CodecMemory`.
//
// Note that the exposed API does *not* support seeking/rewinding, because at
// this layer we only have access to the `CFX_CodecMemory` pushed into
// `ContinueDecode` and we can't reach for `IFX_SeekableReadStream` from which
// the bytes have been pulled into `CFX_CodecMemory`.  This lack of support for
// seeking/rewinding means that we should avoid calling any Skia APIs that may
// necessitate this functionality later (e.g. calling `SkCodec::getFrameCount`
// may attempt to read metadata of subsequent frames).
class CodecMemoryStream final : public SkStream {
 public:
  explicit CodecMemoryStream(UnownedPtr<CSkiaPngContext> context)
      : context_(context) {}

  size_t read(void* buffer, size_t size) override;
  bool isAtEnd() const override;

 private:
  UnownedPtr<CSkiaPngContext> const context_;
};

class CSkiaPngContext final : public ProgressiveDecoderIface::Context {
 public:
  // Caller needs to guarantee that `pDelegate` lives longer than
  // `CSkiaPngContext`.
  explicit CSkiaPngContext(PngDecoderDelegate* pDelegate)
      : delegate_(pDelegate) {}
  ~CSkiaPngContext() override = default;

  // Starts or resumes decoding `codec_memory`.
  //
  // Returns `true` upon success and `false` upon failure.
  //
  // Communicates decoding results by calling `PngDecoderDelegate` methods
  // on the delegate passed earlier to the constructor.
  bool ContinueDecode(RetainPtr<CFX_CodecMemory> codec_memory);

  CFX_CodecMemory* current_input_chunk() { return current_input_chunk_.Get(); }

 private:
  // Starts or resumes decoding a PNG image from `current_input_chunk_`.
  bool ContinueDecode();

  // Creates `decoder_` and `skia_output_buffer_`.
  bool CreateDecoder();

  // Uses `decoder_` to decode pixels.
  // Calls `HandleDecodedRow` for decoded rows.
  bool DecodePixels();
  void HandleDecodedRow(int row_number);

  // `this` shouldn't be reused after reporting a fatal failure (after returning
  // `false` from `ContinueDecode`).  Each piece of code that reports a fatal
  // failure should call `ResetAfterFatalDecodingFailure` to 1) conserve
  // resources (e.g. deallocate `decoder_`) and 2) as a defense-in-depth (`this`
  // shouldn't be reused after a fatal failure, but if it is reused, then
  // resetting the state should reduce the number of states and pointer
  // lifetimes to worry about.
  void ResetAfterFatalDecodingFailure() {
    decoder_.reset();
    skia_output_buffer_.reset();
    intermediate_bgra_buffer_.reset();
    started_incremental_decode_ = false;
    rows_decoded_ = std::numeric_limits<int>::max();
  }

  UnownedPtr<PngDecoderDelegate> const delegate_;

  // `current_input_chunk_` stores PNG input while `ContinueDecode` executes
  // (exposing the PNG input to `CodecMemoryStream`).  `current_input_chunk_` is
  // null at all other times (i.e. outside of `ContinueDecode` calls).
  RetainPtr<CFX_CodecMemory> current_input_chunk_;

  std::unique_ptr<SkCodec> decoder_;

  // Depending on `DecodedColorType` returned from `delegate_->PngReadHeader` we
  // need to decode either to BGRA or BGR.  `SkColorType` doesn't support asking
  // Skia to decode to BGR, so we use an intermediate buffer in this case.
  //
  // When decoding BGRA: `skia_output_buffer_` points directly to the memory
  // returned from `delegate_->PngAskScanlineBuf(0)`
  // (`intermediate_bgra_buffer_` is `nullopt`).
  //
  // When decoding BGR: `skia_output_buffer_` points to
  // `intermediate_bgra_buffer_`. We manually strip the alpha channel in
  // `HandleDecodedRow` when copying from `intermediate_bgra_buffer_` into the
  // buffer provided by `delegate_->PngAskScanlineBuf(...)`.
  std::optional<SkPixmap> skia_output_buffer_;        // `SkPixmap` = non-owned
  std::optional<SkBitmap> intermediate_bgra_buffer_;  // `SkBitmap` = owned

  bool started_incremental_decode_ = false;

  // Number of rows which have been decoded and communicated to
  // `PngDecoderDelegate::PngFillScanlineBufCompleted`.
  //
  // Initially initialized to an invalid, max-`int` value.
  int rows_decoded_ = std::numeric_limits<int>::max();
};

size_t CodecMemoryStream::read(void* buffer, size_t size) {
  uint8_t* bytes = static_cast<uint8_t*>(buffer);
  auto byte_span = UNSAFE_BUFFERS(pdfium::span(bytes, size));
  return context_->current_input_chunk()->ReadBlock(byte_span);
}

bool CodecMemoryStream::isAtEnd() const {
  return context_->current_input_chunk()->IsEOF();
}

bool CSkiaPngContext::ContinueDecode(RetainPtr<CFX_CodecMemory> codec_memory) {
  CHECK(!current_input_chunk_);
  current_input_chunk_ = std::move(codec_memory);

  bool result = ContinueDecode();
  if (result) {
    CHECK(current_input_chunk_->IsEOF());
  }

  current_input_chunk_.Reset();
  return result;
}

bool CSkiaPngContext::ContinueDecode() {
  CHECK(current_input_chunk_);

  if (!decoder_) {
    if (!CreateDecoder()) {
      return false;
    }
  }

  if (decoder_) {
    CHECK(skia_output_buffer_.has_value());
    if (!DecodePixels()) {
      return false;
    }
  }

  return true;
}

bool CSkiaPngContext::CreateDecoder() {
  // `CodecMemoryStream` should only be used underneath `ContinueDecode`
  // (since it is implemented on top of `current_input_chunk_`).
  CHECK(current_input_chunk_);
  auto stream = std::make_unique<CodecMemoryStream>(this);

  CHECK(!decoder_);
  SkCodec::Result result = SkCodec::kSuccess;
#ifdef PDF_ENABLE_RUST_PNG
  fprintf(stderr, "DO NOT SUBMIT - ad-hoc test\n");
  NOTREACHED();
  decoder_ = SkPngRustDecoder::Decode(std::move(stream), &result);
#else
  decoder_ = SkPngDecoder::Decode(std::move(stream), &result);
#endif
  switch (result) {
    case SkCodec::kSuccess:
      CHECK(decoder_);
      break;  // continue decoding below
    case SkCodec::kIncompleteInput:
      return true;  // continue decoding when called again later
    default:
      ResetAfterFatalDecodingFailure();
      return false;
  }

  SkImageInfo info = decoder_->getInfo();
  PngDecoderDelegate::EncodedColorType encoded_color_type =
      GetEncodedColorType(info);
  int bits_per_component =
      (8 * info.bytesPerPixel()) /
      PngDecoderDelegate::GetNumberOfComponents(encoded_color_type);
  constexpr int kPass = !0;
  PngDecoderDelegate::DecodedColorType target_color_type;
  double target_gamma = 0.0;
  if (!delegate_->PngReadHeader(info.width(), info.height(), bits_per_component,
                                kPass, encoded_color_type, &target_color_type,
                                &target_gamma)) {
    ResetAfterFatalDecodingFailure();
    return false;
  }

  SkImageInfo skia_output_info =
      info.makeColorSpace(GetTargetColorSpace(target_gamma));

  // `skia_output_info` is intentionally always set to BGRA, even if
  // `target_color_type` asks for BGR.  In the BGR scenario we use
  // `intermediate_bgra_buffer_` and strip the alpha channel in
  // `HandleDecodedRow`.
  skia_output_info = skia_output_info.makeColorType(kBGRA_8888_SkColorType);

  CHECK(!skia_output_buffer_.has_value());
  CHECK(!intermediate_bgra_buffer_.has_value());
  switch (target_color_type) {
    case PngDecoderDelegate::DecodedColorType::kBgr:
      intermediate_bgra_buffer_ = SkBitmap();
      if (!intermediate_bgra_buffer_->tryAllocPixels(skia_output_info)) {
        ResetAfterFatalDecodingFailure();
        return false;
      }
      skia_output_buffer_ =
          SkPixmap(intermediate_bgra_buffer_->info(),
                   intermediate_bgra_buffer_->pixmap().writable_addr(),
                   intermediate_bgra_buffer_->rowBytes());
      break;
    case PngDecoderDelegate::DecodedColorType::kBgra:
      skia_output_buffer_ =
          SkPixmap(skia_output_info, delegate_->PngAskScanlineBuf(0),
                   skia_output_info.minRowBytes());
      break;
  }

  return true;
}

bool CSkiaPngContext::DecodePixels() {
  // Decoding uses `CodecMemoryStream` which should only be used underneath
  // `ContinueDecode` (since it is implemented on top of
  // `current_input_chunk_`).
  CHECK(current_input_chunk_);

  // Caller should ensure (e.g. by calling `CreateDecoder`) that the `decoder_`
  // has been successfully constructed and that output format+destination has
  // been determined.
  CHECK(decoder_);
  CHECK(skia_output_buffer_);

  // Caller shouldn't try to decode more than the available number of rows.
  CHECK_LE(rows_decoded_, skia_output_buffer_->height());

  if (!started_incremental_decode_) {
    SkCodec::Result result = decoder_->startIncrementalDecode(
        skia_output_buffer_->info(), skia_output_buffer_->writable_addr(),
        skia_output_buffer_->rowBytes());
    switch (result) {
      case SkCodec::kSuccess:
        started_incremental_decode_ = true;
        rows_decoded_ = 0;
        break;
      case SkCodec::kIncompleteInput:
        return true;  // continue decoding when called again later
      default:
        ResetAfterFatalDecodingFailure();
        return false;
    }
  }

  int new_value_of_rows_decoded = 0;
  SkCodec::Result result =
      decoder_->incrementalDecode(&new_value_of_rows_decoded);
  switch (result) {
    case SkCodec::kSuccess:
    case SkCodec::kIncompleteInput:
      break;
    default:
      ResetAfterFatalDecodingFailure();
      return false;
  }

  for (; rows_decoded_ < new_value_of_rows_decoded; rows_decoded_++) {
    HandleDecodedRow(rows_decoded_);
  }

  return true;
}

void CSkiaPngContext::HandleDecodedRow(int row_number) {
  CHECK(skia_output_buffer_);

  // If needed, then take BGRA pixels and convert/copy them into BGR format.
  if (intermediate_bgra_buffer_.has_value()) {
    // Check that each pixel takes 4 bytes and there is no padding between rows.
    size_t width = skia_output_buffer_->width();
    CHECK_EQ(skia_output_buffer_->rowBytes() % 4, 0);
    CHECK_EQ(skia_output_buffer_->rowBytes() / 4, width);

    CHECK_LT(row_number, skia_output_buffer_->height());
    const uint32_t* src_ptr = skia_output_buffer_->addr32(0, row_number);
    auto src_bgra =
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(src_ptr),
                                 skia_output_buffer_->rowBytes());

    // No integer overlow in `width * 3` is guaranteed because of
    // `CHECK_EQ(...rowBytes() / 4, width)` above.
    uint8_t* dst_ptr = delegate_->PngAskScanlineBuf(row_number);
    auto dst_bgr = std::span<uint8_t>(dst_ptr, width * 3);

    while (!src_bgra.empty()) {
      pdfium::span<uint8_t> dst_pixel = dst_bgr.first<3u>();
      pdfium::span<const uint8_t> src_pixel = src_bgra.first<4u>();

      // Copy the RGB channels and intentionally ignore the alpha channel.
      fxcrt::spancpy(dst_pixel, src_pixel.first<3u>());

      src_bgra = src_bgra.subspan(src_pixel.size());
      dst_bgr = dst_bgr.subspan(dst_pixel.size());
    }
    CHECK(src_bgra.empty());
    CHECK(dst_bgr.empty());
  }

  delegate_->PngFillScanlineBufCompleted(row_number);
}

}  // namespace

// static
std::unique_ptr<ProgressiveDecoderIface::Context> SkiaPngDecoder::StartDecode(
    PngDecoderDelegate* pDelegate) {
  return std::make_unique<CSkiaPngContext>(pDelegate);
}

// static
bool SkiaPngDecoder::ContinueDecode(ProgressiveDecoderIface::Context* pContext,
                                    RetainPtr<CFX_CodecMemory> codec_memory) {
  auto* ctx = static_cast<CSkiaPngContext*>(pContext);
  return ctx->ContinueDecode(std::move(codec_memory));
}

}  // namespace fxcodec
