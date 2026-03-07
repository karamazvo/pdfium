// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_WIN32_CWIN32_DEVICECAPS_H_
#define CORE_FXGE_WIN32_CWIN32_DEVICECAPS_H_

class DeviceCapsAPI {
 public:
  static int GetBitsPerPixel(HDC hDC);
  static int GetWidth(HDC hDC);
  static int GetHeight(HDC hDC);
  static int GetHorzSize(HDC hDC);
  static int GetVertSize(HDC hDC);
  static int GetPixelsY(HDC hDC);
  static int GetDeviceType(HDC hDC);
};

#endif  // CORE_FXGE_WIN32_CWIN32_DEVICECAPS_H_
