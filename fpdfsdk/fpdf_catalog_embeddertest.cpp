// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdf_catalog.h"

#include <vector>

#include "fpdfsdk/cpdfsdk_helpers.h"
#include "public/fpdf_edit.h"
#include "testing/embedder_test.h"
#include "testing/fx_string_testhelpers.h"

namespace {

// "en-US" + NUL in UTF-16LE = 6 * 2 bytes.
constexpr unsigned long kEnUsByteSize = 12;

}  // namespace

using FPDFCatalogTest = EmbedderTest;

TEST_F(FPDFCatalogTest, SetLanguageInvalidDocument) {
  // Document cannot be nullptr.
  ScopedFPDFWideString en_us_str = GetFPDFWideString(L"en-US");
  EXPECT_FALSE(FPDFCatalog_SetLanguage(nullptr, en_us_str.get()));

  // Language cannot be null.
  CreateEmptyDocumentWithoutFormFillEnvironment();
  EXPECT_FALSE(FPDFCatalog_SetLanguage(document(), nullptr));
}

TEST_F(FPDFCatalogTest, SetLanguageNewDocument) {
  CreateEmptyDocumentWithoutFormFillEnvironment();

  ScopedFPDFWideString en_us_str = GetFPDFWideString(L"en-US");
  EXPECT_TRUE(FPDFCatalog_SetLanguage(document(), en_us_str.get()));

  std::vector<FPDF_WCHAR> buffer = GetFPDFWideStringBuffer(kEnUsByteSize);
  EXPECT_EQ(kEnUsByteSize,
            FPDFCatalog_GetLanguage(document(), buffer.data(), kEnUsByteSize));
  EXPECT_EQ(L"en-US", GetPlatformWString(buffer.data()));
}

TEST_F(FPDFCatalogTest, SetLanguageExistingDocument) {
  ASSERT_TRUE(OpenDocument("tagged_table.pdf"));

  // The PDF already has an existing entry for /Lang.
  std::vector<FPDF_WCHAR> buffer = GetFPDFWideStringBuffer(kEnUsByteSize);
  EXPECT_EQ(kEnUsByteSize,
            FPDFCatalog_GetLanguage(document(), buffer.data(), kEnUsByteSize));
  EXPECT_EQ(L"en-US", GetPlatformWString(buffer.data()));

  // Replace the existing entry.
  ScopedFPDFWideString hu_str = GetFPDFWideString(L"hu");
  EXPECT_TRUE(FPDFCatalog_SetLanguage(document(), hu_str.get()));

  EXPECT_EQ(6u,
            FPDFCatalog_GetLanguage(document(), buffer.data(), kEnUsByteSize));
  EXPECT_EQ(L"hu", GetPlatformWString(buffer.data()));
}

TEST_F(FPDFCatalogTest, GetLanguageInvalidDocument) {
  // Document cannot be nullptr.
  EXPECT_EQ(0u, FPDFCatalog_GetLanguage(nullptr, nullptr, 0));

  // New document has no Lang entry.
  CreateEmptyDocumentWithoutFormFillEnvironment();
  EXPECT_EQ(2u, FPDFCatalog_GetLanguage(document(), nullptr, 0));
}

TEST_F(FPDFCatalogTest, GetLanguageRoundTrip) {
  CreateEmptyDocumentWithoutFormFillEnvironment();

  // Set language.
  ScopedFPDFWideString en_us_str = GetFPDFWideString(L"en-US");
  EXPECT_TRUE(FPDFCatalog_SetLanguage(document(), en_us_str.get()));

  // Query size. Expected: "en-US" + NUL in UTF-16LE = 6 * 2 bytes.
  EXPECT_EQ(kEnUsByteSize, FPDFCatalog_GetLanguage(document(), nullptr, 0));

  // Get actual value.
  std::vector<FPDF_WCHAR> buffer = GetFPDFWideStringBuffer(kEnUsByteSize);
  EXPECT_EQ(kEnUsByteSize,
            FPDFCatalog_GetLanguage(document(), buffer.data(), kEnUsByteSize));
  EXPECT_EQ(L"en-US", GetPlatformWString(buffer.data()));
}

TEST_F(FPDFCatalogTest, GetLanguageExistingDocument) {
  ASSERT_TRUE(OpenDocument("tagged_table.pdf"));

  EXPECT_EQ(kEnUsByteSize, FPDFCatalog_GetLanguage(document(), nullptr, 0));

  // Get actual value.
  std::vector<FPDF_WCHAR> buffer = GetFPDFWideStringBuffer(kEnUsByteSize);
  EXPECT_EQ(kEnUsByteSize,
            FPDFCatalog_GetLanguage(document(), buffer.data(), kEnUsByteSize));
  EXPECT_EQ(L"en-US", GetPlatformWString(buffer.data()));
}
