// Copyright 2015 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include "build/build_config.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "public/cpp/fpdf_scopers.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/test_support.h"

using FPDFParserDecodeEmbedderTest = EmbedderTest;

const std::string& GetPathPrefix() {
  static const std::string path_prefix =
      "CompareBitmapWithImage/FPDFParserDecodeEmbedderTest/";
  return path_prefix;
}

TEST_F(FPDFParserDecodeEmbedderTest, Bug552046) {
  // Tests specifying multiple image filters for a stream. Should not cause a
  // crash when rendered.
  ASSERT_TRUE(OpenDocument("bug_552046.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  CompareBitmapWithImage(
      bitmap.get(), 612, 792,
      (GetPathPrefix() + "bug_552046_expected.pdf.0.png").c_str());
}

TEST_F(FPDFParserDecodeEmbedderTest, Bug555784) {
  // Tests bad input to the run length decoder that caused a heap overflow.
  // Should not cause a crash when rendered.
  ASSERT_TRUE(OpenDocument("bug_555784.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  CompareBitmapWithImage(
      bitmap.get(), 612, 792,
      (GetPathPrefix() + "bug_555784_expected.pdf.0.png").c_str());
}

TEST_F(FPDFParserDecodeEmbedderTest, Bug455199) {
  // Tests object numbers with a value > 01000000.
  // Should open successfully.
  ASSERT_TRUE(OpenDocument("bug_455199.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());

  CompareBitmapWithImage(
      bitmap.get(), 200, 200,
      (GetPathPrefix() + "bug_455199_expected.pdf.0.png").c_str());
}
