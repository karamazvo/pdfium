# r25-cancel-1-0003: Bounded Exact Masked-Image Composition

Locked: 2026-08-07 18:15:00 CST

## Revision

- Revision: `r25-cancel-1-0003`
- Base: `r25-cancel-1-0002`
- Patch: `patches/experiments/cancellation/0003-bounded-exact-masked-image-composition.patch`
- Workflow: `.github/workflows/pdfium-android-arm64-r25-cancel-1-0003-bounded-exact-masked-image-composition.yml`
- Scope: native PDFium masked-image execution only; no Kotlin/JNI scheduler,
  classifier, cache policy, or huge-path executor change

## Device Evidence

The `cancel-1-0002` capture for `disquisitionesa00gaus.pdf` showed:

- 756-pixel motion previews at about 203 ms median;
- 1080-pixel settle renders at about 488 ms median;
- cancellation-to-return at 16 ms median and 83 ms P90;
- one 143 ms cancellation tail on page 634;
- no 540-pixel hidden-thumbnail renders;
- negligible render-session lock wait.

The affected scan pages place JPX color and JBIG2 coverage images with an
axis-aligned matrix. They therefore do not enter the affine transform changed
by `cancel-1-0002`.

## Root Cause

Normal-page revision `0011` correctly replaces the canonical temporary BGRA
mask materialization with an exact direct AGG coverage composite. The direct
executor called `CFX_DIBitmap::CompositeBitmap()` once for the complete draw
rectangle, however. Its general scanline loop had no cancellation boundary.

Masked-image rendering also used nested color and mask renderers with a null
pause indicator. Consequently, the outer render cancellation token could be
observed by JPX/JBIG2 loading and the main stretcher, then become invisible
again during nested masked-image stretch, matte correction, or final direct
coverage composition.

This is a progressive-execution contract gap, not a document classification
problem.

## Invariant

Every expensive stage owned by one masked-image render must observe the same
request cancellation token at a bounded natural work-unit boundary.
Unsupported execution must be distinguishable from cancellation so an
obsolete render can never enter the canonical fallback and repeat work.
Completed rendering must retain canonical PDFium pixels.

## Mechanism

The existing render-status cancellation snapshot is passed through the full
masked-image chain:

1. Nested color-image stretch uses a cancel-only pause adapter.
2. Nested mask stretch uses the same adapter.
3. Matte correction checks cancellation every 32 destination rows.
4. Exact AGG coverage composition processes at most 32 rows or 32,768 pixels
   per unit, whichever is smaller.
5. Render-device masked composition returns one explicit result:
   `unsupported`, `success`, `cancelled`, or terminal `failure`.
6. Only `unsupported` may enter the canonical alpha-materialization fallback.
7. `CPDF_RenderStatus` checks cancellation again after the final image
   continuation before considering background fallback.

The chunk executor invokes the existing `CompositeBitmap()` with consecutive
row slices. Source rows, mask rows, destination rows, blend mode, scanline
compositor, and pixel order are unchanged.

## Architecture Properties

- Pixel-exact: successful execution uses the same compositor and row order.
- Fail-closed: unsupported execution retains the canonical fallback; partial
  failure or cancellation never falls through after changing pixels.
- Universal: based only on operation semantics and cancellation state.
- Memory-bounded: no bitmap, cache, queue, index, or retained task is added.
- Lock-neutral: no mutex, critical section, or synchronization owner is added.
- UI-thread neutral: all checks execute on the existing native render worker.
- Single source of truth: `CPDF_RenderStatus` remains the cancellation owner.
- Minimal: one result enum and bounded loop extend the existing exact executor.

An obsolete render may leave partial pixels in its private destination bitmap,
which is already PDFium's progressive-render cancellation contract. The app
must discard the cancelled bitmap and never publish it.

## Expected Result

For MRC scan pages using JPX color plus JBIG2 coverage:

- cancellation P95 target below 50-75 ms;
- removal of the observed 143 ms final-composite tail when that stage owns it;
- no blank or partial bitmap publication;
- uncancelled median regression below 3%;
- no material reduction in cold 756-pixel rendering time, because this
  revision improves stale-work release rather than codec throughput.

## Validation

The patch includes native unit coverage that:

- compares completed bounded direct composition byte-for-byte with canonical
  alpha-mask materialization and composition;
- cancels on the second chunk check and verifies that an earlier row changed
  while an unprocessed final row remains unchanged;
- verifies cancellation returns `cancelled`, not `unsupported`.

Local patch validation completed:

- generated as an incremental diff after the complete `cancel-1-0002` stack;
- `git diff --check` passes;
- reverse-apply verification passes;
- no document name, page classifier, threshold, lock, cache, or Android
  hot-loop logging was introduced.

The GitHub workflow is authoritative for the Android C++ build and
`pdfium_unittests` compile.

## Next Decision

Do not add another cancellation revision unless device evidence still shows a
post-cancel P95 above 75 ms. If that occurs, locate the remaining indivisible
codec or raster stage and subdivide that stage directly.

Cold MRC throughput is a separate track. Its next architectural target is one
bounded destination-row executor that consumes decoded JPX color and JBIG2
coverage without duplicate materialization while preserving the same PDFium
pixel equations.
