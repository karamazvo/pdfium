# r25-4 Exact Streaming Acquisition

**Locked:** 2026-07-27 (Asia/Taipei)
**Updated through:** r25-4-0127 on 2026-07-28 (Asia/Taipei)
**Extends:** r25-3-0123
**First revision:** r25-4-0124

## Objective

Reduce cold acquisition for operation-dense PDF content by removing
representation and dispatch work that is not required by PDF semantics. Preserve
the r25-3 ordered RenderProgram, canonical PDFium editing model, exact AGG
executor, painter order, cancellation, and memory ceilings.

The performance objective is:

- 3-6x lower cold acquisition for dense exact-lowerable streams;
- 10x or better warm acquisition after a later bounded compiled-program cache;
- no measurable regression for canonical-only pages;
- no page identity, object-count, path-count, or Kotlin classification route.

Ten times faster cold acquisition is a target, not an assumption. Every phase
must demonstrate an end-to-end timing reduction while preserving exact command,
memory, replay, and pixel results.

## Non-Negotiable Model

```text
authoritative PDF content stream
              |
              v
one exact ordered interpreter
       |                 |
       | exact lowering  | unsupported / uncertain
       v                 v
immutable RenderProgram  canonical PDFium object
       \                 /
        \ source ordinals
         v
one ordered replay into the same PDFium device and bitmap
```

The RenderProgram remains a derived immutable sidecar. It is not an alternate
document, renderer selection, or page classification. Canonical page objects
remain available through exact one-time materialization from the original
content stream. Editing, enumeration, save, or mutation first canonicalizes the
holder and invalidates the sidecar.

## Correctness And Fidelity Invariants

1. The PDF content stream is authoritative.
2. Every operation has one source ordinal and is interpreted once in order.
3. Exact native lowering commits only after complete semantic validation.
4. Failed, unknown, or unsupported lowering remains canonical at the same
   ordinal.
5. Native and canonical operations draw into one device and bitmap.
6. No native-to-canonical whole-holder restart is allowed after native pixels.
7. Packed storage may share immutable state but does not merge raster or
   composite operations unless pixel equivalence is proven.
8. Coordinates remain full precision; no quantization or approximate bounds.
9. Spatial bounds are conservative: false positives are allowed and false
   negatives are forbidden.
10. Holder mutation or canonical materialization invalidates the sidecar before
    another replay.
11. Cached data owns immutable values and never stores live page-object
    pointers.
12. Any structural, semantic, driver, or memory failure selects canonical
    behavior before the affected operation's first pixel.

## Essential And Removable Work

Cold rendering must decode the content stream, interpret state-changing
operators in order, validate exact lowering, retain replay state, and rasterize
visible contributing operations. It does not require:

- parsing a path paint terminator twice;
- generic operator-map dispatch for already recognized path-local operators;
- temporary per-path object or container allocation for omitted operations;
- repeated immutable graphics-state lowering;
- one state record per homogeneous line;
- dynamically growing per-bin vectors followed by a flattening copy;
- rebuilding a sealed immutable program for every later tile or warm open.

## Phases And Revisions

### r25-4-0124: Single-Pass Path Paint Dispatch

`ParsePathObject()` consumes recognized path paint terminators and invokes the
existing handler directly. It no longer rewinds and asks the outer parser to
tokenize, look up, and dispatch the same operator again.

Supported terminators are the existing PDF path paint/end operators:

```text
S s f F f* B B* b b* n
```

The operation handlers, exact lowerer, canonical fallback, source ordinal,
RenderProgram format, replay, and pixels are unchanged. An unrecognized
operator still rewinds to the last completed path operator and follows the
canonical parser path.

This revision adds no cache, page-sized allocation, classifier, threshold,
thread, lock, bitmap, or JNI/Kotlin work.

Acceptance:

- parser tests cover every recognized terminator and unrecognized fallback;
- Q16 command/opcode/omission/state/clip/visibility/spatial/byte/replay/pixel
  counters remain identical to 0123;
- Q16 `acquireMs` and `compileWindowMs` fall materially;
- 11.pdf, EP23, and canonical-only pages do not regress;
- RenderProgram format remains version 21 and retained memory remains capped at
  96 MiB.

### r25-4-0125: Exact Streaming Line Compiler

0125 supersedes and excludes failed 0124. It applies directly after accepted
0123 and does not invoke a path-paint handler from inside `Handle_MoveTo()`.

After the ordinary exact lowerer establishes one immutable parsed-line context,
a fixed-scratch stream scanner transactionally recognizes a complete balanced
graphics-state unit containing one transformed two-point stroke:

```text
q a b c d e f cm x0 y0 m x1 y1 l S Q
```

On a match, the scanner decodes the matrix and four points directly from the
authoritative stream span. It composes the same `cm` matrix in the same order
as PDFium and emits the line through the existing exact
`RecordVeloceParsedLine()` lowerer. The outer `q` and `Q` are consumed as one
transaction because their net graphics-state effect is exactly zero. The final
CTM bookkeeping is updated to the restored state.

Successful native ownership omits the temporary path object. A lowering
rejection creates the ordinary canonical `CPDF_PathObject` at the same
already-recorded source ordinal. A unit crossing a content-stream boundary is
rejected. On any grammar or boundary miss, the stream position, outer parameter
stack, path state, and graphics state remain unchanged and the existing parser
consumes the unit normally.

The first exact line remains on the existing parser and establishes exact
lowering context. The scanner is then armed only after a valid `Q` restore.
Therefore canonical-only pages and pages that never lower an exact line do not
pay speculative stream scanning. This is exact operation-level reuse, not a
document classifier, count threshold, or approximate parser.

The decoded Q16 stream establishes why this grammar is the Pareto target:
2,944,028 `S` operators exist, 2,940,604 are simple `m l S` lines, and
2,940,119 match the complete balanced `q cm m l S Q` unit. A bare consecutive
`m l S` scanner would match essentially none of that workload.

Acceptance:

- `parserFusedLines` is materially greater than zero on operation-dense exact
  line streams and approaches consecutive eligible line count;
- Q16 command, omission, opcode, state, clip, visibility, spatial, retained
  byte, replay, draw, and pixel results remain identical to 0123;
- Q16 acquisition falls materially relative to controlled 0123 runs;
- a grammar miss leaves parser position unchanged;
- a unit crossing a content-stream boundary leaves parser position unchanged;
- a lowering rejection retains the canonical object at the same ordinal;
- 11.pdf, EP23, and canonical-only pages remain within controlled timing noise;
- RenderProgram format 21 and the 96 MiB retained ceiling remain unchanged.

### r25-4-0126: Exact Packed Translation-Line Compiler

The 0125 device result falsified the assumption that temporary page-object,
path, and per-token dispatcher removal was the dominant remaining Q16 cost.
The complete balanced-unit scanner matched 2,940,112 lines, but acquisition
was 5,683 ms versus 5,536 ms on 0123. The retained representation was
counter-for-counter unchanged.

Decoded-stream analysis then identified a stronger exact invariant:
2,940,119 balanced units use this canonical token grammar:

```text
q 1 0 0 1 tx ty cm 0 0 m dx dy l S Q
```

0126 therefore changes the work, not the routing:

- transactionally require the six constant tokens to be the exact one-byte
  PDF number tokens `1 0 0 1 0 0`;
- convert only `tx`, `ty`, `dx`, and `dy`, reducing ten number conversions to
  four for a proven unit;
- lower the exact zero-origin line to a 16-byte immutable payload containing
  `end`, `translation_x`, and `translation_y`, instead of the generic 24-byte
  start/end/translation payload;
- reconstruct the exact zero start point at replay and use the same state,
  matrix, clip, visibility, spatial index, cancellation, ordered native run,
  and AGG draw batch;
- retain the general 0125 scanner and canonical PDFium path on any token,
  state, transform, budget, or lowering mismatch.

The compact payload is stored in fixed 4,096-entry chunks and shares the
existing total 3 Mi-line and 96 MiB program ceilings. It adds no document
classifier, threshold, page-sized temporary array, mutable cache, thread,
lock, JNI/Kotlin path, bitmap, or UI-thread work. RenderProgram format advances
from 21 to 22 because the immutable opcode and payload schema change.

Acceptance:

- `parserConstantFoldedLines` approaches `parserFusedLines` on exact canonical
  translation-origin streams;
- `compactTranslationLines` owns the non-no-op subset while
  `nativeOpaqueLines` retains only general lines;
- Q16 acquisition falls materially below 0123/0125 and retained bytes fall by
  about eight bytes per compact drawable line, within chunk-capacity effects;
- command count, omission count, exact no-op count, state, clip, visibility,
  source order, spatial candidates, draw count, and pixels remain exact;
- alternate but valid spellings such as `1.0` fail closed to the general
  scanner without advancing parser state;
- 11.pdf, EP23, and canonical-only pages remain within controlled timing
  noise.

Device result:

- Q16 retained commands and exact lowering stayed stable:
  `commands=3,165,420`, `parserFusedLines=2,940,112`,
  `parserConstantFoldedLines=2,940,112`,
  `compactTranslationLines=2,365,882`, and
  `exactNoOpLines=574,229`;
- retained bytes fell from 82,373,915 on 0123 to 63,532,399, a 22.9%
  reduction;
- the spatial representation did not improve:
  `spatialPostings=2,662,901` remained one entry per drawable command;
- Q16 measured `acquireMs=6049`, `replayMs=2246`, and `totalMs=8296`,
  versus 5,536/1,434/6,971 on the controlled 0123 baseline;
- 11.pdf and EP23 also ran slower in this sample.

0126 is therefore accepted as an exact bounded-memory representation
improvement, but not as a latency improvement. It identifies eager
per-command spatial-posting construction and traversal as the next dominant
duplicated work.

### r25-4-0127: Bounded Spatial Command Blocks

Replace the 32x32 grid's per-command ordinal postings, 1,024 growing bin
vectors, and flattening copy with one immutable source-order block table:

- each block covers at most 32 consecutive native source commands;
- each 32-byte entry stores holder-space union bounds, source start/count,
  flags, and an exact candidate bit mask;
- exact no-ops may occupy source ordinals inside a block but never set a
  candidate bit;
- canonical gaps terminate a block, so the spatial table never spans an
  unsupported painter-order barrier;
- unknown or non-finite bounds produce an unbounded block that is always
  selected, preserving fail-open correctness;
- full-page replay returns before allocating or scanning candidate words when
  the device clip contains the holder bounds;
- region replay scans blocks, marks exact native candidates from intersecting
  blocks, and merges them with existing mandatory canonical ranges in source
  order;
- the device clip remains the final exact coverage authority. A block false
  positive can add bounded work but cannot change pixels.

The builder owns at most 128K blocks and final retained storage remains under
the existing 96 MiB RenderProgram ceiling. It introduces no classifier,
threshold, mutable cache, page-sized bitmap, thread, lock, JNI/Kotlin path, or
UI-thread work. RenderProgram format advances from 22 to 23 because the
immutable spatial representation changes.

Acceptance:

- Q16 no longer reports `spatialPostings`; it reports bounded
  `spatialBlocks`, `spatialCoveredCommands`, `spatialUnboundedBlocks`, and
  `spatialIndexBytes`;
- Q16 spatial bytes fall materially below the 0126 value of 10,663,988 bytes,
  with block count near the drawable source span divided by 32;
- full-page replay uses `spatialMode=0` and does not scan the block table;
- a sparse tile tests bounded block metadata and submits only exact candidate
  bits from selected blocks;
- command counts, exact no-ops, native runs, canonical barriers, painter
  order, draws, and pixels remain unchanged;
- unsupported or unknown bounds fail open to rendering, never to omission;
- 11.pdf, EP23, and canonical-only pages remain within controlled timing
  noise.

### r25-4-0128: Bounded Immutable Program Cache

Cache only sealed, resource-independent immutable programs. Cache identity must
include document/content identity, stream generation, resource dependencies,
PDFium revision, and RenderProgram format. Use explicit entry and byte ceilings
with LRU eviction. A miss follows the cold path; a stale or uncertain entry is
discarded before replay.

This phase targets 10x warm acquisition. It does not move parsing, compilation,
or cache I/O onto the UI thread.

## Threading And Memory Contract

- Parsing, construction, replay, and canonical materialization use the existing
  PDFium worker/session ownership.
- No UI-thread rendering or compilation.
- No global RenderProgram lock.
- No parallel interpretation of a stateful content stream.
- Existing cancellation remains at bounded construction/replay boundaries.
- Retained RenderProgram ceiling remains 96 MiB until a revision explicitly
  replaces it with a lower measured bound.
- Parser-local scratch and execution packets remain fixed-capacity.
- Temporary construction memory must be included in acceptance measurements.

## Baseline

The r25-3-0123 device result for Q16 is:

```text
commands=3,165,420
retained commands=225,297
omitted page objects=2,940,123
spatial postings=2,662,901
actual bytes=82,373,915
logical retained bytes=67,496,923
acquireMs=5,536
bitmapMs=1,434
totalMs=6,971
launch-to-first-visible=7,231 ms
```

Acquisition is 79.4% of total rendering time. The r25-4 sequence must reduce
that cost without changing the exact retained representation until the
corresponding representation phase explicitly changes it.

## 0124 Device Result

**Measured:** 2026-07-27 (Asia/Taipei)
**Status:** not accepted; do not start 0125 from this result

The 0124 device run preserved the exact compiled representation for 11.pdf,
Q16, and EP23. Q16 remained counter-for-counter identical to 0123:

```text
commands=3,165,420
retained commands=225,297
omitted page objects=2,940,123
native opaque lines=2,365,894
exact no-op lines=574,229
native runs=320,091
spatial cells=535
spatial postings=2,662,901
actual bytes=82,373,915
logical retained bytes=67,496,923
```

The timing acceptance criterion failed:

| Case | 0123 acquire / bitmap / total | 0124 acquire / bitmap / total | Result |
| --- | --- | --- | --- |
| 11.pdf preview | 109 / 111 / 221 ms | 265 / 350 / 618 ms | 2.80x slower total |
| Q16 preview | 5,536 / 1,434 / 6,971 ms | 16,414 / 4,907 / 21,323 ms | 3.06x slower total |
| EP23 p2 preview | 386 / 107 / 493 ms | 903 / 372 / 1,280 ms | 2.60x slower total |
| EP23 p3 preview | 257 / 98 / 355 ms | 303 / 123 / 426 ms | 1.20x slower total |

The data does not yet prove that direct terminator dispatch caused the entire
regression. The unchanged replay path also slowed by about 3.2x on 11.pdf and
3.4x on Q16 while executing identical work counters. EP23 p1 improved from
115 ms to 79 ms and p3 remained much closer to baseline. That combination is
consistent with device CPU/thermal/runtime variance, not a deterministic
increase in RenderProgram work. Conversely, acquisition also regressed, so
0124 has not demonstrated its intended benefit and cannot be accepted from
representation equivalence alone.

There is also a parser-state boundary that must be corrected or formally
proved before this mechanism continues: direct dispatch currently invokes the
paint handler from inside `Handle_MoveTo()` before the outer parser clears the
two `m` operands. Existing path-paint handlers do not consume those operands,
and the resulting program is identical in this corpus, but the optimized path
must present exactly the same empty parameter state as ordinary outer-parser
dispatch.

Required decision procedure:

1. Treat 0123 as the accepted performance baseline.
2. Do not advance from the current 0124 implementation. Revision 0125 must
   apply directly after 0123 and exclude 0124.
3. Run isolated 0123/0124/0123 previews after process restart under comparable
   device temperature and power state.
4. Compare Q16 acquisition separately from replay. Identical replay work must
   remain within normal run variance; otherwise the run or artifact is not a
   valid parser benchmark.
5. Revision 0125 supersedes 0124 with a transactional complete-operation
   scanner. Keep 0124 only as rejected revision history.

## 0125 Device Result

**Measured:** 2026-07-28 (Asia/Taipei)

**Status:** correctness foundation retained; standalone performance acceptance
failed

0125 proved that Q16 is dominated by the exact balanced transformed-line
grammar:

```text
parserFusedLines=2,940,112
commands=3,165,420
retained commands=225,297
omitted page objects=2,940,123
native opaque lines=2,365,894
exact no-op lines=574,229
native runs=320,091
spatial postings=2,662,901
actual bytes=82,373,915
```

However, Q16 acquisition was 5,683 ms, statistically unchanged from the 0123
baseline of 5,536 ms. The experiment therefore ruled out temporary path-object
construction, path-vector allocation, and outer operator dispatch as the
dominant remaining acquisition cost. The scanner still converted ten numbers,
constructed the same generic six-float line payload, computed exact bounds,
and inserted the same spatial postings for every accepted command.

0126 proved that those constant conversions and retained payload bytes were
real memory costs but not the dominant latency cost. 0127 therefore removes
the remaining eager per-command spatial-posting representation while
preserving the exact ordered command program.

## Proof Policy

No revision is accepted from counters alone. Each build requires:

1. clean application over the complete declared patch stack;
2. targeted unit-test compilation and execution where host support permits;
3. exact normal, mixed, clipping, transparency, editing, 11.pdf, Q16, and EP23
   pixel comparison against canonical PDFium;
4. unchanged command and memory counters unless the revision explicitly owns a
   representation change;
5. lower end-to-end visible latency on device;
6. confirmation that canonical-only pages remain on ordinary PDFium without a
   sidecar.
