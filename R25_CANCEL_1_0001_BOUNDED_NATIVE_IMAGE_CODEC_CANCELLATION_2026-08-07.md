# r25-cancel-1-0001: Bounded Native Image/Codec Cancellation

Locked: 2026-08-07 14:16:52 CST

## Revision

- Revision: `r25-cancel-1-0001`
- Base: `r25-8-0138` + normal-page experiments `0005`, `0006`, and `0011`
- Patch: `patches/experiments/cancellation/0001-bounded-native-image-codec-cancellation.patch`
- Workflow: `.github/workflows/pdfium-android-arm64-r25-cancel-1-0001-bounded-image-codec-cancellation.yml`
- Scope: native PDFium execution only; no Kotlin scheduler, JNI API, page classifier, or document-specific routing change

## Root Cause

The app scheduler can mark a preview or tile request obsolete immediately, but cancellation is useful only when the active native operation observes it. PDFium already checks between page objects and the JBIG2 decoder already consumes a `PauseIndicatorIface`. However, bundled OpenJPEG entered synchronous `opj_decode()` without a cancellation channel, and the standard vertical image stretch completed as one monolithic phase.

The observed class of failure is therefore not slow file opening. It is stale render work retaining PDFium's single render owner after the viewport has moved. A previously observed `lockWaitMs=19161` with `newDocumentMs=6` is consistent with this ownership delay.

## Invariant

An obsolete render request must stop at the next natural codec or raster work-unit boundary, must not publish a partial bitmap, and must release the render owner for the newest visible request. An uncancelled request must execute the canonical pixel pipeline unchanged.

## Mechanism

`CPDF_RenderStatus` exposes the existing render callback as a non-owning `CancelIndicatorIface`. The signal is passed explicitly through the existing image loader and page-image-cache call chain. It is not stored in a cache and does not create another owner.

Bundled OpenJPEG observes cancellation:

- before each JPEG 2000 tile;
- before scheduling and starting tier-1 codeblocks;
- between tier-2, tier-1, inverse-wavelet, color-transform, and level-shift phases;
- between components and output rows;
- while converting PDFium's JPX output and soft masks.

The normal PDFium stretcher makes its existing vertical pass resumable and checks at the same bounded row cadence used by the other stretch paths. JBIG2 continues to use its existing progressive pause channel; no second JBIG2 mechanism is added.

Cancellation is separate from progressive pause:

- Pause means the same render remains valid and will resume.
- Cancellation means the request is obsolete and its result is abandoned.

After a cancelled image operation, `CPDF_RenderStatus` marks the render stopped and does not invoke `DrawObjWithBackground()`. This prevents a cancelled decode from replacing valid continuity with a blank fallback.

## Architecture Properties

- Pixel-correct: no decode, conversion, stretch, or composite math changes when cancellation is false.
- Universal: keyed only by the existing request cancellation state, not filename, page type, codec size, or page classification.
- Memory-bounded: no new bitmap, cache, queue, index, or retained callback owner.
- Lock-neutral: no mutex, critical section, or synchronization owner is added.
- UI-thread neutral: all checks run inside the existing native render worker.
- Single source of truth: the existing render callback snapshot remains authoritative.
- Fail-closed: unsupported/system OpenJPEG builds retain canonical decode behavior with pre/post decode checks.
- Responsive: cancellation granularity follows natural codec units and ten-row stretch units rather than whole images.

## Expected Result

This revision should reduce the time between a stale preview/tile cancellation and admission of the newest visible request. The strongest effect is expected on scanned pages containing JPX images and on large image stretches. Fast scrolling should spend less time waiting behind invisible work.

This revision is not expected to make a fully visible, uncancelled cold render materially faster. Its uncancelled overhead is sparse callback checks and should be measured for regression, with a target below 3% median render time.

## Validation

Completed locally:

- patch applies cleanly after the declared accepted stack;
- `git diff --check` passes;
- modified bundled OpenJPEG C translation units pass `clang -std=c11 -fsyntax-only`;
- a unit test cancels JPX before decode and then performs a successful canonical decode of the same source;
- workflow guards reject document-specific routing, new caches, new locks, and hot-loop Android logging.

The GitHub workflow is the authoritative full C++ build and unit-test compile because the local source worktree does not contain gclient-generated Android build dependencies.

## Device Evaluation

Evaluate two independent outcomes:

1. Uncancelled correctness and throughput on `6Steps`, `11.pdf`, `Q16`, and EP23. Pixels must remain unchanged and median render time must not regress materially.
2. Cancellation latency on `disquisitionesa00gaus.pdf`: begin a scanned-page preview or tile, immediately scroll or change zoom, and measure the delay until the newest visible request begins native rendering.

Success is lower stale-request ownership time and faster newest-visible admission. Do not interpret a faster blank result as success; cancelled results must not be published.

## Residual Bound

Cancellation is cooperative, not preemptive. The largest remaining indivisible OpenJPEG units are an active codeblock and an inverse-wavelet or color-transform component. If device data still shows a long cancellation tail inside one of those units, the next revision should subdivide that exact codec phase rather than add scheduler generations, page classifiers, or duplicate render queues.
