# r25-cancel-1-0002: Bounded Affine Image Transform Cancellation

Locked: 2026-08-07 17:08:52 CST

## Revision

- Revision: `r25-cancel-1-0002`
- Base: `r25-cancel-1-0001`
- Patch: `patches/experiments/cancellation/0002-bounded-affine-image-transform-cancellation.patch`
- Workflow: `.github/workflows/pdfium-android-arm64-r25-cancel-1-0002-bounded-affine-image-transform-cancellation.yml`
- Scope: native PDFium image execution only; no Kotlin/JNI scheduler, page classifier, cache policy, or huge-path executor change

## Root Cause

`CFX_ImageTransformer::ContinueOther()` accepted PDFium's existing `PauseIndicatorIface` but ignored it. After decode and stretch completed, a rotated, sheared, or otherwise affine image entered one bilinear loop over the complete destination bitmap. A request that became stale during that loop could not release the single native render owner until the whole transform finished.

This is a generic progressive-execution contract violation. It is independent of document identity, page classification, source codec, or output scale.

## Invariant

Every expensive progressive image stage must return to its existing caller after a bounded natural work unit. Uncancelled rendering must preserve the canonical destination dimensions, matrix, interpolation, pixel order, and composite result. Incomplete output must never be published.

## Mechanism

The existing affine bilinear loop now accepts an exact destination-row range. `ContinueOther()` processes at most:

- 32 destination rows; or
- 32,768 destination pixels;

whichever produces fewer rows, with a minimum of one complete row.

Between chunks it returns through the existing `CFX_ImageTransformer::Continue()` contract. The existing render-status cancellation snapshot remains the only cancellation owner.

The transformed bitmap is allocated once and retained privately by the transformer while incomplete. The completed stretch remains in `CFX_BitmapStorer`; only after the last affine row does the transformer atomically replace it with the completed destination bitmap. Destruction after cancellation releases the private partial bitmap without exposing it.

`CFX_DIBBase::TransformTo()` is synchronous, so it drains the same continuation until completion. Progressive AGG rendering continues to yield to its caller and can abandon stale work.

## Correctness

- The same `CFX_BilinearMatrix` calculation is used.
- Rows and pixels execute in the same ascending order.
- Channel interpolation and fixed-point rounding are unchanged.
- Source stretch data remains immutable for the entire transform.
- No partially transformed bitmap reaches composition.
- Allocation failure retains the prior fail-closed behavior.
- A unit test verifies multiple affine continuations and byte-identical output between progressive completion and synchronous `TransformTo()`.

## Architecture Properties

- Universal: applies to every affine PDFium image transform.
- Pixel-exact: scheduling boundaries do not alter raster math.
- Memory-bounded: one destination bitmap already required by the canonical transform; no duplicate output, cache, queue, or index.
- Lock-neutral: no mutex or synchronization primitive.
- UI-thread neutral: all work remains on PDFium's native render worker.
- Single source of truth: the existing render callback snapshot controls cancellation.
- Minimal abstraction: one row cursor and one private destination owner inside the existing transformer.
- Responsive: stale work returns after a bounded pixel/row unit instead of a complete destination bitmap.

## Expected Result

This revision should reduce long cancellation tails for affine images, especially while fast-scrolling scanned or image-heavy pages and while replacing stale zoom tiles. It does not make a fully visible uncancelled affine image substantially faster; its purpose is faster admission of the newest visible request.

Expected device evidence:

- fewer stale image renders with `postAbortMs` above 100 ms;
- lower `postAbortMs` P95, with a target of 50-100 ms;
- no blank or partially transformed image publication;
- uncancelled median render regression below 3%;
- no change to `11.pdf`/`Q16` huge-path routing or output.

## Validation

Local validation:

- patch generated as an increment after `r25-cancel-1-0001`;
- `git diff --check` passes;
- patch has no filename/page routing, threshold classifier, lock, cache, or Android hot-loop log;
- all known `CFX_ImageTransformer` callers were audited: progressive AGG preserves continuation, synchronous `TransformTo()` drains it.

The GitHub workflow is authoritative for the full Android C++ build and `pdfium_unittests`, because the local patch-authoring source does not include gclient-generated build dependencies.

## Residual Bound

Pure 90-degree `SwapXY()` remains monolithic, and a single exceptionally wide affine destination row remains the minimum indivisible work unit. Device traces should justify either subdivision before another native cancellation revision is added.
