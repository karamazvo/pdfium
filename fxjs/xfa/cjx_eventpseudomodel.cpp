// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_eventpseudomodel.h"

#include <algorithm>

#include "core/fxcrt/notreached.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/span.h"
#include "fxjs/fxv8.h"
#include "fxjs/xfa/cfxjse_engine.h"
#include "v8/include/v8-primitive.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/parser/cscript_eventpseudomodel.h"

namespace {

void StringProperty(v8::Isolate* isolate,
                    v8::Local<v8::Value>* pReturn,
                    WideString* wsValue,
                    bool bSetting) {
  if (bSetting) {
    *wsValue = fxv8::ReentrantToWideStringHelper(isolate, *pReturn);
    return;
  }
  *pReturn = fxv8::NewStringHelper(isolate, wsValue->ToUTF8().AsStringView());
}

void IntegerProperty(v8::Isolate* isolate,
                     v8::Local<v8::Value>* pReturn,
                     int32_t* iValue,
                     bool bSetting) {
  if (bSetting) {
    *iValue = fxv8::ReentrantToInt32Helper(isolate, *pReturn);
    return;
  }
  *pReturn = fxv8::NewNumberHelper(isolate, *iValue);
}

void BooleanProperty(v8::Isolate* isolate,
                     v8::Local<v8::Value>* pReturn,
                     bool* bValue,
                     bool bSetting) {
  if (bSetting) {
    *bValue = fxv8::ReentrantToBooleanHelper(isolate, *pReturn);
    return;
  }
  *pReturn = fxv8::NewBooleanHelper(isolate, *bValue);
}

}  // namespace

const CJX_MethodSpec CJX_EventPseudoModel::MethodSpecs[] = {
    {"emit", emit_static},
    {"reset", reset_static}};

CJX_EventPseudoModel::CJX_EventPseudoModel(CScript_EventPseudoModel* model)
    : CJX_Object(model) {
  DefineMethods(MethodSpecs);
}

CJX_EventPseudoModel::~CJX_EventPseudoModel() = default;

bool CJX_EventPseudoModel::DynamicTypeIs(TypeTag eType) const {
  return eType == static_type__ || ParentType__::DynamicTypeIs(eType);
}

void CJX_EventPseudoModel::cancelAction(v8::Isolate* isolate,
                                        v8::Local<v8::Value>* value,
                                        bool bSetting,
                                        XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::CancelAction, bSetting);
}

void CJX_EventPseudoModel::change(v8::Isolate* isolate,
                                  v8::Local<v8::Value>* value,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::Change, bSetting);
}

void CJX_EventPseudoModel::commitKey(v8::Isolate* isolate,
                                     v8::Local<v8::Value>* value,
                                     bool bSetting,
                                     XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::CommitKey, bSetting);
}

void CJX_EventPseudoModel::fullText(v8::Isolate* isolate,
                                    v8::Local<v8::Value>* value,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::FullText, bSetting);
}

void CJX_EventPseudoModel::keyDown(v8::Isolate* isolate,
                                   v8::Local<v8::Value>* value,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::Keydown, bSetting);
}

void CJX_EventPseudoModel::modifier(v8::Isolate* isolate,
                                    v8::Local<v8::Value>* value,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::Modifier, bSetting);
}

void CJX_EventPseudoModel::newContentType(v8::Isolate* isolate,
                                          v8::Local<v8::Value>* value,
                                          bool bSetting,
                                          XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::NewContentType, bSetting);
}

void CJX_EventPseudoModel::newText(v8::Isolate* isolate,
                                   v8::Local<v8::Value>* value,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  if (bSetting) {
    return;
  }

  CXFA_EventParam* pEventParam =
      GetDocument()->GetScriptContext()->GetEventParam();
  if (!pEventParam) {
    return;
  }

  *value = fxv8::NewStringHelper(
      isolate, pEventParam->GetNewText().ToUTF8().AsStringView());
}

void CJX_EventPseudoModel::prevContentType(v8::Isolate* isolate,
                                           v8::Local<v8::Value>* value,
                                           bool bSetting,
                                           XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::PreviousContentType, bSetting);
}

void CJX_EventPseudoModel::prevText(v8::Isolate* isolate,
                                    v8::Local<v8::Value>* value,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::PreviousText, bSetting);
}

void CJX_EventPseudoModel::reenter(v8::Isolate* isolate,
                                   v8::Local<v8::Value>* value,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::Reenter, bSetting);
}

void CJX_EventPseudoModel::selEnd(v8::Isolate* isolate,
                                  v8::Local<v8::Value>* value,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::SelectionEnd, bSetting);
}

void CJX_EventPseudoModel::selStart(v8::Isolate* isolate,
                                    v8::Local<v8::Value>* value,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::SelectionStart, bSetting);
}

void CJX_EventPseudoModel::shift(v8::Isolate* isolate,
                                 v8::Local<v8::Value>* value,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::Shift, bSetting);
}

void CJX_EventPseudoModel::soapFaultCode(v8::Isolate* isolate,
                                         v8::Local<v8::Value>* value,
                                         bool bSetting,
                                         XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::SoapFaultCode, bSetting);
}

void CJX_EventPseudoModel::soapFaultString(v8::Isolate* isolate,
                                           v8::Local<v8::Value>* value,
                                           bool bSetting,
                                           XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::SoapFaultString, bSetting);
}

void CJX_EventPseudoModel::target(v8::Isolate* isolate,
                                  v8::Local<v8::Value>* value,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Property(isolate, value, XFA_Event::Target, bSetting);
}

CJS_Result CJX_EventPseudoModel::emit(
    CFXJSE_Engine* runtime,
    pdfium::span<v8::Local<v8::Value>> params) {
  CXFA_EventParam* pEventParam = runtime->GetEventParam();
  if (!pEventParam) {
    return CJS_Result::Success();
  }

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify) {
    return CJS_Result::Success();
  }

  pNotify->HandleWidgetEvent(runtime->GetEventTarget(), pEventParam);
  return CJS_Result::Success();
}

CJS_Result CJX_EventPseudoModel::reset(
    CFXJSE_Engine* runtime,
    pdfium::span<v8::Local<v8::Value>> params) {
  CXFA_EventParam* pEventParam = runtime->GetEventParam();
  if (pEventParam) {
    *pEventParam = CXFA_EventParam(XFA_EVENT_Unknown);
  }

  return CJS_Result::Success();
}

void CJX_EventPseudoModel::Property(v8::Isolate* isolate,
                                    v8::Local<v8::Value>* value,
                                    XFA_Event dwFlag,
                                    bool bSetting) {
  // Only the cancelAction, selStart, selEnd and change properties are writable.
  if (bSetting && dwFlag != XFA_Event::CancelAction &&
      dwFlag != XFA_Event::SelectionStart &&
      dwFlag != XFA_Event::SelectionEnd && dwFlag != XFA_Event::Change) {
    return;
  }

  CFXJSE_Engine* pScriptContext = GetDocument()->GetScriptContext();
  CXFA_EventParam* pEventParam = pScriptContext->GetEventParam();
  if (!pEventParam) {
    return;
  }

  switch (dwFlag) {
    case XFA_Event::CancelAction:
      BooleanProperty(isolate, value, &pEventParam->cancel_action_, bSetting);
      break;
    case XFA_Event::Change:
      StringProperty(isolate, value, &pEventParam->change_, bSetting);
      break;
    case XFA_Event::CommitKey:
      IntegerProperty(isolate, value, &pEventParam->commit_key_, bSetting);
      break;
    case XFA_Event::FullText:
      StringProperty(isolate, value, &pEventParam->full_text_, bSetting);
      break;
    case XFA_Event::Keydown:
      BooleanProperty(isolate, value, &pEventParam->key_down_, bSetting);
      break;
    case XFA_Event::Modifier:
      BooleanProperty(isolate, value, &pEventParam->modifier_, bSetting);
      break;
    case XFA_Event::NewContentType:
      StringProperty(isolate, value, &pEventParam->new_content_type_, bSetting);
      break;
    case XFA_Event::NewText:
      NOTREACHED();
    case XFA_Event::PreviousContentType:
      StringProperty(isolate, value, &pEventParam->prev_content_type_,
                     bSetting);
      break;
    case XFA_Event::PreviousText:
      StringProperty(isolate, value, &pEventParam->prev_text_, bSetting);
      break;
    case XFA_Event::Reenter:
      BooleanProperty(isolate, value, &pEventParam->reenter_, bSetting);
      break;
    case XFA_Event::SelectionEnd:
      IntegerProperty(isolate, value, &pEventParam->sel_end_, bSetting);

      pEventParam->sel_end_ = std::max(0, pEventParam->sel_end_);
      pEventParam->sel_end_ = std::min(
          pEventParam->sel_end_,
          pdfium::checked_cast<int32_t>(pEventParam->prev_text_.GetLength()));
      pEventParam->sel_start_ =
          std::min(pEventParam->sel_start_, pEventParam->sel_end_);
      break;
    case XFA_Event::SelectionStart:
      IntegerProperty(isolate, value, &pEventParam->sel_start_, bSetting);
      pEventParam->sel_start_ = std::max(0, pEventParam->sel_start_);
      pEventParam->sel_start_ = std::min(
          pEventParam->sel_start_,
          pdfium::checked_cast<int32_t>(pEventParam->prev_text_.GetLength()));
      pEventParam->sel_end_ =
          std::max(pEventParam->sel_start_, pEventParam->sel_end_);
      break;
    case XFA_Event::Shift:
      BooleanProperty(isolate, value, &pEventParam->shift_, bSetting);
      break;
    case XFA_Event::SoapFaultCode:
      StringProperty(isolate, value, &pEventParam->soap_fault_code_, bSetting);
      break;
    case XFA_Event::SoapFaultString:
      StringProperty(isolate, value, &pEventParam->soap_fault_string_,
                     bSetting);
      break;
    case XFA_Event::Target:
      break;
  }
}
