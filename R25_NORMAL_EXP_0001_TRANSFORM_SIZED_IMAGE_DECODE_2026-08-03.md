# r25-normal-exp-0001: Transform-Sized Image Decode

Date: 2026-08-03 (Asia/Taipei)

Status: isolated experiment, not an accepted release revision

Base: `r25-8-0138` ordered stroke renderer tape

Artifact: `libpdfium-android-arm64-r25-normal-exp-0001-transform-sized-image-decode`

## Purpose

Measure whether normal-page preview latency can be reduced by giving PDFium's
existing DCT/JPX reduced-resolution decoders the image's actual transformed
destination demand instead of the entire render-device size.

This experiment is intentionally based on `0138`. It does not include later
acquisition, profiling, Form-tape, or other rendering experiments.

## Root Cause

The canonical image renderer currently sends `{deviceWidth, deviceHeight}` to
`CPDF_ImageLoader` for every image. An image occupying only part of a preview
therefore often requests more decoded pixels than its transformed destination
can consume.

Patch `01-jpeg-downscale-on-decode.patch` already makes DCT honor PDFium's
`resolution_levels_to_skip`; JPX already honors the same value. The missing
input is an accurate per-image destination demand.

## Invariant

For each source image axis:

```text
requiredPixels = ceil(length(imageAxis transformed into device space))
requiredPixels = min(requiredPixels, canonicalDeviceDemand)
```

The outward rounding prevents a one-pixel undersize. Bounding by the existing
device demand ensures this experiment never asks a decoder for more pixels than
the `0138` behavior. Non-finite, degenerate, or oversized device geometry
returns `{0, 0}`, preserving PDFium's full-resolution fail-closed path.

The same destination demand is passed to explicit masks and soft masks because
they cover the same transformed unit square as the color image.

## Pipeline

```text
CPDF_ImageRenderer
  -> image-to-device source-axis lengths
  -> bounded CFX_Size decode demand
  -> CPDF_ImageLoader
  -> existing resolution-aware CPDF_PageImageCache
  -> CPDF_DIB resolution_levels_to_skip
       -> DCT: reduced JPEG IDCT before pixel allocation
       -> JPX: reduced-resolution decode before pixel allocation
       -> other codecs: unchanged canonical full decode
  -> existing AGG transform/composite
```

## Architectural Cost

- Two matrix-axis length calculations and two outward rounds per image load.
- No page or document classification.
- No threshold routing.
- No new cache or retained representation.
- No heap allocation.
- No synchronization or thread change.
- No JNI, Kotlin, scheduler, or UI-thread work.
- Existing resolution-aware image-cache replacement remains the single source
  of truth for low- versus high-resolution cached images.

## Expected Result

The change can improve DCT/JPX images only when the transformed destination is
small enough to cross a power-of-two decoder level. It will not improve
Flate/raw/indexed image decode, text, paths, or fixed AGG compositing cost.

For the 6Steps corpus, the most relevant checks are preview renders at 756,
540, and 432 pixel page widths. Images around 977 source pixels wide should
begin selecting half-resolution decode when their transformed width falls
below roughly 489 pixels.

## Acceptance Gate

Compare the new artifact directly with `r25-8-0138` using the same device,
bitmap sizes, cold/warm state, and scroll sequence.

Accept only if all are true:

1. 6Steps preview render time materially improves at reduced preview sizes.
2. No missing image, mask, matte, alpha, color, or page content appears across
   6Steps, Study Notes, `disquisitionesa00gaus.pdf`, 11, Q16, and EP23.
3. Full-resolution settled tiles remain visually equivalent within the
   existing reduced-JPEG decode tolerance.
4. Normal text/vector pages do not regress outside run-to-run noise.
5. Memory remains bounded by the existing page image cache.

Reject if the improvement exists only after a warm cache, if masks become
misaligned, or if a low-resolution cache result is reused for a later
higher-resolution request.

## Files

- Patch: `patches/experiments/normal-page/0001-transform-sized-image-decode.patch`
- Workflow: `.github/workflows/pdfium-android-arm64-r25-normal-exp-0001-transform-sized-image-decode.yml`
