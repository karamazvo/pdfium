# r25-normal-exp-0005: Bounded Streaming Image Renderer

Date: 2026-08-04 (Asia/Taipei)

## Status and base

- Experimental normal-page build, not a shipping revision.
- Base: accepted `r25-8-0138` only.
- Independent of rejected `r25-normal-exp-0001`, `0003`, and `0004`.
- Patch: `patches/experiments/normal-page/0005-bounded-streaming-image-renderer.patch`.
- Workflow: `pdfium-android-arm64-r25-normal-exp-0005-bounded-streaming-image-renderer.yml`.

## Problem

PDFium's `CStretchEngine` horizontally filters every source row into a full
`destination clip width x source clip height` intermediate. It then reads the
entire intermediate again for vertical filtering. Large images therefore pay
for a potentially multi-megabyte temporary allocation plus a full write/read
memory pass even when each output row depends on only a few source rows.

This experiment changes that execution model. It does not replace PDFium's
decoder, page image cache, resampling weights, compositing, or color handling.

## Architecture

```text
                         exact PDFium weight tables
                                    |
source scanline -> horizontal filter -> bounded row ring -> vertical filter
                                                               |
                                                    ComposeScanline(output row)
```

The renderer computes PDFium's existing horizontal and vertical weight tables
before consuming pixels. It selects the bounded executor only when all of the
following are proven:

1. The canonical full intermediate exceeds 1 MiB.
2. Vertical source ranges are valid and monotonically forward.
3. The maximum exact vertical support fits in a row ring of at most 1 MiB.
4. The ring is smaller than the canonical full intermediate.

Otherwise, `CStretchEngine` uses its unchanged full-intermediate executor.

The row ring is indexed by source-row ordinal. Each source row is horizontally
filtered once, retained only while an output row can reference it, vertically
filtered immediately, and then naturally overwritten. There is no page
classification, document-specific policy, persistent cache, second rendering
representation, UI-thread work, or new lock.

## Correctness contract

- Pixel ownership remains canonical PDFium.
- Both executors use the same `WeightTable` implementation.
- Horizontal and vertical fixed-point accumulation order is preserved.
- Source painter order, image-mask semantics, alpha handling, palette handling,
  clipping, and destination composition are unchanged.
- Unsupported, invalid, non-monotonic, oversized, or non-saving cases fail
  closed to the canonical executor before source pixels are consumed.
- Unit coverage compares complete output buffers from bounded and forced
  canonical execution for 8-bit masks, BGR, BGRA, smoothed, and no-smoothing
  inputs. It also verifies that small images remain canonical.

## Resource and cancellation contract

- New temporary storage is hard-capped at 1 MiB per active stretch.
- No allocation occurs per row or per pixel.
- The ring replaces the full stretch intermediate; it is not additional cache.
- Progressive pause checks occur while source rows are admitted and while
  output rows are emitted, with a maximum cadence of ten row work units.
- Existing stale-render cancellation reaches the same PDFium pause interface.

## Expected result

The intended improvement is lower allocation pressure, lower memory traffic,
and earlier output-row production for large scaled images. Small images are
deliberately unchanged. This patch does not eliminate source decode or page
image cache realization, so it should not be described as decoder-to-device
fusion and is not expected to turn an expensive codec decode into a 20 ms
render by itself.

For `The-6-Steps-...pdf`, compare cold preview render duration and canceled
render latency against `r25-8-0138`. Accept the experiment only if:

- rendered pixels and resolution are unchanged;
- cold visible-page time improves materially without worsening the p50 normal
  page path;
- fast-scroll cancellation does not wait behind a full image stretch;
- no new blank-page, delayed-image, or resolution artifacts appear.

There is intentionally no per-image Android log in the hot loop. Activation is
proven by unit tests and the workflow contract; end-to-end value is measured by
the existing render timing logs.

## Follow-up boundary

If this improves stretch time but decode still dominates, the next architectural
step is a separate decoder-to-row-consumer experiment that preserves PDFium's
page-cache ownership while avoiding mandatory full source realization. Do not
combine that lifetime and caching change into this experiment.
