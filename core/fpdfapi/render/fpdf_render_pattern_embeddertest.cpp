// Copyright 2015 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include "public/cpp/fpdf_scopers.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

class FPDFRenderPatternEmbedderTest : public EmbedderTest {};

static const std::string& GetPathPrefix() {
  static const std::string path_prefix =
      "CompareBitmapWithImage/FPDFRenderPatternEmbedderTest/";
  return path_prefix;
}

TEST_F(FPDFRenderPatternEmbedderTest, LoadError547706) {
  // Test shading where object is a dictionary instead of a stream.
  ASSERT_TRUE(OpenDocument("bug_547706.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  CompareBitmapWithImage(
      bitmap.get(), 612, 792,
      (GetPathPrefix() + "LoadError547706_expected.pdf.0.png").c_str());
}
