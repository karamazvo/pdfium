// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/gif/lzw_decompressor.h"

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/ptr_util.h"

namespace fxcodec {

std::unique_ptr<LZWDecompressor> LZWDecompressor::Create(uint8_t color_exp,
                                                         uint8_t code_exp) {
  // |color_exp| generates 2^(n + 1) codes, where as the code_exp reserves 2^n.
  // This is a quirk of the GIF spec.
  if (code_exp > GIF_MAX_LZW_EXP || code_exp < color_exp + 1) {
    return nullptr;
  }

  // Private ctor.
  return pdfium::WrapUnique(new LZWDecompressor(color_exp, code_exp));
}

LZWDecompressor::LZWDecompressor(uint8_t color_exp, uint8_t code_exp)
    : code_size_(code_exp),
      code_color_end_(static_cast<uint16_t>(1 << (color_exp + 1))),
      code_clear_(static_cast<uint16_t>(1 << code_exp)),
      code_end_(static_cast<uint16_t>((1 << code_exp) + 1)) {
  ClearTable();
}

LZWDecompressor::~LZWDecompressor() = default;
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "third_party/pdfium/core/fxge/cfx_span.h"

// Define a result type for the Decode function using absl::variant.
// It will hold either a Status enum on failure/incompletion or a span of the
// written data on success.
using LZWDecodeResult = absl::variant<LZWDecompressor::Status, pdfium::span<uint8_t>>;

LZWDecompressor::LZWDecodeResult LZWDecompressor::Decode(
    pdfium::span<uint8_t> dest_span) {
  // If there's no compressed input data available, we can't proceed.
  if (avail_input_.empty()) {
    return Status::kUnfinished;
  }

  // If the destination buffer has no space, we cannot write anything.
  if (dest_span.empty()) {
    return Status::kInsufficientDestSize;
  }

  // Use a size_t to track the total bytes written into the destination span.
  size_t total_written = 0;

  // First, extract any data left in the internal buffer from a previous call.
  if (decompressed_next_ != 0) {
    pdfium::span<uint8_t> output_target = dest_span.subspan(total_written);
    size_t extracted_size = ExtractData(output_target);
    total_written += extracted_size;

    // If the internal buffer is still not empty, dest_span was too small.
    if (decompressed_next_ != 0) {
      return Status::kInsufficientDestSize;
    }
  }

  // Main decoding loop: continues as long as there is space in the destination
  // and there is input data to process.
  while (total_written < dest_span.size() &&
         (!avail_input_.empty() || bits_left_ >= code_size_cur_)) {
    // Check for invalid code size, which indicates a stream error.
    if (code_size_cur_ > GIF_MAX_LZW_EXP) {
      return Status::kError;
    }

    // Refill the bit buffer from the input stream if needed.
    if (!avail_input_.empty()) {
      // This check protects against overflow in the bit buffer.
      if (bits_left_ > 31) {
        return Status::kError;
      }

      FX_SAFE_UINT32 safe_code = avail_input_.front();
      safe_code <<= bits_left_;
      safe_code |= code_store_;
      if (!safe_code.IsValid()) {
        return Status::kError;
      }

      code_store_ = safe_code.ValueOrDie();
      avail_input_ = avail_input_.subspan(1);
      bits_left_ += 8;
    }

    // Process all full codes currently available in the bit buffer.
    while (bits_left_ >= code_size_cur_) {
      // Ensure there's still space before processing the next code.
      if (total_written >= dest_span.size()) {
        return Status::kInsufficientDestSize;
      }

      uint16_t code =
          static_cast<uint16_t>(code_store_) & ((1 << code_size_cur_) - 1);
      code_store_ >>= code_size_cur_;
      bits_left_ -= code_size_cur_;

      if (code == code_clear_) {
        ClearTable();
        continue;
      }
      if (code == code_end_) {
        // End-of-data code found. Return success with the subspan of
        // the destination that has been written to.
        return dest_span.first(total_written);
      }

      // Standard LZW decoding logic.
      if (code_old_ != kInvalidCode) { // Use a named constant for clarity
        if (code_next_ < GIF_MAX_LZW_CODE) {
          if (code == code_next_) {
            AddCode(code_old_, code_first_);
            if (!DecodeString(code)) {
              return Status::kError;
            }
          } else if (code > code_next_) {
            return Status::kError; // Invalid code sequence
          } else {
            if (!DecodeString(code)) {
              return Status::kError;
            }
            uint8_t append_char = decompressed_[decompressed_next_ - 1];
            AddCode(code_old_, append_char);
          }
        }
      } else {
        if (!DecodeString(code)) {
          return Status::kError;
        }
      }

      code_old_ = code;

      // Extract the newly decoded string into the available part of dest_span.
      pdfium::span<uint8_t> output_target = dest_span.subspan(total_written);
      size_t extracted_size = ExtractData(output_target);
      total_written += extracted_size;

      // If ExtractData couldn't write everything, the destination is full.
      if (decompressed_next_ != 0) {
        return Status::kInsufficientDestSize;
      }
    }
  }

  // The loop terminated. If the end code hasn't been found, the process is
  // either unfinished (ran out of input) or requires a larger destination.
  // If more input is available or data is pending, the destination was too small.
  if (!avail_input_.empty() || decompressed_next_ != 0) {
      return Status::kInsufficientDestSize;
  }
  
  // Otherwise, we've run out of input before seeing the end code.
  return Status::kUnfinished;
}
void LZWDecompressor::ClearTable() {
  code_size_cur_ = code_size_ + 1;
  code_next_ = code_end_ + 1;
  code_old_ = static_cast<uint16_t>(-1);
  std::ranges::fill(code_table_, CodeEntry{});  // Aggregate initialization.
  static_assert(std::is_aggregate_v<CodeEntry>);
  for (uint16_t i = 0; i < code_clear_; i++) {
    code_table_[i].suffix = static_cast<uint8_t>(i);
  }
  decompressed_.resize(code_next_ - code_clear_ + 1);
  decompressed_next_ = 0;
}

void LZWDecompressor::AddCode(uint16_t prefix_code, uint8_t append_char) {
  if (code_next_ == GIF_MAX_LZW_CODE) {
    return;
  }

  code_table_[code_next_].prefix = prefix_code;
  code_table_[code_next_].suffix = append_char;
  if (++code_next_ < GIF_MAX_LZW_CODE) {
    if (code_next_ >> code_size_cur_) {
      code_size_cur_++;
    }
  }
}

bool LZWDecompressor::DecodeString(uint16_t code) {
  decompressed_.resize(code_next_ - code_clear_ + 1);
  decompressed_next_ = 0;

  while (code >= code_clear_ && code <= code_next_) {
    if (code == code_table_[code].prefix ||
        decompressed_next_ >= decompressed_.size()) {
      return false;
    }

    decompressed_[decompressed_next_++] = code_table_[code].suffix;
    code = code_table_[code].prefix;
  }

  if (code >= code_color_end_) {
    return false;
  }

  decompressed_[decompressed_next_++] = static_cast<uint8_t>(code);
  code_first_ = static_cast<uint8_t>(code);
  return true;
}

size_t LZWDecompressor::ExtractData(pdfium::span<uint8_t> dest_span) {
  if (dest_span.empty()) {
    return 0;
  }
  size_t copy_size = std::min(dest_span.size(), decompressed_next_);
  UNSAFE_TODO(std::reverse_copy(
      decompressed_.data() + decompressed_next_ - copy_size,
      decompressed_.data() + decompressed_next_, dest_span.data()));
  decompressed_next_ -= copy_size;
  return copy_size;
}

}  // namespace fxcodec
