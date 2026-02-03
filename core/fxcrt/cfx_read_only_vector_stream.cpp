// Copyright 2022 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_read_only_vector_stream.h"

#include <utility>

#include "build/build_config.h"
#include "core/fxcrt/cfx_read_only_span_stream.h"
#include "core/fxcrt/span.h"

#if BUILDFLAG(IS_POSIX)
#include "core/fxcrt/mapped_data_bytes.h"
#endif

CFX_ReadOnlyVectorStream::CFX_ReadOnlyVectorStream(DataVector<uint8_t> data)
    : data_(std::move(data)),
      stream_(pdfium::MakeRetain<CFX_ReadOnlySpanStream>(data_)) {}

CFX_ReadOnlyVectorStream::CFX_ReadOnlyVectorStream(
    FixedSizeDataVector<uint8_t> data)
    : fixed_data_(std::move(data)),
      stream_(pdfium::MakeRetain<CFX_ReadOnlySpanStream>(fixed_data_)) {}

#if BUILDFLAG(IS_POSIX)
CFX_ReadOnlyVectorStream::CFX_ReadOnlyVectorStream(const ByteString& file_name)
    : mapped_data_(MappedDataBytes::Create(file_name)) {
  if (mapped_data_) {
    stream_ = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(mapped_data_->span());
  }
}
#endif

CFX_ReadOnlyVectorStream::~CFX_ReadOnlyVectorStream() = default;

FX_FILESIZE CFX_ReadOnlyVectorStream::GetSize() {
  return stream_->GetSize();
}

bool CFX_ReadOnlyVectorStream::ReadBlockAtOffset(pdfium::span<uint8_t> buffer,
                                                 FX_FILESIZE offset) {
  return stream_->ReadBlockAtOffset(buffer, offset);
}

pdfium::span<const uint8_t> CFX_ReadOnlyVectorStream::span() const {
  return stream_->span();
}
