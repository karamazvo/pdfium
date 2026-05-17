#!/usr/bin/env bash
set -euo pipefail

PATCH_FILE="patches/experiments/0001-jpeg-downscale-on-decode.patch"

echo "=== Apply experimental performance patches ==="

if [ ! -f "$PATCH_FILE" ]; then
  echo "ERROR: missing patch file: $PATCH_FILE" >&2
  exit 1
fi

echo "Patch:"
echo "  $PATCH_FILE"

echo
echo "=== Verify patch can apply cleanly ==="

if git apply --check "$PATCH_FILE"; then
  echo "OK: patch applies cleanly."
else
  echo "ERROR: experiment patch does not apply cleanly to current PDFium checkout." >&2
  echo
  echo "Files touched by patch:"
  grep '^diff --git ' "$PATCH_FILE" || true
  exit 1
fi

echo
echo "=== Apply patch ==="
git apply "$PATCH_FILE"

echo
echo "=== Show patched files ==="
git diff --stat

echo
echo "=== Verify JPEG downscale symbols/text are present ==="
grep -R "scale_denom" core/fxcodec/jpeg/jpegmodule.cpp
grep -R "resolution_levels_to_skip" \
  core/fxcodec/jpeg/jpegmodule.cpp \
  core/fpdfapi/page/cpdf_dib.cpp \
  core/fpdfapi/page/cpdf_streamparser.cpp || true

echo
echo "OK: experimental JPEG downscale-on-decode patch applied."
