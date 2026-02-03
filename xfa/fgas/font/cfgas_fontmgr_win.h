// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_FONT_CFGAS_FONTMGR_WIN_H_
#define XFA_FGAS_FONT_CFGAS_FONTMGR_WIN_H_

#include "build/build_config.h"

#if !BUILDFLAG(IS_WIN)
#error "Built on wrong platform"
#endif

#include <array>
#include <deque>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_codepage.h"
#include "xfa/fgas/font/cfgas_fontmgr.h"

struct FX_FONTSIGNATURE {
  std::array<uint32_t, 4> fsUsb;
  std::array<uint32_t, 2> fsCsb;
};

inline bool operator==(const FX_FONTSIGNATURE& left,
                       const FX_FONTSIGNATURE& right) {
  return left.fsUsb[0] == right.fsUsb[0] && left.fsUsb[1] == right.fsUsb[1] &&
         left.fsUsb[2] == right.fsUsb[2] && left.fsUsb[3] == right.fsUsb[3] &&
         left.fsCsb[0] == right.fsCsb[0] && left.fsCsb[1] == right.fsCsb[1];
}

struct FX_FONTDESCRIPTOR {
  wchar_t wsFontFace[32];
  uint32_t dwFontStyles;
  FX_Charset uCharSet;
  FX_FONTSIGNATURE FontSignature;
};

inline bool operator==(const FX_FONTDESCRIPTOR& left,
                       const FX_FONTDESCRIPTOR& right) {
  return left.uCharSet == right.uCharSet &&
         left.dwFontStyles == right.dwFontStyles &&
         left.FontSignature == right.FontSignature &&
         UNSAFE_TODO(wcscmp(left.wsFontFace, right.wsFontFace)) == 0;
}

class CFGAS_FontMgrWin final : public CFGAS_FontMgr {
 public:
  CFGAS_FontMgrWin();
  ~CFGAS_FontMgrWin() override;

  // CFGAS_FontMgr:
  bool EnumFonts() override;
  RetainPtr<CFGAS_GEFont> GetFontByCodePage(
      FX_CodePage wCodePage,
      uint32_t dwFontStyles,
      const wchar_t* pszFontFamily) override;
  RetainPtr<CFGAS_GEFont> LoadFont(const wchar_t* pszFontFamily,
                                   uint32_t dwFontStyles,
                                   FX_CodePage wCodePage) override;

 protected:
  RetainPtr<CFGAS_GEFont> GetFontByUnicodeImpl(wchar_t wUnicode,
                                               uint32_t dwFontStyles,
                                               const wchar_t* pszFontFamily,
                                               uint32_t dwHash,
                                               FX_CodePage wCodePage,
                                               uint16_t wBitField) override;

 private:
  const FX_FONTDESCRIPTOR* FindFont(const wchar_t* pszFontFamily,
                                    uint32_t dwFontStyles,
                                    bool matchParagraphStyle,
                                    FX_CodePage wCodePage,
                                    uint32_t dwUSB,
                                    wchar_t wUnicode);

  std::deque<FX_FONTDESCRIPTOR> font_faces_;
};

#endif  // XFA_FGAS_FONT_CFGAS_FONTMGR_WIN_H_
