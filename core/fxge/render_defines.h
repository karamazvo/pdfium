// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_RENDER_DEFINES_H_
#define CORE_FXGE_RENDER_DEFINES_H_

enum class DeviceCapsId : int {
  kPixelWidth,
  kPixelHeight,
  kBitsPixel,
  kHorzSize,
  kVertSize,
  kRenderCaps,
};

enum class RenderCapsFlag : int {
  kGetBits = 0x01,
  kAlphaPath = 0x02,
  kAlphaImage = 0x04,
  kAlphaOutput = 0x08,
  kBlendMode = 0x10,
  kSoftClip = 0x20,
  kByteMaskOutput = 0x40,
#ifdef PDF_USE_SKIA
  kFillStrokePath = 0x80,
  kShading = 0x100,
  kPremultipliedAlpha = 0x200,
#endif  // PDF_USE_SKIA
};

#endif  // CORE_FXGE_RENDER_DEFINES_H_
