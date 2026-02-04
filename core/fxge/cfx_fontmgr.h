// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_FONTMGR_H_
#define CORE_FXGE_CFX_FONTMGR_H_

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <map>
#include <memory>
#include <tuple>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxcrt/observed_ptr.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxge/cfx_face.h"
#include "core/fxge/cfx_systemfontcache.h"
#include "core/fxge/freetype/fx_freetype.h"

class CFX_FontMapper;

class CFX_FontMgr : public CFX_SystemFontCache {
 public:
  // `index` must be less than `CFX_FontMapper::kNumStandardFonts`.
  static pdfium::span<const uint8_t> GetStandardFont(size_t index);
  static pdfium::span<const uint8_t> GetGenericSansFont();
  static pdfium::span<const uint8_t> GetGenericSerifFont();

  CFX_FontMgr();
  ~CFX_FontMgr();

  RetainPtr<CFX_Face> NewFixedFace(RetainPtr<Entry> desc,
                                   pdfium::span<const uint8_t> span,
                                   uint32_t face_index);

  // Always present.
  CFX_FontMapper* GetBuiltinMapper() const { return builtin_mapper_.get(); }

  FXFT_LibraryRec* GetFTLibrary() const { return ft_library_.get(); }
  bool FTLibrarySupportsHinting() const { return ft_library_supports_hinting_; }

 private:
  // Must come before |builtin_mapper_| and |face_map_|.
  ScopedFXFTLibraryRec const ft_library_;
  std::unique_ptr<CFX_FontMapper> builtin_mapper_;
  const bool ft_library_supports_hinting_;
};

#endif  // CORE_FXGE_CFX_FONTMGR_H_
