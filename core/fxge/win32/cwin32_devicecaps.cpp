// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/win32/cwin32_devicecaps.h"

int DeviceCapsAPI::GetBitsPerPixel(HDC hDC) {
  return ::GetDeviceCaps(hDC, BITSPIXEL);
}

int DeviceCapsAPI::GetWidth(HDC hDC) {
  return ::GetDeviceCaps(hDC, HORZRES);
}

int DeviceCapsAPI::GetHeight(HDC hDC) {
  return ::GetDeviceCaps(hDC, VERTRES);
}

int DeviceCapsAPI::GetHorzSize(HDC hDC) {
  return ::GetDeviceCaps(hDC, HORZSIZE);
}

int DeviceCapsAPI::GetVertSize(HDC hDC) {
  return ::GetDeviceCaps(hDC, VERTSIZE);
}

int DeviceCapsAPI::GetPixelsY(HDC hDC) {
  return ::GetDeviceCaps(hDC, LOGPIXELSY);
}

int DeviceCapsAPI::GetDeviceType(HDC hDC) {
  return ::GetDeviceCaps(hDC, TECHNOLOGY);
}
