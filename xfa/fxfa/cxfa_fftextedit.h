// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_FFTEXTEDIT_H_
#define XFA_FXFA_CXFA_FFTEXTEDIT_H_

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/widestring.h"
#include "v8/include/cppgc/prefinalizer.h"
#include "xfa/fxfa/cxfa_fffield.h"

class CFX_Matrix;
class CXFA_FFWidget;

namespace pdfium {
class CFWL_EventTextWillChange;
class CFWL_Widget;
class IFWL_WidgetDelegate;
}  // namespace pdfium

class CXFA_FFTextEdit : public CXFA_FFField {
  CPPGC_USING_PRE_FINALIZER(CXFA_FFTextEdit, PreFinalize);

 public:
  CONSTRUCT_VIA_MAKE_GARBAGE_COLLECTED;
  ~CXFA_FFTextEdit() override;

  void PreFinalize();

  void Trace(cppgc::Visitor* visitor) const override;

  // CXFA_FFField
  [[nodiscard]] bool LoadWidget() override;
  void UpdateWidgetProperty() override;
  [[nodiscard]] bool AcceptsFocusOnButtonDown(
      Mask<XFA_FWL_KeyFlag> dwFlags,
      const CFX_PointF& point,
      CFWL_MessageMouse::MouseCommand command) override;
  [[nodiscard]] bool OnLButtonDown(Mask<XFA_FWL_KeyFlag> dwFlags,
                                   const CFX_PointF& point) override;
  [[nodiscard]] bool OnRButtonDown(Mask<XFA_FWL_KeyFlag> dwFlags,
                                   const CFX_PointF& point) override;
  [[nodiscard]] bool OnRButtonUp(Mask<XFA_FWL_KeyFlag> dwFlags,
                                 const CFX_PointF& point) override;
  [[nodiscard]] [[nodiscard]] bool OnSetFocus(
      CXFA_FFWidget* pOldWidget) override;
  [[nodiscard]] [[nodiscard]] bool OnKillFocus(
      CXFA_FFWidget* pNewWidget) override;
  void OnProcessMessage(CFWL_Message* pMessage) override;
  void OnProcessEvent(pdfium::CFWL_Event* pEvent) override;
  void OnDrawWidget(CFGAS_GEGraphics* pGraphics,
                    const CFX_Matrix& matrix) override;

  void OnTextWillChange(pdfium::CFWL_Widget* pWidget,
                        pdfium::CFWL_EventTextWillChange* change);
  void OnTextFull(CFWL_Widget* pWidget);

  // CXFA_FFWidget
  [[nodiscard]] bool CanUndo() override;
  [[nodiscard]] bool CanRedo() override;
  [[nodiscard]] bool CanCopy() override;
  [[nodiscard]] bool CanCut() override;
  [[nodiscard]] bool CanPaste() override;
  [[nodiscard]] bool CanSelectAll() override;
  [[nodiscard]] bool Undo() override;
  [[nodiscard]] bool Redo() override;
  std::optional<WideString> Copy() override;
  std::optional<WideString> Cut() override;
  [[nodiscard]] bool Paste(const WideString& wsPaste) override;
  void SelectAll() override;
  void Delete() override;
  void DeSelect() override;
  WideString GetText() override;
  FormFieldType GetFormFieldType() override;

 protected:
  explicit CXFA_FFTextEdit(CXFA_Node* pNode);
  uint32_t GetAlignment();

  cppgc::Member<IFWL_WidgetDelegate> m_pOldDelegate;

 private:
  [[nodiscard]] bool CommitData() override;
  [[nodiscard]] bool UpdateFWLData() override;
  [[nodiscard]] bool IsDataChanged() override;
  void ValidateNumberField(const WideString& wsText);
};

#endif  // XFA_FXFA_CXFA_FFTEXTEDIT_H_
