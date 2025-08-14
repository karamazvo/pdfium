// Copyright 2022 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/css/cfx_cssdata.h"

#include "core/fxcrt/bytestring.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CSSDataTest, PropertyHashes) {
  uint32_t max_hash = 0;
#undef CSS_PROP____
#define CSS_PROP____(a, b, c, d)                                       \
  {\
    EXPECT_EQ(FX_HashCode_GetAsIfW(b), static_cast<uint32_t>(c)) << b; \
    EXPECT_GT(static_cast<uint32_t>(c), max_hash) << b;                \
    max_hash = c;                                                      \
  }
#include "core/fxcrt/css/properties.inc"
#undef CSS_PROP____
}

TEST(CSSDataTest, PropertyValueHashes) {
  uint32_t max_hash = 0;
#undef CSS_PROP_VALUE____
#define CSS_PROP_VALUE____(a, b, c)                                    \
  {\
    EXPECT_EQ(FX_HashCode_GetAsIfW(b), static_cast<uint32_t>(c)) << b; \
    EXPECT_GT(static_cast<uint32_t>(c), max_hash) << b;                \
    max_hash = c;                                                      \
  }
#include "core/fxcrt/css/property_values.inc"
#undef CSS_PROP_VALUE____
}

TEST(CSSDataTest, GetPropertyByName) {
  const CFX_CSSData::Property* p = CFX_CSSData::GetPropertyByName(L"color");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSProperty::Color, p->eName);

  p = CFX_CSSData::GetPropertyByName(L"border");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSProperty::Border, p->eName);

  p = CFX_CSSData::GetPropertyByName(L"not-a-property");
  EXPECT_FALSE(p);
}

TEST(CSSDataTest, GetPropertyByEnum) {
  const CFX_CSSData::Property* p =
      CFX_CSSData::GetPropertyByEnum(CFX_CSSProperty::Color);
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSProperty::Color, p->eName);

  p = CFX_CSSData::GetPropertyByEnum(CFX_CSSProperty::Border);
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSProperty::Border, p->eName);
}

TEST(CSSDataTest, GetLengthUnitByName) {
  const CFX_CSSData::LengthUnit* p = CFX_CSSData::GetLengthUnitByName(L"px");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSNumber::Unit::kPixels, p->type);

  p = CFX_CSSData::GetLengthUnitByName(L"em");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSNumber::Unit::kEMS, p->type);

  p = CFX_CSSData::GetLengthUnitByName(L"in");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSNumber::Unit::kInches, p->type);

  p = CFX_CSSData::GetLengthUnitByName(L"cm");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSNumber::Unit::kCentiMeters, p->type);

  p = CFX_CSSData::GetLengthUnitByName(L"mm");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSNumber::Unit::kMilliMeters, p->type);

  p = CFX_CSSData::GetLengthUnitByName(L"pc");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSNumber::Unit::kPicas, p->type);

  p = CFX_CSSData::GetLengthUnitByName(L"pt");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSNumber::Unit::kPoints, p->type);

  p = CFX_CSSData::GetLengthUnitByName(L"ex");
  ASSERT_TRUE(p);
  EXPECT_EQ(CFX_CSSNumber::Unit::kEXS, p->type);

  p = CFX_CSSData::GetLengthUnitByName(L"xx");
  EXPECT_FALSE(p);
}

TEST(CSSDataTest, GetColorByName) {
  const CFX_CSSData::Color* p = CFX_CSSData::GetColorByName(L"red");
  ASSERT_TRUE(p);
  EXPECT_EQ(0xffff0000, p->value);

  p = CFX_CSSData::GetColorByName(L"blue");
  ASSERT_TRUE(p);
  EXPECT_EQ(0xff0000ff, p->value);

  p = CFX_CSSData::GetColorByName(L"not-a-color");
  EXPECT_FALSE(p);
}
