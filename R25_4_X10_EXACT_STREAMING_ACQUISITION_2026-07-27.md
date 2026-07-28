# r25-4 Exact Streaming Acquisition

**Locked:** 2026-07-27 (Asia/Taipei)
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

### r25-4-0126: Packed Homogeneous Geometry Blocks

Store consecutive exact operations with identical immutable render state in
bounded blocks:

```text
state identity + ordinal range + packed full-precision geometry
```

Each operation remains independently rasterized and composited in painter
order. Blocks may reuse setup and storage but cannot cross canonical, clip,
visibility, transparency, blend, or mutation boundaries.

### r25-4-0127: Bounded Direct Spatial Construction

Replace per-bin growing vectors and the flattening copy with a bounded final
array construction. Prefer conservative block entries where useful and retain
per-operation entries only where required for effective culling. Construction
and final storage share the existing program budget and never coexist as two
unbounded page-sized representations.

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
