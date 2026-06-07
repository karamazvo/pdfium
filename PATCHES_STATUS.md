# xPDFSDK PDFium Patch Status

**Last updated:** 2026-06-03

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

---

## Workflow table

| Workflow file | Trigger | Status |
|---|---|---|
| `release-pdfium-android-arm64-agg.yml` | `workflow_dispatch` | **CURRENT PRODUCTION RELEASE BUILD.** Applies only the 3 ship patches (`patches/ship/01..03`). Uses the Variant C shrink link config (`pdf_use_partition_alloc=false`, `default_min_sdk_version=23`, `--gc-sections` + auto-`--undefined` for `FPDF_*`/`FORM_*`/`FSDK_*`). Minimal `.so` (~6.6 MB), no diagnostic markers, no `libandroid.so` dependency. Audit step rejects the build if any `XPDF_ATRACE_SCOPED` leaks in; canary-API survival check rejects the build if `--gc-sections` ever drops a public entry point. |
| `release-pdfium-android-arm64-veloce.yml` | `workflow_dispatch` | **CURRENT VELOCE RELEASE BUILD.** Applies ship patches `01..07`, including internal-access exports, skip-rasterization, and render-abort probes. Use this for VIR admission/replay work and cancellation experiments. |
| `libpdfium_patch_build.yml` | `workflow_dispatch` | **NEXT VELOCE CLASSIFICATION BUILD.** Applies ship patches `01..08`, including `FPDFEx_LoadPageWithClassification`. Verifies all Veloce symbols, the new classification export, and the 64-byte classification struct ABI before packaging `libpdfium.so`. |
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
