// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/cfx_gemodule.h"

#include "core/fxcrt/check.h"
#include "core/fxge/cfx_folderfontinfo.h"

namespace {

CFX_GEModule* g_GEModule = nullptr;

#if defined(PDF_USE_SKIA)
CFX_GEModule::RendererType g_renderer_type = CFX_GEModule::kDefaultRenderer;
#endif

}  // namespace

// static
#if defined(PDF_USE_SKIA)
void CFX_GEModule::Create(const char** pUserFontPaths,
                          RendererType renderer_type,
                          CFX_FontMgr::FontBackend backend) {
  g_renderer_type = renderer_type;
#else
void CFX_GEModule::Create(const char** pUserFontPaths,
                          CFX_FontMgr::FontBackend backend) {
#endif
  DCHECK(!g_GEModule);
  g_GEModule = new CFX_GEModule(pUserFontPaths, backend);
  g_GEModule->platform_->Init();
  g_GEModule->font_mgr_->GetBuiltinMapper()->SetSystemFontInfo(
      g_GEModule->platform_->CreateDefaultSystemFontInfo());
}

// static
void CFX_GEModule::Destroy() {
  DCHECK(g_GEModule);
  g_GEModule->platform_->Terminate();
  delete g_GEModule;
  g_GEModule = nullptr;
}

// static
CFX_GEModule* CFX_GEModule::Get() {
  DCHECK(g_GEModule);
  return g_GEModule;
}

// static
bool CFX_GEModule::UseSkiaRenderer() {
#if defined(PDF_USE_SKIA)
  return g_renderer_type == RendererType::kSkia;
#else
  return false;
#endif
}

#if defined(PDF_USE_SKIA)
// static
void CFX_GEModule::SetRendererType(RendererType renderer_type) {
  g_renderer_type = renderer_type;
}
#endif

CFX_GEModule::CFX_GEModule(const char** pUserFontPaths,
                           CFX_FontMgr::FontBackend backend)
    : platform_(PlatformIface::Create()),
      font_mgr_(std::make_unique<CFX_FontMgr>(backend)),
      user_font_paths_(pUserFontPaths) {}

CFX_GEModule::~CFX_GEModule() = default;
