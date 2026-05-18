#!/usr/bin/env bash
#
# Apply 0003 + 0004 + 0005 + 0007 in order.
#
# 0007 adds colorspace-dispatch trace markers (CPDF_ColorSpace and
# CPDF_ICCBasedCS and CPDF_DeviceCS TranslateImageLine paths) to
# pinpoint which inner branch of TranslateScanline24bpp consumes the
# observed 1.4 ms/scanline wall time.
#
# 0006 (the ICC bypass diagnostic) is intentionally NOT applied here --
# it was a one-off diagnostic that produced wrong colors. This build is
# pure observability + no-op for correctness.
set -euo pipefail

PATCH_DOWNSCALE="patches/experiments/0003-jpeg-downscale-on-decode.patch"
PATCH_TRACE="patches/experiments/0004-native-trace-markers.patch"
PATCH_TRACE_DEEP="patches/experiments/0005-native-trace-markers-deep.patch"
PATCH_CS_DISPATCH="patches/experiments/0007-colorspace-dispatch-markers.patch"

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

apply_one "0003 JPEG downscale-on-decode (v3)"           "$PATCH_DOWNSCALE"
echo
apply_one "0004 native ATrace markers (top-level)"       "$PATCH_TRACE"
echo
apply_one "0005 native ATrace markers (deep DIB)"        "$PATCH_TRACE_DEEP"
echo
apply_one "0007 colorspace-dispatch trace markers"       "$PATCH_CS_DISPATCH"

echo
echo "=== Verify expected 0007 slice names landed ==="

EXPECTED_NEW_SLICES=(
  "pdfium.ColorSpace::TranslateImageLine.baseSlow"
  "pdfium.ICCBasedCS::TranslateImageLine"
  "pdfium.ICCBased.sRGBfast"
  "pdfium.ICCBased.baseFallback"
  "pdfium.ICCBased.profileTranslate"
  "pdfium.ICCBased.cachePath"
  "pdfium.ICCBased.cacheBuild"
  "pdfium.DeviceCS::TranslateImageLine"
)

MISSING=()
for slice in "${EXPECTED_NEW_SLICES[@]}"; do
  if ! grep -RFq "\"$slice\"" \
    core/fpdfapi/page/cpdf_colorspace.cpp \
    core/fpdfapi/page/cpdf_devicecs.cpp \
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
echo "OK: 0003 + 0004 + 0005 + 0007 all applied successfully."
echo
echo "Capture a trace with debug.xpdf.trace=1; expand a slow"
echo "pdfium.TranslateScanline24bpp slice. Exactly one of the new"
echo "pdfium.ICCBased.* / pdfium.DeviceCS.* / .baseSlow slices will"
echo "be the dominant child."
