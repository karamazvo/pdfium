// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_PNG_SKIA_PNG_DECODER_H_
#define CORE_FXCODEC_PNG_SKIA_PNG_DECODER_H_

#include <memory>

#include "core/fxcodec/progressive_decoder_iface.h"
#include "core/fxcrt/retain_ptr.h"

#ifndef PDF_ENABLE_XFA_PNG
#error "PNG must be enabled"
#endif

namespace fxcodec {

class CFX_DIBAttribute;
class PngDecoderDelegate;

// PNG decoder based on the Skia library.
class SkiaPngDecoder {
 public:
  static std::unique_ptr<ProgressiveDecoderIface::Context> StartDecode(
      PngDecoderDelegate* pDelegate);

  static bool ContinueDecode(ProgressiveDecoderIface::Context* pContext,
                             RetainPtr<CFX_CodecMemory> codec_memory);

  SkiaPngDecoder() = delete;
  SkiaPngDecoder(const SkiaPngDecoder&) = delete;
  SkiaPngDecoder& operator=(const SkiaPngDecoder&) = delete;
};

}  // namespace fxcodec

using PngDecoder = fxcodec::SkiaPngDecoder;

#endif  // CORE_FXCODEC_PNG_SKIA_PNG_DECODER_H_
