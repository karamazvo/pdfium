#!/usr/bin/env bash
#
# Apply BOTH the JPEG downscale-on-decode (v3) and the native ATrace
# markers (0004) experiment patches, in order. Modeled on
# `apply_experiment_patches.sh` which applies only 0003.
#
# This script is invoked by
# `.github/workflows/build-pdfium-android-arm64-agg-jpeg-downscale-trace.yml`
# in the karamazvo/pdfium fork's CI. It must be run with CWD set to the
# PDFium checkout root (where the `core/` and `fpdfsdk/` directories live).
#
# The 0003 patch must apply first because 0004's hunks in
# `core/fxcodec/jpeg/jpegmodule.cpp` overlap context-wise with 0003 --
# 0004 was authored on top of 0003 (verified locally with git apply --check
# against a v3-baseline-then-0004 sequence).
set -euo pipefail

PATCH_DOWNSCALE="patches/experiments/0003-jpeg-downscale-on-decode.patch"
PATCH_TRACE="patches/experiments/0004-native-trace-markers.patch"

echo "=== Apply 0003 JPEG downscale-on-decode (v3) ==="

if [ ! -f "$PATCH_DOWNSCALE" ]; then
  echo "ERROR: missing patch file: $PATCH_DOWNSCALE" >&2
  exit 1
fi

echo "Patch metadata:"
grep '^Subject:' "$PATCH_DOWNSCALE" || true

if git apply --check "$PATCH_DOWNSCALE"; then
  echo "OK: 0003 applies cleanly."
else
  echo "ERROR: 0003 does not apply cleanly to current PDFium checkout." >&2
  echo "Changed files in 0003:"
  grep '^diff --git ' "$PATCH_DOWNSCALE" || true
  exit 1
fi

git apply "$PATCH_DOWNSCALE"

echo
echo "=== Apply 0004 native ATrace markers ==="

if [ ! -f "$PATCH_TRACE" ]; then
  echo "ERROR: missing patch file: $PATCH_TRACE" >&2
  exit 1
fi

echo "Patch metadata:"
grep '^Subject:' "$PATCH_TRACE" || true

if git apply --check "$PATCH_TRACE"; then
  echo "OK: 0004 applies cleanly on top of 0003."
else
  echo "ERROR: 0004 does not apply cleanly. 0003 must be applied first." >&2
  echo "Changed files in 0004:"
  grep '^diff --git ' "$PATCH_TRACE" || true
  exit 1
fi

git apply "$PATCH_TRACE"

echo
echo "=== Verify both patches landed ==="

echo "--- 0003 markers (downscale logic) ---"
grep -R "resolution_levels_to_skip" \
  core/fpdfapi/page/cpdf_dib.cpp \
  core/fxcodec/jpeg/jpegmodule.cpp \
  | head -10

grep -R "default_scale_denom_" core/fxcodec/jpeg/jpegmodule.cpp | head -5

echo
echo "--- 0004 markers (ATrace) ---"

# The trace marker macros expand to ATrace_beginSection on Android.
# We grep for both the macro use AND the underlying API reference.
grep -R "XPDF_ATRACE_SCOPED\|XPDF_ATRACE_BEGIN\|ATrace_beginSection" \
  core/fpdfapi/render/cpdf_renderstatus.cpp \
  core/fxcodec/jpeg/jpegmodule.cpp \
  fpdfsdk/fpdf_view.cpp \
  core/fpdfapi/page/cpdf_pageimagecache.cpp \
  | head -20

# Sanity: every named pdfium.* slice should be present in source.
EXPECTED_SLICES=(
  "pdfium.FPDF_RenderPageBitmap"
  "pdfium.RenderObjectList"
  "pdfium.ProcessText"
  "pdfium.ProcessPath"
  "pdfium.ProcessImage"
  "pdfium.ProcessShading"
  "pdfium.ProcessForm"
  "pdfium.DrawObjWithBackground"
  "pdfium.ProcessTransparency"
  "pdfium.LoadSMask"
  "pdfium.StartGetCachedBitmap"
  "pdfium.JpegRewind"
)

echo
echo "--- Confirm every expected pdfium.* slice name is present in source ---"
MISSING=()
for slice in "${EXPECTED_SLICES[@]}"; do
  if ! grep -RFq "\"$slice\"" \
    core/fpdfapi/render/cpdf_renderstatus.cpp \
    core/fxcodec/jpeg/jpegmodule.cpp \
    fpdfsdk/fpdf_view.cpp \
    core/fpdfapi/page/cpdf_pageimagecache.cpp \
  ; then
    MISSING+=("$slice")
  else
    echo "  OK $slice"
  fi
done

if [ "${#MISSING[@]}" -gt 0 ]; then
  echo "ERROR: expected slice name(s) not found in source after patch:" >&2
  printf '  %s\n' "${MISSING[@]}" >&2
  exit 1
fi

echo
echo "=== Combined diff stat ==="
git diff --stat

echo
echo "OK: 0003 + 0004 both applied successfully."
