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
echo "=== Resolve libunwind for link ==="

# History: this script previously linked Chromium's in-tree libunwind
# source_set as a pile of .o files. That worked when PDFium's `pdfium`
# library target depended on libunwind. Modern PDFium has decoupled
# them -- libunwind is now only a dep of test binaries like
# pdfium_test, not of //:pdfium. So when we build `ninja -C $OUT pdfium`
# nothing builds libunwind, and our discovery returns 0 .o files.
#
# The correct fix: use the libunwind that ships with the clang/NDK
# toolchain. clang's resource directory contains a prebuilt
# libunwind-<arch>-android.a; the NDK sysroot also has a
# libunwind.a in usr/lib/<triple>/<api>/.
#
# Locate one and link it via -l. Fall back to building Chromium's
# in-tree libunwind if no prebuilt is found.

LIBUNWIND_LINK_FLAGS=()

# Candidate 1: clang's compiler-rt prebuilt libunwind.
CLANG_LIB_DIR="$(dirname "$BUILTINS")"
echo "Searching clang resource dir: $CLANG_LIB_DIR"
CLANG_LIBUNWIND="$(find "$CLANG_LIB_DIR" -maxdepth 1 -type f -name 'libunwind*aarch64*android*.a' 2>/dev/null | head -1)"
if [ -z "$CLANG_LIBUNWIND" ]; then
  CLANG_LIBUNWIND="$(find "$CLANG_LIB_DIR" -maxdepth 1 -type f -name 'libunwind*.a' 2>/dev/null | head -1)"
fi

# Candidate 2: NDK sysroot libunwind for android29 / aarch64.
SYSROOT_LIB="$SYSROOT/usr/lib/aarch64-linux-android"
NDK_LIBUNWIND="$(find "$SYSROOT_LIB" -maxdepth 3 -type f -name 'libunwind.a' 2>/dev/null | head -1)"

# Candidate 3: NDK lib root, anywhere.
if [ -z "$NDK_LIBUNWIND" ]; then
  NDK_LIBUNWIND="$(find "$SYSROOT/usr/lib" -type f -name 'libunwind*.a' 2>/dev/null \
                    | grep -E 'aarch64' | head -1 || true)"
fi

echo "Candidate libunwind archives:"
echo "  clang compiler-rt: ${CLANG_LIBUNWIND:-(not found)}"
echo "  NDK sysroot:       ${NDK_LIBUNWIND:-(not found)}"

if [ -n "$CLANG_LIBUNWIND" ]; then
  LIBUNWIND_LINK_FLAGS=("$CLANG_LIBUNWIND")
  echo "Using clang compiler-rt libunwind: $CLANG_LIBUNWIND"
elif [ -n "$NDK_LIBUNWIND" ]; then
  LIBUNWIND_LINK_FLAGS=("$NDK_LIBUNWIND")
  echo "Using NDK sysroot libunwind: $NDK_LIBUNWIND"
else
  # Last-ditch: try to force-build Chromium's in-tree libunwind.
  # This is unlikely to succeed because nothing links libunwind in
  # the AGG build, but try in case the rules can produce .o files
  # standalone.
  echo "No prebuilt libunwind archive found in toolchain or sysroot."
  echo "Trying to force-build Chromium's in-tree libunwind objects..."
  if [ -f "$OUT/obj/buildtools/third_party/libunwind/libunwind.ninja" ]; then
    # Read the .ninja file's `build` lines to learn what outputs it
    # produces, then ask the main ninja to build them.
    NINJA_OUTPUTS="$(grep -E '^build ' "$OUT/obj/buildtools/third_party/libunwind/libunwind.ninja" \
      | sed -E 's/^build ([^:]+):.*/\1/' \
      | tr ' ' '\n' \
      | grep -E '\.o$' \
      | head -30 || true)"
    if [ -n "$NINJA_OUTPUTS" ]; then
      echo "Outputs declared by libunwind.ninja:"
      echo "$NINJA_OUTPUTS" | head -10 | sed 's|^|  |'
      while IFS= read -r tgt; do
        [ -z "$tgt" ] && continue
        ninja -C "$OUT" "$tgt" 2>&1 | tail -5 || true
      done <<< "$NINJA_OUTPUTS"
    fi
  fi
  mapfile -t LIBUNWIND_OBJECTS < <(
    find "$OUT/obj" -type f -name '*.o' 2>/dev/null | grep '/libunwind' | sort -u
  )
  if [ "${#LIBUNWIND_OBJECTS[@]}" -gt 0 ]; then
    echo "Built ${#LIBUNWIND_OBJECTS[@]} libunwind .o files from Chromium in-tree libunwind"
    LIBUNWIND_LINK_FLAGS=("${LIBUNWIND_OBJECTS[@]}")
  else
    echo "ERROR: no libunwind archive available" >&2
    echo
    echo "All .a files under clang resource dir:" >&2
    find "$CLANG_LIB_DIR" -maxdepth 2 -type f -name '*.a' 2>/dev/null | head -30 | sed 's|^|  |' >&2
    echo
    echo "All libunwind* under NDK sysroot:" >&2
    find "$SYSROOT" -type f -name 'libunwind*' 2>/dev/null | head -10 | sed 's|^|  |' >&2
    exit 1
  fi
fi

echo "libunwind link inputs (${#LIBUNWIND_LINK_FLAGS[@]}):"
printf '  %s\n' "${LIBUNWIND_LINK_FLAGS[@]:0:5}"

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
