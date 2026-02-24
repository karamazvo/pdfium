// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/cfx_alternative_charmap_selector.h"

#include <memory>

#include "core/fxcrt/ptr_util.h"
#include "core/fxge/cfx_face.h"
#include "core/fxge/cfx_font.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/fx_fontencoding.h"

namespace {

constexpr fxge::FontEncoding kEncodingIDs[] = {
    fxge::FontEncoding::kSymbol,      fxge::FontEncoding::kUnicode,
    fxge::FontEncoding::kSjis,        fxge::FontEncoding::kGB2312,
    fxge::FontEncoding::kBig5,        fxge::FontEncoding::kWansung,
    fxge::FontEncoding::kJohab,       fxge::FontEncoding::kAdobeStandard,
    fxge::FontEncoding::kAdobeExpert, fxge::FontEncoding::kAdobeCustom,
    fxge::FontEncoding::kLatin1,      fxge::FontEncoding::kOldLatin2,
    fxge::FontEncoding::kAppleRoman,
};

}  // namespace

CFX_AlternativeCharmapSelector::CFX_AlternativeCharmapSelector(
    CFX_Font* font,
    fxge::FontEncoding encoding_id)
    : font_(font), encoding_id_(encoding_id) {}

CFX_AlternativeCharmapSelector::~CFX_AlternativeCharmapSelector() = default;

uint32_t CFX_AlternativeCharmapSelector::GlyphFromCharCode(uint32_t charcode) {
  RetainPtr<CFX_Face> face = font_->GetFace();
  uint32_t char_index = face->GetCharIndex(charcode);
  if (char_index > 0) {
    return char_index;
  }

  size_t map_index = 0;
  while (map_index < face->GetCharMapCount()) {
    fxge::FontEncoding encoding_id =
        face->GetCharMapEncodingByIndex(map_index++);
    if (encoding_id_ == encoding_id) {
      continue;
    }
    if (!face->SelectCharMap(encoding_id)) {
      continue;
    }
    char_index = face->GetCharIndex(charcode);
    if (char_index > 0) {
      encoding_id_ = encoding_id;
      return char_index;
    }
  }
  face->SelectCharMap(encoding_id_);
  return 0;
}

uint32_t CFX_AlternativeCharmapSelector::CharCodeFromUnicode(
    wchar_t Unicode) const {
  if (encoding_id_ == fxge::FontEncoding::kUnicode ||
      encoding_id_ == fxge::FontEncoding::kSymbol) {
    return Unicode;
  }
  RetainPtr<CFX_Face> face = font_->GetFace();
  for (size_t i = 0; i < face->GetCharMapCount(); i++) {
    fxge::FontEncoding encoding_id = face->GetCharMapEncodingByIndex(i);
    if (encoding_id == fxge::FontEncoding::kUnicode ||
        encoding_id == fxge::FontEncoding::kSymbol) {
      return Unicode;
    }
  }
  return kInvalidCharCode;
}

// static
std::unique_ptr<CFX_AlternativeCharmapSelector>
CFX_AlternativeCharmapSelector::Create(CFX_Font* font) {
  if (!font || !font->GetFace()) {
    return nullptr;
  }
  for (fxge::FontEncoding id : kEncodingIDs) {
    if (font->GetFace()->SelectCharMap(id)) {
      // Private ctor.
      return pdfium::WrapUnique(new CFX_AlternativeCharmapSelector(font, id));
    }
  }
  return nullptr;
}
