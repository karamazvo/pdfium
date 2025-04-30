// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_STRING_VIEW_TEMPLATE_H_
#define CORE_FXCRT_STRING_VIEW_TEMPLATE_H_

#include <ctype.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_memcpy_wrappers.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"

namespace fxcrt {

namespace internal {

inline constexpr size_t string_view_dynamic_extent =
    std::numeric_limits<size_t>::max();

// Computes a fixed extent if possible from a source container type `T`.
template <typename T>
inline constexpr size_t kStringViewComputedExtentImpl =
    string_view_dynamic_extent;
template <typename T, size_t N>
inline constexpr size_t kStringViewComputedExtentImpl<T[N]> = N;
template <typename T>
inline constexpr size_t kStringViewComputedExtent =
    kStringViewComputedExtentImpl<std::remove_cvref_t<T>>;

}  // namespace internal

// An immutable string with caller-provided storage which must outlive the
// string itself. These are not necessarily nul-terminated, so that substring
// extraction (via the Substr(), First(), and Last() methods) is copy-free.
//
// String view arguments should be passed by value, since they are small,
// rather than const-ref, even if they are not modified.
//
// Front() and Back() tolerate empty strings and must return NUL in those
// cases. Substr(), First(), and Last() tolerate out-of-range indices and
// must return an empty string view in those cases. The aim here is allowing
// callers to avoid range-checking first.
template <typename T>
class StringViewTemplate {
 public:
  using CharType = T;
  using UnsignedType = typename std::make_unsigned<CharType>::type;
  using const_iterator = const CharType*;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr StringViewTemplate() noexcept = default;
  constexpr StringViewTemplate(const StringViewTemplate& src) noexcept =
      default;

  template <size_t N = internal::kStringViewComputedExtent<CharType>>
    requires(N == internal::string_view_dynamic_extent)
  // Deliberately implicit to avoid calling on every string literal.
  // NOLINTNEXTLINE(runtime/explicit)
  StringViewTemplate(const CharType* ptr) noexcept
      // SAFETY: from length() function.
      : span_(UNSAFE_BUFFERS(
            pdfium::span(ptr,
                         ptr ? std::char_traits<CharType>::length(ptr) : 0))) {}

  template <size_t Extent>
  explicit constexpr StringViewTemplate(
      const CharType (&str LIFETIME_BOUND)[Extent])
      ENABLE_IF_ATTR(str[Extent - 1u] == UnsignedType{0},
                     "requires string literal as input")
      : span_(pdfium::span(str).template first<Extent - 1>()) {}

  explicit constexpr StringViewTemplate(
      const pdfium::span<const CharType>& other) noexcept
      : span_(other) {}

  explicit StringViewTemplate(
      const pdfium::span<const UnsignedType>& other) noexcept
    requires(!std::is_same<UnsignedType, CharType>::value)
  {
    if (!other.empty()) {
      span_ = reinterpret_span<const CharType>(other);
    }
  }

  // Deliberately implicit to avoid calling on every char literal.
  // |ch| must be an lvalue that outlives the StringViewTemplate.
  // NOLINTNEXTLINE(runtime/explicit)
  constexpr StringViewTemplate(const CharType& ch) noexcept
      : span_(pdfium::span_from_ref(ch)) {}

  UNSAFE_BUFFER_USAGE
  constexpr StringViewTemplate(const CharType* ptr, size_t size) noexcept
      // SAFETY: propagated to caller via UNSAFE_BUFFER_USAGE.
      : span_(UNSAFE_BUFFERS(pdfium::span(ptr, size))) {}

  UNSAFE_BUFFER_USAGE StringViewTemplate(const UnsignedType* ptr,
                                         size_t size) noexcept
    requires(!std::is_same<UnsignedType, CharType>::value)
      // SAFETY: propagated to caller via UNSAFE_BUFFER_USAGE.
      : span_(UNSAFE_BUFFERS(
            pdfium::span(reinterpret_cast<const CharType*>(ptr), size))) {}

  StringViewTemplate& operator=(const CharType* src) {
    // SAFETY: caller ensures `src` is nul-terminated so `length()` is correct.
    span_ = UNSAFE_BUFFERS(
        pdfium::span(src, src ? std::char_traits<CharType>::length(src) : 0));
    return *this;
  }

  StringViewTemplate& operator=(const StringViewTemplate& src) {
    span_ = src.span_;
    return *this;
  }

  const_iterator begin() const {
    return reinterpret_cast<const_iterator>(span_.begin());
  }
  const_iterator end() const {
    return reinterpret_cast<const_iterator>(span_.end());
  }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  bool operator==(const StringViewTemplate& other) const {
    return std::equal(span_.begin(), span_.end(), other.span_.begin(),
                      other.span_.end());
  }
  bool operator==(const CharType* ptr) const {
    StringViewTemplate other(ptr);
    return *this == other;
  }
  bool operator!=(const CharType* ptr) const { return !(*this == ptr); }
  bool operator!=(const StringViewTemplate& other) const {
    return !(*this == other);
  }

  bool IsASCII() const {
    for (auto c : *this) {
      if (c <= 0 || c > 127) {  // Questionable signedness of |c|.
        return false;
      }
    }
    return true;
  }

  bool EqualsASCII(const StringViewTemplate<char>& that) const {
    size_t length = GetLength();
    if (length != that.GetLength()) {
      return false;
    }

    for (size_t i = 0; i < length; ++i) {
      auto c = (*this)[i];
      if (c <= 0 || c > 127 ||
          c != that[i]) {  // Questionable signedness of |c|.
        return false;
      }
    }
    return true;
  }

  bool EqualsASCIINoCase(const StringViewTemplate<char>& that) const {
    size_t length = GetLength();
    if (length != that.GetLength()) {
      return false;
    }

    for (size_t i = 0; i < length; ++i) {
      auto c = (*this)[i];
      if (c <= 0 || c > 127 || tolower(c) != tolower(that[i])) {
        return false;
      }
    }
    return true;
  }

  uint32_t GetID() const {
    if (span_.empty()) {
      return 0;
    }

    uint32_t strid = 0;
    size_t size = std::min(static_cast<size_t>(4), span_.size());
    for (size_t i = 0; i < size; i++) {
      strid = strid * 256 + span_[i];
    }

    return strid << ((4 - size) * 8);
  }

  pdfium::span<const UnsignedType> unsigned_span() const {
    return reinterpret_span<const UnsignedType>(span_);
  }
  pdfium::span<const CharType> span() const { return span_; }
  const UnsignedType* unterminated_unsigned_str() const {
    return reinterpret_cast<const UnsignedType*>(span_.data());
  }
  const CharType* unterminated_c_str() const { return span_.data(); }

  constexpr size_t GetLength() const { return span_.size(); }
  constexpr bool IsEmpty() const { return span_.empty(); }
  bool IsValidIndex(size_t index) const { return index < span_.size(); }
  bool IsValidLength(size_t length) const { return length <= span_.size(); }

  // CHECK() if index is out of range (via span's operator[]).
  UnsignedType operator[](const size_t index) const {
    return static_cast<UnsignedType>(span_[index]);
  }

  // CHECK() if index is out of range (via span's operator[]).
  const CharType& CharAt(const size_t index) const { return span_[index]; }

  // Unlike std::string_view::front(), this is always safe and returns a
  // NUL char when the string is empty.
  UnsignedType Front() const { return !span_.empty() ? span_.front() : 0; }

  // Unlike std::string_view::back(), this is always safe and returns a
  // NUL char when the string is empty.
  UnsignedType Back() const { return !span_.empty() ? span_.back() : 0; }

  std::optional<size_t> Find(CharType ch) const {
    const auto* found =
        std::char_traits<CharType>::find(span_.data(), span_.size(), ch);
    return found ? std::optional<size_t>(found - span_.data()) : std::nullopt;
  }

  bool Contains(CharType ch) const { return Find(ch).has_value(); }

  StringViewTemplate Substr(size_t offset) const {
    // Unsigned underflow is well-defined and out-of-range is handled by
    // Substr().
    return Substr(offset, GetLength() - offset);
  }

  StringViewTemplate Substr(size_t first, size_t count) const {
    if (!span_.data()) {
      return StringViewTemplate();
    }

    if (!IsValidIndex(first)) {
      return StringViewTemplate();
    }

    if (count == 0 || !IsValidLength(count)) {
      return StringViewTemplate();
    }

    if (!IsValidIndex(first + count - 1)) {
      return StringViewTemplate();
    }

    // SAFETY: performance-sensitive, checks above equivalent to subspan()'s.
    return UNSAFE_BUFFERS(StringViewTemplate(span_.data() + first, count));
  }

  StringViewTemplate First(size_t count) const { return Substr(0, count); }

  StringViewTemplate Last(size_t count) const {
    // Unsigned underflow is well-defined and out-of-range is handled by
    // Substr().
    return Substr(GetLength() - count, count);
  }

  StringViewTemplate TrimmedRight(CharType ch) const {
    if (IsEmpty()) {
      return StringViewTemplate();
    }

    size_t pos = GetLength();
    while (pos && CharAt(pos - 1) == ch) {
      pos--;
    }

    if (pos == 0) {
      return StringViewTemplate();
    }

    // SAFETY: Loop above keeps `pos` at length of string or less.
    return UNSAFE_BUFFERS(StringViewTemplate(span_.data(), pos));
  }

  bool operator<(const StringViewTemplate& that) const {
    const size_t common_size = std::min(span_.size(), that.span_.size());
    int result = common_size ? std::char_traits<CharType>::compare(
                                   span_.data(), that.span_.data(), common_size)
                             : 0;
    return result < 0 || (result == 0 && span_.size() < that.span_.size());
  }

  bool operator>(const StringViewTemplate& that) const {
    const size_t common_size = std::min(span_.size(), that.span_.size());
    int result = common_size ? std::char_traits<CharType>::compare(
                                   span_.data(), that.span_.data(), common_size)
                             : 0;
    return result > 0 || (result == 0 && span_.size() > that.span_.size());
  }

 protected:
  // This is not a raw_span<> because StringViewTemplates must be passed by
  // value without introducing BackupRefPtr churn. Also, repeated re-assignment
  // of substrings of a StringViewTemplate to itself must avoid the same issue.
  pdfium::span<const CharType> span_;

 private:
  void* operator new(size_t) throw() { return nullptr; }
};

template <typename T>
inline bool operator==(const T* lhs, const StringViewTemplate<T>& rhs) {
  return rhs == lhs;
}
template <typename T>
inline bool operator!=(const T* lhs, const StringViewTemplate<T>& rhs) {
  return rhs != lhs;
}
template <typename T>
inline bool operator<(const T* lhs, const StringViewTemplate<T>& rhs) {
  return rhs > lhs;
}

extern template class StringViewTemplate<char>;
extern template class StringViewTemplate<wchar_t>;

using ByteStringView = StringViewTemplate<char>;
using WideStringView = StringViewTemplate<wchar_t>;

}  // namespace fxcrt

using ByteStringView = fxcrt::ByteStringView;
using WideStringView = fxcrt::WideStringView;

#endif  // CORE_FXCRT_STRING_VIEW_TEMPLATE_H_
