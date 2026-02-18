#!/bin/bash
# Copyright 2026 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# adb_push_test.sh: push files from a PDFium checkout to android emulator.
#
# PRECONDITIONS:
#   Android emulator already configured on host.
#   Android binaries build into out/Android.
#   Script is run from top-level pdfium dirctory.

LOCAL_BUILD_DIR=out/Android
REMOTE_TEST_DIR=/data/local/chromium_tests_root
REMOTE_TMP_DIR=/data/local/tmp

adb push --sync ${LOCAL_BUILD_DIR}/*  ${REMOTE_TMP_DIR}/

# Move test binaries up, replacing python invoker placeholders.
adb push ${LOCAL_BUILD_DIR}/pdfium_unittests__dist/pdfium_unittests ${REMOTE_TMP_DIR}/pdfium_unittests
adb push ${LOCAL_BUILD_DIR}/pdfium_embeddertests__dist/pdfium_embeddertests ${REMOTE_TMP_DIR}/pdfium_embeddertests

# Now move the test resources.
adb push --sync testing/resources ${REMOTE_TEST_DIR}/testing
adb push --sync third_party/NotoSansCJK ${REMOTE_TEST_DIR}/third_party
adb push --sync ${LOCAL_BUILD_DIR}/test_fonts ${REMOTE_TMP_DIR}

# Now run the tests
adb shell ${REMOTE_TMP_DIR}/pdfium_unittests
adb shell ${REMOTE_TMP_DIR}/pdfium_embeddertests
