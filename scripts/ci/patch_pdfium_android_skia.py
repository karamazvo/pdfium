from pathlib import Path
from textwrap import dedent
import base64
import re
import shutil
import subprocess
import urllib.request

ROOT = Path.cwd()

def run(cmd: str):
    print(f"$ {cmd}")
    subprocess.run(cmd, shell=True, check=True)

def parse_gn_block(text: str, start_token: str):
    start = text.find(start_token)
    if start == -1:
        raise RuntimeError(f"Could not find block start: {start_token}")

    brace = text.find("{", start)
    if brace == -1:
        raise RuntimeError(f"Could not find opening brace for: {start_token}")

    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return start, i + 1, text[start:i + 1]

    raise RuntimeError(f"Could not parse block: {start_token}")

def remove_block_if_exists(text: str, start_token: str) -> str:
    while start_token in text:
        s, e, _ = parse_gn_block(text, start_token)
        text = text[:s] + text[e:]
    return text

# =============================================================================
# 1. Vendor standalone expat
# =============================================================================

expat = ROOT / "third_party/expat"
if expat.exists():
    shutil.rmtree(expat)

(expat / "include/expat_config").mkdir(parents=True, exist_ok=True)

raw = urllib.request.urlopen(
    "https://chromium.googlesource.com/chromium/src/+/main/DEPS?format=TEXT"
).read()
deps = base64.b64decode(raw).decode()

m = re.search(r"'libexpat_revision':\s*'([0-9a-f]+)'", deps)
if not m:
    raise RuntimeError("libexpat_revision not found in Chromium DEPS")

rev = m.group(1)
print("libexpat_revision:", rev)

run(f"git clone --quiet https://chromium.googlesource.com/external/github.com/libexpat/libexpat.git {expat / 'src'}")
run(f"git -C {expat / 'src'} checkout --quiet {rev}")

run(
    f'curl -fsSL "https://chromium.googlesource.com/chromium/src/+/main/third_party/expat/include/expat_config/expat_config.h?format=TEXT" '
    f'| base64 -d > "{expat / "include/expat_config/expat_config.h"}"'
)

(expat / "BUILD.gn").write_text(dedent("""\
# Standalone production-only expat target for PDFium Android Skia.
# Avoids Chromium-only fuzzing/test infrastructure.

config("expat_public_config") {
  include_dirs = [
    "src/expat/lib",
    "include/expat_config",
  ]
  defines = [ "XML_STATIC" ]
}

config("expat_internal_config") {
  cflags = [
    "-Wno-unreachable-code",
    "-Wno-implicit-fallthrough",
  ]
}

static_library("expat") {
  sources = [
    "src/expat/lib/expat.h",
    "src/expat/lib/xmlparse.c",
    "src/expat/lib/xmlrole.c",
    "src/expat/lib/xmltok.c",
  ]

  public_configs = [ ":expat_public_config" ]
  configs += [ ":expat_internal_config" ]

  defines = [
    "_LIB",
    "HAVE_EXPAT_CONFIG_H",
  ]
}
"""))

# =============================================================================
# 2. Patch root BUILD.gn for exported FPDF_* APIs
# =============================================================================

build_gn = ROOT / "BUILD.gn"
s = build_gn.read_text()

# Undo older wrong root patch if present.
bad_root_patch = '''  # Android Skia build: if the Skia backend is requested, make it a real
  # dependency of the root PDFium target so the final shared library is linked
  # from one complete GN graph.
  if (defined(checkout_skia) && checkout_skia && pdf_use_skia) {
    deps += [ "//skia" ]
  }'''

upstream_root_block = '''  # TODO(crbug.com/pdfium/1832): Remove !is_android when //third_party/expat is
  # available.
  if (defined(checkout_skia) && checkout_skia && !is_android) {
    deps += [ "//skia" ]
  }'''

if bad_root_patch in s:
    s = s.replace(bad_root_patch, upstream_root_block, 1)

old_public = '''config("pdfium_public_config") {
  defines = []'''

new_public = '''config("pdfium_public_config") {
  # Standalone Android shared-library build:
  # Keep is_component_build=false for the static GN graph,
  # but make public FPDF_* APIs visible in the final manually linked .so.
  defines = [
    "COMPONENT_BUILD",
    "FPDF_IMPLEMENTATION",
  ]'''

if new_public not in s:
    if old_public not in s:
        raise RuntimeError("Could not find pdfium_public_config block")
    s = s.replace(old_public, new_public, 1)

build_gn.write_text(s)

# =============================================================================
# 3. Patch skia/BUILD.gn
# =============================================================================

skia_gn = ROOT / "skia/BUILD.gn"
s = skia_gn.read_text()

# Remove all previous failed compat target attempts.
s = remove_block_if_exists(s, 'source_set("pdfium_android_skia_compat")')
s = remove_block_if_exists(s, 'skia_source_set("pdfium_android_skia_compat")')

# Remove previous direct-force block attempts.
s = re.sub(
    r'''
\s*#\s*Standalone PDFium Android \+ Skia: force required implementation files.*?
\s*sources \+= \[\n
(?:\s*"//third_party/skia/src/ports/SkFontMgr_android_parser\.cpp",\n)?
(?:\s*"//third_party/skia/src/ports/SkFontMgr_custom\.cpp",\n)?
(?:\s*"//third_party/skia/src/ports/SkFontMgr_custom_empty\.cpp",\n)?
(?:\s*"//third_party/skia/src/ports/SkTypeface_proxy\.cpp",\n)?
\s*\]\n
''',
    "\n",
    s,
    flags=re.S | re.X,
)

# Remove earlier wrong parser direct-add block.
s = s.replace("""
    # Required by SkFontMgr_android.cpp in the real Android font-manager path.
    sources += [
      "//third_party/skia/src/ports/SkFontMgr_android_parser.cpp",
      "//third_party/skia/src/core/SkPaintOptionsAndroid.cpp",
    ]
""", "\n")

# Find the Android font-manager block.
pattern = re.compile(
    r'if \(is_android\) \{.*?sources \+= skia_ports_fontmgr_android_sources.*?\n  \}',
    re.S,
)
m = pattern.search(s)
if not m:
    raise RuntimeError("Could not find Android font-manager block in skia/BUILD.gn")

block = m.group(0)

# Important:
# - Keep parser source group, because CI already shows it correctly enters //skia.
# - Remove custom/empty/proxy source groups, because in GitHub CI they do NOT
#   expand into //skia sources, but may cause future duplicate weirdness.
# - Directly add only the missing implementation .cpp files.
for line in [
    "    sources += skia_ports_fontmgr_custom_sources",
    "    sources += skia_ports_fontmgr_empty_sources",
    "    sources += skia_ports_typeface_proxy_sources",
]:
    block = block.replace("\n" + line, "")

anchor = "    sources += skia_ports_fontmgr_android_sources"

if "    sources += skia_ports_fontmgr_android_parser_sources" not in block:
    block = block.replace(
        anchor,
        anchor + "\n    sources += skia_ports_fontmgr_android_parser_sources"
    )

direct_block = '''    # Standalone PDFium Android + Skia:
    # force the missing implementation providers into //skia.
    # Do NOT add SkFontMgr_android_parser.cpp here; it is already provided
    # by skia_ports_fontmgr_android_parser_sources and would duplicate.
    sources += [
      "//third_party/skia/src/ports/SkFontMgr_custom.cpp",
      "//third_party/skia/src/ports/SkFontMgr_custom_empty.cpp",
      "//third_party/skia/src/ports/SkTypeface_proxy.cpp",
    ]'''

if "SkFontMgr_custom_empty.cpp" not in block:
    block = block.replace(
        "    sources += skia_ports_fontmgr_android_parser_sources",
        "    sources += skia_ports_fontmgr_android_parser_sources\n\n" + direct_block
    )

s = s[:m.start()] + block + s[m.end():]

# Remove invalid Android optimization block inside skia_opts.
start, end, skia_opts = parse_gn_block(s, 'skia_source_set("skia_opts")')

bad_android_opt = re.compile(
    r'\n\s*if\s*\(\s*is_android\s*&&\s*!is_debug\s*\)\s*\{\s*'
    r'configs\s*-=\s*\[\s*"//build/config/compiler:default_optimization"\s*\]\s*'
    r'configs\s*\+=\s*\[\s*"//build/config/compiler:optimize_max"\s*\]\s*'
    r'\}\s*',
    re.S,
)

skia_opts, removed = bad_android_opt.subn("\n", skia_opts, count=1)
if removed:
    print("Removed invalid Android optimization block inside skia_opts.")

s = s[:start] + skia_opts + s[end:]
skia_gn.write_text(s)

# =============================================================================
# 4. Verify patch before GN
# =============================================================================

patched = skia_gn.read_text()

for token in [
    "skia_ports_fontmgr_android_parser_sources",
    "SkFontMgr_custom.cpp",
    "SkFontMgr_custom_empty.cpp",
    "SkTypeface_proxy.cpp",
]:
    if token not in patched:
        raise RuntimeError(f"Patch failed: missing {token}")

if "pdfium_android_skia_compat" in patched:
    raise RuntimeError("Patch failed: compat target should not exist")

_, _, final_skia_opts = parse_gn_block(patched, 'skia_source_set("skia_opts")')
if (
    "is_android && !is_debug" in final_skia_opts
    and 'configs -= [ "//build/config/compiler:default_optimization" ]' in final_skia_opts
):
    raise RuntimeError("Patch failed: invalid Android default_optimization block remains inside skia_opts")

print("Patch complete.")
print()
print("=== Android Skia block ===")
run("grep -n -A30 -B4 'skia_ports_fontmgr_android_sources' skia/BUILD.gn")
print()
print("=== skia_opts block ===")
run("grep -n -A45 -B5 'skia_source_set(\"skia_opts\")' skia/BUILD.gn")
