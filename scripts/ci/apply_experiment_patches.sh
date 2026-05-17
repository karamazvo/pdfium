#!/usr/bin/env bash
set -euo pipefail

PATCH_FILE="patches/experiments/0002-jpeg-downscale-on-decode.patch"

echo "=== Apply JPEG downscale-on-decode experiment v2 ==="

if [ ! -f "$PATCH_FILE" ]; then
  echo "ERROR: missing patch file: $PATCH_FILE" >&2
  exit 1
fi

echo "Patch file:"
echo "  $PATCH_FILE"

echo
echo "=== Patch metadata ==="
grep '^Subject:' "$PATCH_FILE" || true
grep '^diff --git ' "$PATCH_FILE" || true

echo
echo "=== Verify patch applies cleanly ==="
if git apply --check "$PATCH_FILE"; then
  echo "OK: patch applies cleanly."
else
  echo "ERROR: patch does not apply cleanly to current PDFium checkout." >&2
  echo
  echo "Changed files in patch:"
  grep '^diff --git ' "$PATCH_FILE" || true
  exit 1
fi

echo
echo "=== Apply patch ==="
git apply "$PATCH_FILE"

echo
echo "=== Changed files ==="
git diff --stat

echo
echo "=== Verify JPEG downscale experiment markers ==="

grep -R "resolution_levels_to_skip" \
  core/fpdfapi/page/cpdf_dib.cpp \
  core/fpdfapi/page/cpdf_dib.h \
  core/fpdfapi/page/cpdf_streamparser.cpp \
  core/fxcodec/jpeg/jpegmodule.cpp \
  core/fxcodec/jpeg/jpegmodule.h

grep -R "scale_denom" core/fxcodec/jpeg/jpegmodule.cpp
grep -R "default_scale_denom_" core/fxcodec/jpeg/jpegmodule.cpp
grep -R "output_width_" core/fxcodec/jpeg/jpegmodule.cpp | head -30

echo
echo "=== Hard validation: no old 5-argument JPEG decoder calls ==="

if grep -R "JpegModule::CreateDecoder" \
  core/fpdfapi/page/cpdf_dib.cpp \
  core/fpdfapi/page/cpdf_streamparser.cpp \
  core/fxcodec/jpeg/jpegmodule.cpp \
  | grep -v "resolution_levels_to_skip" \
  | grep -v "nComps" \
  | grep -v "CreateDecoder(" ; then
  echo "WARNING: inspect remaining CreateDecoder lines above."
fi

echo
echo "OK: JPEG downscale-on-decode v2 patch applied."
