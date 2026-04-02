// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstddef>
#include <cstdint>

#include "third_party/rust/jxl/v0_3/wrapper/lib.rs.h"

namespace pdfium::jxl {

// Legacy stub kept temporarily for bringup; real decoding logic lives in
// jxl_decoder.{h,cc}.
bool SignatureCheck(const uint8_t* data, size_t len) {
  return blink::jxl_rs::jxl_rs_signature_check(
      rust::Slice<const uint8_t>(data, len));
}

}  // namespace pdfium::jxl
