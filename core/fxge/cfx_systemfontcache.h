// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_SYSTEMFONTCACHE_H_
#define CORE_FXGE_CFX_SYSTEMFONTCACHE_H_

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <map>
#include <tuple>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxcrt/observed_ptr.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"

class CFX_Face;

class CFX_SystemFontCache {
 public:
  class Entry final : public Retainable, public Observable {
   public:
    CONSTRUCT_VIA_MAKE_RETAIN;

    pdfium::span<const uint8_t> FontData() const { return font_data_; }
    void SetFace(uint32_t face_index, CFX_Face* face);
    CFX_Face* GetFace(uint32_t face_index) const;

   private:
    explicit Entry(FixedSizeDataVector<uint8_t> data);
    ~Entry() override;

    const FixedSizeDataVector<uint8_t> font_data_;
    std::array<ObservedPtr<CFX_Face>, 16> ttc_faces_;
  };

  CFX_SystemFontCache();
  ~CFX_SystemFontCache();

  RetainPtr<Entry> GetCachedFontDesc(const ByteString& face_name,
                                     int weight,
                                     bool bItalic);
  RetainPtr<Entry> AddCachedFontDesc(const ByteString& face_name,
                                     int weight,
                                     bool bItalic,
                                     FixedSizeDataVector<uint8_t> data);

  RetainPtr<Entry> GetCachedTTCFontDesc(size_t ttc_size, uint32_t checksum);
  RetainPtr<Entry> AddCachedTTCFontDesc(size_t ttc_size,
                                        uint32_t checksum,
                                        FixedSizeDataVector<uint8_t> data);

 protected:
  std::map<std::tuple<ByteString, int, bool>, ObservedPtr<Entry>> face_map_;
  std::map<std::tuple<size_t, uint32_t>, ObservedPtr<Entry>> ttc_face_map_;
};

#endif  // CORE_FXGE_CFX_SYSTEMFONTCACHE_H_
