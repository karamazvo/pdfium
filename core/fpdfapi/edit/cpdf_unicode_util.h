
// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FPDFAPI_EDIT_CPDF_UNICODE_UTIL_H_
#define CORE_FPDFAPI_EDIT_CPDF_UNICODE_UTIL_H_

#include <stdint.h>

#include <map>

#include "core/fxcrt/retain_ptr.h"

class CPDF_Document;
class CPDF_Stream;

// Loads the charcode to unicode mapping into a stream.
RetainPtr<CPDF_Stream> LoadUnicode(
    CPDF_Document* doc,
    const std::multimap<uint32_t, uint32_t>& to_unicode);

#endif  // CORE_FPDFAPI_EDIT_CPDF_UNICODE_UTIL_H_
