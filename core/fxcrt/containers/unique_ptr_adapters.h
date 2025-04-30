// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_CONTAINERS_UNIQUE_PTR_ADAPTERS_H_
#define BASE_CONTAINERS_UNIQUE_PTR_ADAPTERS_H_

#include <memory>

#include "core/fxcrt/unowned_ptr.h"

namespace pdfium {

// UniquePtrMatcher is useful for finding an element in a container of
// unique_ptrs when you have the raw pointer.
//
// Example usage:
//   std::vector<std::unique_ptr<Foo>> vector;
//   Foo* element = ...
//   auto iter = std::ranges::find_if(vector, MatchesUniquePtr(element));
//
// Example of erasing from container:
//   EraseIf(v, MatchesUniquePtr(element));
//
template <class T, class Deleter = std::default_delete<T>>
struct UniquePtrMatcher {
  explicit UniquePtrMatcher(T* t) : t_(t) {}

  bool operator()(const std::unique_ptr<T, Deleter>& o) {
    return o.get() == t_;
  }

 private:
  const UnownedPtr<T> t_;
};

template <class T, class Deleter = std::default_delete<T>>
UniquePtrMatcher<T, Deleter> MatchesUniquePtr(T* t) {
  return UniquePtrMatcher<T, Deleter>(t);
}

}  // namespace pdfium

#endif  // BASE_CONTAINERS_UNIQUE_PTR_ADAPTERS_H_
