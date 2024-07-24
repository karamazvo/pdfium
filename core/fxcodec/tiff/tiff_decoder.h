// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_TIFF_TIFF_DECODER_H_
#define CORE_FXCODEC_TIFF_TIFF_DECODER_H_

#include <stdint.h>

#include <memory>
#include <optional>

#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/progressive_decoder_iface.h"
#include "core/fxcrt/retain_ptr.h"

#ifndef PDF_ENABLE_XFA_TIFF
#error "TIFF must be enabled"
#endif

class CFX_DIBitmap;
class IFX_SeekableReadStream;

namespace fxcodec {

class TiffDecoder {
 public:
  struct FrameInfo {
    int32_t width;
    int32_t height;
    CFX_DIBAttribute attributes;
  };

  static std::unique_ptr<ProgressiveDecoderIface::Context> CreateDecoder(
      const RetainPtr<IFX_SeekableReadStream>& file_ptr);

  static std::optional<FrameInfo> LoadFirstFrameInfo(
      ProgressiveDecoderIface::Context* ctx);
  static bool Decode(ProgressiveDecoderIface::Context* ctx,
                     const RetainPtr<CFX_DIBitmap>& pDIBitmap);

  TiffDecoder() = delete;
  TiffDecoder(const TiffDecoder&) = delete;
  TiffDecoder& operator=(const TiffDecoder&) = delete;
};

}  // namespace fxcodec

using TiffDecoder = fxcodec::TiffDecoder;

#endif  // CORE_FXCODEC_TIFF_TIFF_DECODER_H_
