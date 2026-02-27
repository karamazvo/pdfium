// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_glyphbitmap.h"

#include <utility>

#include "core/fxcrt/check_op.h"
#include "core/fxge/dib/cfx_dibitmap.h"

#if defined(PDF_USE_SKIA)
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "third_party/skia/include/core/SkImageInfo.h"  // nogncheck
#endif

CFX_GlyphBitmap::CFX_GlyphBitmap(int left,
                                 int top,
                                 RetainPtr<CFX_DIBitmap> bitmap)
    : left_(left), top_(top), bitmap_(std::move(bitmap)) {
#if defined(PDF_USE_SKIA)
  if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
    CHECK_EQ(bitmap_->GetBPP(), 8);
    pdfium::span<uint8_t> bitmap_span = bitmap_->GetWritableBuffer();
    if (!bitmap_span.empty()) {
      sk_bitmap_.installPixels(
          SkImageInfo::MakeA8(bitmap_->GetWidth(), bitmap_->GetHeight()),
          bitmap_span.data(), bitmap_->GetPitch());
    }
  }
#endif
}

CFX_GlyphBitmap::~CFX_GlyphBitmap() = default;

RetainPtr<const CFX_DIBitmap> CFX_GlyphBitmap::GetBitmap() const {
  return bitmap_;
}
