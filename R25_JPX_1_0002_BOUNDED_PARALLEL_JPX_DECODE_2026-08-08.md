# r25-jpx-1-0002: Bounded Parallel JPX Decode

**Status:** Build-ready experiment, not accepted until device and pixel validation pass.

**Date:** 2026-08-08 (Asia/Taipei)

**Base:** `r25-cancel-1-0003`

**Excluded:** `r25-image-1-0001`. That destination-side color/mask execution
experiment did not reduce the measured decode-bound cold render and is not a
parent of this revision.

## Purpose

The measured image-heavy page spends most of its cold visible latency inside a
large JPX decode. PDFium already uses OpenJPEG, and OpenJPEG already models
Tier-1 codeblocks as independent jobs, but the Android build compiled the
threading source without enabling its pthread backend and PDFium requested no
workers. The result was serial codeblock decode on the sole PDF render lane.

This revision enables OpenJPEG's existing Android pthread implementation and
requests exactly two decoder-local workers. It changes the execution schedule
of independent codeblock jobs, not the decoded image or PDF render model.

## Correctness Invariant

> The same JPX decode area, resolution, components, codeblock operations,
> colorspace conversion, output layout, PDFium cache ownership, and compositor
> execute exactly once; only independent OpenJPEG codeblock jobs may execute
> concurrently.

Consequences:

- No PDF object is skipped, reordered, approximated, or classified.
- No image resolution or decode region changes.
- PDFium remains the sole owner of decoded pixels and compositing.
- OpenJPEG's existing worker jobs write their assigned codeblock storage; the
  decoder waits for completion before later decode stages consume it.
- Failure to create the bounded worker pool falls back to OpenJPEG's serial
  pool.

## Architecture

```text
PDF image object
    |
    v
PDFium JPX decoder (one canonical decoder, one output)
    |
    +-- OpenJPEG control thread
    |       |
    |       +-- Tier-1 codeblock job --> worker 1
    |       +-- Tier-1 codeblock job --> worker 2
    |       +-- wait for submitted jobs
    |
    v
unchanged component reconstruction and PDFium pixel conversion
    |
    v
unchanged page image cache, transform, mask, blend, and bitmap
```

The pool is decoder-local and capped at two workers. There is no global worker
count, device-core heuristic, new render queue, persistent cache, second pixel
owner, JNI/Kotlin route, or UI-thread work.

## Patch Contents

Patch:
`patches/experiments/jpx/0002-bounded-parallel-jpx-decode.patch`

1. Enables OpenJPEG's existing `MUTEX_pthread` backend for Android only.
2. Makes new Android codecs start serial, so `OPJ_NUM_THREADS` cannot bypass
   PDFium's explicit bound during codec construction.
3. Adds a hard `CJPX_Decoder::kMaxDecodeThreads = 2` bound.
4. Requests two workers for production decode and rejects larger requests.
5. Preserves OpenJPEG's serial fallback when worker-pool creation fails.
6. Adds an RGB, RGBA, and grayscale serial-versus-parallel byte-equivalence
   unit test.

## Cancellation And Memory

The accepted cancellation stack installs one existing render-status cancel
callback in OpenJPEG. Tier-1 workers check the same OpenJPEG event manager, so
parallel decode does not create a second cancellation owner. Cancellation still
abandons unpublished output.

The bounded cost is two decoder-local worker records, thread stacks, and
OpenJPEG's existing per-job codeblock scratch. OpenJPEG throttles its producer
when pending jobs exceed `100 * worker_count`, so the two-worker queue cannot
grow with total page or document length. No persistent bitmap, page cache, or
application task queue is added. The decoder and pool are destroyed together.

## Expected Result

This is a Pareto experiment, not an x10 claim. On a JPX-decode-bound cold page,
two workers should reduce native decode time by roughly 20-40%, subject to
memory bandwidth, codeblock count, and device CPU scheduling. Small images and
non-JPX pages should remain within measurement noise apart from bounded thread
startup/teardown.

## Acceptance Gates

Accept only if all gates pass:

1. **Pixel correctness:** serial and two-worker fixture output is byte-identical;
   corpus screenshots/diffs show no changed pixels.
2. **Logical equivalence:** decoded dimensions, channels, colorspace, pitch,
   resolution, cache behavior, masks, blend, and painter order are unchanged.
3. **Performance:** repeated controlled cold renders improve the decode-bound
   756-wide median by at least 20% (the observed 279 ms reference implies
   `<=223 ms`). A result below 15% is not enough to retain this mechanism.
4. **Responsiveness:** cancellation p95 remains `<=20 ms`, with no stale bitmap
   publication or longer uninterruptible render section.
5. **Regression:** 6Steps, 11.pdf, Q16, and EP23 retain correct pixels and do
   not regress beyond run-to-run noise.
6. **Bounds:** exactly two workers maximum, serial fallback, no new persistent
   cache, no document-specific policy.

Reject the experiment on any pixel mismatch, crash, deadlock, cancellation
regression, or material normal-page slowdown. Do not weaken exactness or add a
document threshold to make the benchmark pass.

## Build

Workflow:
`.github/workflows/pdfium-android-arm64-r25-jpx-1-0002-bounded-parallel-jpx-decode.yml`

Artifact:
`libpdfium-android-arm64-r25-jpx-1-0002-bounded-parallel-jpx-decode`

The workflow compiles the byte-equivalence unit test after linking the Android
shared library and packages the exact patch stack plus `build-info.txt`.
