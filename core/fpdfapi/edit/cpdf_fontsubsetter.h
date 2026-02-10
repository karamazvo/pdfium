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
class CPDF_TextObject;
class CPDF_Font;
class CPDF_Page;

// CPDF_FontSubsetter generate object overrides for any text-related PDF objects
// in order to subset new embedded fonts. CPDF_FontSubsetter only creates new
// PDF objects and does not modify any existing PDF objects. It should be used
// during saving to create objects that should replace existing objects when
// writing the PDF.
class CPDF_FontSubsetter {
 public:
  explicit CPDF_FontSubsetter(CPDF_Document* doc);
  ~CPDF_FontSubsetter();

  // Given `new_obj_nums`, the set of all new object numbers, returns a map of
  // object numbers to new PDF objects that should override the original objects
  // in order to subset any embedded fonts for new text.
  std::map<uint32_t, RetainPtr<const CPDF_Object>> GenerateObjectOverrides(
      const std::set<uint32_t>& new_obj_nums);

 private:
  // Potential fonts that can be subsetted.
  struct SubsetCandidate {
    SubsetCandidate();
    ~SubsetCandidate();

    // PDF font-related objects that need to be overridden during the save.
    // TODO(crbug.com/476127152): Override the root font, CID font, and
    // descriptor.
    RetainPtr<const CPDF_Stream> font_stream;

    // The set of GIDs used by text.
    std::set<uint32_t> used_gids;
  };

  // Gets the subset candidates from all pages.
  void CollectSubsetCandidates();

  // Gets the subset candidates from `page`.
  void CollectSubsetCandidatesFromPage(CPDF_Page* page);

  // Adds the characters used in `text` with `font` to `candidate`.
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
