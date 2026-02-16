// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/mask.h"

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
// Assuming these are Skia-only for now. If this assumption changes, update both
// the #if logic here, as well as the callsites that check these capabilities.
#if defined(PDF_USE_SKIA)
  kFillStrokePath = 0x80,
  kShading = 0x100,
  kPremultipliedAlpha = 0x200,
#endif  // PDF_USE_SKIA
};

constexpr Mask<RenderCapsFlag> RenderCapsFlagMask =
    Mask<RenderCapsFlag>(
        RenderCapsFlag::kGetBits,
        RenderCapsFlag::kAlphaPath,
        RenderCapsFlag::kAlphaImage,
        RenderCapsFlag::kAlphaOutput,
        RenderCapsFlag::kBlendMode,
        RenderCapsFlag::kSoftClip,
        RenderCapsFlag::kByteMaskOutput)
#if defined(PDF_USE_SKIA)
    | Mask<RenderCapsFlag>(
        RenderCapsFlag::kFillStrokePath,
        RenderCapsFlag::kShading,
        RenderCapsFlag::kPremultipliedAlpha)
#endif
    ;

#endif  // CORE_FXGE_RENDER_DEFINES_H_
