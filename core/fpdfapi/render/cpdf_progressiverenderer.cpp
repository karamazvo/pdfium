// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/render/cpdf_progressiverenderer.h"

#include "build/build_config.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fpdfapi/page/cpdf_pageimagecache.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_pageobjectholder.h"
#include "core/fpdfapi/render/cpdf_renderoptions.h"
#include "core/fpdfapi/render/cpdf_renderstatus.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/pauseindicator_iface.h"
#include "core/fxge/cfx_renderdevice.h"

CPDF_ProgressiveRenderer::CPDF_ProgressiveRenderer(
    CPDF_RenderContext* pContext,
    CFX_RenderDevice* pDevice,
    const CPDF_RenderOptions* pOptions)
    : context_(pContext), device_(pDevice), options_(pOptions) {
  CHECK(context_);
  CHECK(device_);
}

CPDF_ProgressiveRenderer::~CPDF_ProgressiveRenderer() {
  if (render_status_) {
    render_status_.reset();  // Release first.
    device_->RestoreState(false);
  }
}

void CPDF_ProgressiveRenderer::Start(PauseIndicatorIface* pPause) {
  if (m_Status != kReady) {
    m_Status = kFailed;
    return;
  }
  m_Status = kToBeContinued;
  Continue(pPause);
}

void CPDF_ProgressiveRenderer::Continue(PauseIndicatorIface* pPause) {
  while (m_Status == kToBeContinued) {
    if (!current_layer_) {
      if (m_LayerIndex >= context_->CountLayers()) {
        m_Status = kDone;
        return;
      }
      current_layer_ = context_->GetLayer(m_LayerIndex);
      m_LastObjectRendered = current_layer_->GetObjectHolder()->end();
      render_status_ = std::make_unique<CPDF_RenderStatus>(context_, device_);
      if (options_) {
        render_status_->SetOptions(*options_);
      }
      render_status_->SetTransparency(
          current_layer_->GetObjectHolder()->GetTransparency());
      render_status_->Initialize(nullptr, nullptr);
      device_->SaveState();
      m_ClipRect = current_layer_->GetMatrix().GetInverse().TransformRect(
          CFX_FloatRect(device_->GetClipBox()));
    }
    CPDF_PageObjectHolder::const_iterator iter;
    CPDF_PageObjectHolder::const_iterator iterEnd =
        current_layer_->GetObjectHolder()->end();
    if (m_LastObjectRendered != iterEnd) {
      iter = m_LastObjectRendered;
      ++iter;
    } else {
      iter = current_layer_->GetObjectHolder()->begin();
    }
    int nObjsToGo = kStepLimit;
    bool is_mask = false;
    while (iter != iterEnd) {
      CPDF_PageObject* pCurObj = iter->get();
      if (pCurObj->IsActive() && pCurObj->GetRect().left <= m_ClipRect.right &&
          pCurObj->GetRect().right >= m_ClipRect.left &&
          pCurObj->GetRect().bottom <= m_ClipRect.top &&
          pCurObj->GetRect().top >= m_ClipRect.bottom) {
        if (options_->GetOptions().bBreakForMasks && pCurObj->IsImage() &&
            pCurObj->AsImage()->GetImage()->IsMask()) {
#if BUILDFLAG(IS_WIN)
          if (device_->GetDeviceType() == DeviceType::kPrinter) {
            m_LastObjectRendered = iter;
            render_status_->ProcessClipPath(pCurObj->clip_path(),
                                            current_layer_->GetMatrix());
            return;
          }
#endif
          is_mask = true;
        }
        if (render_status_->ContinueSingleObject(
                pCurObj, current_layer_->GetMatrix(), pPause)) {
          return;
        }
        if (pCurObj->IsImage() && render_status_->GetRenderOptions()
                                      .GetOptions()
                                      .bLimitedImageCache) {
          context_->GetPageCache()->CacheOptimization(
              render_status_->GetRenderOptions().GetCacheSizeLimit());
        }
        if (pCurObj->IsForm() || pCurObj->IsShading())
          nObjsToGo = 0;
        else
          --nObjsToGo;
      }
      m_LastObjectRendered = iter;
      if (nObjsToGo == 0) {
        if (pPause && pPause->NeedToPauseNow())
          return;
        nObjsToGo = kStepLimit;
      }
      ++iter;
      if (is_mask && iter != iterEnd)
        return;
    }
    if (current_layer_->GetObjectHolder()->GetParseState() ==
        CPDF_PageObjectHolder::ParseState::kParsed) {
      render_status_.reset();
      device_->RestoreState(false);
      current_layer_ = nullptr;
      m_LayerIndex++;
      if (is_mask || (pPause && pPause->NeedToPauseNow()))
        return;
    } else if (is_mask) {
      return;
    } else {
      current_layer_->GetObjectHolder()->ContinueParse(pPause);
      if (current_layer_->GetObjectHolder()->GetParseState() !=
          CPDF_PageObjectHolder::ParseState::kParsed) {
        return;
      }
    }
  }
}
