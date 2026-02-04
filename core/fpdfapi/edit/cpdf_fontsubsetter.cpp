// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/edit/cpdf_fontsubsetter.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
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
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/utf16.h"
#include "hb-subset.h"
#include "hb.h"

namespace {

constexpr uint32_t kMaxBfCharBfRangeEntries = 100;

const char kToUnicodeStart[] =
    "/CIDInit /ProcSet findresource begin\n"
    "12 dict begin\n"
    "begincmap\n"
    "/CIDSystemInfo\n"
    "<</Registry (Adobe)\n"
    "/Ordering (Identity)\n"
    "/Supplement 0\n"
    ">> def\n"
    "/CMapName /Adobe-Identity-H def\n"
    "/CMapType 2 def\n"
    "1 begincodespacerange\n"
    "<0000> <FFFF>\n"
    "endcodespacerange\n";

const char kToUnicodeEnd[] =
    "endcmap\n"
    "CMapName currentdict /CMap defineresource pop\n"
    "end\n"
    "end\n";

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

void AddCharcode(std::ostringstream& buffer, uint32_t number) {
  CHECK_LE(number, 0xFFFF);
  buffer << "<";
  char ans[4];
  FXSYS_IntToFourHexChars(number, ans);
  for (char c : ans) {
    buffer << c;
  }
  buffer << ">";
}

void AddUnicode(std::ostringstream& buffer, uint32_t unicode) {
  if (pdfium::IsHighSurrogate(unicode) || pdfium::IsLowSurrogate(unicode)) {
    unicode = 0;
  }

  char unicode_buf[8];
  pdfium::span<const char> unicode_span = FXSYS_ToUTF16BE(unicode, unicode_buf);
  CHECK(!unicode_span.empty());
  buffer << "<";
  for (char c : unicode_span) {
    buffer << c;
  }
  buffer << ">";
}

// Loads the charcode to unicode mapping into a stream
RetainPtr<CPDF_Stream> LoadUnicode(
    CPDF_Document* doc,
    const std::multimap<uint32_t, uint32_t>& to_unicode) {
  // A map charcode->unicode
  std::map<uint32_t, uint32_t> char_to_unicode_map;
  // Ranges
  std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>>
      char_range_to_unicodes_map;
  std::map<std::pair<uint32_t, uint32_t>, uint32_t>
      char_range_to_consecutive_unicodes_map;

  for (auto it = to_unicode.begin(); it != to_unicode.end(); ++it) {
    uint32_t first_charcode = it->first;
    uint32_t first_unicode = it->second;
    {
      auto next_it = std::next(it);
      if (next_it == to_unicode.end() || first_charcode + 1 != next_it->first) {
        char_to_unicode_map[first_charcode] = first_unicode;
        continue;
      }
    }
    ++it;
    uint32_t current_charcode = it->first;
    uint32_t current_unicode = it->second;
    if (current_charcode % 256 == 0) {
      char_to_unicode_map[first_charcode] = first_unicode;
      char_to_unicode_map[current_charcode] = current_unicode;
      continue;
    }
    const size_t max_extra = 255 - (current_charcode % 256);
    auto next_it = std::next(it);
    if (first_unicode + 1 != current_unicode) {
      std::vector<uint32_t> unicodes = {first_unicode, current_unicode};
      for (size_t i = 0; i < max_extra; ++i) {
        if (next_it == to_unicode.end() ||
            current_charcode + 1 != next_it->first) {
          break;
        }
        ++it;
        ++current_charcode;
        unicodes.push_back(it->second);
        next_it = std::next(it);
      }
      char_range_to_unicodes_map[std::make_pair(first_charcode, it->first)] =
          std::move(unicodes);
      continue;
    }
    for (size_t i = 0; i < max_extra; ++i) {
      if (next_it == to_unicode.end() ||
          current_charcode + 1 != next_it->first ||
          current_unicode + 1 != next_it->second) {
        break;
      }
      ++it;
      ++current_charcode;
      ++current_unicode;
      next_it = std::next(it);
    }
    char_range_to_consecutive_unicodes_map[std::make_pair(
        first_charcode, current_charcode)] = first_unicode;
  }

  std::ostringstream buffer;
  buffer << kToUnicodeStart;

  {
    uint32_t to_process =
        pdfium::checked_cast<uint32_t>(char_to_unicode_map.size());
    auto it = char_to_unicode_map.begin();
    while (to_process) {
      const uint32_t to_process_this_iteration =
          std::min(to_process, kMaxBfCharBfRangeEntries);
      buffer << to_process_this_iteration << " beginbfchar\n";
      for (uint32_t i = 0; i < to_process_this_iteration; ++i) {
        AddCharcode(buffer, it->first);
        buffer << " ";
        AddUnicode(buffer, it->second);
        buffer << "\n";
        ++it;
      }
      buffer << "endbfchar\n";
      to_process -= to_process_this_iteration;
    }
  }

  {
    uint32_t to_process =
        pdfium::checked_cast<uint32_t>(char_range_to_unicodes_map.size());
    auto it = char_range_to_unicodes_map.begin();
    while (to_process) {
      const uint32_t to_process_this_iteration =
          std::min(to_process, kMaxBfCharBfRangeEntries);
      buffer << to_process_this_iteration << " beginbfrange\n";
      for (uint32_t i = 0; i < to_process_this_iteration; ++i) {
        const std::pair<uint32_t, uint32_t>& charcode_range = it->first;
        AddCharcode(buffer, charcode_range.first);
        buffer << " ";
        AddCharcode(buffer, charcode_range.second);
        buffer << " [";
        auto unicodes = pdfium::span(it->second);
        AddUnicode(buffer, unicodes[0]);
        for (uint32_t code : unicodes.subspan(1u)) {
          buffer << " ";
          AddUnicode(buffer, code);
        }
        buffer << "]\n";
        ++it;
      }
      buffer << "endbfrange\n";
      to_process -= to_process_this_iteration;
    }
  }

  {
    uint32_t to_process = pdfium::checked_cast<uint32_t>(
        char_range_to_consecutive_unicodes_map.size());
    auto it = char_range_to_consecutive_unicodes_map.begin();
    while (to_process) {
      const uint32_t to_process_this_iteration =
          std::min(to_process, kMaxBfCharBfRangeEntries);
      buffer << to_process_this_iteration << " beginbfrange\n";
      for (uint32_t i = 0; i < to_process_this_iteration; ++i) {
        const std::pair<uint32_t, uint32_t>& charcode_range = it->first;
        AddCharcode(buffer, charcode_range.first);
        buffer << " ";
        AddCharcode(buffer, charcode_range.second);
        buffer << " ";
        AddUnicode(buffer, it->second);
        buffer << "\n";
        ++it;
      }
      buffer << "endbfrange\n";
      to_process -= to_process_this_iteration;
    }
  }

  buffer << kToUnicodeEnd;
  // Convert string to CPDF_Stream
  std::string str = buffer.str();
  // std::cout << " " << __func__ << " str=" << str << std::endl;
  auto stream = doc->NewIndirect<CPDF_Stream>(pdfium::as_byte_span(str));
  return stream;
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

struct SubsetCandidate {
  SubsetCandidate() = default;
  ~SubsetCandidate() = default;

  ByteString base_font_name;

  // Potential overrides.
  RetainPtr<const CPDF_Stream> font_stream;
  RetainPtr<const CPDF_Dictionary> descriptor;
  RetainPtr<const CPDF_Dictionary> root_font_dict;
  RetainPtr<const CPDF_Dictionary> cid_font_dict;

  // Mappings.
  std::set<uint32_t> used_gids;
  std::map<uint32_t, uint32_t> char_code_to_unicode;
  std::map<uint32_t, uint32_t> char_code_to_gid;
};

std::vector<uint8_t> CreateCIDToGIDMapData(
    const std::map<uint32_t, uint32_t>& map) {
  if (map.empty()) {
    return {};
  }
  uint32_t max_cid = map.rbegin()->first;

  // The map is an array of 16-bit GIDs, indexed by CID.
  // Size = (Max CID + 1) * 2 bytes.
  // Initialize with 0 (GID 0 / .notdef).
  std::vector<uint8_t> map_data((max_cid + 1) * 2, 0);

  for (const auto& [cid, gid] : map) {
    uint16_t gid16 = static_cast<uint16_t>(gid);
    // Write GID in Big Endian
    map_data[cid * 2] = (gid16 >> 8) & 0xFF;
    map_data[cid * 2 + 1] = gid16 & 0xFF;
    std::cout << __func__ << " cid=" << cid << " gid=" << gid << std::endl;
  }
  return map_data;
}

std::set<uint32_t> GetNewObjectNums(CPDF_Document* doc, CPDF_Parser* parser) {
  std::set<uint32_t> new_obj_nums;
  for (const auto& pair : *doc) {
    const uint32_t obj_num = pair.first;
    if (pair.second->GetObjNum() == CPDF_Object::kInvalidObjNum) {
      continue;
    }

    if (parser && parser->IsValidObjectNumber(obj_num) &&
        !parser->IsObjectFree(obj_num)) {
      continue;
    }

    new_obj_nums.insert(obj_num);
  }
  return new_obj_nums;
}

void AddUsedText(SubsetCandidate& candidate,
                 const CPDF_TextObject* text,
                 CPDF_Font* font) {
  const std::vector<uint32_t>& char_codes = text->GetCharCodes();
  std::set<uint32_t>& used_gids = candidate.used_gids;
  for (uint32_t char_code : char_codes) {
    int gid = font->GlyphFromCharCode(char_code, /*pVertGlyph=*/nullptr);
    if (gid != -1) {
      used_gids.insert(static_cast<uint32_t>(gid));
      candidate.char_code_to_gid[char_code] = gid;

      // ToUnicode
      WideString ws = font->UnicodeFromCharCode(char_code);
      if (!ws.IsEmpty()) {
        candidate.char_code_to_unicode[char_code] =
            static_cast<uint32_t>(ws[0]);
      }
    }
    used_gids.insert(0);
  }
}

void GetSubsetCandidatesHelper(
    CPDF_Page* page,
    const std::set<uint32_t>& new_obj_nums,
    std::map<uint32_t, SubsetCandidate>& candidates) {
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

    RetainPtr<const CPDF_Dictionary> root_font_dict = font->GetFontDict();
    if (!root_font_dict ||
        !new_obj_nums.contains(root_font_dict->GetObjNum())) {
      continue;
    }

    RetainPtr<const CPDF_Dictionary> cid_font_dict;
    RetainPtr<const CPDF_Dictionary> descriptor;
    if (font->IsCIDFont()) {
      RetainPtr<const CPDF_Array> descendants =
          root_font_dict->GetArrayFor("DescendantFonts");
      if (!descendants || descendants->IsEmpty()) {
        continue;
      }
      cid_font_dict = descendants->GetDictAt(0);
      if (!cid_font_dict) {
        continue;
      }
      descriptor = cid_font_dict->GetDictFor("FontDescriptor");
    } else {
      descriptor = root_font_dict->GetDictFor("FontDescriptor");
    }
    if (!descriptor) {
      continue;
    }

    RetainPtr<const CPDF_Stream> font_stream =
        descriptor->GetStreamFor("FontFile2");
    if (!font_stream) {
      continue;
    }

    auto& candidate = candidates[font_stream->GetObjNum()];
    if (!candidate.font_stream) {
      candidate.base_font_name = font->GetBaseFontName();
      candidate.font_stream = std::move(font_stream);
      candidate.descriptor = std::move(descriptor);
      candidate.root_font_dict = std::move(root_font_dict);
      candidate.cid_font_dict = std::move(cid_font_dict);
    }
    AddUsedText(candidate, text, font);
  }
}

std::map<uint32_t, SubsetCandidate> GetSubsetCandidates(CPDF_Document* doc,
                                                        CPDF_Parser* parser) {
  std::map<uint32_t, SubsetCandidate> candidates;
  const std::set<uint32_t> new_obj_nums = GetNewObjectNums(doc, parser);
  for (int i = 0; i < doc->GetPageCount(); ++i) {
    RetainPtr<CPDF_Dictionary> page_dict = doc->GetMutablePageDictionary(i);
    if (!page_dict) {
      continue;
    }

    auto page = pdfium::MakeRetain<CPDF_Page>(doc, std::move(page_dict));
    page->ParseContent();
    GetSubsetCandidatesHelper(page, new_obj_nums, candidates);
  }
  return candidates;
}

std::vector<uint8_t> GenerateFontSubset(pdfium::span<const uint8_t> font_data,
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

  // Un comment to render again
  // hb_set_t* no_subset =
  // hb_subset_input_set(input.get(), HB_SUBSET_SETS_NO_SUBSET_TABLE_TAG);
  // hb_set_add(no_subset, HB_TAG('C', 'F', 'F', ' '));

  hb_set_t* glyphs = hb_subset_input_glyph_set(input.get());
  for (uint32_t id : gids) {
    hb_set_add(glyphs, id);
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

  DumpFontTableInfo(subset_face.get(), "SUBSETTED FONT");

  // SAFETY: HarfBuzz guarantees the correct length from hb_blob_get_length.
  return std::vector<uint8_t>(out_data, UNSAFE_BUFFERS(out_data + out_len));
}

}  // namespace

std::map<uint32_t, RetainPtr<CPDF_Object>> GenerateFontSubsetObjectOverrides(
    CPDF_Document* doc,
    CPDF_Parser* parser) {
  std::map<uint32_t, RetainPtr<CPDF_Object>> overrides;
  for (auto& [obj_num, candidate] : GetSubsetCandidates(doc, parser)) {
    auto old_stream_acc =
        pdfium::MakeRetain<CPDF_StreamAcc>(std::move(candidate.font_stream));
    old_stream_acc->LoadAllDataFiltered();
    std::vector<uint8_t> new_font_data =
        GenerateFontSubset(old_stream_acc->GetSpan(), candidate.used_gids);
    std::cout << " size before=" << old_stream_acc->GetSpan().size()
              << " size after =" << new_font_data.size() << std::endl;
    if (new_font_data.empty()) {
      continue;
    }
    auto new_stream = pdfium::MakeRetain<CPDF_Stream>(new_font_data);

    // Update the length metadata to match the new binary size.
    auto new_stream_dict = new_stream->GetMutableDict();
    new_stream_dict->SetNewFor<CPDF_Number>(
        "Length1", static_cast<int>(new_font_data.size()));
    new_stream_dict->RemoveFor("Length2");
    new_stream_dict->RemoveFor("Length3");
    overrides[obj_num] = new_stream;

    // Clone objects and fill names.
    ByteString subset_name = "CHEESE+" + candidate.base_font_name;

    RetainPtr<CPDF_Object> new_root_font_dict =
        candidate.root_font_dict->Clone();
    RetainPtr<CPDF_Object> new_descriptor = candidate.descriptor->Clone();
    new_root_font_dict->AsMutableDictionary()->SetNewFor<CPDF_Name>(
        "BaseFont", subset_name);
    new_descriptor->AsMutableDictionary()->SetNewFor<CPDF_Name>("FontName",
                                                                subset_name);
    overrides[candidate.root_font_dict->GetObjNum()] =
        std::move(new_root_font_dict);
    overrides[candidate.descriptor->GetObjNum()] = std::move(new_descriptor);
    if (candidate.cid_font_dict) {
      RetainPtr<CPDF_Object> new_cid_font_dict =
          candidate.cid_font_dict->Clone();
      new_cid_font_dict->AsMutableDictionary()->SetNewFor<CPDF_Name>(
          "BaseFont", subset_name);
      overrides[candidate.cid_font_dict->GetObjNum()] =
          std::move(new_cid_font_dict);
    }
    std::cout << "XXX name override obj nums="
              << candidate.root_font_dict->GetObjNum() << ","
              << candidate.descriptor->GetObjNum() << ","
              << candidate.cid_font_dict->GetObjNum() << std::endl;

    // Handle /CIDToGIDMap.
    if (candidate.cid_font_dict) {
      // Generate the binary map data
      std::vector<uint8_t> map_data =
          CreateCIDToGIDMapData(candidate.char_code_to_gid);

      if (!map_data.empty()) {
        // Create a new stream for the map
        auto pMapStream = doc->NewIndirect<CPDF_Stream>(pdfium::span(map_data));

        // Clone the CIDFont dictionary
        auto pNewCidFont = ToDictionary(candidate.cid_font_dict->Clone());

        // Point "CIDToGIDMap" to our new stream
        // pNewCidFont->SetNewFor<CPDF_Reference>("CIDToGIDMap", doc,
        //  pMapStream->GetObjNum());
        pNewCidFont->SetNewFor<CPDF_Name>("CIDToGIDMap", "Identity");
        // Override the CIDFont object
        std::cout << "CIDFONT num=" << candidate.cid_font_dict->GetObjNum()
                  << std::endl;
        overrides[candidate.cid_font_dict->GetObjNum()] = pNewCidFont;

        // NOTE: Since we overrode the object in-place (same ObjNum),
        // the Root Font's "DescendantFonts" array still points to this ID,
        // so we don't necessarily need to update the Root Font unless
        // we are also fixing the ToUnicode map on it.
      }
    }

    // Handle /ToUnicode.
    if (!candidate.char_code_to_unicode.empty() && candidate.root_font_dict) {
      // Check if the original had a ToUnicode map
      if (candidate.root_font_dict->KeyExist("ToUnicode")) {
        const CPDF_Object* obj =
            candidate.root_font_dict->GetObjectFor("ToUnicode");
        if (obj && obj->IsReference()) {
          // uint32_t old_id = obj->AsReference()->GetRefObjNum();

          // Generate the NEW content
          std::multimap<uint32_t, uint32_t> to_unicode_multimap;
          for (const auto& [char_code, unicode] :
               candidate.char_code_to_unicode) {
            to_unicode_multimap.insert({char_code, unicode});
          }

          // Skip ToUnicode
          // overrides[old_id] = LoadUnicode(doc, to_unicode_multimap);
          LoadUnicode(doc, to_unicode_multimap);
        }
      }
    }
  }
  std::cout << "XXX " << __func__
            << " final overrides size=" << overrides.size() << std::endl;
  return overrides;
}