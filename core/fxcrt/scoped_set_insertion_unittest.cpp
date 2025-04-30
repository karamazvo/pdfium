// Copyright 2021 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/scoped_set_insertion.h"

#include <set>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

TEST(fxcrt, ScopedSetInsertionStl) {
  std::set<int> container;
  {
    ScopedSetInsertion insertion(&container, 5);
    EXPECT_THAT(container, testing::UnorderedElementsAreArray({5}));

    {
      ScopedSetInsertion insertion2(&container, 6);
      EXPECT_THAT(container, testing::UnorderedElementsAreArray({5, 6}));
    }

    EXPECT_THAT(container, testing::UnorderedElementsAreArray({5}));
  }
  EXPECT_TRUE(container.empty());
}

TEST(fxcrt, ScopedSetInsertionAbsl) {
  absl::flat_hash_set<int> container;
  {
    ScopedSetInsertion insertion(&container, 5);
    EXPECT_THAT(container, testing::UnorderedElementsAreArray({5}));

    {
      ScopedSetInsertion insertion2(&container, 6);
      EXPECT_THAT(container, testing::UnorderedElementsAreArray({5, 6}));
    }

    EXPECT_THAT(container, testing::UnorderedElementsAreArray({5}));
  }
  EXPECT_TRUE(container.empty());
}
