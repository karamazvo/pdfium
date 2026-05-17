#!/usr/bin/env bash
set -euo pipefail

PATCH_FILE="patches/experiments/0001-jpeg-downscale-on-decode.patch"

echo "=== Apply experimental performance patches ==="

if [ ! -f "$PATCH_FILE" ]; then
  echo "ERROR: missing patch file: $PATCH_FILE" >&2
  exit 1
fi

echo "Patch file exists, but we intentionally do NOT use git apply."
echo "Reason: the uploaded patch has malformed hunk metadata and git apply reports it as corrupt."
echo "Applying equivalent source edits programmatically instead."

python3 <<'PY'
from pathlib import Path
import re

def read(path: str) -> str:
    return Path(path).read_text()

def write(path: str, text: str):
    Path(path).write_text(text)

def replace_once(text: str, old: str, new: str, label: str) -> str:
    if new in text:
        print(f"Already patched: {label}")
        return text
    if old not in text:
        raise RuntimeError(f"Could not find block for: {label}")
    print(f"Patch: {label}")
    return text.replace(old, new, 1)

# =============================================================================
# 1. core/fpdfapi/page/cpdf_dib.cpp
# =============================================================================

path = "core/fpdfapi/page/cpdf_dib.cpp"
s = read(path)

s = replace_once(
    s,
    "if (!CreateDCTDecoder(src_span, pParams)) {",
    "if (!CreateDCTDecoder(src_span, pParams, resolution_levels_to_skip)) {",
    "CPDF_DIB::CreateDecoder forwards resolution_levels_to_skip to CreateDCTDecoder",
)

s = replace_once(
    s,
    """bool CPDF_DIB::CreateDCTDecoder(pdfium::span<const uint8_t> src_span,
                                const CPDF_Dictionary* pParams) {""",
    """bool CPDF_DIB::CreateDCTDecoder(pdfium::span<const uint8_t> src_span,
                                const CPDF_Dictionary* pParams,
                                uint8_t resolution_levels_to_skip) {""",
    "CPDF_DIB::CreateDCTDecoder signature gains resolution_levels_to_skip",
)

s = replace_once(
    s,
    """  decoder_ = JpegModule::CreateDecoder(
      src_span, GetWidth(), GetHeight(), components_,
      !pParams || pParams->GetIntegerFor("ColorTransform", 1));""",
    """  decoder_ = JpegModule::CreateDecoder(
      src_span, GetWidth(), GetHeight(), components_,
      resolution_levels_to_skip,
      !pParams || pParams->GetIntegerFor("ColorTransform", 1));""",
    "initial DCT decoder creation forwards resolution_levels_to_skip",
)

# Retry path after detecting JPEG color transform info.
# PDFium formatting can vary slightly across revisions, so patch both known forms.
retry_old_1 = """  decoder_ = JpegModule::CreateDecoder(src_span, GetWidth(), GetHeight(),
                                       components_, info.color_transform);"""

retry_new_1 = """  decoder_ = JpegModule::CreateDecoder(src_span, GetWidth(), GetHeight(),
                                       components_, resolution_levels_to_skip,
                                       info.color_transform);"""

retry_old_2 = """    decoder_ = JpegModule::CreateDecoder(src_span, GetWidth(), GetHeight(),
                                         components_, info.color_transform);"""

retry_new_2 = """    decoder_ = JpegModule::CreateDecoder(src_span, GetWidth(), GetHeight(),
                                         components_, resolution_levels_to_skip,
                                         info.color_transform);"""

if retry_new_1 in s or retry_new_2 in s:
    print("Already patched: retry DCT decoder creation forwards resolution_levels_to_skip")
elif retry_old_1 in s:
    s = s.replace(retry_old_1, retry_new_1, 1)
    print("Patch: retry DCT decoder creation forwards resolution_levels_to_skip")
elif retry_old_2 in s:
    s = s.replace(retry_old_2, retry_new_2, 1)
    print("Patch: retry DCT decoder creation forwards resolution_levels_to_skip")
else:
    raise RuntimeError("Could not find retry DCT decoder creation block")

# Final safety pass for cpdf_dib.cpp:
# Patch any remaining 5-argument JpegModule::CreateDecoder(...) calls.
# This catches formatting variants that exact string replacements may miss.
import re

def patch_remaining_cpdf_dib_create_decoder_calls(text: str) -> str:
    pattern = re.compile(
        r'(JpegModule::CreateDecoder\(\s*'
        r'src_span\s*,\s*'
        r'GetWidth\(\)\s*,\s*'
        r'GetHeight\(\)\s*,\s*'
        r'components_\s*,\s*)'
        r'([^\)]*?color_transform)'
        r'(\s*\))',
        re.S,
    )

    def repl(m):
        full = m.group(0)
        if "resolution_levels_to_skip" in full:
            return full
        return m.group(1) + "resolution_levels_to_skip,\n                                       " + m.group(2) + m.group(3)

    text2, count = pattern.subn(repl, text)
    print(f"Safety patched remaining cpdf_dib.cpp CreateDecoder calls: {count}")
    return text2

s = patch_remaining_cpdf_dib_create_decoder_calls(s)

# Hard fail if cpdf_dib.cpp still contains the old 5-argument retry call.
if re.search(
    r'JpegModule::CreateDecoder\(\s*src_span\s*,\s*GetWidth\(\)\s*,\s*GetHeight\(\)\s*,\s*components_\s*,\s*info\.color_transform\s*\)',
    s,
    re.S,
):
    raise RuntimeError("cpdf_dib.cpp still has an old 5-arg JpegModule::CreateDecoder retry call")

write(path, s)

# =============================================================================
# 2. core/fpdfapi/page/cpdf_dib.h
# =============================================================================

path = "core/fpdfapi/page/cpdf_dib.h"
s = read(path)

s = replace_once(
    s,
    """  bool CreateDCTDecoder(pdfium::span<const uint8_t> src_span,
                        const CPDF_Dictionary* pParams);""",
    """  bool CreateDCTDecoder(pdfium::span<const uint8_t> src_span,
                        const CPDF_Dictionary* pParams,
                        uint8_t resolution_levels_to_skip);""",
    "CPDF_DIB::CreateDCTDecoder declaration gains resolution_levels_to_skip",
)

write(path, s)

# =============================================================================
# 3. core/fpdfapi/page/cpdf_streamparser.cpp
# =============================================================================

path = "core/fpdfapi/page/cpdf_streamparser.cpp"
s = read(path)

s = replace_once(
    s,
    """    std::unique_ptr<ScanlineDecoder> pDecoder = JpegModule::CreateDecoder(
        src_span, width, height, 0,
        !pParam || pParam->GetIntegerFor("ColorTransform", 1));""",
    """    std::unique_ptr<ScanlineDecoder> pDecoder = JpegModule::CreateDecoder(
        src_span, width, height, /*nComps=*/0,
        /*resolution_levels_to_skip=*/0,
        !pParam || pParam->GetIntegerFor("ColorTransform", 1));""",
    "inline DCT images call JpegModule::CreateDecoder with skip=0",
)

write(path, s)

# =============================================================================
# 4. core/fxcodec/jpeg/jpegmodule.h
# =============================================================================

path = "core/fxcodec/jpeg/jpegmodule.h"
s = read(path)

s = replace_once(
    s,
    """      uint32_t width,
      uint32_t height,
      int nComps,
      bool ColorTransform);""",
    """      uint32_t width,
      uint32_t height,
      int nComps,
      uint8_t resolution_levels_to_skip,
      bool ColorTransform);""",
    "JpegModule::CreateDecoder declaration gains resolution_levels_to_skip",
)

write(path, s)

# =============================================================================
# 5. core/fxcodec/jpeg/jpegmodule.cpp
# =============================================================================

path = "core/fxcodec/jpeg/jpegmodule.cpp"
s = read(path)

# 5.1 JpegDecoder::Create declaration inside class.
# Claude's required change:
#   int nComps,
#   uint8_t resolution_levels_to_skip,
#   bool ColorTransform);
s = replace_once(
    s,
    """  bool Create(pdfium::span<const uint8_t> src_span,
              uint32_t width,
              uint32_t height,
              int nComps,
              bool ColorTransform);""",
    """  bool Create(pdfium::span<const uint8_t> src_span,
              uint32_t width,
              uint32_t height,
              int nComps,
              uint8_t resolution_levels_to_skip,
              bool ColorTransform);""",
    "JpegDecoder::Create class declaration gains resolution_levels_to_skip",
)

# 5.2 JpegDecoder::Create out-of-line implementation signature.
# Claude's required change:
#   int nComps,
#   uint8_t resolution_levels_to_skip,
#   bool ColorTransform) {
s = replace_once(
    s,
    """bool JpegDecoder::Create(pdfium::span<const uint8_t> src_span,
                         uint32_t width,
                         uint32_t height,
                         int nComps,
                         bool ColorTransform) {""",
    """bool JpegDecoder::Create(pdfium::span<const uint8_t> src_span,
                         uint32_t width,
                         uint32_t height,
                         int nComps,
                         uint8_t resolution_levels_to_skip,
                         bool ColorTransform) {""",
    "JpegDecoder::Create implementation signature gains resolution_levels_to_skip",
)

# 5.3 Add scale denominator after src_span validation.
s = replace_once(
    s,
    """  src_span_ = JpegScanSOI(src_span);
  if (src_span_.size() < 2) {
    return false;
  }

  PatchUpTrailer();""",
    """  src_span_ = JpegScanSOI(src_span);
  if (src_span_.size() < 2) {
    return false;
  }

  // libjpeg supports decode-time downscale via scale_num/scale_denom.
  // scale_denom must be a power of 2 in [1, 8]. Clamp here.
  const uint32_t scale_denom =
      1u << std::min<uint8_t>(resolution_levels_to_skip, 3);

  PatchUpTrailer();""",
    "compute JPEG decode scale denominator",
)

# 5.4 Apply libjpeg scaling before CalcPitch.
s = replace_once(
    s,
    """  if (common_.cinfo.image_width < width) {
    return false;
  }

  CalcPitch();""",
    """  if (common_.cinfo.image_width < width) {
    return false;
  }

  // Apply downscale-on-decode. libjpeg will skip DCT blocks during the
  // IDCT pass, producing output of dimensions ceil(orig / scale_denom).
  // This must be set before jpeg_start_decompress, called from Rewind().
  common_.cinfo.scale_num = 1;
  common_.cinfo.scale_denom = scale_denom;
  default_scale_denom_ = scale_denom;
  output_width_ = (orig_width_ + scale_denom - 1) / scale_denom;
  output_height_ = (orig_height_ + scale_denom - 1) / scale_denom;

  CalcPitch();""",
    "apply JPEG decode-time downscale before CalcPitch",
)

# 5.5 JpegModule::CreateDecoder public factory signature.
s = replace_once(
    s,
    """std::unique_ptr<ScanlineDecoder> JpegModule::CreateDecoder(
    pdfium::span<const uint8_t> src_span,
    uint32_t width,
    uint32_t height,
    int nComps,
    bool ColorTransform) {""",
    """std::unique_ptr<ScanlineDecoder> JpegModule::CreateDecoder(
    pdfium::span<const uint8_t> src_span,
    uint32_t width,
    uint32_t height,
    int nComps,
    uint8_t resolution_levels_to_skip,
    bool ColorTransform) {""",
    "JpegModule::CreateDecoder definition gains resolution_levels_to_skip",
)

# 5.6 Forward resolution_levels_to_skip into JpegDecoder::Create.
s = replace_once(
    s,
    """  if (!pDecoder->Create(src_span, width, height, nComps, ColorTransform)) {""",
    """  if (!pDecoder->Create(src_span, width, height, nComps,
                        resolution_levels_to_skip, ColorTransform)) {""",
    "JpegModule forwards resolution_levels_to_skip to JpegDecoder",
)

# Hard validation for this file.
if """int nComps,
              bool ColorTransform);""" in s:
    raise RuntimeError("JpegDecoder::Create class declaration still has old 5-arg signature")

if """int nComps,
                         bool ColorTransform) {""" in s:
    raise RuntimeError("JpegDecoder::Create implementation still has old 5-arg signature")

if "uint8_t resolution_levels_to_skip" not in s:
    raise RuntimeError("jpegmodule.cpp missing resolution_levels_to_skip after patch")

write(path, s)

print("Programmatic JPEG downscale experiment patch applied.")
PY

echo
echo "=== Show changed files ==="
git diff --stat

echo
echo "=== Verify patched source contains experiment markers ==="
grep -R "resolution_levels_to_skip" \
  core/fxcodec/jpeg/jpegmodule.cpp \
  core/fxcodec/jpeg/jpegmodule.h \
  core/fpdfapi/page/cpdf_dib.cpp \
  core/fpdfapi/page/cpdf_dib.h \
  core/fpdfapi/page/cpdf_streamparser.cpp

grep -R "scale_denom" core/fxcodec/jpeg/jpegmodule.cpp

echo
echo "OK: experimental JPEG downscale-on-decode edits applied."
