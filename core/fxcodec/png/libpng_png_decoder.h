// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_PNG_LIBPNG_PNG_DECODER_H_
#define CORE_FXCODEC_PNG_LIBPNG_PNG_DECODER_H_

#include <memory>

#include "core/fxcodec/progressive_decoder_iface.h"
#include "core/fxcrt/retain_ptr.h"

#ifndef PDF_ENABLE_XFA_PNG
#error "PNG must be enabled"
#endif

#ifdef PDF_ENABLE_RUST_PNG
// TODO(https://crbug.com/444045690): After adding Rust-PNG-based decoding
// support we should introduce a hard build error if `libpng` is used in
// presence of the `PDF_ENABLE_RUST_PNG` macro definition:
// #error "If Rust PNG is enabled, then `libpng` should not be used."
#endif

namespace fxcodec {

class PngDecoderDelegate;

// PNG decoder that uses the `libpng` library to decode pixels.
class LibpngPngDecoder {
 public:
  static std::unique_ptr<ProgressiveDecoderIface::Context> StartDecode(
      PngDecoderDelegate* pDelegate);

  static bool ContinueDecode(ProgressiveDecoderIface::Context* pContext,
                             RetainPtr<CFX_CodecMemory> codec_memory);

  LibpngPngDecoder() = delete;
  LibpngPngDecoder(const LibpngPngDecoder&) = delete;
  LibpngPngDecoder& operator=(const LibpngPngDecoder&) = delete;
};

}  // namespace fxcodec

using PngDecoder = fxcodec::LibpngPngDecoder;

#endif  // CORE_FXCODEC_PNG_LIBPNG_PNG_DECODER_H_
