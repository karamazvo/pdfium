# r25-18-0148 Canonical Render Phase Profile

**Date:** 2026-08-02 Asia/Taipei
**Revision:** `r25-18-0148`
**Parent performance base:** accepted `r25-8-0138`
**Status:** diagnostic implementation; build and device profile pending

## Purpose

0148 measures the real cost of the current progressive PDFium pipeline before
another optimization is selected. It does not change pixels, scheduling,
eligibility, caches, or rendering policy. Rejected revisions 0139-0147 are not
part of this build.

The older `experiments/0010` profiler is not reused because it is based on the
obsolete r8 stack, does not cover the app's progressive Start/Continue path as
one render, and reports overlapping inclusive totals. 0148 reports exclusive
native phases whose sum cannot double-count nested Forms or transparency.

## End-to-end attribution

For one `Page render success` line, pair the adjacent `PdfRenderCost` and
`VeloceCanonicalProfile` lines from the single `xpdf-session` thread.

```text
request-to-visible
  = queueWaitMs
  + bitmapAcquireUs / 1000
  + engineRenderMs
  + mainDelayMs
  + mainApplyMs

engineRenderMs
  = coordinator/session dispatch
  + sessionWaitUs / 1000
  + nativeTotalMs
  + result handoff

nativeTotalMs
  = acquireMs
  + bitmapRenderMs
  + pageAttributesMs
  + linkMapMs

bitmapRenderMs * 1000
  = PDFium public/SDK setup overhead
  + activeUs

activeUs
  = traversalUs
  + setupUs + clipUs + transparencyUs
  + textUs + pathUs + imageUs + shadingUs + formUs
  + backgroundUs + softMaskUs + acceleratorUs + parseUs
```

Small differences are expected from microsecond/millisecond rounding and code
outside the measured progressive renderer.

## Measured 0147 baseline

The 2026-08-02 0147 device logs establish the coarse boundary before 0148.
These first-preview values are measured, not estimates:

| Document | Acquire/compile | Bitmap render | Native total | App engine | Queue wait |
|---|---:|---:|---:|---:|---:|
| 11.pdf | 200 ms | 177 ms | 378 ms | 393 ms | 2 ms |
| Q16 | 3,783 ms | 1,293 ms | 5,077 ms | 5,091 ms | 6 ms |
| EP23 first captured page | 12 ms | 109 ms | 122 ms | 153 ms | 2 ms |
| 6Steps first page | 23 ms | 142 ms | 166 ms | 187 ms | 2 ms |

The associated acquisition records show:

- 11.pdf: 28,806 source commands, 198,060 us parse, 1,718 us finish,
  3.75 MiB program;
- Q16: 3,165,420 source commands, 3,783,343 us parse, 26,913 us finish,
  56.99 MiB program;
- 6Steps first page: `canonical_no_exact_candidate`, 21,727 us probe/parse,
  no retained RenderProgram.

Across the complete captured logs, successful app render samples had these
coarse medians:

| Document | Engine median | Queue median | Important limitation |
|---|---:|---:|---|
| 11.pdf | 105 ms | 751 ms | serialized tile backlog dominates visibility |
| Q16 | 24 ms | 2,800 ms | one 3,601 ms dense tile blocks later sparse tiles |
| EP23 | 65 ms | 653 ms | four page acquisitions; first page is not p2/p3 |
| 6Steps | 97 ms | 2 ms | canonical bitmap rendering dominates normal-page cost |

`RenderPage slow` is thresholded, so its aggregate averages are biased toward
slow samples. The first-preview rows and app success medians above are valid;
0148 must supply the missing exclusive split inside `bitmapRenderMs`.

## Native profile design

`VeloceCanonicalRenderProfile` is owned by one
`CPDF_ProgressiveRenderer`. It contains fixed arrays only:

- no heap allocation;
- no cache or global map;
- no lock;
- one thread-local non-owning pointer while Continue executes;
- a fixed 96-frame exclusive phase stack;
- one log line when a render completes, closes, or observes cancellation.

Entering a nested phase first charges elapsed time to its parent, pauses the
parent, then charges the child. Exiting resumes the parent. Form children,
soft masks, transparency recursion, and background fallback therefore do not
appear in two duration buckets.

The profiler records these routes independently:

- `route=canonical`: normal progressive page-object rendering;
- `route=path_display_list`: legacy holder display-list executor;
- `route=render_program`: accepted RenderProgram executor.

Never aggregate those routes together when diagnosing canonical pages.

## Log fields

### `PdfRenderView`

```text
queueWaitMs       wait before PdfRenderViewRenderer starts the request
bitmapAcquireUs   exact BitmapPool lookup or Bitmap.createBitmap boundary
engineRenderMs    coordinator call through returned engine result
totalTaskMs       bitmap acquisition plus engine and local result construction
```

### `PdfRenderCost`

```text
sessionWaitUs     time queued behind earlier work on xpdf-session
acquireMs         retained-page lookup or open/parse/evict/close
bitmapRenderMs    PDFium progressive bitmap call
pageAttributesMs  native link/page-attribute extraction
linkMapMs         Kotlin mapping to PdfLink
nativeTotalMs     acquisition through link mapping; excludes sessionWaitUs
```

### `VeloceCanonicalProfile`

```text
activeUs          wall time executing ProgressiveRenderer::Continue
traversalUs       exclusive uncategorized loop/culling/pause/checkpoint work
setupUs           render status, transparency state, device state and clip setup
clipUs            clip path processing
transparencyUs    transparency work excluding nested categorized work
textUs            text and glyph rendering
pathUs            canonical path rendering
imageUs           image start/decode/scale/composite/continuation
shadingUs         shading rendering
formUs            Form overhead excluding nested categorized child work
backgroundUs      fallback buffer setup and composite excluding child drawing
softMaskUs        soft-mask work excluding nested categorized child work
acceleratorUs     path-display-list or RenderProgram entry/replay
parseUs           parse continuation performed during progressive rendering
```

The same line reports object counts, bounds culls, calls per phase,
`cancelChecks`, `cancelObserved`, continuation calls, pause returns, and stack
overflow. Any `depthOverflow=1` sample is invalid and must not be analyzed.

## Cancellation

`cancelChecks` measures existing PDFium checkpoints; it does not add new
checkpoints. `cancelObserved=1` proves the native progressive renderer saw the
abort. Pair it with existing `PdfRenderAbort flag-set/returned` logs:

```text
postAbortMs = native return time - abort flag-set time
```

This distinguishes queue cancellation, cancellation before native entry, and
latency inside a long non-interruptible primitive.

## Profiling protocol

1. Install the `r25-18-0148` artifact and the integration-worktree app with
   `bitmapAcquireUs`, `sessionWaitUs`, and `PdfRenderCost` logging.
2. Force-stop the app before each cold run.
3. Capture at least three cold runs and three warm runs per document.
4. Test 6Steps normal pages, `disquisitionesa00gaus.pdf` scanned-image pages,
   11.pdf, Q16, and EP23.
5. Separate preview and region renders and group native data by `route`.
6. Use medians. Do not compare one profiled run with one historical outlier.

Useful filter:

```bash
adb logcat -v time \
  | grep -E 'Page render (success|dropped)|PdfRenderCost|VeloceCanonicalProfile|PdfRenderAbort'
```

## Profiling overhead rule

0148 is a diagnostic build, not a performance candidate. It calls the steady
clock at exclusive phase boundaries. Run accepted 0138 and 0148 back-to-back;
if 0148 increases median `bitmapRenderMs` by more than 5%, use phase shares and
counts for diagnosis but do not treat its absolute time as production cost.

The next performance revision must be selected from measured dominant cost.
Do not optimize traversal, allocation, path, image, Form, or transparency
until 0148 shows that phase dominates the target document.

## Device result (2026-08-03)

The profile completed on the target device:

- 11 cold acquisition/bitmap measured about 185/121 ms; accelerator work was
  about 99.6% of active bitmap time.
- Q16 cold acquisition/bitmap measured about 3,677/1,630 ms; the dense region
  still selected and drew roughly 2.39M commands.
- EP23 page 2/page 3 acquisition measured about 852/844 ms. Page 3 logged a
  cache hit for the same 28,071-fill, 707,225-point, roughly 10.3-MiB Form
  program, proving the cache was attached only after repeated parsing and
  child materialization.
- 6Steps samples were image dominated.
- Typical session-lane waits and bitmap allocation were below one
  millisecond. No sample reported depth overflow.

Conclusion: do not optimize lock acquisition or bitmap allocation. `0149`
targets the proven late Form-cache ownership issue. Q16 cold root acquisition,
Q16 dense replay, and normal-page image rendering remain separate measured
problems.
