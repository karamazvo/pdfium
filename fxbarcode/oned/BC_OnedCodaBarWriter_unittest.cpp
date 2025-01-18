// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxbarcode/oned/BC_OnedCodaBarWriter.h"

#include <string>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/data_vector.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(OnedCodaBarWriterTest, Encode) {
  CBC_OnedCodaBarWriter writer;

  static constexpr char kExpected1[] =
      "# ##  #  # "  // A Start
      "#  #  # ##";  // B End
  static constexpr size_t kExpected1Len =
      std::char_traits<char>::length(kExpected1);
  DataVector<uint8_t> encoded = writer.Encode("");
  ASSERT_EQ(kExpected1Len, encoded.size());
  for (size_t i = 0; i < kExpected1Len; i++) {
    UNSAFE_TODO(EXPECT_EQ(kExpected1[i] != ' ', !!encoded[i])) << i;
  }
  static constexpr char kExpected2[] =
      "# ##  #  # "  // A Start
      "# # ##  # "   // 1
      "# #  # ## "   // 2
      "##  # # # "   // 3
      "#  #  # ##";  // B End
  static constexpr size_t kExpected2Len =
      std::char_traits<char>::length(kExpected2);
  encoded = writer.Encode("123");
  ASSERT_EQ(kExpected2Len, encoded.size());
  for (size_t i = 0; i < kExpected2Len; i++) {
    UNSAFE_TODO(EXPECT_EQ(kExpected2[i] != ' ', !!encoded[i])) << i;
  }
  static constexpr char kExpected3[] =
      "# ##  #  # "  // A Start
      "# #  ## # "   // -
      "# ##  # # "   // $
      "## ## ## # "  // .
      "## ## # ## "  // /
      "## # ## ## "  // :
      "# ## ## ## "  // +
      "#  #  # ##";  // B End
  static constexpr size_t kExpected3Len =
      std::char_traits<char>::length(kExpected3);
  encoded = writer.Encode("-$./:+");
  ASSERT_EQ(kExpected3Len, encoded.size());
  for (size_t i = 0; i < kExpected3Len; i++) {
    UNSAFE_TODO(EXPECT_EQ(kExpected3[i] != ' ', !!encoded[i])) << i;
  }
  static constexpr char kExpected4[] =
      "# ##  #  # "  // A Start
      "# ## #  # "   // 4
      "## # #  # "   // 5
      "#  # # ## "   // 6
      "## ## ## # "  // .
      "## #  # # "   // 9
      "#  ## # # "   // 8
      "#  # ## # "   // 7
      "## #  # # "   // 9
      "#  ## # # "   // 8
      "#  # ## # "   // 7
      "## #  # # "   // 9
      "#  ## # # "   // 8
      "#  # ## # "   // 7
      "## ## # ## "  // /
      "# # #  ## "   // 0
      "# # #  ## "   // 0
      "# # ##  # "   // 1
      "#  #  # ##";  // B End
  static constexpr size_t kExpected4Len =
      std::char_traits<char>::length(kExpected4);
  encoded = writer.Encode("456.987987987/001");
  ASSERT_EQ(kExpected4Len, encoded.size());
  for (size_t i = 0; i < kExpected4Len; i++) {
    UNSAFE_TODO(EXPECT_EQ(kExpected4[i] != ' ', !!encoded[i])) << i;
  }
}

TEST(OnedCodaBarWriterTest, SetDelimiters) {
  CBC_OnedCodaBarWriter writer;

  EXPECT_TRUE(writer.SetStartChar('A'));
  EXPECT_TRUE(writer.SetStartChar('B'));
  EXPECT_TRUE(writer.SetStartChar('C'));
  EXPECT_TRUE(writer.SetStartChar('D'));
  EXPECT_TRUE(writer.SetStartChar('E'));
  EXPECT_TRUE(writer.SetStartChar('N'));
  EXPECT_TRUE(writer.SetStartChar('T'));
  EXPECT_TRUE(writer.SetStartChar('*'));
  EXPECT_FALSE(writer.SetStartChar('V'));
  EXPECT_FALSE(writer.SetStartChar('0'));
  EXPECT_FALSE(writer.SetStartChar('\0'));
  EXPECT_FALSE(writer.SetStartChar('@'));

  EXPECT_TRUE(writer.SetEndChar('A'));
  EXPECT_TRUE(writer.SetEndChar('B'));
  EXPECT_TRUE(writer.SetEndChar('C'));
  EXPECT_TRUE(writer.SetEndChar('D'));
  EXPECT_TRUE(writer.SetEndChar('E'));
  EXPECT_TRUE(writer.SetEndChar('N'));
  EXPECT_TRUE(writer.SetEndChar('T'));
  EXPECT_TRUE(writer.SetEndChar('*'));
  EXPECT_FALSE(writer.SetEndChar('V'));
  EXPECT_FALSE(writer.SetEndChar('0'));
  EXPECT_FALSE(writer.SetEndChar('\0'));
  EXPECT_FALSE(writer.SetEndChar('@'));

  writer.SetStartChar('N');
  writer.SetEndChar('*');

  static constexpr char kExpected[] =
      "#  #  # ## "  // N (same as B) Start
      "## #  # # "   // 9
      "#  ## # # "   // 8
      "#  # ## # "   // 7
      "# #  #  ##";  // * (same as C) End
  static constexpr size_t kExpectedLen =
      std::char_traits<char>::length(kExpected);
  DataVector<uint8_t> encoded = writer.Encode("987");
  ASSERT_EQ(kExpectedLen, encoded.size());
  for (size_t i = 0; i < kExpectedLen; i++) {
    UNSAFE_TODO(EXPECT_EQ(kExpected[i] != ' ', !!encoded[i])) << i;
  }
}

}  // namespace
