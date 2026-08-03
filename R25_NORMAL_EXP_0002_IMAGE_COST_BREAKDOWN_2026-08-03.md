# r25-normal-exp-0002: Canonical image cost breakdown

Date: 2026-08-03 (Asia/Taipei)

## Purpose

`r25-18-0148` proved that image rendering dominates the sampled normal-page
renders, but its single `imageUs` value cannot identify whether the expensive
work is source realization, mask/offscreen handling, or destination raster.

This diagnostic experiment adds exclusive image subphase timing. It does not
change pixels, decode demand, cache policy, task scheduling, cancellation, or
allocation policy. It intentionally excludes rejected experiment
`r25-normal-exp-0001` so its measurements use the accepted
`r25-8-0138 + r25-18-0148` behavior.

## Atomic image phases

Each phase pauses its parent timer. The values therefore do not double-count
nested work.

1. `imageLoadUs`: page-image-cache lookup and source realization through
   `CPDF_ImageLoader::Start/Continue`. This includes embedded image-mask decode
   because PDFium realizes source and mask through the same loader boundary.
2. `imagePrepareUs`: transfer functions, color-mode conversion, interpolation
   selection, and selection of the rendering route.
3. `imageMaskPaintUs`: explicit masked-image offscreen construction, alpha-mask
   multiplication, and mask-specific composition outside nested device draws.
4. `imagePatternPaintUs`: pattern-colored image-mask offscreen construction and
   pattern-specific composition outside nested device draws.
5. `imageDeviceUs`: render-device scaling, interpolation, rasterization, blend,
   destination composite, and progressive device continuation.

`imageOtherUs` is image-object routing time outside those five boundaries.
`imageUs` remains the compatible total:

```text
imageUs = imageOtherUs + imageLoadUs + imagePrepareUs
        + imageMaskPaintUs + imagePatternPaintUs + imageDeviceUs
```

The log also reports a call count for each subphase. This distinguishes one
expensive decode from repeated realization or paint work.

## Build

Run the GitHub Actions workflow whose name begins:

```text
r25-normal-exp-0002 image cost breakdown
```

Artifact:

```text
libpdfium-android-arm64-r25-normal-exp-0002-image-cost-breakdown
```

## Capture

Filter logcat by `VeloceCanonicalProfile`. Collect cold first-preview and warm
repeat-preview samples separately. For the 6-Steps PDF, include the page number
or app render request line adjacent to each native summary so samples can be
matched to the rendered page.

The decisive comparisons are:

- `imageLoadUs / activeUs`: decoder/cache realization cost.
- `(imageMaskPaintUs + imagePatternPaintUs) / activeUs`: mask/pattern offscreen
  cost.
- `imageDeviceUs / activeUs`: scaling, rasterization, and destination composite.
- `imageLoadCalls` versus image objects: repeated source realization.
- `imageDeviceCalls` versus image objects: repeated or progressive device work.
- cold versus warm `imageLoadUs`: whether the page image cache removes the cost.

Do not use this profiler build for performance acceptance. Its scoped
`steady_clock` calls add diagnostic overhead. Use it only to select the next
optimization, then validate that optimization in a profiler-free build.

## Decision rule

- Load dominates cold and collapses warm: investigate decode/cache reuse.
- Load dominates both cold and warm: investigate cache misses, invalidation, or
  source realization that is not retained.
- Mask paint dominates: eliminate redundant full-size offscreen/mask passes
  while preserving exact PDF compositing.
- Device dominates: focus on demand-sized raster and exact destination-space
  image execution, not a replacement decoder.
- Traversal or non-image phases dominate: image work is not the bottleneck for
  that render; do not optimize the decoder from that sample.
