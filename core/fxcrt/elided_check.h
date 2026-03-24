// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_ELIDED_CHECK_H_
#define CORE_FXCRT_ELIDED_CHECK_H_

#include "core/fxcrt/check.h"

#define ELIDED_CHECK_ON 1

#if defined(ELIDED_CHECK_ON)

asm("check_not_elided_at_line:\n");

#define MAYBE_LINE_ARG int line = __builtin_LINE()
#define MAYBE_TRAILING_LINE_ARG , MAYBE_LINE_ARG
#define MAYBE_PASS_LINE , line
#define ELIDED_CHECK(condition, line)                           \
  do {                                                          \
    if (UNLIKELY(!(condition))) {                               \
      asm("check_not_elided_at_line: .word %c0\n" ::"i"(line)); \
    }                                                           \
  } while (0)

#else  // defined(ELIDED_CHECK_ON)

#define MAYBE_LINE_ARG
#define MAYBE_TRAILING_LINE_ARG
#define MAYBE_PASS_LINE
#define ELIDED_CHECK(condition, line) CHECK(condition)

#endif  // defined(ELIDED_CHECK_ON)
#endif  // CORE_FXCRT_ELIDED_CHECK_H_
