// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_MAPPED_DATA_BYTES_H_
#define CORE_FXCRT_MAPPED_DATA_BYTES_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/span.h"

namespace fxcrt {

class MappedDataBytes {
 public:
  static std::unique_ptr<MappedDataBytes> Create(const ByteString& file_name);

  ~MappedDataBytes();

  MappedDataBytes(const MappedDataBytes&) = delete;
  MappedDataBytes(MappedDataBytes&&) = delete;
  MappedDataBytes& operator=(const MappedDataBytes&) = delete;
  MappedDataBytes& operator=(MappedDataBytes&&) = delete;

  bool empty() const { return mapping_.empty(); }
  size_t size() const { return mapping_.size(); }

  // Explicit access to data via span.
  pdfium::span<const uint8_t> span() const { return mapping_; }

  // Convenience methods to slice the mapped bytes into spans.
  pdfium::span<const uint8_t> subspan(size_t offset) const {
    return span().subspan(offset);
  }
  pdfium::span<const uint8_t> subspan(size_t offset, size_t count) const {
    return span().subspan(offset, count);
  }
  pdfium::span<const uint8_t> first(size_t count) const {
    return span().first(count);
  }
  pdfium::span<const uint8_t> last(size_t count) const {
    return span().last(count);
  }

 private:
  explicit MappedDataBytes(int fd);

  const int fd_;
  pdfium::span<const uint8_t> mapping_;
};

}  // namespace fxcrt

using fxcrt::MappedDataBytes;

#endif  // CORE_FXCRT_MAPPED_DATA_BYTES_H_
