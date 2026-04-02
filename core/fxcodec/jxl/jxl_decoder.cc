// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jxl/jxl_decoder.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"

#if defined(PDF_ENABLE_RUST_JXL)
#include "third_party/rust/jxl/v0_3/wrapper/lib.rs.h"
#endif

namespace pdfium::jxl {

namespace {

// Conservative cap against decompression bombs.
//
// Keep in sync with the mental model from Blink: the jxl-rs wrapper counts
// pixels * channels (samples). 1B samples supports JPEG XL codestream level 5
// (~268M pixels) even for RGBA.
constexpr uint64_t kMaxDecodedSamples = 1024ULL * 1024 * 1024;

}  // namespace

std::optional<Info> ParseInfo(pdfium::span<const uint8_t> data) {
#if !defined(PDF_ENABLE_RUST_JXL)
  return std::nullopt;
#else
  if (data.empty()) {
    return std::nullopt;
  }

  // Use unpremultiplied alpha output.
  rust::Box<blink::jxl_rs::JxlRsDecoder> decoder =
      blink::jxl_rs::jxl_rs_decoder_create(kMaxDecodedSamples,
                                           /*premultiply_alpha=*/false);

  auto result = decoder->parse_basic_info(
      rust::Slice<const uint8_t>(data.data(), data.size()),
      /*all_input=*/true);
  if (result.status != blink::jxl_rs::JxlRsStatus::Success) {
    return std::nullopt;
  }

  blink::jxl_rs::JxlRsBasicInfo info = decoder->get_basic_info();
  if (info.width == 0 || info.height == 0) {
    return std::nullopt;
  }

  Info out;
  out.width = info.width;
  out.height = info.height;
  out.has_alpha = info.has_alpha;
  out.have_animation = info.have_animation;
  return out;
#endif
}

bool DecodeFrame0ToBgra(pdfium::span<const uint8_t> data,
                        uint8_t* dest_bgra,
                        uint32_t dest_stride,
                        uint32_t width,
                        uint32_t height) {
#if !defined(PDF_ENABLE_RUST_JXL)
  return false;
#else
  if (!dest_bgra || width == 0 || height == 0) {
    return false;
  }

  // We only support BGRA8 output for the initial integration.
  const uint32_t min_stride = width * 4u;
  if (dest_stride < min_stride) {
    return false;
  }

  rust::Box<blink::jxl_rs::JxlRsDecoder> decoder =
      blink::jxl_rs::jxl_rs_decoder_create(kMaxDecodedSamples,
                                           /*premultiply_alpha=*/false);

  // Feed data incrementally, honoring bytes_consumed, similar to Blink.
  size_t offset = 0;

  auto remaining = [&]() {
    if (offset > data.size()) {
      return pdfium::span<const uint8_t>();
    }
    return data.subspan(offset);
  };

  // Parse metadata first.
  {
    auto input = remaining();
    auto basic_result = decoder->parse_basic_info(
        rust::Slice<const uint8_t>(input.data(), input.size()),
        /*all_input=*/true);
    if (basic_result.status != blink::jxl_rs::JxlRsStatus::Success) {
      return false;
    }
    if (basic_result.bytes_consumed > input.size()) {
      return false;
    }
    offset += basic_result.bytes_consumed;
  }

  blink::jxl_rs::JxlRsBasicInfo info = decoder->get_basic_info();
  if (info.width != width || info.height != height) {
    // Caller must pass dimensions matching the JXL payload.
    return false;
  }

  // Tell jxl-rs what pixel format we want.
  decoder->set_pixel_format(blink::jxl_rs::JxlRsPixelFormat::Bgra8,
                            info.num_extra_channels);

  // Parse the first frame header.
  {
    auto input = remaining();
    auto frame_result = decoder->parse_frame_header(
        rust::Slice<const uint8_t>(input.data(), input.size()),
        /*all_input=*/true);
    if (frame_result.status != blink::jxl_rs::JxlRsStatus::Success) {
      return false;
    }
    if (frame_result.bytes_consumed > input.size()) {
      return false;
    }
    offset += frame_result.bytes_consumed;
  }

  // Decode frame 0 into the PDFium bitmap buffer.
  {
    auto input = remaining();
    auto decode_result = decoder->decode_frame_with_stride(
        rust::Slice<const uint8_t>(input.data(), input.size()),
        /*all_input=*/true,
        rust::Slice<uint8_t>(dest_bgra, static_cast<size_t>(dest_stride) *
                                            static_cast<size_t>(height)),
        width, height, static_cast<size_t>(dest_stride));

    return decode_result.status == blink::jxl_rs::JxlRsStatus::Success;
  }
#endif
}

}  // namespace pdfium::jxl
