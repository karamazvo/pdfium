# r25-1 Unified RenderProgram Backend Plan

Locked: 2026-07-16 CST

## 1. Baseline And Series Identity

The unified native backend starts with revision `r25-1-0079` from this exact
stack:

```text
r25 stable rendering patches
+ 0051 page dimensions without page parse
+ 0075 holder-owned RenderProgram lifetime
+ 0076 parser-time exact command order
+ r25-1-0079 and later unified-backend patches
```

The workflow excludes RenderPlan v1 patches `0053..0074` and legacy
path-display-list consumers `0077..0078`. Those patches remain immutable A/B
references and are never renamed or deleted.

Post-baseline correction: the 0079/0080 workflows still included the older
`0013..0031` path-display-list executor. Device evidence on a mixed normal page
showed that executor consuming Form holders and returning success while the
published page was incomplete. Therefore 0079/0080 are interface experiments,
not canonical correctness baselines. Revision 0081 disables that executor at
its central native boundary before work; canonical PDFium becomes the sole
pixel owner until the unified executor proves an eligible complete result.

## 2. One Framework

```text
PDF content interpretation
          |
          v
live PDFium holder/page objects  <---- editing and fidelity source of truth
          |
          v
immutable ordered RenderProgram
  command kind + conservative bounds + exact state identity
          |
          v
bounded holder-space candidate index
          |
          v
visible candidates in original painter order
          |
          v
bounded exact execution packets
          |
          v
the current PDFium destination bitmap/device
```

This is not a huge-path fast path. Normal, mixed-content, image, text, Form,
transparency, and path-heavy holders use the same program contract. Native
structural cost and exact semantic support decide whether a command is executed
by the unified backend or the complete holder falls back before drawing.

There is no Kotlin page classification, filename/page rule, warmup render,
speculative full-page compile, native tile scheduler, or second destination
bitmap architecture.

## 3. Single Sources Of Truth

- PDFium page objects own document fidelity, editing, and resource lifetime.
- RenderProgram owns immutable rendering order and command identity.
- Holder-space bounds/index own candidate selection.
- The app owns viewport and tile scheduling.
- PDFium's current render device owns destination pixels.
- One byte-accounted native cache owns reusable compiled program/range data.

Do not add another display list, page classifier, spatial index, render queue,
continuity state machine, or cache for the same responsibility.

## 4. Correctness Contract

Before the first accelerated pixel:

1. The program is sealed and belongs to the live holder.
2. Program and holder command counts match exactly.
3. Every command needed by the requested execution range has an exact supported
   semantic representation.
4. Clip, group, mask, blend, and passthrough nesting is balanced and ordered.
5. Candidate removal is based only on finite conservative bounds.
6. Overflow/unknown-bound commands remain always-replayed.
7. Surviving commands retain original painter order.

If any proof fails, return to canonical PDFium before drawing. Cached data never
stores a raw page-object pointer beyond holder lifetime.

## 5. Cost Contract

- Record order and cheap bounds during work PDFium already performs.
- Do not rescan the complete holder to reconstruct command order.
- Build expensive compiled data only for demanded visible ranges.
- Sparse clips query candidates without scanning every page command.
- Dense clips use compact sequential state-delta replay and reusable bounded
  raster scratch.
- Cancellation is checked between bounded commands/packets; cancelled or
  partial bitmaps are never published.
- Cache lookup holds a lock only long enough to obtain immutable ownership.
  Compilation and replay occur outside the lock.
- Program, index, cache, packet, and scratch bytes have hard admission limits.

## 6. Revision Sequence

| Revision | Unified extension | Pixel behavior |
| --- | --- | --- |
| `r25-1-0079` | Execution interface and benchmark contract; unified backend disabled | Patch neutral; artifact still exposes old executor |
| `r25-1-0080` | Compact command summary and live-object state identity | Unified backend disabled; old executor still exposed |
| `r25-1-0081` | Disable legacy holder executor before work | Canonical PDFium is sole pixel owner |
| `r25-1-0082` | Conservative bounded holder-space candidate index | Unchanged until consumed |
| `r25-1-0083` | Exact path/text vertical executor | Proven subset only |
| `r25-1-0084` | Not emitted; revision remains unused | No artifact |
| `r25-1-0085` | Fail-closed compiled direct-path dispatch into PDFium's existing path renderer | Proven simple paths only |
| `r25-1-0086` | Allocation-free ordered candidate cursor, fixed replay chunks, and exact dense linear mode | Exact ordered pixels |
| `r25-1-0087` | Fixed exact consecutive stroke-state packets with reusable AGG scratch | Proven stroke state only; separate per-path pixels |
| `r25-1-0088` | Clip/image/Form/group/transparency completeness | Proven commands only |
| `r25-1-0089` | Proof-gated blend kernels | Exact eligible blends only |

Every revision extends the same data model and execution interface. A revision
must not install a parallel backend to compensate for a missing command type.

0085 changes the compact command from one byte of kind to two bytes of
`kind + exact flags`, retaining the 32 MiB program ceiling. Its first flag is
compiled only for an unclipped path with simple non-pattern paint, normal
blend, no soft mask, and no transfer function. The same predicate is rerun on
the live object before any accelerated pixel is written. Eligible commands
skip generic recursion, transparency, and object-type dispatch but still use
`CPDF_RenderStatus::ProcessPath()` and the existing `CFX_RenderDevice`; there
is no copied geometry or second rasterizer. Text and every unsupported path
remain ordered canonical commands. In 0085 candidate query output is capped at
one million logical indices and allocator capacity is metered. 0086 removes
that temporary dense-query cliff without changing the command or pixel owner.

### 6.1 Locked 0082 Index Contract

`0082` implements candidate selection metadata without enabling execution:

- holders below 4096 commands retain no index and perform no bounds work;
- admission scans only the first 4096 live objects once, never the complete
  huge holder after parsing;
- later commands enter the index during the existing parser append operation;
- one fixed 32x32 holder-space grid stores ordered command indices only;
- only finite, non-empty, holder-contained path bounds enter spatial bins;
- every non-path or uncertain command enters the always-replay stream;
- postings are capped at 4,194,304 and always-replay entries at 1,048,576;
- a command touching more than 64 bins becomes always-replayed;
- queries reuse caller-owned bounded candidate storage, then sort and
  deduplicate into strictly increasing command order without auxiliary
  containers; live holder bounds must exactly match the index snapshot, and
  invalid or changed bounds return `kUseFullReplay`;
- no render call site consumes the index until 0083 validates the complete
  requested command stream before drawing.

The index is acceleration metadata, not a second display list: command order
and fidelity remain owned by RenderProgram and live PDFium objects.

### 6.2 Locked 0083 Executor Contract

`0083` is the first runtime consumer and deliberately accelerates only the
subset for which candidate omission and execution are already exact:

- the holder must be fully parsed and retain the immutable program/index;
- holder, program, and index command counts must match exactly;
- the complete program may contain only path and text commands;
- text commands stay in the always-replay stream and are never spatially
  omitted in this revision;
- path candidates come only from 0082's finite conservative holder-space
  bounds and are restored to strict painter order by the one index query;
- query output is capped at 262144 commands, bounding candidate storage to
  approximately 1 MiB; denser clips use canonical progressive PDFium;
- every candidate resolves to the live holder object and matches its immutable
  command kind before the first destination write;
- replay performs the same active/exact-bounds rejection as canonical PDFium,
  then calls `CPDF_RenderStatus::RenderSingleObject()` so clip, path, text,
  transparency, blend, color, and device semantics remain PDFium-owned;
- synchronous holder rendering and progressive root rendering call the same
  executor boundary; there is no native tile scheduler or second bitmap;
- cancellation is checked between candidate commands and partial output is
  reported only as `kCancelled`, which the caller must discard;
- owned object dirty, matrix, active, or bounds mutation invalidates the
  program and index in O(1), so edited geometry cannot be culled by stale
  parse-time bounds and rendering does not need a full-holder edit scan;
- missing, unsupported, stale, stop-object, invalid-query, or dense requests
  return before drawing and continue through canonical PDFium.

`0083` does not merge paths, compile geometry, replace AGG, cache raster data,
or claim a dense-preview speedup. Its expected win is removal of the O(n)
whole-holder walk for sparse visible tiles while preserving canonical object
execution exactly. Dense direct execution begins in 0085; bounded chunking and
scratch reuse remain the scoped responsibility of 0086.

### 6.3 Locked 0086 Cursor Contract

`0086` changes candidate delivery, not candidate eligibility or rasterization:

- one stack-owned cursor represents the exact ordered union of at most 1025
  immutable streams: 1024 selected grid bins plus the always-replay stream;
- a fixed binary min-heap merges stream heads by command index and suppresses
  duplicates across every 4096-index output chunk;
- the unified executor has no candidate `std::vector`, logical candidate cap,
  cache, allocator, lock, or retained per-render scratch;
- if selected raw postings are greater than or equal to the complete command
  count, the cursor emits the compact command order linearly; this is exact and
  requires no more source reads than the posting merge it replaces;
- both merge and linear modes preserve original painter order, and live object
  bounds still perform final exact visibility rejection before rasterization;
- direct-path eligibility is a compiled hint and is rerun against live state;
  predicate drift renders that object canonically rather than abandoning a
  partially drawn request;
- a structural holder/program mismatch during streaming returns `kCancelled`,
  requiring the caller to discard partial pixels; invalid index/bounds state
  still returns canonical fallback before the first draw;
- cursor and chunk memory are constant with page size and reported through
  `scratchBytes`; candidate count, chunk count, stream count, and dense linear
  mode are exposed as value-only proof metrics.

This removes redundant sort/deduplicate work, repeated vector growth, and the
one-million-candidate fallback/replay duplication. It does not yet reduce the
cost of rasterizing genuinely visible dense paths; that remains the packet
executor scope beginning in 0087.

### 6.4 Locked 0087 Packet Contract

`0087` changes dense direct-stroke execution, not command eligibility or
candidate selection:

- only consecutive live paths already accepted by the 0085 predicate and
  proven stroke-only enter packet buffering;
- exact graph-state object identity, resolved stroke ARGB, and complete fill
  options are packet state; any difference flushes before the next command;
- text, fills, transparent strokes, unsupported paths, cancellation, and
  capacity are hard ordered boundaries;
- one packet holds at most 256 path references and 16384 source points in
  stack-owned arrays; no packet is retained by the holder or a cache;
- each path keeps its own live geometry and matrix and performs a separate AGG
  rasterization and destination composite in painter order;
- AGG path storage, rasterizer, scanline, and immutable renderer setup are
  reused only within the bounded packet;
- non-AGG drivers execute the same ordered per-path `DrawPath()` loop;
- packet failure returns cancellation so partially written output is discarded
  rather than replayed over.

## 7. Performance Proof

Compare canonical r25 PDFium, r25-0078, r25-1, and MuPDF 1.27.2 on the same
Android device with identical page, clip, matrix, bitmap dimensions, color
mode, and antialiasing settings. Bypass the Android viewer scheduler for native
engine measurements.

Required cold/warm metrics:

```text
programBuildMs indexBuildMs compileMs candidateQueryMs replayMs rasterMs blendMs
totalCommands candidateCommands visitedCommands drawnCommands dispatchCount
pathPacketCommands pathPacketDispatches pathPacketStateFlushes
pathPacketCapacityFlushes maxPathPacketCommands
cacheBytes scratchBytes peakRssBytes cancelLatencyMs
```

Acceptance requires:

- accelerated output passes canonical PDFium pixel/correctness tests;
- sparse huge-command replay visits candidates rather than the whole holder;
- dense replay approaches MuPDF median and p95 on the same clip;
- normal-page median and p95 regress no more than 5% from canonical r25;
- cache/scratch hard limits are never exceeded;
- no UI-thread parse, compile, render, lock wait, or bitmap publication work;
- every reported improvement removes measured work rather than moving it to a
  different phase.

No MuPDF-level performance claim is accepted from interactive logs alone.

## 8. Rejection Rules

Reject a change that:

- introduces a file/page-specific route or Kotlin classification;
- calls the 0077/0078 legacy path-display-list consumer;
- creates a second command order, spatial index, scheduler, or cache owner;
- merges/reorders commands without a complete compositing proof;
- skips unknown, empty, non-finite, or incompletely bounded commands;
- performs full-page warmup or duplicate holder scans;
- keeps a global/cache mutex while compiling or rendering;
- allocates without a hard byte/count ceiling;
- publishes cancelled, incomplete, blank, or stale-generation output;
- improves only scheduling while native visited/drawn/raster work is unchanged.
