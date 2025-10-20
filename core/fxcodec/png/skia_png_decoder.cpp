// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/skia_png_decoder.h"

#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/png/png_decoder_delegate.h"
#include "core/fxcrt/check.h"
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

class SkiaPngContext;

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
  explicit CodecMemoryStream(UnownedPtr<SkiaPngContext> context)
      : context_(context) {}

  size_t read(void* buffer, size_t size) override;
  bool isAtEnd() const override;

 private:
  UnownedPtr<SkiaPngContext> const context_;
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
  // Returns `true` upon success and `false` upon failure.
  //
  // Communicates decoding results by calling `PngDecoderDelegate` methods
  // on the delegate passed earlier to the constructor.
  bool ContinueDecode(RetainPtr<CFX_CodecMemory> codec_memory);

  CFX_CodecMemory* current_input_chunk() { return current_input_chunk_.Get(); }

 private:
  // Starts or resumes decoding a PNG image from `current_input_chunk_`.
  bool ContinueDecode();

  // Creates `decoder_` and then invokes the `PngReadHeader` callback to
  // notifiy the `delegate_` about the image metadata, and to ask the
  // `delegate_` to set `target_gamma_`.
  bool CreateDecoder();

  // Uses `decoder_` to decode pixels into a buffer provided by the `delegate_`.
  bool DecodePixels();

  // `this` shouldn't be reused after reporting a fatal failure (after returning
  // `false` from `ContinueDecode`).  Each piece of code that reports a fatal
  // failure should call `ResetAfterFatalDecodingFailure` to 1) conserve
  // resources (e.g. deallocate `decoder_`) and 2) as a defense-in-depth (`this`
  // shouldn't be reused after a fatal failure, but if it is reused, then
  // resetting the state should reduce the number of states and pointer
  // lifetimes to worry about.
  void ResetAfterFatalDecodingFailure() {
    decoder_.reset();
    started_incremental_decode_ = false;
  }

  UnownedPtr<PngDecoderDelegate> const delegate_;

  // `current_input_chunk_` stores PNG input while `ContinueDecode` executes
  // (exposing the PNG input to `CodecMemoryStream`).  `current_input_chunk_` is
  // null at all other times (i.e. outside of `ContinueDecode` calls).
  RetainPtr<CFX_CodecMemory> current_input_chunk_;

  std::unique_ptr<SkCodec> decoder_;
  double target_gamma_ = 0.0;

  bool started_incremental_decode_ = false;
};

size_t CodecMemoryStream::read(void* buffer, size_t size) {
  uint8_t* bytes = static_cast<uint8_t*>(buffer);
  auto byte_span = UNSAFE_BUFFERS(pdfium::span(bytes, size));
  return context_->current_input_chunk()->ReadBlock(byte_span);
}

bool CodecMemoryStream::isAtEnd() const {
  return context_->current_input_chunk()->IsEOF();
}

bool SkiaPngContext::ContinueDecode(RetainPtr<CFX_CodecMemory> codec_memory) {
  CHECK(!current_input_chunk_);
  current_input_chunk_ = std::move(codec_memory);

  bool result = ContinueDecode();
  if (result) {
    CHECK(current_input_chunk_->IsEOF());
  }

  current_input_chunk_.Reset();
  return result;
}

bool SkiaPngContext::ContinueDecode() {
  CHECK(current_input_chunk_);

  if (!decoder_) {
    if (!CreateDecoder()) {
      return false;
    }
  }

  if (decoder_) {
    if (!DecodePixels()) {
      return false;
    }
  }

  return true;
}

bool SkiaPngContext::CreateDecoder() {
  // `CodecMemoryStream` should only be used underneath `ContinueDecode`
  // (since it is implemented on top of `current_input_chunk_`).
  CHECK(current_input_chunk_);
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
  constexpr int kPass = 1;
  if (!delegate_->PngReadHeader(info.width(), info.height(), kPass,
                                &target_gamma_)) {
    ResetAfterFatalDecodingFailure();
    return false;
  }

  return true;
}

bool SkiaPngContext::DecodePixels() {
  // Decoding uses `CodecMemoryStream` which should only be used underneath
  // `ContinueDecode` (since it is implemented on top of
  // `current_input_chunk_`).
  CHECK(current_input_chunk_);

  // Caller should ensure (e.g. by calling `CreateDecoder`) that the `decoder_`
  // has been already successfully constructed.
  CHECK(decoder_);

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
        break;
      case SkCodec::kIncompleteInput:
        return true;  // continue decoding when called again later
      default:
        ResetAfterFatalDecodingFailure();
        return false;
    }
  }

  SkCodec::Result result = decoder_->incrementalDecode(nullptr);
  switch (result) {
    case SkCodec::kSuccess:
    case SkCodec::kIncompleteInput:
      return true;
    default:
      ResetAfterFatalDecodingFailure();
      return false;
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
