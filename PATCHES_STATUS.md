# xPDFSDK PDFium Patch Status

**Last updated:** 2026-06-20

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
