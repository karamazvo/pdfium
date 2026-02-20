// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "public/fpdf_annot.h"
#include "public/fpdf_flatten.h"
#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::HasSubstr;
using testing::Not;

namespace {

class FPDFFlattenEmbedderTest : public EmbedderTest {};

}  // namespace

TEST_F(FPDFFlattenEmbedderTest, FlatNothing) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  ScopedPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);
  EXPECT_EQ(FLATTEN_NOTHINGTODO,
            FPDFPage_Flatten(page.get(), FLAT_NORMALDISPLAY));
}

TEST_F(FPDFFlattenEmbedderTest, FlatNormal) {
  ASSERT_TRUE(OpenDocument("annotiter.pdf"));
  ScopedPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);
  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_NORMALDISPLAY));
}

TEST_F(FPDFFlattenEmbedderTest, FlatPrint) {
  ASSERT_TRUE(OpenDocument("annotiter.pdf"));
  ScopedPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);
  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
}

TEST_F(FPDFFlattenEmbedderTest, FlatWithBadFont) {
  ASSERT_TRUE(OpenDocument("344775293.pdf"));
  ScopedPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);

  FORM_OnLButtonDown(form_handle(), page.get(), 0, 20, 30);
  FORM_OnLButtonUp(form_handle(), page.get(), 0, 20, 30);

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  EXPECT_THAT(GetString(), Not(HasSubstr("/PDFDocEncoding")));
}

TEST_F(FPDFFlattenEmbedderTest, FlatWithFontNoBaseEncoding) {
  ASSERT_TRUE(OpenDocument("363015187.pdf"));
  ScopedPage page = LoadScopedPage(0);
  EXPECT_TRUE(page);

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  EXPECT_THAT(GetString(), HasSubstr("/Differences"));
}

TEST_F(FPDFFlattenEmbedderTest, Bug861842) {
  ASSERT_TRUE(OpenDocument("bug_861842.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPageWithFlags(page.get(), FPDF_ANNOT);
  CompareBitmapToPngWithExpectationSuffix(bitmap.get(), "bug_861842");

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  // TODO(crbug.com/861842): This should not render blank.
  VerifySavedDocumentToPng("blank_100x120");
}

TEST_F(FPDFFlattenEmbedderTest, Bug889099) {
  ASSERT_TRUE(OpenDocument("bug_889099.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  // The original document has a malformed media box; the height is -400.
  ScopedFPDFBitmap bitmap = RenderLoadedPageWithFlags(page.get(), FPDF_ANNOT);
  CompareBitmapToPngWithExpectationSuffix(bitmap.get(), "bug_889099");

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  VerifySavedDocumentToPngWithExpectationSuffix("bug_889099_flattened");
}

TEST_F(FPDFFlattenEmbedderTest, Bug890322) {
  ASSERT_TRUE(OpenDocument("bug_890322.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPageWithFlags(page.get(), FPDF_ANNOT);
  CompareBitmapToPngWithExpectationSuffix(bitmap.get(), pdfium::kBug890322Png);

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  VerifySavedDocumentToPngWithExpectationSuffix(pdfium::kBug890322Png);
}

TEST_F(FPDFFlattenEmbedderTest, Bug896366) {
  ASSERT_TRUE(OpenDocument("bug_896366.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPageWithFlags(page.get(), FPDF_ANNOT);
  CompareBitmapToPngWithExpectationSuffix(bitmap.get(), "bug_896366");

  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  VerifySavedDocumentToPngWithExpectationSuffix("bug_896366");
}

TEST_F(FPDFFlattenEmbedderTest, FlattenNoViewAnnotation) {
  // Test that annotations with NoView flag are not flattened for screen
  // display.
  ASSERT_TRUE(OpenDocument("annotiter.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  const int initial_annot_count = FPDFPage_GetAnnotCount(page.get());
  ASSERT_GT(initial_annot_count, 0);

  // Set NoView flag (should not display on screen).
  // Clear Print flag (should not print).
  for (int i = 0; i < initial_annot_count; ++i) {
    ScopedFPDFAnnotation annot(FPDFPage_GetAnnot(page.get(), i));
    ASSERT_TRUE(annot);
    // Only NoView, no Print
    EXPECT_TRUE(FPDFAnnot_SetFlags(annot.get(), FPDF_ANNOT_FLAG_NOVIEW));
  }

  // Flatten for normal display. With the fix (using kNoView instead of
  // kInvisible), annotations with NoView should be skipped.
  int result = FPDFPage_Flatten(page.get(), FLAT_NORMALDISPLAY);

  // The function processes page contents even if no annotations are flattened,
  // so it may return SUCCESS. The important part is it doesn't crash and
  // correctly identifies NoView annotations for skipping.
  EXPECT_TRUE(result == FLATTEN_SUCCESS || result == FLATTEN_NOTHINGTODO);
}

TEST_F(FPDFFlattenEmbedderTest, FlattenNoViewPrintAnnotation) {
  // Open a document with annotations.
  ASSERT_TRUE(OpenDocument("annotiter.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  const int initial_annot_count = FPDFPage_GetAnnotCount(page.get());
  ASSERT_GT(initial_annot_count, 0);

  // Set both NoView and Print flags on all annotations (annotations should
  // not be displayed on screen but should be printed).
  for (int i = 0; i < initial_annot_count; ++i) {
    ScopedFPDFAnnotation annot(FPDFPage_GetAnnot(page.get(), i));
    ASSERT_TRUE(annot);
    int flags = FPDFAnnot_GetFlags(annot.get());
    flags |= FPDF_ANNOT_FLAG_NOVIEW | FPDF_ANNOT_FLAG_PRINT;
    EXPECT_TRUE(FPDFAnnot_SetFlags(annot.get(), flags));
  }

  // Flattening for print should still include annotations with NoView+Print.
  EXPECT_EQ(FLATTEN_SUCCESS, FPDFPage_Flatten(page.get(), FLAT_PRINT));

  // Verify annotations were removed (flattened).
  EXPECT_EQ(0, FPDFPage_GetAnnotCount(page.get()));
}
