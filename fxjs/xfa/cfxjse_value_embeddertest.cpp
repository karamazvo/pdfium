// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxjs/xfa/cfxjse_value.h"

#include <memory>
#include <utility>
#include <vector>

#include "fxjs/xfa/cfxjse_engine.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/xfa_js_embedder_test.h"

class CFXJSEValueEmbedderTest : public XFAJSEmbedderTest {};

TEST_F(CFXJSEValueEmbedderTest, Empty) {
  ASSERT_TRUE(OpenDocument("simple_xfa.pdf"));

  auto value = std::make_unique<CFXJSE_Value>();
  EXPECT_TRUE(value->IsEmpty());
  EXPECT_FALSE(value->IsUndefined(isolate()));
  EXPECT_FALSE(value->IsNull(isolate()));
  EXPECT_FALSE(value->IsBoolean(isolate()));
  EXPECT_FALSE(value->IsString(isolate()));
  EXPECT_FALSE(value->IsNumber(isolate()));
  EXPECT_FALSE(value->IsObject(isolate()));
  EXPECT_FALSE(value->IsArray(isolate()));
  EXPECT_FALSE(value->IsFunction(isolate()));
}

TEST_F(CFXJSEValueEmbedderTest, EmptyArrayInsert) {
  ASSERT_TRUE(OpenDocument("simple_xfa.pdf"));

  // Test inserting empty values into arrays.
  auto value = std::make_unique<CFXJSE_Value>();
  std::vector<std::unique_ptr<CFXJSE_Value>> vec;
  vec.push_back(std::move(value));

  CFXJSE_Value array;
  array.SetArray(isolate(), vec);
  EXPECT_TRUE(array.IsArray(isolate()));
}
