// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_simple_parser.h"

#include <stdint.h>

#include "core/fpdfapi/parser/fpdf_parser_utility.h"
#include "core/fxcrt/check_op.h"

CPDF_SimpleParser::CPDF_SimpleParser(pdfium::span<const uint8_t> input)
    : data_(input) {}

CPDF_SimpleParser::~CPDF_SimpleParser() = default;

ByteStringView CPDF_SimpleParser::GetWord() {
  uint8_t cur_char;

  // Skip whitespace and comment lines.
  while (true) {
    if (cur_position_ >= data_.size()) {
      return ByteStringView();
    }

    cur_char = data_[cur_position_++];
    while (PDFCharIsWhitespace(cur_char)) {
      if (cur_position_ >= data_.size()) {
        return ByteStringView();
      }
      cur_char = data_[cur_position_++];
    }

    if (cur_char != '%') {
      break;
    }

    while (true) {
      if (cur_position_ >= data_.size()) {
        return ByteStringView();
      }

      cur_char = data_[cur_position_++];
      if (PDFCharIsLineEnding(cur_char)) {
        break;
      }
    }
  }

  if (PDFCharIsDelimiter(cur_char)) {
    return HandleDelimiter(cur_char);
  }

  uint32_t start_position = cur_position_ - 1;
  while (cur_position_ < data_.size()) {
    cur_char = data_[cur_position_++];

    if (PDFCharIsDelimiter(cur_char) || PDFCharIsWhitespace(cur_char)) {
      cur_position_--;
      break;
    }
  }
  return ByteStringView(
      data_.subspan(start_position, cur_position_ - start_position));
}

ByteStringView CPDF_SimpleParser::HandleDelimiter(uint8_t delimiter) {
  CHECK_EQ(data_[cur_position_ - 1], delimiter);
  uint32_t start_position = cur_position_ - 1;

  if (delimiter == '/') {
    // Find names.
    while (cur_position_ < data_.size()) {
      uint8_t cur_char = data_[cur_position_];
      // Don't include characters that aren't other nor numeric.
      if (!PDFCharIsOther(cur_char) && !PDFCharIsNumeric(cur_char)) {
        return ByteStringView(
            data_.subspan(start_position, cur_position_ - start_position));
      }
      ++cur_position_;
    }
    return ByteStringView();
  } else if (delimiter == '<') {
    if (cur_position_ >= data_.size()) {
      return ByteStringView(
          data_.subspan(start_position, cur_position_ - start_position));
    }
    if (data_[cur_position_++] != '<') {
      while (cur_position_ < data_.size() && data_[cur_position_] != '>') {
        cur_position_++;
      }

      if (cur_position_ < data_.size()) {
        cur_position_++;
      }
    }
  } else if (delimiter == '>') {
    if (cur_position_ >= data_.size()) {
      return ByteStringView(
          data_.subspan(start_position, cur_position_ - start_position));
    }
    if (data_[cur_position_++] != '>') {
      cur_position_--;
    }
  } else if (delimiter == '(') {
    int level = 1;
    while (cur_position_ < data_.size()) {
      if (data_[cur_position_] == ')') {
        level--;
        if (level == 0) {
          break;
        }
      }

      if (data_[cur_position_] == '\\') {
        if (cur_position_ >= data_.size()) {
          break;
        }

        cur_position_++;
      } else if (data_[cur_position_] == '(') {
        level++;
      }
      if (cur_position_ >= data_.size()) {
        break;
      }

      cur_position_++;
    }
    if (cur_position_ < data_.size()) {
      cur_position_++;
    }
  }
  return ByteStringView(
      data_.subspan(start_position, cur_position_ - start_position));
}
