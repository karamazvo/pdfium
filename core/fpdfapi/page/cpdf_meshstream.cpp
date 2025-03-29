// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_meshstream.h"

#include <utility>

#include "core/fpdfapi/page/cpdf_colorspace.h"
#include "core/fpdfapi/page/cpdf_function.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fxcrt/cfx_bitstream.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"

namespace {

// See PDF Reference 1.7, page 315, table 4.32. (Also table 4.33 and 4.34)
bool ShouldCheckBPC(ShadingType type) {
  switch (type) {
    case kFreeFormGouraudTriangleMeshShading:
    case kLatticeFormGouraudTriangleMeshShading:
    case kCoonsPatchMeshShading:
    case kTensorProductPatchMeshShading:
      return true;
    default:
      return false;
  }
}

// Same references as ShouldCheckBPC() above.
bool IsValidBitsPerComponent(uint32_t x) {
  switch (x) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 12:
    case 16:
      return true;
    default:
      return false;
  }
}

// Same references as ShouldCheckBPC() above.
bool IsValidBitsPerCoordinate(uint32_t x) {
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

// See PDF Reference 1.7, page 315, table 4.32. (Also table 4.34)
bool ShouldCheckBitsPerFlag(ShadingType type) {
  switch (type) {
    case kFreeFormGouraudTriangleMeshShading:
    case kCoonsPatchMeshShading:
    case kTensorProductPatchMeshShading:
      return true;
    default:
      return false;
  }
}

// Same references as ShouldCheckBitsPerFlag() above.
bool IsValidBitsPerFlag(uint32_t x) {
  switch (x) {
    case 2:
    case 4:
    case 8:
      return true;
    default:
      return false;
  }
}

}  // namespace

CPDF_MeshVertex::CPDF_MeshVertex() = default;

CPDF_MeshVertex::CPDF_MeshVertex(const CPDF_MeshVertex&) = default;

CPDF_MeshVertex::~CPDF_MeshVertex() = default;

CPDF_MeshStream::CPDF_MeshStream(
    ShadingType type,
    const std::vector<std::unique_ptr<CPDF_Function>>& funcs,
    RetainPtr<const CPDF_Stream> pShadingStream,
    RetainPtr<CPDF_ColorSpace> pCS)
    : m_type(type),
      m_funcs(funcs),
      shading_stream_(std::move(pShadingStream)),
      m_pCS(std::move(pCS)),
      stream_(pdfium::MakeRetain<CPDF_StreamAcc>(shading_stream_)) {}

CPDF_MeshStream::~CPDF_MeshStream() = default;

bool CPDF_MeshStream::Load() {
  stream_->LoadAllDataFiltered();
  m_BitStream = std::make_unique<CFX_BitStream>(stream_->GetSpan());

  RetainPtr<const CPDF_Dictionary> pDict = shading_stream_->GetDict();
  coord_bits_ = pDict->GetIntegerFor("BitsPerCoordinate");
  component_bits_ = pDict->GetIntegerFor("BitsPerComponent");
  if (ShouldCheckBPC(m_type)) {
    if (!IsValidBitsPerCoordinate(coord_bits_)) {
      return false;
    }
    if (!IsValidBitsPerComponent(component_bits_)) {
      return false;
    }
  }

  flag_bits_ = pDict->GetIntegerFor("BitsPerFlag");
  if (ShouldCheckBitsPerFlag(m_type) && !IsValidBitsPerFlag(flag_bits_)) {
    return false;
  }

  uint32_t nComponents = m_pCS->ComponentCount();
  if (nComponents > kMaxComponents)
    return false;

  components_ = m_funcs.empty() ? nComponents : 1;
  RetainPtr<const CPDF_Array> pDecode = pDict->GetArrayFor("Decode");
  if (!pDecode || pDecode->size() != 4 + components_ * 2) {
    return false;
  }

  m_xmin = pDecode->GetFloatAt(0);
  m_xmax = pDecode->GetFloatAt(1);
  m_ymin = pDecode->GetFloatAt(2);
  m_ymax = pDecode->GetFloatAt(3);
  for (uint32_t i = 0; i < components_; ++i) {
    m_ColorMin[i] = pDecode->GetFloatAt(i * 2 + 4);
    m_ColorMax[i] = pDecode->GetFloatAt(i * 2 + 5);
  }
  if (ShouldCheckBPC(m_type)) {
    m_CoordMax = coord_bits_ == 32 ? -1 : (1 << coord_bits_) - 1;
    m_ComponentMax = (1 << component_bits_) - 1;
  }
  return true;
}

void CPDF_MeshStream::SkipBits(uint32_t nbits) {
  m_BitStream->SkipBits(nbits);
}

void CPDF_MeshStream::ByteAlign() {
  m_BitStream->ByteAlign();
}

bool CPDF_MeshStream::IsEOF() const {
  return m_BitStream->IsEOF();
}

bool CPDF_MeshStream::CanReadFlag() const {
  return m_BitStream->BitsRemaining() >= flag_bits_;
}

bool CPDF_MeshStream::CanReadCoords() const {
  return m_BitStream->BitsRemaining() / 2 >= coord_bits_;
}

bool CPDF_MeshStream::CanReadColor() const {
  return m_BitStream->BitsRemaining() / component_bits_ >= components_;
}

uint32_t CPDF_MeshStream::ReadFlag() const {
  DCHECK(ShouldCheckBitsPerFlag(m_type));
  return m_BitStream->GetBits(flag_bits_) & 0x03;
}

CFX_PointF CPDF_MeshStream::ReadCoords() const {
  DCHECK(ShouldCheckBPC(m_type));

  CFX_PointF pos;
  if (coord_bits_ == 32) {
    pos.x = m_xmin + m_BitStream->GetBits(coord_bits_) * (m_xmax - m_xmin) /
                         static_cast<double>(m_CoordMax);
    pos.y = m_ymin + m_BitStream->GetBits(coord_bits_) * (m_ymax - m_ymin) /
                         static_cast<double>(m_CoordMax);
  } else {
    pos.x = m_xmin +
            m_BitStream->GetBits(coord_bits_) * (m_xmax - m_xmin) / m_CoordMax;
    pos.y = m_ymin +
            m_BitStream->GetBits(coord_bits_) * (m_ymax - m_ymin) / m_CoordMax;
  }
  return pos;
}

FX_RGB_STRUCT<float> CPDF_MeshStream::ReadColor() const {
  DCHECK(ShouldCheckBPC(m_type));

  std::array<float, kMaxComponents> color_value;
  for (uint32_t i = 0; i < components_; ++i) {
    color_value[i] = m_ColorMin[i] + m_BitStream->GetBits(component_bits_) *
                                         (m_ColorMax[i] - m_ColorMin[i]) /
                                         m_ComponentMax;
  }
  if (m_funcs.empty()) {
    return m_pCS->GetRGBOrZerosOnError(color_value);
  }
  float result[kMaxComponents] = {};
  for (const auto& func : m_funcs) {
    if (func && func->OutputCount() <= kMaxComponents) {
      func->Call(pdfium::make_span(color_value).first<1u>(), result);
    }
  }
  return m_pCS->GetRGBOrZerosOnError(result);
}

bool CPDF_MeshStream::ReadVertex(const CFX_Matrix& pObject2Bitmap,
                                 CPDF_MeshVertex* vertex,
                                 uint32_t* flag) {
  if (!CanReadFlag())
    return false;
  *flag = ReadFlag();

  if (!CanReadCoords())
    return false;
  vertex->position = pObject2Bitmap.Transform(ReadCoords());

  if (!CanReadColor())
    return false;
  vertex->rgb = ReadColor();
  m_BitStream->ByteAlign();
  return true;
}

std::vector<CPDF_MeshVertex> CPDF_MeshStream::ReadVertexRow(
    const CFX_Matrix& pObject2Bitmap,
    int count) {
  std::vector<CPDF_MeshVertex> vertices;
  for (int i = 0; i < count; ++i) {
    if (m_BitStream->IsEOF() || !CanReadCoords())
      return std::vector<CPDF_MeshVertex>();

    vertices.emplace_back();
    CPDF_MeshVertex& vertex = vertices.back();
    vertex.position = pObject2Bitmap.Transform(ReadCoords());
    if (!CanReadColor())
      return std::vector<CPDF_MeshVertex>();

    vertex.rgb = ReadColor();
    m_BitStream->ByteAlign();
  }
  return vertices;
}
