// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/render/cpdf_imagerenderer.h"

#include <math.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "build/build_config.h"
#include "core/fpdfapi/page/cpdf_dib.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageloader.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fpdfapi/page/cpdf_occontext.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/page/cpdf_pageimagecache.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_shadingpattern.h"
#include "core/fpdfapi/page/cpdf_tilingpattern.h"
#include "core/fpdfapi/page/cpdf_transferfunc.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fpdfapi/render/cpdf_rendercontext.h"
#include "core/fpdfapi/render/cpdf_renderstatus.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/maybe_owned.h"
#include "core/fxcrt/zip.h"
#include "core/fxge/agg/cfx_agg_imagerenderer.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "core/fxge/cfx_fillrenderoptions.h"
#include "core/fxge/cfx_path.h"
#include "core/fxge/dib/cfx_dibbase.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/cfx_imagestretcher.h"

#if BUILDFLAG(IS_WIN)
#include "core/fxge/dib/cfx_imagetransformer.h"
#endif

namespace {

bool IsImageValueTooBig(int val) {
  // Likely large enough for any real rendering need, but sufficiently small
  // that operations like val1 + val2 or -val will not overflow.
  static constexpr int kLimit = 256 * 1024 * 1024;
  FX_SAFE_INT32 safe_val = val;
  safe_val = safe_val.Abs();
  return safe_val.ValueOrDefault(kLimit) >= kLimit;
}

}  // namespace

CPDF_ImageRenderer::CPDF_ImageRenderer(CPDF_RenderStatus* pStatus)
    : render_status_(pStatus), loader_(std::make_unique<CPDF_ImageLoader>()) {}

CPDF_ImageRenderer::~CPDF_ImageRenderer() = default;

bool CPDF_ImageRenderer::StartLoadDIBBase() {
  if (!GetUnitRect().has_value())
    return false;

  if (!loader_->Start(
          image_object_, render_status_->GetContext()->GetPageCache(),
          render_status_->GetFormResource(), render_status_->GetPageResource(),
          m_bStdCS, render_status_->GetGroupFamily(),
          render_status_->GetLoadMask(),
          {render_status_->GetRenderDevice()->GetWidth(),
           render_status_->GetRenderDevice()->GetHeight()})) {
    return false;
  }
  m_Mode = Mode::kDefault;
  return true;
}

bool CPDF_ImageRenderer::StartRenderDIBBase() {
  if (!loader_->GetBitmap()) {
    return false;
  }

  CPDF_GeneralState& state = image_object_->mutable_general_state();
  m_Alpha = state.GetFillAlpha();
  m_pDIBBase = loader_->GetBitmap();
  if (GetRenderOptions().ColorModeIs(CPDF_RenderOptions::kAlpha) &&
      !loader_->GetMask()) {
    return StartBitmapAlpha();
  }
  RetainPtr<const CPDF_Object> pTR = state.GetTR();
  if (pTR) {
    if (!state.GetTransferFunc())
      state.SetTransferFunc(render_status_->GetTransferFunc(std::move(pTR)));

    if (state.GetTransferFunc() && !state.GetTransferFunc()->GetIdentity())
      m_pDIBBase = loader_->TranslateImage(state.GetTransferFunc());
  }
  m_FillArgb = 0;
  pattern_color_ = false;
  pattern_ = nullptr;
  if (m_pDIBBase->IsMaskFormat()) {
    const CPDF_Color* pColor = image_object_->color_state().GetFillColor();
    if (pColor && pColor->IsPattern()) {
      pattern_ = pColor->GetPattern();
      if (pattern_) {
        pattern_color_ = true;
      }
    }
    m_FillArgb = render_status_->GetFillArgb(image_object_);
  } else if (GetRenderOptions().ColorModeIs(CPDF_RenderOptions::kGray)) {
    RetainPtr<CFX_DIBitmap> pClone = m_pDIBBase->Realize();
    if (!pClone)
      return false;

    pClone->ConvertColorScale(0xffffff, 0);
    m_pDIBBase = pClone;
  }
  m_ResampleOptions = FXDIB_ResampleOptions();
  if (GetRenderOptions().GetOptions().bForceHalftone)
    m_ResampleOptions.bHalftone = true;

#if BUILDFLAG(IS_WIN)
  if (render_status_->GetRenderDevice()->GetDeviceType() ==
      DeviceType::kPrinter) {
    HandleFilters();
  }
#endif

  if (GetRenderOptions().GetOptions().bNoImageSmooth)
    m_ResampleOptions.bNoSmoothing = true;
  else if (image_object_->GetImage()->IsInterpol()) {
    m_ResampleOptions.bInterpolateBilinear = true;
  }

  if (loader_->GetMask()) {
    return DrawMaskedImage();
  }

  if (pattern_color_) {
    return DrawPatternImage();
  }

  if (m_Alpha != 1.0f || !state.HasRef() || !state.GetFillOP() ||
      state.GetOPMode() != 0 || state.GetBlendType() != BlendMode::kNormal ||
      state.GetStrokeAlpha() != 1.0f || state.GetFillAlpha() != 1.0f) {
    return StartDIBBase();
  }
  CPDF_Document* pDocument = nullptr;
  CPDF_Page* pPage = nullptr;
  if (auto* pPageCache = render_status_->GetContext()->GetPageCache()) {
    pPage = pPageCache->GetPage();
    pDocument = pPage->GetDocument();
  } else {
    pDocument = image_object_->GetImage()->GetDocument();
  }
  RetainPtr<const CPDF_Dictionary> pPageResources =
      pPage ? pPage->GetPageResources() : nullptr;
  RetainPtr<const CPDF_Dictionary> pStreamDict =
      image_object_->GetImage()->GetStream()->GetDict();
  RetainPtr<const CPDF_Object> pCSObj =
      pStreamDict->GetDirectObjectFor("ColorSpace");
  auto* pData = CPDF_DocPageData::FromDocument(pDocument);
  RetainPtr<CPDF_ColorSpace> pColorSpace =
      pData->GetColorSpace(pCSObj.Get(), pPageResources);
  if (pColorSpace) {
    CPDF_ColorSpace::Family format = pColorSpace->GetFamily();
    if (format == CPDF_ColorSpace::Family::kDeviceCMYK ||
        format == CPDF_ColorSpace::Family::kSeparation ||
        format == CPDF_ColorSpace::Family::kDeviceN) {
      m_BlendType = BlendMode::kDarken;
    }
  }
  return StartDIBBase();
}

bool CPDF_ImageRenderer::Start(CPDF_ImageObject* pImageObject,
                               const CFX_Matrix& mtObj2Device,
                               bool bStdCS) {
  DCHECK(pImageObject);
  m_bStdCS = bStdCS;
  image_object_ = pImageObject;
  m_BlendType = BlendMode::kNormal;
  m_mtObj2Device = mtObj2Device;
  RetainPtr<const CPDF_Dictionary> pOC = image_object_->GetImage()->GetOC();
  if (pOC && !GetRenderOptions().CheckOCGDictVisible(pOC))
    return false;

  m_ImageMatrix = image_object_->matrix() * mtObj2Device;
  if (StartLoadDIBBase())
    return true;

  return StartRenderDIBBase();
}

bool CPDF_ImageRenderer::Start(RetainPtr<CFX_DIBBase> pDIBBase,
                               FX_ARGB bitmap_argb,
                               const CFX_Matrix& mtImage2Device,
                               const FXDIB_ResampleOptions& options,
                               bool bStdCS) {
  m_pDIBBase = std::move(pDIBBase);
  m_FillArgb = bitmap_argb;
  m_Alpha = 1.0f;
  m_ImageMatrix = mtImage2Device;
  m_ResampleOptions = options;
  m_bStdCS = bStdCS;
  m_BlendType = BlendMode::kNormal;
  return StartDIBBase();
}

#if BUILDFLAG(IS_WIN)
bool CPDF_ImageRenderer::IsPrinting() const {
  if (!render_status_->IsPrint()) {
    return false;
  }

  // Make sure the assumption that no printer device supports blend mode holds.
  CHECK(
      !(render_status_->GetRenderDevice()->GetRenderCaps() & FXRC_BLEND_MODE));
  return true;
}

void CPDF_ImageRenderer::HandleFilters() {
  std::optional<DecoderArray> decoder_array =
      GetDecoderArray(image_object_->GetImage()->GetStream()->GetDict());
  if (!decoder_array.has_value()) {
    return;
  }

  for (const auto& decoder : decoder_array.value()) {
    if (decoder.first == "DCTDecode" || decoder.first == "JPXDecode") {
      m_ResampleOptions.bLossy = true;
      return;
    }
  }
}
#endif  // BUILDFLAG(IS_WIN)

FX_RECT CPDF_ImageRenderer::GetDrawRect() const {
  FX_RECT rect = m_ImageMatrix.GetUnitRect().GetOuterRect();
  rect.Intersect(render_status_->GetRenderDevice()->GetClipBox());
  return rect;
}

CFX_Matrix CPDF_ImageRenderer::GetDrawMatrix(const FX_RECT& rect) const {
  CFX_Matrix new_matrix = m_ImageMatrix;
  new_matrix.Translate(-rect.left, -rect.top);
  return new_matrix;
}

RetainPtr<const CFX_DIBitmap> CPDF_ImageRenderer::CalculateDrawImage(
    CFX_DefaultRenderDevice& bitmap_device,
    RetainPtr<CFX_DIBBase> pDIBBase,
    const CFX_Matrix& mtNewMatrix,
    const FX_RECT& rect) const {
  auto mask_bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  if (!mask_bitmap->Create(rect.Width(), rect.Height(),
                           FXDIB_Format::k8bppRgb)) {
    return nullptr;
  }

  {
    // Limit the scope of `mask_device`, so its dtor can flush out pending
    // operations, if any, to `mask_bitmap`.
    CFX_DefaultRenderDevice mask_device;
    CHECK(mask_device.Attach(mask_bitmap));

    CPDF_RenderStatus mask_status(render_status_->GetContext(), &mask_device);
    mask_status.SetDropObjects(render_status_->GetDropObjects());
    mask_status.SetStdCS(true);
    mask_status.Initialize(nullptr, nullptr);

    CPDF_ImageRenderer mask_renderer(&mask_status);
    if (mask_renderer.Start(std::move(pDIBBase), 0xffffffff, mtNewMatrix,
                            m_ResampleOptions, true)) {
      mask_renderer.Continue(nullptr);
    }
    if (loader_->MatteColor() != 0xffffffff) {
      const int matte_r = FXARGB_R(loader_->MatteColor());
      const int matte_g = FXARGB_G(loader_->MatteColor());
      const int matte_b = FXARGB_B(loader_->MatteColor());
      RetainPtr<CFX_DIBitmap> dest_bitmap = bitmap_device.GetBitmap();
      for (int row = 0; row < rect.Height(); row++) {
        auto mask_scan = mask_bitmap->GetScanline(row).first(rect.Width());
        auto dest_scan =
            dest_bitmap->GetWritableScanlineAs<FX_BGRA_STRUCT<uint8_t>>(row);
        for (auto [mask_ref, dest_ref] : fxcrt::Zip(mask_scan, dest_scan)) {
          if (mask_ref == 0) {
            continue;
          }
          int orig_b = (dest_ref.blue - matte_b) * 255 / mask_ref + matte_b;
          int orig_g = (dest_ref.green - matte_g) * 255 / mask_ref + matte_g;
          int orig_r = (dest_ref.red - matte_r) * 255 / mask_ref + matte_r;
          dest_ref.blue = std::clamp(orig_b, 0, 255);
          dest_ref.green = std::clamp(orig_g, 0, 255);
          dest_ref.red = std::clamp(orig_r, 0, 255);
        }
      }
    }
  }
  CHECK(!mask_bitmap->HasPalette());
  mask_bitmap->ConvertFormat(FXDIB_Format::k8bppMask);
  return mask_bitmap;
}

const CPDF_RenderOptions& CPDF_ImageRenderer::GetRenderOptions() const {
  return render_status_->GetRenderOptions();
}

bool CPDF_ImageRenderer::DrawPatternImage() {
#if BUILDFLAG(IS_WIN)
  if (IsPrinting()) {
    m_Result = false;
    return false;
  }
#endif

  FX_RECT rect = GetDrawRect();
  if (rect.IsEmpty())
    return false;

  CFX_Matrix new_matrix = GetDrawMatrix(rect);
  CFX_DefaultRenderDevice bitmap_device;
  if (!bitmap_device.Create(rect.Width(), rect.Height(), FXDIB_Format::kBgra)) {
    return true;
  }

  CPDF_RenderStatus bitmap_status(render_status_->GetContext(), &bitmap_device);
  bitmap_status.SetOptions(GetRenderOptions());
  bitmap_status.SetDropObjects(render_status_->GetDropObjects());
  bitmap_status.SetStdCS(true);
  bitmap_status.Initialize(nullptr, nullptr);

  CFX_Matrix pattern_matrix = m_mtObj2Device;
  pattern_matrix.Translate(-rect.left, -rect.top);
  if (CPDF_TilingPattern* pTilingPattern = pattern_->AsTilingPattern()) {
    bitmap_status.DrawTilingPattern(pTilingPattern, image_object_,
                                    pattern_matrix, false);
  } else if (CPDF_ShadingPattern* pShadingPattern =
                 pattern_->AsShadingPattern()) {
    bitmap_status.DrawShadingPattern(pShadingPattern, image_object_,
                                     pattern_matrix, false);
  }

  RetainPtr<const CFX_DIBitmap> mask_bitmap =
      CalculateDrawImage(bitmap_device, m_pDIBBase, new_matrix, rect);
  if (!mask_bitmap) {
    return true;
  }

  bitmap_device.GetBitmap()->MultiplyAlphaMask(std::move(mask_bitmap));
  render_status_->GetRenderDevice()->SetDIBitsWithBlend(
      bitmap_device.GetBitmap(), rect.left, rect.top, m_BlendType);
  return false;
}

bool CPDF_ImageRenderer::DrawMaskedImage() {
#if BUILDFLAG(IS_WIN)
  if (IsPrinting()) {
    m_Result = false;
    return false;
  }
#endif

  FX_RECT rect = GetDrawRect();
  if (rect.IsEmpty())
    return false;

  CFX_Matrix new_matrix = GetDrawMatrix(rect);
  CFX_DefaultRenderDevice bitmap_device;
  if (!bitmap_device.Create(rect.Width(), rect.Height(), FXDIB_Format::kBgrx)) {
    return true;
  }
  bitmap_device.Clear(0xffffffff);
  CPDF_RenderStatus bitmap_status(render_status_->GetContext(), &bitmap_device);
  bitmap_status.SetDropObjects(render_status_->GetDropObjects());
  bitmap_status.SetStdCS(true);
  bitmap_status.Initialize(nullptr, nullptr);
  CPDF_ImageRenderer bitmap_renderer(&bitmap_status);
  if (bitmap_renderer.Start(m_pDIBBase, 0, new_matrix, m_ResampleOptions,
                            true)) {
    bitmap_renderer.Continue(nullptr);
  }
  RetainPtr<const CFX_DIBitmap> mask_bitmap =
      CalculateDrawImage(bitmap_device, loader_->GetMask(), new_matrix, rect);
  if (!mask_bitmap) {
    return true;
  }

#if defined(PDF_USE_SKIA)
  if (CFX_DefaultRenderDevice::UseSkiaRenderer() &&
      render_status_->GetRenderDevice()->SetBitsWithMask(
          bitmap_device.GetBitmap(), mask_bitmap, rect.left, rect.top, m_Alpha,
          m_BlendType)) {
    return false;
  }
#endif
  bitmap_device.GetBitmap()->MultiplyAlphaMask(std::move(mask_bitmap));
  bitmap_device.GetBitmap()->MultiplyAlpha(m_Alpha);
  render_status_->GetRenderDevice()->SetDIBitsWithBlend(
      bitmap_device.GetBitmap(), rect.left, rect.top, m_BlendType);
  return false;
}

bool CPDF_ImageRenderer::StartDIBBase() {
  if (m_pDIBBase->GetBPP() > 1) {
    FX_SAFE_SIZE_T image_size = m_pDIBBase->GetBPP();
    image_size /= 8;
    image_size *= m_pDIBBase->GetWidth();
    image_size *= m_pDIBBase->GetHeight();
    if (!image_size.IsValid())
      return false;

    if (image_size.ValueOrDie() > kHugeImageSize &&
        !m_ResampleOptions.bHalftone) {
      m_ResampleOptions.bInterpolateBilinear = true;
    }
  }
  RenderDeviceDriverIface::StartResult result =
      render_status_->GetRenderDevice()->StartDIBitsWithBlend(
          m_pDIBBase, m_Alpha, m_FillArgb, m_ImageMatrix, m_ResampleOptions,
          m_BlendType);
  if (result.result == RenderDeviceDriverIface::Result::kSuccess) {
    m_DeviceHandle = std::move(result.agg_image_renderer);
    if (m_DeviceHandle) {
      m_Mode = Mode::kBlend;
      return true;
    }
    return false;
  }

#if BUILDFLAG(IS_WIN)
  if (result.result == RenderDeviceDriverIface::Result::kNotSupported) {
    return StartDIBBaseFallback();
  }
#endif

  CHECK_EQ(result.result, RenderDeviceDriverIface::Result::kFailure);
  m_Result = false;
  return false;
}

#if BUILDFLAG(IS_WIN)
bool CPDF_ImageRenderer::StartDIBBaseFallback() {
  if ((fabs(m_ImageMatrix.b) >= 0.5f || m_ImageMatrix.a == 0) ||
      (fabs(m_ImageMatrix.c) >= 0.5f || m_ImageMatrix.d == 0)) {
    if (IsPrinting()) {
      m_Result = false;
      return false;
    }

    std::optional<FX_RECT> image_rect = GetUnitRect();
    if (!image_rect.has_value())
      return false;

    FX_RECT clip_box = render_status_->GetRenderDevice()->GetClipBox();
    clip_box.Intersect(image_rect.value());
    m_Mode = Mode::kTransform;
    transformer_ = std::make_unique<CFX_ImageTransformer>(
        m_pDIBBase, m_ImageMatrix, m_ResampleOptions, &clip_box);
    return true;
  }

  std::optional<FX_RECT> image_rect = GetUnitRect();
  if (!image_rect.has_value())
    return false;

  int dest_left;
  int dest_top;
  int dest_width;
  int dest_height;
  if (!GetDimensionsFromUnitRect(image_rect.value(), &dest_left, &dest_top,
                                 &dest_width, &dest_height)) {
    return false;
  }

  if (m_pDIBBase->IsOpaqueImage() && m_Alpha == 1.0f) {
    if (render_status_->GetRenderDevice()->StretchDIBitsWithFlagsAndBlend(
            m_pDIBBase, dest_left, dest_top, dest_width, dest_height,
            m_ResampleOptions, m_BlendType)) {
      return false;
    }
  }
  if (m_pDIBBase->IsMaskFormat()) {
    if (m_Alpha != 1.0f) {
      m_FillArgb = FXARGB_MUL_ALPHA(m_FillArgb, FXSYS_roundf(m_Alpha * 255));
    }
    if (render_status_->GetRenderDevice()->StretchBitMaskWithFlags(
            m_pDIBBase, dest_left, dest_top, dest_width, dest_height,
            m_FillArgb, m_ResampleOptions)) {
      return false;
    }
  }

  if (IsPrinting()) {
    m_Result = false;
    return true;
  }

  FX_RECT clip_box = render_status_->GetRenderDevice()->GetClipBox();
  FX_RECT dest_rect = clip_box;
  dest_rect.Intersect(image_rect.value());
  FX_RECT dest_clip(
      dest_rect.left - image_rect->left, dest_rect.top - image_rect->top,
      dest_rect.right - image_rect->left, dest_rect.bottom - image_rect->top);
  RetainPtr<CFX_DIBitmap> stretched = m_pDIBBase->StretchTo(
      dest_width, dest_height, m_ResampleOptions, &dest_clip);
  if (stretched) {
    render_status_->CompositeDIBitmap(std::move(stretched), dest_rect.left,
                                      dest_rect.top, m_FillArgb, m_Alpha,
                                      m_BlendType, CPDF_Transparency());
  }
  return false;
}
#endif  // BUILDFLAG(IS_WIN)

bool CPDF_ImageRenderer::StartBitmapAlpha() {
  if (m_pDIBBase->IsOpaqueImage()) {
    CFX_Path path;
    path.AppendRect(0, 0, 1, 1);
    path.Transform(m_ImageMatrix);
    const int bitmap_alpha = FXSYS_roundf(m_Alpha * 255);
    uint32_t fill_color =
        ArgbEncode(0xff, bitmap_alpha, bitmap_alpha, bitmap_alpha);
    render_status_->GetRenderDevice()->DrawPath(
        path, nullptr, nullptr, fill_color, 0,
        CFX_FillRenderOptions::WindingOptions());
    return false;
  }

  RetainPtr<CFX_DIBBase> alpha_mask =
      m_pDIBBase->IsMaskFormat() ? m_pDIBBase : m_pDIBBase->CloneAlphaMask();
  if (fabs(m_ImageMatrix.b) >= 0.5f || fabs(m_ImageMatrix.c) >= 0.5f) {
    int left;
    int top;
    alpha_mask = alpha_mask->TransformTo(m_ImageMatrix, &left, &top);
    if (!alpha_mask) {
      return true;
    }

    const int bitmap_alpha = FXSYS_roundf(m_Alpha * 255);
    render_status_->GetRenderDevice()->SetBitMask(
        std::move(alpha_mask), left, top,
        ArgbEncode(0xff, bitmap_alpha, bitmap_alpha, bitmap_alpha));
    return false;
  }

  std::optional<FX_RECT> image_rect = GetUnitRect();
  if (!image_rect.has_value())
    return false;

  int left;
  int top;
  int dest_width;
  int dest_height;
  if (!GetDimensionsFromUnitRect(image_rect.value(), &left, &top, &dest_width,
                                 &dest_height)) {
    return false;
  }

  const int bitmap_alpha = FXSYS_roundf(m_Alpha * 255);
  render_status_->GetRenderDevice()->StretchBitMask(
      std::move(alpha_mask), left, top, dest_width, dest_height,
      ArgbEncode(0xff, bitmap_alpha, bitmap_alpha, bitmap_alpha));
  return false;
}

bool CPDF_ImageRenderer::Continue(PauseIndicatorIface* pPause) {
  switch (m_Mode) {
    case Mode::kNone:
      return false;
    case Mode::kDefault:
      return ContinueDefault(pPause);
    case Mode::kBlend:
      return ContinueBlend(pPause);
#if BUILDFLAG(IS_WIN)
    case Mode::kTransform:
      return ContinueTransform(pPause);
#endif
  }
}

bool CPDF_ImageRenderer::ContinueDefault(PauseIndicatorIface* pPause) {
  if (loader_->Continue(pPause)) {
    return true;
  }

  if (!StartRenderDIBBase())
    return false;

  if (m_Mode == Mode::kDefault)
    return false;

  return Continue(pPause);
}

bool CPDF_ImageRenderer::ContinueBlend(PauseIndicatorIface* pPause) {
  return render_status_->GetRenderDevice()->ContinueDIBits(m_DeviceHandle.get(),
                                                           pPause);
}

#if BUILDFLAG(IS_WIN)
bool CPDF_ImageRenderer::ContinueTransform(PauseIndicatorIface* pPause) {
  if (transformer_->Continue(pPause)) {
    return true;
  }

  RetainPtr<CFX_DIBitmap> bitmap = transformer_->DetachBitmap();
  if (!bitmap) {
    return false;
  }

  if (bitmap->IsMaskFormat()) {
    if (m_Alpha != 1.0f) {
      m_FillArgb = FXARGB_MUL_ALPHA(m_FillArgb, FXSYS_roundf(m_Alpha * 255));
    }
    m_Result = render_status_->GetRenderDevice()->SetBitMask(
        std::move(bitmap), transformer_->result().left,
        transformer_->result().top, m_FillArgb);
  } else {
    bitmap->MultiplyAlpha(m_Alpha);
    m_Result = render_status_->GetRenderDevice()->SetDIBitsWithBlend(
        std::move(bitmap), transformer_->result().left,
        transformer_->result().top, m_BlendType);
  }
  return false;
}
#endif  // BUILDFLAG(IS_WIN)

std::optional<FX_RECT> CPDF_ImageRenderer::GetUnitRect() const {
  CFX_FloatRect image_rect_f = m_ImageMatrix.GetUnitRect();
  FX_RECT image_rect = image_rect_f.GetOuterRect();
  if (!image_rect.Valid())
    return std::nullopt;
  return image_rect;
}

bool CPDF_ImageRenderer::GetDimensionsFromUnitRect(const FX_RECT& rect,
                                                   int* left,
                                                   int* top,
                                                   int* width,
                                                   int* height) const {
  DCHECK(rect.Valid());

  int dest_width = rect.Width();
  int dest_height = rect.Height();
  if (IsImageValueTooBig(dest_width) || IsImageValueTooBig(dest_height))
    return false;

  if (m_ImageMatrix.a < 0)
    dest_width = -dest_width;

  if (m_ImageMatrix.d > 0)
    dest_height = -dest_height;

  int dest_left = dest_width > 0 ? rect.left : rect.right;
  int dest_top = dest_height > 0 ? rect.top : rect.bottom;
  if (IsImageValueTooBig(dest_left) || IsImageValueTooBig(dest_top))
    return false;

  *left = dest_left;
  *top = dest_top;
  *width = dest_width;
  *height = dest_height;
  return true;
}
