# r25-normal-3-0009: Exact Fixed-Support Resampler

Date: 2026-08-06 (Asia/Taipei)

## Decision

Start a new normal-page experiment from the best accepted latency base:

`r25-8-0138 + normal-exp-0005 + normal-exp-0006`

Do not include `normal-exp-0007` or `r25-normal-2-0008`. The 6Steps first-scroll
measurement for 0008 averaged about 81 ms and reached 205 ms, so destination-row
fusion did not improve the accepted 0006 latency baseline.

## Bottleneck addressed

The image profile separates the remaining native cost into source realization and
device scaling/composition. UI application and drawing are normally 0-1 ms. In the
scaler, every destination pixel still executes a variable-support loop, performs
weight lookups, and repeatedly resolves source positions even when PDFium's exact
weight table contains only one or two adjacent source samples.

0009 specializes that common mathematical case. It does not change image decode
dimensions, interpolation policy, image quality, page routing, or scheduling.

## Mechanism

The authoritative `CStretchEngine::WeightTable` remains the single source of truth.
For a non-empty weight span containing one or two source samples:

1. Load the existing fixed-point weights once.
2. Load the one or two adjacent source samples directly.
3. Execute the same products, sum, and `PixelFromFixed()` truncation as the generic
   loop.
4. Reuse the same operation for the horizontal pass and for both canonical and
   bounded-row vertical passes.

Variable support and alpha-sensitive transforms retain the existing generic loop.
The optimization therefore fails closed by representation, not by a document or
page heuristic.

## Invariants

- PDFium's calculated weights and two-pass ordering are unchanged.
- Source and destination dimensions are unchanged.
- No approximation, reduced-resolution decode, classifier, or threshold is added.
- No buffer, table, cache, persistent object, thread, or lock is added.
- The 0005 one-MiB bounded-row memory cap remains authoritative.
- The 0005/0006 cancellation cadence remains unchanged.
- Heavy-path rendering remains owned by the accepted r25-8-0138 executor.
- Canonical rendering remains the fallback for every unsupported sampling case.

## Verification

`ExactFixedSupportMatchesWeightTableReference` independently computes the two-pass
result from the same public weight table for mask and BGR data, then compares every
output channel. Existing bounded-row versus canonical tests continue to cover both
execution modes and BGRA fallback behavior.

The GitHub workflow is:

`.github/workflows/pdfium-android-arm64-r25-normal-3-0009-exact-fixed-support-resampler.yml`

The native patch is:

`patches/experiments/normal-page/0009-exact-fixed-support-resampler.patch`

## Performance gate

Compare repeated cold first-scroll runs of 6Steps against 0006 on the same device,
viewport, build type, and interaction sequence.

Accept 0009 only when all of the following hold:

- Pixel output and resolution are unchanged.
- Median first-scroll render time improves by at least 10%.
- The intended useful target is a 10-20% whole-page improvement, corresponding to
  roughly 15-30% of the device scaling phase.
- Maximum render time and cancellation responsiveness do not regress.
- 11.pdf, Q16, and EP23 retain r25-8-0138 behavior and correctness.

This is not expected to provide a 10x improvement. If the measured whole-page gain
is below 10%, stop this direction; the next meaningful experiment must reduce source
realization work rather than add another destination executor.
