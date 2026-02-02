// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/embedder_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class FPDFJxlDecodeEmbedderTest : public EmbedderTest {};

}  // namespace

TEST_F(FPDFJxlDecodeEmbedderTest, JxlDecodeRenders) {
#if !defined(PDF_ENABLE_RUST_JXL)
  GTEST_SKIP() << "Built without PDF_ENABLE_RUST_JXL";
#else
  ASSERT_TRUE(OpenDocument("jxl_decode.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  // Expectation lives at testing/resources/embedder_tests/jxl_decode.png
  CompareBitmapToPng(bitmap.get(), "jxl_decode");
#endif
}

TEST_F(FPDFJxlDecodeEmbedderTest, JxlDecodeDiceAlphaRenders) {
#if !defined(PDF_ENABLE_RUST_JXL)
  GTEST_SKIP() << "Built without PDF_ENABLE_RUST_JXL";
#else
  ASSERT_TRUE(OpenDocument("jxl_dice_decode.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  // Expectation lives at testing/resources/embedder_tests/jxl_dice_decode.png
  CompareBitmapToPng(bitmap.get(), "jxl_dice_decode");
#endif
}

TEST_F(FPDFJxlDecodeEmbedderTest, JxlDecodeAnimIcosRendersFirstFrame) {
#if !defined(PDF_ENABLE_RUST_JXL)
  GTEST_SKIP() << "Built without PDF_ENABLE_RUST_JXL";
#else
  ASSERT_TRUE(OpenDocument("jxl_anim_icos_decode.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  // Expectation PNG: testing/resources/embedder_tests/jxl_anim_icos_decode.png
  // This should correspond to the first frame only.
  CompareBitmapToPng(bitmap.get(), "jxl_anim_icos_decode");
#endif
}
