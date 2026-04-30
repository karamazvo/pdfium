// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdf_dict.h"
#include "testing/embedder_test.h"
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

  FPDF_RESULT_STRING type_res = FPDF_DictionaryGetString(page_dict, "Type");
  EXPECT_TRUE(type_res.success);
  EXPECT_STREQ("Page", reinterpret_cast<const char*>(type_res.value));
}

TEST_F(FPDFDictEmbedderTest, GetRectFromPageDict) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  ASSERT_TRUE(page_dict);

  // Expected MediaBox: [0 0 612 792]
  FS_RECTF media_box = FPDF_DictionaryGetRect(page_dict, "MediaBox");
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

  // Check /LC 0
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

  // Check /SA true
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

  // Check /ML 4
  FPDF_RESULT_FLOAT ml_res = FPDF_DictionaryGetFloat(g0_dict, "ML");
  EXPECT_TRUE(ml_res.success);
  EXPECT_FLOAT_EQ(4.0f, ml_res.value);
}

TEST_F(FPDFDictEmbedderTest, GetNonExistentKeyFails) {
  ASSERT_TRUE(OpenDocument("about_blank.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  ASSERT_TRUE(page_dict);

  FPDF_RESULT_INT fail_res = FPDF_DictionaryGetInt(page_dict, "NotARealKey");
  EXPECT_FALSE(fail_res.success);
}

TEST_F(FPDFDictEmbedderTest, GetFontResourceNames) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  ASSERT_TRUE(page_dict);

  FPDF_DICTIONARY res_dict = FPDF_DictionaryGetDict(page_dict, "Resources");
  ASSERT_TRUE(res_dict);
  FPDF_DICTIONARY font_dict = FPDF_DictionaryGetDict(res_dict, "Font");
  ASSERT_TRUE(font_dict);

  FPDF_DICTIONARY f1_dict = FPDF_DictionaryGetDict(font_dict, "F1");
  ASSERT_TRUE(f1_dict);
  FPDF_RESULT_STRING f1_base = FPDF_DictionaryGetString(f1_dict, "BaseFont");
  EXPECT_TRUE(f1_base.success);
  EXPECT_STREQ("Times-Roman", reinterpret_cast<const char*>(f1_base.value));

  FPDF_DICTIONARY f2_dict = FPDF_DictionaryGetDict(font_dict, "F2");
  ASSERT_TRUE(f2_dict);
  FPDF_RESULT_STRING f2_base = FPDF_DictionaryGetString(f2_dict, "BaseFont");
  EXPECT_TRUE(f2_base.success);
  EXPECT_STREQ("Helvetica", reinterpret_cast<const char*>(f2_base.value));
}

TEST_F(FPDFDictEmbedderTest, GetFontSubtype) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_DICTIONARY page_dict = FPDF_GetPageDictionary(document(), 0);
  FPDF_DICTIONARY res_dict = FPDF_DictionaryGetDict(page_dict, "Resources");
  FPDF_DICTIONARY font_dict = FPDF_DictionaryGetDict(res_dict, "Font");
  FPDF_DICTIONARY f1_dict = FPDF_DictionaryGetDict(font_dict, "F1");
  ASSERT_TRUE(f1_dict);

  FPDF_RESULT_STRING subtype = FPDF_DictionaryGetString(f1_dict, "Subtype");
  EXPECT_TRUE(subtype.success);
  EXPECT_STREQ("Type1", reinterpret_cast<const char*>(subtype.value));
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
