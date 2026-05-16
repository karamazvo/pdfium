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
    raise RuntimeError("libexpat_revision not found")
rev = m.group(1)

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
# 2. Export FPDF_* APIs
# =============================================================================

build_gn = ROOT / "BUILD.gn"
s = build_gn.read_text()

old = '''config("pdfium_public_config") {
  defines = []'''

new = '''config("pdfium_public_config") {
  defines = [
    "COMPONENT_BUILD",
    "FPDF_IMPLEMENTATION",
  ]'''

if new not in s:
    if old not in s:
        raise RuntimeError("pdfium_public_config block not found")
    s = s.replace(old, new, 1)

build_gn.write_text(s)

# =============================================================================
# 3. Patch skia/BUILD.gn
# =============================================================================

skia_gn = ROOT / "skia/BUILD.gn"
s = skia_gn.read_text()

# Remove older wrong manual patch if present.
s = s.replace("""
    # Required by SkFontMgr_android.cpp in the real Android font-manager path.
    sources += [
      "//third_party/skia/src/ports/SkFontMgr_android_parser.cpp",
      "//third_party/skia/src/core/SkPaintOptionsAndroid.cpp",
    ]
""", "\n")

# Add missing Android Skia source groups.
fontmgr_pattern = re.compile(
    r'if \(is_android\) \{.*?sources \+= skia_ports_fontmgr_android_sources.*?\n  \}',
    re.S,
)
m = fontmgr_pattern.search(s)
if not m:
    raise RuntimeError("Android font-manager block not found in skia/BUILD.gn")

block = m.group(0)
needed = [
    "    sources += skia_ports_fontmgr_android_parser_sources",
    "    sources += skia_ports_fontmgr_custom_sources",
    "    sources += skia_ports_fontmgr_empty_sources",
    "    sources += skia_ports_typeface_proxy_sources",
]
missing = [x for x in needed if x not in block]

if missing:
    anchor = "    sources += skia_ports_fontmgr_android_sources"
    block2 = block.replace(anchor, anchor + "\n" + "\n".join(missing))
    s = s[:m.start()] + block2 + s[m.end():]

# Robustly remove the invalid Android-only configs block inside skia_opts.
# This block causes:
# ERROR at //skia/BUILD.gn:521:5: Undefined identifier. configs -= ...
bad_block = '''  if (is_android && !is_debug) {
    configs -= [ "//build/config/compiler:default_optimization" ]
    configs += [ "//build/config/compiler:optimize_max" ]
  }
'''

if bad_block in s:
    s = s.replace(bad_block, "", 1)
else:
    # Fallback: remove same block even if spacing changed.
    s2 = re.sub(
        r'\n\s*if\s*\(\s*is_android\s*&&\s*!is_debug\s*\)\s*\{\s*'
        r'configs\s*-=\s*\[\s*"//build/config/compiler:default_optimization"\s*\]\s*'
        r'configs\s*\+=\s*\[\s*"//build/config/compiler:optimize_max"\s*\]\s*'
        r'\}\s*',
        "\n",
        s,
        count=1,
        flags=re.S,
    )
    if s2 == s:
        print("WARNING: invalid skia_opts Android configs block not found; maybe upstream already removed it.")
    s = s2

skia_gn.write_text(s)

# =============================================================================
# 4. Hard verification before GN gen
# =============================================================================

patched = skia_gn.read_text()

if 'configs -= [ "//build/config/compiler:default_optimization" ]' in patched:
    raise RuntimeError("Patch failed: invalid default_optimization configs block still exists in skia/BUILD.gn")

for required in [
    "skia_ports_fontmgr_android_parser_sources",
    "skia_ports_fontmgr_custom_sources",
    "skia_ports_fontmgr_empty_sources",
    "skia_ports_typeface_proxy_sources",
]:
    if required not in patched:
        raise RuntimeError(f"Patch failed: missing {required}")

print("Patch complete.")
print()
print("=== Android Skia block ===")
run("grep -n -A18 -B4 'skia_ports_fontmgr_android_sources' skia/BUILD.gn")
print()
print("=== skia_opts bad block check ===")
run("grep -n 'default_optimization' skia/BUILD.gn || true")
