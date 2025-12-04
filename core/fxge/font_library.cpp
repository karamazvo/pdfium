// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/font_library.h"

#include <memory>

#include "core/fxcrt/notreached.h"
#include "core/fxge/freetype/fx_freetype.h"

namespace {

// Can be overridden at runtime.
FontLibrary::Type g_font_library_type = FontLibrary::kDefaultType;

}  // namespace

// static
std::unique_ptr<FontLibrary> FontLibrary::Create() {
  switch (g_font_library_type) {
    case Type::kFreeType:
      return std::make_unique<FreeTypeFontLibrary>();
#if defined(PDF_ENABLE_FONTATIONS)
    case Type::kFontations:
      NOTREACHED();  // Actually not implemented.
#endif
  }
  NOTREACHED();
}

// static
void FontLibrary::SetType(Type type) {
  g_font_library_type = type;
}
