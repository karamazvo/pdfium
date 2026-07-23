# xPDFSDK PDFium Patch Status

**Last updated:** 2026-07-22

## TL;DR

To build the **Veloce release** Android arm64 PDFium used by the
visible-first / VIR experiments:

> Run workflow: **`Release PDFium Android arm64 Veloce`**
>
> File: `.github/workflows/release-pdfium-android-arm64-veloce.yml`
> Artifact: `libpdfium-android-arm64-veloce-release`

This applies the seven ship patches under `patches/ship/`: the three
production render-speed patches, the Veloce internal-access exports,
the R12 skip-rasterization probe, the R18/R19 render-abort probe, and
the deeper RenderSingleObject abort checkpoints.
No diagnostic ATrace markers are included. The skip-rasterization and
render-abort toggles are default-off, so normal PDFium rendering is
unchanged unless xPDFSDK explicitly enables them.

For a minimal end-user-only PDFium without Veloce internal access or
diagnostic toggles, use `release-pdfium-android-arm64-agg.yml` instead.

To build the next Veloce classification binary with the new
`FPDFEx_LoadPageWithClassification` export:

> Run workflow: **`libpdfium patch build`**
>
> File: `.github/workflows/libpdfium_patch_build.yml`
> Artifact: `libpdfium-android-arm64-veloce-classification-release`

This applies the same seven Veloce patches plus
`patches/ship/08-load-page-with-classification.patch`, verifies the new
symbol in both `libpdfium.a` and the final `libpdfium.so`, and verifies the
`FPDFEx_PageClassification` ABI remains 64 bytes.

To build the r5 scoped-cancel binary:

> Run workflow: **`Build patched PDFium Android arm64 r5 callbacks`**
>
> File: `.github/workflows/pdfium-android-arm64-r5-callbacks.yml`
> Artifact: `libpdfium-android-arm64-r5-callbacks`

This applies ship patches `01..09`: the previous Veloce stack plus
`patches/ship/09-render-callbacks-scoped-cancel.patch`, which adds
`FPDFEx_SetRenderCallbacks()` while preserving the existing
`FPDFEx_SetRenderAbort()` bridge.

To build the r7 path-display-list binary:

> Run workflow: **`Build patched PDFium Android arm64 r7 path display list`**
>
> File: `.github/workflows/pdfium-android-arm64-r7-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r7-path-display-list`

This applies ship patches `01..09,0011,0012,0013`. Patch 0013 adds the
default-off `FPDFEX_FEATURE_PATH_DISPLAY_LIST_FORM` feature bit and a
separate native path display-list implementation for eligible fill-only
path Form XObjects such as `/Meta20`. Unsupported content falls back to
PDFium's original `RenderObjectList()` path before drawing.

To build the r28 ordered text-passthrough path-display-list binary:

> Run workflow: **`r28 ordered text passthrough path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r28-ordered-text-passthrough-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r28-ordered-text-passthrough-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034`.
Patch 0034 keeps the existing Veloce path/stroke/blend replay kernels and adds
ordered `PathRun` + text-passthrough segments so one text object no longer
rejects an otherwise accelerable path-dense holder. Image/Form/shading
passthrough remains unsupported in r28 and rejects before drawing.

To build the r29 stroke-run flush telemetry path-display-list binary:

> Run workflow: **`r29 stroke-run flush telemetry path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r29-stroke-run-flush-telemetry-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r29-stroke-run-flush-telemetry-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035`.
Patch 0035 is telemetry-only. It keeps r28 rendering behavior unchanged and
adds stroke-run flush reason counters so Q16-style pages can explain why a
low-paint-count page still produces many stroke run draw calls.

To build the r30 non-overlap fill-barrier stroke-packing path-display-list binary:

> Run workflow: **`r30 non-overlap fill-barrier stroke packing path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r30-nonoverlap-fill-barrier-stroke-packing-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r30-nonoverlap-fill-barrier-stroke-packing-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035,0036`.
Patch 0036 lets a pending stroke run stay open across a normal non-stroke path
only when expanded device-space bounds prove the barrier is disjoint from the
pending stroke run. Overlapping or unknown barriers still flush exactly as r29.

To build the r31 holder-space spatial-index path-display-list binary:

> Run workflow: **`r31 holder-space spatial index path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r31-holder-space-spatial-index-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r31-holder-space-spatial-index-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035,0036,0037`.
Patch 0037 builds a 32x32 holder-space spatial index once with the cached
native path display list. Tile replay transforms the device clip back into
holder space, queries candidate bins, sorts candidate node ids back into
display-list order, and then reuses the existing segment/stroke/blend replay.
Broad preview clips fall back to the old sequential scan.

To build the r32 text-passthrough cache UAF fix path-display-list binary:

> Run workflow: **`r32 text passthrough cache UAF fix path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r32-text-passthrough-cache-uaf-fix-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r32-text-passthrough-cache-uaf-fix-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035,0036,0037,0038`.
Patch 0038 fixes the r31 text-passthrough cache lifetime hazard: ordered text
passthrough segments keep raw `CPDF_PageObject*` pointers for current-holder
replay, so display lists containing text passthrough are no longer inserted into
the process cache. Path-only cached display lists remain cacheable.

To build the r33 text-passthrough index cache path-display-list binary:

> Run workflow: **`r33 text passthrough index cache path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r33-text-passthrough-index-cache-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r33-text-passthrough-index-cache-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035,0036,0037,0038,0039`.
Patch 0039 replaces r32's conservative text-passthrough cache disable with a
cache-safe representation: cached text passthrough segments store holder object
indexes, replay resolves those indexes from the current live holder before any
drawing, and process-cache insertion is restored for Q16-style text-barrier
pages. If the current holder cannot resolve the expected text barrier indexes,
Veloce returns `not_eligible` before drawing instead of dereferencing stale
page-object pointers.

To build the r34 fill-barrier telemetry path-display-list binary:

> Run workflow: **`r34 fill barrier telemetry path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r34-fill-barrier-telemetry-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r34-fill-barrier-telemetry-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035,0036,0037,0038,0039,0040`.
Patch 0040 is telemetry-only. It adds compact `VelocePathDLFill` log lines for
normal barriers that are not pure stroke-run nodes and break Q16-style packing, including
fill-only/fill+stroke counts, fill rule, rect-like/thin/empty device bounds,
path point counts, and pending-stroke overlap/disjoint/unknown counts. Rendering
order, stroke-run packing decisions, cancellation, and fallback behavior are
unchanged.

To build the r37 blend group cancellation path-display-list binary:

> Run workflow: **`r37 blend group cancellation path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r37-blend-group-cancellation-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r37-blend-group-cancellation-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035,0036,0037,0038,0039,0040,0041`.
Patch 0041 keeps successful `BlendGroupRun` output unchanged, but checks
`ShouldCancelRender()` inside group-buffer blend flushes. Obsolete high-zoom
tiles can now abort during allocation/raster/composite work instead of blocking
until a whole blend run completes.

To build the r38 blend-run widening telemetry path-display-list binary:

> Run workflow: **`r38 blend run widening telemetry path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r38-blend-run-widening-telemetry-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r38-blend-run-widening-telemetry-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035,0036,0037,0038,0039,0040,0041,0042`.
Patch 0042 is telemetry-only. It adds `VelocePathDLBlend` and group-buffer
pixel counters so `11.pdf`-style successful tile cost can be explained before
any safe multi-paint blend-run widening is attempted.

To build the r39 primitive-run telemetry path-display-list binary:

> Run workflow: **`r39 primitive run telemetry path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r39-primitive-run-telemetry-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r39-primitive-run-telemetry-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035,0036,0037,0038,0039,0040,0041,0042,0043`.
Patch 0043 is telemetry-only. It precomputes point/subpath stats for each
cached path, then reports candidate/culled/drawn primitive totals in
`VelocePathDLPrimitive`. This is the measurement bridge for a future
MuPDF-style primitive spatial index: it tells us whether dense Q16/Error tiles
still spend time because one visible path node contains too many internal
subpaths or points.

To build the r40 fill-barrier proof telemetry path-display-list binary:

> Run workflow: **`r40 fill barrier proof telemetry path display-list - Build patched PDFium Android arm64`**
>
> File: `.github/workflows/pdfium-android-arm64-r40-fill-barrier-proof-telemetry-path-display-list.yml`
> Artifact: `libpdfium-android-arm64-r40-fill-barrier-proof-telemetry-path-display-list`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023,0024,0025,0026,0029,0030,0031,0032,0034,0035,0036,0037,0038,0039,0040,0041,0042,0043,0044`.
Patch 0044 is telemetry-only. It adds `VelocePathDLFillProof` lines that
classify fill barriers still blocking stroke-run packing by no-pixel, thin,
rect-like, same-color, and coarse device-rect containment predicates. It does
not cross any additional barriers; it only measures whether r41/r42 can safely
promote a narrow pixel-equivalent crossing rule.

To build the r9 path-display-list no-op clip binary:

> Run workflow: **`Build patched PDFium Android arm64 r9 path display-list no-op clip`**
>
> File: `.github/workflows/pdfium-android-arm64-r9-path-display-list-noop-clip.yml`
> Artifact: `libpdfium-android-arm64-r9-path-display-list-noop-clip`

This applies ship patches `01..09,0011,0012,0013,0014,0015`. Patch 0015
keeps the same default-off path display-list feature flag, but normalizes
eligible path nodes whose clip is a single rectangle containing the object's
own bounds to `kNoClipKey`. On Meta20-class pages, the expected validation
signal is high `noopClips`, much lower `clips` / `clipSwitches`, and lower
`replayMs` in the `VelocePathDL` log line.

To build the r10 path-display-list stacked-rectangle no-op clip binary:

> Run workflow: **`Build patched PDFium Android arm64 r10 path display-list stacked rect no-op clip`**
>
> File: `.github/workflows/pdfium-android-arm64-r10-path-display-list-stacked-rect-noop-clip.yml`
> Artifact: `libpdfium-android-arm64-r10-path-display-list-stacked-rect-noop-clip`

This applies ship patches `01..09,0011,0012,0013,0014,0015,0016`. Patch 0016
extends R9's no-op clip detector from a single rectangle path to stacked
rectangle-like clip components. This targets the decoded `/Meta20` stream shape
seen in `error.pdf`, where inherited Form/page clips can be represented as a
multi-component `CPDF_ClipPath`.

**Release build size: ~6.6 MB** (down from a ~13.7 MB baseline).
This is achieved by the shrink link strategy promoted from the
size-experiment workflow's Variant C:

  - `pdf_use_partition_alloc = false` in args.gn (drops Chromium's
    allocator shim; PDFium uses the libc allocator).
  - `default_min_sdk_version = 23` (avoids `__INTRODUCED_IN_<api>`
    shim symbols, matching bblanchon/pdfium-binaries).
  - At link time: drop `--whole-archive`, add `--gc-sections`, and
    auto-build a `--undefined=<sym>` list from every `FPDF_*`,
    `FORM_*`, `FSDK_*` symbol defined in `libpdfium.a` (via
    `llvm-nm`). The linker then GC's only code unreachable from
    that root set — bit-identical content, ~50% size reduction.

This roughly matches bblanchon/pdfium-binaries (~6.4 MB) for the
same configuration; the ~200 KB delta is PDFium revision drift and
third-party version pins, not actionable from our side.

**Per-PDFium-roll cost:** zero. The `--undefined` set is rebuilt
from `llvm-nm` on every build, so newly added FPDF/FORM/FSDK APIs
are automatically preserved. A 24-API survival check in the link
script (and another in the post-link strip step) fails the build
loudly if any canary API is GC'd. If that check ever fires, fix
the root cause — do not add ad-hoc symbol exceptions.

For a build with diagnostic trace markers (useful for future
performance investigations, but slightly larger `.so`):

> Run workflow: **`Build PDFium Android arm64 AGG + DeviceN Fast (0009)`**
>
> File: `.github/workflows/build-pdfium-android-arm64-agg-devicen-fast.yml`
> Artifact: `libpdfium-android-arm64-agg-devicen-fast`

This applies the experiment-numbered patches (0003 + 0004 + 0005 + 0007
+ 0008 + 0009) including all the `pdfium.*` / `pdfium009.*` ATrace markers.
Zero-cost at runtime when tracing is disabled, but adds string literals
and the `libandroid.so` runtime dependency.

---

## Patch table

| # | Status | Patch file | What it does |
|---|---|---|---|
| 0001 | DELETED | (removed) | Initial JPEG downscale -- superseded |
| 0002 | DELETED | (removed) | JPEG downscale v2 -- superseded by 0003 |
| **0003** | **SHIP** | `0003-jpeg-downscale-on-decode.patch` | Plumb `resolution_levels_to_skip` into JPEG decoder. Engages libjpeg's `scale_num/scale_denom` so embedded JPEGs decode at target size, not native. ~5-10x speedup on image-heavy PDFs at small render sizes. |
| 0004 | DIAGNOSTIC | `0004-native-trace-markers.patch` | Top-level ATrace markers (`pdfium.*`). Zero-cost when tracing disabled; lets you capture Perfetto traces from production. Safe to ship. |
| 0005 | DIAGNOSTIC | `0005-native-trace-markers-deep.patch` | Deeper markers inside CPDF_DIB load + scanline path. Same zero-cost-when-off property. Safe to ship. |
| 0006 | DIAGNOSTIC ONLY -- **DO NOT SHIP** | `0006-icc-skip-shortcut.patch` | Debug-gated bypass of ICC color transform. Produces wrong colors on CMYK content when enabled. Used to prove ICC-style per-pixel work was the bottleneck on some PDFs. Disabled by default. |
| 0007 | DIAGNOSTIC | `0007-colorspace-dispatch-markers.patch` | Colorspace-dispatch markers (`pdfium.IndexedCS::*`, `pdfium.ICCBasedCS::*`, etc.). Pinpointed `CPDF_ColorSpace::TranslateImageLine.baseSlow` as the hot path. Safe to ship. |
| **0008** | **SHIP** | `0008-fast-indexed-separation-cs.patch` | Fast `CPDF_IndexedCS::TranslateImageLine` + `CPDF_SeparationCS::TranslateImageLine` overrides. Precomputed BGR lookup tables (max 768 bytes per CS). Bit-identical output. |
| **0009** | **SHIP** | `0009-devicen-fast-and-prefixed-markers.patch` | Fast `CPDF_DeviceNCS::TranslateImageLine` override (table for N=1/N=2, fall through for N>=3). Plus `pdfium009.*` prefixed markers so traces verify which build is running. This was the patch that finally hit the dominant bottleneck on the test picture-book PDF: ~5000ms/page render -> ~135ms/page render. |
| ship 04 | VELOCE SHIP | `patches/ship/04-veloce-internal-access.patch` | Thin C exports for page-object metadata needed by VIR admission and replay: blend/alpha/soft-mask/form-group/page-object iteration, plus ABI sentinels. |
| ship 05 | VELOCE DIAGNOSTIC | `patches/ship/05-veloce-skip-rasterization-probe.patch` | Default-off `FPDFEx_SetSkipRasterization()` probe. Walks page objects but skips `RenderSingleObject()` to isolate rasterizer cost. |
| ship 06 | VELOCE DIAGNOSTIC | `patches/ship/06-veloce-render-abort-probe.patch` | Default-off `FPDFEx_SetRenderAbort()` probe. Lets xPDFSDK ask PDFium to return early at `RenderObjectList()` page-object boundaries after a progressive render is cancelled. |
| ship 07 | VELOCE DIAGNOSTIC | `patches/ship/07-veloce-render-abort-deeper.patch` | Extends the default-off render-abort probe deeper into `RenderSingleObject()`, progressive image continuation, Form XObject recursion, transparency rendering, Type3 Form rendering, and soft-mask rendering. |
| ship 08 | VELOCE SHIP | `patches/ship/08-load-page-with-classification.patch` | Adds `FPDFEx_LoadPageWithClassification()` and the 64-byte `FPDFEx_PageClassification` ABI so xPDFSDK can classify pages at load time and route path-dense pages before the first render. |
| ship 09 | VELOCE SHIP | `patches/ship/09-render-callbacks-scoped-cancel.patch` | Adds `FPDFEx_SetRenderCallbacks()` and a per-render scoped cancellation callback table layered on the existing 06/07 abort checkpoints. |
| ship 0011 | VELOCE SHIP | `patches/ship/0011-veloce-form-skip-mask.patch` | Adds the default-off `FPDFEX_RENDER_SKIP_FORM` skip-mask bit for Form-omitting first-pass experiments. |
| ship 0012 | VELOCE SHIP | `patches/ship/0012-veloce-image-skip-mask.patch` | Adds the default-off `FPDFEX_RENDER_SKIP_HUGE_IMAGE` skip-mask bit and threshold setter for huge image placeholder first-pass experiments. |
| ship 0013 | VELOCE EXPERIMENTAL | `patches/ship/0013-veloce-path-display-list-form.patch` | Adds `FPDFEX_FEATURE_PATH_DISPLAY_LIST_FORM` and an isolated path display-list fast path hooked from `ProcessForm()`. Eligible fill-only path Forms replay directly into the same render device; unsupported content falls back to normal PDFium rendering before drawing. |
| ship 0014 | VELOCE EXPERIMENTAL | `patches/ship/0014-veloce-path-display-list-cache.patch` | Extends the path display-list fast path with a bounded process-local cache keyed by document, Form holder, holder dictionary object number, and path-smoothing mode. Subsequent renders log `cache=hit` and skip visible-lane compile cost. |
| ship 0015 | VELOCE EXPERIMENTAL | `patches/ship/0015-veloce-path-display-list-noop-clip.patch` | Normalizes single-rectangle clips that contain the path object's bounds to `kNoClipKey` during display-list compile. This avoids replaying thousands of no-op clip state transitions and logs `noopClips`. |
| ship 0016 | VELOCE EXPERIMENTAL | `patches/ship/0016-veloce-path-display-list-stacked-rect-noop-clip.patch` | Extends 0015 to treat stacked rectangle-like clip paths as one intersection. If the intersection contains the path object's bounds, the display-list node stores `kNoClipKey`. |
| ship 0017 | VELOCE EXPERIMENTAL | `patches/ship/0017-veloce-holder-root-page-path-display-list.patch` | Extends native Veloce path display-list rendering from Form holders to root page holders. Path-only dense root pages such as `11.pdf` can render through the same native compiler/cache/replay path as `error.pdf` `/Meta20`, with `holderKind=root_page|form` telemetry. |
| ship 0018 | VELOCE EXPERIMENTAL | `patches/ship/0018-veloce-root-holder-context-layer.patch` | Fixes the r11 root hook to identify root renders by `CPDF_RenderContext` layer-holder identity instead of `pObjectHolder->IsPage()`. This preserves the Form path and lets root path-only pages such as `11.pdf` enter `holderKind=root_page`. |
| ship 0019 | VELOCE EXPERIMENTAL | `patches/ship/0019-veloce-progressive-root-path-display-list.patch` | Adds the missing root holder hook to `CPDF_ProgressiveRenderer::Continue()`, the Android `FPDF_RenderPageBitmap()` root-page path. This lets path-only root pages such as `11.pdf` enter `holderKind=root_page` before the progressive per-object loop. |
| ship 0020 | VELOCE EXPERIMENTAL | `patches/ship/0020-veloce-path-display-list-allow-fill-alpha.patch` | Narrows the transparency reject rule so ordinary fill alpha remains eligible for fill-only path replay. Soft masks and non-normal blend modes still fall back to PDFium. |
| ship 0021 | VELOCE EXPERIMENTAL | `patches/ship/0021-veloce-path-display-list-stroke-replay.patch` | Adds native stroked-path replay to the Veloce path display list. Stroke color and graph state are stored in side tables and replayed through PDFium's existing `DrawPath()` primitive; soft masks, blend modes, and pattern paint still fall back. |
| ship 0022 | VELOCE EXPERIMENTAL | `patches/ship/0022-veloce-path-display-list-split-transparency-rejects.patch` | Splits the broad path display-list `transparency` rejection into `soft_mask` and `blend_mode` telemetry. Rendering behavior is unchanged; this identifies pages like `11.pdf` as blend-mode blocked. |
| ship 0023 | VELOCE EXPERIMENTAL | `patches/ship/0023-veloce-path-display-list-darken-blend-replay.patch` | Allows `/BM /Darken` path nodes in the native Veloce path display list. Darken paths render into a temporary BGRA bitmap and composite back into the same PDFium render device through `SetDIBitsWithBlend()`, preserving blend semantics instead of treating Darken as normal paint. |
| ship 0033 | REVERTED | _(not shipped)_ | Reserved for the reverted group-buffer resolution-cap experiment. Do not reuse this number; r28 intentionally applies `0034` after `0032`. |
| ship 0034 | VELOCE EXPERIMENTAL | `patches/ship/0034-veloce-path-display-list-ordered-text-passthrough.patch` | Adds ordered path/text segment replay on top of the existing path display-list stack. Text objects are rendered via `RenderSingleObject()` at their original painter-order position, then clip state is cleared so the next accelerated path segment installs its own clip. |
| ship 0035 | VELOCE TELEMETRY | `patches/ship/0035-veloce-path-display-list-stroke-run-flush-telemetry.patch` | Adds telemetry-only stroke-run flush reason counters (`strokeRunFlushPaint`, `strokeRunFlushColor`, `strokeRunFlushGraphState`, `strokeRunFlushPathStyle`, `strokeRunFlushFillMode`, `strokeRunFlushClip`, `strokeRunFlushBlend`, `strokeRunFlushSegment`, `strokeRunFlushCapacity`, `strokeRunFlushEnd`) without changing replay decisions. |
| ship 0036 | VELOCE EXPERIMENTAL | `patches/ship/0036-veloce-path-display-list-nonoverlap-fill-barrier-stroke-packing.patch` | Allows stroke-run packing to cross normal non-stroke path barriers only when expanded clipped device bounds prove the pending stroke run and barrier are disjoint. Adds `strokeRunFillBarriersCrossed` and `strokeRunFillBarriersBlocked` telemetry. |
| ship 0037 | VELOCE EXPERIMENTAL | `patches/ship/0037-veloce-path-display-list-holder-space-spatial-index.patch` | Builds a cached holder-space 32x32 spatial index for path display-list nodes and uses it to query tile candidates before segment replay. Candidate ids are sorted back into original display-list order; segment/text/blend/clip barriers remain the correctness boundary. Adds `spatialIndex*` telemetry. |
| ship 0038 | VELOCE EXPERIMENTAL | `patches/ship/0038-veloce-path-display-list-disable-text-passthrough-cache.patch` | Temporarily disables process-cache insertion for display lists containing raw text-passthrough pointers to avoid cache-hit use-after-free. Superseded by 0039 but kept in sequence for patch history. |
| ship 0039 | VELOCE EXPERIMENTAL | `patches/ship/0039-veloce-path-display-list-text-passthrough-index-cache.patch` | Replaces raw text-passthrough pointers with holder object indexes, resolves live text objects before replay, and restores cache insertion for Q16-style text-barrier pages. |
| ship 0040 | VELOCE TELEMETRY | `patches/ship/0040-veloce-path-display-list-fill-barrier-telemetry.patch` | Emits compact `VelocePathDLFill` logs explaining normal non-stroke path barriers that break Q16 stroke-run packing. Telemetry-only. |
| ship 0041 | VELOCE EXPERIMENTAL | `patches/ship/0041-veloce-path-display-list-blend-group-cancellation.patch` | Adds cooperative cancellation checkpoints inside `BlendGroupRun` group-buffer flushes. Successful output is unchanged; cancelled tiles return `kCancelled` and are discarded. |
| ship 0042 | VELOCE TELEMETRY | `patches/ship/0042-veloce-path-display-list-blend-run-widening-telemetry.patch` | Emits `VelocePathDLBlend` group-buffer pixel cost and blend paint-barrier opportunity counters for future safe `11.pdf` blend-run widening. Telemetry-only. |
| ship 0043 | VELOCE TELEMETRY | `patches/ship/0043-veloce-path-display-list-primitive-run-telemetry.patch` | Precomputes per-path point/subpath stats and logs candidate/culled/drawn primitive totals through `VelocePathDLPrimitive`, without changing replay or drawing. This decides whether the next MuPDF-style primitive spatial index is worth building. |
| ship 0044 | VELOCE TELEMETRY | `patches/ship/0044-veloce-path-display-list-fill-barrier-proof-telemetry.patch` | Emits `VelocePathDLFillProof` counters for blocked fill barriers, measuring no-pixel, thin, rect-like, same-color, and device-rect containment predicates. Telemetry-only; no new barrier crossing behavior. |
| ship 0075 | R25-1 FOUNDATION | `patches/ship/0075-veloce-render-program-v2-ownership-boundary.patch` | Establishes holder-owned immutable RenderProgram lifetime; live PDFium objects remain the fidelity/editing source of truth. |
| ship 0076 | R25-1 FOUNDATION | `patches/ship/0076-veloce-render-program-parser-command-order.patch` | Records one command per parsed holder object in exact painter order during the existing parser append pass. |
| ship 0079 | R25-1 FOUNDATION | `patches/ship/0079-veloce-unified-render-program-backend-interface.patch` | Defines the fail-closed unified backend and benchmark contract without changing pixels. |
| ship 0080 | R25-1 FOUNDATION | `patches/ship/0080-veloce-render-program-compact-command-summary.patch` | Adds O(1) command-kind counts without rescanning page objects. |
| ship 0081 | R25-1 CORRECTNESS | `patches/ship/0081-veloce-disable-legacy-path-display-list.patch` | Makes canonical PDFium the sole fallback pixel owner and disables the legacy PathDL consumer before work. |
| ship 0082 | R25-1 INDEX | `patches/ship/0082-veloce-render-program-holder-space-candidate-index.patch` | Adds a bounded, fail-closed 32x32 holder-space command index built during parsing. |
| ship 0083 | R25-1 EXECUTOR | `patches/ship/0083-veloce-render-program-exact-path-text-executor.patch` | Executes validated ordered path/text candidates into PDFium's existing destination device. |
| ship 0084 | SKIPPED | _(not emitted)_ | Reserved; do not reuse this revision number. |
| ship 0085 | R25-1 EXECUTOR | `patches/ship/0085-veloce-render-program-direct-path-dispatch.patch` | Adds fail-closed direct path dispatch while retaining canonical barriers for unsupported state. |
| ship 0086 | R25-1 INDEX | `patches/ship/0086-veloce-render-program-streaming-candidate-cursor.patch` | Replaces dynamic candidate vectors/caps with an allocation-free ordered cursor and exact dense linear replay. |
| ship 0087 | R25-1 EXECUTOR | `patches/ship/0087-veloce-render-program-exact-path-state-packets.patch` | Groups proven stroke paths into bounded exact-state packets without merging geometry or changing per-path raster/composite order. |
| ship 0088 | R25-1 TELEMETRY | `patches/ship/0088-veloce-render-program-cost-attribution.patch` | For Android holders already admitted at 4096 commands, emits one bounded `VeloceRenderProgram` summary with admitted/index lifetime, query/replay time, raw postings, bins, candidates, visited/drawn commands, packet flushes, fallback reason, and memory. Normal holders remain unmeasured; rendering policy and pixels are unchanged. |
| ship 0089 | R25-1 EXECUTOR | `patches/ship/0089-veloce-render-program-exact-darken-spans.patch` | For proven fill-only or stroke-only Darken paths, sends each object's AGG coverage directly to PDFium's existing Darken scanline compositor. This removes the per-object BGRA transparency bitmap and second pixel traversal without merging objects or changing painter order, rasterization, or integer blend semantics. Unsupported cases fail closed before destination mutation. |

---

## Workflow table

| Workflow file | Trigger | Status |
|---|---|---|
| `release-pdfium-android-arm64-agg.yml` | `workflow_dispatch` | **CURRENT PRODUCTION RELEASE BUILD.** Applies only the 3 ship patches (`patches/ship/01..03`). Uses the Variant C shrink link config (`pdf_use_partition_alloc=false`, `default_min_sdk_version=23`, `--gc-sections` + auto-`--undefined` for `FPDF_*`/`FORM_*`/`FSDK_*`). Minimal `.so` (~6.6 MB), no diagnostic markers, no `libandroid.so` dependency. Audit step rejects the build if any `XPDF_ATRACE_SCOPED` leaks in; canary-API survival check rejects the build if `--gc-sections` ever drops a public entry point. |
| `release-pdfium-android-arm64-veloce.yml` | `workflow_dispatch` | **CURRENT VELOCE RELEASE BUILD.** Applies ship patches `01..07`, including internal-access exports, skip-rasterization, and render-abort probes. Use this for VIR admission/replay work and cancellation experiments. |
| `libpdfium_patch_build.yml` | `workflow_dispatch` | **NEXT VELOCE CLASSIFICATION BUILD.** Applies ship patches `01..08`, including `FPDFEx_LoadPageWithClassification`. Verifies all Veloce symbols, the new classification export, and the 64-byte classification struct ABI before packaging `libpdfium.so`. |
| `pdfium-android-arm64-r5-callbacks.yml` | `workflow_dispatch` | **R5 SCOPED-CANCEL BUILD.** Applies ship patches `01..09`, including the scoped render callback API. Verifies `FPDFEx_SetRenderAbort` and `FPDFEx_SetRenderCallbacks` in the final shared object. |
| `pdfium-android-arm64-r6-image-skip.yml` | `workflow_dispatch` | **R6 IMAGE-SKIP BUILD.** Applies ship patches `01..09,0011,0012`, including Form skip-mask and huge-image placeholder deferral. Verifies skip-mask APIs and image deferral hooks before packaging. |
| `pdfium-android-arm64-r7-path-display-list.yml` | `workflow_dispatch` | **R7 PATH DISPLAY-LIST BUILD.** Applies ship patches `01..09,0011,0012,0013`, including the default-off path-only Form display-list fast path. Verifies the feature flag, `ProcessForm()` hook, separate implementation file, and cancellation checkpoint token before packaging. |
| `pdfium-android-arm64-r8-path-display-list-cache.yml` | `workflow_dispatch` | **R8 PATH DISPLAY-LIST CACHE BUILD.** Applies ship patches `01..09,0011,0012,0013,0014`, including cached eligible Form path display lists. Verifies cache symbols and `cache=hit|miss` telemetry before packaging. |
| `pdfium-android-arm64-r9-path-display-list-noop-clip.yml` | `workflow_dispatch` | **R9 PATH DISPLAY-LIST NO-OP CLIP BUILD.** Applies ship patches `01..09,0011,0012,0013,0014,0015`, including no-op rectangular clip normalization for eligible path display-list nodes. Verifies `noopClips` telemetry before packaging. |
| `pdfium-android-arm64-r10-path-display-list-stacked-rect-noop-clip.yml` | `workflow_dispatch` | **R10 PATH DISPLAY-LIST STACKED RECT NO-OP CLIP BUILD.** Applies ship patches `01..09,0011,0012,0013,0014,0015,0016`, including stacked rectangle clip normalization. Verifies the rectangle-intersection code token before packaging. |
| `pdfium-android-arm64-r11-holder-path-display-list.yml` | `workflow_dispatch` | **R11 HOLDER PATH DISPLAY-LIST BUILD.** Applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017`, including the root-page holder Veloce hook for path-only dense pages such as `11.pdf`. Verifies `holderKind` telemetry and root/form call sites before packaging. |
| `pdfium-android-arm64-r12-holder-context-root-path-display-list.yml` | `workflow_dispatch` | **R12 HOLDER CONTEXT-ROOT PATH DISPLAY-LIST BUILD.** Applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018`, including the corrected render-context layer identity gate for root path-only pages. Verifies `context_->CountLayers()` and holder-kind telemetry before packaging. |
| `pdfium-android-arm64-r13-progressive-root-path-display-list.yml` | `workflow_dispatch` | **R13 PROGRESSIVE ROOT PATH DISPLAY-LIST BUILD.** Applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019`, including the progressive renderer root-page hook required by `FPDF_RenderPageBitmap()`. Verifies the `cpdf_progressiverenderer.cpp` Veloce root call before packaging. |
| `pdfium-android-arm64-r14-path-alpha-display-list.yml` | `workflow_dispatch` | **R14 PATH ALPHA DISPLAY-LIST BUILD.** Applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020`, including the normal fill-alpha eligibility fix for native Veloce root/path replay. Verifies fill alpha is no longer a hard reject before packaging. |
| `pdfium-android-arm64-r15-stroke-path-display-list.yml` | `workflow_dispatch` | **R15 STROKE PATH DISPLAY-LIST BUILD.** Applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021`, including native stroked-path replay for dense root pages such as `11.pdf`. Verifies stroke alpha is replayed through `DrawPath()` instead of rejected as transparency. |
| `pdfium-android-arm64-r16-transparency-reject-diagnostics.yml` | `workflow_dispatch` | **R16 TRANSPARENCY REJECT DIAGNOSTICS BUILD.** Applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022`. Rendering behavior is unchanged, but `VelocePathDL` now reports `reason=blend_mode` or `reason=soft_mask` instead of the broad `reason=transparency`. |
| `pdfium-android-arm64-r17-darken-blend-path-display-list.yml` | `workflow_dispatch` | **R17 DARKEN BLEND PATH DISPLAY-LIST BUILD.** Applies ship patches `01..09,0011,0012,0013,0014,0015,0016,0017,0018,0019,0020,0021,0022,0023`. This is the first correctness-preserving native Veloce attempt for `11.pdf`-class `/BM /Darken` dense path pages. Expected log: `holderKind=root_page`, `result=rendered`, and nonzero `blendNodes`. |
| `pdfium-android-arm64-r28-ordered-text-passthrough-path-display-list.yml` | `workflow_dispatch` | **R28 ORDERED TEXT PASSTHROUGH PATH DISPLAY-LIST BUILD.** Applies ship patches through `0034`, skipping deprecated `0027/0028`. The workflow name starts with `r28` so it is easy to spot in GitHub Actions. Expected log: nonzero `pathSegments` and `textPassthroughObjects` on Q16xxx-class text-barrier pages, with image/Form/shading passthrough still rejected before drawing. |
| `pdfium-android-arm64-r29-stroke-run-flush-telemetry-path-display-list.yml` | `workflow_dispatch` | **R29 STROKE-RUN FLUSH TELEMETRY PATH DISPLAY-LIST BUILD.** Applies ship patches through `0035`, skipping deprecated `0027/0028`. The workflow name starts with `r29` so it is easy to spot in GitHub Actions. Expected log: the r28 segment counters plus `strokeRunFlush*` counters explaining `strokeRunDraws` on Q16-style CAD pages. |
| `pdfium-android-arm64-r30-nonoverlap-fill-barrier-stroke-packing-path-display-list.yml` | `workflow_dispatch` | **R30 NON-OVERLAP FILL-BARRIER STROKE PACKING PATH DISPLAY-LIST BUILD.** Applies ship patches through `0036`, skipping deprecated `0027/0028`. The workflow name starts with `r30` so it is easy to spot in GitHub Actions. Expected log: lower `strokeRunFlushFillMode` when fill barriers are disjoint, plus `strokeRunFillBarriersCrossed/Blocked` counters. |
| `pdfium-android-arm64-r31-holder-space-spatial-index-path-display-list.yml` | `workflow_dispatch` | **R31 HOLDER-SPACE SPATIAL INDEX PATH DISPLAY-LIST BUILD.** Applies ship patches through `0037`, skipping deprecated `0027/0028`. The workflow name starts with `r31` so it is easy to spot in GitHub Actions. Expected log: nonzero `spatialIndexBins`, candidate counts, and large `spatialIndexSkippedByTile` on sparse Q16 zoom tiles while broad preview clips report `spatialIndexFallbackFullScan`. |
| `pdfium-android-arm64-r32-text-passthrough-cache-uaf-fix-path-display-list.yml` | `workflow_dispatch` | **R32 TEXT PASSTHROUGH CACHE UAF FIX PATH DISPLAY-LIST BUILD.** Applies ship patches through `0038`, skipping deprecated `0027/0028`. The workflow name starts with `r32` so it is easy to spot in GitHub Actions. Expected log: no cache-hit crash on Q16 text-passthrough pages, while path-only pages remain cached. |
| `pdfium-android-arm64-r33-text-passthrough-index-cache-path-display-list.yml` | `workflow_dispatch` | **R33 TEXT PASSTHROUGH INDEX CACHE PATH DISPLAY-LIST BUILD.** Applies ship patches through `0039`, skipping deprecated `0027/0028`. The workflow name starts with `r33` so it is easy to spot in GitHub Actions. Expected log: Q16 text-passthrough pages can return to `cache=hit` without retaining raw `CPDF_PageObject*` pointers. |
| `pdfium-android-arm64-r34-fill-barrier-telemetry-path-display-list.yml` | `workflow_dispatch` | **R34 FILL-BARRIER TELEMETRY PATH DISPLAY-LIST BUILD.** Applies ship patches through `0040`, skipping deprecated `0027/0028`. The workflow name starts with `r34` so it is easy to spot in GitHub Actions. Expected log: compact `VelocePathDLFill` lines explain whether Q16 `strokeRunFlushFillMode` is dominated by fill-only/fill+stroke barriers, rect-like/thin geometry, or real pending-stroke overlap. |
| `pdfium-android-arm64-r37-blend-group-cancellation-path-display-list.yml` | `workflow_dispatch` | **R37 BLEND GROUP CANCELLATION PATH DISPLAY-LIST BUILD.** Applies ship patches through `0041`, skipping deprecated `0027/0028`. The workflow name starts with `r37` so it is easy to spot in GitHub Actions. Expected log: `blendCancelChecks` increments on `11.pdf`-style blend tiles, and obsolete tiles can return `cancelled` during group-buffer work. |
| `pdfium-android-arm64-r38-blend-run-widening-telemetry-path-display-list.yml` | `workflow_dispatch` | **R38 BLEND RUN WIDENING TELEMETRY PATH DISPLAY-LIST BUILD.** Applies ship patches through `0042`, skipping deprecated `0027/0028`. The workflow name starts with `r38` so it is easy to spot in GitHub Actions. Expected log: compact `VelocePathDLBlend` lines with group pixel cost and paint-barrier opportunity counters. |
| `pdfium-android-arm64-r39-primitive-run-telemetry-path-display-list.yml` | `workflow_dispatch` | **R39 PRIMITIVE RUN TELEMETRY PATH DISPLAY-LIST BUILD.** Applies ship patches through `0043`, skipping deprecated `0027/0028`. The workflow name starts with `r39` so it is easy to spot in GitHub Actions. Expected log: compact `VelocePathDLPrimitive` lines with candidate/culled/drawn path points and subpaths for Q16/Error dense-tile analysis. |
| `pdfium-android-arm64-r40-fill-barrier-proof-telemetry-path-display-list.yml` | `workflow_dispatch` | **R40 FILL-BARRIER PROOF TELEMETRY PATH DISPLAY-LIST BUILD.** Applies ship patches through `0044`, skipping deprecated `0027/0028`. The workflow name starts with `r40` so it is easy to spot in GitHub Actions. Expected log: compact `VelocePathDLFillProof` lines classifying stroke-run-blocking fill barriers before any crossing behavior is promoted. |
| `pdfium-android-arm64-r25-1-0088-render-program-cost-attribution.yml` | `workflow_dispatch` | **R25-1-0088 RENDERPROGRAM COST ATTRIBUTION BUILD.** Extends the correctness-first r25-1 stack through 0088. Expected Android log tag: `VeloceRenderProgram`. This revision selects the 0089 optimization from measured `postings/candidates/visited/drawn/packetDispatches` ratios; it does not itself promise a rendering speedup. |
| `pdfium-android-arm64-r25-1-0089-exact-darken-spans.yml` | `workflow_dispatch` | **R25-1-0089 EXACT DARKEN SPAN BUILD.** First measured 0088 follow-up. Expected `11.pdf` proof is nonzero `directDarken` and `darkenDispatches`, with `darkenCanonical` exposing fail-closed cases. Dispatch count remains object-ordered; the speedup comes from removing each eligible object's temporary bitmap allocation and second pixel pass. Q16 normal-stroke execution is intentionally unchanged. |
| `pdfium-android-arm64-r25-2-0099-ordered-block-skip-darken-spans.yml` | `workflow_dispatch` | **R25-2-0099 ORDERED BLOCK + DARKEN BUILD.** Applies the exact generation-2 stack through 0099. Q16 proof is nonzero `blocksSkipped`/`commandsSkipped`; `11.pdf` proof is nonzero `nativeDarkenPaths`/`darkenDraws`. Unknown bounds, mutation drift, unsupported blend contexts, and driver rejection fail open to ordered canonical rendering. |
| `pdfium-android-arm64-r25-2-0100-ordered-line-batches-proven-outer-darken.yml` | `workflow_dispatch` | **R25-2-0100 FAILED COMPILE.** Historical workflow retained for audit. The bundled AGG `path_storage` has no `remove_all()` member, so no 0100 artifact was produced. Use 0102. |
| `pdfium-android-arm64-r25-2-0101-reusable-agg-path-reset.yml` | `workflow_dispatch` | **R25-2-0101 FAILED VERIFICATION.** The C++ build and API checks passed, but the workflow's overbroad `direct_darken` substring guard rejected the valid generation-2 local identifier `direct_darken_context`. No 0101 artifact was produced. Use 0102. |
| `pdfium-android-arm64-r25-2-0102-precise-generation1-guard.yml` | `workflow_dispatch` | **R25-2-0102 PRECISE GENERATION-1 GUARD BUILD.** Retains 0101's allocation-preserving AGG reset and 0100 executor, but checks exact generation-1 symbols instead of the generic `direct_darken` substring. Native markers advance to 0102; format, pixels, memory policy, and execution are unchanged. |
| `pdfium-android-arm64-r25-3-0103-owned-ordered-program.yml` | `workflow_dispatch` | **R25-3-0103 OWNED ORDERED PROGRAM BUILD.** Removes command-count routing and compiles exact native opcodes from command zero in the same ordered bytecode as canonical PDFium barriers. Native replay validates mutation once, resolves visibility once per run, and avoids per-command live-object state/visibility/bounds work. Expected Q16 proof: `nativeObjectLookups` is near `visibilityRunChecks`, not `nativeDraws`; expected 11.pdf proof: `darkenContextDirect=1`, nonzero `darkenDraws`, and near-zero `darkenFallbacks`. Memory remains capped at 96 MiB. |
| `pdfium-android-arm64-r25-3-0104-sparse-exact-sidecar.yml` | `workflow_dispatch` | **R25-3-0104 SPARSE EXACT SIDECAR BUILD.** Canonical gaps have no copied opcode/bounds metadata and canonical-only holders retain no sidecar. Exact native metadata is sealed once under the 96 MiB cap. |
| `pdfium-android-arm64-r25-3-0105-ordered-sparse-cursor.yml` | `workflow_dispatch` | **R25-3-0105 ORDERED SPARSE CURSOR BUILD.** Corrects cursor fast-forward across canonical barriers so off-tile native leaves/path blocks are skipped once and cancellation follows visited work. |
| `pdfium-android-arm64-r25-3-0106-ordered-mixed-fill-executor.yml` | `workflow_dispatch` | **R25-3-0106 ORDERED MIXED FILL EXECUTOR BUILD.** Adds exact owned opaque fills and one fixed-capacity AGG line/fill packet while preserving independent source-order raster/composite semantics. |
| `pdfium-android-arm64-r25-3-0107-compact-spatial-ordinal-program.yml` | `workflow_dispatch` | **R25-3-0107 COMPACT SPATIAL ORDINAL PROGRAM BUILD.** Replaces per-line state/matrix duplication and source-local leaves with compact line runs and a bounded 32x32 holder-space ordinal index. Sparse tile replay uses one reusable render-local bitset and source-order run jumps; unknown bounds and budget pressure fail open. Expected Q16 proof: low `spatialCandidates`, high `spatialCulled`, and `commandsVisited` proportional to visible work. |
| `libpdfium_experiment_profile_build.yml` | `workflow_dispatch` | **EXPERIMENT PROFILE BUILD.** Applies ship patches `01..08` plus `patches/experiments/0010-render-profile-and-content-skip.patch`. Use this to measure per-stage `FPDF_RenderPageBitmap()` costs and test skip-text/path/image/shading/Form/transparency/soft-mask speedups on selected PDFs. Artifact: `libpdfium-android-arm64-veloce-render-profile-experiment`. |
| `release-size-experiment.yml` | `workflow_dispatch` | Side-by-side size comparison (baseline / A / B / C). Variant C is the current ship config. Run this when a future PDFium roll or build refactor unexpectedly changes the size profile — it isolates which lever moved. |
| `build-pdfium-android-arm64-agg-devicen-fast.yml` | `workflow_dispatch` | Diagnostic ship build. Applies 0003+0004+0005+0007+0008+0009, retains trace markers. Use for future perf investigations. |
| `build-pdfium-android-arm64-agg-jpeg-downscale.yml` | `workflow_dispatch` | Minimal historical ship (0003 only). Kept for rollback. |
| `build-pdfium-android-arm64-agg-jpeg-downscale-trace.yml` | `workflow_dispatch` | Diagnostic (0003+0004). |
| `build-pdfium-android-arm64-agg-jpeg-downscale-trace-deep.yml` | `workflow_dispatch` | Diagnostic (0003+0004+0005). |
| `build-pdfium-android-arm64-agg-icc-skip-experiment.yml` | `workflow_dispatch` | Diagnostic ONLY. Includes 0006 wrong-color bypass. |
| `build-pdfium-android-arm64-agg-colorspace-trace.yml` | `workflow_dispatch` | Diagnostic (0003+0004+0005+0007). |
| `build-pdfium-android-arm64-agg-fast-indexed.yml` | `workflow_dispatch` | First fix attempt (0008). 0009 build supersedes this. |
| `build-pdfium-android-arm64-skia.yml` | `workflow_dispatch` | Skia variant. Confirmed NOT faster than AGG on the test PDF (ICC + base-class slow path is independent of renderer backend). |

---

## How to verify a build is the 0009 (current) ship build on device

1. Vendor the `.so` from the 0009 artifact into
   `android/pdfsdk/libs/PdfiumAndroidKt-2.0.0/core/src/main/jniLibs/arm64-v8a/libpdfium.so`.
2. Install + capture a Perfetto trace:
   ```bash
   adb shell setprop debug.xpdf.trace 1
   adb shell am force-stop com.xapper.pdf.reader
   bash /Users/shchao/Code/xPDFSDK/android/meta/profile/capture_perfetto_render.sh 10
   # Then scroll an image-heavy PDF while it records.
   ```
3. Open the `.perfetto-trace` in https://ui.perfetto.dev and search for
   **`pdfium009.`**. If you see slices, this is the 0009 build. If zero, the
   `.so` on device is older.

---

## Diagnostic history (for future debuggers)

The investigation took multiple iterations. Each diagnostic patch answered
one question and unlocked the next:

1. **Phase 1 -- 0003 alone:** JPEG decode speedup landed, but image-heavy
   PDFs still slow. Hypothesis: SMask / ICC compositing dominates.
2. **Phase 2 -- 0004/0005 markers:** showed `xpdf.renderBitmap.region[N]`
   at 7s wall-clock, fully CPU-bound, with all time inside
   `pdfium.StartGetCachedBitmap`. The patched `JpegRewind` was narrow ->
   JPEG decode is NOT the bottleneck on this content.
3. **Phase 3 -- 0007 colorspace markers:** narrowed it to
   `pdfium.ColorSpace::TranslateImageLine.baseSlow` at ~1.4 ms/scanline.
   That's the per-pixel virtual-dispatch loop on colorspaces that don't
   override TranslateImageLine.
4. **Phase 4 -- 0006 ICC-bypass diagnostic:** confirmed bypassing the per-
   pixel work made renders 25-40x faster, but produced wrong colors.
   Confirmed the loop is the bottleneck, not the JPEG decode.
5. **Phase 5 -- 0008 Indexed/Separation overrides:** ZERO Indexed/Separation
   slices fired in the resulting trace. Wrong CS family.
6. **Phase 6 -- 0009 DeviceN override + prefix markers:** `pdfium009.DeviceNCS::
   TranslateImageLine` fired 14,619 times. `baseSlow` dropped to ZERO.
   Picture-book renders dropped from ~5s to ~135ms.

The lesson: **keyword-counting the raw PDF bytes is insufficient** (slow-
path colorspaces were declared inside compressed object streams). Always
decompress streams when scanning for colorspace usage.

---

## Upstreaming roadmap (suggested)

The three "SHIP" patches are independent and each suitable for an upstream
PDFium PR:

1. **0003 -- JPEG downscale-on-decode** -- closes a documented gap
   (JPEG2000 path honors `resolution_levels_to_skip`, JPEG doesn't).
2. **0008 -- Fast IndexedCS/SeparationCS** -- pure perf optimization,
   bit-identical output, no behavior change.
3. **0009 -- Fast DeviceNCS for N<=2** -- same pattern, same bit-identical
   guarantee.

Recommend waiting 1-2 weeks of production deployment to confirm no
regressions before submitting upstream.
