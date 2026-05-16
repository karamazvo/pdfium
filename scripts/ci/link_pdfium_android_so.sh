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

echo "=== Collect project dependency closure from GN ==="

mapfile -t PROJECT_TARGETS < <(
  {
    echo "//:pdfium"
    echo "//skia:skia"
    gn desc "$OUT" //:pdfium deps --all
    gn desc "$OUT" //skia:skia deps --all
  } | sed '/^[[:space:]]*$/d' | sort -u
)

PROJECT_INPUTS=()
for t in "${PROJECT_TARGETS[@]}"; do
  while IFS= read -r out; do
    p="${out#//}"
    case "$p" in
      out/android_arm64/*.a|out/android_arm64/*.o)
        PROJECT_INPUTS+=("$p")
        ;;
    esac
  done < <(gn desc "$OUT" "$t" outputs 2>/dev/null || true)
done

mapfile -t PROJECT_INPUTS_UNIQ < <(
  printf '%s\n' "${PROJECT_INPUTS[@]}" \
    | sort -u \
    | grep -v '^out/android_arm64/obj/libpdfium\.a$'
)

echo "Project inputs:"
printf '  %s\n' "${PROJECT_INPUTS_UNIQ[@]}"

echo
echo "=== Build missing project inputs ==="

MISSING_PROJECT=()
for p in "${PROJECT_INPUTS_UNIQ[@]}"; do
  if [ ! -f "$p" ]; then
    MISSING_PROJECT+=("${p#$OUT/}")
  fi
done

if [ "${#MISSING_PROJECT[@]}" -gt 0 ]; then
  printf 'Need to build project inputs:\n'
  printf '  %s\n' "${MISSING_PROJECT[@]}"
  ninja -C "$OUT" "${MISSING_PROJECT[@]}"
else
  echo "All project inputs already exist."
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
echo "=== Sanity: look for SkFontMgr_New_Custom_Empty provider ==="
FOUND_EMPTY=0
for p in "${PROJECT_INPUTS_UNIQ[@]}"; do
  if [ -f "$p" ]; then
    if third_party/llvm-build/Release+Asserts/bin/llvm-nm -C "$p" 2>/dev/null \
      | grep -E ' [TWDV] SkFontMgr_New_Custom_Empty\(\)$' >/dev/null; then
      echo "Found provider: $p"
      FOUND_EMPTY=1
    fi
  fi
done

if [ "$FOUND_EMPTY" = "0" ]; then
  echo "WARNING: did not find SkFontMgr_New_Custom_Empty() in collected project inputs."
  echo "The final link may still fail."
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
    "${PROJECT_INPUTS_UNIQ[@]}" \
    "${RUNTIME_INPUTS_UNIQ[@]}" \
    "${LIBUNWIND_OBJECTS[@]}" \
  -Wl,--end-group \
  "$BUILTINS" \
  -Wl,--no-undefined \
  -Wl,--export-dynamic-symbol=FPDF* \
  -Wl,--export-dynamic-symbol=FORM_* \
  -Wl,--wrap=getcwd \
  -Wl,--wrap=realpath \
  -Wl,-z,max-page-size=16384 \
  -llog -landroid -ldl -lm

echo
echo "=== Final shared library ==="
ls -lh "$SO"
file "$SO"
