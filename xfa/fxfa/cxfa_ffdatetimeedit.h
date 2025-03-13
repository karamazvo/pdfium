// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_FFDATETIMEEDIT_H_
#define XFA_FXFA_CXFA_FFDATETIMEEDIT_H_

#include "core/fxcrt/fx_coordinates.h"
#include "xfa/fxfa/cxfa_fftextedit.h"

namespace pdfium {
class CFWL_DateTimePicker;
class CFWL_Widget;
}  // namespace pdfium

class CXFA_FFDateTimeEdit final : public CXFA_FFTextEdit {
 public:
  explicit CXFA_FFDateTimeEdit(CXFA_Node* pNode);
  ~CXFA_FFDateTimeEdit() override;

  // CXFA_FFTextEdit
  CFX_RectF GetBBox(FocusOption focus) override;
  [[nodiscard]] bool LoadWidget() override;
  void UpdateWidgetProperty() override;
  void OnProcessEvent(pdfium::CFWL_Event* pEvent) override;

  void OnSelectChanged(pdfium::CFWL_Widget* pWidget,
                       int32_t iYear,
                       int32_t iMonth,
                       int32_t iDay);

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

 private:
  [[nodiscard]] bool PtInActiveRect(const CFX_PointF& point) override;
  [[nodiscard]] bool CommitData() override;
  [[nodiscard]] bool UpdateFWLData() override;
  [[nodiscard]] bool IsDataChanged() override;

  pdfium::CFWL_DateTimePicker* GetPickerWidget();

  uint32_t GetAlignment();
};

#endif  // XFA_FXFA_CXFA_FFDATETIMEEDIT_H_
