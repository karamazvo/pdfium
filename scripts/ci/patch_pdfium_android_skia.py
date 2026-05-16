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

def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise RuntimeError(f"Could not find block: {label}")
    return text.replace(old, new, 1)

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
# 2. Export FPDF_* APIs from static-build final .so
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
    s = replace_once(s, old_public, new_public, "pdfium_public_config")

build_gn.write_text(s)

# =============================================================================
# 3. Patch skia/BUILD.gn
# =============================================================================

skia_gn = ROOT / "skia/BUILD.gn"
s = skia_gn.read_text()

# 3.1 Complete Android Skia source groups.
anchor = "    sources += skia_ports_fontmgr_android_sources"

android_block_match = re.search(
    r'if \(is_android\) \{.*?' + re.escape(anchor) + r'.*?\n  \}',
    s,
    re.S,
)
if not android_block_match:
    raise RuntimeError("Could not find Android font-manager block in skia/BUILD.gn")

block = android_block_match.group(0)

needed_lines = [
    "    sources += skia_ports_fontmgr_android_parser_sources",
    "    sources += skia_ports_fontmgr_custom_sources",
    "    sources += skia_ports_fontmgr_empty_sources",
    "    sources += skia_ports_typeface_proxy_sources",
]

missing_lines = [line for line in needed_lines if line not in block]

if missing_lines:
    new_block = block.replace(anchor, anchor + "\n" + "\n".join(missing_lines))
    s = s[:android_block_match.start()] + new_block + s[android_block_match.end():]

# 3.2 Remove only the invalid Android optimization block inside skia_opts.
start = s.find('skia_source_set("skia_opts")')
if start == -1:
    raise RuntimeError('Could not find skia_source_set("skia_opts") block')

brace = s.find("{", start)
if brace == -1:
    raise RuntimeError("Could not find opening brace of skia_opts")

depth = 0
end = None
for i in range(brace, len(s)):
    if s[i] == "{":
        depth += 1
    elif s[i] == "}":
        depth -= 1
        if depth == 0:
            end = i + 1
            break

if end is None:
    raise RuntimeError("Could not parse skia_opts block")

before = s[:start]
skia_opts = s[start:end]
after = s[end:]

bad_android_opt = re.compile(
    r'\n\s*if\s*\(\s*is_android\s*&&\s*!is_debug\s*\)\s*\{\s*'
    r'configs\s*-=\s*\[\s*"//build/config/compiler:default_optimization"\s*\]\s*'
    r'configs\s*\+=\s*\[\s*"//build/config/compiler:optimize_max"\s*\]\s*'
    r'\}\s*',
    re.S,
)

skia_opts2, count = bad_android_opt.subn("\n", skia_opts, count=1)
if count == 0:
    print("No invalid Android optimization block found inside skia_opts, maybe upstream changed it.")
else:
    print("Removed invalid Android optimization block inside skia_opts.")

s = before + skia_opts2 + after
skia_gn.write_text(s)

# =============================================================================
# 4. Verify patch
# =============================================================================

patched = skia_gn.read_text()

for required in [
    "skia_ports_fontmgr_android_parser_sources",
    "skia_ports_fontmgr_custom_sources",
    "skia_ports_fontmgr_empty_sources",
    "skia_ports_typeface_proxy_sources",
]:
    if required not in patched:
        raise RuntimeError(f"Patch failed: missing {required}")

start = patched.find('skia_source_set("skia_opts")')
brace = patched.find("{", start)
depth = 0
end = None
for i in range(brace, len(patched)):
    if patched[i] == "{":
        depth += 1
    elif patched[i] == "}":
        depth -= 1
        if depth == 0:
            end = i + 1
            break

skia_opts_final = patched[start:end]

if (
    "if (is_android && !is_debug)" in skia_opts_final
    and 'configs -= [ "//build/config/compiler:default_optimization" ]' in skia_opts_final
):
    raise RuntimeError("Patch failed: bad Android default_optimization block remains inside skia_opts")

print("Patch complete.")
print()
print("=== Android Skia block ===")
run("grep -n -A18 -B4 'skia_ports_fontmgr_android_sources' skia/BUILD.gn")
print()
print("=== skia_opts block ===")
run("grep -n -A45 -B5 'skia_source_set(\"skia_opts\")' skia/BUILD.gn")
