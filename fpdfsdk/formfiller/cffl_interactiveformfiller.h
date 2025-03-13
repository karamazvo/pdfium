// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FORMFILLER_CFFL_INTERACTIVEFORMFILLER_H_
#define FPDFSDK_FORMFILLER_CFFL_INTERACTIVEFORMFILLER_H_

#include <map>
#include <memory>
#include <utility>

#include "core/fxcrt/cfx_timer.h"
#include "core/fxcrt/mask.h"
#include "core/fxcrt/observed_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "fpdfsdk/cpdfsdk_annot.h"
#include "fpdfsdk/pwl/ipwl_fillernotify.h"
#include "public/fpdf_fwlevent.h"

class CFFL_FormField;
class CPDFSDK_PageView;
class CPDFSDK_Widget;

class CFFL_InteractiveFormFiller final : public IPWL_FillerNotify {
 public:
  class CallbackIface {
   public:
    virtual ~CallbackIface() = default;

    virtual void OnSetFieldInputFocus(const WideString& text) = 0;
    virtual void OnCalculate(ObservedPtr<CPDFSDK_Annot>& pAnnot) = 0;
    virtual void OnFormat(ObservedPtr<CPDFSDK_Annot>& pAnnot) = 0;
    virtual void Invalidate(IPDF_Page* pPage, const FX_RECT& rect) = 0;
    virtual CPDFSDK_PageView* GetOrCreatePageView(IPDF_Page* pPage) = 0;
    virtual CPDFSDK_PageView* GetPageView(IPDF_Page* pPage) = 0;
    virtual CFX_Timer::HandlerIface* GetTimerHandler() = 0;
    virtual CPDFSDK_Annot* GetFocusAnnot() const = 0;
    [[nodiscard]] virtual bool SetFocusAnnot(
        ObservedPtr<CPDFSDK_Annot>& pAnnot) = 0;
    virtual void InvalidateRect(CPDFSDK_Widget* pWidget,
                                const CFX_FloatRect& rect) = 0;
    virtual void OutputSelectedRect(CFFL_FormField* pFormField,
                                    const CFX_FloatRect& rect) = 0;
    [[nodiscard]] virtual bool IsSelectionImplemented() const = 0;
    virtual void SetCursor(CursorStyle nCursorStyle) = 0;

    // See PDF Reference 1.7, table 3.20 for the permission bits. Returns true
    // if any bit in |flags| is set.
    [[nodiscard]] virtual bool HasPermissions(uint32_t flags) const = 0;
    virtual void OnChange() = 0;
  };

  explicit CFFL_InteractiveFormFiller(CallbackIface* pCallbackIface);
  ~CFFL_InteractiveFormFiller() override;

  [[nodiscard]] bool Annot_HitTest(const CPDFSDK_Widget* pWidget,
                                   const CFX_PointF& point);
  FX_RECT GetViewBBox(const CPDFSDK_PageView* pPageView,
                      CPDFSDK_Widget* pWidget);

  void OnDraw(CPDFSDK_PageView* pPageView,
              CPDFSDK_Widget* pWidget,
              CFX_RenderDevice* pDevice,
              const CFX_Matrix& mtUser2Device);
  void OnDelete(CPDFSDK_Widget* pWidget);

  void OnMouseEnter(CPDFSDK_PageView* pPageView,
                    ObservedPtr<CPDFSDK_Widget>& pWidget,
                    Mask<FWL_EVENTFLAG> nFlag);
  void OnMouseExit(CPDFSDK_PageView* pPageView,
                   ObservedPtr<CPDFSDK_Widget>& pWidget,
                   Mask<FWL_EVENTFLAG> nFlag);
  [[nodiscard]] bool OnLButtonDown(CPDFSDK_PageView* pPageView,
                                   ObservedPtr<CPDFSDK_Widget>& pWidget,
                                   Mask<FWL_EVENTFLAG> nFlags,
                                   const CFX_PointF& point);
  [[nodiscard]] bool OnLButtonUp(CPDFSDK_PageView* pPageView,
                                 ObservedPtr<CPDFSDK_Widget>& pWidget,
                                 Mask<FWL_EVENTFLAG> nFlags,
                                 const CFX_PointF& point);
  [[nodiscard]] bool OnLButtonDblClk(CPDFSDK_PageView* pPageView,
                                     ObservedPtr<CPDFSDK_Widget>& pWidget,
                                     Mask<FWL_EVENTFLAG> nFlags,
                                     const CFX_PointF& point);
  [[nodiscard]] bool OnMouseMove(CPDFSDK_PageView* pPageView,
                                 ObservedPtr<CPDFSDK_Widget>& pWidget,
                                 Mask<FWL_EVENTFLAG> nFlags,
                                 const CFX_PointF& point);
  [[nodiscard]] bool OnMouseWheel(CPDFSDK_PageView* pPageView,
                                  ObservedPtr<CPDFSDK_Widget>& pWidget,
                                  Mask<FWL_EVENTFLAG> nFlags,
                                  const CFX_PointF& point,
                                  const CFX_Vector& delta);
  [[nodiscard]] bool OnRButtonDown(CPDFSDK_PageView* pPageView,
                                   ObservedPtr<CPDFSDK_Widget>& pWidget,
                                   Mask<FWL_EVENTFLAG> nFlags,
                                   const CFX_PointF& point);
  [[nodiscard]] bool OnRButtonUp(CPDFSDK_PageView* pPageView,
                                 ObservedPtr<CPDFSDK_Widget>& pWidget,
                                 Mask<FWL_EVENTFLAG> nFlags,
                                 const CFX_PointF& point);

  [[nodiscard]] bool OnKeyDown(CPDFSDK_Widget* pWidget,
                               FWL_VKEYCODE nKeyCode,
                               Mask<FWL_EVENTFLAG> nFlags);
  [[nodiscard]] bool OnChar(CPDFSDK_Widget* pWidget,
                            uint32_t nChar,
                            Mask<FWL_EVENTFLAG> nFlags);

  [[nodiscard]] bool OnSetFocus(ObservedPtr<CPDFSDK_Widget>& pWidget,
                                Mask<FWL_EVENTFLAG> nFlag);
  [[nodiscard]] bool OnKillFocus(ObservedPtr<CPDFSDK_Widget>& pWidget,
                                 Mask<FWL_EVENTFLAG> nFlag);

  // Wrapper methods for CallbackIface
  void OnSetFieldInputFocus(const WideString& text);
  void Invalidate(IPDF_Page* pPage, const FX_RECT& rect);
  CPDFSDK_PageView* GetOrCreatePageView(IPDF_Page* pPage);
  CPDFSDK_PageView* GetPageView(IPDF_Page* pPage);
  CFX_Timer::HandlerIface* GetTimerHandler();
  void OnChange();

  CFFL_FormField* GetFormFieldForTesting(CPDFSDK_Widget* pAnnot) {
    return GetFormField(pAnnot);
  }

  WideString GetText(CPDFSDK_Widget* pWidget);
  WideString GetSelectedText(CPDFSDK_Widget* pWidget);
  void ReplaceAndKeepSelection(CPDFSDK_Widget* pWidget, const WideString& text);
  void ReplaceSelection(CPDFSDK_Widget* pWidget, const WideString& text);
  [[nodiscard]] bool SelectAllText(CPDFSDK_Widget* pWidget);

  [[nodiscard]] bool CanUndo(CPDFSDK_Widget* pWidget);
  [[nodiscard]] bool CanRedo(CPDFSDK_Widget* pWidget);
  [[nodiscard]] bool Undo(CPDFSDK_Widget* pWidget);
  [[nodiscard]] bool Redo(CPDFSDK_Widget* pWidget);

  [[nodiscard]] static bool IsVisible(CPDFSDK_Widget* pWidget);
  [[nodiscard]] static bool IsReadOnly(CPDFSDK_Widget* pWidget);
  [[nodiscard]] static bool IsValidAnnot(const CPDFSDK_PageView* pPageView,
                                         CPDFSDK_Widget* pWidget);

  [[nodiscard]] bool OnKeyStrokeCommit(ObservedPtr<CPDFSDK_Widget>& pWidget,
                                       const CPDFSDK_PageView* pPageView,
                                       Mask<FWL_EVENTFLAG> nFlag);
  [[nodiscard]] bool OnValidate(ObservedPtr<CPDFSDK_Widget>& pWidget,
                                const CPDFSDK_PageView* pPageView,
                                Mask<FWL_EVENTFLAG> nFlag);
  void OnCalculate(ObservedPtr<CPDFSDK_Widget>& pWidget);
  void OnFormat(ObservedPtr<CPDFSDK_Widget>& pWidget);
  [[nodiscard]] bool OnButtonUp(ObservedPtr<CPDFSDK_Widget>& pWidget,
                                const CPDFSDK_PageView* pPageView,
                                Mask<FWL_EVENTFLAG> nFlag);

  [[nodiscard]] bool SetIndexSelected(ObservedPtr<CPDFSDK_Widget>& pWidget,
                                      int index,
                                      bool selected);
  [[nodiscard]] bool IsIndexSelected(ObservedPtr<CPDFSDK_Widget>& pWidget,
                                     int index);

 private:
  using WidgetToFormFillerMap =
      std::map<CPDFSDK_Widget*, std::unique_ptr<CFFL_FormField>>;

  // IPWL_FillerNotify:
  void InvalidateRect(PerWindowData* pWidgetData,
                      const CFX_FloatRect& rect) override;
  void OutputSelectedRect(PerWindowData* pWidgetData,
                          const CFX_FloatRect& rect) override;
  [[nodiscard]] bool IsSelectionImplemented() const override;
  void SetCursor(CursorStyle nCursorStyle) override;
  void QueryWherePopup(const PerWindowData* pAttached,
                       float fPopupMin,
                       float fPopupMax,
                       bool* bBottom,
                       float* fPopupRet) override;
  BeforeKeystrokeResult OnBeforeKeyStroke(const PerWindowData* pAttached,
                                          WideString& strChange,
                                          const WideString& strChangeEx,
                                          int nSelStart,
                                          int nSelEnd,
                                          bool bKeyDown,
                                          Mask<FWL_EVENTFLAG> nFlag) override;
  [[nodiscard]] bool OnPopupPreOpen(const PerWindowData* pAttached,
                                    Mask<FWL_EVENTFLAG> nFlag) override;
  [[nodiscard]] bool OnPopupPostOpen(const PerWindowData* pAttached,
                                     Mask<FWL_EVENTFLAG> nFlag) override;

#ifdef PDF_ENABLE_XFA
  void SetFocusAnnotTab(CPDFSDK_Widget* pWidget, bool bSameField, bool bNext);
  [[nodiscard]] bool OnClick(ObservedPtr<CPDFSDK_Widget>& pWidget,
                             const CPDFSDK_PageView* pPageView,
                             Mask<FWL_EVENTFLAG> nFlag);
  [[nodiscard]] bool OnFull(ObservedPtr<CPDFSDK_Widget>& pAnnot,
                            const CPDFSDK_PageView* pPageView,
                            Mask<FWL_EVENTFLAG> nFlag);
  [[nodiscard]] bool OnPreOpen(ObservedPtr<CPDFSDK_Widget>& pWidget,
                               const CPDFSDK_PageView* pPageView,
                               Mask<FWL_EVENTFLAG> nFlag);
  [[nodiscard]] bool OnPostOpen(ObservedPtr<CPDFSDK_Widget>& pWidget,
                                const CPDFSDK_PageView* pPageView,
                                Mask<FWL_EVENTFLAG> nFlag);
#endif  // PDF_ENABLE_XFA

  [[nodiscard]] bool IsFillingAllowed(CPDFSDK_Widget* pWidget) const;
  CFFL_FormField* GetFormField(CPDFSDK_Widget* pWidget);
  CFFL_FormField* GetOrCreateFormField(CPDFSDK_Widget* pWidget);
  void UnregisterFormField(CPDFSDK_Widget* pWidget);

  UnownedPtr<CallbackIface> const m_pCallbackIface;
  WidgetToFormFillerMap m_Map;
  bool m_bNotifying = false;
};

#endif  // FPDFSDK_FORMFILLER_CFFL_INTERACTIVEFORMFILLER_H_
