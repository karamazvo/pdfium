// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_WIN32_CWIN32_DEVICECAPS_H_
#define CORE_FXGE_WIN32_CWIN32_DEVICECAPS_H_

namespace w32_caps {

inline int GetBitsPerPixel(HDC hDC) {
  return ::GetDeviceCaps(hDC, BITSPIXEL);
}
inline int GetWidth(HDC hDC) {
  return ::GetDeviceCaps(hDC, HORZRES);
}
inline int GetHeight(HDC hDC) {
  return ::GetDeviceCaps(hDC, VERTRES);
}
inline int GetHorzSize(HDC hDC) {
  return ::GetDeviceCaps(hDC, HORZSIZE);
}
inline int GetVertSize(HDC hDC) {
  return ::GetDeviceCaps(hDC, VERTSIZE);
}
inline int GetPixelsY(HDC hDC) {
  return ::GetDeviceCaps(hDC, LOGPIXELSY);
}
inline int GetDeviceType(HDC hDC) {
  return ::GetDeviceCaps(hDC, TECHNOLOGY);
}

}  // namespace w32_caps

#endif  // CORE_FXGE_WIN32_CWIN32_DEVICECAPS_H_
