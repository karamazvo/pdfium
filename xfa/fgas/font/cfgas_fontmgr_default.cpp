// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/font/cfgas_fontmgr_default.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#error "Built on wrong platform"
#endif

#include <stdint.h>

#include <algorithm>
#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "core/fxcrt/byteorder.h"
#include "core/fxcrt/cfx_read_only_vector_stream.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxge/cfx_font.h"
#include "core/fxge/cfx_fontmapper.h"
#include "core/fxge/cfx_fontmgr.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/fx_fontencoding.h"
#include "xfa/fgas/font/cfgas_gefont.h"
#include "xfa/fgas/font/fgas_fontutils.h"

namespace {

constexpr auto kCodePages =
    std::to_array<const FX_CodePage>({FX_CodePage::kMSWin_WesternEuropean,
                                      FX_CodePage::kMSWin_EasternEuropean,
                                      FX_CodePage::kMSWin_Cyrillic,
                                      FX_CodePage::kMSWin_Greek,
                                      FX_CodePage::kMSWin_Turkish,
                                      FX_CodePage::kMSWin_Hebrew,
                                      FX_CodePage::kMSWin_Arabic,
                                      FX_CodePage::kMSWin_Baltic,
                                      FX_CodePage::kMSWin_Vietnamese,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kMSDOS_Thai,
                                      FX_CodePage::kShiftJIS,
                                      FX_CodePage::kChineseSimplified,
                                      FX_CodePage::kHangul,
                                      FX_CodePage::kChineseTraditional,
                                      FX_CodePage::kJohab,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kDefANSI,
                                      FX_CodePage::kMSDOS_Greek2,
                                      FX_CodePage::kMSDOS_Russian,
                                      FX_CodePage::kMSDOS_Norwegian,
                                      FX_CodePage::kMSDOS_Arabic,
                                      FX_CodePage::kMSDOS_FrenchCanadian,
                                      FX_CodePage::kMSDOS_Hebrew,
                                      FX_CodePage::kMSDOS_Icelandic,
                                      FX_CodePage::kMSDOS_Portuguese,
                                      FX_CodePage::kMSDOS_Turkish,
                                      FX_CodePage::kMSDOS_Cyrillic,
                                      FX_CodePage::kMSDOS_EasternEuropean,
                                      FX_CodePage::kMSDOS_Baltic,
                                      FX_CodePage::kMSDOS_Greek1,
                                      FX_CodePage::kArabic_ASMO708,
                                      FX_CodePage::kMSDOS_WesternEuropean,
                                      FX_CodePage::kMSDOS_US});

uint16_t FX_GetCodePageBit(FX_CodePage wCodePage) {
  for (size_t i = 0; i < kCodePages.size(); ++i) {
    if (kCodePages[i] == wCodePage) {
      return static_cast<uint16_t>(i);
    }
  }
  return static_cast<uint16_t>(-1);
}

uint16_t FX_GetUnicodeBit(wchar_t wcUnicode) {
  const FGAS_FONTUSB* x = FGAS_GetUnicodeBitField(wcUnicode);
  return x ? x->wBitField : FGAS_FONTUSB::kNoBitField;
}

uint16_t ReadUInt16FromSpanAtOffset(pdfium::span<const uint8_t> data,
                                    size_t offset) {
  return fxcrt::GetUInt16MSBFirst(data.subspan(offset).first<2u>());
}

std::vector<WideString> GetNames(pdfium::span<const uint8_t> name_table) {
  std::vector<WideString> results;
  if (name_table.empty()) {
    return results;
  }

  uint16_t nNameCount = ReadUInt16FromSpanAtOffset(name_table, 2);
  pdfium::span<const uint8_t> str =
      name_table.subspan(ReadUInt16FromSpanAtOffset(name_table, 4));
  pdfium::span<const uint8_t> name_record = name_table.subspan<6u>();
  for (uint16_t i = 0; i < nNameCount; ++i) {
    uint16_t nNameID = ReadUInt16FromSpanAtOffset(name_table, i * 12 + 6);
    if (nNameID != 1) {
      continue;
    }

    uint16_t nPlatformID = ReadUInt16FromSpanAtOffset(name_record, i * 12);
    uint16_t nNameLength = ReadUInt16FromSpanAtOffset(name_record, i * 12 + 8);
    uint16_t nNameOffset = ReadUInt16FromSpanAtOffset(name_record, i * 12 + 10);
    if (nPlatformID != 1) {
      WideString wsFamily;
      for (uint16_t j = 0; j < nNameLength / 2; ++j) {
        wchar_t wcTemp = ReadUInt16FromSpanAtOffset(str, nNameOffset + j * 2);
        wsFamily += wcTemp;
      }
      results.push_back(wsFamily);
      continue;
    }

    // Avoid out of bounds crashes if the length and/or offset are wrong.
    if (static_cast<size_t>(nNameLength) + nNameOffset >= str.size()) {
      continue;
    }

    WideString wsFamily;
    for (uint16_t j = 0; j < nNameLength; ++j) {
      wchar_t wcTemp = str[nNameOffset + j];
      wsFamily += wcTemp;
    }
    results.push_back(wsFamily);
  }
  return results;
}

RetainPtr<CFX_ReadOnlyVectorStream> CreateFontStream(
    CFX_FontMapper* font_mapper,
    size_t index) {
  FixedSizeDataVector<uint8_t> buffer = font_mapper->RawBytesForIndex(index);
  if (buffer.empty()) {
    return nullptr;
  }
  return pdfium::MakeRetain<CFX_ReadOnlyVectorStream>(std::move(buffer));
}

RetainPtr<CFX_ReadOnlyVectorStream> CreateFontStream(
    const ByteString& bsFaceName) {
  CFX_FontMgr* font_mgr = CFX_GEModule::Get()->GetFontMgr();
  CFX_FontMapper* font_mapper = font_mgr->GetBuiltinMapper();
  font_mapper->LoadInstalledFonts();

  for (size_t i = 0; i < font_mapper->GetFaceSize(); ++i) {
    if (font_mapper->GetFaceName(i) == bsFaceName) {
      return CreateFontStream(font_mapper, i);
    }
  }
  return nullptr;
}

bool IsPartName(const WideString& name1, const WideString& name2) {
  return name1.Contains(name2.AsStringView());
}

int32_t CalcPenalty(CFGAS_FontDescriptor* pInstalled,
                    FX_CodePage wCodePage,
                    uint32_t dwFontStyles,
                    const WideString& FontName,
                    wchar_t wcUnicode) {
  int32_t nPenalty = 30000;
  if (FontName.GetLength() != 0) {
    if (FontName != pInstalled->face_name_) {
      size_t i;
      for (i = 0; i < pInstalled->family_names_.size(); ++i) {
        if (pInstalled->family_names_[i] == FontName) {
          break;
        }
      }
      if (i == pInstalled->family_names_.size()) {
        nPenalty += 0xFFFF;
      } else {
        nPenalty -= 28000;
      }
    } else {
      nPenalty -= 30000;
    }
    if (nPenalty == 30000 && !IsPartName(pInstalled->face_name_, FontName)) {
      size_t i;
      for (i = 0; i < pInstalled->family_names_.size(); i++) {
        if (IsPartName(pInstalled->family_names_[i], FontName)) {
          break;
        }
      }
      if (i == pInstalled->family_names_.size()) {
        nPenalty += 0xFFFF;
      } else {
        nPenalty -= 26000;
      }
    } else {
      nPenalty -= 27000;
    }
  }
  uint32_t dwStyleMask = pInstalled->font_styles_ ^ dwFontStyles;
  if (FontStyleIsForceBold(dwStyleMask)) {
    nPenalty += 4500;
  }
  if (FontStyleIsFixedPitch(dwStyleMask)) {
    nPenalty += 10000;
  }
  if (FontStyleIsItalic(dwStyleMask)) {
    nPenalty += 10000;
  }
  if (FontStyleIsSerif(dwStyleMask)) {
    nPenalty += 500;
  }
  if (FontStyleIsSymbolic(dwStyleMask)) {
    nPenalty += 0xFFFF;
  }
  if (nPenalty >= 0xFFFF) {
    return 0xFFFF;
  }

  uint16_t wBit =
      (wCodePage == FX_CodePage::kDefANSI || wCodePage == FX_CodePage::kFailure)
          ? static_cast<uint16_t>(-1)
          : FX_GetCodePageBit(wCodePage);
  if (wBit != static_cast<uint16_t>(-1)) {
    DCHECK(wBit < 64);
    if ((pInstalled->csb_[wBit / 32] & (1 << (wBit % 32))) == 0) {
      nPenalty += 0xFFFF;
    } else {
      nPenalty -= 60000;
    }
  }
  wBit = (wcUnicode == 0 || wcUnicode == 0xFFFE) ? FGAS_FONTUSB::kNoBitField
                                                 : FX_GetUnicodeBit(wcUnicode);
  if (wBit != FGAS_FONTUSB::kNoBitField) {
    DCHECK(wBit < 128);
    if ((pInstalled->usb_[wBit / 32] & (1 << (wBit % 32))) == 0) {
      nPenalty += 0xFFFF;
    } else {
      nPenalty -= 60000;
    }
  }
  return nPenalty;
}

}  // namespace

CFGAS_FontDescriptor::CFGAS_FontDescriptor() = default;

CFGAS_FontDescriptor::~CFGAS_FontDescriptor() = default;

bool CFGAS_FontDescriptor::VerifyUnicode(wchar_t unicode) {
  if (!face_) {
    RetainPtr<CFX_ReadOnlyVectorStream> pFileRead =
        CreateFontStream(face_name_.ToUTF8());
    if (!pFileRead) {
      return false;
    }
    RetainPtr<CFX_Face> ft_face = CFX_Face::NewFromVectorStream(
        CFX_GEModule::Get()->GetFontMgr(), pFileRead, face_index_);
    if (!ft_face) {
      return false;
    }
    face_ = std::move(ft_face);
  }
  return face_->SelectCharMap(fxge::FontEncoding::kUnicode) &&
         face_->GetCharIndex(unicode);
}

CFGAS_FontMgrDefault::CFGAS_FontMgrDefault() = default;

CFGAS_FontMgrDefault::~CFGAS_FontMgrDefault() = default;

void CFGAS_FontMgrDefault::EnsureFontsEnumerated() {
  if (fonts_enumerated_) {
    return;
  }
  fonts_enumerated_ = true;
  EnumFontsFromFontMapper();
}

bool CFGAS_FontMgrDefault::EnumFontsFromFontMapper() {
  CFX_FontMapper* font_mapper =
      CFX_GEModule::Get()->GetFontMgr()->GetBuiltinMapper();
  font_mapper->LoadInstalledFonts();

  for (size_t i = 0; i < font_mapper->GetFaceSize(); ++i) {
    RetainPtr<CFX_ReadOnlyVectorStream> font_stream =
        CreateFontStream(font_mapper, i);
    if (!font_stream) {
      continue;
    }

    WideString face_name =
        WideString::FromDefANSI(font_mapper->GetFaceName(i).AsStringView());
    RegisterFaces(font_stream, face_name);
  }

  return !installed_fonts_.empty();
}

bool CFGAS_FontMgrDefault::EnumFonts() {
  return EnumFontsFromFontMapper();
}

RetainPtr<CFGAS_GEFont> CFGAS_FontMgrDefault::GetFontByCodePage(
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

  if (!pdfium::Contains(hash_2candidate_list_, dwHash)) {
    hash_2candidate_list_[dwHash] =
        MatchFonts(wCodePage, dwFontStyles, WideString(pszFontFamily), 0);
  }
  if (hash_2candidate_list_[dwHash].empty()) {
    return nullptr;
  }

  CFGAS_FontDescriptor* pDesc = hash_2candidate_list_[dwHash].front().font;
  RetainPtr<CFGAS_GEFont> font =
      LoadFontInternal(pDesc->face_name_, pDesc->face_index_);

  if (!font) {
    return nullptr;
  }

  font->SetLogicalFontStyle(dwFontStyles);
  font_vector->push_back(font);
  return font;
}

RetainPtr<CFGAS_GEFont> CFGAS_FontMgrDefault::LoadFont(
    const wchar_t* pszFontFamily,
    uint32_t dwFontStyles,
    FX_CodePage wCodePage) {
  return GetFontByCodePage(wCodePage, dwFontStyles, pszFontFamily);
}

RetainPtr<CFGAS_GEFont> CFGAS_FontMgrDefault::GetFontByUnicodeImpl(
    wchar_t wUnicode,
    uint32_t dwFontStyles,
    const wchar_t* pszFontFamily,
    uint32_t dwHash,
    FX_CodePage wCodePage,
    uint16_t /* wBitField*/) {
  if (!pdfium::Contains(hash_2candidate_list_, dwHash)) {
    hash_2candidate_list_[dwHash] =
        MatchFonts(wCodePage, dwFontStyles, pszFontFamily, wUnicode);
  }
  for (const auto& info : hash_2candidate_list_[dwHash]) {
    CFGAS_FontDescriptor* pDesc = info.font;
    if (!pDesc->VerifyUnicode(wUnicode)) {
      continue;
    }
    RetainPtr<CFGAS_GEFont> font =
        LoadFontInternal(pDesc->face_name_, pDesc->face_index_);
    if (!font) {
      continue;
    }
    font->SetLogicalFontStyle(dwFontStyles);
    hash_2fonts_[dwHash].push_back(font);
    return font;
  }
  if (!pszFontFamily) {
    failed_unicodes_set_.insert(wUnicode);
  }
  return nullptr;
}

RetainPtr<CFGAS_GEFont> CFGAS_FontMgrDefault::LoadFontInternal(
    const WideString& face_name,
    int32_t face_index) {
  RetainPtr<CFX_ReadOnlyVectorStream> font_stream =
      CreateFontStream(face_name.ToUTF8());
  if (!font_stream) {
    return nullptr;
  }
  auto internal_font = std::make_unique<CFX_Font>();
  if (!internal_font->LoadFromVectorStream(font_stream, face_index)) {
    return nullptr;
  }
  return CFGAS_GEFont::LoadFont(std::move(internal_font));
}

std::vector<CFGAS_FontDescriptor::Rank> CFGAS_FontMgrDefault::MatchFonts(
    FX_CodePage wCodePage,
    uint32_t dwFontStyles,
    const WideString& FontName,
    wchar_t wcUnicode) {
  EnsureFontsEnumerated();
  std::vector<CFGAS_FontDescriptor::Rank> matched_fonts;
  for (const auto& font : installed_fonts_) {
    int32_t nPenalty =
        CalcPenalty(font.get(), wCodePage, dwFontStyles, FontName, wcUnicode);
    if (nPenalty >= 0xffff) {
      continue;
    }
    matched_fonts.push_back({font.get(), nPenalty});
    if (matched_fonts.size() == 0xffff) {
      break;
    }
  }
  std::stable_sort(matched_fonts.begin(), matched_fonts.end());
  return matched_fonts;
}

void CFGAS_FontMgrDefault::RegisterFace(RetainPtr<CFX_Face> face,
                                        int face_index,
                                        const WideString& wsFaceName) {
  if (!face->IsScalable()) {
    return;
  }

  auto font = std::make_unique<CFGAS_FontDescriptor>();
  font->font_styles_ |= face->GetFontStyle();

  std::optional<std::array<uint32_t, 4>> unicode_range =
      face->GetOs2UnicodeRange();
  if (unicode_range.has_value()) {
    fxcrt::Copy(unicode_range.value(), font->usb_);
  }

  std::optional<std::array<uint32_t, 2>> code_page_range =
      face->GetOs2CodePageRange();
  if (code_page_range.has_value()) {
    fxcrt::Copy(code_page_range.value(), font->csb_);
  }

  static constexpr uint32_t kNameTag =
      CFX_FontMapper::MakeTag('n', 'a', 'm', 'e');

  DataVector<uint8_t> table;
  size_t table_size = face->GetSfntTable(kNameTag, table);
  if (table_size) {
    table.resize(table_size);
    if (!face->GetSfntTable(kNameTag, table)) {
      table.clear();
    }
  }
  font->family_names_ = GetNames(table);
  font->family_names_.push_back(
      WideString::FromUTF8(face->GetFamilyName().AsStringView()));
  font->face_name_ = wsFaceName;
  font->face_index_ = face_index;
  installed_fonts_.push_back(std::move(font));
}

void CFGAS_FontMgrDefault::RegisterFaces(
    const RetainPtr<CFX_ReadOnlyVectorStream>& font_stream,
    const WideString& face_name) {
  int index = 0;
  int num_faces = 0;
  do {
    RetainPtr<CFX_Face> face = CFX_Face::NewFromVectorStream(
        CFX_GEModule::Get()->GetFontMgr(), font_stream, index);
    if (!face) {
      ++index;
      continue;
    }
    // All faces keep number of faces. It can be retrieved from any one face.
    if (num_faces == 0) {
      num_faces = face->GetNumFaces();
    }
    RegisterFace(face, index, face_name);
    ++index;
  } while (index < num_faces);
}
