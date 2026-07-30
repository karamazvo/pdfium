# r25-8 Ordered Stroke Renderer Tape

**Locked:** 2026-07-30 (Asia/Taipei)
**Status:** implemented, pending Android build and device acceptance
**Extends:** r25-7-0137 representation, whose acquisition optimization was not
accepted for performance
**First revision:** r25-8-0138

## Measured Boundary

The 0137 Q16 run preserved the exact 0136 representation but did not improve
cold visible latency:

```text
acquireMs          4,512
bitmapRenderMs     1,872
previewTotalMs     6,387
firstTileMs        6,725
lineBatchCommands  approximately 2.4 million drawable strokes
lineRasterPasses   one per drawable stroke
```

The 0137 compiler change covered 2,940,110 compact commands, but saved only a
few milliseconds of finalization. Acquisition micro-optimization is therefore
closed. This revision targets the independent replay cost.

The ordered AGG driver already keeps one render-local path, rasterizer,
scanline, and transform cache. It still constructs a new `CFX_AggRenderer` for
every source stroke. On dense path pages that repeats clip lookup, retained
bitmap references, color conversion, destination-format selection, and
compositor setup millions of times even though a bounded packet has one exact
stroke state, color, clip, and render-option set.

## Invariant

> Every PDF paint operation remains an independent rasterization and
> source-over composite in source order. Only immutable renderer setup shared
> by an already homogeneous bounded packet may be reused.

Overlapping antialiased strokes must not be merged into one AGG rasterizer.
Independent source-over coverage uses integer rounding and is not generally
equivalent to one unioned coverage mask. The accepted executor therefore keeps:

```text
lineRasterPasses == source stroke draws
```

## r25-8-0138 Mechanism

For each existing maximum-256-entry ordered packet:

1. Validate every operation, matrix, stroke option, state, and color before the
   first packet pixel.
2. Require every stroke entry to exactly equal the packet's shared
   `CFX_FillRenderOptions`; otherwise reject before pixels and retain canonical
   fallback.
3. Construct one stack-owned normal-blend `CFX_AggRenderer` when the packet
   contains a stroke.
4. Reset and rasterize each stroke independently in source order.
5. Submit every resulting scanline through the same immutable renderer.
6. Continue constructing a separate renderer for each ordered fill, since fill
   color and fill rule are per operation.
7. Destroy packet-local renderer state at the existing packet boundary.

The packet remains the existing cancellation and memory bound. No page-sized
scratch, retained raster tape, or persistent renderer cache is added.

## Correctness

- No geometry is merged or reordered.
- Every source draw still performs one AGG raster pass.
- Every source draw still composites immediately before the following source
  operation.
- Clip, backdrop, destination, color, alpha, antialiasing, stroke mode, and
  source-over behavior are identical within the synchronous packet.
- A mismatched stroke option rejects the complete packet before its first
  pixel.
- Canonical fallback remains valid only on pre-pixel rejection.
- Canonical-only pages never create the ordered context or packet renderer.

Unit coverage compares ordered packet pixels with independent canonical
`DrawPath()` pixels, covers direct butt-cap and generic round-cap strokes, and
proves option mismatch rejection leaves the destination unchanged.

## Resource Policy

- No new heap allocation in the per-source loop.
- One stack-owned renderer per maximum-256-entry packet.
- Existing render-local AGG path/rasterizer/scanline scratch is reused.
- Existing 96 MiB compiled-program ceiling is unchanged.
- Existing 256-command cancellation cadence is unchanged.
- No cache, thread, lock, JNI/Kotlin policy, classifier, threshold, or
  UI-thread work.
- RenderProgram format remains version 24.

## Device Proof

The Android replay line adds:

```text
strokeRendererBuilds
```

Expected invariants:

```text
lineRasterPasses == lineBatchCommands
strokeRendererBuilds <= lineBatchDispatches
strokeRendererBuilds << lineBatchCommands
lineBatchFallbacks == 0
```

For Q16, the intended setup reduction is from approximately 2.4 million
renderer constructions toward approximately one construction per 256-entry
packet. This does not claim to remove the unavoidable independent AGG raster
passes.

## Acceptance

Accept 0138 only when repeated same-device medians show:

1. Q16 representation, omission, spatial, native-run, memory, and pixel
   counters match 0137.
2. `lineRasterPasses == lineBatchCommands`.
3. `strokeRendererBuilds` tracks stroke-containing packet dispatches rather
   than source draws.
4. Q16 preview `replayUs` and `bitmapRenderMs` improve by at least 15 percent.
5. Acquisition remains within controlled noise because 0138 does not alter
   parsing or construction.
6. Q16 table lines, 11.pdf blend output, EP23, Study Notes, and 6Steps remain
   pixel-correct.
7. First-visible, zoom, cancellation, and memory behavior do not regress.

If the replay gain is below 15 percent, renderer construction is not the
dominant remaining executor cost. Do not add another setup micro-optimization;
the next design must target exact line scan conversion while preserving
independent source-over semantics.
