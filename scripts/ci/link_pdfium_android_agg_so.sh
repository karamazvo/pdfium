#!/usr/bin/env bash
set -euo pipefail

# Allow callers to override the build dir (e.g. the size-experiment
# workflow builds into out/variant_baseline / out/variant_b / etc.).
# Default is the production release path.
OUT="${PDFIUM_AGG_BUILD_DIR:-out/android_arm64_agg}"
PDFIUM_A="$OUT/obj/libpdfium.a"
SO="$OUT/libpdfium.so"

CC="third_party/llvm-build/Release+Asserts/bin/clang"
TARGET="aarch64-linux-android29"
SYSROOT="third_party/android_toolchain/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
BUILTINS="third_party/llvm-build/Release+Asserts/lib/clang/23/lib/linux/libclang_rt.builtins-aarch64-android.a"
LLVM_NM="third_party/llvm-build/Release+Asserts/bin/llvm-nm"

test -f "$PDFIUM_A"
test -x "$CC"
test -d "$SYSROOT"
test -f "$BUILTINS"
test -x "$LLVM_NM"

echo "=== Collect AGG project static-library closure from GN ==="
echo "Build dir (OUT): $OUT"
echo "Build dir contents:"
ls -la "$OUT" 2>&1 | head -20

echo
echo "=== gn desc //:pdfium deps --all ==="
GN_DESC_RAW="$(gn desc "$OUT" //:pdfium deps --all 2>&1 || true)"
echo "$GN_DESC_RAW" | head -50
echo "  ... ($(echo "$GN_DESC_RAW" | wc -l) lines total)"

mapfile -t PROJECT_TARGETS < <(
  {
    echo "//:pdfium"
    gn desc "$OUT" //:pdfium deps --all 2>/dev/null || true
  } | sed '/^[[:space:]]*$/d' | sort -u
)

echo
echo "PROJECT_TARGETS (${#PROJECT_TARGETS[@]} entries):"
printf '  %s\n' "${PROJECT_TARGETS[@]:0:10}"
if [ "${#PROJECT_TARGETS[@]}" -gt 10 ]; then
  echo "  ... (${#PROJECT_TARGETS[@]} total)"
fi

# If gn desc gave us only `//:pdfium` itself (1 entry), something is
# wrong -- a non-trivial PDFium build has dozens of deps. Fail fast
# with diagnostic info so we don't silently produce a broken .so.
if [ "${#PROJECT_TARGETS[@]}" -lt 2 ]; then
  echo "ERROR: gn desc reported zero transitive deps for //:pdfium" >&2
  echo "       This means the build dir is in an unexpected state." >&2
  echo
  echo "Build dir tree (3 levels):"
  find "$OUT" -maxdepth 3 -type d 2>/dev/null | head -30
  echo
  echo "gn args (effective):"
  gn args "$OUT" --list --short 2>&1 | head -30 || true
  echo
  echo "Direct gn desc invocation:"
  gn desc "$OUT" //:pdfium 2>&1 | head -30 || true
  exit 1
fi

# Discover static archives directly from the filesystem.
#
# History: this used to use `gn desc <target> outputs` to walk the
# dependency graph. That worked when most PDFium subdirs built
# static_library targets, but modern PDFium has refactored most of
# them to `source_set` -- and `gn desc <source_set> outputs` errors
# with "Don't know how to display outputs for source_set", because
# a source_set rolls into its parent's archive instead of producing
# its own .a.
#
# The simpler and more robust approach: find every .a file the build
# system actually produced under $OUT/obj, then exclude libpdfium.a
# itself (we add that separately via --whole-archive). This catches
# everything ninja actually built: third-party deps like
# libfreetype.a, libabsl_*.a, libjpeg.a, lcms2.a, libopenjpeg2.a,
# partition_alloc.a, zlib.a, plus anything else still emitted as a
# static_library.
mapfile -t PROJECT_ARCHIVES_UNIQ < <(
  find "$OUT/obj" -type f -name '*.a' 2>/dev/null \
    | grep -v "^${OUT}/obj/libpdfium\.a$" \
    | sort -u
)

echo
echo "Discovered project archives ($((${#PROJECT_ARCHIVES_UNIQ[@]})) files):"
if [ "${#PROJECT_ARCHIVES_UNIQ[@]}" -gt 0 ]; then
  printf '  %s\n' "${PROJECT_ARCHIVES_UNIQ[@]:0:15}"
  if [ "${#PROJECT_ARCHIVES_UNIQ[@]}" -gt 15 ]; then
    echo "  ... ($((${#PROJECT_ARCHIVES_UNIQ[@]} - 15)) more)"
  fi
else
  echo "  (none)"
fi

echo "PDFium archive:"
echo "  $PDFIUM_A"
echo
echo "Project archives:"
printf '  %s\n' "${PROJECT_ARCHIVES_UNIQ[@]}"

echo
echo "=== Verify this is not a Skia build ==="

if printf '%s\n' "${PROJECT_ARCHIVES_UNIQ[@]}" | grep -q '/skia/libskia\.a'; then
  echo "ERROR: AGG variant unexpectedly includes libskia.a" >&2
  exit 1
fi

if "$LLVM_NM" -A -C "$PDFIUM_A" 2>/dev/null | grep -E 'SkTypeface_proxy|SkFontMgr_New_Custom_Empty' >/dev/null; then
  echo "WARNING: Skia font-manager symbols found in AGG PDFium archive."
  echo "This may indicate args are not pure AGG."
fi

echo
echo "=== Shared-library runtime closure ==="
# Previously this was a separate `gn desc //build/config:shared_library_deps`
# discovery walk. Same source_set problem as PROJECT_ARCHIVES: most
# runtime targets are source_sets now. The filesystem-based discovery
# above already captures every .a file under $OUT/obj, which includes
# libc++.a, libc++abi.a, libabsl_*.a, partition_alloc.a, and every
# other runtime dependency. So we don't need a second discovery loop.
echo "Folded into PROJECT_ARCHIVES_UNIQ above."
echo "All static archives (.a) under $OUT/obj are included via"
echo "--start-group / --end-group at link time."

echo
echo "=== Resolve C++ runtime + libunwind archives ==="
#
# Why we resolve these manually:
#   This script links with `-nostdlib++` and `--unwindlib=none`, which
#   tells clang NOT to auto-link libc++/libc++abi/libunwind. That's
#   because Chromium's in-tree libc++ supplies a custom allocator
#   wired into partition_alloc, and we want to use that exact one
#   (not the NDK sysroot's libc++). For libunwind, modern PDFium has
#   decoupled libunwind from //:pdfium so we have to build it ourselves
#   or use a prebuilt.
#
# The most robust approach: read the .ninja files Chromium GENERATED
# for these source_sets and force-ninja-build their declared .o
# outputs. Works on every PDFium revision because GN regenerates the
# .ninja files for the current source tree.

RUNTIME_RESOLUTION_TARGETS=()
NINJA_RULE_FILES=(
  # libunwind
  "$OUT/obj/buildtools/third_party/libunwind/libunwind.ninja"
  "$OUT/obj/third_party/libunwind/libunwind.ninja"
  # libc++
  "$OUT/obj/buildtools/third_party/libc++/libc++.ninja"
  "$OUT/obj/third_party/libc++/libc++.ninja"
  # libc++abi
  "$OUT/obj/buildtools/third_party/libc++abi/libc++abi.ninja"
  "$OUT/obj/third_party/libc++abi/libc++abi.ninja"
)

for nf in "${NINJA_RULE_FILES[@]}"; do
  [ -f "$nf" ] || continue
  echo "Reading $nf:"
  # Parse `build <output>: rule deps...` lines. We want the output
  # paths that end in .o.
  while IFS= read -r tgt; do
    [ -z "$tgt" ] && continue
    RUNTIME_RESOLUTION_TARGETS+=("$tgt")
  done < <(
    grep -E '^build [^:]+\.o:' "$nf" \
      | sed -E 's/^build ([^:]+\.o):.*/\1/'
  )
done

# De-dupe.
if [ "${#RUNTIME_RESOLUTION_TARGETS[@]}" -gt 0 ]; then
  mapfile -t RUNTIME_RESOLUTION_TARGETS < <(
    printf '%s\n' "${RUNTIME_RESOLUTION_TARGETS[@]}" | sort -u
  )
fi

echo "Total runtime .o targets to build: ${#RUNTIME_RESOLUTION_TARGETS[@]}"
if [ "${#RUNTIME_RESOLUTION_TARGETS[@]}" -gt 0 ]; then
  echo "Sample:"
  printf '  %s\n' "${RUNTIME_RESOLUTION_TARGETS[@]:0:8}"
fi

# Force ninja to build them all in one invocation (much faster than
# per-target, and avoids redundant graph traversal).
if [ "${#RUNTIME_RESOLUTION_TARGETS[@]}" -gt 0 ]; then
  echo "Force-building C++ runtime + libunwind .o files..."
  ninja -C "$OUT" "${RUNTIME_RESOLUTION_TARGETS[@]}" 2>&1 | tail -10 || true
fi

# Collect the built .o files. Filter by directory name so we know
# which library each one came from (useful for diagnostics if the
# link still fails).
mapfile -t LIBUNWIND_OBJECTS < <(
  find "$OUT/obj" -type f -name '*.o' 2>/dev/null | grep '/libunwind/' | sort -u
)
mapfile -t LIBCXX_OBJECTS < <(
  find "$OUT/obj" -type f -name '*.o' 2>/dev/null | grep '/libc++/' | grep -v 'libc++abi' | sort -u
)
mapfile -t LIBCXXABI_OBJECTS < <(
  find "$OUT/obj" -type f -name '*.o' 2>/dev/null | grep '/libc++abi/' | sort -u
)

echo "Built libunwind objects: ${#LIBUNWIND_OBJECTS[@]}"
echo "Built libc++    objects: ${#LIBCXX_OBJECTS[@]}"
echo "Built libc++abi objects: ${#LIBCXXABI_OBJECTS[@]}"

if [ "${#LIBUNWIND_OBJECTS[@]}" -eq 0 ] \
   || [ "${#LIBCXX_OBJECTS[@]}" -eq 0 ] \
   || [ "${#LIBCXXABI_OBJECTS[@]}" -eq 0 ]; then
  echo "ERROR: missing C++ runtime or libunwind objects" >&2
  echo
  echo ".ninja files available under $OUT/obj/buildtools (3 levels):" >&2
  find "$OUT/obj/buildtools" -maxdepth 4 2>/dev/null | head -40 | sed 's|^|  |' >&2
  exit 1
fi

# Combine. Order matters less inside --start-group, but keep grouped
# for readability.
LIBUNWIND_LINK_FLAGS=("${LIBUNWIND_OBJECTS[@]}" "${LIBCXX_OBJECTS[@]}" "${LIBCXXABI_OBJECTS[@]}")
echo "Total runtime .o files to link: ${#LIBUNWIND_LINK_FLAGS[@]}"

echo
echo "=== Link final AGG libpdfium.so ==="

rm -f "$SO"

"$CC" \
  --target="$TARGET" \
  --sysroot="$SYSROOT" \
  -shared \
  -o "$SO" \
  -nostdlib++ \
  --unwindlib=none \
  -Wl,--whole-archive \
    "$PDFIUM_A" \
  -Wl,--no-whole-archive \
  -Wl,--start-group \
    "${PROJECT_ARCHIVES_UNIQ[@]}" \
    "${LIBUNWIND_LINK_FLAGS[@]}" \
  -Wl,--end-group \
  "$BUILTINS" \
  -Wl,--no-undefined \
  -Wl,--export-dynamic-symbol=FPDF* \
  -Wl,--export-dynamic-symbol=FORM_* \
  -Wl,--undefined=FPDF_InitLibrary \
  -Wl,--undefined=FPDF_InitLibraryWithConfig \
  -Wl,--wrap=getcwd \
  -Wl,--wrap=realpath \
  -Wl,-z,max-page-size=16384 \
  -llog -landroid -ldl -lm

echo
echo "=== Final AGG shared library ==="
ls -lh "$SO"
file "$SO"

echo
echo "=== Validate final .so: no known-bad unresolved symbols ==="

if "$LLVM_NM" -D -C "$SO" 2>/dev/null \
  | grep ' U ' \
  | grep -E 'SkTypeface_proxy|SkFontMgr_New_Custom_Empty|SkFontMgr_Android_Parser|SkLanguage::getParent|__real_|std::get_new_handler|__cxa_guard_|_Unwind_'; then
  echo "ERROR: unresolved known-bad symbols remain in final AGG libpdfium.so" >&2
  exit 1
fi

echo "OK: no known-bad unresolved symbols."

echo
echo "=== Validate exported PDFium APIs ==="

if "$LLVM_NM" -D --defined-only -C "$SO" 2>/dev/null \
  | grep -E ' (FPDF_InitLibrary|FPDF_InitLibraryWithConfig|FPDF_LoadDocument|FPDF_RenderPageBitmap)' >/dev/null; then
  "$LLVM_NM" -D --defined-only -C "$SO" 2>/dev/null \
    | grep -E ' (FPDF_InitLibrary|FPDF_InitLibraryWithConfig|FPDF_LoadDocument|FPDF_RenderPageBitmap)' \
    | head -20
else
  echo "ERROR: expected FPDF_* APIs are not exported." >&2
  exit 1
fi

echo
echo "=== Dynamic dependencies ==="
readelf -d "$SO" | grep NEEDED || true
