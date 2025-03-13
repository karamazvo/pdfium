// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_FFSIGNATURE_H_
#define XFA_FXFA_CXFA_FFSIGNATURE_H_

#include "xfa/fxfa/cxfa_fffield.h"

class CXFA_FFSignature final : public CXFA_FFField {
 public:
  CONSTRUCT_VIA_MAKE_GARBAGE_COLLECTED;
  ~CXFA_FFSignature() override;

  // CXFA_FFField
  void RenderWidget(CFGAS_GEGraphics* pGS,
                    const CFX_Matrix& matrix,
                    HighlightOption highlight) override;
  [[nodiscard]] bool LoadWidget() override;
  [[nodiscard]] bool AcceptsFocusOnButtonDown(
      Mask<XFA_FWL_KeyFlag> dwFlags,
      const CFX_PointF& point,
      CFWL_MessageMouse::MouseCommand command) override;
  [[nodiscard]] bool OnMouseEnter() override;
  [[nodiscard]] bool OnMouseExit() override;
  [[nodiscard]] bool OnLButtonDown(Mask<XFA_FWL_KeyFlag> dwFlags,
                                   const CFX_PointF& point) override;
  [[nodiscard]] bool OnLButtonUp(Mask<XFA_FWL_KeyFlag> dwFlags,
                                 const CFX_PointF& point) override;
  [[nodiscard]] bool OnLButtonDblClk(Mask<XFA_FWL_KeyFlag> dwFlags,
                                     const CFX_PointF& point) override;
  [[nodiscard]] bool OnMouseMove(Mask<XFA_FWL_KeyFlag> dwFlags,
                                 const CFX_PointF& point) override;
  [[nodiscard]] bool OnMouseWheel(Mask<XFA_FWL_KeyFlag> dwFlags,
                                  const CFX_PointF& point,
                                  const CFX_Vector& delta) override;
  [[nodiscard]] bool OnRButtonDown(Mask<XFA_FWL_KeyFlag> dwFlags,
                                   const CFX_PointF& point) override;
  [[nodiscard]] bool OnRButtonUp(Mask<XFA_FWL_KeyFlag> dwFlags,
                                 const CFX_PointF& point) override;
  [[nodiscard]] bool OnRButtonDblClk(Mask<XFA_FWL_KeyFlag> dwFlags,
                                     const CFX_PointF& point) override;
  [[nodiscard]] bool OnKeyDown(XFA_FWL_VKEYCODE dwKeyCode,
                               Mask<XFA_FWL_KeyFlag> dwFlags) override;
  [[nodiscard]] bool OnChar(uint32_t dwChar,
                            Mask<XFA_FWL_KeyFlag> dwFlags) override;
  FWL_WidgetHit HitTest(const CFX_PointF& point) override;
  FormFieldType GetFormFieldType() override;

 private:
  explicit CXFA_FFSignature(CXFA_Node* pNode);
};

#endif  // XFA_FXFA_CXFA_FFSIGNATURE_H_
