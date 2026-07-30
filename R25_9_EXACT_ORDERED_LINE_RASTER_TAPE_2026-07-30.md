# r25-9 Exact Ordered Line Raster Tape

**Locked:** 2026-07-30 (Asia/Taipei)
**Status:** implemented, pending Android build and device acceptance
**Extends:** accepted-in-principle r25-8-0138 replay setup reduction
**First revision:** r25-9-0139

## Measured Boundary

The 0138 Q16 cold preview established:

```text
acquireMs=3185
bitmapRenderMs=1465
lineBatchCommands=2590767
lineRasterPasses=2590767
strokeRendererBuilds=10221
directButtLineDraws=2365894
```

0138 reduced renderer construction by 253.5x and lowered bitmap rendering from
1872 ms to 1465 ms. It did not remove the generic AGG path-storage round trip
performed for each of the 2,365,894 exact butt lines.

## Root Cause

The accepted direct butt-line lowerer already computes the exact four vertices
that PDFium's AGG `conv_stroke` emits. Before 0139, every line then:

1. cleared `agg::path_storage`;
2. wrote four vertices plus `end_poly`;
3. rewound and read those five commands through AGG's generic vertex-source
   loop;
4. transformed each vertex;
5. submitted it to the rasterizer;
6. rasterized and composited the line independently.

Steps 1-3 are a redundant temporary representation. They do not contribute to
PDF semantics, clipping, antialias coverage, or painter order.

## 0139 Mechanism

`RasterizeDirectButtStrokeLineTape()` keeps the existing eligibility proof and
four-vertex arithmetic, then applies the same normalized transform and sends
the same command sequence directly to `rasterizer_scanline_aa::add_vertex()`:

```text
move_to(v0)
line_to(v1)
line_to(v2)
line_to(v3)
end_poly | close
```

The tape is the caller's existing maximum-256-entry ordered packet. No second
command buffer, heap allocation, cache, index, scheduler, or state machine is
introduced.

## Correctness Invariants

For every accepted line:

```text
same endpoint hard clipping
same segment-length and minimum-width math
same four local vertices
same float transform order
same AGG path commands
same rasterizer clipping and fill rule
one source command == one rasterizer reset == one raster pass
one raster pass == one immediate source-over composite
```

Lines are never merged, including disjoint or overlapping lines. Repeated
antialias edge coverage therefore accumulates exactly as canonical PDFium
requires.

Any non-butt cap, dash, degenerate segment, zero-area stroke, path stroke,
non-finite packet matrix, option mismatch, unsupported color/state, or
non-AGG device continues through the existing generic/canonical path.

## Resource Bounds

- Existing packet capacity remains 256 commands.
- Existing cancellation cadence remains bounded by replay work units.
- Existing 96 MiB RenderProgram limit is unchanged.
- RenderProgram format remains version 24.
- No persistent memory is added.
- No JNI, Kotlin, UI-thread, lock, or request-scheduling behavior changes.
- Normal pages remain success-lazy and canonical when exact lowering does not
  activate.

## Device Proof

The replay line moves `replayUs` immediately after `result`, before Android's
log-length truncation, and reports:

```text
lineTapeDraws
genericStrokeDraws
```

Expected Q16 invariants:

```text
lineRasterPasses == lineBatchCommands
lineTapeDraws + genericStrokeDraws <= lineBatchCommands
lineTapeDraws approximately 2.36 million for the cold preview
strokeRendererBuilds == stroke-containing lineBatchDispatches
lineBatchFallbacks == 0
```

## Acceptance

Accept 0139 only when repeated same-device cold medians show:

1. Q16 command, omission, payload, no-op, native-run, spatial, memory, and
   fallback counters match 0138.
2. `lineRasterPasses == lineBatchCommands`.
3. Q16 preview `lineTapeDraws` matches 0138 `directButtLineDraws`.
4. Q16 table lines, overlap coverage, 11.pdf blend output, EP23, Study Notes,
   and 6Steps remain pixel-correct.
5. Q16 preview `replayUs` and `bitmapRenderMs` improve by at least 10 percent
   relative to repeated 0138 medians.
6. Acquisition remains within controlled noise because 0139 does not alter
   parsing or RenderProgram construction.
7. First-visible, zoom, cancellation, memory, and normal-page behavior do not
   regress.

If the replay gain is below 10 percent, the removed path-storage round trip is
not a dominant cost. Do not add another executor setup micro-optimization.
The next performance series must target acquisition or a separately proven
exact scan converter, not line coalescing or heuristic eligibility.
