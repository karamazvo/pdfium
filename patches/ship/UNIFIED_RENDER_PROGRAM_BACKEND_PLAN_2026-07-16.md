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
| `r25-1-0084` | Dense path execution kernel and bounded scratch reuse | Exact ordered pixels |
| `r25-1-0085` | Clip/image/Form/group/transparency completeness | Proven commands only |
| `r25-1-0086` | Proof-gated blend kernels | Exact eligible blends only |

Every revision extends the same data model and execution interface. A revision
must not install a parallel backend to compensate for a missing command type.

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

## 7. Performance Proof

Compare canonical r25 PDFium, r25-0078, r25-1, and MuPDF 1.27.2 on the same
Android device with identical page, clip, matrix, bitmap dimensions, color
mode, and antialiasing settings. Bypass the Android viewer scheduler for native
engine measurements.

Required cold/warm metrics:

```text
programBuildMs indexBuildMs compileMs candidateQueryMs replayMs rasterMs blendMs
totalCommands candidateCommands visitedCommands drawnCommands dispatchCount
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
