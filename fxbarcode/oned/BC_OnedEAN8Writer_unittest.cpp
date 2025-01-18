// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxbarcode/oned/BC_OnedEAN8Writer.h"

#include <string>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/data_vector.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(OnedEAN8WriterTest, Encode) {
  CBC_OnedEAN8Writer writer;
  writer.InitEANWriter();

  // EAN-8 barcodes encode 8-digit numbers into 67 modules in a unidimensional
  // disposition.
  EXPECT_TRUE(writer.Encode("").empty());
  EXPECT_TRUE(writer.Encode("123").empty());
  EXPECT_TRUE(writer.Encode("1234567").empty());
  EXPECT_TRUE(writer.Encode("123456789").empty());

  static constexpr char kExpected1[] =
      "# #"      // Start
      "  ##  #"  // 1 L
      "  #  ##"  // 2 L
      " #### #"  // 3 L
      " #   ##"  // 4 L
      " # # "    // Middle
      "#  ### "  // 5 R
      "# #    "  // 6 R
      "#   #  "  // 7 R
      "###  # "  // 0 R
      "# #";     // End
  static constexpr size_t kExpected1Len =
      std::char_traits<char>::length(kExpected1);
  DataVector<uint8_t> encoded = writer.Encode("12345670");
  ASSERT_EQ(kExpected1Len, encoded.size());
  for (size_t i = 0; i < kExpected1Len; i++) {
    UNSAFE_TODO(EXPECT_EQ(kExpected1[i] != ' ', !!encoded[i])) << i;
  }
  static constexpr char kExpected2[] =
      "# #"      // Start
      "   # ##"  // 9 L
      "   # ##"  // 9 L
      " #   ##"  // 4 L
      " #   ##"  // 4 L
      " # # "    // Middle
      "##  ## "  // 1 R
      "##  ## "  // 1 R
      "###  # "  // 0 R
      "# ###  "  // 4 R
      "# #";     // End
  static constexpr size_t kExpected2Len =
      std::char_traits<char>::length(kExpected2);
  encoded = writer.Encode("99441104");
  ASSERT_EQ(kExpected2Len, encoded.size());
  for (size_t i = 0; i < kExpected2Len; i++) {
    UNSAFE_TODO(EXPECT_EQ(kExpected2[i] != ' ', !!encoded[i])) << i;
  }
}

TEST(OnedEAN8WriterTest, Checksum) {
  CBC_OnedEAN8Writer writer;
  writer.InitEANWriter();
  EXPECT_EQ(0, writer.CalcChecksum(""));
  EXPECT_EQ(6, writer.CalcChecksum("123"));
  EXPECT_EQ(0, writer.CalcChecksum("1234567"));
  EXPECT_EQ(4, writer.CalcChecksum("9944110"));
}

}  // namespace
