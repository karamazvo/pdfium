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

# Remove older wrong manual-source patch if present.
s = s.replace("""
    # Required by SkFontMgr_android.cpp in the real Android font-manager path.
    sources += [
      "//third_party/skia/src/ports/SkFontMgr_android_parser.cpp",
      "//third_party/skia/src/core/SkPaintOptionsAndroid.cpp",
    ]
""", "\n")

# Remove previous direct-force patch if present.
s = re.sub(
    r'''
\s*#\s*Standalone PDFium Android \+ Skia: force required implementation files\.\n
\s*#\s*These must be compiled, not merely referenced through Skia headers\.\n
\s*sources \+= \[\n
(?:\s*"//third_party/skia/src/ports/SkFontMgr_android_parser\.cpp",\n)?
(?:\s*"//third_party/skia/src/ports/SkFontMgr_custom\.cpp",\n)?
(?:\s*"//third_party/skia/src/ports/SkFontMgr_custom_empty\.cpp",\n)?
(?:\s*"//third_party/skia/src/ports/SkTypeface_proxy\.cpp",\n)?
\s*\]\n
''',
    "\n",
    s,
    flags=re.X,
)

# 3.1 Add correct Android Skia source groups to //skia.
anchor = "    sources += skia_ports_fontmgr_android_sources"

android_block_match = re.search(
    r'if \(is_android\) \{.*?' + re.escape(anchor) + r'.*?\n  \}',
    s,
    re.S,
)
if not android_block_match:
    raise RuntimeError("Could not find Android font-manager block in skia/BUILD.gn")

block = android_block_match.group(0)

required_group_lines = [
    "    sources += skia_ports_fontmgr_android_parser_sources",
    "    sources += skia_ports_fontmgr_custom_sources",
    "    sources += skia_ports_fontmgr_empty_sources",
    "    sources += skia_ports_typeface_proxy_sources",
]

missing_group_lines = [line for line in required_group_lines if line not in block]

if missing_group_lines:
    new_block = block.replace(anchor, anchor + "\n" + "\n".join(missing_group_lines))
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

# 3.3 Add a separate compat source_set for fallback implementation files.
#
# Important:
# Do NOT add these directly to //skia sources, because some may already be
# included by Skia source-group variables. Direct addition can produce:
#   Duplicate object file: SkFontMgr_android_parser.o
#
# A separate target gives separate object paths:
#   obj/skia/pdfium_android_skia_compat/*.o
compat_target = '''
# PDFium standalone Android Skia compatibility target.
# Kept separate from //skia sources to avoid duplicate object names when
# upstream Skia source-group variables already include some files.
source_set("pdfium_android_skia_compat") {
  sources = [
    "//third_party/skia/src/ports/SkTypeface_proxy.cpp",
  ]

  deps = [
    "//third_party/skia:skia_core_and_effects",
  ]

  public_deps = [
    "//third_party/skia:skia_core_public",
  ]

  configs -= [ "//build/config/compiler:chromium_code" ]
  configs += [ "//build/config/compiler:no_chromium_code" ]
}
'''

if 'source_set("pdfium_android_skia_compat")' not in s:
    s = s.rstrip() + "\n\n" + compat_target + "\n"

# 3.4 Wire compat target into //skia deps.
skia_start, skia_end, skia_block = parse_gn_block(s, 'static_library("skia")')
if ':pdfium_android_skia_compat' not in skia_block:
    # Prefer appending to an existing deps list if present.
    deps_match = re.search(r'\n\s*deps\s*=\s*\[', skia_block)
    if deps_match:
        insert_pos = deps_match.end()
        skia_block2 = skia_block[:insert_pos] + '\n    ":pdfium_android_skia_compat",' + skia_block[insert_pos:]
    else:
        # Fallback: add deps block near top of static_library("skia").
        brace_pos = skia_block.find("{")
        skia_block2 = (
            skia_block[:brace_pos + 1]
            + '\n  deps = [ ":pdfium_android_skia_compat" ]'
            + skia_block[brace_pos + 1:]
        )
    s = s[:skia_start] + skia_block2 + s[skia_end:]

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
    'source_set("pdfium_android_skia_compat")',
    "SkTypeface_proxy.cpp",
    ':pdfium_android_skia_compat',
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
run("grep -n -A24 -B4 'skia_ports_fontmgr_android_sources' skia/BUILD.gn")
print()
print("=== Compat target ===")
run("grep -n -A28 -B4 'pdfium_android_skia_compat' skia/BUILD.gn")
print()
print("=== skia_opts block ===")
run("grep -n -A45 -B5 'skia_source_set(\"skia_opts\")' skia/BUILD.gn")
