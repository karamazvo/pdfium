// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_CONFIG_PDFIUM_CONFIG_H_
#define SKIA_CONFIG_PDFIUM_CONFIG_H_

// This is a SkFontMgr which will use FreeType to decode font data.
#define PDF_SKIA_FONT_MANAGER_HEADER \
  "third_party/skia/include/ports/SkFontMgr_empty.h"
#define PDF_SKIA_FONT_MANAGER SkFontMgr_New_Custom_Empty

#endif  // SKIA_CONFIG_PDFIUM_CONFIG_H_
