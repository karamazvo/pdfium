#!/bin/bash
# Copyright 2025 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Convert hungarian style members to chromium style, one camel-case hump at a time.

FILES=`git grep -l '\bm_' *.h *.cc *.cpp`

# Preserve public members by renaming out of the way
sed -i 's/m_FileLen\b/__m_FileLen/g'               `git grep -l m_FileLen`
sed -i 's/m_GetBlock\b/__m_GetBlock/g'             `git grep -l m_FileLen`
sed -i 's/m_Param\b/__m_Param/g'                   `git grep -l m_Param`
sed -i 's/m_RendererType\b/__m_RendererType/g'     `git grep -l m_RendererType`
sed -i 's/m_isolate\b/__m_isolate/g'               `git grep -l m_isolate`
sed -i 's/m_pFormfillinfo\b/__m_pFormfillinfo/g'   `git grep -l m_pFormfillinfo`
sed -i 's/m_pIsolate\b/__m_pIsolate/g'             `git grep -l m_pIsolate`
sed -i 's/m_pJsPlatform\b/__m_pJsPlatform/g'       `git grep -l m_pJsPlatform`
sed -i 's/m_pPlatform\b/__m_pPlatform/g'           `git grep -l m_pPlatform`
sed -i 's/m_pUserFontPaths\b/__m_pUserFontPaths/g' `git grep -l m_pUserFontPaths`
sed -i 's/m_v8EmbedderSlot\b/__m_v8EmbedderSlot/g' `git grep -l m_v8EmbedderSlot`

PREFIX='\bm_\(p\|b\|e\|n\|i\|f\|dw\|ws\|bs\|cr\|sz\)'
LC_PREFIX='\bm_\([a-z]\+\)'
NO_PREFIX='\bm_'
FIRST_HUMP='\([A-Z0-9]\+[a-z]\+\)'
LAST_HUMP='\([A-Z0-9]\+[a-z]*\)\b'
MID_HUMP='\([A-Z0-9]\+[a-z]\+\)'

sed -i "s/${PREFIX}${FIRST_HUMP}${MID_HUMP}${MID_HUMP}${MID_HUMP}${LAST_HUMP}/\\L\\2_\\3_\\4_\\5_\\6_/g" $FILES
sed -i "s/${PREFIX}${FIRST_HUMP}${MID_HUMP}${MID_HUMP}${LAST_HUMP}/\\L\\2_\\3_\\4_\\5_/g" $FILES
sed -i "s/${PREFIX}${FIRST_HUMP}${MID_HUMP}${LAST_HUMP}/\\L\\2_\\3_\\4_/g" $FILES
sed -i "s/${PREFIX}${FIRST_HUMP}${LAST_HUMP}/\\L\\2_\\3_/g" $FILES
sed -i "s/${PREFIX}${LAST_HUMP}/\\L\\2_/g" $FILES

sed -i "s/${LC_PREFIX}${FIRST_HUMP}${MID_HUMP}${MID_HUMP}${MID_HUMP}${LAST_HUMP}/\\L\\1_\\2_\\3_\\4_\\5_\\6_/g" $FILES
sed -i "s/${LC_PREFIX}${FIRST_HUMP}${MID_HUMP}${MID_HUMP}${LAST_HUMP}/\\L\\1_\\2_\\3_\\4_\\5_/g" $FILES
sed -i "s/${LC_PREFIX}${FIRST_HUMP}${MID_HUMP}${LAST_HUMP}/\\L\\1_\\2_\\3_\\4_/g" $FILES
sed -i "s/${LC_PREFIX}${FIRST_HUMP}${LAST_HUMP}/\\L\\1_\\2_\\3_/g" $FILES
sed -i "s/${LC_PREFIX}${LAST_HUMP}/\\L\\1_\\2_/g" $FILES

sed -i "s/${NO_PREFIX}${FIRST_HUMP}${MID_HUMP}${MID_HUMP}${MID_HUMP}${LAST_HUMP}/\\L\\1_\\2_\\3_\\4_\\5_/g" $FILES
sed -i "s/${NO_PREFIX}${FIRST_HUMP}${MID_HUMP}${MID_HUMP}${LAST_HUMP}/\\L\\1_\\2_\\3_\\4_/g" $FILES
sed -i "s/${NO_PREFIX}${FIRST_HUMP}${MID_HUMP}${LAST_HUMP}/\\L\\1_\\2_\\3_/g" $FILES
sed -i "s/${NO_PREFIX}${FIRST_HUMP}${LAST_HUMP}/\\L\\1_\\2_/g" $FILES
sed -i "s/${NO_PREFIX}${LAST_HUMP}/\\L\\1_/g" $FILES

sed -i 's/\bm_\(\w\+\)\b/\L\1_/g' $FILES

# Revert files we didn't want to touch.
git checkout -- public/
git checkout -- third_party/

# Restore public members renamed out of the way
sed -i 's/__m_FileLen/m_FileLen/g'               `git grep -l m_FileLen`
sed -i 's/__m_GetBlock/m_GetBlock/g'             `git grep -l m_FileLen`
sed -i 's/__m_Param/m_Param/g'                   `git grep -l m_Param`
sed -i 's/__m_RendererType/m_RendererType/g'     `git grep -l m_RendererType`
sed -i 's/__m_isolate/m_isolate/g'               `git grep -l m_isolate`
sed -i 's/__m_pFormfillinfo/m_pFormfillinfo/g'   `git grep -l m_pFormfillinfo`
sed -i 's/__m_pIsolate/m_pIsolate/g'             `git grep -l m_pIsolate`
sed -i 's/__m_pJsPlatform/m_pJsPlatform/g'       `git grep -l m_pJsPlatform`
sed -i 's/__m_pPlatform/m_pPlatform/g'           `git grep -l m_pPlatform`
sed -i 's/__m_pUserFontPaths/m_pUserFontPaths/g' `git grep -l m_pUserFontPaths`
sed -i 's/__m_v8EmbedderSlot/m_v8EmbedderSlot/g' `git grep -l m_v8EmbedderSlot`
