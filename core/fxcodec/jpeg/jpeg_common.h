// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_JPEG_JPEG_COMMON_H_
#define CORE_FXCODEC_JPEG_JPEG_COMMON_H_

// Common code for interacting with libjpeg shared by other files in
// core/fxcodec/jpeg/. Not intended to be included in headers.

#include <setjmp.h>
#include <stdio.h>

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
// windows.h must come before the third_party/libjpeg_turbo includes.
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#undef FAR
#if defined(USE_SYSTEM_LIBJPEG)
#include <jerror.h>
#include <jpeglib.h>
#elif defined(USE_LIBJPEG_TURBO)
#include "third_party/libjpeg_turbo/jerror.h"
#include "third_party/libjpeg_turbo/jpeglib.h"
#else
#include "third_party/libjpeg/jerror.h"
#include "third_party/libjpeg/jpeglib.h"
#endif

struct JpegCommon {
  jmp_buf m_JmpBuf;
  struct jpeg_decompress_struct m_Cinfo;
  struct jpeg_error_mgr m_ErrMgr;
  struct jpeg_source_mgr m_SrcMgr;
};
typedef struct JpegCommon JpegCommon;

void src_do_nothing(j_decompress_ptr cinfo);
boolean src_fill_buffer(j_decompress_ptr cinfo);
boolean src_resync(j_decompress_ptr cinfo, int desired);
void error_do_nothing(j_common_ptr cinfo);
void error_do_nothing_int(j_common_ptr cinfo, int);
void error_do_nothing_char(j_common_ptr cinfo, char*);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CORE_FXCODEC_JPEG_JPEG_COMMON_H_
