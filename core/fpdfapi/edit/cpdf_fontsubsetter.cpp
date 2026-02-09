// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/edit/cpdf_fontsubsetter.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "core/fpdfapi/edit/cpdf_unicode_util.h"
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
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "cpdf_creator.h"
#include "cpdf_fontsubsetter.h"
#include "hb-subset.h"
#include "hb.h"

namespace {

void DumpFontTableInfo(hb_face_t* face, const std::string& label) {
  std::cout << "--- Font Table Dump: " << label << " ---" << std::endl;

  // 1. Get the number of tables
  unsigned int table_count = hb_face_get_table_tags(face, 0, nullptr, nullptr);
  std::vector<hb_tag_t> tags(table_count);

  // 2. Retrieve the tags
  hb_face_get_table_tags(face, 0, &table_count, tags.data());

  unsigned int total_size = 0;
  for (hb_tag_t tag : tags) {
    // Convert the 4-byte tag to a human-readable string
    char name[5];
    hb_tag_to_string(tag, name);
    name[4] = '\0';

    // Get the size of this specific table
    hb_blob_t* table_blob = hb_face_reference_table(face, tag);
    unsigned int size = hb_blob_get_length(table_blob);
    total_size += size;
    hb_blob_destroy(table_blob);

    std::cout << "  Table [" << name << "]: " << std::setw(8) << size
              << " bytes";

    // Special Debug Info for 'maxp' table (Glyph Count)
    if (tag == HB_TAG('m', 'a', 'x', 'p')) {
      hb_blob_t* maxp_blob = hb_face_reference_table(face, tag);
      unsigned int len;
      const char* data = hb_blob_get_data(maxp_blob, &len);
      if (len >= 6) {
        // numGlyphs is at offset 4 in the maxp table (uint16_t)
        UNSAFE_TODO(uint16_t num_glyphs = (static_cast<uint8_t>(data[4]) << 8) |
                                          static_cast<uint8_t>(data[5]));
        std::cout << "  (numGlyphs: " << num_glyphs << ")";
      }
      hb_blob_destroy(maxp_blob);
    }
    std::cout << std::endl;
  }

  std::cout << "Total Binary Size: " << total_size << " bytes" << std::endl;
  std::cout << "--------------------------------------" << std::endl;
}

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

struct FontSubset {
  std::vector<uint8_t> font_data;
  RetainPtr<const CPDF_Object> widths;
};

// 9.6.4 Font Subsets: the font name must begin with a tag followed by a plus
// sign (+). The tag must consist of six uppercase letters.
ByteString GenerateFontSubsetName(const ByteString& base_font_name) {
  static constexpr int kTagLength = 6;
  static constexpr auto kCharset = std::to_array<char>(
      {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
       'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'});

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, kCharset.size() - 1);

  ByteString tag;
  for (int i = 0; i < kTagLength; ++i) {
    tag += kCharset[dis(gen)];
  }

  return tag + "+" + base_font_name;
}

FontSubset GenerateFontSubset(CPDF_Document* doc,
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
  DumpFontTableInfo(face.get(), "ORIGINAL FONT");
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

  // Create a temporary font object to extract metrics
  hb_font_t* hb_font = hb_font_create(subset_face.get());

  // Build the /W array: [start_cid [w1 w2 w3] next_cid [w4] ...]
  auto widths = doc->NewIndirect<CPDF_Array>();

  for (uint32_t gid : gids) {
    // 1. Get the horizontal advance from the font data
    hb_position_t h_advance = hb_font_get_glyph_h_advance(hb_font, gid);

    // 2. Add to PDF /W array.
    // Format: CID [ Width ]
    widths->AppendNew<CPDF_Number>(static_cast<int>(gid));
    auto width = widths->AppendNew<CPDF_Array>();
    width->AppendNew<CPDF_Number>(static_cast<int>(h_advance));
  }

  hb_font_destroy(hb_font);

  DumpFontTableInfo(subset_face.get(), "SUBSETTED FONT");

  // SAFETY: HarfBuzz guarantees the correct length from hb_blob_get_length.
  return {std::vector<uint8_t>(out_data, UNSAFE_BUFFERS(out_data + out_len)),
          widths};
}

}  // namespace

CPDF_FontSubsetter::CPDF_FontSubsetter(CPDF_Document* doc, CPDF_Parser* parser)
    : doc_(doc), parser_(parser) {}

CPDF_FontSubsetter::~CPDF_FontSubsetter() = default;

std::map<uint32_t, RetainPtr<const CPDF_Object>>
CPDF_FontSubsetter::GenerateObjectOverrides() {
  CollectNewObjectNums();
  CollectSubsetCandidates();

  std::map<uint32_t, RetainPtr<const CPDF_Object>> overrides;
  for (auto& [obj_num, candidate] : candidates_) {
    auto old_stream_acc =
        pdfium::MakeRetain<CPDF_StreamAcc>(candidate.font_stream);
    old_stream_acc->LoadAllDataFiltered();
    auto old_stream_span = old_stream_acc->GetSpan();

    FontSubset subset =
        GenerateFontSubset(doc_, old_stream_span, candidate.used_gids);
    if (subset.font_data.empty()) {
      continue;
    }

    auto new_stream = pdfium::MakeRetain<CPDF_Stream>(subset.font_data);

    bool is_cff = (old_stream_span.size() > 4 && old_stream_span[0] == 'O' &&
                   old_stream_span[1] == 'T' && old_stream_span[2] == 'T' &&
                   old_stream_span[3] == 'O');
    CPDF_Dictionary* mutable_new_stream = new_stream->GetMutableDict();
    if (is_cff) {
      mutable_new_stream->SetNewFor<CPDF_Name>("Subtype", "OpenType");
    } else {
      // TrueType fonts requires a Length1 entry.
      mutable_new_stream->SetNewFor<CPDF_Number>(
          "Length1", static_cast<int>(subset.font_data.size()));
    }

    // Override the root font dict.
    RetainPtr<CPDF_Object> new_root_font = candidate.root_font->Clone();
    new_root_font->AsMutableDictionary()->SetNewFor<CPDF_Name>(
        "BaseFont", candidate.subset_font_name);
    overrides[candidate.root_font->GetObjNum()] = new_root_font;

    // Override the CID font dict if necessary.
    if (candidate.cid_font) {
      RetainPtr<CPDF_Object> new_cid_font = candidate.cid_font->Clone();
      CPDF_Dictionary* mutable_cid_font = new_cid_font->AsMutableDictionary();
      mutable_cid_font->SetNewFor<CPDF_Name>("BaseFont",
                                             candidate.subset_font_name);
      if (is_cff) {
        mutable_cid_font->SetNewFor<CPDF_Name>("Subtype", "CIDFontType0");
      }
      overrides[candidate.cid_font->GetObjNum()] = new_cid_font;
    }

    // Override the font descriptor.
    RetainPtr<CPDF_Object> new_descriptor = candidate.descriptor->Clone();
    CPDF_Dictionary* mutable_descriptor = new_descriptor->AsMutableDictionary();
    mutable_descriptor->SetNewFor<CPDF_Name>("FontName",
                                             candidate.subset_font_name);
    if (is_cff) {
      // Set the third bit position for symbolic (outside standard Latin
      // character set), or sixth for nonsymbolic.
      mutable_descriptor->SetNewFor<CPDF_Number>("Flags", 4);
      mutable_descriptor->RemoveFor("FontFile2");
      mutable_descriptor->SetNewFor<CPDF_Reference>("FontFile3", doc_, obj_num);
    }
    overrides[candidate.descriptor->GetObjNum()] = new_descriptor;

    // Override Widths.
    RetainPtr<const CPDF_Array> widths = candidate.cid_font->GetArrayFor("W");
    if (widths) {
      overrides[widths->GetObjNum()] = subset.widths;
    }

    // Override ToUnicode.
    RetainPtr<const CPDF_Stream> to_unicode =
        candidate.root_font->GetStreamFor("ToUnicode");
    if (to_unicode) {
      RetainPtr<CPDF_Stream> unicode_stream =
          LoadUnicode(doc_, candidate.char_code_to_unicode);
      overrides[to_unicode->GetObjNum()] = unicode_stream;
    }

    overrides[obj_num] = new_stream;
  }
  return overrides;
}

CPDF_FontSubsetter::SubsetCandidate::SubsetCandidate() = default;

CPDF_FontSubsetter::SubsetCandidate::~SubsetCandidate() = default;

void CPDF_FontSubsetter::CollectNewObjectNums() {
  for (const auto& pair : *doc_) {
    const uint32_t obj_num = pair.first;
    if (pair.second->GetObjNum() == CPDF_Object::kInvalidObjNum) {
      continue;
    }

    if (parser_ && parser_->IsValidObjectNumber(obj_num) &&
        !parser_->IsObjectFree(obj_num)) {
      continue;
    }

    new_obj_nums_.insert(obj_num);
  }
}

void CPDF_FontSubsetter::CollectSubsetCandidates() {
  for (int i = 0; i < doc_->GetPageCount(); ++i) {
    RetainPtr<CPDF_Dictionary> page_dict = doc_->GetMutablePageDictionary(i);
    if (!page_dict) {
      continue;
    }

    auto page = pdfium::MakeRetain<CPDF_Page>(doc_, page_dict);
    page->ParseContent();
    CollectSubsetCandidatesFromPage(page);
  }
}

void CPDF_FontSubsetter::CollectSubsetCandidatesFromPage(CPDF_Page* page) {
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
    if (!root_font || !new_obj_nums_.contains(root_font->GetObjNum())) {
      continue;
    }

    RetainPtr<const CPDF_Dictionary> cid_font;
    RetainPtr<const CPDF_Dictionary> descriptor;
    if (font->IsCIDFont()) {
      RetainPtr<const CPDF_Array> descendants =
          root_font->GetArrayFor("DescendantFonts");
      if (!descendants || descendants->IsEmpty()) {
        continue;
      }
      cid_font = descendants->GetDictAt(0);
      if (!cid_font) {
        continue;
      }
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

    auto& candidate = candidates_[font_stream->GetObjNum()];
    if (!candidate.font_stream) {
      candidate.subset_font_name =
          GenerateFontSubsetName(font->GetBaseFontName());
      candidate.font_stream = font_stream;
      candidate.root_font = root_font;
      candidate.cid_font = cid_font;
      candidate.descriptor = descriptor;
    }
    AddUsedText(candidate, text, font);
  }
}

void CPDF_FontSubsetter::AddUsedText(SubsetCandidate& candidate,
                                     const CPDF_TextObject* text,
                                     CPDF_Font* font) {
  const std::vector<uint32_t>& char_codes = text->GetCharCodes();
  std::set<uint32_t>& used_gids = candidate.used_gids;
  for (uint32_t char_code : char_codes) {
    int gid = font->GlyphFromCharCode(char_code, /*pVertGlyph=*/nullptr);
    if (gid != -1) {
      used_gids.insert(static_cast<uint32_t>(gid));
      WideString unicode = font->UnicodeFromCharCode(char_code);
      if (!unicode.IsEmpty()) {
        candidate.char_code_to_unicode.emplace(
            char_code, static_cast<uint32_t>(unicode[0]));
      }
    }
  }
}
