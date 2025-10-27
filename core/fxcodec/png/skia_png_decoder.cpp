// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/skia_png_decoder.h"

#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/png/png_decoder_delegate.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/unowned_ptr.h"
#include "third_party/skia/include/codec/SkCodec.h"
#include "third_party/skia/include/core/SkStream.h"

#ifdef PDF_ENABLE_RUST_PNG
#include "third_party/skia/include/codec/SkPngRustDecoder.h"
#else
#include "third_party/skia/include/codec/SkPngDecoder.h"
#endif

namespace fxcodec {

namespace {

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
// Note that the exposed API does *not* support seeking/rewinding, because this
// layer onle has access to the `CFX_CodecMemory` pushed into `ContinueDecode`
// and can't reach for `IFX_SeekableReadStream` from which the bytes have been
// pulled into `CFX_CodecMemory`.  This lack of support for seeking/rewinding
// requires avoiding calling any Skia APIs that may necessitate this
// functionality later (e.g. calling `SkCodec::getFrameCount` may attempt to
// read metadata of subsequent frames).
class CodecMemoryStream final : public SkStream {
 public:
  explicit CodecMemoryStream(RetainPtr<CFX_CodecMemory> codec_memory)
      : codec_memory_(std::move(codec_memory)) {}

  size_t read(void* buffer, size_t size) override {
    // SAFETY: Relying on the caller to pass correct `buffer` and `size`.
    uint8_t* bytes = static_cast<uint8_t*>(buffer);
    auto byte_span = UNSAFE_BUFFERS(pdfium::span(bytes, size));

    return codec_memory_->ReadBlock(byte_span);
  }

  bool isAtEnd() const override { return codec_memory_->IsEOF(); }

 private:
  RetainPtr<CFX_CodecMemory> const codec_memory_;
};

class SkiaPngContext final : public ProgressiveDecoderIface::Context {
 public:
  // Caller needs to guarantee that `pDelegate` lives longer than
  // `SkiaPngContext`.
  explicit SkiaPngContext(PngDecoderDelegate* pDelegate)
      : delegate_(pDelegate) {}
  ~SkiaPngContext() override = default;

  // Starts or resumes decoding `codec_memory`.
  //
  // Returns `false` upon failure.  Returns `true` when either 1) the whole
  // image has been successfully decoded or 2) the image has been partially
  // decoded but decoding should be continued/retried when more input data
  // is available.
  //
  // Communicates image metadata (once read/available) via
  // `PngDecoderDelegate::PngReadHeader`.  Writes decoded BGRA pixels to the
  // buffer provided via `PngDecoderDelegate::PngAskImageBuf`.
  bool ContinueDecode(RetainPtr<CFX_CodecMemory> codec_memory);

 private:
  UnownedPtr<PngDecoderDelegate> const delegate_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
  std::unique_ptr<SkCodec> decoder_;
  bool delegate_accepted_image_metadata_ = false;
  double target_gamma_ = 0.0;
  bool started_incremental_decode_ = false;
};

bool SkiaPngContext::ContinueDecode(RetainPtr<CFX_CodecMemory> codec_memory) {
  // `ProgressiveDecoder` guarantees that all calls to `ContinueDecode` use the
  // same `codec_memory`.  Therefore `SkiaPngContext` expects that
  // `codec_memory` passed to `CodecMemoryStream` below remains the right one to
  // use going forward.
  if (!codec_memory_) {
    codec_memory_ = codec_memory;
  }
  CHECK_EQ(&*codec_memory_, &*codec_memory);

  if (!decoder_) {
    auto stream = std::make_unique<CodecMemoryStream>(std::move(codec_memory));
    SkCodec::Result result = SkCodec::kSuccess;
#ifdef PDF_ENABLE_RUST_PNG
    decoder_ = SkPngRustDecoder::Decode(std::move(stream), &result);
#else
    decoder_ = SkPngDecoder::Decode(std::move(stream), &result);
#endif
    switch (result) {
      case SkCodec::kSuccess:
        break;  // continue decoding
      case SkCodec::kIncompleteInput:
        // Rewind to start from the beginning of input when retrying later.
        // This will also prompt `ProgressiveDecoder::ReadMoreData` to grow
        // the `codec_memory_` as needed.
        codec_memory_->Seek(0);
        return true;  // retry when called later with more data
      default:
        decoder_.reset();
        return false;  // fatal error
    }
  }

  CHECK(decoder_);
  if (!delegate_accepted_image_metadata_) {
    SkImageInfo info = decoder_->getInfo();
    if (!delegate_->PngReadHeader(info.width(), info.height(),
                                  &target_gamma_)) {
      return false;
    }
    delegate_accepted_image_metadata_ = true;
  }

  if (!started_incremental_decode_) {
    SkImageInfo dst_info =
        decoder_->getInfo()
            .makeColorSpace(GetTargetColorSpace(target_gamma_))
            .makeColorType(kBGRA_8888_SkColorType);

    pdfium::span<uint8_t> dst_buffer = delegate_->PngAskImageBuf();
    FX_SAFE_SIZE_T row_bytes = dst_buffer.size();
    row_bytes /= dst_info.height();

    SkCodec::Result result = decoder_->startIncrementalDecode(
        dst_info, dst_buffer.data(), row_bytes.ValueOrDie());
    switch (result) {
      case SkCodec::kSuccess:
        started_incremental_decode_ = true;
        break;  // continue decoding
      case SkCodec::kIncompleteInput:
        return true;  // retry when called later with more data
      default:
        decoder_.reset();
        return false;  // fatal error
    }
  }

  SkCodec::Result result = decoder_->incrementalDecode(nullptr);
  switch (result) {
    case SkCodec::kSuccess:
      decoder_.reset();
      delegate_->PngFinishedDecoding();
      return true;  // finished decoding
    case SkCodec::kIncompleteInput:
      return true;  // retry when called later with more data
    default:
      decoder_.reset();
      return false;  // fatal error
  }
}

}  // namespace

// static
std::unique_ptr<ProgressiveDecoderIface::Context> SkiaPngDecoder::StartDecode(
    PngDecoderDelegate* pDelegate) {
  return std::make_unique<SkiaPngContext>(pDelegate);
}

// static
bool SkiaPngDecoder::ContinueDecode(ProgressiveDecoderIface::Context* context,
                                    RetainPtr<CFX_CodecMemory> codec_memory) {
  auto* ctx = static_cast<SkiaPngContext*>(context);
  return ctx->ContinueDecode(std::move(codec_memory));
}

}  // namespace fxcodec
