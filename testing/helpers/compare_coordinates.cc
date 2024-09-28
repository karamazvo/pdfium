// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/helpers/compare_coordinates.h"
#include "public/fpdfview.h"
#include "testing/gtest/include/gtest/gtest.h"
void Compare_FS_RECTF(const FS_RECTF& model, const FS_RECTF& subject) {
  EXPECT_FLOAT_EQ(model.left, subject.left);
  EXPECT_FLOAT_EQ(model.top, subject.top);
  EXPECT_FLOAT_EQ(model.right, subject.right);
  EXPECT_FLOAT_EQ(model.top, subject.top);
}
