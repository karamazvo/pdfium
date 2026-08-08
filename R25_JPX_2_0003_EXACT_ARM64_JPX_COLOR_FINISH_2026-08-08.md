# r25-jpx-2-0003: Exact ARM64 JPX Color Finish

Locked: 2026-08-08 CST

## Revision

- Revision: `r25-jpx-2-0003`
- Base: `r25-cancel-1-0003`
- Patch: `patches/experiments/jpx/0003-exact-arm64-jpx-color-finish.patch`
- Workflow: `.github/workflows/pdfium-android-arm64-r25-jpx-2-0003-exact-arm64-jpx-color-finish.yml`
- Excluded: rejected `r25-image-1-0001` and `r25-jpx-1-0002`

This is a native PDFium/OpenJPEG experiment. It does not change Kotlin/JNI
scheduling, page routing, the huge-path executor, PDF parsing, editing, save
semantics, image resolution, decode area, masks, blending, or painter order.

## Evidence And Pivot

`r25-jpx-1-0002` did not meet its acceptance gate. Compared with
`r25-cancel-1-0003`, the controlled device run showed:

- completed 756-pixel median approximately 11% slower;
- first cold zoom tile approximately 13% slower;
- warm tile median unchanged at roughly 56 ms;
- cancellation p95 worsening from roughly 20 ms to 27 ms.

OpenJPEG source review explains the result. Its Tier-1 decoder and both 5/3
and 9/7 inverse DWT implementations already submit bounded jobs to the codec
thread pool. Another component or DWT scheduler would duplicate coordination
around memory-bandwidth-bound work and risk oversubscription.

The remaining serial work for the observed scan-page resources is the final
color path. Each page contains two 8-bit RGB JPX images and a JBIG2 mask. The
JPX streams use irreversible 9/7 transform and MCT. On ARM64, OpenJPEG's MCT
is scalar because its existing SIMD branch is SSE-only. PDFium then performs
another scalar pass over each component to write interleaved destination
bytes.

## Invariant

> Execute the same irreversible MCT operations and the same 8-bit component
> conversion in the same sample and channel order; change only the number of
> independent lanes processed by one CPU instruction.

The revision must preserve:

- the same OpenJPEG decode area, reduction level, tile, and components;
- `y + v * 1.402`, `y - u * 0.34413 - v * 0.71414`, and
  `y + u * 1.772` with the same operation grouping;
- no fused multiply-add contraction;
- the same signed-component offset;
- the same low-byte conversion used by `static_cast<uint8_t>`;
- RGB/BGR swapping and optional alpha channel positions;
- row padding initialized to `0xff`;
- the existing per-output-row cancellation boundary;
- canonical scalar execution for every unsupported input.

## Mechanism

```text
OpenJPEG Tier-1 and inverse DWT
              |
              v
three planar float components
              |
              v
ARM64 NEON irreversible MCT, four independent samples per vector
              |
              v
three or four planar 8-bit integer components
              |
              v
ARM64 NEON low-byte narrowing plus RGB/RGBA interleaved stores,
sixteen independent pixels per vector group
              |
              v
unchanged PDFium image cache, transform, mask, blend, and bitmap
```

The MCT uses separate multiply and add/subtract intrinsics matching the scalar
expression tree. It intentionally does not use fused multiply-add.

The packing path activates only for three- or four-channel, non-null, 8-bit
components with the dimensions and precision already validated by PDFium.
Every other case enters the unchanged scalar implementation. The common SIMD
path does not construct the two temporary `channel_bufs` and `adjust_comps`
vectors because their values are consumed directly.

## Resource Contract

- No new thread or worker pool.
- No new lock or synchronization point.
- No new persistent or destination-sized allocation.
- No cache, second pixel owner, page classifier, or document-specific policy.
- Existing component buffers and destination bitmap remain authoritative.
- Existing row cancellation checks remain before each packed output row.
- Scalar fallback remains available on non-ARM targets and unsupported JPX.

## Local Proof

The patch was authored against the exact accepted stack through
`r25-cancel-1-0003`; the rejected worker patch was reversed first. A local
ARM64 probe tested 10,000 randomized groups for each operation:

- NEON narrowing matched scalar `static_cast<uint8_t>(sample + offset)` for
  every byte;
- NEON MCT outputs were bit-identical to the scalar operation grouping;
- `third_party/libopenjpeg/mct.c` passed ARM64 C syntax compilation;
- the patch applies cleanly to the selective base and passes `git diff
  --check`.

The GitHub workflow also fails if worker-pool symbols from `r25-jpx-1-0002`
appear in the applied source.

## Expected Result And Gate

This is not an x10 claim. It removes serial arithmetic and memory passes after
entropy/wavelet decode. Expected cold JPX improvement is approximately
10-25%; warm cached tiles and non-JPX pages should remain within noise.

Accept only if all gates pass:

1. completed output is byte-identical on the native pixel corpus;
2. repeated cold 756-pixel JPX median improves at least 15% from
   `r25-cancel-1-0003`;
3. first cold tile improves without degrading warm tiles;
4. cancellation p95 does not exceed the accepted baseline;
5. 6Steps, 11.pdf, Q16, and EP23 retain correct pixels and performance;
6. no blank, partial, stale, or lower-resolution publication appears.

If the gain is below 15%, reject this patch and move to an exact fused
MCT-level-shift-destination writer. Do not add more decoder threads or a
document-specific routing threshold.
