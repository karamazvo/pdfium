// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fdrm/fx_crypt.h"

#include <utility>

#include "core/fxcrt/byteorder.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/stl_util.h"

namespace {

const uint8_t md5_padding[64] = {0x80};

// RFC 1321 auxiliary function F (encompasses F, G, H, I by round).
uint32_t F(uint32_t x, uint32_t y, uint32_t z, size_t i) {
  return i == 0   ? (z ^ (x & (y ^ z)))
         : i == 1 ? (y ^ (z & (x ^ y)))
         : i == 2 ? (x ^ y ^ z)
                  : (y ^ (x | ~z));
}

// RFC 1321 section 3.4: k index per round.
size_t GetK(size_t i) {
  return i < 16   ? i
         : i < 32 ? (1 + 5 * i) % 16
         : i < 48 ? (5 + 3 * i) % 16
                  : (7 * i) % 16;
}

// RFC 1321: ROTATE_LEFT.
uint32_t RotateLeft(uint32_t x, size_t n) {
  return ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)));
}

void md5_process(CRYPT_md5_context* ctx, pdfium::span<const uint8_t, 64> data) {
  std::array<uint32_t, 16> data_32;
  for (size_t i = 0; i < 16; ++i) {
    data_32[i] =
        fxcrt::GetUInt32LSBFirst(data.subspan(4 * i).template first<4>());
  }

  std::array<uint32_t, 4> state;
  fxcrt::Copy(ctx->state, state);
  static constexpr std::array<uint32_t, 64> kMd5Constants = {
      0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
      0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
      0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
      0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
      0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
      0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
      0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
      0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
      0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
      0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
      0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
  };
  static constexpr std::array<std::array<uint32_t, 4>, 4> kShifts = {{
      {7, 12, 17, 22},
      {5, 9, 14, 20},
      {4, 11, 16, 23},
      {6, 10, 15, 21},
  }};
  for (size_t i = 0; i < 64; ++i) {
    // ((64-i) % 4) maps to the first thing passed in the cyclical register.
    // Consequently ((65-i) % 4) maps to the second and so on.
    state[(64 - i) % 4] += F(state[(65 - i) % 4], state[(66 - i) % 4],
                             state[(67 - i) % 4], i / 16) +
                           data_32[GetK(i)] + kMd5Constants[i];
    state[(64 - i) % 4] =
        RotateLeft(state[(64 - i) % 4], kShifts[i / 16][i % 4]) +
        state[(65 - i) % 4];
  }
  for (size_t i = 0; i < 4; ++i) {
    ctx->state[i] += state[i];
  }
}

}  // namespace

void CRYPT_ArcFourSetup(CRYPT_rc4_context* context,
                        pdfium::span<const uint8_t> key) {
  context->x = 0;
  context->y = 0;
  for (int i = 0; i < CRYPT_rc4_context::kPermutationLength; ++i) {
    context->m[i] = i;
  }

  int j = 0;
  for (int i = 0; i < CRYPT_rc4_context::kPermutationLength; ++i) {
    size_t size = key.size();
    j = (j + context->m[i] + (size ? key[i % size] : 0)) & 0xFF;
    std::swap(context->m[i], context->m[j]);
  }
}

void CRYPT_ArcFourCrypt(CRYPT_rc4_context* context,
                        pdfium::span<uint8_t> data) {
  for (auto& datum : data) {
    context->x = (context->x + 1) & 0xFF;
    context->y = (context->y + context->m[context->x]) & 0xFF;
    std::swap(context->m[context->x], context->m[context->y]);
    datum ^=
        context->m[(context->m[context->x] + context->m[context->y]) & 0xFF];
  }
}

void CRYPT_ArcFourCryptBlock(pdfium::span<uint8_t> data,
                             pdfium::span<const uint8_t> key) {
  CRYPT_rc4_context s;
  CRYPT_ArcFourSetup(&s, key);
  CRYPT_ArcFourCrypt(&s, data);
}

CRYPT_md5_context CRYPT_MD5Start() {
  CRYPT_md5_context context;
  context.total[0] = 0;
  context.total[1] = 0;
  context.state[0] = 0x67452301;
  context.state[1] = 0xEFCDAB89;
  context.state[2] = 0x98BADCFE;
  context.state[3] = 0x10325476;
  return context;
}

void CRYPT_MD5Update(CRYPT_md5_context* context,
                     pdfium::span<const uint8_t> data) {
  if (data.empty()) {
    return;
  }

  uint32_t left = (context->total[0] >> 3) & 0x3F;
  uint32_t fill = 64 - left;
  context->total[0] += data.size() << 3;
  context->total[1] += data.size() >> 29;
  context->total[0] &= 0xFFFFFFFF;
  context->total[1] += context->total[0] < data.size() << 3;

  const pdfium::span<uint8_t> buffer_span = pdfium::span(context->buffer);
  if (left && data.size() >= fill) {
    fxcrt::Copy(data.first(fill), buffer_span.subspan(left));
    md5_process(context, context->buffer);
    data = data.subspan(fill);
    left = 0;
  }
  while (data.size() >= 64) {
    md5_process(context, data.first<64u>());
    data = data.subspan<64u>();
  }
  if (!data.empty()) {
    fxcrt::Copy(data, buffer_span.subspan(left));
  }
}

void CRYPT_MD5Finish(CRYPT_md5_context* context,
                     pdfium::span<uint8_t, 16> digest) {
  uint8_t msglen[8];
  auto msglen_span = pdfium::span(msglen);
  fxcrt::PutUInt32LSBFirst(context->total[0], msglen_span.subspan<0, 4>());
  fxcrt::PutUInt32LSBFirst(context->total[1], msglen_span.subspan<4, 4>());
  uint32_t last = (context->total[0] >> 3) & 0x3F;
  uint32_t padn = (last < 56) ? (56 - last) : (120 - last);
  CRYPT_MD5Update(context, pdfium::span(md5_padding).first(padn));
  CRYPT_MD5Update(context, msglen);
  for (size_t i = 0; i < 4; ++i) {
    fxcrt::PutUInt32LSBFirst(context->state[i],
                             digest.subspan(4 * i).template first<4>());
  }
}

void CRYPT_MD5Generate(pdfium::span<const uint8_t> data,
                       pdfium::span<uint8_t, 16> digest) {
  CRYPT_md5_context ctx = CRYPT_MD5Start();
  CRYPT_MD5Update(&ctx, data);
  CRYPT_MD5Finish(&ctx, digest);
}
