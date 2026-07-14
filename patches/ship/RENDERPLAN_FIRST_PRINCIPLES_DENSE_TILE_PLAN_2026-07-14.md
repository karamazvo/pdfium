# RenderPlan Dense-Tile Performance Plan

Locked: 2026-07-14 17:24:53 CST

## 1. Goal and non-negotiable invariants

The goal is a unified native PDFium pipeline that removes work which is
provably redundant while preserving PDF rendering semantics for normal pages,
huge-path pages, and mixed-content pages.

The optimization order is:

1. Correctness and exact painter order.
2. Do not compile, visit, transform, or rasterize commands outside the visible
   clip when this can be proved safely.
3. Cache immutable work only when its identity and lifetime are exact.
4. Make unavoidable visible raster work cheaper without merging commands or
   changing compositing semantics.
5. Keep every cache, packet, index, and scratch allocation bounded.
6. Keep compilation and rendering off the UI thread and avoid global locks in
   the replay loop.

No filename/page heuristic, `classifyAllPages`, warmup render, speculative
full-page compile, geometry merge, paint reordering, or partial result is part
of this design.

## 2. Evidence from the current Q16 trace

The worst first exact tile in the 2026-07-14 trace reports:

| Metric | Value |
|---|---:|
| total | 11,600 ms |
| compile | 7,006 ms |
| replay | 4,594 ms |
| compiled nodes | 2,975,167 |
| replay visited | 2,846,132 |
| replay drawn | 2,839,441 |
| stroke draw calls | 2,174,538 |
| stroke state/matrix flushes | 2,042,632 |
| spatial index query | 0 ms |

The later dense cache-hit tile still draws 2,366,462 nodes and spends 3,534 ms
in replay. Sparse tiles complete in roughly 1-184 ms and skip about 3 million
objects before compile/replay.

Conclusions:

- Holder-space culling is already valuable and must remain the candidate
  selector.
- The worst tile genuinely intersects most path commands. No culling algorithm
  can remove half of work that is actually visible.
- A 50% dense-tile gain, if available, must come primarily from reducing the
  more than two million dispatch/setup boundaries and allocator/state churn.
- Cold compile and cache-hit replay are separate costs and must not be mixed in
  one optimization or one success metric.

## 3. Unified pipeline

```text
live PDFium holder
        |
        v
ordered RenderPlan skeleton (cached, object indices only)
        |
        +--> Passthrough / blend / unsupported barrier
        |        exact PDFium rendering, in painter order
        |
        +--> PathRun segment
                 |
                 v
        holder-space segment/chunk bounds
                 |
          outside visible clip? ---- yes ---> skip whole range
                 |
                no
                 v
        immutable compiled command program
        (exact object order, paint table, graph state, matrix, bbox)
                 |
          bounded holder-space index query
                 |
          conservative candidate IDs in original order
                 |
                 v
        bounded exact AGG command packet
        (same paint/clip; one exact matrix per path)
                 |
                 v
        separate transform + raster + source-over composite per command
                 |
                 v
        the same destination bitmap used by normal PDFium rendering
```

There is one source of truth: the ordered RenderPlan. Culling chooses a subset
inside a PathRun but never reorders across a barrier. The compiled command
program is an immutable acceleration representation, not an alternative page
model. PDFium page objects remain authoritative for fidelity and editing.

## 4. Cost layers and ownership

### 4.1 Candidate selection

- Build bounds and index entries in holder space so they are reusable across
  zoom levels and tiles.
- Skip a segment only when every member contributed a finite conservative
  bound and the complete segment bound is outside the clip.
- Unindexable commands go into an always-replayed overflow set.
- Candidate IDs are consumed in original command order.
- The candidate mask is fixed-size per bounded chunk and allocated only for a
  partial query.

This layer removes provably invisible work. It must not be used to claim a
dense-tile gain when `replayDrawn` is close to `compiledPathRunNodes`.

### 4.2 Immutable compiled program and cache

- Key by live document/holder identity, holder dictionary identity, exact
  object range, render-affecting options, and a generation/lifetime token.
- Store no raw `CPDF_PageObject*` in a process cache. Passthrough objects are
  resolved by object index from the live holder immediately before use.
- Lookup holds a narrow cache mutex only long enough to obtain a shared
  immutable entry. Compile and replay never run while holding that mutex.
- Compile only demanded visible PathRun ranges. There is no page warmup.

The current cache is entry- and node-bounded. The next cache revision must
replace the node proxy with measured byte accounting:

```text
entry bytes = command arrays + path storage + paint/graph-state tables
            + spatial bins/overflow IDs + allocator overhead estimate
```

Use both a hard byte ceiling and an entry ceiling, LRU eviction, and a
single-entry admission rule. An entry larger than the total budget is replayed
for the current request but is not retained. Cache metrics must include bytes,
admissions, rejections, hits, misses, and evictions.

This cache can remove the observed 7-second cold compile from later tiles. It
cannot remove the first demanded compile without forbidden warmup, so first
tile latency also needs range-level compilation and cheaper compilation data
structures.

### 4.3 Exact raster throughput

Revision 0074 changes the bounded stroke packet from:

```text
same paint + same matrix -> packet
matrix change            -> flush
```

to:

```text
same paint/clip -> packet of {path, exact matrix}
paint/clip/fill/blend/passthrough/cancel/capacity -> flush
```

The packet remains bounded to 256 paths and 16,384 path points. AGG applies
the canonical PDFium matrix decomposition for each command, resets logical
rasterizer/path state, rasterizes each path separately, and composites each
path separately in source order. Only renderer setup and device-owned scratch
capacity are reused.

This is not geometry batching. It preserves antialiasing, overlap, alpha,
stroke adjustment, dash/cap/join state, and painter order.

### 4.4 Scheduling

Native work is cancellable at segment/chunk/packet boundaries. The app submits
only current visible exact tiles and retains the prior valid raster until an
exact replacement is ready. Native optimization must not add a second tile
scheduler or publish partial bitmaps.

## 5. Revision plan and proof gates

### r25-0074: per-matrix exact AGG command packets

Implemented in this change.

Success gates:

- `strokeMatrixChangesBatched` is substantial on the slow Q16 tile.
- `strokeDrawCalls` and `strokeStateFlushes` fall materially.
- `replayDrawn` remains comparable for the same clip.
- Normal-page and huge-page pixel comparisons show no changed output.
- Cancellation and peak memory remain bounded by the existing packet caps.

The expected gain is workload-dependent. A greater than 50% replay improvement
is plausible only if matrix-only churn explains most of the 2.04M flushes; the
new telemetry is the proof. No gain is claimed before measurement.

### r25-0075: byte-accounted immutable program cache

- Add exact/upper-bound `EstimatedBytes()` for compiled programs and indices.
- Replace global node-count retention with hard byte and entry budgets.
- Keep compile/replay outside the cache mutex.
- Add byte/admission/eviction telemetry.
- Do not change eligibility or pixels.

Success gate: repeated visible tiles eliminate cold compile while resident
bytes never exceed the configured ceiling.

### r25-0076: visible-demand range compilation

Use the RenderPlan's conservative segment/chunk bounds before command
compilation. Compile only intersecting bounded ranges, and reuse immutable
range entries across adjacent tiles. Unknown bounds remain fail-closed and are
compiled.

Success gate: cold `segmentedCompileMs` falls in proportion to skipped
`PathRun` objects without missing output. This is most valuable when the tile
intersects a small page region; it is not expected to help a truly dense tile
that intersects almost all ranges.

### r25-0077: only after telemetry

If 0074 still leaves large same-effective-state transitions, evaluate a
canonical immutable paint/graph-state key. Equivalent-state proof must include
all stroke-affecting fields. If equivalence cannot be proven, keep the barrier.
Do not add a heuristic normalization.

## 6. Rejection criteria

Reject or revert an optimization if any of these occurs:

- It merges geometry or changes composite count/order.
- It skips an object based on incomplete/empty/non-finite bounds.
- It stores page-object pointers beyond live-holder scope.
- It scans or compiles the whole page before a visible request needs it.
- It adds an unbounded vector, bitmap, cache, or scratch high-water mark.
- It holds a global/cache lock while compiling or rendering.
- It introduces UI-thread parsing, classification, rendering, or waiting.
- It improves only a named PDF or depends on filename/page heuristics.
- It publishes cancelled/partial output or replaces a valid raster with blank.
- It has no metric that distinguishes work removed from work merely moved.

## 7. Required test matrix

1. Pixel equivalence at preview and multiple zooms for ordinary text/image,
   transparency, clipping, forms, annotations, and mixed-content pages.
2. Huge-path performance and visual checks for `11.pdf`, Q16, and
   `error.pdf` pages 2/3, without any file-specific route.
3. Dense and sparse tile traces comparing compile, replay, candidates, drawn
   nodes, dispatches, matrix changes batched, cancellation, and cache bytes.
4. Repeated zoom/scroll cancellation while retaining the last valid raster.
5. Cache pressure across many documents proving deterministic eviction and no
   stale-holder access.

The optimization is accepted only when correctness passes first and measured
cost is removed from the critical visible path rather than shifted elsewhere.
