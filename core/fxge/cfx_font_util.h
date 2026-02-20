// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_CFX_FONT_UTIL_H_
#define CORE_FXGE_CFX_FONT_UTIL_H_

namespace fxcrt {
class ByteString;
}

namespace fxge {

// Removes the "XXXXXX+" prefix from a subsetted font name if present. The
// prefix must be 6 uppercase ASCII letters followed by a '+'.
void MaybeRemoveSubsettedFontPrefix(fxcrt::ByteString& font_name);

}  // namespace fxge

#endif  // CORE_FXGE_CFX_FONT_UTIL_H_
