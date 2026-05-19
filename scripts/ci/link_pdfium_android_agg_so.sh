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

# Sample what `gn desc <target> outputs` actually produces. The
# previous run had 237 PROJECT_TARGETS but 0 archives matched the
# case pattern -- something about the output format isn't what we
# expect.
echo
echo "=== Sample 'gn desc outputs' format (//:pdfium + first 5 archive targets) ==="
for t in //:pdfium \
         //core/fpdfdoc:fpdfdoc \
         //core/fpdftext:fpdftext \
         //core/fxcodec:fxcodec \
         //core/fxcrt:fxcrt \
         //core/fxge:fxge \
         ; do
  echo "--- $t ---"
  gn desc "$OUT" "$t" outputs 2>&1 | head -5
done

PROJECT_ARCHIVES=()
for t in "${PROJECT_TARGETS[@]}"; do
  while IFS= read -r out; do
    # gn desc outputs paths in 3 possible forms:
    #   //path/in/source/tree           (source files, ignore)
    #   //out/<builddir>/...            (build outputs)
    #   path/relative/to/builddir       (no // prefix, build outputs)
    # Normalize: strip a leading // if present, then strip a
    # `out/<builddir>/` prefix to leave a builddir-relative path.
    # We accept the entry if it ends in .a -- the archive format
    # check is more reliable than path matching.
    raw="$out"
    p="${raw#//}"
    # Accept any path that ends in .a (static archive).
    case "$p" in
      *.a)
        # Build a path relative to $OUT. p might already start with
        # $OUT/, in which case keep it. Otherwise prepend $OUT/.
        case "$p" in
          "$OUT"/*) PROJECT_ARCHIVES+=("$p") ;;
          *)        PROJECT_ARCHIVES+=("$OUT/$p") ;;
        esac
        ;;
    esac
  done < <(gn desc "$OUT" "$t" outputs 2>/dev/null || true)
done

mapfile -t PROJECT_ARCHIVES_UNIQ < <(
  if [ "${#PROJECT_ARCHIVES[@]}" -gt 0 ]; then
    printf '%s\n' "${PROJECT_ARCHIVES[@]}" \
      | sort -u \
      | grep -v "^${OUT}/obj/libpdfium\.a$"
  fi
)

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
echo "=== Collect shared-library runtime closure from GN ==="

mapfile -t RUNTIME_TARGETS < <(
  {
    echo "//build/config:shared_library_deps"
    gn desc "$OUT" //build/config:shared_library_deps deps --all
  } | sed '/^[[:space:]]*$/d' | sort -u
)

RUNTIME_INPUTS=()
for t in "${RUNTIME_TARGETS[@]}"; do
  while IFS= read -r out; do
    raw="$out"
    p="${raw#//}"
    # Accept any path that ends in .a or .o.
    case "$p" in
      *.a|*.o)
        case "$p" in
          "$OUT"/*) RUNTIME_INPUTS+=("$p") ;;
          *)        RUNTIME_INPUTS+=("$OUT/$p") ;;
        esac
        ;;
    esac
  done < <(gn desc "$OUT" "$t" outputs 2>/dev/null || true)
done

mapfile -t RUNTIME_INPUTS_UNIQ < <(
  if [ "${#RUNTIME_INPUTS[@]}" -gt 0 ]; then
    printf '%s\n' "${RUNTIME_INPUTS[@]}" | sort -u
  fi
)

echo "Runtime inputs:"
printf '  %s\n' "${RUNTIME_INPUTS_UNIQ[@]}"

echo
echo "=== Build missing runtime inputs ==="

MISSING_RUNTIME=()
for p in "${RUNTIME_INPUTS_UNIQ[@]}"; do
  if [ ! -f "$p" ]; then
    MISSING_RUNTIME+=("${p#$OUT/}")
  fi
done

if [ "${#MISSING_RUNTIME[@]}" -gt 0 ]; then
  printf 'Need to build runtime inputs:\n'
  printf '  %s\n' "${MISSING_RUNTIME[@]}"
  ninja -C "$OUT" "${MISSING_RUNTIME[@]}"
else
  echo "All runtime inputs already exist."
fi

echo
echo "=== Collect Chromium in-tree libunwind source-set objects ==="

# The libunwind layout has shifted over PDFium revisions. Search
# for the canonical location first, then fall back to a broader
# find. We accept any directory whose name contains "libunwind".
LIBUNWIND_DIR=""
for candidate in \
    "$OUT/obj/buildtools/third_party/libunwind/libunwind" \
    "$OUT/obj/third_party/libunwind/libunwind" \
    "$OUT/obj/buildtools/third_party/libunwind/libunwind_a" \
    ; do
  if [ -d "$candidate" ] && find "$candidate" -maxdepth 2 -name '*.o' | grep -q .; then
    LIBUNWIND_DIR="$candidate"
    break
  fi
done

# If still not found, try building one of several plausible target
# names and search again.
if [ -z "$LIBUNWIND_DIR" ]; then
  echo "libunwind not pre-built at any known path; attempting ninja build..."
  for tgt in \
      'obj/buildtools/third_party/libunwind/libunwind.stamp' \
      'obj/third_party/libunwind/libunwind.stamp' \
      '//buildtools/third_party/libunwind:libunwind' \
      ; do
    if ninja -C "$OUT" "$tgt" 2>/dev/null; then
      echo "  built target: $tgt"
      break
    fi
  done
  # Now search again across the whole obj tree.
  CANDIDATE_DIR="$(find "$OUT/obj" -type d -name 'libunwind*' 2>/dev/null \
    | while read -r d; do
        if find "$d" -maxdepth 2 -name '*.o' 2>/dev/null | grep -q .; then
          echo "$d"
          break
        fi
      done)"
  if [ -n "$CANDIDATE_DIR" ]; then
    LIBUNWIND_DIR="$CANDIDATE_DIR"
  fi
fi

if [ -z "$LIBUNWIND_DIR" ]; then
  echo "ERROR: could not locate libunwind .o files under $OUT/obj" >&2
  echo "Candidate dirs found:" >&2
  find "$OUT/obj" -type d -name 'libunwind*' 2>/dev/null | head -10 >&2
  exit 1
fi

echo "libunwind dir: $LIBUNWIND_DIR"

mapfile -t LIBUNWIND_OBJECTS < <(
  find "$LIBUNWIND_DIR" -maxdepth 2 -type f -name '*.o' | sort
)

if [ "${#LIBUNWIND_OBJECTS[@]}" -eq 0 ]; then
  echo "ERROR: $LIBUNWIND_DIR has no .o files" >&2
  exit 1
fi

echo "libunwind objects:"
printf '  %s\n' "${LIBUNWIND_OBJECTS[@]}"

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
    "${RUNTIME_INPUTS_UNIQ[@]}" \
    "${LIBUNWIND_OBJECTS[@]}" \
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
