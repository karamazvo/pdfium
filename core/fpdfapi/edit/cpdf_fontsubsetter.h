// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FPDFAPI_EDIT_CPDF_FONTSUBSETTER_H_
#define CORE_FPDFAPI_EDIT_CPDF_FONTSUBSETTER_H_

#include <stdint.h>

#include <map>
#include <set>

#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_parser.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CPDF_Object;
class CPDF_Stream;
class CPDF_Dictionary;
class CPDF_TextObject;
class CPDF_Font;
class CPDF_Page;

class CPDF_FontSubsetter {
 public:
  CPDF_FontSubsetter(CPDF_Document* doc, CPDF_Parser* parser);
  ~CPDF_FontSubsetter();

  std::map<uint32_t, RetainPtr<const CPDF_Object>> GenerateObjectOverrides();

 private:
  struct SubsetCandidate {
    SubsetCandidate();
    ~SubsetCandidate();

    // The new font name after subsetting.
    ByteString subset_font_name;

    // PDF font-related objects that need to be overridden during the save.
    RetainPtr<const CPDF_Stream> font_stream;
    RetainPtr<const CPDF_Dictionary> root_font;
    RetainPtr<const CPDF_Dictionary> cid_font;
    RetainPtr<const CPDF_Dictionary> descriptor;

    // Mappings.
    std::set<uint32_t> used_gids;
    std::multimap<uint32_t, uint32_t> char_code_to_unicode;
  };

  void CollectNewObjectNums();

  void CollectSubsetCandidates();

  void CollectSubsetCandidatesFromPage(CPDF_Page* page);

  void AddUsedText(SubsetCandidate& candidate,
                   const CPDF_TextObject* text,
                   CPDF_Font* font);

  UnownedPtr<CPDF_Document> const doc_;
  UnownedPtr<CPDF_Parser> const parser_;

  // New PDF object numbers that were not in the original PDF.
  std::set<uint32_t> new_obj_nums_;

  // Mapping from font file streams to subset candidates.
  std::map<uint32_t, CPDF_FontSubsetter::SubsetCandidate> candidates_;
};

#endif  // CORE_FPDFAPI_EDIT_CPDF_FONTSUBSETTER_H_
