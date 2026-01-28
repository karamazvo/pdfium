// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <vector>

#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/zip.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;

namespace fxcrt {
namespace {

struct NoCopyNoMove {
  NoCopyNoMove(int val) : value(val) {}
  NoCopyNoMove(const NoCopyNoMove&) { NOTREACHED(); }
  NoCopyNoMove(NoCopyNoMove&&) { NOTREACHED(); }
  NoCopyNoMove& operator=(const NoCopyNoMove&) {
    NOTREACHED();
    return *this;
  }
  NoCopyNoMove& operator=(NoCopyNoMove&&) {
    NOTREACHED();
    return *this;
  }

  int value;
};

}  // namespace

TEST(ZipTest, EmptyZip) {
  std::vector<int> a;
  std::vector<int> b;
  std::vector<int> c;

  for (auto [x, y, z] : Zip(a, b, c)) {
    x *= 10;
    y *= 10;
    z *= 10;
  }

  EXPECT_TRUE(a.empty());
  EXPECT_TRUE(b.empty());
  EXPECT_TRUE(c.empty());

  b.push_back(1);

  for (auto [x, y, z] : Zip(a, b, c)) {
    x *= 10;
    y *= 10;
    z *= 10;
  }

  EXPECT_TRUE(a.empty());
  EXPECT_THAT(b, ElementsAre(1));
  EXPECT_TRUE(c.empty());
}

TEST(Zip, ActualZip) {
  const int stuff[] = {1, 2, 3};
  const int expected[] = {1, 2, 3, 0};
  {
    int output[4] = {};
    for (auto [in, out] : Zip(stuff, output)) {
      out = in;
    }
    EXPECT_THAT(output, ElementsAreArray(expected));
  }
  {
    // Test that ordering of args doesn't matter, except for the size
    // determination.
    int output[4] = {};
    auto sub_output = pdfium::span(output).first<3u>();
    for (auto [out, in] : Zip(sub_output, stuff)) {
      out = in;
    }
    EXPECT_THAT(output, ElementsAreArray(expected));
  }
}

TEST(Zip, ActualZip3) {
  const int stuff1[] = {1, 2, 3};
  const int stuff2[] = {4, 5, 6};
  const int expected[] = {5, 7, 9, 0};
  {
    int output[4] = {};
    for (auto [in1, in2, out] : Zip(stuff1, stuff2, output)) {
      out = in1 + in2;
    }
    EXPECT_THAT(output, ElementsAreArray(expected));
  }
  {
    // Test that ordering of args doesn't matter.
    int output[4] = {};
    for (auto [in1, out, in2] : Zip(stuff1, output, stuff2)) {
      out = in1 + in2;
    }
    EXPECT_THAT(output, ElementsAreArray(expected));
  }
  {
    // Test that ordering of args doesn't matter, except for the size
    // determination.
    int output[4] = {};
    auto sub_output = pdfium::span(output).first<3u>();
    for (auto [out, in1, in2] : Zip(sub_output, stuff1, stuff2)) {
      out = in1 + in2;
    }
    EXPECT_THAT(output, ElementsAreArray(expected));
  }
}

TEST(Zip, NoCopy) {
  // Test that copies do not silently happen when zipping.
  const NoCopyNoMove stuff1[] = {{1}, {2}, {3}};
  const NoCopyNoMove expected[] = {{1}, {2}, {3}};
  for (auto [in1, exp] : Zip(stuff1, expected)) {
    EXPECT_EQ(exp.value, in1.value);
  }
}

TEST(Zip, NoCopy3) {
  // Test that copies do not silently happen when zipping.
  const NoCopyNoMove stuff1[] = {{1}, {2}, {3}};
  const NoCopyNoMove stuff2[] = {{4}, {5}, {6}};
  const NoCopyNoMove expected[] = {{5}, {7}, {9}};
  for (auto [in1, in2, exp] : Zip(stuff1, stuff2, expected)) {
    EXPECT_EQ(exp.value, in1.value + in2.value);
  }
}

TEST(Zip, BadArgumentsZip) {
  pdfium::span<const int> nothing;
  int stuff[] = {1, 2, 3};

  auto ranges = Zip(stuff, nothing);
  auto it = ranges.begin();
  EXPECT_DEATH(std::advance(it, 1), ".*");
}

TEST(Zip, BadArgumentsZip3) {
  pdfium::span<const int> nothing;
  int stuff[] = {1, 2, 3};

  auto ranges = Zip(stuff, stuff, nothing);
  auto it = ranges.begin();
  EXPECT_DEATH(std::advance(it, 1), ".*");
}

}  // namespace fxcrt
