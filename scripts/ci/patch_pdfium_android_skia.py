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
# 2. Export public FPDF_* APIs
# =============================================================================

build_gn = ROOT / "BUILD.gn"
s = build_gn.read_text()

old_public = '''config("pdfium_public_config") {
  defines = []'''

new_public = '''config("pdfium_public_config") {
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

# Remove older wrong manual source patch if present.
s = s.replace("""
    # Required by SkFontMgr_android.cpp in the real Android font-manager path.
    sources += [
      "//third_party/skia/src/ports/SkFontMgr_android_parser.cpp",
      "//third_party/skia/src/core/SkPaintOptionsAndroid.cpp",
    ]
""", "\n")

# 3.1 Find the Android font-manager block inside //skia.
anchor = "    sources += skia_ports_fontmgr_android_sources"

android_block_match = re.search(
    r'if \(is_android\) \{.*?' + re.escape(anchor) + r'.*?\n  \}',
    s,
    re.S,
)
if not android_block_match:
    raise RuntimeError("Could not find Android font-manager block in skia/BUILD.gn")

block = android_block_match.group(0)

# Use BOTH source groups and exact source files.
# The exact files are the important part. This prevents CI from reaching link
# without SkTypeface_proxy.o / SkFontMgr_custom_empty.o / Android parser objects.
required_group_lines = [
    "    sources += skia_ports_fontmgr_android_parser_sources",
    "    sources += skia_ports_fontmgr_custom_sources",
    "    sources += skia_ports_fontmgr_empty_sources",
    "    sources += skia_ports_typeface_proxy_sources",
]

required_exact_sources = [
    '      "//third_party/skia/src/ports/SkFontMgr_android_parser.cpp",',
    '      "//third_party/skia/src/ports/SkFontMgr_custom.cpp",',
    '      "//third_party/skia/src/ports/SkFontMgr_custom_empty.cpp",',
    '      "//third_party/skia/src/ports/SkTypeface_proxy.cpp",',
]

insert_lines = []

for line in required_group_lines:
    if line not in block:
        insert_lines.append(line)

missing_exact = [line for line in required_exact_sources if line not in block]
if missing_exact:
    insert_lines.extend([
        "",
        "    # Standalone PDFium Android + Skia: force required implementation files.",
        "    # These must be compiled, not merely referenced through Skia headers.",
        "    sources += [",
        *missing_exact,
        "    ]",
    ])

if insert_lines:
    new_block = block.replace(anchor, anchor + "\n" + "\n".join(insert_lines))
    s = s[:android_block_match.start()] + new_block + s[android_block_match.end():]

# 3.2 Remove only the invalid Android optimization block inside skia_opts.
start, end, skia_opts = parse_gn_block(s, 'skia_source_set("skia_opts")')

bad_android_opt = re.compile(
    r'\n\s*if\s*\(\s*is_android\s*&&\s*!is_debug\s*\)\s*\{\s*'
    r'configs\s*-=\s*\[\s*"//build/config/compiler:default_optimization"\s*\]\s*'
    r'configs\s*\+=\s*\[\s*"//build/config/compiler:optimize_max"\s*\]\s*'
    r'\}\s*',
    re.S,
)

skia_opts2, count = bad_android_opt.subn("\n", skia_opts, count=1)
if count:
    print("Removed invalid Android optimization block inside skia_opts.")
else:
    print("No invalid Android optimization block inside skia_opts, maybe upstream already changed.")

s = s[:start] + skia_opts2 + s[end:]
skia_gn.write_text(s)

# =============================================================================
# 4. Verify patch file contents before GN
# =============================================================================

patched = skia_gn.read_text()

for token in [
    "skia_ports_fontmgr_android_parser_sources",
    "skia_ports_fontmgr_custom_sources",
    "skia_ports_fontmgr_empty_sources",
    "skia_ports_typeface_proxy_sources",
    "SkFontMgr_android_parser.cpp",
    "SkFontMgr_custom.cpp",
    "SkFontMgr_custom_empty.cpp",
    "SkTypeface_proxy.cpp",
]:
    if token not in patched:
        raise RuntimeError(f"Patch failed: missing {token}")

_, _, final_skia_opts = parse_gn_block(patched, 'skia_source_set("skia_opts")')

if (
    "is_android && !is_debug" in final_skia_opts
    and 'configs -= [ "//build/config/compiler:default_optimization" ]' in final_skia_opts
):
    raise RuntimeError("Patch failed: invalid Android default_optimization block remains inside skia_opts")

print("Patch complete.")
print()
print("=== Android Skia block ===")
run("grep -n -A28 -B4 'skia_ports_fontmgr_android_sources' skia/BUILD.gn")
print()
print("=== skia_opts block ===")
run("grep -n -A45 -B5 'skia_source_set(\"skia_opts\")' skia/BUILD.gn")
