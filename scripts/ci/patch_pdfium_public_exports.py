from pathlib import Path

p = Path("BUILD.gn")
s = p.read_text()

old = '''config("pdfium_public_config") {
  defines = []'''

new = '''config("pdfium_public_config") {
  # Standalone manually linked shared-library build:
  # Keep is_component_build=false for the static GN graph,
  # but make public FPDF_* APIs visible in the final libpdfium.so.
  defines = [
    "COMPONENT_BUILD",
    "FPDF_IMPLEMENTATION",
  ]'''

if new in s:
    print("Public export patch already applied.")
elif old in s:
    s = s.replace(old, new, 1)
    p.write_text(s)
    print("Applied public FPDF_* export patch.")
else:
    raise RuntimeError("Could not find pdfium_public_config defines block in BUILD.gn")
