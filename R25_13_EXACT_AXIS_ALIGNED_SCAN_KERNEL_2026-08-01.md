# r25-13 Exact Axis-Aligned Scan Kernel

**Locked:** 2026-08-01 (Asia/Taipei)

**First revision:** r25-13-0143

**Extends:** accepted r25-8-0138

**Excludes:** rejected r25-9-0139 through r25-12-0142

**Status:** performance rejected by device result; keep for traceability and do
not extend

## Device Result

Q16 exercised 1,553,522 exact axis-aligned draws, about 60% of 2,590,767
raster passes, but bitmap time was 1,478 ms versus 1,465 ms on accepted 0138.
Acquisition regressed from 3,185 ms to 3,978 ms and total preview from 4,651 ms
to 5,456 ms. The kernel therefore did not reduce the dominant measured cost.
`r25-14-0144` restarts from 0138 and excludes this patch.

## Root Cost

The accepted 0138 Q16 preview still performs about 2.59 million independent
stroke raster/composite operations. It already reuses renderer, rasterizer,
scanline, transform, and path scratch. The direct butt-line path avoids the
generic stroker, but every four-vertex rectangle still enters AGG's general
cell builder, row index, per-row cell sort, sweep, and scanline renderer.

0142 changed the command-to-driver representation without reducing this work
and regressed Q16 total time by about 29 percent. This closes representation
packing as the next step.

## Mechanism

0143 keeps the 0138 ordered packet and renderer. After the existing exact
butt-line geometry is produced, it recognizes only an exact device-axis-aligned
rectangle:

```text
accepted compact/native line
  -> existing exact four stroke vertices
  -> exact device-axis-aligned rectangle proof
  -> AGG-equivalent 8-bit edge/interior coverage spans
  -> existing CFX_AggRenderer
  -> immediate source-order composite
```

The rectangle has at most three spans per row: partial left edge, constant
interior, and partial right edge. Coverage uses AGG's 8-bit subpixel scale and
aliased threshold. The existing reusable `agg::scanline_u8` owns storage
bounded by device clip width; no page-sized or per-line allocation is added.

Every non-axis-aligned operation stays on the accepted 0138 four-vertex AGG
rasterizer. This is exact primitive dispatch, not page classification.

## Correctness Invariant

Every source paint remains independent and immediately composited in painter
order. The kernel does not merge, union, reorder, defer, or cache pixels.

Eligibility requires all existing 0138 line checks plus exact equality of the
opposite rectangle edges after the accepted transform arithmetic. Quantized
empty rectangles return to the accepted AGG path before pixels. Clip box,
clip mask, backdrop, destination format, color, alpha, antialias mode, and
compositor remain owned by `CFX_AggRenderer`.

Unit coverage compares canonical `DrawPath()` and ordered replay for:

- horizontal and vertical strokes;
- fractional translation and width;
- alpha;
- hard clipping;
- aliased rendering;
- diagonal fail-closed fallback;
- round-cap generic fallback.

## Resource And Thread Policy

- Existing maximum-256 command packet remains the cancellation bound.
- Existing scanline storage is reused and bounded by output clip width.
- No per-line heap allocation, persistent cache, page classifier, threshold,
  lock, worker, JNI/Kotlin path, or UI-thread work is added.
- Canonical PDFium remains the source of truth and fallback.
- RenderProgram format stays version 24 because retained representation is
  unchanged.

## Device Proof

The replay log adds:

```text
revision=r25-13-0143
mode=exact_axis_aligned_scan_kernel
exactAxisAlignedLineDraws
directButtLineDraws
genericStrokeDraws
lineRasterPasses
replayUs
```

Required invariants:

```text
exactAxisAlignedLineDraws <= directButtLineDraws
directButtLineDraws + genericStrokeDraws <= lineBatchCommands
lineRasterPasses == lineBatchCommands
lineBatchFallbacks == 0
```

## Acceptance

Accept 0143 only when all conditions hold:

1. Android PDFium and `pdfium_unittests` compile.
2. Unit pixel comparisons pass for axis, clipped, aliased, alpha, diagonal,
   and generic-cap cases.
3. Canonical-versus-accelerated lossless device captures have zero differing
   pixels for the acceptance corpus.
4. Q16 `exactAxisAlignedLineDraws` coverage is high enough to explain the
   measured gain.
5. Q16 repeated cold median `bitmapRenderMs` improves by at least 20 percent
   from 0138 and total preview does not regress.
6. 11.pdf, EP23, Study Notes, 6Steps, and scanned pages do not regress.
7. Memory, cancellation, first-visible behavior, and tile publication do not
   regress.

The order-of-magnitude kernel gate remains stricter: an isolated exact scan
kernel must eventually demonstrate at least 5x lower primitive raster CPU.
0143 is the first exact geometry class in that kernel architecture. If Q16
coverage is low or bitmap gain is below 20 percent, do not add approximate
angle tolerances. Continue only with another bit-exact primitive class or stop
the dense-kernel direction.
