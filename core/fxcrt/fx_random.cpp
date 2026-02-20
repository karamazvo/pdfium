// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/fx_random.h"

#include <stdint.h>

#include <array>

#include "build/build_config.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"

#if BUILDFLAG(IS_WIN)
#include <wincrypt.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

namespace {

constexpr uint32_t kTwistOffset = 456;
constexpr uint32_t kUpperMask = 0x80000000;
constexpr uint32_t kLowerMask = 0x7fffffff;

bool g_global_seed_initialized = false;
uint32_t g_global_seed = 0;

#if BUILDFLAG(IS_WIN)
bool GenerateSeedFromCryptoRandom(uint32_t* seed) {
  HCRYPTPROV hCP = 0;
  if (!::CryptAcquireContext(&hCP, nullptr, nullptr, PROV_RSA_FULL, 0) ||
      !hCP) {
    return false;
  }
  ::CryptGenRandom(hCP, sizeof(uint32_t), reinterpret_cast<uint8_t*>(seed));
  ::CryptReleaseContext(hCP, 0);
  return true;
}
#endif

uint32_t GenerateSeedFromEnvironment() {
  char stack_marker;
  uintptr_t stack_address = reinterpret_cast<uintptr_t>(&stack_marker);
  uint32_t seed = ~static_cast<uint32_t>(stack_address >> 3);
#if BUILDFLAG(IS_WIN)
  SYSTEMTIME system_time;
  GetSystemTime(&system_time);
  seed ^= static_cast<uint32_t>(system_time.wSecond) * 1000000;
  seed ^= static_cast<uint32_t>(system_time.wMilliseconds) * 1000;
  seed ^= GetCurrentProcessId();
#else
  struct timeval current_time;
  gettimeofday(&current_time, nullptr);
  seed ^= static_cast<uint32_t>(current_time.tv_sec) * 1000000;
  seed ^= static_cast<uint32_t>(current_time.tv_usec);
  seed ^= static_cast<uint32_t>(getpid());
#endif
  return seed;
}

uint32_t GetNextGlobalSeed() {
  if (!g_global_seed_initialized) {
#if BUILDFLAG(IS_WIN)
    if (!GenerateSeedFromCryptoRandom(&g_global_seed)) {
      g_global_seed = GenerateSeedFromEnvironment();
    }
#else
    g_global_seed = GenerateSeedFromEnvironment();
#endif
    g_global_seed_initialized = true;
  }
  return ++g_global_seed;
}

}  // namespace

FX_Random::FX_Random(uint32_t seed) {
  state_[0] = seed;
  for (uint32_t i = 1; i < kStateSize; ++i) {
    const uint32_t prev = state_[i - 1];
    state_[i] = (1812433253UL * (prev ^ (prev >> 30)) + i);
  }
  next_index_ = kStateSize;
}

FX_Random::~FX_Random() = default;

// static
void FX_Random::Fill(pdfium::span<uint32_t> buffer) {
  FX_Random next_generator(GetNextGlobalSeed());
  for (uint32_t& val : buffer) {
    val = next_generator.Generate();
  }
}

uint32_t FX_Random::Generate() {
  uint32_t bits;
  if (next_index_ >= kStateSize) {
    static constexpr std::array<uint32_t, 2> kMatrixATable = {{0, 0x9908b0df}};
    uint32_t i;
    for (i = 0; i < kStateSize - kTwistOffset; ++i) {
      bits = (state_[i] & kUpperMask) | (state_[i + 1] & kLowerMask);
      state_[i] =
          state_[i + kTwistOffset] ^ (bits >> 1) ^ kMatrixATable[bits & 1];
    }
    for (; i < kStateSize - 1; ++i) {
      bits = (state_[i] & kUpperMask) | (state_[i + 1] & kLowerMask);
      // `kTwistOffset` - `kStateSize` underflows, but this is safe because
      // unsigned underflow is well-defined.
      state_[i] = state_[i + (kTwistOffset - kStateSize)] ^ (bits >> 1) ^
                  kMatrixATable[bits & 1];
    }
    bits = (state_[kStateSize - 1] & kUpperMask) | (state_[0] & kLowerMask);
    state_[kStateSize - 1] =
        state_[kTwistOffset - 1] ^ (bits >> 1) ^ kMatrixATable[bits & 1];
    next_index_ = 0;
  }
  bits = state_[next_index_++];
  bits ^= (bits >> 11);
  bits ^= (bits << 7) & 0x9d2c5680UL;
  bits ^= (bits << 15) & 0xefc60000UL;
  bits ^= (bits >> 18);
  return bits;
}
