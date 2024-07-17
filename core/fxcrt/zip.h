// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_ZIP_H_
#define CORE_FXCRT_ZIP_H_

#include <stdint.h>

#include <tuple>
#include <utility>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"

namespace fxcrt {

// Vastly simplified implementation of ideas from C++23 zip_view<>. Allows
// safe traversal of two ranges with a single bounds check per iteration.

// Example usage:
//   struct RGB { uint8_t r; uint8_t g; uint8_t b; };
//   const uint8_t gray[256] = { ... };
//   RGB rgbs[260];
//   for (auto [in, out] : Zip(gray, rgbs)) {
//     out.r = in;
//     out.g = in;
//     out.b = in;
//   }
// which fills the first 256 elements of rgbs with the corresponding gray
// value in each component, say.

// Differences include:
// - Only zips together two views instead of N.
// - Size is determined by the first view, which must be smaller than the
//   second view.
// - First view is presumed to be "input-like" and is const, second view is
//   presumed to be "output-like" and is non-const.
// - Only those methods required to support use in a range-based for-loop
//   are provided.

template <typename... Ts>
class ZipView {
 public:
  struct Iter {
    bool operator==(const Iter& that) const {
      return std::get<0>(iterators) == std::get<0>(that.iterators);
    }
    bool operator!=(const Iter& that) const {
      return std::get<0>(iterators) != std::get<0>(that.iterators);
    }

    UNSAFE_BUFFER_USAGE Iter& operator++() {
      // SAFETY: required from caller.
      UNSAFE_BUFFERS(PlusPlusHelper(std::make_index_sequence<sizeof...(Ts)>()));
      return *this;
    }

    std::tuple<typename Ts::reference...> operator*() const {
      return {*(std::get<0>(iterators)), *(std::get<1>(iterators))};
    }

    template <size_t... Ns>
    UNSAFE_BUFFER_USAGE void PlusPlusHelper(std::index_sequence<Ns...>) {
      UNSAFE_BUFFERS({ (++std::get<Ns>(iterators), ...); });
    }

    std::tuple<typename Ts::iterator...> iterators;
  };

  explicit ZipView(Ts... spans) : spans_(spans...) {
    CHECK_LE(std::get<0>(spans_).size(), std::get<1>(spans_).size());
  }

  Iter begin() const {
    return {{std::get<0>(spans_).begin(), std::get<1>(spans_).begin()}};
  }
  Iter end() const {
    return {{std::get<0>(spans_).end(), std::get<1>(spans_).end()}};
  }

 private:
  std::tuple<Ts...> spans_;
};

template <typename... Ts>
auto Zip(Ts&&... args) {
  return ZipView(pdfium::span(args)...);
}

}  // namespace fxcrt

#endif  // CORE_FXCRT_ZIP_H_
