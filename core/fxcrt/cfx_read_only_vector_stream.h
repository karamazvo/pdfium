// Copyright 2022 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_CFX_READ_ONLY_VECTOR_STREAM_H_
#define CORE_FXCRT_CFX_READ_ONLY_VECTOR_STREAM_H_

#include <stdint.h>

#include "build/build_config.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/retain_ptr.h"

class CFX_ReadOnlySpanStream;

#if BUILDFLAG(IS_POSIX)
#include <memory>

namespace fxcrt {
class MappedDataBytes;
}  // namespace fxcrt
#endif

class CFX_ReadOnlyVectorStream final : public IFX_SeekableReadStream {
 public:
  CONSTRUCT_VIA_MAKE_RETAIN;

  // IFX_SeekableReadStream:
  FX_FILESIZE GetSize() override;
  bool ReadBlockAtOffset(pdfium::span<uint8_t> buffer,
                         FX_FILESIZE offset) override;

  pdfium::span<const uint8_t> span() const;

 private:
  explicit CFX_ReadOnlyVectorStream(DataVector<uint8_t> data);
  explicit CFX_ReadOnlyVectorStream(FixedSizeDataVector<uint8_t> data);
#if BUILDFLAG(IS_POSIX)
  explicit CFX_ReadOnlyVectorStream(const ByteString& file_name);
#endif
  ~CFX_ReadOnlyVectorStream() override;

  const DataVector<uint8_t> data_;
  const FixedSizeDataVector<uint8_t> fixed_data_;
#if BUILDFLAG(IS_POSIX)
  const std::unique_ptr<fxcrt::MappedDataBytes> mapped_data_;
#endif
  // Spans over either `data_`, `fixed_data_`, or `mapped_data_`.
  RetainPtr<CFX_ReadOnlySpanStream> stream_;
};

#endif  // CORE_FXCRT_CFX_READ_ONLY_VECTOR_STREAM_H_
