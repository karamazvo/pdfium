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

skia_gn = ROOT / "skia/BUILD.gn"
s = skia_gn.read_text()

fontmgr_pattern = re.compile(
    r'if \(is_android\) \{.*?sources \+= skia_ports_fontmgr_android_sources.*?\n  \}',
    re.S,
)
m = fontmgr_pattern.search(s)
if not m:
    raise RuntimeError("Android font-manager block not found")

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

opt_pattern = re.compile(r'''
(\s*skia_source_set\("skia_opts"\)\s*\{.*?)
\s*if \(is_android && !is_debug\) \{
\s*configs -= \[ "//build/config/compiler:default_optimization" \]
\s*configs \+= \[ "//build/config/compiler:optimize_max" \]
\s*\}
(.*?\n\s*\})
''', re.S | re.X)

mo = opt_pattern.search(s)
if mo:
    s = s[:mo.start()] + mo.group(1) + mo.group(2) + s[mo.end():]

skia_gn.write_text(s)

print("Patch complete.")
