// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_glyphbitmap.h"

#include <utility>

#include "core/fxge/dib/cfx_dibitmap.h"

#if defined(PDF_USE_SKIA)
#include "third_party/skia/include/core/SkImageInfo.h"  // nogncheck
#endif

CFX_GlyphBitmap::CFX_GlyphBitmap(int left,
                                 int top,
                                 RetainPtr<CFX_DIBitmap> bitmap)
    : left_(left), top_(top), bitmap_(std::move(bitmap)) {
#if defined(PDF_USE_SKIA)
  pdfium::span<uint8_t> bitmap_span = bitmap->GetWritableBuffer();
  if (!bitmap_span.empty()) {
    SkImageInfo info =
        SkImageInfo::MakeN32Premul(bitmap->GetWidth(), bitmap->GetHeight());
    // Assuming 4 bytes per pixel (RGBA/BGRA)
    const size_t row_bytes = bitmap->GetPitch() * 4;
    sk_bitmap_.installPixels(info, bitmap_span.data(), row_bytes);
  }
#endif
}

CFX_GlyphBitmap::~CFX_GlyphBitmap() = default;

RetainPtr<const CFX_DIBitmap> CFX_GlyphBitmap::GetBitmap() const {
  return bitmap_;
}
