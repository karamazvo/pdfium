#!/bin/bash
# Copyright 2025 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Convert hungarian style locals to chromium style, one camel-case hump at a time.

FILES=`git grep -l '\b[a-z]\+[A-Z]\w*\b' *.h *.cc *.cpp`

DROP_PREFIX='\b\(p\|b\|e\|n\|i\|f\|dw\|ws\|bs\|cr\|sz\)'

HUMP='\([A-Z0-9]\+[a-z]\+\)'
LAST_HUMP='\([A-Z0-9]\+[a-z]*\)\b'
HUMP1="${LAST_HUMP}"
HUMP2="${HUMP}${HUMP1}"
HUMP3="${HUMP}${HUMP2}"
HUMP4="${HUMP}${HUMP3}"
HUMP5="${HUMP}${HUMP4}"

OUT2="\\L\\2"
OUT23="${OUT2}_\\3"
OUT234="${OUT23}_\\4"
OUT2345="${OUT234}_\\5"
OUT23456="${OUT2345}_\\6"

sed -i "s/${DROP_PREFIX}${HUMP5}/${OUT23456}/g" $FILES
sed -i "s/${DROP_PREFIX}${HUMP4}/${OUT2345}/g" $FILES
sed -i "s/${DROP_PREFIX}${HUMP3}/${OUT234}/g" $FILES
sed -i "s/${DROP_PREFIX}${HUMP2}/${OUT23}/g" $FILES
sed -i "s/${DROP_PREFIX}${HUMP1}/${OUT2}/g" $FILES

# Revert files we didn't want to touch.
git checkout -- public/
git checkout -- third_party/
