# Production PDFium patches (ship-quality)

This directory contains the **four independent, self-contained patches**
that ship in production builds. Unlike the `experiments/` directory,
these patches:

- Are **complete and self-contained** — each applies to a clean PDFium
  HEAD without depending on the others.
- Are **order-independent** — apply them in any sequence (1→2→3,
  3→2→1, etc.) and the result is identical.
- Contain **no diagnostic / debug-only code** — no ATrace markers, no
  Android-specific includes. Each is portable to any platform PDFium
  supports.
- Are **bit-identical output** versus baseline PDFium where it matters
  (patches 02 and 03 are pure speedups; patch 01's libjpeg scale path
  produces visually-equivalent output via libjpeg's documented API; patch
  04 adds exports only and does not change renderer behavior).

## The four patches

| # | File | Effect |
|---|---|---|
| 01 | `01-jpeg-downscale-on-decode.patch` | Embedded JPEG images decode at the embedder's requested target size via libjpeg's `scale_num/scale_denom`, instead of always decoding at native size and downscaling the resulting bitmap. ~5-64× decode-time speedup at small render sizes. |
| 02 | `02-indexed-separation-fast.patch` | Adds fast `TranslateImageLine` overrides for `CPDF_IndexedCS` and `CPDF_SeparationCS` via precomputed BGR lookup tables. Replaces per-pixel virtual `GetRGB()` dispatch + float math with a 1-byte-index → 3-byte memcpy. Bit-identical output. ~50-200× per-scanline speedup. |
| 03 | `03-devicen-fast.patch` | Same fast-table pattern for `CPDF_DeviceNCS` (N=1 and N=2 component variants; N≥3 falls through to the existing slow path). Bit-identical output. ~50-200× per-scanline speedup. |
| 04 | `04-veloce-internal-access.patch` | Adds Veloce internal-access exports for graphics-state proof, rectangular clip detection, Form transparency groups, fast object iteration, and ABI sentinels. No rendering semantics change. |

## Why this directory exists

During development the patches went through many iterations under
`patches/experiments/` (numbered 0001..0009). Many of those were
diagnostic-only (trace markers, ICC bypass, etc.) and depend on each
other. The `ship/` directory is the **clean, audit-ready, upstreamable**
subset.

## How to apply

```bash
cd /path/to/pdfium
git apply patches/ship/01-jpeg-downscale-on-decode.patch
git apply patches/ship/02-indexed-separation-fast.patch
git apply patches/ship/03-devicen-fast.patch
git apply patches/ship/04-veloce-internal-access.patch
```

(Any order works.)

## Cumulative effect (measured)

On a representative image-heavy picture-book PDF, applying patches 01-03:

| Phase | Per-page render | Δ |
|---|---|---|
| Baseline (vanilla PDFium HEAD) | ~5,000 ms | — |
| With patches 01-03 | ~135 ms | **~37× faster** |

Visual quality: bit-identical output for patches 02 and 03; libjpeg
scale_denom path (patch 01) produces standard downsampled output per
the documented libjpeg API. Patch 04 does not affect rendering output.

## Files changed by each patch

- **01 JPEG downscale**: `core/fpdfapi/page/cpdf_dib.{h,cpp}`,
  `core/fpdfapi/page/cpdf_streamparser.cpp`,
  `core/fxcodec/jpeg/jpegmodule.{h,cpp}`,
  `BUILD.gn`.
- **02 IndexedCS + SeparationCS**: `core/fpdfapi/page/cpdf_indexedcs.{h,cpp}`,
  `core/fpdfapi/page/cpdf_colorspace.cpp` (SeparationCS only).
- **03 DeviceNCS**: `core/fpdfapi/page/cpdf_colorspace.cpp` (DeviceNCS only).
- **04 Veloce internal access**: `public/fpdf_edit.h`,
  `fpdfsdk/fpdf_editpage.cpp`.

No two patches touch the same lines. Patches 02 and 03 both touch
`cpdf_colorspace.cpp`, but in different sections (SeparationCS vs.
DeviceNCS), so they coexist cleanly.

## Upstreaming

Patches 01-03 are independently suitable for upstream PRs to the PDFium
project. Patch 04 is intentionally xPDFSDK/Veloce-specific because it
exports internal PDFium state for a local rendering accelerator.

The patches in `experiments/` (0001–0009) are **NOT** upstreamable —
they include Android-specific ATrace dependencies and diagnostic debug
toggles. Production builds should use only this `ship/` directory.
