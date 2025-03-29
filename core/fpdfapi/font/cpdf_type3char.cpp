// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/font/cpdf_type3char.h"

#include <utility>

#include "core/fxcrt/fx_system.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"

namespace {

constexpr float kTextUnitInGlyphUnit = 1000.0f;

}  // namespace

CPDF_Type3Char::CPDF_Type3Char() = default;

CPDF_Type3Char::~CPDF_Type3Char() = default;

// static
float CPDF_Type3Char::TextUnitToGlyphUnit(float fTextUnit) {
  return fTextUnit * kTextUnitInGlyphUnit;
}

// static
void CPDF_Type3Char::TextUnitRectToGlyphUnitRect(CFX_FloatRect* pRect) {
  pRect->Scale(kTextUnitInGlyphUnit);
}

bool CPDF_Type3Char::LoadBitmapFromSoleImageOfForm() {
  if (bitmap_ || !form_) {
    return true;
  }

  if (colored_) {
    return false;
  }

  auto result = form_->GetBitmapAndMatrixFromSoleImageOfForm();
  if (!result.has_value())
    return false;

  std::tie(bitmap_, m_ImageMatrix) = result.value();
  form_.reset();
  return true;
}

void CPDF_Type3Char::InitializeFromStreamData(bool bColored,
                                              pdfium::span<const float> pData) {
  colored_ = bColored;
  m_Width = FXSYS_roundf(TextUnitToGlyphUnit(pData[0]));
  m_BBox.left = FXSYS_roundf(TextUnitToGlyphUnit(pData[2]));
  m_BBox.bottom = FXSYS_roundf(TextUnitToGlyphUnit(pData[3]));
  m_BBox.right = FXSYS_roundf(TextUnitToGlyphUnit(pData[4]));
  m_BBox.top = FXSYS_roundf(TextUnitToGlyphUnit(pData[5]));
}

void CPDF_Type3Char::WillBeDestroyed() {
  // Break cycles.
  form_.reset();
}

void CPDF_Type3Char::Transform(CPDF_Font::FormIface* pForm,
                               const CFX_Matrix& matrix) {
  m_Width = m_Width * matrix.GetXUnit() + 0.5f;

  CFX_FloatRect char_rect;
  if (m_BBox.right <= m_BBox.left || m_BBox.bottom >= m_BBox.top) {
    char_rect = pForm->CalcBoundingBox();
    TextUnitRectToGlyphUnitRect(&char_rect);
  } else {
    char_rect = CFX_FloatRect(m_BBox);
  }

  m_BBox = matrix.TransformRect(char_rect).ToRoundedFxRect();
}

void CPDF_Type3Char::SetForm(std::unique_ptr<CPDF_Font::FormIface> pForm) {
  form_ = std::move(pForm);
}

RetainPtr<CFX_DIBitmap> CPDF_Type3Char::GetBitmap() {
  return bitmap_;
}
