// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/edit/cpdf_fontsubsetter.h"

#include <hb-subset.h>
#include <hb.h>

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/page/cpdf_textobject.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_parser.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"

namespace {

template <auto DestroyFunction>
struct HbDeleter {
  template <typename T>
  void operator()(T* ptr) const {
    DestroyFunction(ptr);
  }
};

using HBBlob = std::unique_ptr<hb_blob_t, HbDeleter<hb_blob_destroy>>;
using HBFace = std::unique_ptr<hb_face_t, HbDeleter<hb_face_destroy>>;
using HBSubsetInput =
    std::unique_ptr<hb_subset_input_t, HbDeleter<hb_subset_input_destroy>>;

std::vector<uint8_t> GenerateFontSubset(CPDF_Document* doc,
                                        pdfium::span<const uint8_t> font_data,
                                        const std::set<uint32_t>& gids) {
  // Wrap the data.
  HBBlob blob(
      hb_blob_create_or_fail(reinterpret_cast<const char*>(font_data.data()),
                             static_cast<uint32_t>(font_data.size()),
                             HB_MEMORY_MODE_READONLY, nullptr, nullptr));
  if (!blob) {
    return {};
  }

  HBSubsetInput input(hb_subset_input_create_or_fail());
  if (!input) {
    return {};
  }

  hb_subset_input_set_flags(input.get(), HB_SUBSET_FLAGS_RETAIN_GIDS);

  hb_set_t* glyphs = hb_subset_input_glyph_set(input.get());
  for (uint32_t gid : gids) {
    hb_set_add(glyphs, gid);
  }
  hb_set_add(glyphs, 0);

  HBFace face(hb_face_create(blob.get(), 0));
  HBFace subset_face(hb_subset_or_fail(face.get(), input.get()));
  if (!subset_face) {
    return {};
  }

  HBBlob subset_blob(hb_face_reference_blob(subset_face.get()));
  const char* out_data = hb_blob_get_data(subset_blob.get(), nullptr);
  unsigned int out_len = hb_blob_get_length(subset_blob.get());
  if (!out_data || out_len == 0) {
    return {};
  }

  // SAFETY: HarfBuzz guarantees the correct length from hb_blob_get_length.
  return std::vector<uint8_t>(out_data, UNSAFE_BUFFERS(out_data + out_len));
}

}  // namespace

CPDF_FontSubsetter::CPDF_FontSubsetter(CPDF_Document* doc) : doc_(doc) {}

CPDF_FontSubsetter::~CPDF_FontSubsetter() = default;

std::map<uint32_t, RetainPtr<const CPDF_Object>>
CPDF_FontSubsetter::GenerateObjectOverrides(
    pdfium::span<const uint32_t> new_obj_nums) {
  if (new_obj_nums.empty()) {
    return {};
  }

  CollectSubsetCandidates(new_obj_nums);

  std::map<uint32_t, RetainPtr<const CPDF_Object>> overrides;
  for (auto& [obj_num, candidate] : candidates_) {
    auto old_stream_acc =
        pdfium::MakeRetain<CPDF_StreamAcc>(candidate.font_stream);
    old_stream_acc->LoadAllDataFiltered();
    auto old_stream_span = old_stream_acc->GetSpan();

    std::vector<uint8_t> new_font_data =
        GenerateFontSubset(doc_, old_stream_span, candidate.used_gids);
    if (new_font_data.empty()) {
      continue;
    }

    // Override the font file stream.
    // TODO(crbug.com/476127152): Correctly support OpenType CFF.
    auto new_stream = pdfium::MakeRetain<CPDF_Stream>(new_font_data);
    // TrueType fonts requires a Length1 entry.
    new_stream->GetMutableDict()->SetNewFor<CPDF_Number>(
        "Length1", static_cast<int>(new_font_data.size()));
    overrides[obj_num] = new_stream;
  }
  return overrides;
}

CPDF_FontSubsetter::SubsetCandidate::SubsetCandidate() = default;

CPDF_FontSubsetter::SubsetCandidate::~SubsetCandidate() = default;

void CPDF_FontSubsetter::CollectSubsetCandidates(
    pdfium::span<const uint32_t> new_obj_nums) {
  for (int i = 0; i < doc_->GetPageCount(); ++i) {
    RetainPtr<CPDF_Dictionary> page_dict = doc_->GetMutablePageDictionary(i);
    if (!page_dict) {
      continue;
    }

    auto page = pdfium::MakeRetain<CPDF_Page>(doc_, page_dict);
    page->ParseContent();
    CollectSubsetCandidatesFromPage(page, new_obj_nums);
  }
}

void CPDF_FontSubsetter::CollectSubsetCandidatesFromPage(
    CPDF_Page* page,
    pdfium::span<const uint32_t> new_obj_nums) {
  for (const auto& page_obj : *page) {
    if (!page_obj->IsText()) {
      continue;
    }

    // Search for text and font files.
    const CPDF_TextObject* text = page_obj->AsText();
    RetainPtr<CPDF_Font> font = text->GetFont();
    if (!font) {
      continue;
    }

    RetainPtr<const CPDF_Dictionary> root_font = font->GetFontDict();
    if (!root_font) {
      continue;
    }

    if (!std::ranges::binary_search(new_obj_nums, root_font->GetObjNum())) {
      continue;
    }

    RetainPtr<const CPDF_Dictionary> descriptor;
    if (font->IsCIDFont()) {
      RetainPtr<const CPDF_Array> descendants =
          root_font->GetArrayFor("DescendantFonts");
      CHECK(descendants);
      RetainPtr<const CPDF_Dictionary> cid_font = descendants->GetDictAt(0);
      CHECK(cid_font);
      descriptor = cid_font->GetDictFor("FontDescriptor");
    } else {
      descriptor = root_font->GetDictFor("FontDescriptor");
    }
    if (!descriptor) {
      continue;
    }

    RetainPtr<const CPDF_Stream> font_stream =
        descriptor->GetStreamFor("FontFile2");
    if (!font_stream) {
      continue;
    }

    uint32_t obj_num = font_stream->GetObjNum();
    auto& candidate = candidates_[obj_num];
    if (!candidate.font_stream) {
      candidate.font_stream = font_stream;
    }
    AddUsedText(text, font, candidate);
  }
}

void CPDF_FontSubsetter::AddUsedText(const CPDF_TextObject* text,
                                     CPDF_Font* font,
                                     SubsetCandidate& candidate) {
  const std::vector<uint32_t>& char_codes = text->GetCharCodes();
  std::set<uint32_t>& used_gids = candidate.used_gids;
  for (uint32_t char_code : char_codes) {
    int gid = font->GlyphFromCharCode(char_code, /*pVertGlyph=*/nullptr);
    if (gid != -1) {
      used_gids.insert(static_cast<uint32_t>(gid));
    }
  }
}
