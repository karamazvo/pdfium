#!/bin/bash

# Convert hungarian style members to chromium style, one camel-case hump at a time.

FILES=`git grep -l '\bm_' *.h *.cc *.cpp`
sed -i 's/\bm_\(p\|b\|e\|n\|ws\|bs\)\([A-Z][a-z][a-z]*\)\b/\L\2_/g' $FILES
sed -i 's/\bm_\(p\|b\|e\|n\|ws\|bs\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\b/\L\2_\3_/g' $FILES
sed -i 's/\bm_\(p\|b\|e\|n\|ws\|bs\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\b/\L\2_\3_\4_/g' $FILES
sed -i 's/\bm_\(p\|b\|e\|n\|ws\|bs\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\b/\L\2_\3_\4_\5_/g' $FILES
sed -i 's/\bm_\(p\|b\|e\|n\|ws\|bs\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\([A-Z][a-z][a-z]*\)\b/\L\2_\3_\4_\5_\6_/g' $FILES

# Revert public headers and files depending only on them
git checkout -- public/
git checkout -- testing/embedder_test_environment.cpp

# Fixup individual cases of public header with m_ member
sed -i 's/m_pJsPlatform/m_pJsPlatform/g' `git grep -l m_pJsPlatform`
sed -i 's/m_pFormFillInfo/m_pFormFillInfo/g' `git grep -l m_pFormFillInfo`
sed -i 's/m_pUserFontPaths/m_pUserFontPaths/g' `git grep -l m_pUserFontPaths`

# m_pPlatform and m_pIsolate are trickier and need context
sed -i 's/\bconfig\(\.\|->\)platform_/config\1m_pPlatform/g' `git grep -l platform_`
sed -i 's/\bconfig\(\.\|->\)isolate_/config\1m_pIsolate/g' `git grep -l isolate_`
sed -i 's/\bconfig_\(\.\|->\)platform_/config_\1m_pPlatform/g' `git grep -l platform_`
sed -i 's/\bconfig_\(\.\|->\)isolate_/config_\1m_pIsolate/g' `git grep -l isolate_`
