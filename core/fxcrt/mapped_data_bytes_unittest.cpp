// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/mapped_data_bytes.h"

#include <fcntl.h>
#include <unistd.h>

#include "build/build_config.h"
#include "core/fxcrt/bytestring.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !BUILDFLAG(IS_POSIX)
#error "Built on wrong platform"
#endif

namespace fxcrt {

TEST(MappedDataBytes, CreateNotFound) {
  auto mapping = MappedDataBytes::Create("non_existent_file_asdfghjkl");
  EXPECT_FALSE(mapping);
}

TEST(MappedDataBytes, CreateEmpty) {
  char temp_name[] = "/tmp/pdfium_empty_XXXXXX";
  int fd = mkstemp(temp_name);
  ASSERT_GE(fd, 0);
  close(fd);

  auto mapping = MappedDataBytes::Create(temp_name);
  ASSERT_TRUE(mapping);
  EXPECT_TRUE(mapping->empty());
  EXPECT_EQ(0u, mapping->size());
  EXPECT_TRUE(mapping->span().empty());

  unlink(temp_name);
}

TEST(MappedDataBytes, CreateNormal) {
  char temp_name[] = "/tmp/pdfium_normal_XXXXXX";
  int fd = mkstemp(temp_name);
  ASSERT_GE(fd, 0);
  static const uint8_t kData[] = {'h', 'e', 'l', 'l', 'o'};
  ASSERT_EQ(static_cast<ssize_t>(sizeof(kData)),
            write(fd, kData, sizeof(kData)));
  close(fd);

  auto mapping = MappedDataBytes::Create(temp_name);
  ASSERT_TRUE(mapping);
  EXPECT_FALSE(mapping->empty());
  EXPECT_EQ(sizeof(kData), mapping->size());
  EXPECT_EQ(mapping->span(), pdfium::span(kData));

  unlink(temp_name);
}

}  // namespace fxcrt
