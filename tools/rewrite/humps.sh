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

DROP_PREFIX='\bm_\(p\|b\|e\|n\|i\|f\|dw\|ws\|bs\|cr\|sz\)'
LC_PREFIX='\bm_\([a-z]\+\)'
NO_PREFIX='\bm_'

HUMP='\([A-Z0-9]\+[a-z]\+\)'
LAST_HUMP='\([A-Z0-9]\+[a-z]*\)\b'
HUMP1="${LAST_HUMP}"
HUMP2="${HUMP}${HUMP1}"
HUMP3="${HUMP}${HUMP2}"
HUMP4="${HUMP}${HUMP3}"
HUMP5="${HUMP}${HUMP4}"

OUT1="\\L\\1_"
OUT12="${OUT1}\\2_"
OUT123="${OUT12}\\3_"
OUT1234="${OUT123}\\4_"
OUT12345="${OUT1234}\\5_"
OUT123456="${OUT12345}\\6_"

OUT2="\\L\\2_"
OUT23="${OUT2}\\3_"
OUT234="${OUT23}\\4_"
OUT2345="${OUT234}\\5_"
OUT23456="${OUT2345}\\6_"

sed -i "s/${DROP_PREFIX}${HUMP5}/${OUT23456}/g" $FILES
sed -i "s/${DROP_PREFIX}${HUMP4}/${OUT2345}/g" $FILES
sed -i "s/${DROP_PREFIX}${HUMP3}/${OUT234}/g" $FILES
sed -i "s/${DROP_PREFIX}${HUMP2}/${OUT23}/g" $FILES
sed -i "s/${DROP_PREFIX}${HUMP1}/${OUT2}/g" $FILES

sed -i "s/${LC_PREFIX}${HUMP5}/${OUT123456}/g" $FILES
sed -i "s/${LC_PREFIX}${HUMP4}/${OUT12345}/g" $FILES
sed -i "s/${LC_PREFIX}${HUMP3}/${OUT1234}/g" $FILES
sed -i "s/${LC_PREFIX}${HUMP2}/${OUT123}/g" $FILES
sed -i "s/${LC_PREFIX}${HUMP1}/${OUT12}/g" $FILES

sed -i "s/${NO_PREFIX}${HUMP5}/${OUT12345}/g" $FILES
sed -i "s/${NO_PREFIX}${HUMP4}/${OUT1234}/g" $FILES
sed -i "s/${NO_PREFIX}${HUMP3}/${OUT123}/g" $FILES
sed -i "s/${NO_PREFIX}${HUMP2}/${OUT12}/g" $FILES
sed -i "s/${NO_PREFIX}${HUMP1}/${OUT1}/g" $FILES

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
