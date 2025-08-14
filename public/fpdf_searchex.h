// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef PUBLIC_FPDF_SEARCHEX_H_
#define PUBLIC_FPDF_SEARCHEX_H_

// NOLINTNEXTLINE(build/include)
#include "fpdfview.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Get the character index in the text page's character list.
//
//   text_page  - a text page information structure.
//   nTextIndex - index of a character in the string returned from
//                FPDFText_GetText().
//
// Returns the index of the character in the text page's character list.
// Returns -1 on error.
//
// The text page's character list is a list of all characters on the page,
// including virtual characters.
FPDF_EXPORT int FPDF_CALLCONV
FPDFText_GetCharIndexFromTextIndex(FPDF_TEXTPAGE text_page, int nTextIndex);

// Get the character index in the string returned from FPDFText_GetText().
//
//   text_page  - a text page information structure.
//   nCharIndex - index of a character in the text page's character list.
//
// Returns the index of the character in the string returned from
// FPDFText_GetText(). Returns -1 on error.
//
// The text page's character list is a list of all characters on the page,
// including virtual characters.
FPDF_EXPORT int FPDF_CALLCONV
FPDFText_GetTextIndexFromCharIndex(FPDF_TEXTPAGE text_page, int nCharIndex);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PUBLIC_FPDF_SEARCHEX_H_
