// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/edit/cpdf_fontsubsetter.h"

#include <stdint.h>

#include <string>
#include <vector>

#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/retain_ptr.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/fx_string_testhelpers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"
#include "testing/utils/path_service.h"

namespace {

CPDF_Document* GetCPDFDocument(FPDF_DOCUMENT document) {
  // This is cheating slightly to avoid a layering violation, since this file
  // cannot include fpdfsdk/cpdfsdk_helpers.h to get access to
  // GetCPDFDocument().
  return reinterpret_cast<CPDF_Document*>((document));
}

}  // namespace

class CPDFFontSubsetterTest : public EmbedderTest {
 public:
  void InsertNewTextObject(const std::wstring& text,
                           FPDF_PAGE page,
                           FPDF_FONT font) {
    FPDF_PAGEOBJECT text_object =
        FPDFPageObj_CreateTextObj(document(), font, 20.0f);
    EXPECT_TRUE(text_object);

    ScopedFPDFWideString fpdf_text = GetFPDFWideString(text);
    EXPECT_TRUE(FPDFText_SetText(text_object, fpdf_text.get()));

    const FS_MATRIX matrix{1, 0, 0, 1, 50, 200};
    ASSERT_TRUE(FPDFPageObj_TransformF(text_object, &matrix));
    FPDFPage_InsertObject(page, text_object);
    EXPECT_TRUE(FPDFPage_GenerateContent(page));
  }

  std::string GetTestFontFilePath(const std::string& file_name) {
    return PathService::GetThirdPartyFilePath("test_fonts/test_fonts/" +
                                              file_name);
  }
};

TEST_F(CPDFFontSubsetterTest, NoNewText) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  CPDF_FontSubsetter subsetter(GetCPDFDocument(document()));

  std::vector<uint32_t> new_obj_nums;
  EXPECT_TRUE(subsetter.GenerateObjectOverrides(new_obj_nums).empty());

  new_obj_nums.push_back(1);
  EXPECT_TRUE(subsetter.GenerateObjectOverrides(new_obj_nums).empty());

  // No-text object.
  FPDF_PAGEOBJECT rect = FPDFPageObj_CreateNewRect(20, 100, 50, 50);
  FPDFPage_InsertObject(page.get(), rect);
  new_obj_nums.push_back(1);
  EXPECT_TRUE(subsetter.GenerateObjectOverrides(new_obj_nums).empty());
}

TEST_F(CPDFFontSubsetterTest, OpenType) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  // Load font data.
  const std::string font_path =
      GetTestFontFilePath("NotoSansCJKjp-Regular.otf");
  ASSERT_FALSE(font_path.empty());

  std::vector<uint8_t> font_data = GetFileContents(font_path.c_str());
  ASSERT_EQ(16427228u, font_data.size());

  ScopedFPDFFont font(FPDFText_LoadFont(document(), font_data.data(),
                                        font_data.size(), FPDF_FONT_TRUETYPE,
                                        /*cid=*/true));
  ASSERT_TRUE(font);

  ASSERT_NO_FATAL_FAILURE(InsertNewTextObject(L"这", page.get(), font.get()));

  // Run the subsetter. In production, objects 5 to 13 are for the new text
  // object.
  CPDF_FontSubsetter subsetter(GetCPDFDocument(document()));
  std::vector<uint32_t> kTestObjNums{5, 6, 7, 8, 9, 10, 11, 12, 13};
  auto overrides = subsetter.GenerateObjectOverrides(kTestObjNums);

  // Check the overridden objects.
  ASSERT_EQ(1u, overrides.size());
  auto it = overrides.find(9);
  ASSERT_NE(it, overrides.end());

  auto* new_font_stream = it->second->AsStream();
  ASSERT_TRUE(new_font_stream);
  // TODO(crbug.com/476127152): Size can be further reduced by subsetting glyph
  // widths and Unicode mapping.
  EXPECT_EQ(445996u, new_font_stream->GetRawSize());
}

TEST_F(CPDFFontSubsetterTest, TrueType) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  // Load font data.
  const std::string font_path = GetTestFontFilePath("Arimo-Regular.ttf");
  ASSERT_FALSE(font_path.empty());

  std::vector<uint8_t> font_data = GetFileContents(font_path.c_str());
  ASSERT_EQ(436180u, font_data.size());

  ScopedFPDFFont font(FPDFText_LoadFont(document(), font_data.data(),
                                        font_data.size(), FPDF_FONT_TRUETYPE,
                                        /*cid=*/true));
  ASSERT_TRUE(font);

  ASSERT_NO_FATAL_FAILURE(
      InsertNewTextObject(L"Hello world", page.get(), font.get()));

  // Run the subsetter. In production, objects 5 to 13 are for the new text
  // object.
  CPDF_FontSubsetter subsetter(GetCPDFDocument(document()));
  std::vector<uint32_t> kTestObjNums{5, 6, 7, 8, 9, 10, 11, 12, 13};
  auto overrides = subsetter.GenerateObjectOverrides(kTestObjNums);

  // Check the overridden objects.
  ASSERT_EQ(1u, overrides.size());
  auto it = overrides.find(9);
  ASSERT_NE(it, overrides.end());

  auto* new_font_stream = it->second->AsStream();
  ASSERT_TRUE(new_font_stream);
  // TODO(crbug.com/476127152): Size can be further reduced by subsetting glyph
  // widths and Unicode mapping.
  EXPECT_EQ(13276u, new_font_stream->GetRawSize());
}

TEST_F(CPDFFontSubsetterTest, SingleFontMultipleText) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  // Load font data.
  const std::string font_path =
      GetTestFontFilePath("NotoSansCJKjp-Regular.otf");
  ASSERT_FALSE(font_path.empty());

  std::vector<uint8_t> font_data = GetFileContents(font_path.c_str());
  ASSERT_EQ(16427228u, font_data.size());

  ScopedFPDFFont font(FPDFText_LoadFont(document(), font_data.data(),
                                        font_data.size(), FPDF_FONT_TRUETYPE,
                                        /*cid=*/true));
  ASSERT_TRUE(font);

  ASSERT_NO_FATAL_FAILURE(InsertNewTextObject(L"这", page.get(), font.get()));
  ASSERT_NO_FATAL_FAILURE(
      InsertNewTextObject(L"是第一句。", page.get(), font.get()));

  // Run the subsetter. In production, objects 5 to 13 are for the new text
  // object.
  CPDF_FontSubsetter subsetter(GetCPDFDocument(document()));
  std::vector<uint32_t> kTestObjNums{5, 6, 7, 8, 9, 10, 11, 12, 13};
  auto overrides = subsetter.GenerateObjectOverrides(kTestObjNums);

  // Check the overridden objects.
  ASSERT_EQ(1u, overrides.size());
  auto it = overrides.find(9);
  ASSERT_NE(it, overrides.end());

  auto* new_font_stream = it->second->AsStream();
  ASSERT_TRUE(new_font_stream);
  // TODO(crbug.com/476127152): Size can be further reduced by subsetting glyph
  // widths and Unicode mapping.
  EXPECT_EQ(446728u, new_font_stream->GetRawSize());
}

TEST_F(CPDFFontSubsetterTest, MultipleFontMultipleText) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  // Load font data.
  const std::string font_path1 =
      GetTestFontFilePath("NotoSansCJKjp-Regular.otf");
  ASSERT_FALSE(font_path1.empty());
  const std::string font_path2 = GetTestFontFilePath("Arimo-Regular.ttf");
  ASSERT_FALSE(font_path2.empty());

  std::vector<uint8_t> font_data1 = GetFileContents(font_path1.c_str());
  ASSERT_EQ(16427228u, font_data1.size());
  std::vector<uint8_t> font_data2 = GetFileContents(font_path2.c_str());
  ASSERT_EQ(436180u, font_data2.size());

  ScopedFPDFFont font1(FPDFText_LoadFont(document(), font_data1.data(),
                                         font_data1.size(), FPDF_FONT_TRUETYPE,
                                         /*cid=*/true));
  ASSERT_TRUE(font1);
  ScopedFPDFFont font2(FPDFText_LoadFont(document(), font_data2.data(),
                                         font_data2.size(), FPDF_FONT_TRUETYPE,
                                         /*cid=*/true));
  ASSERT_TRUE(font2);

  ASSERT_NO_FATAL_FAILURE(
      InsertNewTextObject(L"Hello", page.get(), font1.get()));
  ASSERT_NO_FATAL_FAILURE(
      InsertNewTextObject(L"Goodbye", page.get(), font2.get()));

  // Run the subsetter.
  CPDF_FontSubsetter subsetter(GetCPDFDocument(document()));
  std::vector<uint32_t> kTestObjNums{5,  6,  7,  8,  9,  10, 11, 12, 13,
                                     14, 15, 16, 17, 18, 19, 20, 21, 22};
  auto overrides = subsetter.GenerateObjectOverrides(kTestObjNums);

  // Check the overridden objects.
  ASSERT_EQ(2u, overrides.size());

  // First font.
  auto it = overrides.find(9);
  ASSERT_NE(it, overrides.end());

  auto* new_font_stream = it->second->AsStream();
  ASSERT_TRUE(new_font_stream);
  // TODO(crbug.com/476127152): Size can be further reduced by subsetting glyph
  // widths and Unicode mapping.
  EXPECT_EQ(2848u, new_font_stream->GetRawSize());

  // Second font.
  it = overrides.find(16);
  ASSERT_NE(it, overrides.end());

  new_font_stream = it->second->AsStream();
  ASSERT_TRUE(new_font_stream);
  // TODO(crbug.com/476127152): Size can be further reduced by subsetting glyph
  // widths and Unicode mapping.
  EXPECT_EQ(13332u, new_font_stream->GetRawSize());
}
