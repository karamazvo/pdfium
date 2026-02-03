// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_FONT_CFGAS_FONTMGR_DEFAULT_H_
#define XFA_FGAS_FONT_CFGAS_FONTMGR_DEFAULT_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#error "Built on wrong platform"
#endif

#include <array>
#include <map>
#include <memory>
#include <vector>

#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr_exclusion.h"
#include "core/fxcrt/widestring.h"
#include "xfa/fgas/font/cfgas_fontmgr.h"

class CFX_Face;
class CFX_ReadOnlyVectorStream;

class CFGAS_FontDescriptor {
 public:
  struct Rank {
    UNOWNED_PTR_EXCLUSION CFGAS_FontDescriptor* font;  // POD struct.
    int32_t nPenalty;

    bool operator>(const Rank& other) const {
      return nPenalty > other.nPenalty;
    }
    bool operator<(const Rank& other) const {
      return nPenalty < other.nPenalty;
    }
    friend inline bool operator==(const Rank& lhs, const Rank& rhs) {
      return lhs.nPenalty == rhs.nPenalty;
    }
  };

  CFGAS_FontDescriptor();
  ~CFGAS_FontDescriptor();

  bool VerifyUnicode(wchar_t unicode);

  int32_t face_index_ = 0;
  uint32_t font_styles_ = 0;
  WideString face_name_;
  RetainPtr<CFX_Face> face_;  // May be null until required.
  std::vector<WideString> family_names_;
  std::array<uint32_t, 4> usb_ = {};
  std::array<uint32_t, 2> csb_ = {};
};

class CFGAS_FontMgrDefault final : public CFGAS_FontMgr {
 public:
  CFGAS_FontMgrDefault();
  ~CFGAS_FontMgrDefault() override;

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
  friend class CFGASFontMgr_LazyEnumeration_Test;

  bool EnumFontsFromFontMapper();
  void RegisterFace(RetainPtr<CFX_Face> face,
                    int face_index,
                    const WideString& face_name);
  void RegisterFaces(const RetainPtr<CFX_ReadOnlyVectorStream>& font_stream,
                     const WideString& face_name);
  std::vector<CFGAS_FontDescriptor::Rank> MatchFonts(FX_CodePage wCodePage,
                                                     uint32_t dwFontStyles,
                                                     const WideString& FontName,
                                                     wchar_t wcUnicode);
  RetainPtr<CFGAS_GEFont> LoadFontInternal(const WideString& face_name,
                                           int32_t face_index);
  void EnsureFontsEnumerated();

  bool fonts_enumerated_ = false;
  std::vector<std::unique_ptr<CFGAS_FontDescriptor>> installed_fonts_;
  std::map<uint32_t, std::vector<CFGAS_FontDescriptor::Rank>>
      hash_2candidate_list_;
};

#endif  // XFA_FGAS_FONT_CFGAS_FONTMGR_DEFAULT_H_
