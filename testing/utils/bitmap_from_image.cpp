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
  std::cerr << "LOG: BitmapFromPng called with relative path: " << image_path
            << std::endl;
  std::string filepath = PathService::GetTestFilePath(image_path);
  std::cerr << "LOG: Absolute resolved path: " << filepath << std::endl;
  std::ifstream file(filepath.c_str(), std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    std::cerr << "ERROR: Failed to open file at: " << filepath << std::endl;
    return nullptr;
  }
  std::cerr << "LOG: File opened successfully." << std::endl;
  std::streamsize file_size = file.tellg();
  if (file_size <= 0) {
    std::cerr << "ERROR: File size is non-positive: " << file_size << std::endl;
    return nullptr;
  }
  std::cerr << "LOG: File size reported as: " << file_size << " bytes."
            << std::endl;
  file.seekg(0, std::ios::beg);
  std::vector<uint8_t> png_data(static_cast<size_t>(file_size));

  if (!file.read(reinterpret_cast<char*>(png_data.data()), file_size)) {
    std::cerr << "ERROR: Failed to read full file contents." << std::endl;
    return nullptr;
  }
  std::cerr << "LOG: Successfully read " << png_data.size() << " bytes."
            << std::endl;
  int width = 0;
  int height = 0;

  pdfium::span<const uint8_t> input_span(png_data.data(), png_data.size());
  std::vector<uint8_t> pixel_data =
      image_diff_png::DecodePNG(input_span, true, &width, &height);
  std::cerr << "LOG: DecodePNG finished. Width: " << width
            << ", Height: " << height << ", Pixels: " << pixel_data.size()
            << std::endl;

  if (pixel_data.empty() || width <= 0 || height <= 0) {
    std::cerr << "ERROR: Decoding failed or dimensions invalid." << std::endl;
    return nullptr;
  }
  FX_SAFE_SIZE_T expected_size = FX_SAFE_SIZE_T(width) * height * 4;
  if (expected_size.ValueOrDie() != pixel_data.size()) {
    std::cerr << "WARNING: Decoded size mismatch. Expected: "
              << static_cast<unsigned long long>(expected_size.ValueOrDie())
              << ", Actual: "
              << static_cast<unsigned long long>(pixel_data.size())
              << std::endl;
  }

  std::cerr << "LOG: First 16 BGRA bytes (4 pixels): ";
  for (size_t i = 0; i < std::min((size_t)16, pixel_data.size()); ++i) {
    std::cerr << std::hex << (int)pixel_data[i] << " ";
  }
  std::cerr << std::dec << std::endl;

  return FPDFBitmap_CreateEx(width, height, FPDFBitmap_BGRA, pixel_data.data(),
                             width * 4);
}
