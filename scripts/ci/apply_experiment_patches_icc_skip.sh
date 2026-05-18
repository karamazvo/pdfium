#!/usr/bin/env bash
#
# Apply 0003 + 0004 + 0005 + 0006 in order.
#
# 0006 is a diagnostic that short-circuits PDFium's per-pixel ICC color
# transform. Gated on `adb shell setprop debug.xpdf.skipIccTransform 1`.
# Colors will be visibly wrong on CMYK content when the flag is set --
# that is intentional. The whole point is to confirm whether ICC
# transform is the wall-time bottleneck.
set -euo pipefail

PATCH_DOWNSCALE="patches/experiments/0003-jpeg-downscale-on-decode.patch"
PATCH_TRACE="patches/experiments/0004-native-trace-markers.patch"
PATCH_TRACE_DEEP="patches/experiments/0005-native-trace-markers-deep.patch"
PATCH_ICC_SKIP="patches/experiments/0006-icc-skip-shortcut.patch"

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

apply_one "0003 JPEG downscale-on-decode (v3)"        "$PATCH_DOWNSCALE"
echo
apply_one "0004 native ATrace markers (top-level)"    "$PATCH_TRACE"
echo
apply_one "0005 native ATrace markers (deep)"         "$PATCH_TRACE_DEEP"
echo
apply_one "0006 ICC transform short-circuit"          "$PATCH_ICC_SKIP"

echo
echo "=== Verify the 0006 short-circuit landed ==="

# Should find the new XpdfSkipIccTransform helper + both short-circuit sites.
grep -n "XpdfSkipIccTransform\|debug.xpdf.skipIccTransform" \
  core/fpdfapi/page/cpdf_dib.cpp | head -10

echo
echo "Confirm both call sites short-circuit:"
sites=$(grep -c "if (XpdfSkipIccTransform())" core/fpdfapi/page/cpdf_dib.cpp || true)
if [ "$sites" -ne 2 ]; then
  echo "ERROR: expected 2 'if (XpdfSkipIccTransform())' sites, found $sites" >&2
  exit 1
fi
echo "  OK 2 short-circuit sites present"

echo
echo "=== Combined diff stat ==="
git diff --stat

echo
echo "OK: 0003 + 0004 + 0005 + 0006 all applied successfully."
echo
echo "RUNTIME USAGE:"
echo "  adb shell setprop debug.xpdf.skipIccTransform 1"
echo "  adb shell am force-stop com.xapper.pdf.reader"
echo "  (re-open the test PDF)"
echo
echo "  Expected: render times collapse on CMYK PDFs."
echo "  Expected: colors look visibly wrong (this is by design)."
