# r25-jpx-3-0004: Exact Fused JPX Output Sink

Locked: 2026-08-08 CST

## Revision

- Revision: `r25-jpx-3-0004`
- Performance baseline: accepted `r25-cancel-1-0003`
- Patch: `patches/experiments/jpx/0004-exact-fused-jpx-output-sink.patch`
- Workflow: `.github/workflows/pdfium-android-arm64-r25-jpx-3-0004-exact-fused-jpx-output-sink.yml`
- Excluded: rejected `r25-image-1-0001`, `r25-jpx-1-0002`, and
  `r25-jpx-2-0003`

This is a native PDFium/OpenJPEG experiment. It does not change Kotlin/JNI
scheduling, page routing, image resolution, decode area, PDF parsing, editing,
save semantics, the huge-path executor, masks, blend order, or the UI thread.

## Why The Previous Direction Was Rejected

`r25-jpx-2-0003` retained two full image passes after inverse DWT: planar MCT,
then planar-to-interleaved PDFium packing. Device evidence against
`r25-cancel-1-0003` showed:

- first-visible latency regressed from about 735 to 972 ms;
- cold page 0 regressed from about 351 to 468 ms;
- paired completed 756-pixel renders regressed about 49% by mean and 53% by
  median;
- warm tiles were effectively unchanged at 59 versus 60 ms;
- cancellation p95 moved from about 16 to 19 ms.

The result rejects separate SIMD finish passes as a useful parent. It does not
reject eliminating those passes.

## Invariant

> For an exactly supported JPX transform, consume each post-DWT sample once
> and publish the same final PDFium bytes; otherwise execute the unchanged
> canonical OpenJPEG and PDFium pipeline before any accelerated pixel is
> published.

The sink preserves:

- the same decode area, reduction level, tile, and component selection;
- the same scalar irreversible MCT expression grouping;
- `opj_lrintf()`, the same level shift, and the same 8-bit clamp;
- the existing PDFium RGB/BGR layout decision and row padding;
- painter order, image cache ownership, mask, transform, and blend semantics;
- cancellation without publishing a partial destination.

## Architecture

```text
PDFium final bitmap allocation
             |
             v
OpenJPEG Tier-1 and inverse DWT
             |
             v
three post-DWT float planes
             |
             v
one fused scalar pass:
  inverse MCT -> round -> level shift -> clamp -> interleaved BGR store
             |
             v
authoritative PDFium bitmap
             |
             v
unchanged cache, transform, mask, blend, and render pipeline
```

OpenJPEG asks PDFium for a caller-owned destination only after exact decoded
dimensions are known. The destination callback uses the existing
`JpxDecodeConversion` to validate the final PDFium format and allocate the one
authoritative bitmap. OpenJPEG marks the sink used only after every output
pixel completes.

## Exact Eligibility

The fused sink requires all of the following:

- bundled OpenJPEG and whole-image decode;
- exactly one zero-origin whole-image tile and tile zero, matching OpenJPEG's
  no-copy component-buffer transfer case;
- no selected-component subset;
- exactly three components;
- standard MCT (`mct == 1`), irreversible 9/7 transform, unsigned 8-bit
  components, and level shift 128;
- equal, unsubsampled component dimensions;
- each decoded resolution equals its complete allocated DWT plane;
- PDFium's existing conversion resolves to three-channel BGR without ARGB
  conversion;
- no JP2 palette or CDEF channel mapping.

Every failed predicate returns to the unchanged canonical path. These are
semantic capability checks, not document names, page classes, thresholds, or
content heuristics.

## Resource And Cancellation Contract

- One final destination bitmap; no second destination-sized bitmap.
- No new cache, retained sidecar, worker, thread, mutex, or lock.
- No new document or page classifier.
- Only a stack-scoped callback context is live during synchronous decode.
- Cancellation is checked at most every 256 output pixels.
- A cancelled or failed sink destroys its private partial bitmap and returns
  failure; it cannot become a displayed or cached result.
- Unsupported inputs preserve canonical allocation and execution.

## Local Proof

The patch was authored against the exact accepted stack through
`r25-cancel-1-0003`, with all rejected image and JPX experiments excluded.

Local checks completed:

- bundled OpenJPEG C sources pass strict syntax compilation;
- an actual single-tile irreversible JP2 was decoded both canonically and
  through the new output callback;
- every final BGR byte matched the canonical decoded components;
- a 65,536-sample randomized test matched the bundled OpenJPEG MCT plus level
  shift implementation exactly;
- patch whitespace and application checks pass.

The GitHub workflow performs the full Android arm64 PDFium and unit-test build
and rejects leaked worker scheduling, the previous separate SIMD finish pass,
document-specific policy, new caches, locks, or threads.

## Expected Result And Acceptance Gate

This is not an x10 claim. It removes one complete MCT write/read cycle and the
following component packing read pass for eligible JPX resources. A realistic
cold decode target is 15-35%; entropy and inverse DWT remain unchanged.

The one-time runtime attestation is:

```text
PdfJpxFused revision=r25-jpx-3-0004 event=exact_fused_output width=<w> height=<h> channels=3
```

Accept only if all gates pass:

1. native pixel-corpus output is byte-identical;
2. repeated cold 756-pixel JPX median improves at least 15% from
   `r25-cancel-1-0003`;
3. first-visible and cold page 0 improve without warm-tile regression;
4. cancellation p95 does not exceed the accepted baseline;
5. 6Steps, 11.pdf, Q16, and EP23 retain correct pixels and performance;
6. no blank, partial, stale, or lower-resolution publication appears.

If the sink does not attest on the measured resource, inspect which exact
eligibility predicate failed. Do not broaden eligibility approximately. If it
attests but the repeated cold gain is below 15%, reject this direction rather
than stacking another finish pass.
