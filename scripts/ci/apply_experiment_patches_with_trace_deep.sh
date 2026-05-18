#!/usr/bin/env bash
#
# Apply 0003 (JPEG downscale-on-decode), 0004 (top-level ATrace markers),
# AND 0005 (deep ATrace markers in DIB load + scanline path), in order.
#
# 0005 builds on 0003 + 0004 -- it adds markers in functions that 0004
# already references via includes/macros. The order is hard-coded.
set -euo pipefail

PATCH_DOWNSCALE="patches/experiments/0003-jpeg-downscale-on-decode.patch"
PATCH_TRACE="patches/experiments/0004-native-trace-markers.patch"
PATCH_TRACE_DEEP="patches/experiments/0005-native-trace-markers-deep.patch"

apply_one() {
  local name="$1"
  local path="$2"
  echo "=== Apply $name ==="
  if [ ! -f "$path" ]; then
    echo "ERROR: missing patch file: $path" >&2
    exit 1
  fi
  grep '^Subject:' "$path" || true
  if git apply --check "$path"; then
    echo "OK: $name applies cleanly."
  else
    echo "ERROR: $name does not apply cleanly." >&2
    grep '^diff --git ' "$path" || true
    exit 1
  fi
  git apply "$path"
}

apply_one "0003 JPEG downscale-on-decode (v3)" "$PATCH_DOWNSCALE"
echo
apply_one "0004 native ATrace markers (top-level)" "$PATCH_TRACE"
echo
apply_one "0005 native ATrace markers (deep -- DIB + scanline path)" "$PATCH_TRACE_DEEP"

echo
echo "=== Verify all three patches landed ==="

echo "--- 0003 markers ---"
grep -R "resolution_levels_to_skip" core/fxcodec/jpeg/jpegmodule.cpp | head -5

echo
echo "--- 0004 markers ---"
grep -R "XPDF_ATRACE_SCOPED\|XPDF_ATRACE_BEGIN" \
  core/fpdfapi/render/cpdf_renderstatus.cpp \
  core/fxcodec/jpeg/jpegmodule.cpp \
  fpdfsdk/fpdf_view.cpp \
  | head -10

echo
echo "--- 0005 markers (deep) ---"
grep -R "XPDF_ATRACE_SCOPED" \
  core/fpdfapi/page/cpdf_dib.cpp \
  core/fpdfapi/page/cpdf_pageimagecache.cpp \
  core/fxcodec/scanlinedecoder.cpp \
  core/fxge/dib/cfx_dibbase.cpp \
  | head -20

# Hard-validate every expected slice name lives in source after all patches.
EXPECTED_SLICES=(
  # 0004 markers
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
  # 0005 markers
  "pdfium.StartLoadDIBBase"
  "pdfium.LoadInternal"
  "pdfium.CreateDecoder"
  "pdfium.TranslateScanline24bpp"
  "pdfium.Entry::StartGetCachedBitmap"
  "pdfium.ContinueGetCachedBitmap"
  "pdfium.MakeCachedImage"
  "pdfium.ScanlineDecoder.GetScanline"
  "pdfium.DIBBase::ClipToInternal"
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
    core/fpdfapi/page/cpdf_dib.cpp \
    core/fxcodec/scanlinedecoder.cpp \
    core/fxge/dib/cfx_dibbase.cpp \
  ; then
    MISSING+=("$slice")
  else
    echo "  OK $slice"
  fi
done

if [ "${#MISSING[@]}" -gt 0 ]; then
  echo "ERROR: expected slice name(s) not found after patches:" >&2
  printf '  %s\n' "${MISSING[@]}" >&2
  exit 1
fi

echo
echo "=== Combined diff stat ==="
git diff --stat

echo
echo "OK: 0003 + 0004 + 0005 all applied successfully."
