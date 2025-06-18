// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/crt/cfgas_decimal.h"

#include <math.h>

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/span.h"

#define FXMATH_DECIMAL_SCALELIMIT 0x1c
#define FXMATH_DECIMAL_RSHIFT32BIT(x) ((x) >> 0x10 >> 0x10)
#define FXMATH_DECIMAL_LSHIFT32BIT(x) ((x) << 0x10 << 0x10)

namespace {

inline uint8_t decimal_helper_div10(uint64_t& phi,
                                    uint64_t& pmid,
                                    uint64_t& plo) {
  uint8_t retVal;
  pmid += FXMATH_DECIMAL_LSHIFT32BIT(phi % 0xA);
  phi /= 0xA;
  plo += FXMATH_DECIMAL_LSHIFT32BIT(pmid % 0xA);
  pmid /= 0xA;
  retVal = plo % 0xA;
  plo /= 0xA;
  return retVal;
}

inline uint8_t decimal_helper_div10_any(pdfium::span<uint64_t> nums) {
  uint8_t retVal = 0;
  const int numcount = static_cast<int>(nums.size());
  for (int i = numcount - 1; i > 0; i--) {
    nums[i - 1] += FXMATH_DECIMAL_LSHIFT32BIT(nums[i] % 0xA);
    nums[i] /= 0xA;
  }
  if (numcount > 0) {
    retVal = nums[0] % 0xA;
    nums[0] /= 0xA;
  }
  return retVal;
}

inline void decimal_helper_mul10(uint64_t& phi, uint64_t& pmid, uint64_t& plo) {
  plo *= 0xA;
  pmid = pmid * 0xA + FXMATH_DECIMAL_RSHIFT32BIT(plo);
  plo = (uint32_t)plo;
  phi = phi * 0xA + FXMATH_DECIMAL_RSHIFT32BIT(pmid);
  pmid = (uint32_t)pmid;
}

inline void decimal_helper_mul10_any(pdfium::span<uint64_t> nums) {
  if (nums.empty()) {
    return;
  }
  nums[0] *= 0xA;
  const int numcount = static_cast<int>(nums.size());
  for (int i = 1; i < numcount; i++) {
    nums[i] = nums[i] * 0xA + FXMATH_DECIMAL_RSHIFT32BIT(nums[i - 1]);
    nums[i - 1] = (uint32_t)nums[i - 1];
  }
}

inline void decimal_helper_normalize(uint64_t& phi,
                                     uint64_t& pmid,
                                     uint64_t& plo) {
  phi += FXMATH_DECIMAL_RSHIFT32BIT(pmid);
  pmid = (uint32_t)pmid;
  pmid += FXMATH_DECIMAL_RSHIFT32BIT(plo);
  plo = (uint32_t)plo;
  phi += FXMATH_DECIMAL_RSHIFT32BIT(pmid);
  pmid = (uint32_t)pmid;
}

inline void decimal_helper_normalize_any(pdfium::span<uint64_t> nums) {
  const int len = static_cast<int>(nums.size());
  for (int i = len - 2; i > 0; i--) {
    nums[i + 1] += FXMATH_DECIMAL_RSHIFT32BIT(nums[i]);
    nums[i] = (uint32_t)nums[i];
  }
  for (int i = 0; i < len - 1; i++) {
    nums[i + 1] += FXMATH_DECIMAL_RSHIFT32BIT(nums[i]);
    nums[i] = (uint32_t)nums[i];
  }
}

inline int8_t decimal_helper_raw_compare_any(pdfium::span<const uint64_t> a,
                                             pdfium::span<const uint64_t> b) {
  int8_t retVal = 0;
  const int al = static_cast<int>(a.size());
  const int bl = static_cast<int>(b.size());
  for (int i = std::max(al - 1, bl - 1); i >= 0; i--) {
    uint64_t l = (i >= al ? 0 : a[i]);
    uint64_t r = (i >= bl ? 0 : b[i]);
    retVal += (l > r ? 1 : (l < r ? -1 : 0));
    if (retVal) {
      return retVal;
    }
  }
  return retVal;
}

inline void decimal_helper_dec_any(pdfium::span<uint64_t> a) {
  for (size_t i = 0; i < a.size(); i++) {
    if (a[i]--) {
      return;
    }
  }
}

inline void decimal_helper_inc_any(pdfium::span<uint64_t> a) {
  for (size_t i = 0; i < a.size(); i++) {
    a[i]++;
    if ((uint32_t)a[i] == a[i]) {
      return;
    }
    a[i] = 0;
  }
}

inline void decimal_helper_raw_mul(pdfium::span<const uint64_t> a,
                                   pdfium::span<const uint64_t> b,
                                   pdfium::span<uint64_t> c) {
  DCHECK(a.size() + b.size() <= c.size());
  std::ranges::fill(c, 0);

  const int al = static_cast<int>(a.size());
  const int bl = static_cast<int>(b.size());
  const int cl = static_cast<int>(c.size());
  for (int i = 0; i < al; i++) {
    for (int j = 0; j < bl; j++) {
      uint64_t m = (uint64_t)a[i] * b[j];
      c[i + j] += (uint32_t)m;
      c[i + j + 1] += FXMATH_DECIMAL_RSHIFT32BIT(m);
    }
  }
  for (int i = 0; i < cl - 1; i++) {
    c[i + 1] += FXMATH_DECIMAL_RSHIFT32BIT(c[i]);
    c[i] = (uint32_t)c[i];
  }
  for (int i = 0; i < cl; i++) {
    c[i] = (uint32_t)c[i];
  }
}

inline void decimal_helper_raw_div(pdfium::span<const uint64_t> a,
                                   pdfium::span<const uint64_t> b,
                                   pdfium::span<uint64_t> c) {
  std::ranges::fill(c, 0);

  std::array<uint64_t, 16> left = {};
  std::array<uint64_t, 16> right = {};
  CHECK(a.size() <= right.size());
  pdfium::span(right).first(a.size()).copy_from(a);

  std::array<uint64_t, 16> tmp;
  while (decimal_helper_raw_compare_any(pdfium::span(left).first(a.size()),
                                        pdfium::span(right).first(a.size())) <=
         0) {
    std::array<uint64_t, 16> cur;
    for (size_t i = 0; i < a.size(); i++) {
      cur[i] = left[i] + right[i];
    }

    const int al = static_cast<int>(a.size());
    for (int i = al - 1; i >= 0; i--) {
      if (i) {
        cur[i - 1] += FXMATH_DECIMAL_LSHIFT32BIT(cur[i] % 2);
      }
      cur[i] /= 2;
    }

    decimal_helper_raw_mul(pdfium::span(cur).first(a.size()), b, tmp);
    switch (decimal_helper_raw_compare_any(tmp, a)) {
      case -1:
        left = cur;
        left[0]++;
        decimal_helper_normalize_any(pdfium::span(left).first(a.size()));
        break;
      case 1:
        right = cur;
        decimal_helper_dec_any(pdfium::span(right).first(a.size()));
        break;
      case 0: {
        size_t len = std::min(a.size(), c.size());
        c.first(len).copy_from(pdfium::span(cur).first(len));
        return;
      }
    }
  }
  size_t len = std::min(a.size(), c.size());
  c.first(len).copy_from(pdfium::span(left).first(len));
}

inline bool decimal_helper_outofrange(pdfium::span<const uint64_t> a,
                                      uint8_t goal) {
  if (goal >= a.size()) {
    return false;
  }
  for (size_t i = goal; i < a.size(); i++) {
    if (a[i]) {
      return true;
    }
  }
  return false;
}

inline void decimal_helper_shrinkintorange(pdfium::span<uint64_t> a,
                                           uint8_t goal,
                                           uint8_t& scale) {
  bool bRoundUp = false;
  while (scale != 0 && (scale > FXMATH_DECIMAL_SCALELIMIT ||
                        decimal_helper_outofrange(a, goal))) {
    bRoundUp = decimal_helper_div10_any(a) >= 5;
    scale--;
  }
  if (bRoundUp) {
    CHECK(goal <= a.size());
    decimal_helper_normalize_any(a.first(goal));
    decimal_helper_inc_any(a.first(goal));
  }
}

inline void decimal_helper_truncate(uint64_t& phi,
                                    uint64_t& pmid,
                                    uint64_t& plo,
                                    uint8_t& scale,
                                    uint8_t minscale = 0) {
  while (scale > minscale) {
    uint64_t thi = phi, tmid = pmid, tlo = plo;
    if (decimal_helper_div10(thi, tmid, tlo) != 0) {
      break;
    }

    phi = thi;
    pmid = tmid;
    plo = tlo;
    scale--;
  }
}

}  // namespace

CFGAS_Decimal::CFGAS_Decimal() = default;

CFGAS_Decimal::CFGAS_Decimal(uint64_t val)
    : mid_(static_cast<uint32_t>(FXMATH_DECIMAL_RSHIFT32BIT(val))),
      lo_(static_cast<uint32_t>(val)) {}

CFGAS_Decimal::CFGAS_Decimal(uint32_t val) : lo_(static_cast<uint32_t>(val)) {}

CFGAS_Decimal::CFGAS_Decimal(uint32_t lo,
                             uint32_t mid,
                             uint32_t hi,
                             bool neg,
                             uint8_t scale)
    : hi_(hi),
      mid_(mid),
      lo_(lo),
      neg_(neg && IsNotZero()),
      u_scale_(scale > FXMATH_DECIMAL_SCALELIMIT ? 0 : scale) {}

CFGAS_Decimal::CFGAS_Decimal(int32_t val) {
  if (val >= 0) {
    *this = CFGAS_Decimal(static_cast<uint32_t>(val));
  } else if (val == std::numeric_limits<int32_t>::min()) {
    *this = CFGAS_Decimal(static_cast<uint32_t>(val));
    SetNegate();
  } else {
    *this = CFGAS_Decimal(static_cast<uint32_t>(-val));
    SetNegate();
  }
}

CFGAS_Decimal::CFGAS_Decimal(float val, uint8_t scale) {
  float newval = fabs(val);
  float divisor = powf(2.0, 64.0f);
  uint64_t bottom64 = static_cast<uint64_t>(fmodf(newval, divisor));
  uint64_t top64 = static_cast<uint64_t>(newval / divisor);
  uint64_t plo = bottom64 & 0xFFFFFFFF;
  uint64_t pmid = bottom64 >> 32;
  uint64_t phi = top64 & 0xFFFFFFFF;

  newval = fmodf(newval, 1.0f);
  for (uint8_t iter = 0; iter < scale; iter++) {
    decimal_helper_mul10(phi, pmid, plo);
    newval *= 10;
    plo += static_cast<uint64_t>(newval);
    newval = fmodf(newval, 1.0f);
  }

  plo += FXSYS_roundf(newval);
  decimal_helper_normalize(phi, pmid, plo);
  hi_ = static_cast<uint32_t>(phi);
  mid_ = static_cast<uint32_t>(pmid);
  lo_ = static_cast<uint32_t>(plo);
  neg_ = val < 0 && IsNotZero();
  u_scale_ = scale;
}

CFGAS_Decimal::CFGAS_Decimal(WideStringView str) {
  bool pointmet = false;
  bool negmet = false;
  uint8_t scale = 0;
  // Note: Rely on WideStringView::Front() => NUL on empty strings.
  while (str.Front() == ' ') {
    str = str.Substr(1);
  }
  if (str.Front() == '-') {
    negmet = true;
    str = str.Substr(1);
  } else if (str.Front() == '+') {
    str = str.Substr(1);
  }
  while (scale < FXMATH_DECIMAL_SCALELIMIT) {
    if (str.Front() == '.') {
      pointmet = true;
      str = str.Substr(1);
      continue;
    }
    if (!FXSYS_IsDecimalDigit(static_cast<wchar_t>(str.Front()))) {
      break;
    }
    hi_ = hi_ * 0xA + FXMATH_DECIMAL_RSHIFT32BIT((uint64_t)mid_ * 0xA);
    mid_ = mid_ * 0xA + FXMATH_DECIMAL_RSHIFT32BIT((uint64_t)lo_ * 0xA);
    lo_ = lo_ * 0xA + (str.Front() - '0');
    if (pointmet) {
      scale++;
    }
    str = str.Substr(1);
  }
  neg_ = negmet && IsNotZero();
  u_scale_ = scale;
}

WideString CFGAS_Decimal::ToWideString() const {
  WideString retString;
  WideString tmpbuf;
  uint64_t phi = hi_;
  uint64_t pmid = mid_;
  uint64_t plo = lo_;
  while (phi || pmid || plo) {
    tmpbuf += decimal_helper_div10(phi, pmid, plo) + '0';
  }

  uint8_t outputlen = (uint8_t)tmpbuf.GetLength();
  uint8_t scale = u_scale_;
  while (scale >= outputlen) {
    tmpbuf += '0';
    outputlen++;
  }
  if (neg_ && IsNotZero()) {
    retString += '-';
  }

  for (uint8_t idx = 0; idx < outputlen; idx++) {
    if (idx == (outputlen - scale) && scale != 0) {
      retString += '.';
    }
    retString += tmpbuf[outputlen - 1 - idx];
  }
  return retString;
}

float CFGAS_Decimal::ToFloat() const {
  return static_cast<float>(ToDouble());
}

double CFGAS_Decimal::ToDouble() const {
  double pow = (double)(1 << 16) * (1 << 16);
  double base = static_cast<double>(hi_) * pow * pow +
                static_cast<double>(mid_) * pow + static_cast<double>(lo_);
  return (neg_ ? -1 : 1) * base * powf(10.0f, -u_scale_);
}

void CFGAS_Decimal::SetScale(uint8_t newscale) {
  uint8_t oldscale = u_scale_;
  if (oldscale == newscale) {
    return;
  }

  uint64_t phi = hi_;
  uint64_t pmid = mid_;
  uint64_t plo = lo_;
  if (newscale > oldscale) {
    for (uint8_t iter = 0; iter < newscale - oldscale; iter++) {
      decimal_helper_mul10(phi, pmid, plo);
    }

    hi_ = static_cast<uint32_t>(phi);
    mid_ = static_cast<uint32_t>(pmid);
    lo_ = static_cast<uint32_t>(plo);
    neg_ = neg_ && IsNotZero();
    u_scale_ = newscale;
  } else {
    uint64_t point5_hi = 0;
    uint64_t point5_mid = 0;
    uint64_t point5_lo = 5;
    for (uint8_t iter = 0; iter < oldscale - newscale - 1; iter++) {
      decimal_helper_mul10(point5_hi, point5_mid, point5_lo);
    }

    phi += point5_hi;
    pmid += point5_mid;
    plo += point5_lo;
    decimal_helper_normalize(phi, pmid, plo);
    for (uint8_t iter = 0; iter < oldscale - newscale; iter++) {
      decimal_helper_div10(phi, pmid, plo);
    }
  }
  hi_ = static_cast<uint32_t>(phi);
  mid_ = static_cast<uint32_t>(pmid);
  lo_ = static_cast<uint32_t>(plo);
  neg_ = neg_ && IsNotZero();
  u_scale_ = newscale;
}

void CFGAS_Decimal::SetNegate() {
  if (IsNotZero()) {
    neg_ = !neg_;
  }
}

CFGAS_Decimal CFGAS_Decimal::operator*(const CFGAS_Decimal& val) const {
  const std::array<uint64_t, 3> a = {lo_, mid_, hi_};
  const std::array<uint64_t, 3> b = {val.lo_, val.mid_, val.hi_};
  std::array<uint64_t, 6> c;
  decimal_helper_raw_mul(a, b, c);
  bool neg = neg_ ^ val.neg_;
  uint8_t scale = u_scale_ + val.u_scale_;
  decimal_helper_shrinkintorange(c, 3, scale);
  return CFGAS_Decimal(static_cast<uint32_t>(c[0]), static_cast<uint32_t>(c[1]),
                       static_cast<uint32_t>(c[2]), neg, scale);
}

CFGAS_Decimal CFGAS_Decimal::operator/(const CFGAS_Decimal& val) const {
  if (!val.IsNotZero()) {
    return CFGAS_Decimal();
  }

  bool neg = neg_ ^ val.neg_;
  std::array<uint64_t, 7> a = {lo_, mid_, hi_};
  const std::array<uint64_t, 3> b = {val.lo_, val.mid_, val.hi_};
  std::array<uint64_t, 7> c = {0};
  uint8_t scale = 0;
  if (u_scale_ < val.u_scale_) {
    for (int i = val.u_scale_ - u_scale_; i > 0; i--) {
      decimal_helper_mul10_any(a);
    }
  } else {
    scale = u_scale_ - val.u_scale_;
  }

  uint8_t minscale = scale;
  if (!IsNotZero()) {
    return CFGAS_Decimal(0, 0, 0, false, minscale);
  }

  while (!a[6]) {
    decimal_helper_mul10_any(a);
    scale++;
  }

  decimal_helper_div10_any(a);
  scale--;
  decimal_helper_raw_div(pdfium::span(a).first<6u>(), b, c);
  decimal_helper_shrinkintorange(pdfium::span(c).first<6u>(), 3, scale);
  decimal_helper_truncate(c[2], c[1], c[0], scale, minscale);
  return CFGAS_Decimal(static_cast<uint32_t>(c[0]), static_cast<uint32_t>(c[1]),
                       static_cast<uint32_t>(c[2]), neg, scale);
}
