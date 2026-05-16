#!/usr/bin/env bash
set -euo pipefail

OUT=out/android_arm64
PDFIUM_A="$OUT/obj/libpdfium.a"
SO="$OUT/libpdfium.so"

CC="third_party/llvm-build/Release+Asserts/bin/clang"
TARGET="aarch64-linux-android29"
SYSROOT="third_party/android_toolchain/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
BUILTINS="third_party/llvm-build/Release+Asserts/lib/clang/23/lib/linux/libclang_rt.builtins-aarch64-android.a"

test -f "$PDFIUM_A"
test -x "$CC"
test -d "$SYSROOT"
test -f "$BUILTINS"

echo "=== Collect project static-library closure from GN ==="

mapfile -t PROJECT_TARGETS < <(
  {
    echo "//:pdfium"
    echo "//skia:skia"
    gn desc "$OUT" //:pdfium deps --all
    gn desc "$OUT" //skia:skia deps --all
  } | sed '/^[[:space:]]*$/d' | sort -u
)

PROJECT_ARCHIVES=()
for t in "${PROJECT_TARGETS[@]}"; do
  while IFS= read -r out; do
    p="${out#//}"
    case "$p" in
      out/android_arm64/*.a)
        PROJECT_ARCHIVES+=("$p")
        ;;
    esac
  done < <(gn desc "$OUT" "$t" outputs 2>/dev/null || true)
done

mapfile -t PROJECT_ARCHIVES_UNIQ < <(
  printf '%s\n' "${PROJECT_ARCHIVES[@]}" \
    | sort -u \
    | grep -v '^out/android_arm64/obj/libpdfium\.a$'
)

echo "PDFium archive:"
echo "  $PDFIUM_A"
echo
echo "Project archives:"
printf '  %s\n' "${PROJECT_ARCHIVES_UNIQ[@]}"

echo
echo "=== Sanity: required Skia providers inside libskia.a ==="

LLVM_NM="third_party/llvm-build/Release+Asserts/bin/llvm-nm"
SKIA_A="$OUT/obj/skia/libskia.a"

"$LLVM_NM" -A -C "$SKIA_A" 2>/dev/null \
  | grep -E ' [TWDV] (SkFontMgr_New_Custom_Empty\(\)|vtable for SkTypeface_proxy)$' \
  || {
    echo "ERROR: libskia.a does not contain required Android Skia providers." >&2
    echo "Check gn desc out/android_arm64 //skia sources." >&2
    exit 1
  }

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
    p="${out#//}"
    case "$p" in
      out/android_arm64/*.a|out/android_arm64/*.o)
        RUNTIME_INPUTS+=("$p")
        ;;
    esac
  done < <(gn desc "$OUT" "$t" outputs 2>/dev/null || true)
done

mapfile -t RUNTIME_INPUTS_UNIQ < <(
  printf '%s\n' "${RUNTIME_INPUTS[@]}" | sort -u
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

LIBUNWIND_DIR="$OUT/obj/buildtools/third_party/libunwind/libunwind"

if [ ! -d "$LIBUNWIND_DIR" ] || ! find "$LIBUNWIND_DIR" -maxdepth 1 -name '*.o' | grep -q .; then
  ninja -C "$OUT" obj/buildtools/third_party/libunwind/libunwind.stamp
fi

mapfile -t LIBUNWIND_OBJECTS < <(
  find "$LIBUNWIND_DIR" -maxdepth 1 -type f -name '*.o' | sort
)

if [ "${#LIBUNWIND_OBJECTS[@]}" -eq 0 ]; then
  echo "ERROR: no Chromium libunwind objects found" >&2
  exit 1
fi

echo
echo "=== Link final libpdfium.so ==="

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
echo "=== Final shared library ==="
ls -lh "$SO"
file "$SO"
