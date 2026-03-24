// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_WIN32_CTEXT_ONLY_PRINTER_DRIVER_H_
#define CORE_FXGE_WIN32_CTEXT_ONLY_PRINTER_DRIVER_H_

#include <stdint.h>
#include <windows.h>

#include <memory>

#include "core/fxge/cfx_windowsrenderdevice.h"

class CTextOnlyPrinterDriver final : public RenderDeviceDriverIface {
 public:
  explicit CTextOnlyPrinterDriver(HDC hDC);
  ~CTextOnlyPrinterDriver() override;

 private:
  // RenderDeviceDriverIface:
  DeviceType GetDeviceType() const override;
<<<<<<< PATCH SET (f390c46f226aeb6e9b780562f28530d2d7ea3f49 Replace render caps with individual boolean functions)
  bool RenderCapGetBits() const;
  bool RenderCapAlphaPath() const;
  bool RenderCapAlphaImage() const;
  bool RenderCapBlendMode() const;
  bool RenderCapSoftClip() const;
  bool RenderCapAlphaOutput() const;
  bool RenderCapByteMaskOutput() const;
  bool RenderCapFillStrokePath() const;
  bool RenderCapShading() const;
  bool RenderCapPremultipliedAlpha() const;
  int GetPixelWidth() const override;
  int GetPixelHeight() const override;
  int GetBitsPerPixel() const override;
  int GetHorzSize() const override;
  int GetVertSize() const override;
||||||| BASE      (ea6858c6be4f3b4539c8df38deb3f0941002555a Replace GetDeviceCaps with specific getter methods)
  int GetDeviceCaps(int caps_id) const override;
  int GetPixelWidth() const override;
  int GetPixelHeight() const override;
  int GetBitsPerPixel() const override;
  int GetHorzSize() const override;
  int GetVertSize() const override;
=======
  int GetDeviceCaps(int caps_id) const override;
>>>>>>> BASE      (5ee52dc9cede873a800675b6faba773773c75c29 Add test for importing page with OCGs into new document)
  void SaveState() override;
  void RestoreState(bool bKeepSaved) override;
  bool SetClip_PathFill(const CFX_Path& path,
                        const CFX_Matrix* pObject2Device,
                        const CFX_FillRenderOptions& fill_options) override;
  bool SetClip_PathStroke(const CFX_Path& path,
                          const CFX_Matrix* pObject2Device,
                          const CFX_GraphStateData* pGraphState) override;
  bool DrawPath(const CFX_Path& path,
                const CFX_Matrix* pObject2Device,
                const CFX_GraphStateData* pGraphState,
                uint32_t fill_color,
                uint32_t stroke_color,
                const CFX_FillRenderOptions& fill_options) override;
  FX_RECT GetClipBox() const override;
  bool SetDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                 uint32_t color,
                 const FX_RECT& src_rect,
                 int left,
                 int top,
                 BlendMode blend_type) override;
  bool StretchDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                     uint32_t color,
                     int dest_left,
                     int dest_top,
                     int dest_width,
                     int dest_height,
                     const FX_RECT* pClipRect,
                     const FXDIB_ResampleOptions& options,
                     BlendMode blend_type) override;
  StartResult StartDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                          float alpha,
                          uint32_t color,
                          const CFX_Matrix& matrix,
                          const FXDIB_ResampleOptions& options,
                          BlendMode blend_type) override;
  bool DrawDeviceText(pdfium::span<const TextCharPos> pCharPos,
                      CFX_Font* font,
                      const CFX_Matrix& mtObject2Device,
                      float font_size,
                      uint32_t color,
                      const CFX_TextRenderOptions& options) override;
  bool MultiplyAlpha(float alpha) override;
  bool MultiplyAlphaMask(RetainPtr<const CFX_DIBitmap> mask) override;

  HDC dc_handle_;
  const int width_;
  const int height_;
  int bits_per_pixel_;
  const int horz_size_;
  const int vert_size_;
  float origin_y_;
  bool set_origin_;
};

#endif  // CORE_FXGE_WIN32_CTEXT_ONLY_PRINTER_DRIVER_H_
