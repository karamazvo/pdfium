// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_UNICODE_CHARMAP_SELECTOR_H_
#define CORE_FXGE_CFX_UNICODE_CHARMAP_SELECTOR_H_

#include <stdint.h>

#include "core/fxcrt/unowned_ptr.h"

class CFX_Font;

class CFX_UnicodeCharmapSelector {
 public:
  explicit CFX_UnicodeCharmapSelector(const CFX_Font* font);
  virtual ~CFX_UnicodeCharmapSelector();

  virtual uint32_t GlyphFromCharCode(uint32_t charcode);

 protected:
  UnownedPtr<const CFX_Font> const font_;
};

#endif  // CORE_FXGE_CFX_UNICODE_CHARMAP_SELECTOR_H_
