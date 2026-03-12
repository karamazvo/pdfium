// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/pdf_test_environment.h"

#include "core/fxge/cfx_gemodule.h"

PDFTestEnvironment::PDFTestEnvironment() = default;

PDFTestEnvironment::~PDFTestEnvironment() = default;

// testing::Environment:
void PDFTestEnvironment::SetUp() {
#if defined(PDF_USE_SKIA)
  CFX_GEModule::Create(test_fonts_.font_paths(), CFX_GEModule::kDefaultRenderer,
                       CFX_FontMgr::FontBackend::kFreeType);
#else
  CFX_GEModule::Create(test_fonts_.font_paths(),
                       CFX_FontMgr::FontBackend::kFreeType);
#endif
}

void PDFTestEnvironment::TearDown() {
  CFX_GEModule::Destroy();
}
