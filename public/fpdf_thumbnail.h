// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PUBLIC_FPDF_THUMBNAIL_H_
#define PUBLIC_FPDF_THUMBNAIL_H_

#include <stdint.h>

// NOLINTNEXTLINE(build/include)
#include "fpdfview.h"

#ifdef __cplusplus
extern "C" {
#endif

// Experimental API.
// Gets the decoded data from the thumbnail of |page| if it exists.
//
//   page    - handle to a page.
//   buffer  - buffer for holding the decoded image data. Can be NULL.
//   buflen  - length of the buffer in bytes. Can be 0.
//
// Returns the size of the decoded data in bytes. If |buffer| is NULL or
// |buflen| is smaller than the decoded data, this function returns the required
// buffer size. If the thumbnail does not exist, this function returns 0.
FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFPage_GetDecodedThumbnailData(FPDF_PAGE page,
                                 void* buffer,
                                 unsigned long buflen);

// Experimental API.
// Gets the raw data from the thumbnail of |page| if it exists.
//
//   page    - handle to a page.
//   buffer  - buffer for holding the raw image data. Can be NULL.
//   buflen  - length of the buffer in bytes. Can be 0.
//
// Returns the size of the raw data in bytes. If |buffer| is NULL or |buflen| is
// smaller than the raw data, this function returns the required buffer size.
// If the thumbnail does not exist, this function returns 0.
FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFPage_GetRawThumbnailData(FPDF_PAGE page,
                             void* buffer,
                             unsigned long buflen);

// Experimental API.
// Returns the thumbnail of |page| as a FPDF_BITMAP. Returns a nullptr
// if unable to access the thumbnail's stream.
//
//   page - handle to a page.
FPDF_EXPORT FPDF_BITMAP FPDF_CALLCONV
FPDFPage_GetThumbnailAsBitmap(FPDF_PAGE page);

#ifdef __cplusplus
}
#endif

#endif  // PUBLIC_FPDF_THUMBNAIL_H_
