// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/cxfa_eventparam.h"

CXFA_EventParam::CXFA_EventParam(XFA_EVENTTYPE type) : type_(type) {}

CXFA_EventParam::CXFA_EventParam(const CXFA_EventParam& other) = default;

CXFA_EventParam::~CXFA_EventParam() = default;

CXFA_EventParam& CXFA_EventParam::operator=(const CXFA_EventParam& other) =
    default;

CXFA_EventParam& CXFA_EventParam::operator=(CXFA_EventParam&& other) noexcept =
    default;

WideString CXFA_EventParam::GetNewText() const {
  return ws_prev_text_.First(sel_start_) + ws_change_ +
         ws_prev_text_.Last(ws_prev_text_.GetLength() - sel_end_);
}
