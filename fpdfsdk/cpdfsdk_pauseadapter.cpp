// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/cpdfsdk_pauseadapter.h"

CPDFSDK_PauseAdapter::CPDFSDK_PauseAdapter(IFSDK_PAUSE* IPause)
    : ipause_(IPause) {}

CPDFSDK_PauseAdapter::~CPDFSDK_PauseAdapter() = default;

bool CPDFSDK_PauseAdapter::NeedToPauseNow() {
  return ipause_->NeedToPauseNow && ipause_->NeedToPauseNow(ipause_);
}
