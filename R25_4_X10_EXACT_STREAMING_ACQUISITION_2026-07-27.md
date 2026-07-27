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

### r25-4-0125: Exact Direct Numeric Path Scanner

Replace word-buffer and generic `FX_Number`/parameter-stack traffic inside the
path-local parser with an exact span-based numeric scanner. The scanner follows
the same PDF whitespace, comment, delimiter, sign, decimal, and malformed-token
rules. Any token outside the proven path grammar falls back transactionally
before modifying path or graphics state.

The scanner feeds the same path points and operation handlers as canonical
PDFium. It is syntax specialization, not semantic approximation.

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
