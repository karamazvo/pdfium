# r25-jpx-6-0007: Bounded Exact JPX Source-Window Reuse

Date: 2026-08-10 (Asia/Taipei)

## Parent

`r25-jpx-5-0006` is the direct parent. The accepted base remains PDFium r25
plus page dimensions `0051`, ordered stroke renderer `0138`, the accepted
normal-image and cancellation patches, and exact clipped JPX decode
`0005-0006`.

## Measured problem

`0006` approximately doubled zoom-region output throughput on
`disquisitionesa00gaus.pdf`, but it did not improve the full-page 756-pixel
preview. PDFium already caches complete decoded images by page image stream.
It cannot safely cache the clipped result from `0005`, because that bitmap
contains pixels for only one source rectangle and zeros outside it.

Consequently, a later request for the same or a contained source rectangle
decodes the JPX stream again even when the previous exact decoded window is
still alive during rendering.

## Invariant

> A retained clipped JPX bitmap may satisfy a later request only when it
> belongs to the same page-image cache entry, its source rectangle completely
> contains the later required source rectangle, and the later request has the
> identical maximum decode size.

Anything else follows the existing decode path. There is no overlap estimate,
union, sparse merge, page classification, or document-specific policy.

## Mechanism

The existing `CPDF_PageImageCache` owns one additional retained source window:

- the normal complete-image cache is checked first;
- a source-window hit requires exact rectangle containment and the identical
  requested decode size;
- successful clipped JPX output is retained directly, without copying pixels;
- at most one image entry in a page cache owns a source window;
- replacing the owner releases the previous window;
- a complete-image decode releases a now-redundant source window;
- cache reset, image mutation, LRU entry eviction, page close, and document
  close release the window through existing ownership;
- canceled, failed, incomplete, oversized, or non-clipped output is never
  inserted.

The hard retained-window ceiling is 16 MiB per page cache. The app's native
render-page LRU retains at most four pages, bounding incremental source-window
memory to 64 MiB in this integration. This patch adds no worker, lock, queue,
global map, or UI/JNI operation.

## Correctness

The cache never changes decode, color conversion, mask, stretcher, blending,
or painter-order math. A hit returns the same immutable `SourceWindowDIB`
that the canonical renderer consumed previously. Rectangle containment proves
that every source sample required by the later stretcher request is present.
The exact requested-size check prevents either lower- or higher-resolution JPX
reduction output from substituting for the decode that `0006` would perform.
The retained bitmap's logical dimensions are also validated against the
request.

The unit test establishes:

1. a contained source rectangle reuses the same retained bitmap;
2. a rectangle extending outside retained coverage does not reuse it;
3. the existing canonical larger-after-thumbnail cache test remains compiled.

## Expected performance

This revision deliberately does not claim a cold-preview improvement.

- First 756-pixel full-page preview: unchanged.
- First request for a new clipped rectangle: unchanged from `0006`.
- Repeated render of the same region: removes the repeated JPX decode.
- A contained pan/settle request at the identical requested decode size:
  removes the repeated JPX decode.
- Disjoint regions: unchanged, with one bounded replacement.
- Normal, vector, DCT, JBIG2, 11, Q16, and EP23 paths: no new decode work.

## Acceptance

Accept only if all of the following hold:

1. PDFium unit tests compile and `ReusesContainedJpxSourceWindow` passes.
2. Full-resolution pixel comparison against `0006` is byte-identical for
   preview and zoom regions, including color and masks.
3. Repeated or contained zoom-region renders show a material native-time
   reduction.
4. Cold first preview, cancellation latency, and normal-page rendering remain
   within measurement noise of `0006`.
5. Memory remains within the 16 MiB per-page-window contract.

If the measured interaction consists only of disjoint first-use regions,
`0007` should be rejected as low-value rather than broadened into an
approximate or unbounded spatial image cache.
