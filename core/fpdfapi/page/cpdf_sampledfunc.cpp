// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_sampledfunc.h"

#include <algorithm>
#include <utility>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fxcrt/cfx_bitstream.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_memory_wrappers.h"
#include "core/fxcrt/fx_safe_types.h"
#include "third_party/abseil-cpp/absl/container/inlined_vector.h"

namespace {

// See PDF Reference 1.7, page 170, table 3.36.
bool IsValidBitsPerSample(uint32_t x) {
  switch (x) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 12:
    case 16:
    case 24:
    case 32:
      return true;
    default:
      return false;
  }
}

}  // namespace

CPDF_SampledFunc::CPDF_SampledFunc() : CPDF_Function(Type::kType0Sampled) {}

CPDF_SampledFunc::~CPDF_SampledFunc() = default;

bool CPDF_SampledFunc::v_Init(const CPDF_Object* pObj, VisitedSet* pVisited) {
  RetainPtr<const CPDF_Stream> pStream(pObj->AsStream());
  if (!pStream)
    return false;

  RetainPtr<const CPDF_Dictionary> pDict = pStream->GetDict();
  RetainPtr<const CPDF_Array> pSize = pDict->GetArrayFor("Size");
  if (!pSize || pSize->IsEmpty())
    return false;

  bits_per_sample_ = pDict->GetIntegerFor("BitsPerSample");
  if (!IsValidBitsPerSample(bits_per_sample_)) {
    return false;
  }

  FX_SAFE_UINT32 nTotalSampleBits = bits_per_sample_;
  nTotalSampleBits *= outputs_;
  RetainPtr<const CPDF_Array> pEncode = pDict->GetArrayFor("Encode");
  m_EncodeInfo.resize(inputs_);
  for (uint32_t i = 0; i < inputs_; i++) {
    int size = pSize->GetIntegerAt(i);
    if (size <= 0)
      return false;

    m_EncodeInfo[i].sizes = size;
    nTotalSampleBits *= m_EncodeInfo[i].sizes;
    if (pEncode) {
      m_EncodeInfo[i].encode_min = pEncode->GetFloatAt(i * 2);
      m_EncodeInfo[i].encode_max = pEncode->GetFloatAt(i * 2 + 1);
    } else {
      m_EncodeInfo[i].encode_min = 0;
      m_EncodeInfo[i].encode_max =
          m_EncodeInfo[i].sizes == 1 ? 1 : m_EncodeInfo[i].sizes - 1;
    }
  }
  FX_SAFE_UINT32 nTotalSampleBytes = (nTotalSampleBits + 7) / 8;
  if (!nTotalSampleBytes.IsValid() || nTotalSampleBytes.ValueOrDie() == 0)
    return false;

  m_SampleMax = 0xffffffff >> (32 - bits_per_sample_);
  sample_stream_ = pdfium::MakeRetain<CPDF_StreamAcc>(std::move(pStream));
  sample_stream_->LoadAllDataFiltered();
  if (nTotalSampleBytes.ValueOrDie() > sample_stream_->GetSize()) {
    return false;
  }

  RetainPtr<const CPDF_Array> pDecode = pDict->GetArrayFor("Decode");
  m_DecodeInfo.resize(outputs_);
  for (uint32_t i = 0; i < outputs_; i++) {
    if (pDecode) {
      m_DecodeInfo[i].decode_min = pDecode->GetFloatAt(2 * i);
      m_DecodeInfo[i].decode_max = pDecode->GetFloatAt(2 * i + 1);
    } else {
      m_DecodeInfo[i].decode_min = m_Ranges[i * 2];
      m_DecodeInfo[i].decode_max = m_Ranges[i * 2 + 1];
    }
  }
  return true;
}

bool CPDF_SampledFunc::v_Call(pdfium::span<const float> inputs,
                              pdfium::span<float> results) const {
  int pos = 0;
  absl::InlinedVector<float, 16, FxAllocAllocator<float>> encoded_input_buf(
      inputs_);
  absl::InlinedVector<uint32_t, 32, FxAllocAllocator<uint32_t>> int_buf(
      inputs_ * 2);
  UNSAFE_TODO({
    float* encoded_input = encoded_input_buf.data();
    uint32_t* index = int_buf.data();
    uint32_t* blocksize = index + inputs_;
    for (uint32_t i = 0; i < inputs_; i++) {
      if (i == 0) {
        blocksize[i] = 1;
      } else {
        blocksize[i] = blocksize[i - 1] * m_EncodeInfo[i - 1].sizes;
      }
      encoded_input[i] =
          Interpolate(inputs[i], m_Domains[i * 2], m_Domains[i * 2 + 1],
                      m_EncodeInfo[i].encode_min, m_EncodeInfo[i].encode_max);
      index[i] = std::clamp(static_cast<uint32_t>(encoded_input[i]), 0U,
                            m_EncodeInfo[i].sizes - 1);
      pos += index[i] * blocksize[i];
    }
    FX_SAFE_INT32 bits_to_output = outputs_;
    bits_to_output *= bits_per_sample_;
    if (!bits_to_output.IsValid()) {
      return false;
    }

    int bits_to_skip;
    {
      FX_SAFE_INT32 bitpos = pos;
      bitpos *= bits_to_output.ValueOrDie();
      bits_to_skip = bitpos.ValueOrDefault(-1);
      if (bits_to_skip < 0) {
        return false;
      }

      FX_SAFE_INT32 range_check = bitpos;
      range_check += bits_to_output.ValueOrDie();
      if (!range_check.IsValid()) {
        return false;
      }
    }

    pdfium::span<const uint8_t> pSampleData = sample_stream_->GetSpan();
    if (pSampleData.empty()) {
      return false;
    }

    CFX_BitStream bitstream(pSampleData);
    bitstream.SkipBits(bits_to_skip);
    for (uint32_t i = 0; i < outputs_; ++i) {
      uint32_t sample = bitstream.GetBits(bits_per_sample_);
      float encoded = sample;
      for (uint32_t j = 0; j < inputs_; ++j) {
        if (index[j] == m_EncodeInfo[j].sizes - 1) {
          if (index[j] == 0) {
            encoded = encoded_input[j] * sample;
          }
        } else {
          FX_SAFE_INT32 bitpos2 = blocksize[j];
          bitpos2 += pos;
          bitpos2 *= outputs_;
          bitpos2 += i;
          bitpos2 *= bits_per_sample_;
          int bits_to_skip2 = bitpos2.ValueOrDefault(-1);
          if (bits_to_skip2 < 0) {
            return false;
          }

          CFX_BitStream bitstream2(pSampleData);
          bitstream2.SkipBits(bits_to_skip2);
          float sample2 =
              static_cast<float>(bitstream2.GetBits(bits_per_sample_));
          encoded += (encoded_input[j] - index[j]) * (sample2 - sample);
        }
      }
      results[i] =
          Interpolate(encoded, 0, m_SampleMax, m_DecodeInfo[i].decode_min,
                      m_DecodeInfo[i].decode_max);
    }
  });
  return true;
}

#if defined(PDF_USE_SKIA)
RetainPtr<CPDF_StreamAcc> CPDF_SampledFunc::GetSampleStream() const {
  return sample_stream_;
}
#endif
