// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_GLYPHBITMAP_H_
#define CORE_FXGE_CFX_GLYPHBITMAP_H_

#include "core/fxcrt/retain_ptr.h"

#if defined(PDF_USE_SKIA)
#include "third_party/skia/include/core/SkBitmap.h"
#endif  // defined(PDF_USE_SKIA)

class CFX_DIBitmap;

class CFX_GlyphBitmap {
 public:
  CFX_GlyphBitmap(int left, int top, RetainPtr<CFX_DIBitmap> bitmap);
  CFX_GlyphBitmap(const CFX_GlyphBitmap&) = delete;
  CFX_GlyphBitmap& operator=(const CFX_GlyphBitmap&) = delete;
  ~CFX_GlyphBitmap();

#if defined(PDF_USE_SKIA)
  SkBitmap& GetSkBitmap() { return sk_bitmap_; }
#endif  // defined(PDF_USE_SKIA)
  RetainPtr<const CFX_DIBitmap> GetBitmap() const;
  int left() const { return left_; }
  int top() const { return top_; }

 private:
  const int left_;
  const int top_;
  const RetainPtr<CFX_DIBitmap> bitmap_;  // Must outlive `sk_bitmap_`.
#if defined(PDF_USE_SKIA)
  SkBitmap sk_bitmap_;
#endif  // defined(PDF_USE_SKIA)
};

#endif  // CORE_FXGE_CFX_GLYPHBITMAP_H_
