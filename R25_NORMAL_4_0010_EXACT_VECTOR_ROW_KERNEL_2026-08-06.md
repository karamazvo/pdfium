# r25-normal-4-0010: Exact Vector Row Kernel

Date: 2026-08-06 (Asia/Taipei)

## Decision

Start a new normal-page experiment from the accepted latency base:

`r25-8-0138 + normal-exp-0005 + normal-exp-0006`

Do not include `normal-exp-0007`, `r25-normal-2-0008`, or
`r25-normal-3-0009`. On the complete 32-page 6Steps motion sample, 0009
improved the maximum from about 202 ms to 168 ms but regressed average,
median, and P90 relative to 0006. It therefore failed the 10% acceptance gate.

## Root cause addressed

The accepted 0005/0006 pipeline has already removed the oversized full-page
intermediate and duplicate decoder realization. Its vertical image filter still
executes one scalar source-row loop per destination channel, even when PDFium's
authoritative weight table proves that the result uses only one or two rows.

0010 changes only that execution cost. It does not change decode dimensions,
image quality, filtering policy, page routing, caching, scheduling, or the
canonical PDF model.

## Mechanism

`CStretchEngine::WeightTable` remains the single source of truth.

For 8-bit mask and packed BGR output:

1. A one-row span is copied only when its exact weight is 65536.
2. A two-row span is accepted only when both weights sum exactly to 65536.
3. ARM64 NEON widens each source byte, computes the two unsigned fixed-point
   products in 32-bit lanes, adds them, and shifts the sum right by 16.
4. Remaining bytes use the same scalar products, sum, and truncation.
5. Variable support, alpha output, other formats, and non-ARM64 execution use
   the unchanged canonical loops.

The kernel treats active mask and BGR row bytes uniformly because the same
vertical weights apply independently to every channel. It adds no format
conversion or deinterleaving stage.

## Invariants

- PDFium's calculated weights are unchanged.
- Horizontal filtering and vertical filtering remain separate and ordered.
- Products are accumulated before the existing `>> 16` truncation.
- No approximation, reduced-resolution decode, threshold, or classifier exists.
- No bitmap, row buffer, table, cache, persistent object, thread, lock, or log is
  added.
- The 0005 one-MiB bounded-row cap remains authoritative.
- The 0005/0006 cancellation cadence remains unchanged.
- Heavy-path rendering remains owned by r25-8-0138.
- Unsupported cases fail closed to canonical PDFium before producing pixels.

## Verification

`ExactVectorRowsMatchWeightTableReference` independently calculates both filter
passes from `WeightTable` and compares every output channel for mask and BGR.
The 29-pixel destination width covers vector blocks and scalar tails. Existing
bounded-row versus canonical tests continue to cover both memory modes and BGRA
fallback.

The GitHub workflow is:

`.github/workflows/pdfium-android-arm64-r25-normal-4-0010-exact-vector-row-kernel.yml`

The native patch is:

`patches/experiments/normal-page/0010-exact-vector-row-kernel.patch`

## Performance gate

Compare repeated cold first-scroll runs of 6Steps against 0006 on the same
device, viewport, build type, and interaction sequence.

Accept 0010 only when all conditions hold:

- Pixel output and resolution are unchanged.
- Page-matched median render time improves by at least 10%.
- Maximum latency and cancellation responsiveness do not regress.
- 11.pdf, Q16, and EP23 retain r25-8-0138 performance and correctness.

The expected whole-page gain is approximately 5-15%; this is not a 10x
architecture. If the measured gain is below 10%, stop exact-resampler work. The
remaining normal-page cost is source realization or raster/composition and must
be addressed there rather than by another sampling special case.
