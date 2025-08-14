// Copyright 2022 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>

#include "core/fxcrt/fx_safe_types.h"
#include "testing/gtest/include/gtest/gtest.h"

// PDFium relies on safe types handling the --2147483648 boundary
// condition without overflow.
TEST(FXSafeTypes, UnaryMinus) {
  FX_SAFE_INT32 safe_val = std::numeric_limits<int32_t>::min();
  EXPECT_TRUE(safe_val.IsValid());
  EXPECT_FALSE((-safe_val).IsValid());
}

TEST(FXSafeTypes, SubtractFromZero) {
  FX_SAFE_INT32 safe_val = std::numeric_limits<int32_t>::min();
  EXPECT_TRUE(safe_val.IsValid());
  EXPECT_FALSE((0 - safe_val).IsValid());
}

TEST(FXSafeTypes, Addition) {
  FX_SAFE_UINT32 safe_val = std::numeric_limits<uint32_t>::max();
  EXPECT_TRUE(safe_val.IsValid());
  EXPECT_FALSE((safe_val + 1).IsValid());

  FX_SAFE_INT32 safe_val2 = std::numeric_limits<int32_t>::max();
  EXPECT_TRUE(safe_val2.IsValid());
  EXPECT_FALSE((safe_val2 + 1).IsValid());
}

TEST(FXSafeTypes, Multiplication) {
  FX_SAFE_UINT32 safe_val = std::numeric_limits<uint32_t>::max();
  EXPECT_TRUE(safe_val.IsValid());
  EXPECT_FALSE((safe_val * 2).IsValid());

  FX_SAFE_INT32 safe_val2 = std::numeric_limits<int32_t>::max();
  EXPECT_TRUE(safe_val2.IsValid());
  EXPECT_FALSE((safe_val2 * 2).IsValid());
}

TEST(FXSafeTypes, SizeT) {
  FX_SAFE_SIZE_T safe_val = std::numeric_limits<size_t>::max();
  EXPECT_TRUE(safe_val.IsValid());
  EXPECT_FALSE((safe_val + 1).IsValid());
}
