// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cfx_scanlinecompositor.h"

#include <stdint.h>

#include <array>

#include "core/fxcrt/span.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxge/dib/fx_dib.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr FX_BGRA_STRUCT<uint8_t> kDestScan[] = {
    {.blue = 255, .green = 100, .red = 0, .alpha = 0},
    {.blue = 255, .green = 100, .red = 0, .alpha = 0},
    {.blue = 255, .green = 100, .red = 0, .alpha = 255},
    {.blue = 255, .green = 100, .red = 0, .alpha = 255},
    {.blue = 255, .green = 100, .red = 0, .alpha = 100},
    {.blue = 255, .green = 100, .red = 0, .alpha = 100},
    {.blue = 255, .green = 100, .red = 0, .alpha = 200},
    {.blue = 255, .green = 100, .red = 0, .alpha = 200},
};
constexpr FX_BGRA_STRUCT<uint8_t> kSrcScan1[] = {
    {.blue = 255, .green = 100, .red = 0, .alpha = 0},
    {.blue = 255, .green = 100, .red = 0, .alpha = 255},
    {.blue = 255, .green = 100, .red = 0, .alpha = 0},
    {.blue = 255, .green = 100, .red = 0, .alpha = 255},
    {.blue = 255, .green = 100, .red = 0, .alpha = 100},
    {.blue = 255, .green = 100, .red = 0, .alpha = 200},
    {.blue = 255, .green = 100, .red = 0, .alpha = 100},
    {.blue = 255, .green = 100, .red = 0, .alpha = 200},
};
constexpr FX_BGRA_STRUCT<uint8_t> kSrcScan2[] = {
    {.blue = 100, .green = 0, .red = 255, .alpha = 0},
    {.blue = 100, .green = 0, .red = 255, .alpha = 255},
    {.blue = 100, .green = 0, .red = 255, .alpha = 0},
    {.blue = 100, .green = 0, .red = 255, .alpha = 255},
    {.blue = 100, .green = 0, .red = 255, .alpha = 100},
    {.blue = 100, .green = 0, .red = 255, .alpha = 200},
    {.blue = 100, .green = 0, .red = 255, .alpha = 100},
    {.blue = 100, .green = 0, .red = 255, .alpha = 200},
};
constexpr FX_BGRA_STRUCT<uint8_t> kSrcScan3[] = {
    {.blue = 0, .green = 255, .red = 100, .alpha = 0},
    {.blue = 0, .green = 255, .red = 100, .alpha = 255},
    {.blue = 0, .green = 255, .red = 100, .alpha = 0},
    {.blue = 0, .green = 255, .red = 100, .alpha = 255},
    {.blue = 0, .green = 255, .red = 100, .alpha = 100},
    {.blue = 0, .green = 255, .red = 100, .alpha = 200},
    {.blue = 0, .green = 255, .red = 100, .alpha = 100},
    {.blue = 0, .green = 255, .red = 100, .alpha = 200},
};

}  // namespace

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraNormal) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kNormal,
                              /*bRgbByteOrder=*/false));

  {
    std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
    fxcrt::Copy(kDestScan, dest_scan);
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kSrcScan1),
                                      dest_scan.size(), {});
    EXPECT_EQ(255, dest_scan[0].blue);
    EXPECT_EQ(100, dest_scan[0].green);
    EXPECT_EQ(0, dest_scan[0].red);
    EXPECT_EQ(0, dest_scan[0].alpha);
    EXPECT_EQ(255, dest_scan[1].blue);
    EXPECT_EQ(100, dest_scan[1].green);
    EXPECT_EQ(0, dest_scan[1].red);
    EXPECT_EQ(255, dest_scan[1].alpha);
    EXPECT_EQ(255, dest_scan[2].blue);
    EXPECT_EQ(100, dest_scan[2].green);
    EXPECT_EQ(0, dest_scan[2].red);
    EXPECT_EQ(255, dest_scan[2].alpha);
    EXPECT_EQ(255, dest_scan[3].blue);
    EXPECT_EQ(100, dest_scan[3].green);
    EXPECT_EQ(0, dest_scan[3].red);
    EXPECT_EQ(255, dest_scan[3].alpha);
    EXPECT_EQ(255, dest_scan[4].blue);
    EXPECT_EQ(100, dest_scan[4].green);
    EXPECT_EQ(0, dest_scan[4].red);
    EXPECT_EQ(161, dest_scan[4].alpha);
    EXPECT_EQ(255, dest_scan[5].blue);
    EXPECT_EQ(100, dest_scan[5].green);
    EXPECT_EQ(0, dest_scan[5].red);
    EXPECT_EQ(222, dest_scan[5].alpha);
    EXPECT_EQ(255, dest_scan[6].blue);
    EXPECT_EQ(100, dest_scan[6].green);
    EXPECT_EQ(0, dest_scan[6].red);
    EXPECT_EQ(222, dest_scan[6].alpha);
    EXPECT_EQ(255, dest_scan[7].blue);
    EXPECT_EQ(100, dest_scan[7].green);
    EXPECT_EQ(0, dest_scan[7].red);
    EXPECT_EQ(244, dest_scan[7].alpha);
  }
  {
    std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
    fxcrt::Copy(kDestScan, dest_scan);
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kSrcScan2),
                                      dest_scan.size(), {});
    EXPECT_EQ(100, dest_scan[0].blue);
    EXPECT_EQ(0, dest_scan[0].green);
    EXPECT_EQ(255, dest_scan[0].red);
    EXPECT_EQ(0, dest_scan[0].alpha);
    EXPECT_EQ(100, dest_scan[1].blue);
    EXPECT_EQ(0, dest_scan[1].green);
    EXPECT_EQ(255, dest_scan[1].red);
    EXPECT_EQ(255, dest_scan[1].alpha);
    EXPECT_EQ(255, dest_scan[2].blue);
    EXPECT_EQ(100, dest_scan[2].green);
    EXPECT_EQ(0, dest_scan[2].red);
    EXPECT_EQ(255, dest_scan[2].alpha);
    EXPECT_EQ(100, dest_scan[3].blue);
    EXPECT_EQ(0, dest_scan[3].green);
    EXPECT_EQ(255, dest_scan[3].red);
    EXPECT_EQ(255, dest_scan[3].alpha);
    EXPECT_EQ(158, dest_scan[4].blue);
    EXPECT_EQ(38, dest_scan[4].green);
    EXPECT_EQ(158, dest_scan[4].red);
    EXPECT_EQ(161, dest_scan[4].alpha);
    EXPECT_EQ(115, dest_scan[5].blue);
    EXPECT_EQ(10, dest_scan[5].green);
    EXPECT_EQ(229, dest_scan[5].red);
    EXPECT_EQ(222, dest_scan[5].alpha);
    EXPECT_EQ(185, dest_scan[6].blue);
    EXPECT_EQ(55, dest_scan[6].green);
    EXPECT_EQ(114, dest_scan[6].red);
    EXPECT_EQ(222, dest_scan[6].alpha);
    EXPECT_EQ(127, dest_scan[7].blue);
    EXPECT_EQ(18, dest_scan[7].green);
    EXPECT_EQ(209, dest_scan[7].red);
    EXPECT_EQ(244, dest_scan[7].alpha);
  }
  {
    std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
    fxcrt::Copy(kDestScan, dest_scan);
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kSrcScan3),
                                      dest_scan.size(), {});
    EXPECT_EQ(0, dest_scan[0].blue);
    EXPECT_EQ(255, dest_scan[0].green);
    EXPECT_EQ(100, dest_scan[0].red);
    EXPECT_EQ(0, dest_scan[0].alpha);
    EXPECT_EQ(0, dest_scan[1].blue);
    EXPECT_EQ(255, dest_scan[1].green);
    EXPECT_EQ(100, dest_scan[1].red);
    EXPECT_EQ(255, dest_scan[1].alpha);
    EXPECT_EQ(255, dest_scan[2].blue);
    EXPECT_EQ(100, dest_scan[2].green);
    EXPECT_EQ(0, dest_scan[2].red);
    EXPECT_EQ(255, dest_scan[2].alpha);
    EXPECT_EQ(0, dest_scan[3].blue);
    EXPECT_EQ(255, dest_scan[3].green);
    EXPECT_EQ(100, dest_scan[3].red);
    EXPECT_EQ(255, dest_scan[3].alpha);
    EXPECT_EQ(97, dest_scan[4].blue);
    EXPECT_EQ(196, dest_scan[4].green);
    EXPECT_EQ(61, dest_scan[4].red);
    EXPECT_EQ(161, dest_scan[4].alpha);
    EXPECT_EQ(26, dest_scan[5].blue);
    EXPECT_EQ(239, dest_scan[5].green);
    EXPECT_EQ(89, dest_scan[5].red);
    EXPECT_EQ(222, dest_scan[5].alpha);
    EXPECT_EQ(141, dest_scan[6].blue);
    EXPECT_EQ(169, dest_scan[6].green);
    EXPECT_EQ(44, dest_scan[6].red);
    EXPECT_EQ(222, dest_scan[6].alpha);
    EXPECT_EQ(46, dest_scan[7].blue);
    EXPECT_EQ(227, dest_scan[7].green);
    EXPECT_EQ(81, dest_scan[7].red);
    EXPECT_EQ(244, dest_scan[7].alpha);
  }
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraDarken) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kDarken,
                              /*bRgbByteOrder=*/false));

  {
    std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
    fxcrt::Copy(kDestScan, dest_scan);
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kSrcScan1),
                                      dest_scan.size(), {});
    EXPECT_EQ(255, dest_scan[0].blue);
    EXPECT_EQ(100, dest_scan[0].green);
    EXPECT_EQ(0, dest_scan[0].red);
    EXPECT_EQ(0, dest_scan[0].alpha);
    EXPECT_EQ(255, dest_scan[1].blue);
    EXPECT_EQ(100, dest_scan[1].green);
    EXPECT_EQ(0, dest_scan[1].red);
    EXPECT_EQ(255, dest_scan[1].alpha);
    EXPECT_EQ(255, dest_scan[2].blue);
    EXPECT_EQ(100, dest_scan[2].green);
    EXPECT_EQ(0, dest_scan[2].red);
    EXPECT_EQ(255, dest_scan[2].alpha);
    EXPECT_EQ(255, dest_scan[3].blue);
    EXPECT_EQ(100, dest_scan[3].green);
    EXPECT_EQ(0, dest_scan[3].red);
    EXPECT_EQ(255, dest_scan[3].alpha);
    EXPECT_EQ(255, dest_scan[4].blue);
    EXPECT_EQ(100, dest_scan[4].green);
    EXPECT_EQ(0, dest_scan[4].red);
    EXPECT_EQ(161, dest_scan[4].alpha);
    EXPECT_EQ(255, dest_scan[5].blue);
    EXPECT_EQ(100, dest_scan[5].green);
    EXPECT_EQ(0, dest_scan[5].red);
    EXPECT_EQ(222, dest_scan[5].alpha);
    EXPECT_EQ(255, dest_scan[6].blue);
    EXPECT_EQ(100, dest_scan[6].green);
    EXPECT_EQ(0, dest_scan[6].red);
    EXPECT_EQ(222, dest_scan[6].alpha);
    EXPECT_EQ(255, dest_scan[7].blue);
    EXPECT_EQ(100, dest_scan[7].green);
    EXPECT_EQ(0, dest_scan[7].red);
    EXPECT_EQ(244, dest_scan[7].alpha);
  }
  {
    std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
    fxcrt::Copy(kDestScan, dest_scan);
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kSrcScan2),
                                      dest_scan.size(), {});
    EXPECT_EQ(100, dest_scan[0].blue);
    EXPECT_EQ(0, dest_scan[0].green);
    EXPECT_EQ(255, dest_scan[0].red);
    EXPECT_EQ(0, dest_scan[0].alpha);
    EXPECT_EQ(100, dest_scan[1].blue);
    EXPECT_EQ(0, dest_scan[1].green);
    EXPECT_EQ(255, dest_scan[1].red);
    EXPECT_EQ(255, dest_scan[1].alpha);
    EXPECT_EQ(255, dest_scan[2].blue);
    EXPECT_EQ(100, dest_scan[2].green);
    EXPECT_EQ(0, dest_scan[2].red);
    EXPECT_EQ(255, dest_scan[2].alpha);
    EXPECT_EQ(100, dest_scan[3].blue);
    EXPECT_EQ(0, dest_scan[3].green);
    EXPECT_EQ(0, dest_scan[3].red);
    EXPECT_EQ(255, dest_scan[3].alpha);
    EXPECT_EQ(158, dest_scan[4].blue);
    EXPECT_EQ(38, dest_scan[4].green);
    EXPECT_EQ(96, dest_scan[4].red);
    EXPECT_EQ(161, dest_scan[4].alpha);
    EXPECT_EQ(115, dest_scan[5].blue);
    EXPECT_EQ(10, dest_scan[5].green);
    EXPECT_EQ(139, dest_scan[5].red);
    EXPECT_EQ(222, dest_scan[5].alpha);
    EXPECT_EQ(185, dest_scan[6].blue);
    EXPECT_EQ(55, dest_scan[6].green);
    EXPECT_EQ(24, dest_scan[6].red);
    EXPECT_EQ(222, dest_scan[6].alpha);
    EXPECT_EQ(127, dest_scan[7].blue);
    EXPECT_EQ(18, dest_scan[7].green);
    EXPECT_EQ(45, dest_scan[7].red);
    EXPECT_EQ(244, dest_scan[7].alpha);
  }
  {
    std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
    fxcrt::Copy(kDestScan, dest_scan);
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kSrcScan3),
                                      dest_scan.size(), {});
    EXPECT_EQ(0, dest_scan[0].blue);
    EXPECT_EQ(255, dest_scan[0].green);
    EXPECT_EQ(100, dest_scan[0].red);
    EXPECT_EQ(0, dest_scan[0].alpha);
    EXPECT_EQ(0, dest_scan[1].blue);
    EXPECT_EQ(255, dest_scan[1].green);
    EXPECT_EQ(100, dest_scan[1].red);
    EXPECT_EQ(255, dest_scan[1].alpha);
    EXPECT_EQ(255, dest_scan[2].blue);
    EXPECT_EQ(100, dest_scan[2].green);
    EXPECT_EQ(0, dest_scan[2].red);
    EXPECT_EQ(255, dest_scan[2].alpha);
    EXPECT_EQ(0, dest_scan[3].blue);
    EXPECT_EQ(100, dest_scan[3].green);
    EXPECT_EQ(0, dest_scan[3].red);
    EXPECT_EQ(255, dest_scan[3].alpha);
    EXPECT_EQ(97, dest_scan[4].blue);
    EXPECT_EQ(158, dest_scan[4].green);
    EXPECT_EQ(37, dest_scan[4].red);
    EXPECT_EQ(161, dest_scan[4].alpha);
    EXPECT_EQ(26, dest_scan[5].blue);
    EXPECT_EQ(184, dest_scan[5].green);
    EXPECT_EQ(53, dest_scan[5].red);
    EXPECT_EQ(222, dest_scan[5].alpha);
    EXPECT_EQ(141, dest_scan[6].blue);
    EXPECT_EQ(114, dest_scan[6].green);
    EXPECT_EQ(9, dest_scan[6].red);
    EXPECT_EQ(222, dest_scan[6].alpha);
    EXPECT_EQ(46, dest_scan[7].blue);
    EXPECT_EQ(127, dest_scan[7].green);
    EXPECT_EQ(17, dest_scan[7].red);
    EXPECT_EQ(244, dest_scan[7].alpha);
  }
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraHue) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kHue,
                              /*bRgbByteOrder=*/false));

  {
    std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
    fxcrt::Copy(kDestScan, dest_scan);
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kSrcScan1),
                                      dest_scan.size(), {});
    EXPECT_EQ(255, dest_scan[0].blue);
    EXPECT_EQ(100, dest_scan[0].green);
    EXPECT_EQ(0, dest_scan[0].red);
    EXPECT_EQ(0, dest_scan[0].alpha);
    EXPECT_EQ(255, dest_scan[1].blue);
    EXPECT_EQ(100, dest_scan[1].green);
    EXPECT_EQ(0, dest_scan[1].red);
    EXPECT_EQ(255, dest_scan[1].alpha);
    EXPECT_EQ(255, dest_scan[2].blue);
    EXPECT_EQ(100, dest_scan[2].green);
    EXPECT_EQ(0, dest_scan[2].red);
    EXPECT_EQ(255, dest_scan[2].alpha);
    EXPECT_EQ(255, dest_scan[3].blue);
    EXPECT_EQ(100, dest_scan[3].green);
    EXPECT_EQ(0, dest_scan[3].red);
    EXPECT_EQ(255, dest_scan[3].alpha);
    EXPECT_EQ(255, dest_scan[4].blue);
    EXPECT_EQ(100, dest_scan[4].green);
    EXPECT_EQ(0, dest_scan[4].red);
    EXPECT_EQ(161, dest_scan[4].alpha);
    EXPECT_EQ(255, dest_scan[5].blue);
    EXPECT_EQ(100, dest_scan[5].green);
    EXPECT_EQ(0, dest_scan[5].red);
    EXPECT_EQ(222, dest_scan[5].alpha);
    EXPECT_EQ(255, dest_scan[6].blue);
    EXPECT_EQ(100, dest_scan[6].green);
    EXPECT_EQ(0, dest_scan[6].red);
    EXPECT_EQ(222, dest_scan[6].alpha);
    EXPECT_EQ(255, dest_scan[7].blue);
    EXPECT_EQ(100, dest_scan[7].green);
    EXPECT_EQ(0, dest_scan[7].red);
    EXPECT_EQ(244, dest_scan[7].alpha);
  }
  {
    std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
    fxcrt::Copy(kDestScan, dest_scan);
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kSrcScan2),
                                      dest_scan.size(), {});
    EXPECT_EQ(100, dest_scan[0].blue);
    EXPECT_EQ(0, dest_scan[0].green);
    EXPECT_EQ(255, dest_scan[0].red);
    EXPECT_EQ(0, dest_scan[0].alpha);
    EXPECT_EQ(100, dest_scan[1].blue);
    EXPECT_EQ(0, dest_scan[1].green);
    EXPECT_EQ(255, dest_scan[1].red);
    EXPECT_EQ(255, dest_scan[1].alpha);
    EXPECT_EQ(255, dest_scan[2].blue);
    EXPECT_EQ(100, dest_scan[2].green);
    EXPECT_EQ(0, dest_scan[2].red);
    EXPECT_EQ(255, dest_scan[2].alpha);
    EXPECT_EQ(100, dest_scan[3].blue);
    EXPECT_EQ(0, dest_scan[3].green);
    EXPECT_EQ(255, dest_scan[3].red);
    EXPECT_EQ(255, dest_scan[3].alpha);
    EXPECT_EQ(158, dest_scan[4].blue);
    EXPECT_EQ(38, dest_scan[4].green);
    EXPECT_EQ(158, dest_scan[4].red);
    EXPECT_EQ(161, dest_scan[4].alpha);
    EXPECT_EQ(115, dest_scan[5].blue);
    EXPECT_EQ(10, dest_scan[5].green);
    EXPECT_EQ(229, dest_scan[5].red);
    EXPECT_EQ(222, dest_scan[5].alpha);
    EXPECT_EQ(185, dest_scan[6].blue);
    EXPECT_EQ(55, dest_scan[6].green);
    EXPECT_EQ(114, dest_scan[6].red);
    EXPECT_EQ(222, dest_scan[6].alpha);
    EXPECT_EQ(127, dest_scan[7].blue);
    EXPECT_EQ(18, dest_scan[7].green);
    EXPECT_EQ(209, dest_scan[7].red);
    EXPECT_EQ(244, dest_scan[7].alpha);
  }
  {
    std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
    fxcrt::Copy(kDestScan, dest_scan);
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kSrcScan3),
                                      dest_scan.size(), {});
    EXPECT_EQ(0, dest_scan[0].blue);
    EXPECT_EQ(255, dest_scan[0].green);
    EXPECT_EQ(100, dest_scan[0].red);
    EXPECT_EQ(0, dest_scan[0].alpha);
    EXPECT_EQ(0, dest_scan[1].blue);
    EXPECT_EQ(255, dest_scan[1].green);
    EXPECT_EQ(100, dest_scan[1].red);
    EXPECT_EQ(255, dest_scan[1].alpha);
    EXPECT_EQ(255, dest_scan[2].blue);
    EXPECT_EQ(100, dest_scan[2].green);
    EXPECT_EQ(0, dest_scan[2].red);
    EXPECT_EQ(255, dest_scan[2].alpha);
    EXPECT_EQ(0, dest_scan[3].blue);
    EXPECT_EQ(123, dest_scan[3].green);
    EXPECT_EQ(49, dest_scan[3].red);
    EXPECT_EQ(255, dest_scan[3].alpha);
    EXPECT_EQ(97, dest_scan[4].blue);
    EXPECT_EQ(163, dest_scan[4].green);
    EXPECT_EQ(49, dest_scan[4].red);
    EXPECT_EQ(161, dest_scan[4].alpha);
    EXPECT_EQ(26, dest_scan[5].blue);
    EXPECT_EQ(192, dest_scan[5].green);
    EXPECT_EQ(71, dest_scan[5].red);
    EXPECT_EQ(222, dest_scan[5].alpha);
    EXPECT_EQ(141, dest_scan[6].blue);
    EXPECT_EQ(122, dest_scan[6].green);
    EXPECT_EQ(26, dest_scan[6].red);
    EXPECT_EQ(222, dest_scan[6].alpha);
    EXPECT_EQ(46, dest_scan[7].blue);
    EXPECT_EQ(141, dest_scan[7].green);
    EXPECT_EQ(49, dest_scan[7].red);
    EXPECT_EQ(244, dest_scan[7].alpha);
  }
}
