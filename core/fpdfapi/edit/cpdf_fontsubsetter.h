// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FPDFAPI_EDIT_CPDF_FONTSUBSETTER_H_
#define CORE_FPDFAPI_EDIT_CPDF_FONTSUBSETTER_H_

#include <stdint.h>
#include <map>
#include <set>
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_object.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CPDF_Page;

class CPDF_FontSubsetter {
 public:
  explicit CPDF_FontSubsetter(CPDF_Document* doc);
  ~CPDF_FontSubsetter();

  std::map<uint32_t, RetainPtr<CPDF_Object>>
  GenerateFontSubsetObjectOverrides();

 private:
  struct SubsetCandidate {
    SubsetCandidate();
    ~SubsetCandidate();

    RetainPtr<const CPDF_Stream> stream;
    RetainPtr<const CPDF_Dictionary> descriptor;
    std::set<uint32_t> used_gids;
  };

  void AddTextAndFonts(CPDF_Page* page);

  UnownedPtr<CPDF_Document> const doc_;

  // Stream object number to font data
  std::map<uint32_t, SubsetCandidate> candidates_;
};

#endif  // CORE_FPDFAPI_EDIT_CPDF_FONTSUBSETTER_H_
