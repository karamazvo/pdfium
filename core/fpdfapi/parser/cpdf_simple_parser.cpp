// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_simple_parser.h"

#include <cstdint>

#include "core/fpdfapi/parser/fpdf_parser_utility.h"
#include "core/fxcrt/check_op.h"

CPDF_SimpleParser::CPDF_SimpleParser(pdfium::span<const uint8_t> input)
    : data_(input) {}

CPDF_SimpleParser::~CPDF_SimpleParser() = default;

ByteStringView CPDF_SimpleParser::GetWord() {
  uint8_t cur_char;

  // Skip whitespace and comment lines.
  while (true) {
    if (cur_pos_ >= data_.size()) {
      return ByteStringView();
    }

    cur_char = data_[cur_pos_++];
    while (PDFCharIsWhitespace(cur_char)) {
      if (cur_pos_ >= data_.size()) {
        return ByteStringView();
      }
      cur_char = data_[cur_pos_++];
    }

    if (cur_char != '%') {
      break;
    }

    while (true) {
      if (cur_pos_ >= data_.size()) {
        return ByteStringView();
      }

      cur_char = data_[cur_pos_++];
      if (PDFCharIsLineEnding(cur_char)) {
        break;
      }
    }
  }

  uint32_t start_pos = cur_pos_ - 1;
  // Find names.
  if (cur_char == '/') {
    while (cur_pos_ < data_.size()) {
      cur_char = data_[cur_pos_++];
      if (!PDFCharIsOther(cur_char) && !PDFCharIsNumeric(cur_char)) {
        // Don't include the current character.
        cur_pos_--;
        break;
      }
    }
    return ByteStringView(data_.subspan(start_pos, cur_pos_ - start_pos));
  }

  if (PDFCharIsDelimiter(cur_char)) {
    return HandleDelimiter(cur_char);
  }

  while (cur_pos_ < data_.size()) {
    cur_char = data_[cur_pos_++];

    if (PDFCharIsDelimiter(cur_char) || PDFCharIsWhitespace(cur_char)) {
      cur_pos_--;
      break;
    }
  }
  return ByteStringView(data_.subspan(start_pos, cur_pos_ - start_pos));
}

ByteStringView CPDF_SimpleParser::HandleDelimiter(uint8_t delimiter) {
  CHECK_EQ(data_[cur_pos_ - 1], delimiter);
  uint32_t start_pos = cur_pos_ - 1;

  if (delimiter == '<') {
    if (cur_pos_ >= data_.size()) {
      return ByteStringView(data_.subspan(start_pos, cur_pos_ - start_pos));
    }
    if (data_[cur_pos_++] != '<') {
      while (cur_pos_ < data_.size() && data_[cur_pos_] != '>') {
        cur_pos_++;
      }

      if (cur_pos_ < data_.size()) {
        cur_pos_++;
      }
    }
  } else if (delimiter == '>') {
    if (cur_pos_ >= data_.size()) {
      return ByteStringView(data_.subspan(start_pos, cur_pos_ - start_pos));
    }
    if (data_[cur_pos_++] != '>') {
      cur_pos_--;
    }
  } else if (delimiter == '(') {
    int level = 1;
    while (cur_pos_ < data_.size()) {
      if (data_[cur_pos_] == ')') {
        level--;
        if (level == 0)
          break;
      }

      if (data_[cur_pos_] == '\\') {
        if (cur_pos_ >= data_.size()) {
          break;
        }

        cur_pos_++;
      } else if (data_[cur_pos_] == '(') {
        level++;
      }
      if (cur_pos_ >= data_.size()) {
        break;
      }

      cur_pos_++;
    }
    if (cur_pos_ < data_.size()) {
      cur_pos_++;
    }
  }
  return ByteStringView(data_.subspan(start_pos, cur_pos_ - start_pos));
}
