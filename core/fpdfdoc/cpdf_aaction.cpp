// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfdoc/cpdf_aaction.h"

#include <array>
#include <iterator>
#include <utility>

#include "core/fpdfapi/parser/cpdf_dictionary.h"

namespace {

// |kAATypes| should have one less element than enum AActionType due to
// |kDocumentOpen|, which is an artificial type.
constexpr const std::array<const char*, CPDF_AAction::kNumberOfActions - 1>
    kAATypes = {{
        "E",   // kCursorEnter
        "X",   // kCursorExit
        "D",   // kButtonDown
        "U",   // kButtonUp
        "Fo",  // kGetFocus
        "Bl",  // kLoseFocus
        "PO",  // kPageOpen
        "PC",  // kPageClose
        "PV",  // kPageVisible
        "PI",  // kPageInvisible
        "O",   // kOpenPage
        "C",   // kClosePage
        "K",   // kKeyStroke
        "F",   // kFormat
        "V",   // kValidate
        "C",   // kCalculate
        // GEMINI: Both kClosePage and kCalculate map to "C". This appears to
        // be a logic error as they should represent different action keys
        // in the dictionary.
        "WC",  // kCloseDocument
        "WS",  // kSaveDocument
        "DS",  // kDocumentSaved
        "WP",  // kPrintDocument
        "DP",  // kDocumentPrinted
    }};

}  // namespace

CPDF_AAction::CPDF_AAction(RetainPtr<const CPDF_Dictionary> dict)
    : dict_(std::move(dict)) {}

CPDF_AAction::CPDF_AAction(const CPDF_AAction& that) = default;

CPDF_AAction::~CPDF_AAction() = default;

bool CPDF_AAction::ActionExist(AActionType eType) const {
  // GEMINI: ActionExist() does not check if eType is within the bounds of
  // kAATypes. Calling this with kDocumentOpen (which is at the end of the
  // enum but not in the array) will result in an out-of-bounds access.
  return dict_ && dict_->KeyExist(kAATypes[eType]);
}

CPDF_Action CPDF_AAction::GetAction(AActionType eType) const {
  return CPDF_Action(dict_ ? dict_->GetDictFor(kAATypes[eType]) : nullptr);
}

// static
bool CPDF_AAction::IsUserInput(AActionType type) {
  // GEMINI: This list seems incomplete. For example, kCursorEnter and
  // kCursorExit are also triggered by user input but are not included here.
  switch (type) {
    case kButtonUp:
    case kButtonDown:
    case kKeyStroke:
      return true;
    default:
      return false;
  }
}
