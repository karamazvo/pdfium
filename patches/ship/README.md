# Production PDFium patches (ship-quality)

This directory contains the **eight self-contained patches**
used by the Veloce PDFium build. Unlike the `experiments/` directory,
these patches:

- Are **complete and audit-ready**. Patches 01-04 apply independently to a
  clean PDFium HEAD. Patch 05 applies to clean PDFium HEAD. Patches 06 and
  07 are intentionally layered after patch 05 because they extend the same
  Veloce render-loop/export surface. Patch 08 is layered after the Veloce
  fpdfview export patches and adds first-render page classification.
- Are applied in numeric order by the Veloce release workflow.
- Contain **no ATrace markers** and no Android-specific includes. Patches
  01-04 are production-safe by default. Patch 05 is diagnostic/default-off:
  it only changes rendering when `FPDFEx_SetSkipRasterization()` is
  explicitly enabled by a measurement harness.
- Are **bit-identical output** versus baseline PDFium where it matters
  (patches 02 and 03 are pure speedups; patch 01's libjpeg scale path
  produces visually-equivalent output via libjpeg's documented API; patch
  04 adds exports only and does not change renderer behavior; patch 05
  preserves normal output while its toggle remains disabled).
  Patches 06 and 07 also preserve normal output while the abort flag remains
  disabled.

## The eight patches

| # | File | Effect |
|---|---|---|
| 01 | `01-jpeg-downscale-on-decode.patch` | Embedded JPEG images decode at the embedder's requested target size via libjpeg's `scale_num/scale_denom`, instead of always decoding at native size and downscaling the resulting bitmap. ~5-64× decode-time speedup at small render sizes. |
| 02 | `02-indexed-separation-fast.patch` | Adds fast `TranslateImageLine` overrides for `CPDF_IndexedCS` and `CPDF_SeparationCS` via precomputed BGR lookup tables. Replaces per-pixel virtual `GetRGB()` dispatch + float math with a 1-byte-index → 3-byte memcpy. Bit-identical output. ~50-200× per-scanline speedup. |
| 03 | `03-devicen-fast.patch` | Same fast-table pattern for `CPDF_DeviceNCS` (N=1 and N=2 component variants; N≥3 falls through to the existing slow path). Bit-identical output. ~50-200× per-scanline speedup. |
| 04 | `04-veloce-internal-access.patch` | Adds Veloce internal-access exports for graphics-state proof, rectangular clip detection, Form transparency groups, fast object iteration, and ABI sentinels. No rendering semantics change. |
| 05 | `05-veloce-skip-rasterization-probe.patch` | Adds the R12 diagnostic export `FPDFEx_SetSkipRasterization()`. When enabled, `FPDF_RenderPageBitmap()` walks page objects and cheap visibility rejection but skips `RenderSingleObject()`, producing a blank/unchanged bitmap for rasterizer-cost measurement. Default disabled. |
| 06 | `06-veloce-render-abort-probe.patch` | Adds the R18/R19 diagnostic export `FPDFEx_SetRenderAbort()`. When enabled, `CPDF_RenderStatus::RenderObjectList()` returns early at page-object boundaries so a cancelled progressive render can release xPDFSDK's single render lane. Default disabled. |
| 07 | `07-veloce-render-abort-deeper.patch` | Adds deeper abort checkpoints inside `RenderSingleObject()`, progressive image continuation, Form XObject recursion, transparency rendering, Type3 Form rendering, and soft-mask rendering. Default disabled through patch 06's abort flag. |
| 08 | `08-load-page-with-classification.patch` | Adds `FPDFEx_LoadPageWithClassification()` plus the fixed 64-byte `FPDFEx_PageClassification` ABI so xPDFSDK can route path-dense pages on the first render. No rendering semantics change. |
| 09 | `09-render-callbacks-scoped-cancel.patch` | Adds `FPDFEx_SetRenderCallbacks()` and a per-render scoped cancellation callback table layered on the existing 06/07 abort checkpoints. Existing `FPDFEx_SetRenderAbort()` remains supported. |

## Why this directory exists

During development the patches went through many iterations under
`patches/experiments/` (numbered 0001..0010). Many of those were
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
git apply patches/ship/05-veloce-skip-rasterization-probe.patch
git apply patches/ship/06-veloce-render-abort-probe.patch
git apply patches/ship/07-veloce-render-abort-deeper.patch
git apply patches/ship/08-load-page-with-classification.patch
git apply patches/ship/09-render-callbacks-scoped-cancel.patch
```

Patches 06 and 07 depend on patch 05 and must be applied after it.
Patch 07 also depends on patch 06. Patch 08 depends on the fpdfview export
surface after patches 05 and 06. Patch 09 depends on patches 06 and 07 because
it reuses their render-abort checkpoints. The release workflow applies the
numeric order shown above.

## Cumulative effect (measured)

On a representative image-heavy picture-book PDF, applying patches 01-03:

| Phase | Per-page render | Δ |
|---|---|---|
| Baseline (vanilla PDFium HEAD) | ~5,000 ms | — |
| With patches 01-03 | ~135 ms | **~37× faster** |

Visual quality: bit-identical output for patches 02 and 03; libjpeg
scale_denom path (patch 01) produces standard downsampled output per
the documented libjpeg API. Patch 04 does not affect rendering output.
Patch 05 does not affect rendering output unless its diagnostic toggle is
explicitly enabled. Patches 06 and 07 do not affect rendering output unless
the abort flag is explicitly enabled. Patch 08 does not alter
`FPDF_LoadPage()` behavior; callers must opt in by resolving and calling the
new `FPDFEx_LoadPageWithClassification()` symbol.

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
- **05 Veloce skip rasterization probe**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`,
  `core/fpdfapi/render/veloce_skip_rasterization.h`,
  `fpdfsdk/fpdf_view.cpp`,
  `public/fpdfview.h`.
- **06 Veloce render abort probe**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`,
  `core/fpdfapi/render/veloce_render_abort.h`,
  `fpdfsdk/fpdf_view.cpp`,
  `public/fpdfview.h`.
- **07 Veloce deeper render abort checkpoints**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`.
- **08 Veloce load-page classification**:
  `fpdfsdk/fpdf_view.cpp`, `public/fpdfview.h`.

Patches 02 and 03 both touch
`cpdf_colorspace.cpp`, but in different sections (SeparationCS vs.
DeviceNCS), so they coexist cleanly.
Patches 05, 06, and 07 touch `cpdf_renderstatus.cpp`. Patch 06 is authored
against the post-05 source, and patch 07 is authored against the post-06
source. Patch 05 skips rasterization for measurement; patch 06 exposes the
abort flag and checks top-level object boundaries; patch 07 checks deeper
phase boundaries inside `RenderSingleObject()`.
Patch 08 touches `fpdf_view.cpp` and `fpdfview.h`; it is authored after the
Veloce fpdfview exports from patches 05 and 06.

## Upstreaming

Patches 01-03 are independently suitable for upstream PRs to the PDFium
project. Patch 04 is intentionally xPDFSDK/Veloce-specific because it
exports internal PDFium state for a local rendering accelerator.
Patches 05, 06, and 07 are intentionally xPDFSDK/Veloce-specific and
diagnostic-only.

The patches in `experiments/` (0001–0009) are **NOT** upstreamable —
they include Android-specific ATrace dependencies and ad hoc diagnostic
debug toggles. The Veloce workflow uses this `ship/` directory because
patch 05 is a narrow, named, default-off probe with explicit artifact
provenance and symbol verification.
