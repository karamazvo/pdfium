// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/android/cfpf_skiapathfont.h"

CFPF_SkiaPathFont::CFPF_SkiaPathFont(const ByteString& path,
                                     const ByteString& pFamily,
                                     uint32_t dwStyle,
                                     int32_t iFaceIndex,
                                     uint32_t dwCharsets,
                                     int32_t iGlyphNum)
    : bs_path_(path),
      bs_family_(pFamily),
      dw_style_(dwStyle),
      face_index_(iFaceIndex),
      dw_charsets_(dwCharsets),
      glyph_num_(iGlyphNum) {}

CFPF_SkiaPathFont::~CFPF_SkiaPathFont() = default;
