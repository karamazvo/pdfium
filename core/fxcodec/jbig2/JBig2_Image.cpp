// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/jbig2/JBig2_Image.h"

#include <limits.h>
#include <stddef.h>

#include <algorithm>
#include <memory>

#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_2d_size.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_memcpy_wrappers.h"
#include "core/fxcrt/fx_memory.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span_util.h"

#define JBIG2_GETDWORD(buf)                  \
  ((static_cast<uint32_t>((buf)[0]) << 24) | \
   (static_cast<uint32_t>((buf)[1]) << 16) | \
   (static_cast<uint32_t>((buf)[2]) << 8) |  \
   (static_cast<uint32_t>((buf)[3]) << 0))

#define JBIG2_PUTDWORD(buf, val)                 \
  ((buf)[0] = static_cast<uint8_t>((val) >> 24), \
   (buf)[1] = static_cast<uint8_t>((val) >> 16), \
   (buf)[2] = static_cast<uint8_t>((val) >> 8),  \
   (buf)[3] = static_cast<uint8_t>((val) >> 0))

namespace {

const int kMaxImagePixels = INT_MAX - 31;
const int kMaxImageBytes = kMaxImagePixels / 8;

int BitIndexToByte(int index) {
  return index / 8;
}

int BitIndexToAlignedByte(int index) {
  return index / 32 * 4;
}

uint32_t DoCompose(JBig2ComposeOp op, uint32_t val1, uint32_t val2) {
  switch (op) {
    case JBIG2_COMPOSE_OR:
      return val1 | val2;
    case JBIG2_COMPOSE_AND:
      return val1 & val2;
    case JBIG2_COMPOSE_XOR:
      return val1 ^ val2;
    case JBIG2_COMPOSE_XNOR:
      return ~(val1 ^ val2);
    case JBIG2_COMPOSE_REPLACE:
      return val1;
  }
  NOTREACHED();
}

uint32_t DoComposeWithMask(JBig2ComposeOp op,
                           uint32_t val1,
                           uint32_t val2,
                           uint32_t mask) {
  return (val2 & ~mask) | (DoCompose(op, val1, val2) & mask);
}

}  // namespace

CJBig2_Image::CJBig2_Image(int32_t w, int32_t h) {
  if (w <= 0 || h <= 0 || w > kMaxImagePixels) {
    return;
  }

  int32_t stride_pixels = FxAlignToBoundary<32>(w);
  if (h > kMaxImagePixels / stride_pixels) {
    return;
  }

  width_ = w;
  height_ = h;
  stride_ = stride_pixels / 8;
  CHECK_GE(stride_, 0);
  data_.Reset(std::unique_ptr<uint8_t, FxFreeDeleter>(
      FX_Alloc2D(uint8_t, stride_, height_)));
}

CJBig2_Image::CJBig2_Image(int32_t w,
                           int32_t h,
                           int32_t stride,
                           pdfium::span<uint8_t> pBuf) {
  if (w < 0 || h < 0) {
    return;
  }

  // Stride must be word-aligned.
  if (stride < 0 || stride > kMaxImageBytes || stride % 4 != 0) {
    return;
  }

  int32_t stride_pixels = 8 * stride;
  if (stride_pixels < w || h > kMaxImagePixels / stride_pixels) {
    return;
  }

  width_ = w;
  height_ = h;
  stride_ = stride;
  CHECK_GE(stride_, 0);
  data_.Reset(pBuf.data());
}

CJBig2_Image::CJBig2_Image(const CJBig2_Image& other)
    : width_(other.width_), height_(other.height_), stride_(other.stride_) {
  if (other.has_data()) {
    data_.Reset(std::unique_ptr<uint8_t, FxFreeDeleter>(
        FX_Alloc2D(uint8_t, stride_, height_)));
    fxcrt::spancpy(span(), other.span());
  }
}

CJBig2_Image::~CJBig2_Image() = default;

// static
bool CJBig2_Image::IsValidImageSize(int32_t w, int32_t h) {
  return w > 0 && w <= kJBig2MaxImageSize && h > 0 && h <= kJBig2MaxImageSize;
}

pdfium::span<const uint8_t> CJBig2_Image::span() const {
  // SAFETY: If `data_` is owned, then `this` must have allocate the right
  // amount. If `data_` is not owned, then safety requires correctness from the
  // caller that constructed `this`. Also requires caller to check has_data()
  // returns true.
  return UNSAFE_BUFFERS(pdfium::span(data(), Fx2DSizeOrDie(stride_, height_)));
}

pdfium::span<uint8_t> CJBig2_Image::span() {
  // SAFETY: Same as const-version of span() above.
  return UNSAFE_BUFFERS(pdfium::span(data(), Fx2DSizeOrDie(stride_, height_)));
}

int CJBig2_Image::GetPixel(int32_t x, int32_t y) const {
  if (!IsValidPixel(x, y)) {
    return 0;
  }

  pdfium::span<const uint8_t> line = GetLine(y);
  int32_t m = BitIndexToByte(x);
  int32_t n = x & 7;
  return (line[m] >> (7 - n)) & 1;
}

void CJBig2_Image::SetPixel(int32_t x, int32_t y, int v) {
  if (!IsValidPixel(x, y)) {
    return;
  }

  pdfium::span<uint8_t> line = GetLine(y);
  int32_t m = BitIndexToByte(x);
  int32_t n = 1 << (7 - (x & 7));
  if (v) {
    line[m] |= n;
  } else {
    line[m] &= ~n;
  }
}

pdfium::span<const uint8_t> CJBig2_Image::GetLine(int32_t y) const {
  return span().subspan(GetLineOffset(y), static_cast<size_t>(stride_));
}

pdfium::span<uint8_t> CJBig2_Image::GetLine(int32_t y) {
  return span().subspan(GetLineOffset(y), static_cast<size_t>(stride_));
}

void CJBig2_Image::CopyLine(int32_t hTo, int32_t hFrom) {
  if (!IsValidLine(hTo)) {
    return;
  }

  pdfium::span<uint8_t> dest = GetLine(hTo);
  if (!IsValidLine(hFrom)) {
    std::ranges::fill(dest, 0);
    return;
  }

  fxcrt::spancpy(dest, GetLine(hFrom));
}

void CJBig2_Image::Fill(bool v) {
  if (!has_data()) {
    return;
  }

  std::ranges::fill(span(), v ? 0xff : 0);
}

bool CJBig2_Image::ComposeTo(CJBig2_Image* pDst,
                             int64_t x,
                             int64_t y,
                             JBig2ComposeOp op) {
  return data_ &&
         ComposeToInternal(pDst, x, y, op, FX_RECT(0, 0, width_, height_));
}

bool CJBig2_Image::ComposeToWithRect(CJBig2_Image* pDst,
                                     int64_t x,
                                     int64_t y,
                                     const FX_RECT& rtSrc,
                                     JBig2ComposeOp op) {
  return data_ && ComposeToInternal(pDst, x, y, op, rtSrc);
}

bool CJBig2_Image::ComposeFrom(int64_t x,
                               int64_t y,
                               CJBig2_Image* pSrc,
                               JBig2ComposeOp op) {
  return data_ && pSrc->ComposeTo(this, x, y, op);
}

bool CJBig2_Image::ComposeFromWithRect(int64_t x,
                                       int64_t y,
                                       CJBig2_Image* pSrc,
                                       const FX_RECT& rtSrc,
                                       JBig2ComposeOp op) {
  return data_ && pSrc->ComposeToWithRect(this, x, y, rtSrc, op);
}

std::unique_ptr<CJBig2_Image> CJBig2_Image::SubImage(int32_t x,
                                                     int32_t y,
                                                     int32_t w,
                                                     int32_t h) const {
  auto image = std::make_unique<CJBig2_Image>(w, h);
  if (!image->has_data() || !has_data()) {
    return image;
  }

  if (x < 0 || x >= width_ || y < 0 || y >= height_) {
    return image;
  }

  // Fast case when byte-aligned, normal slow case otherwise.
  if ((x & 7) == 0) {
    SubImageFast(x, y, w, h, image.get());
  } else {
    SubImageSlow(x, y, w, h, image.get());
  }

  return image;
}

bool CJBig2_Image::IsValidLine(int32_t y) const {
  return y >= 0 && y < height_ && has_data();
}

bool CJBig2_Image::IsValidPixel(int32_t x, int32_t y) const {
  return x >= 0 && x < width_ && IsValidLine(y);
}

size_t CJBig2_Image::GetLineOffset(int32_t y) const {
  FX_SAFE_SIZE_T size = stride_;
  size *= y;
  return size.ValueOrDie();
}

void CJBig2_Image::SubImageFast(int32_t x,
                                int32_t y,
                                int32_t w,
                                int32_t h,
                                CJBig2_Image* image) const {
  int32_t m = BitIndexToByte(x);
  size_t bytes_to_copy = std::min(image->stride_, stride_ - m);
  int32_t lines_to_copy = std::min(image->height_, height_ - y);
  for (int32_t j = 0; j < lines_to_copy; ++j) {
    pdfium::span<const uint8_t> src =
        GetLine(y + j).subspan(static_cast<size_t>(m), bytes_to_copy);
    pdfium::span<uint8_t> dest = image->GetLine(j).first(bytes_to_copy);
    fxcrt::spancpy(dest, src);
  }
}

void CJBig2_Image::SubImageSlow(int32_t x,
                                int32_t y,
                                int32_t w,
                                int32_t h,
                                CJBig2_Image* image) const {
  int32_t m = BitIndexToAlignedByte(x);
  int32_t n = x & 31;
  size_t bytes_to_copy = std::min(image->stride_, stride_ - m);
  int32_t lines_to_copy = std::min(image->height_, height_ - y);
  for (int32_t j = 0; j < lines_to_copy; ++j) {
    pdfium::span<const uint8_t> src =
        GetLine(y + j).subspan(static_cast<size_t>(m));
    pdfium::span<uint8_t> dest = image->GetLine(j).first(bytes_to_copy);
    while (!dest.empty()) {
      auto src_bytes = src.take_first<4u>();
      auto dest_bytes = dest.take_first<4u>();
      uint32_t val = JBIG2_GETDWORD(src_bytes) << n;
      if (src.size() >= 4) {
        val |= (JBIG2_GETDWORD(src.first<4u>()) >> (32 - n));
      }
      JBIG2_PUTDWORD(dest_bytes, val);
    }
  }
}

void CJBig2_Image::Expand(int32_t h, bool v) {
  if (!has_data() || h <= height_ || h > kMaxImageBytes / stride_) {
    return;
  }

  // Won't die unless kMaxImageBytes were to be increased someday.
  const size_t current_size = Fx2DSizeOrDie(height_, stride_);
  const size_t desired_size = Fx2DSizeOrDie(h, stride_);

  if (data_.IsOwned()) {
    data_.Reset(std::unique_ptr<uint8_t, FxFreeDeleter>(
        FX_Realloc(uint8_t, data_.ReleaseAndClear().release(), desired_size)));
  } else {
    pdfium::span<const uint8_t> external_buffer = span();
    data_.Reset(std::unique_ptr<uint8_t, FxFreeDeleter>(
        FX_Alloc(uint8_t, desired_size)));
    fxcrt::spancpy(span(), external_buffer);
  }
  // NOTE: Must update `height_` first, so a subsequent span() call will create
  // a span that includes the expanded portion of memory, which needs to be
  // filled. Do not reuse other spans here.
  height_ = h;
  std::ranges::fill(span().subspan(current_size), v ? 0xff : 0);
}

bool CJBig2_Image::ComposeToInternal(CJBig2_Image* pDst,
                                     int64_t x_in,
                                     int64_t y_in,
                                     JBig2ComposeOp op,
                                     const FX_RECT& rtSrc) {
  DCHECK(data_);

  // TODO(weili): Check whether the range check is correct. Should x>=1048576?
  if (x_in < -1048576 || x_in > 1048576 || y_in < -1048576 || y_in > 1048576) {
    return false;
  }
  const int32_t x = static_cast<int32_t>(x_in);
  const int32_t y = static_cast<int32_t>(y_in);

  const int32_t sw = rtSrc.Width();
  const int32_t sh = rtSrc.Height();

  const int32_t xs0 = x < 0 ? -x : 0;
  int32_t xs1;
  FX_SAFE_INT32 iChecked = pDst->width_;
  iChecked -= x;
  if (iChecked.IsValid() && sw > iChecked.ValueOrDie()) {
    xs1 = iChecked.ValueOrDie();
  } else {
    xs1 = sw;
  }

  const int32_t ys0 = y < 0 ? -y : 0;
  int32_t ys1;
  iChecked = pDst->height_;
  iChecked -= y;
  if (iChecked.IsValid() && sh > iChecked.ValueOrDie()) {
    ys1 = iChecked.ValueOrDie();
  } else {
    ys1 = sh;
  }

  if (ys0 >= ys1 || xs0 >= xs1) {
    return false;
  }

  const int32_t xd0 = std::max(x, 0);
  const int32_t yd0 = std::max(y, 0);
  const int32_t w = xs1 - xs0;
  const int32_t h = ys1 - ys0;
  const int32_t xd1 = xd0 + w;
  const uint32_t d1 = xd0 & 31;
  const uint32_t d2 = xd1 & 31;
  const uint32_t s1 = xs0 & 31;
  const uint32_t maskL = 0xffffffff >> d1;
  const uint32_t maskR = 0xffffffff << ((32 - (xd1 & 31)) % 32);
  const uint32_t maskM = maskL & maskR;

  const int src_start_line = rtSrc.top + ys0;
  const int dest_start_line = yd0;
  const size_t src_offset =
      pdfium::checked_cast<size_t>(BitIndexToAlignedByte(xs0 + rtSrc.left));
  const size_t dest_offset =
      pdfium::checked_cast<size_t>(BitIndexToAlignedByte(xd0));
  const int32_t lineLeft = stride_ - BitIndexToAlignedByte(xs0);

  if ((xd0 & ~31) == ((xd1 - 1) & ~31)) {
    if ((xs0 & ~31) == ((xs1 - 1) & ~31)) {
      if (s1 > d1) {
        const uint32_t shift = s1 - d1;
        for (int32_t i = 0; i < h; ++i) {
          int src_line = src_start_line + i;
          if (src_line >= height_) {
            return false;
          }
          int dest_line = dest_start_line + i;
          CHECK_LT(dest_line, pDst->height_);
          auto src = GetLine(src_line).subspan(src_offset);
          auto dest = pDst->GetLine(dest_line).subspan(dest_offset);

          auto src_bytes = src.first<4u>();
          auto dest_bytes = dest.first<4u>();
          uint32_t tmp1 = JBIG2_GETDWORD(src_bytes) << shift;
          uint32_t tmp2 = JBIG2_GETDWORD(dest_bytes);
          JBIG2_PUTDWORD(dest_bytes, DoComposeWithMask(op, tmp1, tmp2, maskM));
        }
        return true;
      }

      const uint32_t shift = d1 - s1;
      for (int32_t i = 0; i < h; ++i) {
        int src_line = src_start_line + i;
        if (src_line >= height_) {
          return false;
        }
        int dest_line = dest_start_line + i;
        CHECK_LT(dest_line, pDst->height_);
        auto src = GetLine(src_line).subspan(src_offset);
        auto dest = pDst->GetLine(dest_line).subspan(dest_offset);

        auto src_bytes = src.first<4u>();
        auto dest_bytes = dest.first<4u>();
        uint32_t tmp1 = JBIG2_GETDWORD(src_bytes) >> shift;
        uint32_t tmp2 = JBIG2_GETDWORD(dest_bytes);
        JBIG2_PUTDWORD(dest_bytes, DoComposeWithMask(op, tmp1, tmp2, maskM));
      }
      return true;
    }

    const uint32_t shift1 = s1 - d1;
    const uint32_t shift2 = 32 - shift1;
    for (int32_t i = 0; i < h; ++i) {
      int src_line = src_start_line + i;
      if (src_line >= height_) {
        return false;
      }
      int dest_line = dest_start_line + i;
      CHECK_LT(dest_line, pDst->height_);
      auto src = GetLine(src_line).subspan(src_offset);
      auto dest = pDst->GetLine(dest_line).subspan(dest_offset);

      auto src_bytes1 = src.first<4u>();
      auto src_bytes2 = src.subspan<4u, 4u>();
      auto dest_bytes = dest.first<4u>();
      uint32_t tmp1 = (JBIG2_GETDWORD(src_bytes1) << shift1) |
                      (JBIG2_GETDWORD(src_bytes2) >> shift2);
      uint32_t tmp2 = JBIG2_GETDWORD(dest_bytes);
      JBIG2_PUTDWORD(dest_bytes, DoComposeWithMask(op, tmp1, tmp2, maskM));
    }
    return true;
  }

  if (s1 > d1) {
    const uint32_t shift1 = s1 - d1;
    const uint32_t shift2 = 32 - shift1;
    int32_t middleDwords = (xd1 >> 5) - ((xd0 + 31) >> 5);
    for (int32_t i = 0; i < h; ++i) {
      int src_line = src_start_line + i;
      if (src_line >= height_) {
        return false;
      }
      int dest_line = dest_start_line + i;
      CHECK_LT(dest_line, pDst->height_);
      const uint8_t* lineSrc = GetLine(src_line).subspan(src_offset).data();
      uint8_t* lineDst = pDst->GetLine(dest_line).subspan(dest_offset).data();

      UNSAFE_TODO({
        const uint8_t* sp = lineSrc;
        uint8_t* dp = lineDst;
        if (d1 != 0) {
          uint32_t tmp1 = (JBIG2_GETDWORD(sp) << shift1) |
                          (JBIG2_GETDWORD(sp + 4) >> shift2);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          JBIG2_PUTDWORD(dp, DoComposeWithMask(op, tmp1, tmp2, maskL));
          sp += 4;
          dp += 4;
        }
        for (int32_t xx = 0; xx < middleDwords; xx++) {
          uint32_t tmp1 = (JBIG2_GETDWORD(sp) << shift1) |
                          (JBIG2_GETDWORD(sp + 4) >> shift2);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          JBIG2_PUTDWORD(dp, DoCompose(op, tmp1, tmp2));
          sp += 4;
          dp += 4;
        }
        if (d2 != 0) {
          uint32_t tmp1 =
              (JBIG2_GETDWORD(sp) << shift1) |
              (((sp + 4) < lineSrc + lineLeft ? JBIG2_GETDWORD(sp + 4) : 0) >>
               shift2);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          JBIG2_PUTDWORD(dp, DoComposeWithMask(op, tmp1, tmp2, maskR));
        }
      });
    }
    return true;
  }

  if (s1 == d1) {
    const int32_t middleDwords = (xd1 >> 5) - ((xd0 + 31) >> 5);
    for (int32_t i = 0; i < h; ++i) {
      int src_line = src_start_line + i;
      if (src_line >= height_) {
        return false;
      }
      int dest_line = dest_start_line + i;
      CHECK_LT(dest_line, pDst->height_);
      const uint8_t* lineSrc = GetLine(src_line).subspan(src_offset).data();
      uint8_t* lineDst = pDst->GetLine(dest_line).subspan(dest_offset).data();

      UNSAFE_TODO({
        const uint8_t* sp = lineSrc;
        uint8_t* dp = lineDst;
        if (d1 != 0) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          JBIG2_PUTDWORD(dp, DoComposeWithMask(op, tmp1, tmp2, maskL));
          sp += 4;
          dp += 4;
        }
        for (int32_t xx = 0; xx < middleDwords; xx++) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          JBIG2_PUTDWORD(dp, DoCompose(op, tmp1, tmp2));
          sp += 4;
          dp += 4;
        }
        if (d2 != 0) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          JBIG2_PUTDWORD(dp, DoComposeWithMask(op, tmp1, tmp2, maskR));
        }
      });
    }
    return true;
  }

  const uint32_t shift1 = d1 - s1;
  const uint32_t shift2 = 32 - shift1;
  const int32_t middleDwords = (xd1 >> 5) - ((xd0 + 31) >> 5);
  for (int32_t i = 0; i < h; ++i) {
    int src_line = src_start_line + i;
    if (src_line >= height_) {
      return false;
    }
    int dest_line = dest_start_line + i;
    CHECK_LT(dest_line, pDst->height_);
    const uint8_t* lineSrc = GetLine(src_line).subspan(src_offset).data();
    uint8_t* lineDst = pDst->GetLine(dest_line).subspan(dest_offset).data();

    UNSAFE_TODO({
      const uint8_t* sp = lineSrc;
      uint8_t* dp = lineDst;
      if (d1 != 0) {
        uint32_t tmp1 = JBIG2_GETDWORD(sp) >> shift1;
        uint32_t tmp2 = JBIG2_GETDWORD(dp);
        JBIG2_PUTDWORD(dp, DoComposeWithMask(op, tmp1, tmp2, maskL));
        dp += 4;
      }
      for (int32_t xx = 0; xx < middleDwords; xx++) {
        uint32_t tmp1 = (JBIG2_GETDWORD(sp) << shift2) |
                        ((JBIG2_GETDWORD(sp + 4)) >> shift1);
        uint32_t tmp2 = JBIG2_GETDWORD(dp);
        JBIG2_PUTDWORD(dp, DoCompose(op, tmp1, tmp2));
        sp += 4;
        dp += 4;
      }
      if (d2 != 0) {
        uint32_t tmp1 =
            (JBIG2_GETDWORD(sp) << shift2) |
            (((sp + 4) < lineSrc + lineLeft ? JBIG2_GETDWORD(sp + 4) : 0) >>
             shift1);
        uint32_t tmp2 = JBIG2_GETDWORD(dp);
        JBIG2_PUTDWORD(dp, DoComposeWithMask(op, tmp1, tmp2, maskR));
      }
    });
  }
  return true;
}
