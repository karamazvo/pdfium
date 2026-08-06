# r25-normal-5-0011: Exact Mask Execution Plan

Date: 2026-08-06 (Asia/Taipei)

## Decision

Start a new normal-page experiment from the accepted latency base:

`r25-8-0138 + normal-exp-0005 + normal-exp-0006`

Do not include `normal-exp-0007`, `r25-normal-2-0008`,
`r25-normal-3-0009`, or `r25-normal-4-0010`. The complete 6Steps sample
showed no aggregate improvement from 0010: average remained about 63.8 ms,
median regressed from about 56.5 ms to 72 ms, 19 of 32 pages became slower,
and only maximum latency improved. Exact resampler specialization is therefore
closed until a new profile proves a different dominant cost.

## Root cause addressed

PDFium currently realizes and executes every soft mask as variable coverage.
That is necessary while coverage is unknown, but it repeats unnecessary mask
stretch and composition work when a fully decoded 8-bit mask is the identity
operation: every pixel is exactly 255.

For genuinely variable masks, AGG also takes an avoidable route in an exact
common case: it first materializes a BGRA bitmap carrying the mask as alpha and
then composites that bitmap. AGG's existing scanline compositor can consume
the same mask directly as coverage.

The invariant is:

> A mask may be removed only after complete decoding proves every logical
> coverage byte is 255. Any variable, incomplete, malformed, or unsupported
> state remains canonical.

## Mechanism

0011 models mask execution as one exact three-state property on `CFX_DIBBase`:

- `kUnknown`: incomplete or not tracked; canonical behavior is mandatory.
- `kOpaque`: all rows have completed and every logical byte is exactly 255.
- `kVariable`: at least one byte differs from 255.

`CanonicalRowCache` learns this property while copying rows it already must
decode for the accepted 0006 pipeline. It does not perform a second scan. Once
a cached mask is proven opaque, the next cache reuse drops the identity mask
and corrects the existing page-cache byte accounting.

For variable masks, AGG implements `SetBitsWithMask()` only when all exact
equivalence predicates hold:

- source is opaque BGRx;
- mask is 8-bit coverage with identical dimensions;
- alpha is exactly 1;
- blend mode is Normal;
- group knockout is disabled;
- no soft clip mask is active.

The existing `CompositeBitmap()` scanline path then consumes mask bytes
directly. Any failed predicate returns `false` before pixels are produced and
the unchanged canonical BGRA path runs.

## Architecture properties

- No document name, page class, object-count threshold, or content routing.
- No second decode, mask scan, bitmap, cache, spatial index, thread, or lock.
- The only persistent metadata is the exact three-state property in the
  existing row-cache object.
- The accepted one-MiB bounded-row cap and cancellation cadence are unchanged.
- Proven identity masks are released, so retained memory can decrease.
- Source order, PDFium page objects, image color conversion, interpolation,
  alpha semantics, and canonical fallback remain authoritative.
- Heavy-path rendering remains owned by r25-8-0138.

## Verification

`OpaqueBitmapMaskMatchesCanonicalBgraComposite` compares direct mask coverage
against PDFium's canonical BGRA materialization over BGR, BGRx, and BGRA
destinations, multiple destination alpha values, coverage values from 0 to
255, and both byte orders.

The GitHub workflow is:

`.github/workflows/pdfium-android-arm64-r25-normal-5-0011-exact-mask-execution-plan.yml`

The native patch is:

`patches/experiments/normal-page/0011-exact-mask-execution-plan.patch`

## Performance gate

This revision targets the measured mask and image-device portions; it is not a
10x architecture. Compare page-matched cold and repeated first-scroll samples
of 6Steps against 0006 on the same device and interaction sequence.

Accept 0011 only when all conditions hold:

- Pixel output and resolution are unchanged.
- Page-matched median or P90 improves by at least 10% on mask-bearing pages.
- Whole-document average does not regress.
- Maximum latency and cancellation responsiveness do not regress.
- 11.pdf, Q16, and EP23 retain r25-8-0138 performance and correctness.

Expected behavior is a modest cold-render reduction for eligible variable
masks and a larger repeated-render reduction only where a mask proves fully
opaque. If the gate fails, stop mask optimization. The next architecture must
target source image realization and raster/composition as a single compiled
image operation, because those remain the dominant normal-page costs.
