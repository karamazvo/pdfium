# r25-12 Direct Compact Line Span Executor

**Locked:** 2026-07-31 (Asia/Taipei)

**First revision:** r25-12-0142

**Extends:** accepted r25-8-0138

**Excludes:** unaccepted r25-9-0139, r25-10-0140, and r25-11-0141

**Status:** implemented, pending CI build and device acceptance

## Measured Boundary

The accepted 0138 Q16 cold preview separated the remaining work:

```text
acquireMs about 3185
bitmapRenderMs about 1465
compactTranslationLines about 2365882
lineRasterPasses about 2590767
```

0138 proved that reusing one AGG renderer per bounded homogeneous packet
improves replay by about 21.7 percent. It still expands every compact 16-byte
translation line into a larger `CFX_OrderedPathDraw`, constructs a full
object-to-device matrix, writes and rereads generic path storage, and then
enters AGG.

The 0141 packed parser covered all 2,940,110 eligible commands but regressed
Q16 acquisition from about 3185 ms to 4045 ms and total preview from about
4651 ms to 6054 ms. High coverage without speedup closes that parser-layout
micro-optimization. 0142 therefore restarts from 0138.

## Why Parse-Time Pixel Fusion Is Not 0142

`FPDF_LoadPage()` completes `CPDF_Page::ParseContent()` before a render device,
target matrix, or device clip exists. Rendering while parsing would require
either:

1. a new load-and-render public lifecycle plus JNI adoption; or
2. an extra page-sized scratch bitmap that is discarded on any later
   eligibility or memory failure.

Neither is a small exact optimization. The first changes API ownership and
the second adds memory and fallback work. This boundary is documented rather
than hidden behind a patch that still performs the same parse and replay.

## 0142 Mechanism

For exact compact translation-origin lines only:

```text
RenderProgram 16-byte lines
  -> bounded 256-entry compact stack packet
  -> CFX_RenderDevice compact-line interface
  -> AGG full-packet fail-closed preflight
  -> bounded precomputed four-vertex stack tape
  -> one independent raster and immediate composite per source command
```

The primary path removes:

- per-line `CFX_OrderedPathDraw` expansion;
- per-line `CFX_Matrix` materialization in `CPDF_RenderStatus`;
- generic `agg::path_storage` write/rewind/read for compact lines;
- repeated stroke transform setup inside the established packet.

The existing ordered packet path remains the fail-closed fallback. Generic
lines, fills, multisegment paths, Darken paths, canonical barriers, clips,
visibility changes, and unsupported devices keep their prior behavior.

## Correctness Invariant

0142 does not merge paint operations.

Every accepted PDF stroke still has:

```text
one source ordinal
one independent rasterizer reset
one raster pass
one immediate source-over composite
```

The complete compact packet validates finite geometry, translations, matrix,
determinant, stroke width, cap, dash state, color, and options before its first
pixel. Any rejection reconstructs the established ordered packet in the same
source order. Clip, visibility, state, canonical barriers, and cancellation
remain outer RenderProgram boundaries.

## Resource Bounds

- Compact packets remain capped at 256 commands.
- Render-status compact storage is 4 KiB on the render-worker stack.
- Driver preflight storage is 8 KiB on the render-worker stack.
- No heap allocation, persistent cache, classifier, threshold, lock, thread,
  JNI/Kotlin work, or UI-thread work is added.
- Existing 3M-line and 96 MiB RenderProgram limits remain authoritative.
- Cancellation remains checked by bounded replay work units; cancelled queued
  packets have not emitted pixels.
- Canonical-only pages never call the compact-line interface.

## Device Proof

The replay log reports:

```text
revision=r25-12-0142
mode=direct_compact_line_span
compactLineSpanCommands
compactLineSpanDispatches
compactLineSpanFallbacks
lineRasterPasses
replayUs
```

Expected Q16 proof:

```text
compactLineSpanCommands is near compactTranslationLines
compactLineSpanFallbacks == 0
lineRasterPasses == lineBatchCommands
commands, omissions, native runs, spatial counters, bytes, and pixels match 0138
```

## Acceptance

Retain 0142 only if repeated cold device medians show:

1. Q16 pixels and all representation/order/fallback counters match 0138.
2. Q16 compact span coverage is high and fallback is zero.
3. Q16 replay and bitmap time improve by at least 10 percent.
4. Q16 acquisition remains within controlled noise.
5. 11.pdf and EP23 remain pixel-correct and within controlled timing noise.
6. Study Notes, 6Steps, and ordinary pages remain canonical when no exact
   compact line sidecar exists and do not regress measurably.
7. First-visible, zoom, cancellation, and memory behavior do not regress.

If replay improves by less than 10 percent, stop representation-boundary
tuning. The remaining Q16 cost is independent scan conversion and
compositing; the next project must be a separately proven exact scan kernel,
not line merging, disjoint coalescing, another parser scanner, or a
document-specific route.
