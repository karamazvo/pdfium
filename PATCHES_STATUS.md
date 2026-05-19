# xPDFSDK PDFium Patch Status

**Last updated:** 2026-05-19

## TL;DR

To build the **production release** Android arm64 PDFium:

> Run workflow: **`Release PDFium Android arm64 AGG`**
>
> File: `.github/workflows/release-pdfium-android-arm64-agg.yml`
> Artifact: `libpdfium-android-arm64-agg-release`

This applies the three independent, self-contained patches under
`patches/ship/` (01 JPEG downscale, 02 IndexedCS+SeparationCS fast,
03 DeviceNCS fast). No diagnostic markers; no Android-specific
ATrace dependency; smallest possible `.so`. This is what to vendor
into the app for end users.

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

---

## Workflow table

| Workflow file | Trigger | Status |
|---|---|---|
| `release-pdfium-android-arm64-agg.yml` | `workflow_dispatch` | **CURRENT PRODUCTION RELEASE BUILD.** Applies only the 3 ship patches (`patches/ship/01..03`). Minimal `.so`, no diagnostic markers, no `libandroid.so` dependency. Audit step rejects the build if any `XPDF_ATRACE_SCOPED` leaks in. |
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
