// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/font/cfgas_fontdescriptor.h"

#include <ranges>
#include <utility>

#include "core/fxcrt/byteorder.h"
#include "core/fxcrt/cfx_read_only_vector_stream.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxge/cfx_face.h"
#include "core/fxge/cfx_fontmapper.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/fx_fontencoding.h"

namespace {

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

}  // namespace

CFGAS_FontDescriptor::CFGAS_FontDescriptor() = default;

CFGAS_FontDescriptor::~CFGAS_FontDescriptor() = default;

// static
std::unique_ptr<CFGAS_FontDescriptor> CFGAS_FontDescriptor::CreateFromStream(
    const RetainPtr<CFX_ReadOnlyVectorStream>& font_stream,
    int face_index,
    const WideString& face_name) {
  RetainPtr<CFX_Face> face = CFX_Face::NewFromVectorStream(
      CFX_GEModule::Get()->GetFontMgr(), font_stream, face_index);
  if (!face->IsScalable()) {
    return nullptr;
  }

  auto font = std::make_unique<CFGAS_FontDescriptor>();
  font->font_styles_ |= face->GetFontStyle();

  std::optional<std::array<uint32_t, 4>> unicode_range =
      face->GetOs2UnicodeRange();
  if (unicode_range.has_value()) {
    fxcrt::Copy(unicode_range.value(), font->usb_);
  } else {
    std::ranges::fill(font->usb_, 0);
  }

  std::optional<std::array<uint32_t, 2>> code_page_range =
      face->GetOs2CodePageRange();
  if (code_page_range.has_value()) {
    fxcrt::Copy(code_page_range.value(), font->csb_);
  } else {
    std::ranges::fill(font->csb_, 0);
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
  font->face_name_ = face_name;
  font->face_index_ = face_index;

  return font;
}

int CFGAS_FontDescriptor::GetNumFaces() const {
  return face_->GetNumFaces();
}

bool CFGAS_FontDescriptor::VerifyUnicodeForFontDescriptor(wchar_t unicode) {
  if (!face_) {
    RetainPtr<CFX_ReadOnlyVectorStream> file_read =
        CFX_FontMapper::CreateFontStream(face_name_.ToUTF8());
    if (!file_read) {
      return false;
    }
    RetainPtr<CFX_Face> ft_face = CFX_Face::NewFromVectorStream(
        CFX_GEModule::Get()->GetFontMgr(), file_read, face_index_);
    if (!ft_face) {
      return false;
    }
    face_ = std::move(ft_face);
  }
  return face_->SelectCharMap(fxge::FontEncoding::kUnicode) &&
         face_->GetCharIndex(unicode);
}
