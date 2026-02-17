#!/bin/bash
# Copyright 2026 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# adb_push_test.sh: push files from a chromium checkout to android emulator.
#
# PRECONDITIONS:
#   Android emulator already configured on host.
#   Android binaries build into out/Android.
#   Script is run from top-level pdfium dirctory.

adb push --sync out/Android/*  /data/local/tmp/

# Move test binaries up, replacing python invoker placeholders.
adb push out/Android/pdfium_unittests__dist/pdfium_unittests /data/local/tmp/pdfium_unittests
adb push out/Android/pdfium_embeddertests__dist/pdfium_embeddertests /data/local/tmp/pdfium_embeddertests

# Now move the test resources.
adb push --sync testing/resources /data/local/chromium_tests_root/testing
adb push --sync third_party/NotoSansCJK/ /data/local/chromium_tests_root/third_party/NotoSansCJK
