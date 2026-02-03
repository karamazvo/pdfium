// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_FONT_CFGAS_FONTMGR_H_
#define XFA_FGAS_FONT_CFGAS_FONTMGR_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "core/fxcrt/fx_codepage_forward.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/widestring.h"

class CFGAS_GEFont;

uint32_t ShortFormHash(FX_CodePage wCodePage,
                       uint32_t dwFontStyles,
                       WideStringView wsFontFamily);

class CFGAS_FontMgr {
 public:
  static std::unique_ptr<CFGAS_FontMgr> Create();
  virtual ~CFGAS_FontMgr();

  virtual bool EnumFonts() = 0;
  virtual RetainPtr<CFGAS_GEFont> GetFontByCodePage(
      FX_CodePage wCodePage,
      uint32_t dwFontStyles,
      const wchar_t* pszFontFamily) = 0;
  virtual RetainPtr<CFGAS_GEFont> LoadFont(const wchar_t* pszFontFamily,
                                           uint32_t dwFontStyles,
                                           FX_CodePage wCodePage) = 0;

  RetainPtr<CFGAS_GEFont> GetFontByUnicode(wchar_t wUnicode,
                                           uint32_t dwFontStyles,
                                           const wchar_t* pszFontFamily);

 protected:
  CFGAS_FontMgr();

  virtual RetainPtr<CFGAS_GEFont> GetFontByUnicodeImpl(
      wchar_t wUnicode,
      uint32_t dwFontStyles,
      const wchar_t* pszFontFamily,
      uint32_t dwHash,
      FX_CodePage wCodePage,
      uint16_t wBitField) = 0;

  std::map<uint32_t, std::vector<RetainPtr<CFGAS_GEFont>>> hash_2fonts_;
  std::set<wchar_t> failed_unicodes_set_;
};

#endif  // XFA_FGAS_FONT_CFGAS_FONTMGR_H_
