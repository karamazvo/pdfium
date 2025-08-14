// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_seekablestreamproxy.h"

#include <iterator>

#include "core/fxcrt/cfx_read_only_span_stream.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(SeekableStreamProxyTest, NullStream) {
  auto proxy_stream = pdfium::MakeRetain<CFX_SeekableStreamProxy>(
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(
          pdfium::span<const uint8_t>()));

  wchar_t buffer[16];
  EXPECT_EQ(0u, proxy_stream->ReadBlock(buffer));
}

TEST(SeekableStreamProxyTest, DefaultStreamBOMNotRecognized) {
  ByteStringView data = "abcd";
  auto proxy_stream = pdfium::MakeRetain<CFX_SeekableStreamProxy>(
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(data.unsigned_span()));

  wchar_t buffer[16];
  EXPECT_EQ(0u, proxy_stream->ReadBlock(buffer));
}

TEST(SeekableStreamProxyTest, UTF8Stream) {
  ByteStringView data = "\xEF\xBB\xBF*\xC2\xA2*";
  auto proxy_stream = pdfium::MakeRetain<CFX_SeekableStreamProxy>(
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(data.unsigned_span()));

  wchar_t buffer[16];
  EXPECT_EQ(3u, proxy_stream->ReadBlock(buffer));
  EXPECT_EQ(L'*', buffer[0]);
  EXPECT_EQ(L'\u00A2', buffer[1]);
  EXPECT_EQ(L'*', buffer[2]);
}

TEST(SeekableStreamProxyTest, UTF16LEStream) {
  // Test embedded NUL not ending in NUL.
  const uint8_t data[] = {0xFF, 0xFE, 0x41, 0x00, 0x42, 0x01};
  auto proxy_stream = pdfium::MakeRetain<CFX_SeekableStreamProxy>(
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(data));

  wchar_t buffer[16];
  EXPECT_EQ(2u, proxy_stream->ReadBlock(buffer));
  EXPECT_EQ(L'A', buffer[0]);
  EXPECT_EQ(L'\u0142', buffer[1]);
}

TEST(SeekableStreamProxyTest, UTF16BEStream) {
  const uint8_t data[] = {0xFE, 0xFF, 0x00, 0x41, 0x01, 0x42};
  auto proxy_stream = pdfium::MakeRetain<CFX_SeekableStreamProxy>(
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(data));

  wchar_t buffer[16];
  EXPECT_EQ(2u, proxy_stream->ReadBlock(buffer));
  EXPECT_EQ(L'A', buffer[0]);
  EXPECT_EQ(L'\u0142', buffer[1]);
}

TEST(SeekableStreamProxyTest, GetSize) {
  const uint8_t data[] = {1, 2, 3, 4};
  auto proxy_stream = pdfium::MakeRetain<CFX_SeekableStreamProxy>(
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(data));
  EXPECT_EQ(4, proxy_stream->GetSize());
}

TEST(SeekableStreamProxyTest, IsEOF) {
  const uint8_t data[] = {1, 2, 3, 4};
  auto proxy_stream = pdfium::MakeRetain<CFX_SeekableStreamProxy>(
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(data));
  EXPECT_FALSE(proxy_stream->IsEOF());
  wchar_t buffer[4];
  proxy_stream->ReadBlock(buffer);
  EXPECT_TRUE(proxy_stream->IsEOF());
}

TEST(SeekableStreamProxyTest, SetAndGetCodePage) {
  const uint8_t data[] = {1, 2, 3, 4};
  auto proxy_stream = pdfium::MakeRetain<CFX_SeekableStreamProxy>(
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(data));
  proxy_stream->SetCodePage(FX_CodePage::kUTF16BE);
  EXPECT_EQ(FX_CodePage::kUTF16BE, proxy_stream->GetCodePage());
}
