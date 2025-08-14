// Copyright 2022 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/binary_buffer.h"

#include <utility>
#include <vector>

#include "core/fxcrt/bytestring.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace fxcrt {

TEST(BinaryBuffer, Empty) {
  BinaryBuffer buffer;
  EXPECT_TRUE(buffer.IsEmpty());
  EXPECT_EQ(0u, buffer.GetSize());
  EXPECT_EQ(0u, buffer.GetLength());
  EXPECT_TRUE(buffer.GetSpan().empty());
}

TEST(BinaryBuffer, MoveConstruct) {
  BinaryBuffer buffer;
  buffer.AppendUint8(65u);

  BinaryBuffer buffer2(std::move(buffer));

  EXPECT_TRUE(buffer.IsEmpty());
  EXPECT_EQ(0u, buffer.GetSize());
  EXPECT_EQ(0u, buffer.GetLength());
  EXPECT_TRUE(buffer.GetSpan().empty());

  EXPECT_FALSE(buffer2.IsEmpty());
  EXPECT_EQ(1u, buffer2.GetSize());
  EXPECT_EQ(1u, buffer2.GetLength());
  EXPECT_EQ(65u, buffer2.GetSpan()[0]);
}

TEST(BinaryBuffer, MoveAssign) {
  BinaryBuffer buffer;
  BinaryBuffer buffer2;
  buffer.AppendUint8(65u);
  buffer2 = std::move(buffer);

  EXPECT_TRUE(buffer.IsEmpty());
  EXPECT_EQ(0u, buffer.GetSize());
  EXPECT_EQ(0u, buffer.GetLength());
  EXPECT_TRUE(buffer.GetSpan().empty());

  EXPECT_FALSE(buffer2.IsEmpty());
  EXPECT_EQ(1u, buffer2.GetSize());
  ASSERT_EQ(1u, buffer2.GetLength());
  EXPECT_EQ(65u, buffer2.GetSpan()[0]);
}

TEST(BinaryBuffer, Clear) {
  BinaryBuffer buffer;
  buffer.AppendUint8(65u);
  buffer.Clear();
  EXPECT_TRUE(buffer.IsEmpty());
  EXPECT_EQ(0u, buffer.GetSize());
  EXPECT_EQ(0u, buffer.GetLength());
  EXPECT_TRUE(buffer.GetSpan().empty());
}

TEST(BinaryBuffer, AppendSpans) {
  BinaryBuffer buffer;
  std::vector<uint8_t> aaa(3, 65u);
  std::vector<uint8_t> bbb(3, 66u);
  buffer.AppendSpan(aaa);
  buffer.AppendSpan(bbb);
  EXPECT_FALSE(buffer.IsEmpty());
  EXPECT_EQ(6u, buffer.GetSize());
  EXPECT_EQ(6u, buffer.GetLength());
  EXPECT_EQ(65u, buffer.GetSpan()[0]);
  EXPECT_EQ(65u, buffer.GetSpan()[1]);
  EXPECT_EQ(65u, buffer.GetSpan()[2]);
  EXPECT_EQ(66u, buffer.GetSpan()[3]);
  EXPECT_EQ(66u, buffer.GetSpan()[4]);
  EXPECT_EQ(66u, buffer.GetSpan()[5]);
}

TEST(BinaryBuffer, AppendBlocks) {
  BinaryBuffer buffer;
  std::vector<uint8_t> aaa(3, 65u);
  std::vector<uint8_t> bbb(3, 66u);
  buffer.AppendSpan(aaa);
  buffer.AppendSpan(bbb);
  EXPECT_EQ(6u, buffer.GetSize());
  EXPECT_EQ(6u, buffer.GetLength());
  EXPECT_EQ(65u, buffer.GetSpan()[0]);
  EXPECT_EQ(65u, buffer.GetSpan()[1]);
  EXPECT_EQ(65u, buffer.GetSpan()[2]);
  EXPECT_EQ(66u, buffer.GetSpan()[3]);
  EXPECT_EQ(66u, buffer.GetSpan()[4]);
  EXPECT_EQ(66u, buffer.GetSpan()[5]);
}

TEST(BinaryBuffer, AppendStrings) {
  BinaryBuffer buffer;
  buffer.AppendString("AA");
  buffer.AppendString("BB");
  EXPECT_EQ(4u, buffer.GetSize());
  EXPECT_EQ(4u, buffer.GetLength());
  EXPECT_EQ(65u, buffer.GetSpan()[0]);
  EXPECT_EQ(65u, buffer.GetSpan()[1]);
  EXPECT_EQ(66u, buffer.GetSpan()[2]);
  EXPECT_EQ(66u, buffer.GetSpan()[3]);
}

TEST(BinaryBuffer, AppendBytes) {
  BinaryBuffer buffer;
  buffer.AppendUint8(65u);
  buffer.AppendUint8(66u);
  EXPECT_EQ(2u, buffer.GetSize());
  EXPECT_EQ(2u, buffer.GetLength());
  EXPECT_EQ(65u, buffer.GetSpan()[0]);
  EXPECT_EQ(66u, buffer.GetSpan()[1]);
}

// Assumes little endian.
TEST(BinaryBuffer, AppendUint16) {
  BinaryBuffer buffer;
  buffer.AppendUint16(0x4321);
  EXPECT_EQ(2u, buffer.GetSize());
  EXPECT_EQ(2u, buffer.GetLength());
  EXPECT_EQ(0x21u, buffer.GetSpan()[0]);
  EXPECT_EQ(0x43u, buffer.GetSpan()[1]);
}

// Assumes little endian.
TEST(BinaryBuffer, AppendUint32) {
  BinaryBuffer buffer;
  buffer.AppendUint32(0x87654321);
  EXPECT_EQ(4u, buffer.GetSize());
  EXPECT_EQ(4u, buffer.GetLength());
  EXPECT_EQ(0x21u, buffer.GetSpan()[0]);
  EXPECT_EQ(0x43u, buffer.GetSpan()[1]);
  EXPECT_EQ(0x65u, buffer.GetSpan()[2]);
  EXPECT_EQ(0x87u, buffer.GetSpan()[3]);
}

TEST(BinaryBuffer, AppendDouble) {
  BinaryBuffer buffer;
  buffer.AppendDouble(1234.5678);
  EXPECT_EQ(8u, buffer.GetSize());
  EXPECT_EQ(8u, buffer.GetLength());
  // arch-dependent bit pattern.
}

TEST(BinaryBuffer, GetMutableSpan) {
  BinaryBuffer buffer;
  buffer.AppendUint8(65u);
  pdfium::span<uint8_t> span = buffer.GetMutableSpan();
  ASSERT_EQ(1u, span.size());
  EXPECT_EQ(65u, span[0]);
  span[0] = 66u;
  pdfium::span<const uint8_t> const_span = buffer.GetSpan();
  ASSERT_EQ(1u, const_span.size());
  EXPECT_EQ(66u, const_span[0]);
}

TEST(BinaryBuffer, SetAllocStep) {
  BinaryBuffer buffer;
  buffer.SetAllocStep(256);
  buffer.AppendUint8(1);
  buffer.EstimateSize(1);
  EXPECT_GE(buffer.GetSpan().size(), 1u);
}

TEST(BinaryBuffer, EstimateSize) {
  BinaryBuffer buffer;
  buffer.EstimateSize(100);
  buffer.AppendUint8(1);
  EXPECT_GE(buffer.GetSpan().size(), 1u);
}

class BinaryBufferUnderTest : public BinaryBuffer {
 public:
  void CallDeleteBuf(size_t start_index, size_t count) {
    DeleteBuf(start_index, count);
  }
};

TEST(BinaryBuffer, DeleteBuf) {
  BinaryBufferUnderTest buffer;
  buffer.AppendString("ABC");
  buffer.CallDeleteBuf(1, 1);
  ASSERT_EQ(2u, buffer.GetSize());
  EXPECT_EQ('A', buffer.GetSpan()[0]);
  EXPECT_EQ('C', buffer.GetSpan()[1]);
}

TEST(BinaryBuffer, DetachBuffer) {
  BinaryBuffer buffer;
  buffer.AppendString("ABC");
  DataVector<uint8_t> data = buffer.DetachBuffer();
  EXPECT_TRUE(buffer.IsEmpty());
  ASSERT_EQ(3u, data.size());
  EXPECT_EQ('A', data[0]);
  EXPECT_EQ('B', data[1]);
  EXPECT_EQ('C', data[2]);
}

}  // namespace fxcrt
