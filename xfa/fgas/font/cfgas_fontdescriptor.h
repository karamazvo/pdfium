// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_FONT_CFGAS_FONTDESCRIPTOR_H_
#define XFA_FGAS_FONT_CFGAS_FONTDESCRIPTOR_H_

#include <stdint.h>

#include <array>
#include <memory>
#include <vector>

#include "build/build_config.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/widestring.h"

#if BUILDFLAG(IS_WIN)
#error "Built on the wrong platform"
#endif

class CFX_Face;
class CFX_ReadOnlyVectorStream;

// Represents metatdata about a font that isn't necessarily loaded yet.
class CFGAS_FontDescriptor {
 public:
  static std::unique_ptr<CFGAS_FontDescriptor> CreateFromStream(
      const RetainPtr<CFX_ReadOnlyVectorStream>& font_stream,
      int face_index,
      const WideString& face_name);

  CFGAS_FontDescriptor();
  ~CFGAS_FontDescriptor();

  int GetNumFaces() const;
  bool VerifyUnicodeForFontDescriptor(wchar_t unicode);

  int32_t face_index_ = 0;
  uint32_t font_styles_ = 0;
  WideString face_name_;
  RetainPtr<CFX_Face> face_;
  std::vector<WideString> family_names_;
  std::array<uint32_t, 4> usb_ = {};
  std::array<uint32_t, 2> csb_ = {};
};

#endif  // XFA_FGAS_FONT_CFGAS_FONTDESCRIPTOR_H_
