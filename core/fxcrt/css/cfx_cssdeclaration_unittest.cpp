// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/css/cfx_cssdeclaration.h"

#include <optional>

#include "core/fxcrt/css/cfx_csscolorvalue.h"
#include "core/fxcrt/css/cfx_cssstringvalue.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CFXCSSDeclarationTest, ParseCSSString) {
  EXPECT_EQ(L"abc", CFX_CSSDeclaration::ParseCSSString(L"\"abc\""));
  EXPECT_EQ(L"abc", CFX_CSSDeclaration::ParseCSSString(L"'abc'"));
  EXPECT_EQ(L"abc", CFX_CSSDeclaration::ParseCSSString(L"abc"));
  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSString(L""));
  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSString(L"''"));
  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSString(L"\"\""));
}
