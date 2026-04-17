// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fdrm/fx_crypt_sha.h"

#include <algorithm>
#include <array>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/stl_util.h"

namespace sha384 {
constexpr uint64_t Maj(uint64_t x, uint64_t y, uint64_t z) {
  return (x & y) | (z & (x | y));
}

constexpr uint64_t Ch(uint64_t x, uint64_t y, uint64_t z) {
  return z ^ (x & (y ^ z));
}

constexpr uint64_t Shr(uint64_t x, int n) {
  return x >> n;
}

constexpr uint64_t Rotr(uint64_t x, int n) {
  return Shr(x, n) | (x << (64 - n));
}

// NIST: σ0
constexpr uint64_t LowerSigma0(uint64_t x) {
  return Rotr(x, 1) ^ Rotr(x, 8) ^ Shr(x, 7);
}

// NIST: σ1
constexpr uint64_t LowerSigma1(uint64_t x) {
  return Rotr(x, 19) ^ Rotr(x, 61) ^ Shr(x, 6);
}

// NIST: Σ0
constexpr uint64_t UpperSigma0(uint64_t x) {
  return Rotr(x, 28) ^ Rotr(x, 34) ^ Rotr(x, 39);
}

// NIST: Σ1
constexpr uint64_t UpperSigma1(uint64_t x) {
  return Rotr(x, 14) ^ Rotr(x, 18) ^ Rotr(x, 41);
}

uint64_t ExpandWord(pdfium::span<uint64_t> W, size_t t) {
  W[t] = LowerSigma1(W[t - 2]) + W[t - 7] + LowerSigma0(W[t - 15]) + W[t - 16];
  return W[t];
}

}  // namespace sha384

namespace sha256 {
constexpr uint32_t Shr(uint32_t x, int n) {
  return (x & 0xFFFFFFFF) >> n;
}

constexpr uint32_t Rotr(uint32_t x, int n) {
  return Shr(x, n) | (x << (32 - n));
}

constexpr uint32_t Rol(uint32_t x, int n) {
  return (x << n) | (x >> (32 - n));
}

constexpr uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) {
  return (x & y) | (z & (x | y));
}

constexpr uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) {
  return z ^ (x & (y ^ z));
}

// NIST: σ0
constexpr uint32_t LowerSigma0(uint32_t x) {
  return Rotr(x, 7) ^ Rotr(x, 18) ^ Shr(x, 3);
}

// NIST: σ1
constexpr uint32_t LowerSigma1(uint32_t x) {
  return Rotr(x, 17) ^ Rotr(x, 19) ^ Shr(x, 10);
}

// NIST: Σ0
constexpr uint32_t UpperSigma0(uint32_t x) {
  return Rotr(x, 2) ^ Rotr(x, 13) ^ Rotr(x, 22);
}

// NIST: Σ1
constexpr uint32_t UpperSigma1(uint32_t x) {
  return Rotr(x, 6) ^ Rotr(x, 11) ^ Rotr(x, 25);
}

uint32_t ExpandWord(pdfium::span<uint32_t> W, size_t t) {
  W[t] = LowerSigma1(W[t - 2]) + W[t - 7] + LowerSigma0(W[t - 15]) + W[t - 16];
  return W[t];
}

}  // namespace sha256

namespace {

void ShaSetUint32(uint64_t n, pdfium::span<uint8_t> buffer, size_t index) {
  for (size_t i = 0; i < 4; ++i) {
    buffer[index + i] = static_cast<uint8_t>(n >> (24 - 8 * i));
  }
}

void ShaSetUint64(uint64_t n, pdfium::span<uint8_t> buffer, size_t index) {
  for (size_t i = 0; i < 8; ++i) {
    buffer[index + i] = static_cast<uint8_t>(n >> (56 - 8 * i));
  }
}

void SHA_Core_Init(pdfium::span<uint32_t, 5> h) {
  h[0] = 0x67452301;
  h[1] = 0xefcdab89;
  h[2] = 0x98badcfe;
  h[3] = 0x10325476;
  h[4] = 0xc3d2e1f0;
}

void SHATransform(pdfium::span<uint32_t> digest, pdfium::span<uint32_t> block) {
  std::array<uint32_t, 80> w;
  int t;
  for (t = 0; t < 16; t++) {
    w[t] = block[t];
  }
  for (t = 16; t < 80; t++) {
    uint32_t tmp = w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16];
    w[t] = sha256::Rol(tmp, 1);
  }
  uint32_t a = digest[0];
  uint32_t b = digest[1];
  uint32_t c = digest[2];
  uint32_t d = digest[3];
  uint32_t e = digest[4];
  for (t = 0; t < 20; t++) {
    uint32_t tmp =
        sha256::Rol(a, 5) + ((b & c) | (d & ~b)) + e + w[t] + 0x5a827999;
    e = d;
    d = c;
    c = sha256::Rol(b, 30);
    b = a;
    a = tmp;
  }
  for (t = 20; t < 40; t++) {
    uint32_t tmp = sha256::Rol(a, 5) + (b ^ c ^ d) + e + w[t] + 0x6ed9eba1;
    e = d;
    d = c;
    c = sha256::Rol(b, 30);
    b = a;
    a = tmp;
  }
  for (t = 40; t < 60; t++) {
    uint32_t tmp = sha256::Rol(a, 5) + ((b & c) | (b & d) | (c & d)) + e +
                   w[t] + 0x8f1bbcdc;
    e = d;
    d = c;
    c = sha256::Rol(b, 30);
    b = a;
    a = tmp;
  }
  for (t = 60; t < 80; t++) {
    uint32_t tmp = sha256::Rol(a, 5) + (b ^ c ^ d) + e + w[t] + 0xca62c1d6;
    e = d;
    d = c;
    c = sha256::Rol(b, 30);
    b = a;
    a = tmp;
  }
  digest[0] += a;
  digest[1] += b;
  digest[2] += c;
  digest[3] += d;
  digest[4] += e;
}

constexpr auto constants = std::to_array<const uint64_t>({
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL,
    0xe9b5dba58189dbbcULL, 0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL, 0xd807aa98a3030242ULL,
    0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL,
    0xc19bf174cf692694ULL, 0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL, 0x2de92c6f592b0275ULL,
    0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL,
    0xbf597fc7beef0ee4ULL, 0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL, 0x27b70a8546d22ffcULL,
    0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL,
    0x92722c851482353bULL, 0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL, 0xd192e819d6ef5218ULL,
    0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL,
    0x34b0bcb5e19b48a8ULL, 0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL, 0x748f82ee5defb2fcULL,
    0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL,
    0xc67178f2e372532bULL, 0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL, 0x06f067aa72176fbaULL,
    0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL,
    0x431d67c49c100d4cULL, 0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL,
});

const uint8_t kSha256Padding[64] = {0x80};
const uint8_t kSha384Padding[128] = {0x80};

void sha256_process(CRYPT_sha2_context* ctx,
                    pdfium::span<const uint8_t, 64> data) {
  std::array<uint32_t, 64> W;
  for (size_t i = 0; i < 16; ++i) {
    uint32_t val = 0;
    for (uint8_t byte : data.subspan(4 * i, 4u)) {
      val = (val << 8) | byte;
    }
    W[i] = val;
  }
  std::array<uint32_t, 8> letters;
  for (size_t i = 0; i < 8; i++) {
    letters[i] = static_cast<uint32_t>(ctx->state[i]);
  }
  for (size_t i = 0; i < 64; ++i) {
    uint32_t word = (i < 16) ? W[i] : sha256::ExpandWord(W, i);

    // temp1 uses H, E, F, G.
    // If H is (15-i)%8, then E is (12-i)%8, F is (13-i)%8, G is (14-i)%8.
    uint32_t temp1 = letters[(15 - i) % 8] +
                     sha256::UpperSigma1(letters[(12 - i) % 8]) +
                     sha256::Ch(letters[(12 - i) % 8], letters[(13 - i) % 8],
                                letters[(14 - i) % 8]) +
                     word + static_cast<uint32_t>(constants[i] >> 32);

    // (8-i % 8) maps to the first thing passed in the cyclical register.
    // Consequently (9-i % 8) maps to the second (10-i % 8) to the third etc.
    uint32_t temp2 = sha256::UpperSigma0(letters[(8 - i) % 8]) +
                     sha256::Maj(letters[(8 - i) % 8], letters[(9 - i) % 8],
                                 letters[(10 - i) % 8]);

    letters[(11 - i) % 8] += temp1;
    letters[(15 - i) % 8] = temp1 + temp2;
  }
  for (size_t i = 0; i < 8; ++i) {
    ctx->state[i] += letters[i];
  }
}

void sha384_process(CRYPT_sha2_context* ctx,
                    pdfium::span<const uint8_t, 128> data) {
  std::array<uint64_t, 80> W;
  for (size_t i = 0; i < 16; ++i) {
    uint64_t val = 0;
    for (uint8_t byte : data.subspan(8 * i, 8u)) {
      val = (val << 8) | byte;
    }
    W[i] = val;
  }
  std::array<uint64_t, 8> letters;
  for (size_t i = 0; i < 8; ++i) {
    letters[i] = ctx->state[i];
  }
  for (size_t i = 0; i < 80; ++i) {
    uint64_t word = (i < 16) ? W[i] : sha384::ExpandWord(W, i);

    uint64_t temp1 = letters[(15 - i) % 8] +
                     sha384::UpperSigma1(letters[(12 - i) % 8]) +
                     sha384::Ch(letters[(12 - i) % 8], letters[(13 - i) % 8],
                                letters[(14 - i) % 8]) +
                     word + constants[i];

    uint64_t temp2 = sha384::UpperSigma0(letters[(8 - i) % 8]) +
                     sha384::Maj(letters[(8 - i) % 8], letters[(9 - i) % 8],
                                 letters[(10 - i) % 8]);

    letters[(11 - i) % 8] += temp1;
    letters[(15 - i) % 8] = temp1 + temp2;
  }
  for (size_t i = 0; i < 8; ++i) {
    ctx->state[i] += letters[i];
  }
}

}  // namespace

void CRYPT_SHA1Start(CRYPT_sha1_context* context) {
  SHA_Core_Init(context->h);
  context->total_bytes = 0;
  context->blkused = 0;
}

void CRYPT_SHA1Update(CRYPT_sha1_context* context,
                      pdfium::span<const uint8_t> data) {
  const auto block_span = pdfium::span(context->block);
  context->total_bytes += data.size();
  if (context->blkused && data.size() < 64 - context->blkused) {
    fxcrt::Copy(data, block_span.subspan(context->blkused));
    context->blkused += data.size();
    return;
  }
  std::array<uint32_t, 16> wordblock;
  while (data.size() >= 64 - context->blkused) {
    fxcrt::Copy(data.first(64 - context->blkused),
                block_span.subspan(context->blkused));
    data = data.subspan(64 - context->blkused);
    for (int i = 0; i < 16; i++) {
      wordblock[i] = (((uint32_t)context->block[i * 4 + 0]) << 24) |
                     (((uint32_t)context->block[i * 4 + 1]) << 16) |
                     (((uint32_t)context->block[i * 4 + 2]) << 8) |
                     (((uint32_t)context->block[i * 4 + 3]) << 0);
    }
    SHATransform(context->h, wordblock);
    context->blkused = 0;
  }
  fxcrt::Copy(data, block_span);
  context->blkused = static_cast<uint32_t>(data.size());
}

void CRYPT_SHA1Finish(CRYPT_sha1_context* context,
                      pdfium::span<uint8_t, 20> digest) {
  uint64_t total_bits = 8 * context->total_bytes;  // Prior to padding.
  std::array<uint8_t, 64> c;
  uint8_t pad;
  if (context->blkused >= 56) {
    pad = 56 + 64 - context->blkused;
  } else {
    pad = 56 - context->blkused;
  }
  std::ranges::fill(pdfium::span(c).first(pad), 0);
  c[0] = 0x80;
  CRYPT_SHA1Update(context, pdfium::span(c).first(pad));
  c[0] = (total_bits >> 56) & 0xFF;
  c[1] = (total_bits >> 48) & 0xFF;
  c[2] = (total_bits >> 40) & 0xFF;
  c[3] = (total_bits >> 32) & 0xFF;
  c[4] = (total_bits >> 24) & 0xFF;
  c[5] = (total_bits >> 16) & 0xFF;
  c[6] = (total_bits >> 8) & 0xFF;
  c[7] = (total_bits >> 0) & 0xFF;
  CRYPT_SHA1Update(context, pdfium::span(c).first<8u>());
  for (int i = 0; i < 5; i++) {
    digest[i * 4] = (context->h[i] >> 24) & 0xFF;
    digest[i * 4 + 1] = (context->h[i] >> 16) & 0xFF;
    digest[i * 4 + 2] = (context->h[i] >> 8) & 0xFF;
    digest[i * 4 + 3] = (context->h[i]) & 0xFF;
  }
}

DataVector<uint8_t> CRYPT_SHA1Generate(pdfium::span<const uint8_t> data) {
  CRYPT_sha1_context s;
  CRYPT_SHA1Start(&s);
  CRYPT_SHA1Update(&s, data);

  DataVector<uint8_t> digest(20);
  CRYPT_SHA1Finish(&s, pdfium::span<uint8_t>(digest).first<20u>());
  return digest;
}

void CRYPT_SHA256Start(CRYPT_sha2_context* context) {
  context->total_bytes = 0;
  context->state[0] = 0x6A09E667;
  context->state[1] = 0xBB67AE85;
  context->state[2] = 0x3C6EF372;
  context->state[3] = 0xA54FF53A;
  context->state[4] = 0x510E527F;
  context->state[5] = 0x9B05688C;
  context->state[6] = 0x1F83D9AB;
  context->state[7] = 0x5BE0CD19;
  std::ranges::fill(context->buffer, 0);
}

void CRYPT_SHA256Update(CRYPT_sha2_context* context,
                        pdfium::span<const uint8_t> data) {
  if (data.empty()) {
    return;
  }
  const auto buffer_span = pdfium::span(context->buffer);
  uint32_t left = context->total_bytes & 0x3F;
  uint32_t fill = 64 - left;
  context->total_bytes += data.size();
  if (left && data.size() >= fill) {
    fxcrt::Copy(data.first(fill), buffer_span.subspan(left));
    sha256_process(context, buffer_span.first<64u>());
    data = data.subspan(fill);
    left = 0;
  }
  while (data.size() >= 64) {
    sha256_process(context, data.first<64u>());
    data = data.subspan(64u);
  }
  if (!data.empty()) {
    fxcrt::Copy(data, buffer_span.subspan(left));
  }
}

void CRYPT_SHA256Finish(CRYPT_sha2_context* context,
                        pdfium::span<uint8_t, 32> digest) {
  uint8_t msglen[8];
  uint64_t total_bits = 8 * context->total_bytes;  // Prior to padding.
  ShaSetUint64(total_bits, pdfium::span(msglen), 0);
  uint32_t last = context->total_bytes & 0x3F;
  uint32_t padn = (last < 56) ? (56 - last) : (120 - last);
  CRYPT_SHA256Update(context, pdfium::span(kSha256Padding).first(padn));
  CRYPT_SHA256Update(context, msglen);
  for (size_t i = 0; i < 8; ++i) {
    ShaSetUint32(context->state[i], digest, 4 * i);
  }
}

DataVector<uint8_t> CRYPT_SHA256Generate(pdfium::span<const uint8_t> data) {
  CRYPT_sha2_context ctx;
  CRYPT_SHA256Start(&ctx);
  CRYPT_SHA256Update(&ctx, data);

  DataVector<uint8_t> digest(32);
  CRYPT_SHA256Finish(&ctx, pdfium::span<uint8_t>(digest).first<32u>());
  return digest;
}

void CRYPT_SHA384Start(CRYPT_sha2_context* context) {
  context->total_bytes = 0;
  context->state[0] = 0xcbbb9d5dc1059ed8ULL;
  context->state[1] = 0x629a292a367cd507ULL;
  context->state[2] = 0x9159015a3070dd17ULL;
  context->state[3] = 0x152fecd8f70e5939ULL;
  context->state[4] = 0x67332667ffc00b31ULL;
  context->state[5] = 0x8eb44a8768581511ULL;
  context->state[6] = 0xdb0c2e0d64f98fa7ULL;
  context->state[7] = 0x47b5481dbefa4fa4ULL;
  std::ranges::fill(context->buffer, 0);
}

void CRYPT_SHA384Update(CRYPT_sha2_context* context,
                        pdfium::span<const uint8_t> data) {
  if (data.empty()) {
    return;
  }
  const auto buffer_span = pdfium::span(context->buffer);
  uint32_t left = context->total_bytes & 0x7F;
  uint32_t fill = 128 - left;
  context->total_bytes += data.size();
  if (left && data.size() >= fill) {
    fxcrt::Copy(data.first(fill), buffer_span.subspan(left));
    sha384_process(context, buffer_span.first<128u>());
    data = data.subspan(fill);
    left = 0;
  }
  while (data.size() >= 128) {
    sha384_process(context, data.first<128u>());
    data = data.subspan<128u>();
  }
  if (!data.empty()) {
    fxcrt::Copy(data, buffer_span.subspan(left));
  }
}

void CRYPT_SHA384Finish(CRYPT_sha2_context* context,
                        pdfium::span<uint8_t, 48> digest) {
  uint8_t msglen[16];
  uint64_t total_bits = 8 * context->total_bytes;  // Prior to padding.
  ShaSetUint64(0ULL, pdfium::span(msglen), 0);
  ShaSetUint64(total_bits, pdfium::span(msglen), 8);
  uint32_t last = context->total_bytes & 0x7F;
  uint32_t padn = (last < 112) ? (112 - last) : (240 - last);
  CRYPT_SHA384Update(context, pdfium::span(kSha384Padding).first(padn));
  CRYPT_SHA384Update(context, msglen);
  for (size_t i = 0; i < 6; ++i) {
    ShaSetUint64(context->state[i], digest, 8 * i);
  }
}

DataVector<uint8_t> CRYPT_SHA384Generate(pdfium::span<const uint8_t> data) {
  CRYPT_sha2_context context;
  CRYPT_SHA384Start(&context);
  CRYPT_SHA384Update(&context, data);

  DataVector<uint8_t> digest(48);
  CRYPT_SHA384Finish(&context, pdfium::span<uint8_t>(digest).first<48u>());
  return digest;
}

void CRYPT_SHA512Start(CRYPT_sha2_context* context) {
  context->total_bytes = 0;
  context->state[0] = 0x6a09e667f3bcc908ULL;
  context->state[1] = 0xbb67ae8584caa73bULL;
  context->state[2] = 0x3c6ef372fe94f82bULL;
  context->state[3] = 0xa54ff53a5f1d36f1ULL;
  context->state[4] = 0x510e527fade682d1ULL;
  context->state[5] = 0x9b05688c2b3e6c1fULL;
  context->state[6] = 0x1f83d9abfb41bd6bULL;
  context->state[7] = 0x5be0cd19137e2179ULL;
  std::ranges::fill(context->buffer, 0);
}

void CRYPT_SHA512Update(CRYPT_sha2_context* context,
                        pdfium::span<const uint8_t> data) {
  CRYPT_SHA384Update(context, data);
}

void CRYPT_SHA512Finish(CRYPT_sha2_context* context,
                        pdfium::span<uint8_t, 64> digest) {
  uint8_t msglen[16];
  uint64_t total_bits = 8 * context->total_bytes;
  ShaSetUint64(0ULL, pdfium::span(msglen), 0);
  ShaSetUint64(total_bits, pdfium::span(msglen), 8);
  uint32_t last = context->total_bytes & 0x7F;
  uint32_t padn = (last < 112) ? (112 - last) : (240 - last);
  CRYPT_SHA512Update(context, pdfium::span(kSha384Padding).first(padn));
  CRYPT_SHA512Update(context, msglen);
  for (size_t i = 0; i < 8; ++i) {
    ShaSetUint64(context->state[i], digest, 8 * i);
  }
}

DataVector<uint8_t> CRYPT_SHA512Generate(pdfium::span<const uint8_t> data) {
  CRYPT_sha2_context context;
  CRYPT_SHA512Start(&context);
  CRYPT_SHA512Update(&context, data);

  DataVector<uint8_t> digest(64);
  CRYPT_SHA512Finish(&context, pdfium::span<uint8_t>(digest).first<64u>());
  return digest;
}
