# r25-normal-exp-0006: Canonical Decode-to-Stretch Row Pipeline

Date: 2026-08-04 (Asia/Taipei)

## Status and base

- Experimental normal-page build, not a shipping revision.
- Base: accepted `r25-8-0138` plus `r25-normal-exp-0005`.
- Patch: `patches/experiments/normal-page/0006-canonical-decode-to-stretch-row-pipeline.patch`.
- Workflow: `pdfium-android-arm64-r25-normal-exp-0006-canonical-decode-to-stretch-row-pipeline.yml`.

## Root cost

For ordinary cached images, PDFium currently performs two serial passes before
the image reaches the destination:

```text
decoder -> eagerly realize every canonical source row -> full cached bitmap
                                                       -> stretch rereads rows
                                                       -> destination
```

`0005` bounded the stretch intermediate, but it intentionally left this eager
source realization and reread intact. `0006` removes that duplicate source pass.
It does not reduce decoder resolution, replace a codec, classify pages, or
change PDF image/color/mask semantics.

## Architecture

```text
                         one page-cache owner
                                 |
decoder -> canonical row -> full canonical cache bitmap
                    \----> 0005 bounded stretch row ring -> destination
```

`CanonicalRowCache` replaces eager `CFX_DIBBase::Realize()` for unrealized AGG
image sources. It allocates the same full canonical cache bitmap that eager
realization allocated, but fills it on demand. Its only mutable state is the
length of a monotonically increasing decoded-row prefix.

When a consumer requests row `N`:

1. Rows before `N` that are not cached are decoded once in source order.
2. Each row is copied into its final canonical cache location.
3. The same row is immediately returned to the existing consumer. With `0005`,
   this feeds horizontal filtering without first rereading a separately
   realized source bitmap.
4. Once every row is cached, the decoder/source object is released.

The prefix is the single source of truth. There is no validity map, second
bitmap, per-row allocation, new persistent cache, lock, thread, or scheduler.

## Correctness and fallback contract

- Output dimensions, format, palette, pitch, decoded rows, resampling weights,
  color conversion, alpha, masks, clipping, composition, and painter order stay
  owned by canonical PDFium.
- The row copy length is calculated by the same `CalculatePitch8()` rule used
  by canonical full realization.
- Non-sequential consumers remain correct: requesting a later row completes the
  missing canonical prefix; requesting an earlier row reads the cache.
- Sources that already own decoded pixel memory fail closed to eager
  realization. This avoids retaining two full decoded buffers, including JPX
  sources that decode into a bitmap internally.
- Allocation or pitch-validation failure also falls back before any consumer
  receives a row from the new path.
- The unit test compares arbitrary and fully realized cache rows against a
  separately decoded canonical reference at identical requested dimensions.

## Resource and cancellation contract

- Persistent decoded-pixel memory remains one full canonical page-cache bitmap.
- `0005` remains responsible for bounding the stretch intermediate to 1 MiB.
- No bitmap or helper object is allocated per row or per pixel.
- `SkipToScanline()` checks PDFium's pause interface after every newly cached
  prefix row. A stale render can therefore stop during source realization
  instead of waiting for the former non-progressive full `Realize()` pass.
- Partial rows are safe to retain because the PDF image stream is immutable and
  only the completed prefix is readable. A resumed or later render continues
  from that prefix without decoding a row twice.
- No UI-thread work or new synchronization is introduced.

## Expected result and acceptance

This patch targets cold image-page rendering where decoder output is currently
written once during eager realization and read again by the stretcher. A
realistic improvement is another 10-25% for affected pages, plus materially
lower cancellation latency. It is not expected to provide a 10x gain when the
codec itself dominates.

Compare `r25-normal-exp-0005` and `0006` with the same app build, page sequence,
viewport, gesture, and cold/warm state. The primary native metric is matching
page `bitmapRenderMs`; secondary metrics are `engineRenderMs`, cancellation
latency, and end-to-end visible-page time. Accept only if:

- pixels and image resolution are unchanged;
- `The-6-Steps-...pdf` cold matching-page `bitmapRenderMs` improves materially;
- p50 normal text/image pages do not regress;
- fast-scroll cancellation no longer waits behind eager full realization;
- 11.pdf, Q16.pdf, EP23, masks, JPX, and cache upscaling remain correct.

There is deliberately no per-image activation log in the hot loop. CI proves
the structural contract and unit-level equivalence; existing end-to-end timing
logs measure user value.
