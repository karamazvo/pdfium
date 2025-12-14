// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "testing/utils/bitmap_from_image.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "core/fxcrt/fx_safe_types.h"
#include "testing/image_diff/image_diff_png.h"
#include "testing/utils/path_service.h"

FPDF_BITMAP BitmapFromImage::BitmapFromPng(const char* image_path) {
  std::string filepath = PathService::GetTestFilePath(image_path);
  std::ifstream file(filepath.c_str(), std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return nullptr;
  }

  std::streamsize file_size = file.tellg();
  if (file_size <= 0) {
    return nullptr;
  }

  file.seekg(0, std::ios::beg);
  std::vector<uint8_t> png_data(static_cast<size_t>(file_size));
  if (!file.read(reinterpret_cast<char*>(png_data.data()), file_size)) {
    return nullptr;
  }

  int width = 0;
  int height = 0;

  pdfium::span<const uint8_t> input_span(png_data.data(), png_data.size());
  std::vector<uint8_t> pixel_data =
      image_diff_png::DecodePNG(input_span, true, &width, &height);

  if (pixel_data.empty() || width <= 0 || height <= 0) {
    return nullptr;
  }

  FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, FPDFBitmap_BGRA);
  if (!bitmap) {
    return nullptr;
  }

  uint8_t* dest = static_cast<uint8_t*>(FPDFBitmap_GetBuffer(bitmap));
  int stride = FPDFBitmap_GetStride(bitmap);

  const uint8_t* src = pixel_data.data();
  const size_t row_bytes = static_cast<size_t>(width) * 4;

  for (int y = 0; y < height; ++y) {
    std::copy(src + y * row_bytes, src + (y + 1) * row_bytes,
              dest + y * stride);
  }

  return bitmap;
}
