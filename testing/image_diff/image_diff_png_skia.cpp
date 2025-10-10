// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/image_diff/image_diff_png.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/numerics/checked_math.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"
#include "third_party/skia/include/codec/SkPngDecoder.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/encode/SkPngEncoder.h"

namespace image_diff_png {

namespace {

class COMPONENT_EXPORT(GFX) BufferWStream : public SkWStream {
 public:
  BufferWStream() = default;
  BufferWStream(const BufferWStream&) = delete;
  BufferWStream& operator=(const BufferWStream&) = delete;
  ~BufferWStream() override {}

  // Returns the output buffer by moving.
  std::vector<uint8_t> TakeBuffer() && { return std::move(result_); }

  bool write(const void* buffer, size_t size) final {
    pdfium::span<const uint8_t> src(reinterpret_cast<const uint8_t*>(buffer),
                                    size);
    result_.insert(result_.end(), src.begin(), src.end());
    return true;
  }

  size_t bytesWritten() const final { return result_.size(); }

 private:
  std::vector<uint8_t> result_;
};

std::vector<uint8_t> EncodePNG(pdfium::span<const uint8_t> input,
                               SkColorType color,
                               SkAlphaType alpha,
                               int width,
                               int height,
                               size_t row_byte_width) {
  SkImageInfo info =
      SkImageInfo::Make(width, height, color, alpha, SkColorSpace::MakeSRGB());
  CHECK_NE(0, info.minRowBytes());  // 0 indicates conversion problems.
  CHECK_LE(info.minRowBytes(), row_byte_width);
  CHECK_NE(0, info.computeMinByteSize());  // 0 indicates conversion problems.
  CHECK_LE(info.computeMinByteSize(), input.size());
  SkPixmap pixmap(info, input.data(), row_byte_width);

  BufferWStream output;
  if (!SkPngEncoder::Encode(&output, pixmap, {})) {
    return std::vector<uint8_t>();
  }
  return std::move(output).TakeBuffer();
}

}  // namespace

std::vector<uint8_t> DecodePNG(pdfium::span<const uint8_t> input,
                               bool reverse_byte_order,
                               int* width,
                               int* height) {
  CHECK(width);
  CHECK(height);

  auto stream = std::make_unique<SkMemoryStream>(input.data(), input.size(),
                                                 /*copyData=*/false);
  std::unique_ptr<SkCodec> codec =
      SkPngDecoder::Decode(std::move(stream), nullptr);
  if (!codec) {
    return {};
  }

  SkColorType format =
      reverse_byte_order ? kBGRA_8888_SkColorType : kRGBA_8888_SkColorType;
  SkImageInfo info = codec->getInfo();
  info = info.makeColorType(format);
  info = info.makeColorSpace(SkColorSpace::MakeSRGB());

  std::vector<uint8_t> output;
  output.resize(info.computeMinByteSize());

  SkCodec::Result result =
      codec->getPixels(info, output.data(), info.minRowBytes());
  if (result != SkCodec::kSuccess) {
    return {};
  }

  *width = info.width();
  *height = info.height();
  return output;
}

std::vector<uint8_t> EncodeBGRPNG(pdfium::span<const uint8_t> bgr_input,
                                  int width,
                                  int height,
                                  int row_byte_width) {
  // Check inputs.
  FX_SAFE_SIZE_T expected_minimum_row_byte_width = 3;
  expected_minimum_row_byte_width *= width;
  CHECK_LE(expected_minimum_row_byte_width.ValueOrDie(),
           pdfium::checked_cast<size_t>(row_byte_width));

  FX_SAFE_SIZE_T expected_minimum_input_size = row_byte_width;
  expected_minimum_input_size *= height;
  CHECK_LE(expected_minimum_input_size.ValueOrDie(), bgr_input.size());

  // Convert `bgr_input` into `intermediate_bgra_buf` (because Skia doesn't
  // allow encoding BGR pixels - e.g. `SkColorType` can't represent BGR format
  // where each channel uses 8 bits and the whole pixel uses 24 bits).
  std::vector<uint8_t> intermediate_bgra_buf;
  size_t intermediate_bgra_stride =
      (FX_SAFE_SIZE_T(width) * FX_SAFE_SIZE_T(4)).ValueOrDie();
  {
    size_t required_intermediate_bgra_buf_size =
        (FX_SAFE_SIZE_T(intermediate_bgra_stride) * FX_SAFE_SIZE_T(height))
            .ValueOrDie();
    intermediate_bgra_buf.resize(required_intermediate_bgra_buf_size);

    pdfium::span<const uint8_t> src = bgr_input;
    pdfium::span<uint8_t> dst = intermediate_bgra_buf;
    for (size_t y = 0; y < pdfium::checked_cast<size_t>(height); y++) {
      for (size_t x = 0; x < pdfium::checked_cast<size_t>(width); x++) {
        // `FX_SAFE_SIZE_T` checks used in `intermediate_bgra_buf` calculations
        // above should prevent integer overflow in `x * N` expressions.  And
        // `first(N)` should be checked within `span` implementation.
        pdfium::span<uint8_t> dstSpan = dst.subspan(x * 4).first(4u);
        pdfium::span<const uint8_t> srcSpan = src.subspan(x * 3).first(3u);

        fxcrt::spancpy(dstSpan.first(3u), srcSpan);
        dstSpan[3] = 0xFF;  // opaque
      }
      src = src.subspan(std::min<size_t>(row_byte_width, src.size()));
      dst = dst.subspan(std::min(intermediate_bgra_stride, dst.size()));
    }
  }

  return EncodePNG(intermediate_bgra_buf, kBGRA_8888_SkColorType,
                   kOpaque_SkAlphaType, width, height,
                   intermediate_bgra_stride);
}

std::vector<uint8_t> EncodeRGBAPNG(pdfium::span<const uint8_t> input,
                                   int width,
                                   int height,
                                   int row_byte_width) {
  return EncodePNG(input, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType, width,
                   height, pdfium::checked_cast<size_t>(row_byte_width));
}

std::vector<uint8_t> EncodeBGRAPNG(pdfium::span<const uint8_t> input,
                                   int width,
                                   int height,
                                   int row_byte_width,
                                   bool discard_transparency) {
  return EncodePNG(input, kBGRA_8888_SkColorType, kUnpremul_SkAlphaType, width,
                   height, pdfium::checked_cast<size_t>(row_byte_width));
}

std::vector<uint8_t> EncodeGrayPNG(pdfium::span<const uint8_t> input,
                                   int width,
                                   int height,
                                   int row_byte_width) {
  return EncodePNG(input, kGray_8_SkColorType, kOpaque_SkAlphaType, width,
                   height, pdfium::checked_cast<size_t>(row_byte_width));
}

}  // namespace image_diff_png
