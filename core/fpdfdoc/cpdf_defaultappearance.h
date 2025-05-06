// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFDOC_CPDF_DEFAULTAPPEARANCE_H_
#define CORE_FPDFDOC_CPDF_DEFAULTAPPEARANCE_H_

#include <optional>

#include "core/fxcrt/bytestring.h"
#include "core/fxge/cfx_color.h"

class CPDF_SimpleParser;

class CPDF_DefaultAppearance {
 public:
  struct FontNameAndSize {
    FontNameAndSize();
    FontNameAndSize(const ByteString& name, float size);
    FontNameAndSize(FontNameAndSize&&) noexcept;
    FontNameAndSize& operator=(FontNameAndSize&&) noexcept;
    ~FontNameAndSize();

    std::optional<ByteString> name;
    float size = 0;  // Set to 0 if not found.
  };

  explicit CPDF_DefaultAppearance(const ByteString& csDA);
  CPDF_DefaultAppearance(const CPDF_DefaultAppearance&) = delete;
  CPDF_DefaultAppearance& operator=(const CPDF_DefaultAppearance&) = delete;
  ~CPDF_DefaultAppearance();

  FontNameAndSize GetFont() const;

  std::optional<CFX_Color> GetColor() const;
  std::optional<CFX_Color::TypeAndARGB> GetColorARGB() const;

  static bool FindTagParamFromStartForTesting(CPDF_SimpleParser* parser,
                                              ByteStringView token,
                                              int nParams);

 private:
  const ByteString da_;
};

#endif  // CORE_FPDFDOC_CPDF_DEFAULTAPPEARANCE_H_
