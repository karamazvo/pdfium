// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/cfx_systemfontcache.h"

#include <array>
#include <iterator>
#include <utility>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxge/cfx_face.h"

CFX_SystemFontCache::Entry::Entry(FixedSizeDataVector<uint8_t> data)
    : font_data_(std::move(data)) {}

CFX_SystemFontCache::Entry::~Entry() = default;

void CFX_SystemFontCache::Entry::SetFace(uint32_t face_index, CFX_Face* face) {
  CHECK_LT(face_index, std::size(ttc_faces_));
  ttc_faces_[face_index].Reset(face);
}

CFX_Face* CFX_SystemFontCache::Entry::GetFace(uint32_t face_index) const {
  CHECK_LT(face_index, std::size(ttc_faces_));
  return ttc_faces_[face_index].Get();
}

CFX_SystemFontCache::CFX_SystemFontCache() = default;

CFX_SystemFontCache::~CFX_SystemFontCache() = default;

RetainPtr<CFX_SystemFontCache::Entry> CFX_SystemFontCache::GetCachedFontDesc(
    const ByteString& face_name,
    int weight,
    bool bItalic) {
  auto it = face_map_.find({face_name, weight, bItalic});
  return it != face_map_.end() ? pdfium::WrapRetain(it->second.Get()) : nullptr;
}

RetainPtr<CFX_SystemFontCache::Entry> CFX_SystemFontCache::AddCachedFontDesc(
    const ByteString& face_name,
    int weight,
    bool bItalic,
    FixedSizeDataVector<uint8_t> data) {
  auto font_desc = pdfium::MakeRetain<Entry>(std::move(data));
  face_map_[{face_name, weight, bItalic}].Reset(font_desc.Get());
  return font_desc;
}

RetainPtr<CFX_SystemFontCache::Entry> CFX_SystemFontCache::GetCachedTTCFontDesc(
    size_t ttc_size,
    uint32_t checksum) {
  auto it = ttc_face_map_.find({ttc_size, checksum});
  return it != ttc_face_map_.end() ? pdfium::WrapRetain(it->second.Get())
                                   : nullptr;
}

RetainPtr<CFX_SystemFontCache::Entry> CFX_SystemFontCache::AddCachedTTCFontDesc(
    size_t ttc_size,
    uint32_t checksum,
    FixedSizeDataVector<uint8_t> data) {
  auto pNewDesc = pdfium::MakeRetain<Entry>(std::move(data));
  ttc_face_map_[{ttc_size, checksum}].Reset(pNewDesc.Get());
  return pNewDesc;
}
