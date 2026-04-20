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

const uint8_t md5_padding[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void Md5Process(CryptMd5Context* ctx, pdfium::span<const uint8_t, 64> data) {
  std::array<uint32_t, 16> data_32;
  for (size_t i = 0; i < 16; ++i) {
    auto sub = data.subspan(4 * i);
    data_32[i] = fxcrt::GetUInt32LSBFirst(sub.first<4>());
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
  uint32_t A = state[0];
  uint32_t B = state[1];
  uint32_t C = state[2];
  uint32_t D = state[3];

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))
#define P(a, b, c, d, k, s, t)  \
  {                             \
    a += F(b, c, d) + data_32[k] + t; \
    a = S(a, s) + b;            \
  }

#define F(x, y, z) (z ^ (x & (y ^ z)))
  P(A, B, C, D, 0, 7, kMd5Constants[0]);
  P(D, A, B, C, 1, 12, kMd5Constants[1]);
  P(C, D, A, B, 2, 17, kMd5Constants[2]);
  P(B, C, D, A, 3, 22, kMd5Constants[3]);
  P(A, B, C, D, 4, 7, kMd5Constants[4]);
  P(D, A, B, C, 5, 12, kMd5Constants[5]);
  P(C, D, A, B, 6, 17, kMd5Constants[6]);
  P(B, C, D, A, 7, 22, kMd5Constants[7]);
  P(A, B, C, D, 8, 7, kMd5Constants[8]);
  P(D, A, B, C, 9, 12, kMd5Constants[9]);
  P(C, D, A, B, 10, 17, kMd5Constants[10]);
  P(B, C, D, A, 11, 22, kMd5Constants[11]);
  P(A, B, C, D, 12, 7, kMd5Constants[12]);
  P(D, A, B, C, 13, 12, kMd5Constants[13]);
  P(C, D, A, B, 14, 17, kMd5Constants[14]);
  P(B, C, D, A, 15, 22, kMd5Constants[15]);
#undef F

#define F(x, y, z) (y ^ (z & (x ^ y)))
  P(A, B, C, D, 1, 5, kMd5Constants[16]);
  P(D, A, B, C, 6, 9, kMd5Constants[17]);
  P(C, D, A, B, 11, 14, kMd5Constants[18]);
  P(B, C, D, A, 0, 20, kMd5Constants[19]);
  P(A, B, C, D, 5, 5, kMd5Constants[20]);
  P(D, A, B, C, 10, 9, kMd5Constants[21]);
  P(C, D, A, B, 15, 14, kMd5Constants[22]);
  P(B, C, D, A, 4, 20, kMd5Constants[23]);
  P(A, B, C, D, 9, 5, kMd5Constants[24]);
  P(D, A, B, C, 14, 9, kMd5Constants[25]);
  P(C, D, A, B, 3, 14, kMd5Constants[26]);
  P(B, C, D, A, 8, 20, kMd5Constants[27]);
  P(A, B, C, D, 13, 5, kMd5Constants[28]);
  P(D, A, B, C, 2, 9, kMd5Constants[29]);
  P(C, D, A, B, 7, 14, kMd5Constants[30]);
  P(B, C, D, A, 12, 20, kMd5Constants[31]);
#undef F

#define F(x, y, z) (x ^ y ^ z)
  P(A, B, C, D, 5, 4, kMd5Constants[32]);
  P(D, A, B, C, 8, 11, kMd5Constants[33]);
  P(C, D, A, B, 11, 16, kMd5Constants[34]);
  P(B, C, D, A, 14, 23, kMd5Constants[35]);
  P(A, B, C, D, 1, 4, kMd5Constants[36]);
  P(D, A, B, C, 4, 11, kMd5Constants[37]);
  P(C, D, A, B, 7, 16, kMd5Constants[38]);
  P(B, C, D, A, 10, 23, kMd5Constants[39]);
  P(A, B, C, D, 13, 4, kMd5Constants[40]);
  P(D, A, B, C, 0, 11, kMd5Constants[41]);
  P(C, D, A, B, 3, 16, kMd5Constants[42]);
  P(B, C, D, A, 6, 23, kMd5Constants[43]);
  P(A, B, C, D, 9, 4, kMd5Constants[44]);
  P(D, A, B, C, 12, 11, kMd5Constants[45]);
  P(C, D, A, B, 15, 16, kMd5Constants[46]);
  P(B, C, D, A, 2, 23, kMd5Constants[47]);
#undef F

#define F(x, y, z) (y ^ (x | ~z))
  P(A, B, C, D, 0, 6, kMd5Constants[48]);
  P(D, A, B, C, 7, 10, kMd5Constants[49]);
  P(C, D, A, B, 14, 15, kMd5Constants[50]);
  P(B, C, D, A, 5, 21, kMd5Constants[51]);
  P(A, B, C, D, 12, 6, kMd5Constants[52]);
  P(D, A, B, C, 3, 10, kMd5Constants[53]);
  P(C, D, A, B, 10, 15, kMd5Constants[54]);
  P(B, C, D, A, 1, 21, kMd5Constants[55]);
  P(A, B, C, D, 8, 6, kMd5Constants[56]);
  P(D, A, B, C, 15, 10, kMd5Constants[57]);
  P(C, D, A, B, 6, 15, kMd5Constants[58]);
  P(B, C, D, A, 13, 21, kMd5Constants[59]);
  P(A, B, C, D, 4, 6, kMd5Constants[60]);
  P(D, A, B, C, 11, 10, kMd5Constants[61]);
  P(C, D, A, B, 2, 15, kMd5Constants[62]);
  P(B, C, D, A, 9, 21, kMd5Constants[63]);
#undef F

  ctx->state[0] += A;
  ctx->state[1] += B;
  ctx->state[2] += C;
  ctx->state[3] += D;
}

}  // namespace

void CryptArcFourSetup(CryptRc4Context* context,
                       pdfium::span<const uint8_t> key) {
  context->x = 0;
  context->y = 0;
  for (int i = 0; i < CryptRc4Context::kPermutationLength; ++i) {
    context->m[i] = i;
  }

  int j = 0;
  for (int i = 0; i < CryptRc4Context::kPermutationLength; ++i) {
    size_t size = key.size();
    j = (j + context->m[i] + (size ? key[i % size] : 0)) & 0xFF;
    std::swap(context->m[i], context->m[j]);
  }
}

void CryptArcFourCrypt(CryptRc4Context* context, pdfium::span<uint8_t> data) {
  for (auto& datum : data) {
    context->x = (context->x + 1) & 0xFF;
    context->y = (context->y + context->m[context->x]) & 0xFF;
    std::swap(context->m[context->x], context->m[context->y]);
    datum ^=
        context->m[(context->m[context->x] + context->m[context->y]) & 0xFF];
  }
}

void CryptArcFourCryptBlock(pdfium::span<uint8_t> data,
                            pdfium::span<const uint8_t> key) {
  CryptRc4Context s;
  CryptArcFourSetup(&s, key);
  CryptArcFourCrypt(&s, data);
}

CryptMd5Context CryptMd5Start() {
  CryptMd5Context context;
  context.total[0] = 0;
  context.total[1] = 0;
  context.state[0] = 0x67452301;
  context.state[1] = 0xEFCDAB89;
  context.state[2] = 0x98BADCFE;
  context.state[3] = 0x10325476;
  return context;
}

void CryptMd5Update(CryptMd5Context* context,
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
    Md5Process(context, context->buffer);
    data = data.subspan(fill);
    left = 0;
  }
  while (data.size() >= 64) {
    Md5Process(context, data.first<64u>());
    data = data.subspan<64u>();
  }
  if (!data.empty()) {
    fxcrt::Copy(data, buffer_span.subspan(left));
  }
}

void CryptMd5Finish(CryptMd5Context* context,
                    pdfium::span<uint8_t, 16> digest) {
  uint8_t msglen[8];
  auto msglen_span = pdfium::span(msglen);
  fxcrt::PutUInt32LSBFirst(context->total[0], msglen_span.subspan<0, 4>());
  fxcrt::PutUInt32LSBFirst(context->total[1], msglen_span.subspan<4, 4>());
  uint32_t last = (context->total[0] >> 3) & 0x3F;
  uint32_t padn = (last < 56) ? (56 - last) : (120 - last);
  CryptMd5Update(context, pdfium::span(md5_padding).first(padn));
  CryptMd5Update(context, msglen);
  for (size_t i = 0; i < 4; ++i) {
    fxcrt::PutUInt32LSBFirst(context->state[i],
                             digest.subspan(4 * i).template first<4>());
  }
}

void CryptMd5Generate(pdfium::span<const uint8_t> data,
                      pdfium::span<uint8_t, 16> digest) {
  CryptMd5Context ctx = CryptMd5Start();
  CryptMd5Update(&ctx, data);
  CryptMd5Finish(&ctx, digest);
}
