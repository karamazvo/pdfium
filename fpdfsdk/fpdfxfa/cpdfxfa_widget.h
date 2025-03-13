// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FPDFXFA_CPDFXFA_WIDGET_H_
#define FPDFSDK_FPDFXFA_CPDFXFA_WIDGET_H_

#include "core/fxcrt/fx_coordinates.h"
#include "fpdfsdk/cpdfsdk_annot.h"
#include "v8/include/cppgc/persistent.h"
#include "xfa/fxfa/cxfa_ffwidget.h"

class CPDFSDK_PageView;
class CXFA_FFDocView;
class CXFA_FFWidgetHandler;

class CPDFXFA_Widget final : public CPDFSDK_Annot,
                             CPDFSDK_Annot::UnsafeInputHandlers {
 public:
  CPDFXFA_Widget(CXFA_FFWidget* pXFAFFWidget, CPDFSDK_PageView* pPageView);
  ~CPDFXFA_Widget() override;

  // CPDFSDK_Annot:
  CPDFXFA_Widget* AsXFAWidget() override;
  CPDFSDK_Annot::UnsafeInputHandlers* GetUnsafeInputHandlers() override;
  CPDF_Annot::Subtype GetAnnotSubtype() const override;
  CFX_FloatRect GetRect() const override;
  void OnDraw(CFX_RenderDevice* pDevice,
              const CFX_Matrix& mtUser2Device,
              bool bDrawAnnots) override;
  [[nodiscard]] bool DoHitTest(const CFX_PointF& point) override;
  CFX_FloatRect GetViewBBox() override;
  [[nodiscard]] bool CanUndo() override;
  [[nodiscard]] bool CanRedo() override;
  [[nodiscard]] bool Undo() override;
  [[nodiscard]] bool Redo() override;
  WideString GetText() override;
  WideString GetSelectedText() override;
  void ReplaceAndKeepSelection(const WideString& text) override;
  void ReplaceSelection(const WideString& text) override;
  [[nodiscard]] bool SelectAllText() override;
  [[nodiscard]] bool SetIndexSelected(int index, bool selected) override;
  [[nodiscard]] bool IsIndexSelected(int index) override;

  CXFA_FFWidget* GetXFAFFWidget() const { return m_pXFAFFWidget.Get(); }

  [[nodiscard]] bool OnChangedFocus();

 private:
  // CPDFSDK_Annot::UnsafeInputHandlers:
  void OnMouseEnter(Mask<FWL_EVENTFLAG> nFlags) override;
  void OnMouseExit(Mask<FWL_EVENTFLAG> nFlags) override;
  [[nodiscard]] bool OnLButtonDown(Mask<FWL_EVENTFLAG> nFlags,
                                   const CFX_PointF& point) override;
  [[nodiscard]] bool OnLButtonUp(Mask<FWL_EVENTFLAG> nFlags,
                                 const CFX_PointF& point) override;
  [[nodiscard]] bool OnLButtonDblClk(Mask<FWL_EVENTFLAG> nFlags,
                                     const CFX_PointF& point) override;
  [[nodiscard]] bool OnMouseMove(Mask<FWL_EVENTFLAG> nFlags,
                                 const CFX_PointF& point) override;
  [[nodiscard]] bool OnMouseWheel(Mask<FWL_EVENTFLAG> nFlags,
                                  const CFX_PointF& point,
                                  const CFX_Vector& delta) override;
  [[nodiscard]] bool OnRButtonDown(Mask<FWL_EVENTFLAG> nFlags,
                                   const CFX_PointF& point) override;
  [[nodiscard]] bool OnRButtonUp(Mask<FWL_EVENTFLAG> nFlags,
                                 const CFX_PointF& point) override;
  [[nodiscard]] bool OnChar(uint32_t nChar,
                            Mask<FWL_EVENTFLAG> nFlags) override;
  [[nodiscard]] bool OnKeyDown(FWL_VKEYCODE nKeyCode,
                               Mask<FWL_EVENTFLAG> nFlags) override;
  [[nodiscard]] bool OnSetFocus(Mask<FWL_EVENTFLAG> nFlags) override;
  [[nodiscard]] bool OnKillFocus(Mask<FWL_EVENTFLAG> nFlags) override;

  CXFA_FFDocView* GetDocView();
  CXFA_FFWidgetHandler* GetWidgetHandler();

  cppgc::Persistent<CXFA_FFWidget> const m_pXFAFFWidget;
};

#endif  // FPDFSDK_FPDFXFA_CPDFXFA_WIDGET_H_
