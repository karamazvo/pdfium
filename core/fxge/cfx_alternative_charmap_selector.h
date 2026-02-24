// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_ALTERNATIVE_CHARMAP_SELECTOR_H_
#define CORE_FXGE_CFX_ALTERNATIVE_CHARMAP_SELECTOR_H_

#include <stdint.h>

#include <memory>

#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/cfx_unicode_charmap_selector.h"
#include "core/fxge/fx_fontencoding.h"

class CFX_Font;

class CFX_AlternativeCharmapSelector {
 public:
  static constexpr uint32_t kInvalidCharCode = static_cast<uint32_t>(-1);

  static std::unique_ptr<CFX_AlternativeCharmapSelector> Create(CFX_Font* font);

  ~CFX_AlternativeCharmapSelector();

  uint32_t GlyphFromCharCode(uint32_t charcode);

  // Returns |kInvalidCharCode| on error.
  uint32_t CharCodeFromUnicode(wchar_t Unicode) const;

 private:
  CFX_AlternativeCharmapSelector(CFX_Font* font,
                                 fxge::FontEncoding encoding_id);

  UnownedPtr<const CFX_Font> const font_;
  fxge::FontEncoding encoding_id_;  // Last encoding charcode matched against.
};

#endif  // CORE_FXGE_CFX_ALTERNATIVE_CHARMAP_SELECTOR_H_
