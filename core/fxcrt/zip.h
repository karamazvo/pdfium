// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_ZIP_H_
#define CORE_FXCRT_ZIP_H_

#include <stdint.h>

#include <utility>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"

namespace fxcrt {

// Vastly simplified implementatin of C++23 zip views.

template <typename T, typename U>
class ZipView {
 public:
  struct Iter {
    bool operator==(const Iter& that) const { return first == that.first; }

    bool operator!=(const Iter& that) const { return first != that.first; }

    UNSAFE_BUFFER_USAGE Iter& operator++() {
      // SAFETY: required from caller, enforced by UNSAFE_BUFFER_USAGE.
      UNSAFE_BUFFERS(++first);
      UNSAFE_BUFFERS(++second);
      return *this;
    }

    std::pair<typename T::reference, typename U::reference> operator*() const {
      return {*first, *second};
    }

    T::iterator first;
    U::iterator second;
  };

  ZipView(T first, U second) : first_(first), second_(second) {
    CHECK_LE(first.size(), second.size());
  }

  Iter begin() { return {first_.begin(), second_.begin()}; }
  Iter end() { return {first_.end(), second_.end()}; }

 private:
  T first_;
  U second_;
};

template <typename T, typename U>
auto Zip(T&& first, U&& second) {
  return ZipView(pdfium::span(first), pdfium::span(second));
}

}  // namespace fxcrt

#endif  // CORE_FXCRT_ZIP_H_
