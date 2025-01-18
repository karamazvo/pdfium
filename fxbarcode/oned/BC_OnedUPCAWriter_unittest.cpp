// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxbarcode/oned/BC_OnedUPCAWriter.h"

#include <string>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/data_vector.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(OnedUPCAWriterTest, Encode) {
  CBC_OnedUPCAWriter writer;
  writer.InitEANWriter();

  // UPCA barcodes encode 12-digit numbers into 95 modules in a unidimensional
  // disposition.
  EXPECT_TRUE(writer.Encode("").empty());
  EXPECT_TRUE(writer.Encode("123").empty());
  EXPECT_TRUE(writer.Encode("12345678901").empty());
  EXPECT_TRUE(writer.Encode("1234567890123").empty());

  static constexpr char kExpected1[] =
      "# #"      // Start
      "  ##  #"  // 1 L
      "  #  ##"  // 2 L
      " #### #"  // 3 L
      " #   ##"  // 4 L
      " ##   #"  // 5 L
      " # ####"  // 6 L
      " # # "    // Middle
      "#   #  "  // 7 R
      "#  #   "  // 8 R
      "### #  "  // 9 R
      "###  # "  // 0 R
      "##  ## "  // 1 R
      "## ##  "  // 2 R
      "# #";     // End
  static constexpr size_t kExpected1Len =
      std::char_traits<char>::length(kExpected1);
  DataVector<uint8_t> encoded = writer.Encode("123456789012");
  ASSERT_EQ(kExpected1Len, encoded.size());
  for (size_t i = 0; i < kExpected1Len; i++) {
    UNSAFE_TODO(EXPECT_EQ(kExpected1[i] != ' ', !!encoded[i])) << i;
  }

  encoded = writer.Encode("777666555440");
  static constexpr char kExpected2[] =
      "# #"      // Start
      " ### ##"  // 7 L
      " ### ##"  // 7 L
      " ### ##"  // 7 L
      " # ####"  // 6 L
      " # ####"  // 6 L
      " # ####"  // 6 L
      " # # "    // Middle
      "#  ### "  // 5 R
      "#  ### "  // 5 R
      "#  ### "  // 5 R
      "# ###  "  // 4 R
      "# ###  "  // 4 R
      "###  # "  // 0 R
      "# #";     // End
  static constexpr size_t kExpected2Len =
      std::char_traits<char>::length(kExpected2);
  ASSERT_EQ(kExpected2Len, encoded.size());
  for (size_t i = 0; i < kExpected2Len; i++) {
    UNSAFE_TODO(EXPECT_EQ(kExpected2[i] != ' ', !!encoded[i])) << i;
  }
}

TEST(OnedUPCAWriterTest, Checksum) {
  CBC_OnedUPCAWriter writer;
  writer.InitEANWriter();
  EXPECT_EQ(0, writer.CalcChecksum(""));
  EXPECT_EQ(6, writer.CalcChecksum("123"));
  EXPECT_EQ(2, writer.CalcChecksum("12345678901"));
  EXPECT_EQ(0, writer.CalcChecksum("77766655544"));
}

}  // namespace
