# r25-3 Unified Sparse RenderProgram

**Locked:** 2026-07-22 (Asia/Taipei)

## Decision

Canonical PDFium page objects remain the only source of truth. Generation 3
adds one optional, immutable native sidecar to a holder; it does not create a
second page model or select a renderer by page type. Filename, page size,
object count, path count, and Kotlin classification never choose the path.

During normal parser construction, each path object is offered to an exact
lowerer in source painter order:

- Exact lowering succeeds: retain owned geometry and exact render state in a
  native run at the source object's ordinal.
- Exact lowering fails: retain nothing for that object. The ordinal remains an
  implicit canonical PDFium span.
- No exact lowering succeeds in the holder: retain no sidecar and use unchanged
  canonical PDFium rendering.

The executor walks source ordinals once. Native runs and implicit canonical
gaps draw into the same PDFium render device and destination bitmap. Unsupported
semantics are never approximated, reordered, skipped, or replayed later.

## Architecture

```text
CPDF_PageObjectHolder (authoritative, editable objects)
    |
    | parser-time exact lowering; path objects only
    v
optional VeloceRenderProgram sidecar
    native run [first ordinal, count, exact opcode]
    implicit gap = canonical PDFium objects
    owned geometry/state + bounded holder-space indexes
    |
    | one ordered replay, same render device and bitmap
    v
canonical gap -> RenderSingleObject
native run    -> exact AGG/PDFium device operation
```

The sidecar is an acceleration index, not an alternate document. It contains
no raw page-object pointers and is discarded on holder mutation. Its retained
size is capped at 96 MiB; exceeding any structural or memory limit returns the
entire holder to canonical PDFium before custom pixels are produced.

## r25-3-0103: Owned Unchanged-Holder Replay

Patch: `patches/ship/0103-veloce-render-program-owned-unchanged-holder-replay.patch`

0103 established exact owned native replay:

1. Native opcode compilation starts at ordinal zero. There is no page-routing
   threshold.
2. A single mutation-epoch check occurs before program pixels.
3. Native commands own geometry, matrix, graph state, color, clip, and bounds.
4. Optional-content visibility is resolved once per exact visibility run.
5. Live object lookup is restricted to canonical spans, visibility
   representatives, and pre-pixel driver fallback.
6. Exact top-level Darken strokes can use the PDFium device directly. Real
   groups and unsupported transparency remain canonical.
7. Legacy path-display-list execution is disabled before work.

## r25-3-0104: Sparse Exact Native Sidecar

Patch: `patches/ship/0104-veloce-render-program-sparse-exact-sidecar.patch`

0104 removes the remaining normal-holder shadow cost and makes the
representation proportional to useful native work:

1. The holder allocates the builder lazily at the first path while parsing a
   fresh holder. Text/image-only holders do not allocate a builder.
2. Non-path objects only advance the source ordinal. They receive no copied
   opcode, bounds, command block, or retained metadata.
3. A rejected path leaves no retained command. If all paths reject, `Finish()`
   returns null and replay is canonical PDFium.
4. A 12-byte `VeloceNativeRun` represents a consecutive homogeneous sequence.
   Gaps between runs encode canonical spans without one byte per source object.
5. Immutable native-run, clip/visibility-run, path-block, line-index, and
   memory structure is validated once when the sidecar is sealed. Replay
   consumes the sealed result instead of repeating structural work per tile.
6. Consecutive native lines are indexed in holder-space 64-line leaves. A tile
   can skip a nonintersecting leaf without allocating a candidate list.
7. Command blocks are retained only for owned path runs. Canonical fill gaps do
   not create one block each, which keeps Q16-class memory bounded.
8. An unavailable replay matrix flushes pending native work and renders that
   ordinal canonically, preserving painter order and pixels.

This is a general policy. A normal page with no exact path lowering stays on
canonical PDFium. A mixed page has one ordered stream of native runs and
canonical gaps. A CAD page naturally has many exact native runs. No page is
classified into one of those cases.

## Expected Proof

Canonical-only holders must have no compile/replay event. A retained sidecar
must report:

```text
revision=r25-3-0104 event=compile mode=sparse_owned_sidecar
revision=r25-3-0104 event=replay mode=sparse_owned_sidecar
```

For Q16-class pages, validate that `nativeOpaqueLines` remains near the 0103
count while `commandsSkipped` and `leafCulled` increase for small visible
regions. `nativeObjectLookups` must remain tied to visibility representatives,
canonical gaps, and exceptional fallback, not native draw count.

For 11.pdf, exact Darken paths should remain native when the context is proven
safe:

```text
darkenContextDirect=1
darkenDraws~=nativeDarkenPaths
darkenFallbacks~=0
```

Pixel comparison against canonical PDFium remains mandatory for normal pages,
mixed pages, transparency/groups, editing, preview, full-page, and tile renders.
Performance acceptance requires lower end-to-end visible latency and replay
time; counters alone are not evidence of improvement.

## Remaining Cost, In Order

0104 removes dense shadow metadata and avoids retaining anything for
canonical-only holders. It does not lower Q16's canonical fill objects or
eliminate the raster work for millions of visible lines.

1. **r25-3-0105: exact owned fill opcode.** Own simple solid fill geometry,
   fill rule, color, matrix, clip, and visibility under fail-closed predicates.
   Execute it at its source ordinal; never cross a painter-order barrier.
2. **r25-3-0106: ordered AGG operation executor.** Send bounded consecutive
   owned operations to one device executor while preserving operation raster
   and composite order. Reuse AGG state and scratch across stroke/fill
   boundaries without merging PDF objects.
3. **r25-3-0107: holder-space hierarchy over all owned operations.** Extend
   conservative blocks/leaves to exact fills and paths. Query visible tiles
   without candidate allocation; unknown bounds stay visible.

These extend the same sidecar and executor. They do not add a page classifier,
Kotlin preprocessing pass, second bitmap owner, or alternate renderer.

## Invariants

- PDFium page objects are the single source of truth for fidelity and editing.
- One source order, one destination bitmap, one pixel owner at each ordinal.
- Native execution is allowed only when exact lowering succeeds.
- Unknown or unsupported semantics remain canonical at the same ordinal.
- No native-to-canonical whole-holder restart after native pixels.
- Mutation, unsupported context, invalid structure, or budget failure fails
  before program pixels; per-operation device rejection falls back before that
  operation's first pixel.
- Fixed retained-memory ceilings and stack-bounded replay packets.
- Holder-space indexes are built once and queried allocation-free.
- No filename rule, count threshold, page classification, UI-thread rendering,
  worker pool, global render lock, or tile scheduler is introduced.
