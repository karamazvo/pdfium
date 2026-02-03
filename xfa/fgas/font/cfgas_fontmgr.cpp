// Copyright 2015 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/font/cfgas_fontmgr.h"

#include <stdint.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "build/build_config.h"
#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/stl_util.h"
#include "xfa/fgas/font/cfgas_gefont.h"
#include "xfa/fgas/font/fgas_fontutils.h"

#if BUILDFLAG(IS_WIN)
#include "xfa/fgas/font/cfgas_fontmgr_win.h"
#else
#include "xfa/fgas/font/cfgas_fontmgr_default.h"
#endif

uint32_t ShortFormHash(FX_CodePage wCodePage,
                       uint32_t dwFontStyles,
                       WideStringView wsFontFamily) {
  ByteString bsHash = ByteString::Format("%d, %d", wCodePage, dwFontStyles);
  bsHash += FX_UTF8Encode(wsFontFamily);
  return FX_HashCode_GetA(bsHash.AsStringView());
}

namespace {

uint32_t LongFormHash(FX_CodePage wCodePage,
                      uint16_t wBitField,
                      uint32_t dwFontStyles,
                      WideStringView wsFontFamily) {
  ByteString bsHash =
      ByteString::Format("%d, %d, %d", wCodePage, wBitField, dwFontStyles);
  bsHash += FX_UTF8Encode(wsFontFamily);
  return FX_HashCode_GetA(bsHash.AsStringView());
}

}  // namespace

// static
std::unique_ptr<CFGAS_FontMgr> CFGAS_FontMgr::Create() {
#if BUILDFLAG(IS_WIN)
  return std::make_unique<CFGAS_FontMgrWin>();
#else
  return std::make_unique<CFGAS_FontMgrDefault>();
#endif
}

CFGAS_FontMgr::CFGAS_FontMgr() = default;

CFGAS_FontMgr::~CFGAS_FontMgr() = default;

RetainPtr<CFGAS_GEFont> CFGAS_FontMgr::GetFontByUnicode(
    wchar_t wUnicode,
    uint32_t dwFontStyles,
    const wchar_t* pszFontFamily) {
  if (pdfium::Contains(failed_unicodes_set_, wUnicode)) {
    return nullptr;
  }

  const FGAS_FONTUSB* x = FGAS_GetUnicodeBitField(wUnicode);
  FX_CodePage wCodePage = x ? x->wCodePage : FX_CodePage::kFailure;
  uint16_t wBitField = x ? x->wBitField : FGAS_FONTUSB::kNoBitField;
  uint32_t dwHash =
      wCodePage == FX_CodePage::kFailure
          ? LongFormHash(wCodePage, wBitField, dwFontStyles, pszFontFamily)
          : ShortFormHash(wCodePage, dwFontStyles, pszFontFamily);
  for (auto& font : hash_2fonts_[dwHash]) {
    if (font->VerifyUnicode(wUnicode)) {
      return font;
    }
  }
  return GetFontByUnicodeImpl(wUnicode, dwFontStyles, pszFontFamily, dwHash,
                              wCodePage, wBitField);
}
