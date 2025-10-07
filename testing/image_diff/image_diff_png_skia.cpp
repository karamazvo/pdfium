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
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkStream.h"

#ifdef PDF_ENABLE_RUST_PNG
#include "third_party/skia/include/codec/SkPngRustDecoder.h"
#include "third_party/skia/include/encode/SkPngRustEncoder.h"
#else
#include "third_party/skia/include/codec/SkPngDecoder.h"
#include "third_party/skia/include/encode/SkPngEncoder.h"
#endif

namespace image_diff_png {

namespace {

std::vector<uint8_t> EncodePNG(pdfium::span<const uint8_t> input,
                               SkColorType color,
                               SkAlphaType alpha,
                               size_t width,
                               size_t height,
                               size_t row_byte_width) {
  size_t expected_minimum_input_size =
      (FX_SAFE_SIZE_T(row_byte_width) * FX_SAFE_SIZE_T(height)).ValueOrDie();
  CHECK_LE(expected_minimum_input_size, input.size());

  SkImageInfo info = SkImageInfo::Make(pdfium::checked_cast<int>(width),
                                       pdfium::checked_cast<int>(height), color,
                                       alpha, SkColorSpace::MakeSRGB());
  CHECK_LE(info.computeMinByteSize(), row_byte_width);

  SkPixmap pixmap(info, input.data(), row_byte_width);
#ifdef PDF_ENABLE_RUST_PNG
  sk_sp<SkData> data = SkPngRustEncoder::Encode(pixmap, {});
#else
#error "Using SkPngEncoder results in (not yet investigated) PDFium CQ failures"
#endif
  SkSpan<const uint8_t> span = data->byteSpan();
  return std::vector<uint8_t>(span.begin(), span.end());
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
#ifdef PDF_ENABLE_RUST_PNG
  std::unique_ptr<SkCodec> codec =
      SkPngRustDecoder::Decode(std::move(stream), nullptr);
#else
#error "Using SkPngDecoder results in (not yet investigated) PDFium CQ failures"
#endif
  if (!codec) {
    return {};
  }

  SkColorType format =
      reverse_byte_order ? kBGRA_8888_SkColorType : kRGBA_8888_SkColorType;
  SkImageInfo info = codec->getInfo();
  info = info.makeColorType(format);
  info = info.makeColorSpace(SkColorSpace::MakeSRGB());

  *width = info.width();
  *height = info.height();

  std::vector<uint8_t> output;
  output.resize(info.computeMinByteSize());

  SkCodec::Result result =
      codec->getPixels(info, output.data(), info.minRowBytes());
  if (result != SkCodec::kSuccess) {
    return {};
  }
  return output;
}

std::vector<uint8_t> EncodeBGRPNG(pdfium::span<const uint8_t> bgr_input,
                                  int int_width,
                                  int int_height,
                                  int int_row_byte_width) {
  // Check inputs.
  size_t width = pdfium::checked_cast<size_t>(int_width);
  size_t height = pdfium::checked_cast<size_t>(int_height);
  size_t row_byte_width = pdfium::checked_cast<size_t>(int_row_byte_width);

  size_t expected_minimum_row_byte_width =
      (FX_SAFE_SIZE_T(width) * FX_SAFE_SIZE_T(3)).ValueOrDie();
  CHECK_LE(expected_minimum_row_byte_width, row_byte_width);

  size_t expected_minimum_input_size =
      (FX_SAFE_SIZE_T(row_byte_width) * FX_SAFE_SIZE_T(height)).ValueOrDie();
  CHECK_LE(expected_minimum_input_size, bgr_input.size());

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
    for (size_t y = 0; y < height; y++) {
      for (size_t x = 0; x < width; x++) {
        // `FX_SAFE_SIZE_T` checks used in `intermediate_bgra_buf` calculations
        // above should prevent integer overflow in `x * 4` expressions.  OTOH,
        // the `+ 3` part can still overflow.  This is okay, because 1) this is
        // an unsigned overflow and 2) we rely on runtime `span` checks.
        dst[x * 4 + 0] = src[x * 3 + 0];
        dst[x * 4 + 1] = src[x * 3 + 1];
        dst[x * 4 + 2] = src[x * 3 + 2];
        dst[x * 4 + 3] = 0xFF;  // opaque
      }
      src = src.subspan(std::min<size_t>(row_byte_width, src.size()));
      dst = dst.subspan(std::min(intermediate_bgra_stride, dst.size()));
    }
  }

  // Encode `intermediate_bgra_buf`.
  return EncodePNG(intermediate_bgra_buf, kBGRA_8888_SkColorType,
                   kOpaque_SkAlphaType, width, height,
                   intermediate_bgra_stride);
}

std::vector<uint8_t> EncodeRGBAPNG(pdfium::span<const uint8_t> input,
                                   int width,
                                   int height,
                                   int row_byte_width) {
  return EncodePNG(input, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType,
                   pdfium::checked_cast<size_t>(width),
                   pdfium::checked_cast<size_t>(height),
                   pdfium::checked_cast<size_t>(row_byte_width));
}

std::vector<uint8_t> EncodeBGRAPNG(pdfium::span<const uint8_t> input,
                                   int width,
                                   int height,
                                   int row_byte_width,
                                   bool discard_transparency) {
  return EncodePNG(input, kBGRA_8888_SkColorType, kUnpremul_SkAlphaType,
                   pdfium::checked_cast<size_t>(width),
                   pdfium::checked_cast<size_t>(height),
                   pdfium::checked_cast<size_t>(row_byte_width));
}

std::vector<uint8_t> EncodeGrayPNG(pdfium::span<const uint8_t> input,
                                   int width,
                                   int height,
                                   int row_byte_width) {
  return EncodePNG(input, kGray_8_SkColorType, kOpaque_SkAlphaType,
                   pdfium::checked_cast<size_t>(width),
                   pdfium::checked_cast<size_t>(height),
                   pdfium::checked_cast<size_t>(row_byte_width));
}

}  // namespace image_diff_png
