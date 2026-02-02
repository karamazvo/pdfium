// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_JXL_JXL_DECODER_H_
#define CORE_FXCODEC_JXL_JXL_DECODER_H_

#include <stdint.h>

#include <optional>

#include "core/fxcrt/span.h"

namespace pdfium::jxl {

// Minimal metadata needed by PDFium's image pipeline.
struct Info {
  uint32_t width = 0;
  uint32_t height = 0;
  bool has_alpha = false;
  bool have_animation = false;
};

// Parses enough of the codestream/container to obtain basic metadata.
// Returns nullopt on error.
std::optional<Info> ParseInfo(pdfium::span<const uint8_t> data);

// Decodes frame 0 into BGRA8.
//
// `dest_bgra` must have at least `dest_stride * height` bytes.
// `dest_stride` is bytes per row.
//
// Returns true on success.
bool DecodeFrame0ToBgra(pdfium::span<const uint8_t> data,
                        uint8_t* dest_bgra,
                        uint32_t dest_stride,
                        uint32_t width,
                        uint32_t height);

}  // namespace pdfium::jxl

#endif  // CORE_FXCODEC_JXL_JXL_DECODER_H_
