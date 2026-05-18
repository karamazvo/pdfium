#!/usr/bin/env bash
#
# Apply 0003 + 0004 + 0005 + 0007 + 0008 + 0009 in order.
#
# 0009 adds the DeviceN fast override (table for N=1 / N=2, fall
# through for N>=3) plus diagnostic markers on every remaining slow-
# path-suspect colorspace. All new markers carry the `pdfium009.`
# prefix so a Perfetto trace makes it obvious whether the running
# .so is the 0009 build.
set -euo pipefail

PATCH_DOWNSCALE="patches/experiments/0003-jpeg-downscale-on-decode.patch"
PATCH_TRACE="patches/experiments/0004-native-trace-markers.patch"
PATCH_TRACE_DEEP="patches/experiments/0005-native-trace-markers-deep.patch"
PATCH_CS_DISPATCH="patches/experiments/0007-colorspace-dispatch-markers.patch"
PATCH_FAST_INDEXED="patches/experiments/0008-fast-indexed-separation-cs.patch"
PATCH_DEVICEN="patches/experiments/0009-devicen-fast-and-prefixed-markers.patch"

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

apply_one "0003 JPEG downscale-on-decode (v3)"            "$PATCH_DOWNSCALE"
echo
apply_one "0004 native ATrace markers (top-level)"        "$PATCH_TRACE"
echo
apply_one "0005 native ATrace markers (deep DIB)"         "$PATCH_TRACE_DEEP"
echo
apply_one "0007 colorspace-dispatch trace markers"        "$PATCH_CS_DISPATCH"
echo
apply_one "0008 fast IndexedCS + SeparationCS"            "$PATCH_FAST_INDEXED"
echo
apply_one "0009 DeviceN fast override + pdfium009.* markers" "$PATCH_DEVICEN"

echo
echo "=== Verify 0009 override + markers landed ==="

# DeviceN override implementation marker.
grep -n "CPDF_DeviceNCS::TranslateImageLine\|CPDF_DeviceNCS::BuildBgrLookupTableIfNeeded" \
  core/fpdfapi/page/cpdf_colorspace.cpp \
  | head -10

EXPECTED_PREFIXED_SLICES=(
  "pdfium009.DeviceNCS::TranslateImageLine"
  "pdfium009.CalGrayCS::TranslateImageLine"
  "pdfium009.CalRGBCS::TranslateImageLine"
  "pdfium009.LabCS::TranslateImageLine"
)

echo
echo "=== Confirm 0009 prefixed slice names present in source ==="
MISSING=()
for slice in "${EXPECTED_PREFIXED_SLICES[@]}"; do
  if ! grep -RFq "\"$slice\"" \
    core/fpdfapi/page/cpdf_colorspace.cpp \
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
echo "OK: 0003 + 0004 + 0005 + 0007 + 0008 + 0009 all applied successfully."
echo
echo "After vendoring this .so, the Perfetto trace MUST show slices"
echo "named pdfium009.*  -- if it doesn't, the running .so is older."
