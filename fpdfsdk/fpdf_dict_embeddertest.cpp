// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdf_dict.h"
#include "testing/embedder_test.h"
#include "testing/fx_string_testhelpers.h"
#include "testing/gtest/include/gtest/gtest.h"

class FPDFDictEmbedderTest : public EmbedderTest {};

TEST_F(FPDFDictEmbedderTest, GetPageDictionary) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  EXPECT_NE(nullptr, page_dict);
}

TEST_F(FPDFDictEmbedderTest, GetStringFromPageDict) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  ASSERT_TRUE(page_dict);

  unsigned short buf[64];
  FPDF_DictionaryGetString(page_dict, "Type", buf, sizeof(buf));
  EXPECT_EQ(L"Page", GetPlatformWString(buf));
}

TEST_F(FPDFDictEmbedderTest, GetRectFromPageDict) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  ASSERT_TRUE(page_dict);

  FS_RECTF media_box;
  ASSERT_TRUE(FPDF_DictionaryGetRect(page_dict, "MediaBox", &media_box));

  EXPECT_FLOAT_EQ(0.0f, media_box.left);
  EXPECT_FLOAT_EQ(0.0f, media_box.bottom);
  EXPECT_FLOAT_EQ(612.0f, media_box.right);
  EXPECT_FLOAT_EQ(792.0f, media_box.top);
}

TEST_F(FPDFDictEmbedderTest, GetResourcesDict) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  ASSERT_TRUE(page_dict);

  FPDF_DICTIONARY res_dict = FPDF_DictionaryGetDict(page_dict, "Resources");
  EXPECT_NE(nullptr, res_dict);
}

TEST_F(FPDFDictEmbedderTest, GetNestedExtGStateDict) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  FPDF_DICTIONARY res_dict = FPDF_DictionaryGetDict(page_dict, "Resources");
  ASSERT_TRUE(res_dict);

  FPDF_DICTIONARY extg_dict = FPDF_DictionaryGetDict(res_dict, "ExtGState");
  EXPECT_NE(nullptr, extg_dict);

  FPDF_DICTIONARY g0_dict = FPDF_DictionaryGetDict(extg_dict, "G0");
  EXPECT_NE(nullptr, g0_dict);
}

TEST_F(FPDFDictEmbedderTest, GetIntFromNestedDict) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  FPDF_DICTIONARY res_dict = FPDF_DictionaryGetDict(page_dict, "Resources");
  FPDF_DICTIONARY extg_dict = FPDF_DictionaryGetDict(res_dict, "ExtGState");
  FPDF_DICTIONARY g0_dict = FPDF_DictionaryGetDict(extg_dict, "G0");
  ASSERT_TRUE(g0_dict);

  FPDF_RESULT_INT lc_res = FPDF_DictionaryGetInt(g0_dict, "LC");
  EXPECT_TRUE(lc_res.success);
  EXPECT_EQ(0, lc_res.value);
}

TEST_F(FPDFDictEmbedderTest, GetBoolFromNestedDict) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  FPDF_DICTIONARY res_dict = FPDF_DictionaryGetDict(page_dict, "Resources");
  FPDF_DICTIONARY extg_dict = FPDF_DictionaryGetDict(res_dict, "ExtGState");
  FPDF_DICTIONARY g0_dict = FPDF_DictionaryGetDict(extg_dict, "G0");
  ASSERT_TRUE(g0_dict);

  FPDF_RESULT_BOOL sa_res = FPDF_DictionaryGetBool(g0_dict, "SA");
  EXPECT_TRUE(sa_res.success);
  EXPECT_TRUE(sa_res.value);
}

TEST_F(FPDFDictEmbedderTest, GetFloatFromNestedDict) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  FPDF_DICTIONARY res_dict = FPDF_DictionaryGetDict(page_dict, "Resources");
  FPDF_DICTIONARY extg_dict = FPDF_DictionaryGetDict(res_dict, "ExtGState");
  FPDF_DICTIONARY g0_dict = FPDF_DictionaryGetDict(extg_dict, "G0");
  ASSERT_TRUE(g0_dict);

  FPDF_RESULT_FLOAT ml_res = FPDF_DictionaryGetFloat(g0_dict, "ML");
  EXPECT_TRUE(ml_res.success);
  EXPECT_FLOAT_EQ(4.0f, ml_res.value);
}

TEST_F(FPDFDictEmbedderTest, GetNonExistentKeyFails) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  ASSERT_TRUE(page_dict);

  unsigned short buf[64];
  FPDF_DictionaryGetString(page_dict, "NotARealKey", buf, sizeof(buf));
  EXPECT_EQ(L"", GetPlatformWString(buf));

  FPDF_RESULT_INT fail_res = FPDF_DictionaryGetInt(page_dict, "NotARealKey");
  EXPECT_FALSE(fail_res.success);
}

TEST_F(FPDFDictEmbedderTest, GetFontResourceNames) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  FPDF_DICTIONARY res_dict = FPDF_DictionaryGetDict(page_dict, "Resources");
  FPDF_DICTIONARY font_dict = FPDF_DictionaryGetDict(res_dict, "Font");
  ASSERT_TRUE(font_dict);

  unsigned short buf[64];
  FPDF_DICTIONARY f1_dict = FPDF_DictionaryGetDict(font_dict, "F1");
  FPDF_DictionaryGetString(f1_dict, "BaseFont", buf, sizeof(buf));
  EXPECT_EQ(L"Times-Roman", GetPlatformWString(buf));

  FPDF_DICTIONARY f2_dict = FPDF_DictionaryGetDict(font_dict, "F2");
  FPDF_DictionaryGetString(f2_dict, "BaseFont", buf, sizeof(buf));
  EXPECT_EQ(L"Helvetica", GetPlatformWString(buf));
}

TEST_F(FPDFDictEmbedderTest, GetFontSubtype) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  FPDF_DICTIONARY res_dict = FPDF_DictionaryGetDict(page_dict, "Resources");
  FPDF_DICTIONARY font_dict = FPDF_DictionaryGetDict(res_dict, "Font");
  FPDF_DICTIONARY f1_dict = FPDF_DictionaryGetDict(font_dict, "F1");

  unsigned short buf[64];
  FPDF_DictionaryGetString(f1_dict, "Subtype", buf, sizeof(buf));
  EXPECT_EQ(L"Type1", GetPlatformWString(buf));
}

TEST_F(FPDFDictEmbedderTest, GetPageCountFromDict) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  FPDF_DICTIONARY parent_dict = FPDF_DictionaryGetDict(page_dict, "Parent");
  ASSERT_TRUE(parent_dict);

  FPDF_RESULT_INT count = FPDF_DictionaryGetInt(parent_dict, "Count");
  EXPECT_TRUE(count.success);
  EXPECT_EQ(1, count.value);
}
