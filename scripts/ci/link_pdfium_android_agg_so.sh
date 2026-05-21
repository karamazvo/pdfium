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
echo "=== Detect whether partition_alloc is linked ==="
# `-Wl,--wrap=realpath` and `-Wl,--wrap=getcwd` are needed only when
# partition_alloc's allocator_shim is linked -- it provides the
# corresponding `__wrap_realpath` / `__wrap_getcwd` symbols that
# intercept libc allocator-related calls.
#
# If partition_alloc is NOT in PROJECT_ARCHIVES_UNIQ (e.g. the build
# disabled `pdf_use_partition_alloc`), `--wrap` would cause the
# linker to redirect plain libc calls in libc++/icu into
# `__wrap_<name>` symbols that no archive provides -- link fails
# with "undefined __wrap_realpath".
#
# So make the --wrap flags conditional.
WRAP_FLAGS=()
if printf '%s\n' "${PROJECT_ARCHIVES_UNIQ[@]}" | grep -q 'liballocator_shim\.a'; then
  WRAP_FLAGS=(-Wl,--wrap=getcwd -Wl,--wrap=realpath)
  echo "partition_alloc liballocator_shim.a present -> using --wrap"
else
  echo "partition_alloc liballocator_shim.a NOT present -> dropping --wrap"
fi

echo
echo "=== Choose link mode ==="
# Two link strategies:
#
#   * BASELINE (default): -Wl,--whole-archive on libpdfium.a. Every
#     object stays in the final .so. Produces ~13.7 MB on a stock
#     pdfium build. Safe, no symbol-survival concerns.
#
#   * SHRINK (PDFIUM_AGG_SHRINK=1): drop --whole-archive, add
#     --gc-sections. The linker discards every section unreachable
#     from a kept root. To prevent dlopen-time UnsatisfiedLinkError
#     in the JNI consumer, we explicitly --undefined every public
#     FPDF/FORM/FSDK symbol defined by libpdfium.a so they all
#     survive. Produces ~6.6 MB -- equivalent to bblanchon's
#     reference build at 6.4 MB. Adds a post-link survival check
#     to catch any regression at CI time.
#
# Set PDFIUM_AGG_SHRINK=1 in the environment to opt into the shrink
# build (used by release-pdfium-android-arm64-agg.yml). Default
# leaves baseline behavior unchanged for any callers that still
# expect the larger but unconditionally-symbol-complete .so.
SHRINK_MODE="${PDFIUM_AGG_SHRINK:-0}"
echo "PDFIUM_AGG_SHRINK = $SHRINK_MODE"

UNDEFINED_FLAGS=(
  -Wl,--undefined=FPDF_InitLibrary
  -Wl,--undefined=FPDF_InitLibraryWithConfig
)

if [ "$SHRINK_MODE" = "1" ]; then
  echo
  echo "=== Build --undefined list from libpdfium.a (shrink mode) ==="
  # In shrink mode the linker GC's everything unreachable from kept
  # roots. We have to enumerate every public FPDF/FORM/FSDK symbol
  # defined in libpdfium.a so the JNI wrapper can still dlopen the
  # result. The list auto-regenerates on every build, so a future
  # PDFium upgrade that adds new public APIs picks them up
  # automatically -- no per-upgrade revalidation step.
  AUTO_UNDEFINED=()
  while IFS= read -r sym; do
    AUTO_UNDEFINED+=("-Wl,--undefined=$sym")
  done < <(
    "$LLVM_NM" -A --defined-only "$PDFIUM_A" 2>/dev/null \
      | awk '{print $NF}' \
      | grep -E '^(FPDF|FORM_|FSDK_)' \
      | sort -u
  )
  echo "Auto-discovered ${#AUTO_UNDEFINED[@]} FPDF_*/FORM_*/FSDK_* symbols to preserve"
  UNDEFINED_FLAGS=("${AUTO_UNDEFINED[@]}")
fi

echo
echo "=== Link final AGG libpdfium.so ==="

rm -f "$SO"

if [ "$SHRINK_MODE" = "1" ]; then
  # SHRINK link: no --whole-archive, +--gc-sections.
  "$CC" \
    --target="$TARGET" \
    --sysroot="$SYSROOT" \
    -shared \
    -o "$SO" \
    -nostdlib++ \
    --unwindlib=none \
    "$PDFIUM_A" \
    -Wl,--start-group \
      "${PROJECT_ARCHIVES_UNIQ[@]}" \
      "${LIBUNWIND_LINK_FLAGS[@]}" \
    -Wl,--end-group \
    "$BUILTINS" \
    -Wl,--no-undefined \
    -Wl,--gc-sections \
    -Wl,--export-dynamic-symbol=FPDF* \
    -Wl,--export-dynamic-symbol=FORM_* \
    "${UNDEFINED_FLAGS[@]}" \
    "${WRAP_FLAGS[@]}" \
    -Wl,-z,max-page-size=16384 \
    -llog -landroid -ldl -lm
else
  # BASELINE link: keep --whole-archive, no GC.
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
    "${UNDEFINED_FLAGS[@]}" \
    "${WRAP_FLAGS[@]}" \
    -Wl,-z,max-page-size=16384 \
    -llog -landroid -ldl -lm
fi

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

if [ "$SHRINK_MODE" = "1" ]; then
  echo
  echo "=== Shrink-mode survival check (24 canary symbols across all FPDF families) ==="
  # In shrink mode --gc-sections removes any symbol not reachable
  # from a kept root. The auto-built UNDEFINED_FLAGS list above
  # should preserve every FPDF/FORM/FSDK symbol defined in
  # libpdfium.a -- but if that discovery somehow misses an API,
  # the JNI consumer will fail at dlopen time with
  # UnsatisfiedLinkError. Catch that NOW with a representative
  # sample drawn from every public header family.
  MISSING_APIS=()
  for api in \
      FPDF_InitLibrary FPDF_InitLibraryWithConfig \
      FPDF_LoadDocument FPDF_LoadMemDocument \
      FPDF_RenderPageBitmap FPDF_RenderPageBitmapWithMatrix \
      FPDF_GetPageCount FPDF_LoadPage FPDF_ClosePage \
      FPDF_CloseDocument FPDF_DestroyLibrary \
      FPDFText_LoadPage FPDFText_CountChars FPDFText_GetCharBox \
      FPDFAnnot_GetSubtype FPDFAnnot_GetRect \
      FPDFLink_LoadWebLinks FPDFLink_GetURL \
      FPDFBookmark_GetFirstChild FPDFBookmark_GetTitle \
      FPDFDest_GetDestPageIndex \
      FPDFAction_GetType FPDFAction_GetURIPath \
      FPDF_GetMetaText FPDF_GetFileVersion \
      FPDFPage_GetRotation FPDFPage_HasTransparency \
      FPDFPageObj_GetType FPDFPage_CountObjects \
      FPDFImageObj_GetImagePixelSize \
      FPDF_GetPageSizeByIndex \
      FORM_OnLButtonDown FORM_OnKeyDown FORM_DoDocumentOpenAction \
      ; do
    if ! "$LLVM_NM" -D --defined-only -C "$SO" 2>/dev/null \
        | grep -E " T $api(\b|@)" >/dev/null; then
      MISSING_APIS+=("$api")
    fi
  done
  if [ "${#MISSING_APIS[@]}" -gt 0 ]; then
    echo "ERROR: shrink-mode link dropped these JNI-imported APIs:" >&2
    printf '  %s\n' "${MISSING_APIS[@]}" >&2
    echo "The auto-built --undefined list missed them. Either" >&2
    echo "  - libpdfium.a doesn't actually define them (unexpected)," >&2
    echo "  - llvm-nm's output format isn't what the awk filter expects, or" >&2
    echo "  - the symbol prefix changed in a new PDFium revision." >&2
    exit 1
  fi
  echo "OK: all canary APIs present."

  echo
  echo "=== Shrink-mode override survival check ==="
  # The 3 ship-patch overrides (CPDF_IndexedCS::TranslateImageLine,
  # CPDF_SeparationCS::TranslateImageLine, CPDF_DeviceNCS::TranslateImageLine)
  # are virtual-table entries. If --gc-sections drops a colorspace
  # subclass entirely, PDFium silently falls back to the slow
  # base-class path on every image that uses that colorspace -- a
  # functional REGRESSION (slow renders), NOT a build failure. Check
  # explicitly so we don't ship a binary that's secretly slow.
  MISSING_OVERRIDES=()
  for sym in 'CPDF_IndexedCS::TranslateImageLine' \
             'CPDF_SeparationCS::TranslateImageLine' \
             'CPDF_DeviceNCS::TranslateImageLine' \
             ; do
    if ! "$LLVM_NM" -C "$SO" 2>/dev/null \
        | grep -F "$sym" >/dev/null; then
      MISSING_OVERRIDES+=("$sym")
    fi
  done
  if [ "${#MISSING_OVERRIDES[@]}" -gt 0 ]; then
    echo "ERROR: shrink-mode --gc-sections dropped these ship-patch overrides:" >&2
    printf '  %s\n' "${MISSING_OVERRIDES[@]}" >&2
    echo "These are virtual-table entries; if they're gone the linker GC'd" >&2
    echo "their entire subclass and PDFium will fall back to the slow base" >&2
    echo "path. Add corresponding -Wl,--undefined entries to keep them alive." >&2
    exit 1
  fi
  echo "OK: all 3 ship-patch overrides present."
fi

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
