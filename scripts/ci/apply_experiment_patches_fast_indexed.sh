#!/usr/bin/env bash
#
# Apply 0003 + 0004 + 0005 + 0007 + 0008 in order.
#
# 0008 is the FIX -- adds fast TranslateImageLine overrides for
# Indexed and Separation colorspaces. Bit-identical colors; ~50-200x
# speedup per scanline on indexed-image rendering.
#
# 0004/0005/0007 markers stay in the build so we can verify in
# Perfetto that pdfium.IndexedCS::TranslateImageLine fires instead
# of pdfium.ColorSpace::TranslateImageLine.baseSlow.
set -euo pipefail

PATCH_DOWNSCALE="patches/experiments/0003-jpeg-downscale-on-decode.patch"
PATCH_TRACE="patches/experiments/0004-native-trace-markers.patch"
PATCH_TRACE_DEEP="patches/experiments/0005-native-trace-markers-deep.patch"
PATCH_CS_DISPATCH="patches/experiments/0007-colorspace-dispatch-markers.patch"
PATCH_FAST_INDEXED="patches/experiments/0008-fast-indexed-separation-cs.patch"

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
apply_one "0008 fast IndexedCS + SeparationCS overrides"  "$PATCH_FAST_INDEXED"

echo
echo "=== Verify 0008 override declarations + impls landed ==="

grep -n "CPDF_IndexedCS::TranslateImageLine\|CPDF_SeparationCS::TranslateImageLine\|BuildBgrLookupTableIfNeeded\|bgr_lookup_" \
  core/fpdfapi/page/cpdf_indexedcs.h \
  core/fpdfapi/page/cpdf_indexedcs.cpp \
  core/fpdfapi/page/cpdf_colorspace.cpp \
  | head -20

EXPECTED_NEW_SLICES=(
  "pdfium.IndexedCS::TranslateImageLine"
  "pdfium.SeparationCS::TranslateImageLine"
)

echo
echo "=== Confirm new slice names present ==="
MISSING=()
for slice in "${EXPECTED_NEW_SLICES[@]}"; do
  if ! grep -RFq "\"$slice\"" \
    core/fpdfapi/page/cpdf_indexedcs.cpp \
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
echo "OK: 0003 + 0004 + 0005 + 0007 + 0008 all applied successfully."
echo
echo "Expected user-felt result on image-heavy indexed-color PDFs:"
echo "  - Render time drops from ~5s to ~50-200ms (~25-100x speedup)."
echo "  - Colors are bit-identical to the previous (slow) baseline."
echo "  - The trace marker pdfium.IndexedCS::TranslateImageLine"
echo "    should fire INSTEAD of pdfium.ColorSpace::TranslateImageLine.baseSlow"
echo "    on indexed images."
