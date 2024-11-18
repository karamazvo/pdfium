// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/jpeg/jpeg_common.h"

boolean jpeg_common_create_decompress(JpegCommon* jpeg_common) {
  if (setjmp(jpeg_common->jmpbuf) == -1) {
    return FALSE;
  }
  jpeg_create_decompress(&jpeg_common->cinfo);
  return TRUE;
}

void jpeg_common_src_do_nothing(j_decompress_ptr cinfo) {}

boolean jpeg_common_src_fill_buffer(j_decompress_ptr cinfo) {
  return FALSE;
}

boolean jpeg_common_src_resync(j_decompress_ptr cinfo, int desired) {
  return FALSE;
}

void jpeg_common_error_do_nothing(j_common_ptr cinfo) {}

void jpeg_common_error_do_nothing_int(j_common_ptr cinfo, int arg) {}

void jpeg_common_error_do_nothing_char(j_common_ptr cinfo, char* arg) {}

