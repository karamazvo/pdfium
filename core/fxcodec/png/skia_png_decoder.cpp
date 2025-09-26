// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/skia_png_decoder.h"

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/png/png_decoder_delegate.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"
#include "core/fxcrt/unowned_ptr.h"
#include "png_decoder_delegate.h"
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

  // Starts or continues decoding `codec_memory`.
  //
  // Returns `true` upon success and `false` upon failure.
  //
  // Communicates decoding results by calling `PngDecoderDelegate` methods
  // on the delegate passed earlier to the constructor.
  bool ContinueDecode(RetainPtr<CFX_CodecMemory> codec_memory);

  CFX_CodecMemory* current_input_chunk() { return current_input_chunk_.Get(); }

 private:
  bool ContinueDecode();
  bool CreateDecoder();
  bool DecodePixels();
  void ResetAfterFatalDecodingFailure() {
    decoder_.reset();
    skia_output_info_.reset();
    skia_output_buffer_.reset();
    intermediate_rgba_buffer_.reset();
  }

  UnownedPtr<PngDecoderDelegate> const delegate_;

  // `current_input_chunk_` stores PNG input while `ContinueDecode` executes
  // (exposing the PNG input to `CodecMemoryStream`).  `current_input_chunk_` is
  // null at all other times (i.e. outside of `ContinueDecode` calls).
  RetainPtr<CFX_CodecMemory> current_input_chunk_;

  std::unique_ptr<SkCodec> decoder_;
  std::optional<SkImageInfo> skia_output_info_;

  // Depending on `DecodedColorType` returned from `delegate_->PngReadHeader` we
  // need to decode either to BGRA or BGR.  `SkColorType` doesn't support asking
  // Skia to decode to BGR, so we use an intermediate buffer in this case.
  //
  // When decoding BGRA: `skia_output_buffer_` points directly to the memory
  // returned from `delegate_->PngAskScanlineBuf(0)`
  // (`intermediate_rgba_buffer_` is `nullopt`).
  //
  // When decoding BGR: `skia_output_buffer_` points to
  // `intermediate_rgba_buffer_`. We manually strip the alpha channel in
  // `HandleDecodedRow` when copying from `intermediate_rgba_buffer_` into the
  // buffer provided by `delegate_->PngAskScanlineBuf(...)`.
  std::optional<SkPixmap> skia_output_buffer_;        // `SkPixmap` = non-owned
  std::optional<SkBitmap> intermediate_rgba_buffer_;  // `SkBitmap` = owned
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
  if (!decoder_) {
    if (!CreateDecoder()) {
      return false;
    }
  }

  if (decoder_) {
    CHECK(skia_output_info_.has_value());
    CHECK(skia_output_buffer_.has_value());
    if (!DecodePixels()) {
      return false;
    }
  }

  return true;
}

bool CSkiaPngContext::CreateDecoder() {
  auto stream = std::make_unique<CodecMemoryStream>(this);

  CHECK(!decoder_);
  SkCodec::Result result = SkCodec::kSuccess;
#ifdef PDF_ENABLE_RUST_PNG
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

  CHECK(!skia_output_info_.has_value());
  skia_output_info_ = info.makeColorSpace(GetTargetColorSpace(target_gamma));

  // `skia_output_info_` is intentionally always set to BGRA, even if
  // `target_color_type` asks for BGR.  In the BGR scenario we use
  // `intermediate_rgba_buffer_` and strip the alpha channel in
  // `HandleDecodedRow`.
  skia_output_info_ = skia_output_info_->makeColorType(kBGRA_8888_SkColorType);

  CHECK(!skia_output_buffer_.has_value());
  CHECK(!intermediate_rgba_buffer_.has_value());
  switch (target_color_type) {
    case PngDecoderDelegate::DecodedColorType::kBgr:
      intermediate_rgba_buffer_ = SkBitmap();
      if (!intermediate_rgba_buffer_->tryAllocPixels(*skia_output_info_)) {
        ResetAfterFatalDecodingFailure();
        return false;
      }
      skia_output_buffer_ =
          SkPixmap(intermediate_rgba_buffer_->info(),
                   intermediate_rgba_buffer_->pixmap().writable_addr(),
                   intermediate_rgba_buffer_->rowBytes());
      break;
    case PngDecoderDelegate::DecodedColorType::kBgra:
      skia_output_buffer_ =
          SkPixmap(*skia_output_info_, delegate_->PngAskScanlineBuf(0),
                   skia_output_info_->minRowBytes());
      break;
  }

  return true;
}

bool CSkiaPngContext::DecodePixels() {
  CHECK(decoder_);
  CHECK(skia_output_info_);
  CHECK(skia_output_buffer_);

  NOTREACHED();  // DO NOT SUBMIT - call SkCodec::getImage
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
