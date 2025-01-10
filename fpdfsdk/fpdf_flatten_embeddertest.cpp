// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "core/fpdfapi/page/cpdf_annotcontext.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "public/fpdf_flatten.h"
#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::HasSubstr;
using testing::Not;

namespace {

class FPDFFlattenEmbedderTest : public EmbedderTest {};

}  // namespace

TEST_F(FPDFFlattenEmbedderTest, FlatNothing) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);
  EXPECT_EQ(FLATTEN_NOTHINGTODO,
            FPDFPage_Flatten(page.get(), FLAT_NORMALDISPLAY));
}

TEST_F(FPDFFlattenEmbedderTest, FlatNormal) {
  ASSERT_TRUE(OpenDocument("annotiter.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);
  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_NORMALDISPLAY));
}

TEST_F(FPDFFlattenEmbedderTest, FlatPrint) {
  ASSERT_TRUE(OpenDocument("annotiter.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);
  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
}

TEST_F(FPDFFlattenEmbedderTest, FlatWithBadFont) {
  ASSERT_TRUE(OpenDocument("344775293.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);

  FORM_OnLButtonDown(form_handle(), page.get(), 0, 20, 30);
  FORM_OnLButtonUp(form_handle(), page.get(), 0, 20, 30);

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  EXPECT_THAT(GetString(), Not(HasSubstr("/PDFDocEncoding")));
}

TEST_F(FPDFFlattenEmbedderTest, FlatWithFontNoBaseEncoding) {
  ASSERT_TRUE(OpenDocument("363015187.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  EXPECT_THAT(GetString(), HasSubstr("/Differences"));
}

TEST_F(FPDFFlattenEmbedderTest, Bug387312214) {
  ASSERT_TRUE(OpenDocument("bug_387312214.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  {
    // Document has a MediaBox specified incorrectly with the page tree node,
    // instead of with the page object.
    FS_RECTF mediabox;
    EXPECT_FALSE(FPDFPage_GetMediaBox(page.get(), &mediabox.left,
                                      &mediabox.bottom, &mediabox.right,
                                      &mediabox.top));
  }

  // The page dimensions still match what was specified in the MediaBox that
  // was incorrectly located in the page tree node.
  constexpr int kOriginalPageWidth = 375;
  constexpr int kOriginalPageHeight = 445;
  EXPECT_EQ(kOriginalPageWidth, FPDF_GetPageWidthF(page.get()));
  EXPECT_EQ(kOriginalPageHeight, FPDF_GetPageHeightF(page.get()));

  // Add an ink annotation.
  ScopedFPDFAnnotation ink_annot(
      FPDFPage_CreateAnnot(page.get(), FPDF_ANNOT_INK));
  ASSERT_TRUE(ink_annot);
  CPDF_AnnotContext* context =
      CPDFAnnotContextFromFPDFAnnotation(ink_annot.get());
  ASSERT_TRUE(context);
  const CPDF_Dictionary* annot_dict = context->GetAnnotDict();
  ASSERT_TRUE(annot_dict);

  static constexpr FS_POINTF kStroke[] = {{80.0f, 90.0f}, {81.0f, 91.0f},
                                          {82.0f, 92.0f}, {83.0f, 93.0f},
                                          {84.0f, 94.0f}, {85.0f, 95.0f}};
  EXPECT_EQ(
      0, FPDFAnnot_AddInkStroke(ink_annot.get(), kStroke, std::size(kStroke)));

  // Flatten and save a copy of the document.
  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  // Verify that page dimensions are unchanged after flattening.
  EXPECT_EQ(kOriginalPageWidth, FPDF_GetPageWidthF(page.get()));
  EXPECT_EQ(kOriginalPageHeight, FPDF_GetPageHeightF(page.get()));

  {
    // The saved document result now has a non-empty MediaBox entry.
    FS_RECTF mediabox;
    EXPECT_TRUE(FPDFPage_GetMediaBox(page.get(), &mediabox.left,
                                     &mediabox.bottom, &mediabox.right,
                                     &mediabox.top));
    EXPECT_EQ(0, mediabox.left);
    EXPECT_EQ(0, mediabox.bottom);
    // TODO(crbug.com/387312214): Media box `right` and `top` should match the
    // original page's dimensions from before flattening.
    EXPECT_EQ(612, mediabox.right);
    EXPECT_EQ(792, mediabox.top);
  }
}

TEST_F(FPDFFlattenEmbedderTest, Bug861842) {
  const char* checkbox_checksum = []() {
    if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
#if BUILDFLAG(IS_APPLE)
      return "84a527f16649880525a1a8edc6c24c16";
#else
      return "95fdaa000e81c80892b8d370f77be970";
#endif
    }
#if BUILDFLAG(IS_APPLE)
    return "6aafcb2d98da222964bcdbf5aa1f4f1f";
#else
    return "594265790b81df2d93120d33b72a6ada";
#endif
  }();

  ASSERT_TRUE(OpenDocument("bug_861842.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPageWithFlags(page.get(), FPDF_ANNOT);
  CompareBitmap(bitmap.get(), 100, 120, checkbox_checksum);

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));


  // TODO(crbug.com/861842): This should not render blank.
  static constexpr char kBlankPageHash[] = "48400809c3862dae64b0cd00d51057a4";
  VerifySavedDocument(100, 120, kBlankPageHash);
}

TEST_F(FPDFFlattenEmbedderTest, Bug889099) {
  const char* page_checksum = []() {
    if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
#if BUILDFLAG(IS_WIN)
      return "1d83328d2d1ca12b9c9ea5faa62ac515";
#elif BUILDFLAG(IS_APPLE)
      return "3b6f937deec2d27029cbce02111dc065";
#else
      return "de7119d99f42deab2f4215017bdb16af";
#endif
    }
#if BUILDFLAG(IS_APPLE)
    return "049ed3f1e21fc72f929af3410c64bc8f";
#else
    return "3db87245e3f4e37f4cb18654bbe22d97";
#endif
  }();
  const char* flattened_page_checksum = []() {
    if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
#if BUILDFLAG(IS_WIN)
      return "07deccbd4a42aaf6bf45a525f0be388e";
#elif BUILDFLAG(IS_APPLE)
      return "f1bbe115355a2ad6d8ac34c7ff14ba75";
#else
      return "7978c7b3d643a5f0ac0f03ce759c55fe";
#endif
    }
#if BUILDFLAG(IS_APPLE)
    return "41debc60cf2a8f74c710ec6082d77b18";
#else
    return "0832157462ea70fbbf053e14b1d6457f";
#endif
  }();

  ASSERT_TRUE(OpenDocument("bug_889099.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  // The original document has a malformed media box; the height is -400.
  ScopedFPDFBitmap bitmap = RenderLoadedPageWithFlags(page.get(), FPDF_ANNOT);
  CompareBitmap(bitmap.get(), 300, 400, page_checksum);

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));


  VerifySavedDocument(300, 400, flattened_page_checksum);
}

TEST_F(FPDFFlattenEmbedderTest, Bug890322) {
  const char* checksum = []() {
    if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
      return "793689536cf64fe792c2f241888c0cf3";
    }
    return "6c674642154408e877d88c6c082d67e9";
  }();
  ASSERT_TRUE(OpenDocument("bug_890322.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPageWithFlags(page.get(), FPDF_ANNOT);
  CompareBitmap(bitmap.get(), 200, 200, checksum);

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));


  VerifySavedDocument(200, 200, checksum);
}

TEST_F(FPDFFlattenEmbedderTest, Bug896366) {
  const char* checksum = []() {
    if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
      return "c3cccfadc4c5249e6aa0675e511fa4c3";
    }
    return "f71ab085c52c8445ae785eca3ec858b1";
  }();
  ASSERT_TRUE(OpenDocument("bug_896366.pdf"));
  ScopedEmbedderTestPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPageWithFlags(page.get(), FPDF_ANNOT);
  CompareBitmap(bitmap.get(), 612, 792, checksum);

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));


  VerifySavedDocument(612, 792, checksum);
}
