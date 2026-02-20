// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_font_util.h"

#include <stddef.h>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_extension.h"

namespace fxge {

namespace {

bool IsStrUpper(const ByteString& str) {
  for (char ch : str) {
    if (!FXSYS_IsUpperASCII(ch)) {
      return false;
    }
  }
  return true;
}
}  // namespace

void MaybeRemoveSubsettedFontPrefix(ByteString& font_name) {
  static constexpr size_t kPrefixLength = 6;
  if (font_name.GetLength() > kPrefixLength &&
      font_name[kPrefixLength] == '+' &&
      IsStrUpper(font_name.First(kPrefixLength))) {
    font_name = font_name.Substr(kPrefixLength + 1);
  }
}

}  // namespace fxge
