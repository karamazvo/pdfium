// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/font/cfgas_fontmgr_win.h"

#include "build/build_config.h"

#if !BUILDFLAG(IS_WIN)
#error "Built on wrong platform"
#endif

#include <stdint.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <deque>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_memcpy_wrappers.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/win/win_util.h"
#include "core/fxge/fx_font.h"
#include "xfa/fgas/font/cfgas_gefont.h"
#include "xfa/fgas/font/fgas_fontutils.h"

namespace {

struct FX_FONTMATCHPARAMS {
  const wchar_t* pwsFamily;
  uint32_t dwFontStyles;
  uint32_t dwUSB;
  bool matchParagraphStyle;
  wchar_t wUnicode;
  FX_CodePage wCodePage;
};

int32_t GetSimilarityScore(FX_FONTDESCRIPTOR const* font,
                           uint32_t dwFontStyles) {
  int32_t iValue = 0;
  if (FontStyleIsSymbolic(dwFontStyles) ==
      FontStyleIsSymbolic(font->dwFontStyles)) {
    iValue += 64;
  }
  if (FontStyleIsFixedPitch(dwFontStyles) ==
      FontStyleIsFixedPitch(font->dwFontStyles)) {
    iValue += 32;
  }
  if (FontStyleIsSerif(dwFontStyles) == FontStyleIsSerif(font->dwFontStyles)) {
    iValue += 16;
  }
  if (FontStyleIsScript(dwFontStyles) ==
      FontStyleIsScript(font->dwFontStyles)) {
    iValue += 8;
  }
  return iValue;
}

const FX_FONTDESCRIPTOR* MatchDefaultFont(
    FX_FONTMATCHPARAMS* pParams,
    const std::deque<FX_FONTDESCRIPTOR>& fonts) {
  const FX_FONTDESCRIPTOR* pBestFont = nullptr;
  int32_t iBestSimilar = 0;
  for (const auto& font : fonts) {
    if (FontStyleIsForceBold(font.dwFontStyles) &&
        FontStyleIsItalic(font.dwFontStyles)) {
      continue;
    }

    if (pParams->pwsFamily) {
      if (FXSYS_wcsicmp(pParams->pwsFamily, font.wsFontFace)) {
        continue;
      }
      if (font.uCharSet == FX_Charset::kSymbol) {
        return &font;
      }
    }
    if (font.uCharSet == FX_Charset::kSymbol) {
      continue;
    }
    if (pParams->wCodePage != FX_CodePage::kFailure) {
      if (FX_GetCodePageFromCharset(font.uCharSet) != pParams->wCodePage) {
        continue;
      }
    } else {
      if (pParams->dwUSB < 128) {
        uint32_t dwByte = pParams->dwUSB / 32;
        uint32_t dwUSB = 1 << (pParams->dwUSB % 32);
        if ((font.FontSignature.fsUsb[dwByte] & dwUSB) == 0) {
          continue;
        }
      }
    }
    if (pParams->matchParagraphStyle) {
      if ((font.dwFontStyles & 0x0F) == (pParams->dwFontStyles & 0x0F)) {
        return &font;
      }
      continue;
    }
    if (pParams->pwsFamily) {
      if (FXSYS_wcsicmp(pParams->pwsFamily, font.wsFontFace) == 0) {
        return &font;
      }
    }
    int32_t iSimilarValue = GetSimilarityScore(&font, pParams->dwFontStyles);
    if (iBestSimilar < iSimilarValue) {
      iBestSimilar = iSimilarValue;
      pBestFont = &font;
    }
  }
  return iBestSimilar < 1 ? nullptr : pBestFont;
}

uint32_t GetGdiFontStyles(const LOGFONTW& lf) {
  uint32_t dwStyles = 0;
  if ((lf.lfPitchAndFamily & 0x03) == FIXED_PITCH) {
    dwStyles |= pdfium::kFontStyleFixedPitch;
  }
  uint8_t nFamilies = lf.lfPitchAndFamily & 0xF0;
  if (nFamilies == FF_ROMAN) {
    dwStyles |= pdfium::kFontStyleSerif;
  }
  if (nFamilies == FF_SCRIPT) {
    dwStyles |= pdfium::kFontStyleScript;
  }
  if (lf.lfCharSet == SYMBOL_CHARSET) {
    dwStyles |= pdfium::kFontStyleSymbolic;
  }
  return dwStyles;
}

int32_t CALLBACK GdiFontEnumProc(ENUMLOGFONTEX* lpelfe,
                                 NEWTEXTMETRICEX* lpntme,
                                 DWORD dwFontType,
                                 LPARAM lParam) {
  if (dwFontType != TRUETYPE_FONTTYPE) {
    return 1;
  }
  const LOGFONTW& lf = ((LPENUMLOGFONTEXW)lpelfe)->elfLogFont;
  if (lf.lfFaceName[0] == L'@') {
    return 1;
  }
  FX_FONTDESCRIPTOR font = {};  // Aggregate initialization.
  static_assert(std::is_aggregate_v<decltype(font)>);
  font.uCharSet = FX_GetCharsetFromInt(lf.lfCharSet);
  font.dwFontStyles = GetGdiFontStyles(lf);
  UNSAFE_TODO({
    FXSYS_wcsncpy(font.wsFontFace, (const wchar_t*)lf.lfFaceName, 31);
    font.wsFontFace[31] = 0;
    FXSYS_memcpy(&font.FontSignature, &lpntme->ntmFontSig,
                 sizeof(lpntme->ntmFontSig));
  });
  reinterpret_cast<std::deque<FX_FONTDESCRIPTOR>*>(lParam)->push_back(font);
  return 1;
}

std::deque<FX_FONTDESCRIPTOR> EnumGdiFonts(const wchar_t* pwsFaceName,
                                           wchar_t wUnicode) {
  std::deque<FX_FONTDESCRIPTOR> fonts;
  if (!pdfium::IsUser32AndGdi32Available()) {
    // Without GDI32 and User32, GetDC / EnumFontFamiliesExW / ReleaseDC all
    // fail.
    return fonts;
  }

  LOGFONTW lfFind = {};  // Aggregate initialization.
  static_assert(std::is_aggregate_v<decltype(lfFind)>);
  lfFind.lfCharSet = DEFAULT_CHARSET;
  if (pwsFaceName) {
    UNSAFE_TODO({
      FXSYS_wcsncpy(lfFind.lfFaceName, pwsFaceName, 31);
      lfFind.lfFaceName[31] = 0;
    });
  }
  HDC hDC = ::GetDC(nullptr);
  EnumFontFamiliesExW(hDC, (LPLOGFONTW)&lfFind, (FONTENUMPROCW)GdiFontEnumProc,
                      (LPARAM)&fonts, 0);
  ::ReleaseDC(nullptr, hDC);
  return fonts;
}

}  // namespace

CFGAS_FontMgrWin::CFGAS_FontMgrWin()
    : font_faces_(EnumGdiFonts(nullptr, 0xFEFF)) {}

CFGAS_FontMgrWin::~CFGAS_FontMgrWin() = default;

bool CFGAS_FontMgrWin::EnumFonts() {
  return true;
}

RetainPtr<CFGAS_GEFont> CFGAS_FontMgrWin::GetFontByCodePage(
    FX_CodePage wCodePage,
    uint32_t dwFontStyles,
    const wchar_t* pszFontFamily) {
  uint32_t dwHash = ShortFormHash(wCodePage, dwFontStyles, pszFontFamily);
  auto* font_vector = &hash_2fonts_[dwHash];
  if (!font_vector->empty()) {
    for (auto iter = font_vector->begin(); iter != font_vector->end(); ++iter) {
      if (*iter != nullptr) {
        return *iter;
      }
    }
    return nullptr;
  }

  const FX_FONTDESCRIPTOR* pFD =
      FindFont(pszFontFamily, dwFontStyles, true, wCodePage,
               FGAS_FONTUSB::kNoBitField, 0);
  if (!pFD) {
    pFD = FindFont(nullptr, dwFontStyles, true, wCodePage,
                   FGAS_FONTUSB::kNoBitField, 0);
  }
  if (!pFD) {
    pFD = FindFont(nullptr, dwFontStyles, false, wCodePage,
                   FGAS_FONTUSB::kNoBitField, 0);
  }
  if (!pFD) {
    return nullptr;
  }

  RetainPtr<CFGAS_GEFont> font =
      CFGAS_GEFont::LoadFont(pFD->wsFontFace, dwFontStyles, wCodePage);

  if (!font) {
    return nullptr;
  }

  font->SetLogicalFontStyle(dwFontStyles);
  font_vector->push_back(font);
  return font;
}

RetainPtr<CFGAS_GEFont> CFGAS_FontMgrWin::LoadFont(const wchar_t* pszFontFamily,
                                                   uint32_t dwFontStyles,
                                                   FX_CodePage wCodePage) {
  uint32_t dwHash = ShortFormHash(wCodePage, dwFontStyles, pszFontFamily);
  std::vector<RetainPtr<CFGAS_GEFont>>* font_array = &hash_2fonts_[dwHash];
  if (!font_array->empty()) {
    return font_array->front();
  }

  const FX_FONTDESCRIPTOR* pFD =
      FindFont(pszFontFamily, dwFontStyles, true, wCodePage,
               FGAS_FONTUSB::kNoBitField, 0);
  if (!pFD) {
    pFD = FindFont(pszFontFamily, dwFontStyles, false, wCodePage,
                   FGAS_FONTUSB::kNoBitField, 0);
  }
  if (!pFD) {
    return nullptr;
  }

  RetainPtr<CFGAS_GEFont> font =
      CFGAS_GEFont::LoadFont(pFD->wsFontFace, dwFontStyles, wCodePage);
  if (!font) {
    return nullptr;
  }

  font->SetLogicalFontStyle(dwFontStyles);
  font_array->push_back(font);
  return font;
}

RetainPtr<CFGAS_GEFont> CFGAS_FontMgrWin::GetFontByUnicodeImpl(
    wchar_t wUnicode,
    uint32_t dwFontStyles,
    const wchar_t* pszFontFamily,
    uint32_t dwHash,
    FX_CodePage wCodePage,
    uint16_t wBitField) {
  const FX_FONTDESCRIPTOR* pFD = FindFont(pszFontFamily, dwFontStyles, false,
                                          wCodePage, wBitField, wUnicode);
  if (!pFD && pszFontFamily) {
    pFD =
        FindFont(nullptr, dwFontStyles, false, wCodePage, wBitField, wUnicode);
  }
  if (!pFD) {
    return nullptr;
  }

  FX_CodePage newCodePage = FX_GetCodePageFromCharset(pFD->uCharSet);
  RetainPtr<CFGAS_GEFont> font =
      CFGAS_GEFont::LoadFont(pFD->wsFontFace, dwFontStyles, newCodePage);
  if (!font) {
    return nullptr;
  }

  font->SetLogicalFontStyle(dwFontStyles);
  if (!font->VerifyUnicode(wUnicode)) {
    failed_unicodes_set_.insert(wUnicode);
    return nullptr;
  }

  hash_2fonts_[dwHash].push_back(font);
  return font;
}

const FX_FONTDESCRIPTOR* CFGAS_FontMgrWin::FindFont(
    const wchar_t* pszFontFamily,
    uint32_t dwFontStyles,
    bool matchParagraphStyle,
    FX_CodePage wCodePage,
    uint32_t dwUSB,
    wchar_t wUnicode) {
  FX_FONTMATCHPARAMS params = {};  // Aggregate initialization.
  static_assert(std::is_aggregate_v<decltype(params)>);
  params.dwUSB = dwUSB;
  params.wUnicode = wUnicode;
  params.wCodePage = wCodePage;
  params.pwsFamily = pszFontFamily;
  params.dwFontStyles = dwFontStyles;
  params.matchParagraphStyle = matchParagraphStyle;

  const FX_FONTDESCRIPTOR* pDesc = MatchDefaultFont(&params, font_faces_);
  if (pDesc) {
    return pDesc;
  }

  if (!pszFontFamily) {
    return nullptr;
  }

  // Use a named object to store the returned value of EnumGdiFonts() instead
  // of using a temporary object. This can prevent use-after-free issues since
  // pDesc may point to one of std::deque object's elements.
  std::deque<FX_FONTDESCRIPTOR> namedFonts =
      EnumGdiFonts(pszFontFamily, wUnicode);
  params.pwsFamily = nullptr;
  pDesc = MatchDefaultFont(&params, namedFonts);
  if (!pDesc) {
    return nullptr;
  }

  auto it = std::find(font_faces_.rbegin(), font_faces_.rend(), *pDesc);
  if (it != font_faces_.rend()) {
    return &*it;
  }

  font_faces_.push_back(*pDesc);
  return &font_faces_.back();
}
