// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdf_sysfontinfo.h"

#include <string>
#include <vector>

#include "build/build_config.h"
#include "core/fxcrt/compiler_specific.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Test state for tracking callback invocations
struct FontCallbackTracker {
  int enum_fonts_call_count = 0;
  int map_font_call_count = 0;
  FPDF_SYSFONTINFO* wrapped_font_info = nullptr;
};

FontCallbackTracker& GetTracker() {
  static FontCallbackTracker tracker;
  return tracker;
}

extern "C" {

void FakeRelease(FPDF_SYSFONTINFO* pThis) {}
void FakeEnumFonts(FPDF_SYSFONTINFO* pThis, void* pMapper) {}

void* FakeMapFont(FPDF_SYSFONTINFO* pThis,
                  int weight,
                  FPDF_BOOL bItalic,
                  int charset,
                  int pitch_family,
                  const char* face,
                  FPDF_BOOL* bExact) {
  // Any non-null return will do.
  return pThis;
}

// Tracking wrapper functions that delegate to real system font info
void TrackingEnumFonts(FPDF_SYSFONTINFO* pThis, void* pMapper) {
  FontCallbackTracker& tracker = GetTracker();
  tracker.enum_fonts_call_count++;
  if (tracker.wrapped_font_info && tracker.wrapped_font_info->EnumFonts) {
    tracker.wrapped_font_info->EnumFonts(tracker.wrapped_font_info, pMapper);
  }
}

void* TrackingMapFont(FPDF_SYSFONTINFO* pThis,
                      int weight,
                      FPDF_BOOL bItalic,
                      int charset,
                      int pitch_family,
                      const char* face,
                      FPDF_BOOL* bExact) {
  FontCallbackTracker& tracker = GetTracker();
  tracker.map_font_call_count++;
  if (tracker.wrapped_font_info && tracker.wrapped_font_info->MapFont) {
    return tracker.wrapped_font_info->MapFont(tracker.wrapped_font_info, weight,
                                              bItalic, charset, pitch_family,
                                              face, bExact);
  }
  return nullptr;
}

void* TrackingGetFont(FPDF_SYSFONTINFO* pThis, const char* face) {
  FontCallbackTracker& tracker = GetTracker();
  if (tracker.wrapped_font_info && tracker.wrapped_font_info->GetFont) {
    return tracker.wrapped_font_info->GetFont(tracker.wrapped_font_info, face);
  }
  return nullptr;
}

unsigned long TrackingGetFontData(FPDF_SYSFONTINFO* pThis,
                                  void* hFont,
                                  unsigned int table,
                                  unsigned char* buffer,
                                  unsigned long buf_size) {
  FontCallbackTracker& tracker = GetTracker();
  if (tracker.wrapped_font_info && tracker.wrapped_font_info->GetFontData) {
    return tracker.wrapped_font_info->GetFontData(
        tracker.wrapped_font_info, hFont, table, buffer, buf_size);
  }
  return 0;
}

unsigned long TrackingGetFaceName(FPDF_SYSFONTINFO* pThis,
                                  void* hFont,
                                  char* buffer,
                                  unsigned long buf_size) {
  FontCallbackTracker& tracker = GetTracker();
  if (tracker.wrapped_font_info && tracker.wrapped_font_info->GetFaceName) {
    return tracker.wrapped_font_info->GetFaceName(tracker.wrapped_font_info,
                                                  hFont, buffer, buf_size);
  }
  return 0;
}

int TrackingGetFontCharset(FPDF_SYSFONTINFO* pThis, void* hFont) {
  FontCallbackTracker& tracker = GetTracker();
  if (tracker.wrapped_font_info && tracker.wrapped_font_info->GetFontCharset) {
    return tracker.wrapped_font_info->GetFontCharset(tracker.wrapped_font_info,
                                                     hFont);
  }
  return 0;
}

void TrackingDeleteFont(FPDF_SYSFONTINFO* pThis, void* hFont) {
  FontCallbackTracker& tracker = GetTracker();
  if (tracker.wrapped_font_info && tracker.wrapped_font_info->DeleteFont) {
    tracker.wrapped_font_info->DeleteFont(tracker.wrapped_font_info, hFont);
  }
}

void* FakeGetFont(FPDF_SYSFONTINFO* pThis, const char* face) {
  // Any non-null return will do.
  return pThis;
}

unsigned long FakeGetFontData(FPDF_SYSFONTINFO* pThis,
                              void* hFont,
                              unsigned int table,
                              unsigned char* buffer,
                              unsigned long buf_size) {
  return 0;
}

unsigned long FakeGetFaceName(FPDF_SYSFONTINFO* pThis,
                              void* hFont,
                              char* buffer,
                              unsigned long buf_size) {
  return 0;
}

int FakeGetFontCharset(FPDF_SYSFONTINFO* pThis, void* hFont) {
  return 1;
}

void FakeDeleteFont(FPDF_SYSFONTINFO* pThis, void* hFont) {}

}  // extern "C"

class FPDFPerRequestFontMatchingTest : public EmbedderTest {
 public:
  FPDFPerRequestFontMatchingTest() = default;
  ~FPDFPerRequestFontMatchingTest() override = default;

  void SetUp() override {
    EmbedderTest::SetUp();
    // Get the default system font info
    font_info_ = FPDF_GetDefaultSystemFontInfo();
    ASSERT_TRUE(font_info_);
    // Modify version to 2 for per-request font matching
    font_info_->version = 2;
    FPDF_SetSystemFontInfo(font_info_);
  }

  void TearDown() override {
    EmbedderTest::TearDown();

    // After releasing font_info_ from PDFium, it is safe to free it.
    FPDF_SetSystemFontInfo(nullptr);
    FPDF_FreeDefaultSystemFontInfo(font_info_);

    // Bouncing the library is the only reliable way to fully undo the initial
    // FPDF_SetSystemFontInfo() call at the moment.
    EmbedderTestEnvironment::GetInstance()->TearDown();
    EmbedderTestEnvironment::GetInstance()->SetUp();
  }

  FPDF_SYSFONTINFO* font_info_;
};

class FPDFUnavailableSysFontInfoEmbedderTest : public EmbedderTest {
 public:
  FPDFUnavailableSysFontInfoEmbedderTest() = default;
  ~FPDFUnavailableSysFontInfoEmbedderTest() override = default;

  void SetUp() override {
    EmbedderTest::SetUp();
    font_info_.version = 1;
    font_info_.Release = FakeRelease;
    font_info_.EnumFonts = FakeEnumFonts;
    font_info_.MapFont = FakeMapFont;
    font_info_.GetFont = FakeGetFont;
    font_info_.GetFontData = FakeGetFontData;
    font_info_.GetFaceName = FakeGetFaceName;
    font_info_.GetFontCharset = FakeGetFontCharset;
    font_info_.DeleteFont = FakeDeleteFont;
    FPDF_SetSystemFontInfo(&font_info_);
  }

  void TearDown() override {
    FPDF_SetSystemFontInfo(nullptr);
    EmbedderTest::TearDown();

    // Bouncing the library is the only reliable way to fully undo the initial
    // FPDF_SetSystemFontInfo() call at the moment.
    EmbedderTestEnvironment::GetInstance()->TearDown();
    EmbedderTestEnvironment::GetInstance()->SetUp();
  }

  FPDF_SYSFONTINFO font_info_;
};

class FPDFSysFontInfoEmbedderTest : public EmbedderTest {
 public:
  FPDFSysFontInfoEmbedderTest() = default;
  ~FPDFSysFontInfoEmbedderTest() override = default;

  void SetUp() override {
    EmbedderTest::SetUp();
    font_info_ = FPDF_GetDefaultSystemFontInfo();
    ASSERT_TRUE(font_info_);
    FPDF_SetSystemFontInfo(font_info_);
  }

  void TearDown() override {
    EmbedderTest::TearDown();

    // After releasing `font_info_` from PDFium, it is safe to free it.
    FPDF_SetSystemFontInfo(nullptr);
    FPDF_FreeDefaultSystemFontInfo(font_info_);

    // Bouncing the library is the only reliable way to fully undo the initial
    // FPDF_SetSystemFontInfo() call at the moment.
    EmbedderTestEnvironment::GetInstance()->TearDown();

    EmbedderTestEnvironment::GetInstance()->SetUp();
  }

  FPDF_SYSFONTINFO* font_info_;
};

class FPDFVersion1Versus2ComparisonTest : public EmbedderTest {
 public:
  FPDFVersion1Versus2ComparisonTest() = default;
  ~FPDFVersion1Versus2ComparisonTest() override {
    FontCallbackTracker& tracker = GetTracker();
    if (tracker.wrapped_font_info) {
      FPDF_SetSystemFontInfo(nullptr);
      FPDF_FreeDefaultSystemFontInfo(tracker.wrapped_font_info);
      tracker.wrapped_font_info = nullptr;
    }
  }

  void SetupVersion(int version) {
    FontCallbackTracker& tracker = GetTracker();
    tracker.enum_fonts_call_count = 0;
    tracker.map_font_call_count = 0;

    tracker.wrapped_font_info = FPDF_GetDefaultSystemFontInfo();
    ASSERT_TRUE(tracker.wrapped_font_info);

    tracking_font_info_.version = version;
    tracking_font_info_.Release = nullptr;
    tracking_font_info_.EnumFonts = TrackingEnumFonts;
    tracking_font_info_.MapFont = TrackingMapFont;
    tracking_font_info_.GetFont = TrackingGetFont;
    tracking_font_info_.GetFontData = TrackingGetFontData;
    tracking_font_info_.GetFaceName = TrackingGetFaceName;
    tracking_font_info_.GetFontCharset = TrackingGetFontCharset;
    tracking_font_info_.DeleteFont = TrackingDeleteFont;

    FPDF_SetSystemFontInfo(&tracking_font_info_);
  }

  void TearDown() override {
    FontCallbackTracker& tracker = GetTracker();
    if (tracker.wrapped_font_info) {
      FPDF_SetSystemFontInfo(nullptr);
      FPDF_FreeDefaultSystemFontInfo(tracker.wrapped_font_info);
      tracker.wrapped_font_info = nullptr;
    }
    EmbedderTest::TearDown();
  }

  FPDF_SYSFONTINFO tracking_font_info_ = {};
};

}  // namespace

TEST_F(FPDFPerRequestFontMatchingTest, PerRequestFontMatching) {
  // Verify version 2 works: skips EnumFonts, uses MapFont per-request.
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  ASSERT_EQ(1, FPDF_GetPageCount(document()));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
}

TEST_F(FPDFPerRequestFontMatchingTest, PerRequestWithRendering) {
  // Verify version 2 works end-to-end with rendering.
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  ASSERT_EQ(1, FPDF_GetPageCount(document()));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderPage(page.get());
  ASSERT_EQ(200, FPDFBitmap_GetWidth(bitmap.get()));
  ASSERT_EQ(200, FPDFBitmap_GetHeight(bitmap.get()));
}

TEST_F(FPDFVersion1Versus2ComparisonTest, Version1CallsEnumFonts) {
  SetupVersion(1);

  ASSERT_TRUE(OpenDocument("hello_world.pdf"));

  FontCallbackTracker& tracker = GetTracker();
  // Version 1 should call EnumFonts at least once.
  EXPECT_GT(tracker.enum_fonts_call_count, 0);
}

TEST_F(FPDFVersion1Versus2ComparisonTest, Version2SkipsEnumFonts) {
  SetupVersion(2);

  ASSERT_TRUE(OpenDocument("hello_world.pdf"));

  FontCallbackTracker& tracker = GetTracker();
  // Version 2 should NOT call EnumFonts but should call MapFont.
  EXPECT_EQ(tracker.enum_fonts_call_count, 0);
  EXPECT_GT(tracker.map_font_call_count, 0);
}

TEST_F(FPDFUnavailableSysFontInfoEmbedderTest, Bug972518) {
  ASSERT_TRUE(OpenDocument("bug_972518.pdf"));
  ASSERT_EQ(1, FPDF_GetPageCount(document()));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
}

TEST_F(FPDFSysFontInfoEmbedderTest, DefaultSystemFontInfo) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  ASSERT_EQ(1, FPDF_GetPageCount(document()));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  {
    // Not checking the rendering because it will depend on the fonts installed.
    ScopedFPDFBitmap bitmap = RenderPage(page.get());
    ASSERT_EQ(200, FPDFBitmap_GetWidth(bitmap.get()));
    ASSERT_EQ(200, FPDFBitmap_GetHeight(bitmap.get()));
  }
}

TEST_F(FPDFSysFontInfoEmbedderTest, DefaultTTFMap) {
  static constexpr int kExpectedCharsets[] = {
      FXFONT_ANSI_CHARSET,        FXFONT_SHIFTJIS_CHARSET,
      FXFONT_HANGEUL_CHARSET,     FXFONT_GB2312_CHARSET,
      FXFONT_CHINESEBIG5_CHARSET, FXFONT_ARABIC_CHARSET,
      FXFONT_CYRILLIC_CHARSET,    FXFONT_EASTERNEUROPEAN_CHARSET,
  };
  std::vector<int> charsets;

  const FPDF_CharsetFontMap* cfmap = FPDF_GetDefaultTTFMap();
  ASSERT_TRUE(cfmap);

  // Stop at either end mark.
  while (cfmap->charset != -1 && cfmap->fontname) {
    charsets.push_back(cfmap->charset);
    // SAFETY: requires FPDF_GetDefaultTTFMap() to provide a sentinel.
    UNSAFE_BUFFERS(++cfmap);
  }

  // Confirm end marks only occur as a pair.
  EXPECT_EQ(cfmap->charset, -1);
  EXPECT_EQ(cfmap->fontname, nullptr);

  EXPECT_THAT(charsets, testing::UnorderedElementsAreArray(kExpectedCharsets));
}

TEST_F(FPDFSysFontInfoEmbedderTest, DefaultTTFMapCountAndEntries) {
  static constexpr int kExpectedCharsets[] = {
      FXFONT_ANSI_CHARSET,
      FXFONT_GB2312_CHARSET,
      FXFONT_CHINESEBIG5_CHARSET,
      FXFONT_SHIFTJIS_CHARSET,
      FXFONT_HANGEUL_CHARSET,
      FXFONT_CYRILLIC_CHARSET,
      FXFONT_EASTERNEUROPEAN_CHARSET,
      FXFONT_ARABIC_CHARSET,
  };
  static const std::string kExpectedFontNames[] = {
      "Helvetica", "SimSun", "MingLiU", "MS Gothic", "Batang", "Arial",
#if BUILDFLAG(IS_WIN)
      "Tahoma",
#else
      "Arial",
#endif
      "Arial",
  };
  std::vector<int> charsets;
  std::vector<const char*> font_names;

  const size_t count = FPDF_GetDefaultTTFMapCount();
  for (size_t i = 0; i < count; ++i) {
    const FPDF_CharsetFontMap* entry = FPDF_GetDefaultTTFMapEntry(i);
    ASSERT_TRUE(entry);
    charsets.push_back(entry->charset);
    font_names.push_back(entry->fontname);
  }

  EXPECT_THAT(charsets, testing::ElementsAreArray(kExpectedCharsets));
  EXPECT_THAT(font_names, testing::ElementsAreArray(kExpectedFontNames));

  // Test out of bound indices.
  EXPECT_FALSE(FPDF_GetDefaultTTFMapEntry(count));
  EXPECT_FALSE(FPDF_GetDefaultTTFMapEntry(9999));
}

TEST(FPDFSysFontInfoTest, Version2Accepted) {
  FPDF_SYSFONTINFO font_info = {};
  font_info.version = 2;
  font_info.Release = FakeRelease;
  font_info.EnumFonts = FakeEnumFonts;
  font_info.MapFont = FakeMapFont;
  font_info.GetFont = FakeGetFont;
  font_info.GetFontData = FakeGetFontData;
  font_info.GetFaceName = FakeGetFaceName;
  font_info.GetFontCharset = FakeGetFontCharset;
  font_info.DeleteFont = FakeDeleteFont;

  FPDF_SetSystemFontInfo(&font_info);
  FPDF_SetSystemFontInfo(nullptr);
}

TEST(FPDFSysFontInfoTest, InvalidVersionRejected) {
  FPDF_SYSFONTINFO font_info = {};
  font_info.version = 3;
  font_info.Release = FakeRelease;
  font_info.EnumFonts = FakeEnumFonts;
  font_info.MapFont = FakeMapFont;
  font_info.GetFont = FakeGetFont;
  font_info.GetFontData = FakeGetFontData;
  font_info.GetFaceName = FakeGetFaceName;
  font_info.GetFontCharset = FakeGetFontCharset;
  font_info.DeleteFont = FakeDeleteFont;

  FPDF_SetSystemFontInfo(&font_info);
  FPDF_SetSystemFontInfo(nullptr);
}
