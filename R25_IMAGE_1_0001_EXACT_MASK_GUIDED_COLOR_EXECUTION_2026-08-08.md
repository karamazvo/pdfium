# r25-image-1-0001: Exact Mask-Guided Color Execution

Locked: 2026-08-08 CST

## Revision

- Revision: `r25-image-1-0001`
- Base: `r25-cancel-1-0003`
- Patch: `patches/experiments/image/0001-exact-mask-guided-color-execution.patch`
- Workflow: `.github/workflows/pdfium-android-arm64-r25-image-1-0001-exact-mask-guided-color-execution.yml`
- Scope: native PDFium masked-image rendering only

This revision does not change Kotlin/JNI scheduling, page classification,
normal unmasked images, the heavy-path executor, PDF parsing, editing, or save
semantics.

## Decision

Do not repeat the rejected `normal-page/0007` or `0008` destination-row fusion
experiments. Those revisions removed intermediate allocations but still
performed color resampling across the complete destination and did not improve
the accepted `0006` latency baseline.

`r25-image-1-0001` is a different experiment. It uses the exact, canonically
resampled destination mask to avoid color resampling arithmetic for pixels that
provably cannot affect the output.

## Root Cost

The accepted masked-image path is correct and cancellable, but it still does
this for every destination pixel:

```text
color decode -> horizontal resample -> vertical resample
mask decode  -> mask resample
color + mask -> destination
```

For MRC scan pages, a high-resolution JPX foreground can be paired with a
sparse JBIG2 mask. Most foreground color samples are unobservable because the
final destination mask is exactly zero there. Computing those samples is
redundant work.

## Invariant

Color work may be omitted only when the final canonically resampled destination
coverage is exactly zero. Every pixel with nonzero coverage must use PDFium's
unchanged source decoder, weight tables, fixed-point products, truncation,
scanline compositor, clip, and painter order.

## Mechanism

The eligible path is:

```text
canonical mask decode and resample
              |
              v
exact 8-bit destination coverage
              |
              +-> bounded source-row support map
              |        |
              |        v
canonical color source -> bounded exact color stretcher
                               |
                               v
                    existing AGG compositor
```

1. Render the mask first through the existing PDFium mask renderer.
2. Build one bounded support map from exact nonzero destination coverage.
3. For each destination row, use the existing vertical weight table to mark
   every source row that can contribute to a visible destination pixel.
4. Skip horizontal color products only where no visible destination pixel can
   consume that source-row/column result.
5. Skip vertical color products only where destination mask coverage is zero.
6. Composite every nonzero-coverage pixel with the existing AGG compositor.
7. If the mask is empty, finish without decoding or stretching the color
   source.

The support map is conservative: it may retain extra color work, but it cannot
remove a source contribution needed by a visible destination pixel.

## Logical And Physical Correctness

For Normal source-over composition, destination coverage `c = 0` gives:

```text
out = source * c + destination * (1 - c) = destination
```

Therefore color is mathematically and physically unobservable at exact zero
coverage. The revision does not infer zero from source bounds, file type,
document identity, or an approximation. It reads the final 8-bit destination
mask produced by PDFium's canonical sampler.

For `c != 0`, no color equation changes. The support map includes every source
row in the destination pixel's existing vertical weight range, and the
horizontal pass uses the existing horizontal weights. The operation order and
integer rounding for observable pixels remain unchanged.

Activation requires all predicates before destination mutation:

- opaque color source;
- global alpha exactly 1;
- Normal blend;
- no matte correction;
- no group knockout or backdrop;
- no device clip mask;
- axis-aligned, nondegenerate transform;
- exact 8-bit destination mask matching the clipped image rectangle;
- successful bounded stretcher and support-map allocation.

Any failed predicate or allocation returns to the accepted canonical path
before the new executor writes a destination pixel.

## Resource Contract

- Existing bounded stretcher ring: at most 1 MiB.
- Exact support map: at most 1 MiB.
- Existing destination mask bitmap: unchanged and remains authoritative.
- No destination-sized color offscreen in the eligible direct executor.
- No persistent cache, map, page classifier, thread, lock, JNI call, or Kotlin
  policy.
- Existing page-image cache remains the sole decoded-source owner.
- Existing progressive pause and render cancellation reach color execution.

The map lifetime is one image continuation and is released immediately after
that image finishes or is cancelled.

## Expected Result

This revision can materially improve sparse masked-image pages by reducing
color resampling and eliminating the destination-sized color offscreen. It is
not expected to improve unmasked pages, dense masks, unsupported transforms,
or codec entropy decoding.

No speedup is claimed before device evidence. Compare against
`r25-cancel-1-0003` using the same cold-scroll sequence for
`disquisitionesa00gaus.pdf`:

- 756-pixel motion-preview `engineRenderMs` and `bitmapRenderMs`;
- cancellation-to-return P50/P90/P95;
- first visible publish latency;
- output dimensions and pixel corpus;
- 6Steps, 11.pdf, Q16, and EP23 regression checks.

Accept only if:

1. completed output is byte-equivalent on the native correctness corpus;
2. cold masked-page median improves by at least 15%;
3. cancellation P95 remains below the accepted bound;
4. no blank, partial, lower-resolution, or delayed normal page appears;
5. ordinary and huge-path pages stay within controlled run variance.

If the gain is below 15%, stop this executor direction. The remaining cost is
codec/source realization and needs a codec-level design, not another
destination executor.
