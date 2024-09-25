// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/helpers/compare_rectf.h"

bool Compare_FSRECTF(FS_RECTF& model, FS_RECTF& subject) {
  return model.bottom == subject.bottom && model.left == subject.left &&
         model.right == subject.right && model.top == subject.top;
}
