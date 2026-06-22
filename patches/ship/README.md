# Production PDFium patches (ship-quality)

This directory contains the **production PDFium patch series**
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

## The patches

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
| 0011 | `0011-veloce-form-skip-mask.patch` | Adds `FPDFEx_SetRenderSkipMask(FPDFEX_RENDER_SKIP_FORM)` so callers can render a first pass that omits Form XObjects, then run a full pass later. Default mask is zero, so normal rendering is unchanged. |
| 0012 | `0012-veloce-image-skip-mask.patch` | Adds the `FPDFEX_RENDER_SKIP_HUGE_IMAGE` bit to the same skip mask plus `FPDFEx_SetHugeImagePixelThreshold()`. When set, any non-mask image whose decoded pixel count exceeds the threshold (default 4,000,000 px, ~2000×2000) is skipped and replaced with a flat gray placeholder rect, at any Form-recursion depth. Default mask is zero, so normal rendering is unchanged. |
| 0013 | `0013-veloce-path-display-list-form.patch` | Adds an experimental `FPDFEX_FEATURE_PATH_DISPLAY_LIST_FORM` feature bit. When set, eligible fill-only path Form XObjects are compiled into a compact internal display list and replayed into the current `CFX_RenderDevice`; unsupported content falls back before drawing. Default feature flags are zero, so normal rendering is unchanged. |
| 0014 | `0014-veloce-path-display-list-cache.patch` | Extends 0013 with a bounded process-local cache for compiled Form path display lists, keyed by document, holder, holder dictionary object number, and path-smoothing mode. Repeated renders of the same eligible Form replay from cache and log `cache=hit|miss`; compile cancellation returns `kCancelled` instead of falling back to legacy rendering. |
| 0015 | `0015-veloce-path-display-list-noop-clip.patch` | Extends 0014 with compile-time no-op clip normalization. If an eligible node's clip is one rectangle that contains the object's bounds, the display list stores `kNoClipKey` instead of replaying that clip state and logs `noopClips`. |
| 0016 | `0016-veloce-path-display-list-stacked-rect-noop-clip.patch` | Extends 0015 to support stacked rectangle-like clip components. It intersects all rectangle clips and elides the clip if that intersection contains the path object's bounds. |
| 0017 | `0017-veloce-holder-root-page-path-display-list.patch` | Extends the same native path display-list renderer from Form holders to root page holders. Path-only dense root pages such as `11.pdf` can now use native Veloce instead of the app-level Skia replay path, while Form-heavy pages such as `error.pdf` `/Meta20` continue through the same implementation. Adds `holderKind=root_page|form` telemetry. |
| 0018 | `0018-veloce-root-holder-context-layer.patch` | Corrects the root-page hook to identify top-level render-context layer holders instead of relying on `pObjectHolder->IsPage()`. This preserves the Form hook while allowing root path-only pages such as `11.pdf` to emit `holderKind=root_page`. |
| 0019 | `0019-veloce-progressive-root-path-display-list.patch` | Adds the missing root-page hook to `CPDF_ProgressiveRenderer::Continue()`, which is the Android `FPDF_RenderPageBitmap()` root-page path. This lets `11.pdf` emit `holderKind=root_page` instead of bypassing Veloce and walking every root page object. |
| 0020 | `0020-veloce-path-display-list-allow-fill-alpha.patch` | Allows ordinary fill alpha on fill-only path nodes. PDFium's normal path carries this through `GetFillArgb()` and `DrawPath()` rather than `ProcessTransparency()`, so Veloce can replay it while still rejecting soft masks and non-normal blend modes. |
| 0021 | `0021-veloce-path-display-list-stroke-replay.patch` | Adds native stroked-path replay. Stroke color and graph state are deduplicated in side tables and replayed through the same `DrawPath()` call PDFium uses for `ProcessPath()`. |
| 0022 | `0022-veloce-path-display-list-split-transparency-rejects.patch` | Splits the broad `transparency` reject telemetry into `soft_mask` and `blend_mode`. Rendering behavior is unchanged. |
| 0023 | `0023-veloce-path-display-list-darken-blend-replay.patch` | Allows `/BM /Darken` path nodes in the native Veloce path display list by rendering each blended node into a temporary BGRA bitmap and compositing it back through PDFium's existing `SetDIBitsWithBlend()` path. |

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
git apply patches/ship/0011-veloce-form-skip-mask.patch
git apply patches/ship/0012-veloce-image-skip-mask.patch
git apply patches/ship/0013-veloce-path-display-list-form.patch
git apply patches/ship/0014-veloce-path-display-list-cache.patch
git apply patches/ship/0015-veloce-path-display-list-noop-clip.patch
git apply patches/ship/0016-veloce-path-display-list-stacked-rect-noop-clip.patch
git apply patches/ship/0017-veloce-holder-root-page-path-display-list.patch
git apply patches/ship/0018-veloce-root-holder-context-layer.patch
git apply patches/ship/0019-veloce-progressive-root-path-display-list.patch
git apply patches/ship/0020-veloce-path-display-list-allow-fill-alpha.patch
git apply patches/ship/0021-veloce-path-display-list-stroke-replay.patch
git apply patches/ship/0022-veloce-path-display-list-split-transparency-rejects.patch
git apply patches/ship/0023-veloce-path-display-list-darken-blend-replay.patch
```

Patches 06 and 07 depend on patch 05 and must be applied after it.
Patch 07 also depends on patch 06. Patch 08 depends on the fpdfview export
surface after patches 05 and 06. Patch 09 depends on patches 06 and 07 because
it reuses their render-abort checkpoints. Patch 0011 depends on patch 09
because it adds a sibling `CPDF_RenderStatus` snapshot and is authored against
the post-09 render-status layout. Patch 0012 depends on patch 0011: it adds a
second bit (`FPDFEX_RENDER_SKIP_HUGE_IMAGE`) and threshold setter to the same
`render_skip_mask_`/`veloce_render_skip_mask.{h,cpp}` surface that 0011
introduced, and is authored against the post-0011 `cpdf_renderstatus.{h,cpp}`
layout. Patch 0013 depends on patch 09's render callback feature flags and
patch 0012's post-skip-mask render-status layout. It adds a default-off
path-only Form XObject display-list fast path for Meta20-class workloads.
Patch 0014 depends on patch 0013 and avoids repeated compile cost for cached
eligible Forms. Patch 0015 depends on patch 0014 and avoids repeated clip setup
cost for path nodes whose rectangular clip contains the node bounds.
Patch 0016 depends on patch 0015 and handles inherited rectangle clip stacks
by intersecting their bounds before deciding whether the clip is no-op.
Patch 0017 depends on patch 0016 and extends the display-list path from Form
holders to root page holders. Patch 0018 depends on patch 0017 and corrects
the root holder gate to use render-context layer identity. Patch 0019 depends
on patch 0018 and adds the same root holder attempt to PDFium's progressive
root-page renderer. Patch 0020 depends on patch 0019 and narrows the
transparency reject rule so normal fill alpha remains eligible. Patch 0021
depends on patch 0020 and adds stroked-path replay using graph-state side
tables. Patch 0022 depends on patch 0021 and only splits reject telemetry.
Patch 0023 depends on patch 0022 and allows the specific `/BM /Darken` blend
mode by routing blended path nodes through temporary bitmap compositing while
leaving other blend modes and soft masks in the legacy fallback path.
The release workflow applies the numeric order shown above.

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
- **09 Veloce scoped render callbacks**:
  `BUILD.gn`, `core/fpdfapi/render/BUILD.gn`,
  `core/fpdfapi/render/cpdf_renderstatus.{h,cpp}`,
  `core/fpdfapi/render/veloce_render_callbacks.{h,cpp}`,
  `fpdfsdk/BUILD.gn`, `fpdfsdk/fpdfex_render_callbacks.cpp`,
  `public/fpdfex_render_callbacks.h`.
- **0011 Veloce form skip mask**:
  `BUILD.gn`, `core/fpdfapi/render/BUILD.gn`,
  `core/fpdfapi/render/cpdf_renderstatus.{h,cpp}`,
  `core/fpdfapi/render/veloce_render_skip_mask.{h,cpp}`,
  `fpdfsdk/BUILD.gn`, `fpdfsdk/fpdfex_render_skip_mask.cpp`,
  `public/fpdfex_render_skip_mask.h`.
- **0012 Veloce image skip mask**:
  `core/fpdfapi/render/cpdf_renderstatus.{h,cpp}`,
  `core/fpdfapi/render/veloce_render_skip_mask.{h,cpp}`,
  `fpdfsdk/fpdfex_render_skip_mask.cpp`,
  `public/fpdfex_render_skip_mask.h`.
  No `BUILD.gn` changes — all touched files were already registered as build
  sources by patch 0011.
- **0013 Veloce path display-list Form fast path**:
  `core/fpdfapi/render/BUILD.gn`,
  `core/fpdfapi/render/cpdf_renderstatus.{h,cpp}`,
  `core/fpdfapi/render/veloce_path_display_list.{h,cpp}`,
  `public/fpdfex_render_callbacks.h`.
  The implementation is isolated in separate display-list files and is gated
  by `FPDFEX_FEATURE_PATH_DISPLAY_LIST_FORM`.
- **0014 Veloce path display-list cache**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds a bounded internal cache and `cache=hit|miss` telemetry. No public API
  or build file changes.
- **0015 Veloce path display-list no-op clips**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds no-op rectangular clip normalization and `noopClips` telemetry. No
  public API or build file changes.
- **0016 Veloce path display-list stacked rectangle no-op clips**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Extends no-op clip normalization to inherited rectangle clip stacks. No
  public API or build file changes.
- **0017 Veloce holder-level root page path display list**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`,
  `core/fpdfapi/render/veloce_path_display_list.{h,cpp}`.
  Adds the root-page `RenderObjectList()` hook and holder-kind telemetry. No
  public API or build file changes.
- **0018 Veloce root holder context layer gate**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`.
  Changes the root-page hook gate from `IsPage()` to render-context layer
  holder identity. No public API or build file changes.
- **0019 Veloce progressive root path display list**:
  `core/fpdfapi/render/cpdf_progressiverenderer.cpp`.
  Adds the root-page display-list attempt before the progressive renderer's
  root page-object loop. No public API or build file changes.
- **0020 Veloce path display-list fill alpha eligibility**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Allows normal fill alpha because replay uses `GetFillArgb()` and
  `DrawPath()`, while keeping soft-mask and non-normal blend fallback. No
  public API or build file changes.
- **0021 Veloce path display-list stroke replay**:
  `core/fpdfapi/render/cpdf_renderstatus.h`,
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds stroke colors plus deduplicated `CFX_GraphStateData` side-table replay,
  and removes the incorrect stroke-alpha-as-transparency reject. No public API
  or build file changes.
- **0022 Veloce path display-list reject diagnostics**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Splits broad transparency rejection into `soft_mask` and `blend_mode`
  telemetry. No rendering behavior, public API, or build file changes.
- **0023 Veloce path display-list Darken blend replay**:
  `core/fpdfapi/render/veloce_path_display_list.{h,cpp}`.
  Allows `BlendMode::kDarken` in eligibility, stores blend mode in the paint
  side table, logs `blendNodes`, and replays blended path nodes through a
  temporary BGRA render device plus `SetDIBitsWithBlend()`. No public API or
  build file changes.

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
Patch 09 and 0011 both extend the render-status surface and add public
extension headers/symbols. Patch 0011 is authored after patch 09.
Patch 0012 extends 0011's skip-mask surface with a second bit
(`FPDFEX_RENDER_SKIP_HUGE_IMAGE`) for per-image (rather than per-Form)
deferral, and is authored after 0011.
Patch 0013 extends patch 09's callback feature flags with
`FPDFEX_FEATURE_PATH_DISPLAY_LIST_FORM` and hooks only `ProcessForm()`. If a
Form contains anything outside the v1 eligibility envelope (non-path objects,
strokes, pattern paint, transparency, complex clips, or forced-color render
mode), it returns to the existing `RenderObjectList()` path before drawing.
Patch 0014 is authored after 0013 and only changes
`veloce_path_display_list.cpp`; it keeps the same feature flag and fallback
contract while avoiding repeated compile cost for cached eligible Forms.
Patch 0015 is authored after 0014 and only changes
`veloce_path_display_list.cpp`; it keeps the same feature flag and fallback
contract while avoiding repeated clip setup cost for path nodes whose clip
rectangle contains the node bounds.
Patch 0016 is authored after 0015 and only changes
`veloce_path_display_list.cpp`; it keeps the same feature flag and fallback
contract while handling multi-component rectangle clips created by inherited
Form/page clipping.
Patch 0017 is authored after 0016 and changes `cpdf_renderstatus.cpp` plus
`veloce_path_display_list.{h,cpp}`; it keeps the same feature flag and
all-or-nothing fallback contract while extending the holder fast path from
Form holders to root page holders.
Patch 0018 is authored after 0017 and changes only `cpdf_renderstatus.cpp`;
it keeps the same all-or-nothing fallback contract while making the root
holder check match the actual `CPDF_RenderContext::Render()` layer boundary.
Patch 0019 is authored after 0018 and changes only
`cpdf_progressiverenderer.cpp`; it keeps the same all-or-nothing fallback
contract while covering the Android `FPDF_RenderPageBitmap()` root render path.
Patch 0020 is authored after 0019 and changes only
`veloce_path_display_list.cpp`; it keeps the all-or-nothing fallback contract
for real unsupported transparency while allowing ordinary fill alpha.
Patch 0021 is authored after 0020 and changes only
`cpdf_renderstatus.h` plus `veloce_path_display_list.cpp`; it keeps the same
fallback contract while allowing dense path-only pages whose paths are stroked.
Patch 0022 is authored after 0021 and changes only
`veloce_path_display_list.cpp`; it preserves fallback behavior while making
the next unsupported feature visible in logs.
Patch 0023 is authored after 0022 and changes only
`veloce_path_display_list.{h,cpp}`; it keeps `kNotEligible` as a pre-draw
fallback result and returns `kCancelled` for any replay-time blend failure so
callers do not draw legacy content over partial Veloce output.

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
