// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FPDFAPI_EDIT_CPDF_FONTSUBSETTER_H_
#define CORE_FPDFAPI_EDIT_CPDF_FONTSUBSETTER_H_

#include <stdint.h>

#include <map>

#include "core/fxcrt/retain_ptr.h"

class CPDF_Document;
class CPDF_Object;
class CPDF_Parser;

std::map<uint32_t, RetainPtr<CPDF_Object>> GenerateFontSubsetObjectOverrides(
    CPDF_Document* doc,
    CPDF_Parser* parser);

#endif  // CORE_FPDFAPI_EDIT_CPDF_FONTSUBSETTER_H_
