# r25-3 Unified Sparse RenderProgram

**Locked:** 2026-07-22 (Asia/Taipei)
**Updated through:** r25-3-0123 on 2026-07-27 (Asia/Taipei)

Generation 4 acquisition work continues in
`R25_4_X10_EXACT_STREAMING_ACQUISITION_2026-07-27.md`. It preserves this
sidecar and executor contract while replacing avoidable parser/construction
work.

## Decision

The PDF content stream remains the authoritative source. Canonical PDFium page
objects are the complete editing/fallback representation and are always
available through exact one-time materialization. Generation 3 adds one
optional, immutable native sidecar to a holder; it does not select a renderer
by page type. Filename, page size, object count, path count, and Kotlin
classification never choose the path.

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

### 0104 Device Baseline

The 2026-07-22 device logs establish three different costs:

- MedicineStudyNotes emits no `VeloceRenderProgram2` compile or replay event.
  Canonical preview rendering averages 31 ms and reaches the first tile in
  772 ms. This confirms that canonical-only holders do not retain or replay a
  sidecar.
- Q16 retains 94,664,340 bytes, including 195,219 line leaves and an
  8,540,160-byte range index. The parse/lowering window is 9,973 ms; this
  counter includes page parsing and is not a sidecar-only CPU measurement.
  Full preview replay takes 4,413 ms and issues 167,337 line-batch dispatches.
- Q16 sparse tiles identify roughly 2.94 million culled lines but still visit
  roughly 3.16 million source ordinals and perform roughly 50,000 cancellation
  checks. One poorly culled tile draws 2,744,723 lines and takes 5,872 ms,
  delaying following visible work by more than six seconds.
- 11.pdf retains 3,406,896 bytes, compiles during a 221 ms parse/lowering
  window, reaches the first tile in 709 ms, and renders regions in 12-135 ms.
  Direct Darken execution is active with zero Darken fallback. Its remaining
  delay is request backlog: 360 tile admissions, 18 renders, queue depth 10,
  and queue wait up to 916 ms.

The Q16 sparse-tile counters expose a cursor contract bug. After a canonical
gap, replay reaches the next line leaf before advancing the exhausted leaf
cursor. The first line therefore enters the per-command path and the whole-leaf
fast-forward opportunity is lost. The index identifies invisible work but the
executor still walks it. This must be corrected before adding another index.

### Locked Native Revisions

1. **r25-3-0105: ordered sparse-cursor fast-forward.** Normalize native-run,
   line-leaf, and path-block cursors before dispatching an ordinal. Jump a
   nonintersecting contiguous leaf or block once, advance source and owned-data
   cursors together, and check cancellation on a bounded replay-work cadence
   rather than source-ordinal modulus. Add barrier-dense tests in which every
   line leaf is separated by a canonical object. This changes traversal only;
   it adds no allocation, cache, opcode, or pixel path.

   Acceptance for completed sparse Q16 tiles:

   ```text
   commandsVisited + commandsSkipped == commands
   commandsSkipped >= leafCulled
   commandsVisited ~= canonical ordinals + visible native ordinals
   cancelChecks scales with visited work and jumps, not 3.16M source objects
   ```

   The initial full-page preview is not expected to improve materially because
   all page geometry is visible there.

   **Implementation status (2026-07-22):** implemented in
   `0105-veloce-render-program-ordered-sparse-cursor.patch`, with the
   revision-first `r25-3-0105 ordered sparse cursor` build workflow. The patch
   applies cleanly after 0104 and changes traversal only. Device acceptance is
   pending. In the replay log, `leafRangesSkipped` must become nonzero for
   sparse Q16 tiles, `commandsSkipped` must cover `leafCulled`, and
   `replayWorkUnits`/`cancelChecks` must stop scaling with all 3.16 million
   source ordinals.

2. **r25-3-0106: complete ordered native operation executor.** Add the exact
   owned solid-fill opcode and the bounded mixed stroke/fill AGG executor as one
   change. Lower fill-only paths only when geometry, winding/even-odd rule,
   color, alpha, blend, matrix, clip, and optional-content semantics map exactly
   to the existing PDFium render device. Fill-and-stroke objects and unsupported
   transparency remain canonical at the same ordinal. Reuse owned path/state
   tables; do not introduce a fill-specific page route or scratch bitmap.

   Execute consecutive exact lines, strokes, and fills through one fixed-
   capacity mixed-operation packet. Preserve every operation's rasterization
   and composite order while reusing graph state, clip state, rasterizer
   storage, and device scratch. Cancellation occurs between bounded packets.
   The packet is stack-bounded and cannot cross a canonical ordinal.

   Acceptance for Q16 requires the current 220,968 `rejectPaint` objects to
   become exact native fills when their semantics satisfy the predicates,
   canonical draws to fall accordingly, and canonical/native pixel comparison
   to remain exact across preview and tiles. Full preview must substantially
   reduce the current 167,337 render-status/device dispatches and `replayUs`.
   11.pdf region latency and normal canonical pages must not regress.

   **Implementation status (2026-07-23):** implemented as
   `0106-veloce-render-program-ordered-mixed-fill-executor.patch`. The exact
   fill representation is memory-proportional to semantic diversity rather
   than object count: bitwise-identical geometry is interned once, while each
   source fill owns a 16-byte color/rule/transform instance. Mixed holder-space
   leaves contain consecutive exact lines and fills, so fill lowering removes
   the former leaf-per-barrier index explosion instead of adding a second
   index. The retained sidecar remains capped at 96 MiB.

   Replay submits a fixed 256-entry stack packet to the PDFium render device.
   AGG validates the complete packet before its first pixel, then rasterizes
   and composites every operation independently in source order while reusing
   path storage, rasterizer, scanline, clip, and paint setup. A canonical
   ordinal, clip/visibility/state change, unsupported forced-color behavior,
   unavailable matrix, or driver rejection is a hard ordered boundary and
   falls back before that ordinal's first pixel. No filename, page class,
   scratch bitmap, or Kotlin route is introduced.

   Build proof must show format version 10, `nativeOpaqueFills` near the former
   Q16 `rejectPaint` count, a small `fillGeometries` count, materially fewer
   `leafRanges` and `lineBatchDispatches`, retained bytes below 100,663,296,
   and exact canonical-vs-mixed pixel tests. Device acceptance remains pending.

3. **r25-3-0107: compact spatial ordinal program.** Compact exact line storage
   and add the painter-order spatial ordinal index together, because the index
   cannot fit responsibly while Q16 already retains 94,664,340 bytes. Move
   state, clip, visibility, and matrix-mode identity from each line into exact
   homogeneous line runs. Keep full-precision geometry and translations in one
   24-byte line record; a compact line run selects either the exact interned
   affine matrix or the inline-translation sentinel. Do not quantize drawing
   coordinates. The representation remains selected per operation, not per
   page.

   Build a bounded holder-space hierarchy independent of source locality using
   conservative bounds. A render-local reusable candidate bitmap marks visible
   owned ordinals; replay consumes set bits in source order. Unknown bounds fail
   open. The index cannot reorder operations, cross canonical gaps, or allocate
   per candidate.

   Q16 retained bytes must move materially away from the 96 MiB ceiling before
   the spatial index is admitted. Construction remains single-pass and bounded.
   Acceptance requires candidate and visited work to track visible geometry, a
   greater-than-50% reduction for the recorded 5,872 ms worst region, bounded
   combined sidecar/scratch memory, and exact canonical pixel comparison.

   **Implementation status (2026-07-23):** implemented as
   `0107-veloce-render-program-compact-spatial-ordinal-program.patch`, with the
   revision-first `r25-3-0107 compact spatial ordinal program` workflow.
   Format version 11 removes the former 28-byte per-line state/matrix indexes
   and stores those identities in 12-byte homogeneous line runs. The old
   source-local leaf/coarse index is removed rather than retained beside the
   new structure.

   Exact native line and fill ordinals are indexed once in a fixed 32x32 grid
   over stable holder space. Sparse retained cells own ordered ordinal
   postings. The grid builder is itself lazy until the first exact line or fill,
   so canonical, rejected-path, and Darken-only holders do not pay its temporary
   container cost. Unknown, out-of-holder, and broad bounds enter an
   always-candidate stream; posting overflow or retained-memory pressure drops
   the index and preserves full ordered replay. The retained program remains
   under the existing 96 MiB cap.

   Each render status reuses one command bitset bounded by the holder command
   limit. A tile query marks candidates, then the normal source-order cursor
   jumps only within the current homogeneous native line/fill run. Canonical
   gaps, clip/visibility transitions, native path blocks, painter order, and
   the destination bitmap are unchanged. Full-page clips short-circuit to full
   replay without populating the bitset.

   Device proof must use:

   ```text
   revision=r25-3-0107 event=compile mode=compact_spatial_ordinal_program
   lineRuns=... spatialCells=... spatialPostings=...
   revision=r25-3-0107 event=replay mode=compact_spatial_ordinal_executor
   spatialCandidates=... spatialCulled=... commandsVisited=...
   ```

   Sparse Q16 tiles should show `spatialCulled` near the off-tile native
   line/fill count and `commandsVisited` tracking visible candidates rather
   than all page commands. A full preview may still replay the full page
   because every page-space primitive is potentially visible.

4. **r25-3-0108: persistent AGG ordered context.** The 0107 device run proved
   that sparse Q16 tiles benefit from the ordinal index, while the full preview
   still rasterizes 3,164,996 native operations through 12,464 fixed packets.
   In 0107, each packet reconstructs AGG path storage, rasterizer, and scanline
   scratch even though the destination, driver, and ordered replay are the
   same.

   0108 adds one generic `OrderedPathBatchContext` at the `CFX_RenderDevice`
   boundary. It is created lazily at the first nonempty ordered packet, so
   Darken-only `11.pdf` and canonical-only replay allocate nothing. AGG owns
   the concrete context and initializes its clip box once. Every existing
   256-operation mixed line/fill packet reuses the same logical path storage,
   rasterizer, and scanline capacity. Packet validation still completes before
   the packet's first pixel, and each operation is still rasterized and
   composited independently. The patch does not merge geometry, enlarge
   packets, move cancellation checkpoints, expose Veloce page types to `fxge`,
   or retain scratch beyond one synchronous render.

   Context ownership is explicit. A null context, a context created by another
   driver, or an unsupported driver returns false before changing packet
   pixels. `CPDF_RenderStatus` then replays those same ordinals canonically in
   source order. The existing mixed-order pixel test now draws two packets
   through one context and compares the result with canonical PDFium; it also
   verifies that null and foreign contexts leave the bitmap unchanged.

   Device proof must use:

   ```text
   revision=r25-3-0108 event=replay mode=persistent_agg_ordered_executor
   orderedPathContext=1 lineBatchDispatches=... replayUs=...
   ```

   Q16 must retain the 0107 spatial-culling result while materially reducing
   full-preview and dense-region `replayUs`. `11.pdf` Darken execution is
   deliberately unchanged. Study Notes preview/region p90 must remain within
   10 percent of 0107. Retained RenderProgram bytes are unchanged; AGG scratch
   is bounded by the most complex operation seen in one active render.

5. **r25-3-0109: ordered candidate command backend.** The 0108 device result
   showed that removing packet scratch construction did not move Q16 enough.
   Sparse tiles with very few spatial candidates still accumulated hundreds of
   thousands of `replayWorkUnits`. The cause was structural: replay searched
   each alternating native line/fill run and advanced three sequential payload
   counters even when almost every command was outside the tile.

   0109 makes source ordinals directly executable. Every homogeneous native
   run stores the first payload index for its opcode. An 8-byte lookup record
   per 256 source commands bounds the search to native runs intersecting that
   source block, so jumping to a sparse candidate never crosses intervening
   run boundaries. Consecutive same/next-run commands keep the existing O(1)
   cursor, so dense replay does not pay a block lookup per command. At seal
   time, the program also derives compact mandatory
   ranges containing exactly the commands that a line/fill spatial query may
   not omit: canonical gaps plus owned stroke and Darken commands. Sparse
   replay performs an allocation-free ordered merge of the existing candidate
   bitmap and those mandatory ranges. A selected source ordinal resolves its
   native payload with:

   ```text
   run.first_payload + (source_ordinal - run.first_command)
   ```

   This is one ordered program, not a page classifier or an alternate page
   renderer. Candidate selection may omit only exact native line/fill commands
   whose conservative holder-space bounds do not intersect the tile.
   Canonical objects and non-indexed native commands remain ordered barriers.
   Unknown bounds are already fail-open candidates. Full replay retains the
   existing sequential execution, pixels, clips, state changes, cancellation,
   and canonical fallback behavior.

   Lookup blocks and mandatory ranges share the spatial index lifetime and its
   96 MiB program budget. If the spatial index is unavailable or rejected by
   the budget, both are discarded and full replay is used; canonical-only
   holders still retain no sidecar. The cursor allocates nothing per query and
   scans at most one 64-bit word per 64 source commands.

   Device proof must use:

   ```text
   revision=r25-3-0109 event=compile
   mode=ordinal_addressable_render_program
   nativeRuns=... runLookupBlocks=... mandatoryRuns=... mandatoryCommands=...

   revision=r25-3-0109 event=replay
   mode=ordered_candidate_command_backend
   spatialCandidates=... mandatoryCommands=... cursorWords=...
   replayWorkUnits=... replayUs=...
   ```

   On a sparse Q16 tile, `replayWorkUnits` must approach
   `spatialCandidates + mandatoryCommands`; it must no longer track the number
   of alternating native runs. `cursorWords` is bounded by
   `ceil(command_count / 64)`. Dense/full tiles still pay their real
   rasterization cost, so 0109 does not claim to solve dense Q16 or full-page
   preview throughput. `11.pdf` Darken execution and ordinary full replay are
   intentionally unchanged.

6. **r25-3-0110: single-pass dense RenderProgram.** The 0109 device result
   proves that sparse command selection is no longer the dominant Q16 cost.
   A representative sparse replay selected 140 spatial candidates plus 4,329
   mandatory commands and completed native replay in 4,183 microseconds,
   compared with 320,873 replay work units and 12,269 microseconds in 0108.
   The remaining large costs are the 9,699 ms parse/lowering window and dense
   rasterization: a full replay still executes about 3.2 million operations.

   Source review corrected an earlier premise before implementation. The
   RenderProgram was already offered each page object synchronously from
   `CPDF_PageObjectHolder::AppendPageObject()`. There was no second
   3.2-million-object post-parse compilation scan to remove. The avoidable
   construction cost was the builder recomputing retained sizes and vector
   capacities across all program containers for every accepted object.

   0110 keeps that existing single parser pass and replaces the repeated full
   accounting with exact incremental logical-byte reservation. Each accepted
   command reserves only the bytes it is about to append, including a new
   command block when required. Sealing still measures actual retained vector
   capacities and rejects any program above the unchanged 96 MiB ceiling.
   This removes redundant construction work without adding a parser,
   classifier, cache, allocation owner, or lifecycle.

   Dense replay now removes a second repeated cost. Within an existing
   same-state, same-clip, fixed 256-operation packet, AGG may accumulate
   consecutive simple lines into one rasterizer scan only when conservatively
   expanded device-pixel bounds occupy disjoint cells in a fixed 32x32 stack
   grid. Disjoint destination pixels make source-order compositing equivalent.
   A fill, owned path, overlapping cell, uncertain or non-finite bound, state
   boundary, clip/visibility boundary, canonical ordinal, cancellation
   checkpoint, or packet boundary flushes first and retains independent
   ordered rasterization. No geometry is merged across a semantic barrier.

   Inline translation matrices use a direct composition with the holder
   matrix in the same floating-point operation order as the generic matrix
   multiply. The optimization adds no page-sized memory; its occupancy grid is
   fixed at 128 bytes per synchronous AGG batch call.

   Device proof must use:

   ```text
   revision=r25-3-0110 event=compile
   mode=single_pass_dense_render_program
   logicalRetainedBytes=... incrementalBudgetChecks=... compileWindowMs=...

   revision=r25-3-0110 event=replay
   mode=single_pass_dense_command_backend
   lineBatchCommands=... denseRasterPasses=...
   denseDisjointLineDraws=... replayUs=...
   ```

   `logicalRetainedBytes` and final retained bytes must stay below the same
   cap. Dense Q16 work should show `denseRasterPasses < lineBatchCommands` and
   nonzero `denseDisjointLineDraws`; otherwise the document geometry does not
   expose this exact optimization and no speedup should be claimed. Acceptance
   requires a material reduction in Q16 parse/lowering or dense replay time,
   exact canonical-vs-batched pixel tests, and no regression for 11.pdf or
   canonical Study Notes pages.

7. **r25-3-0111: exact no-op and invariant stroke executor.** Device results
   rejected 0110's dense raster-coalescing premise. Q16 lowered 3,164,996
   line/fill commands, but the 32x32 occupancy grid removed only 2,929 raster
   passes (0.09 percent). Full-preview replay increased from 3,203,543
   microseconds in 0109 to 4,643,138 microseconds in 0110. Inspecting bounds
   and grid cells for every line was therefore more expensive than the tiny
   amount of AGG work it avoided.

   0111 deletes that grid rather than compensating for it. Every command with
   non-empty coverage again receives one independent raster/composite pass in
   exact source order. The retained improvement is based on PDF stroke
   semantics instead of geometric proximity: a two-point open stroke with
   identical endpoints and a butt cap covers no pixels. The builder keeps the
   owned line ordinal but emits no holder-space spatial posting. Full replay
   recognizes the same exact predicate from owned geometry and immutable
   state, then advances before optional-content lookup, clip installation,
   matrix composition, packet construction, or AGG. Round and square caps are
   deliberately not elided. There is no filename, command-count threshold,
   tolerance, overlap estimate, or device-space occupancy policy.

   The remaining independent strokes frequently share an identical affine
   linear part while only translation changes. The existing render-local AGG
   context now caches the normalized transform and its inverse until any of
   `a`, `b`, `c`, or `d` changes. Per-command translation and matrix
   multiplication remain in the original operation order, and rasterization
   still receives the per-command derived scale. This removes repeated matrix
   inversion without retained page memory, a global cache, a lock, or
   cross-thread state.

   Static analysis of Q16's decompressed content found 574,229 exact
   identical-endpoint strokes among roughly 2.94 million two-point strokes.
   This predicts removal of about 18 percent of total Q16 replay commands from
   the raster hot path. It is a corpus estimate, not an eligibility rule.

   Device proof must use:

   ```text
   revision=r25-3-0111 event=compile
   mode=exact_noop_invariant_transform_program
   exactNoOpLines=... spatialPostings=... compileWindowMs=...

   revision=r25-3-0111 event=replay
   mode=exact_noop_invariant_transform_backend
   lineBatchCommands=... lineRasterPasses=...
   exactNoOpLinesSkipped=... strokeTransformBuilds=...
   strokeTransformHits=... replayUs=...
   ```

   For a full Q16 replay, `exactNoOpLinesSkipped` should be close to the
   compile-time `exactNoOpLines`, `lineRasterPasses` must equal
   `lineBatchCommands`, and `strokeTransformHits` should dominate
   `strokeTransformBuilds`. Sparse replay may omit no-op ordinals through the
   spatial index, so its runtime skip count can be lower. Acceptance requires
   lower full-preview and dense-tile `replayUs` than 0110, exact unit-pixel
   equivalence, and no measurable regression for 11.pdf or Study Notes.

   Device results confirmed the semantic skip but not an end-to-end Q16 win.
   It skipped 574,229 exact no-op lines and reduced full replay from 4.64
   seconds in 0110 to 3.14 seconds. Compilation nevertheless measured 13.74
   seconds and first visible measured 18.63 seconds. Dense tiles still issued
   2.15 to 2.42 million independent line raster passes and took 3.30 to 4.15
   seconds. Sparse culling was already effective; the remaining dense work
   genuinely intersected those tiles.

8. **r25-3-0112: direct ordered opaque-line raster kernel.** The measured
   invariant is that dispatch batching is no longer the limiting operation:
   `lineRasterPasses` still equals the number of surviving source operations.
   For an exact non-degenerate, solid, butt-cap line, AGG's generic
   `conv_stroke` emits exactly four polygon vertices. 0112 computes those same
   vertices directly using the same normalized transform, minimum device
   width, hard clipping, half-width, and source order. It then sends the
   polygon to the existing AGG antialias rasterizer and PDFium compositor.

   This is not cross-object batching. Every line retains its own raster and
   source-over composite, so overlapping antialias coverage remains unchanged.
   The mechanism removes only repeated two-point path and generic stroker
   construction. Round and square caps, dashes, zero-area mode, transformed
   degeneracy, complex paths, fills, and invalid state use the existing
   generic or canonical path. No page classification, threshold, persistent
   bitmap, global cache, lock, or UI-thread work is introduced.

   Device proof must use:

   ```text
   revision=r25-3-0112 event=compile
   mode=direct_ordered_opaque_line_program

   revision=r25-3-0112 event=replay
   mode=direct_ordered_opaque_line_backend
   directButtLineDraws=... genericStrokeDraws=...
   lineRasterPasses=... replayUs=...
   ```

   Q16 should show `directButtLineDraws` close to its surviving opaque-line
   count and materially lower dense-tile `replayUs`. `lineRasterPasses` remains
   equal to source operations by design. Acceptance requires exact unit-pixel
   equivalence, no fallback increase, and no measurable regression for
   11.pdf or Study Notes.

9. **r25-3-0113: build correction.** The 0112 Android library compiled, but
   `pdfium_unittests` did not because its new fallback test called the
   `CFX_GraphState` wrapper method `SetLineCap()` on a
   `CFX_GraphStateData` value. 0113 uses the data object's
   `set_line_cap()` method and advances Android revision markers. It changes
   no runtime geometry, eligibility, rasterization, ordering, allocation, or
   pixel behavior.

10. **r25-3-0114: shared exact Form programs.** Fully native Forms that
    perform no resource lookup publish their immutable RenderProgram to a
    document-owned cache. A later Form invocation may reuse it only when the
    stream generation, Form matrix and BBox, transparency semantics, parent
    matrix, and every inherited path-state value consumed by exact lowering
    match bit-for-bit. Inherited clips, partial native coverage, resource
    dependence, stream mutation, global page-object mutation, and live
    command-count drift all reject reuse.

    The live `CPDF_Form` still parses and owns canonical page objects, so
    editing, visibility, unsupported rendering, and fallback retain PDFium's
    normal source of truth. A cache hit removes only duplicate native geometry
    copies, state/geometry interning, hashing, spatial-index construction, and
    sidecar allocation. Cache-owned retention is bounded to 16 entries and
    96 MiB per document, with no cross-document state or new lock.

    Error.pdf pages 2 and 3 reference the same large vector Form. Expected
    device proof is one `event=form_cache result=store` followed by
    `event=form_cache result=hit` for the same `streamObj`, while canonical
    pixel comparisons remain unchanged. The first occurrence still pays one
    parse/lowering pass; eliminating that cold parse belongs to the later
    parser-owned compact-tape phase.

11. **r25-3-0115: exact path-only clip interning.** EP23's shared Form
    contains 28,071 native fills and emits the same rectangular `W* n` clip
    before every fill. PDFium's `CPDF_ClipPath::operator==` compares
    copy-on-write storage identity, so pointer-distinct but semantically
    identical clips previously created 28,071 clip runs. Replay consequently
    reinstalled and rasterized the same clip 28,071 times and flushed the
    fixed ordered packet after every fill.

    The builder now keeps pointer identity as its O(1) fast path and otherwise
    compares path-only clip state exactly: path count, fill rule, point type,
    close flag, and every coordinate float bit. A match reuses the current
    retained run representative. No clip geometry is normalized, rounded, or
    reduced to a rectangle. Text-containing clips, malformed path references,
    and any exact structural difference remain separate, preserving PDFium's
    existing clip transitions and fail-closed behavior.

    The comparison allocates no memory and adds no global table, page
    classifier, lock, or UI-thread work. It reduces retained clip snapshots
    and lets the existing bounded 256-command executor packet span equivalent
    clips while each fill still rasterizes and composites independently at its
    original source ordinal.

    Device proof must use:

    ```text
    revision=r25-3-0115 event=compile mode=exact_clip_interned_program
    nativeOpaqueFills=28071 clipRuns=1
    exactClipChecks=28070 exactClipMatches=28070

    revision=r25-3-0115 event=replay mode=exact_clip_interned_backend
    nativeFills=28071 clipRuns=1
    lineBatchCommands=28071 lineBatchDispatches=~110 maxLineBatch=256
    ```

    Acceptance requires unchanged EP23 pixels, materially lower preview and
    region `replayUs`, no new canonical fallbacks, and no measurable Q16,
    11.pdf, or Study Notes regression. This revision does not claim to solve
    Q16's surviving dense stroke raster cost.

12. **r25-3-0116: adopt parsed path point storage.** The Q16 `compileWindowMs`
    measurement includes PDFium's canonical content parsing and page-object
    construction; it is not a second RenderProgram traversal. Inspection of
    that single pass found a general duplicate operation: the content parser
    already owns the final `std::vector<CFX_Path::Point>`, but
    `AddPathObject()` reconstructs a new `CPDF_Path` one point at a time.
    Each append creates temporary path storage and grows/copies destination
    storage after the exact vector has already been allocated and populated.
    Q16 repeats that work for roughly three million two-point paths.

    `CPDF_Path::SetPoints()` now moves the completed vector into the normal
    copy-on-write path object. It does not merge objects, defer objects, or
    create a second geometry model. Coordinates, point types, close flags,
    object boundaries, graphics state, source ordinals, clip behavior,
    mutation, editing, and canonical fallback remain unchanged. A path used
    for both painting and clipping still follows the existing copy-on-write
    transform boundary.

    The mechanism applies to every parsed path and adds no classifier,
    threshold, cache, retained page memory, synchronization, or UI-thread
    work. The existing RenderProgram format remains version 16 because its
    immutable representation is unchanged.

    Device proof must use:

    ```text
    revision=r25-3-0116 event=compile
    mode=exact_clip_interned_program pathStorage=adopted
    ```

    Q16 acceptance requires the same `commands=3165420`, native opcode
    counts, exact-no-op count, spatial postings, replay draw counts, and
    pixels as 0115, with materially lower `compileWindowMs`, preview
    `acquireMs`, and launch-to-first-tile time. The 0115 baseline is
    `compileWindowMs=11818`, `acquireMs=12891`, and first tile at 15258 ms.
    Replay is intentionally unchanged; its 2.07-second full-preview cost is
    the lower bound after parser construction is removed.

    Device results confirmed that the mechanism is valuable where path
    geometry is nontrivial: 11.pdf compile fell from 277 ms to 162 ms and
    preview render from 487 ms to 352 ms. EP23's shared Form compile fell from
    1159 ms to 291 ms, page-2 acquire from 1286 ms to 443 ms, and page-2 total
    from 1499 ms to 580 ms. Q16 stayed near 12.2 seconds because its dominant
    cost is creating and retaining roughly 2.94 million canonical
    `CPDF_PathObject` instances, not copying their two points.

13. **r25-3-0117: parser-owned compact ordered line tape.** Exact lowering,
    not a page classifier, now decides whether a root-page opaque two-point
    line needs a retained canonical object. The parser builds the same exact
    native command first. Only after that succeeds with finite known bounds
    may the holder omit the canonical line object. Rejected paths, complex
    strokes, fills, Darken, text, images, Forms, shadings, and unsupported
    state remain ordinary PDFium objects.

    Source order remains the single command order. Before the first omission,
    retained object indexes are identical to source ordinals and no map is
    allocated. At the first omission, the builder creates one sorted
    source-ordinal list for retained canonical barriers; replay resolves only
    those barriers through binary search. The map and all native data share
    the existing 96 MiB program budget.

    Optional-content visibility no longer needs a live page-object
    representative. Each visibility run owns an immutable copy-on-write
    `CPDF_ContentMarks` scope and calls PDFium's existing `CPDF_OCContext`
    logic directly. Conservative scope, mark, and string storage estimates
    are charged to the same 96 MiB program budget as geometry and ordinal
    tables. Clip runs, graphics state, colors, geometry, source ordinals, and
    painter order remain exact.

    Compact replay performs all context checks before its first pixel: the
    holder mutation epoch, AGG ordered-path capability, render mode, device
    transform, aggregate line bounds, path matrices, and translated stroke
    alpha. The legacy path display list rejects compact holders before
    iteration. If construction exceeds budget, validation fails, a render
    context is unsupported, or an API requests page objects for editing or
    enumeration, the page reparses its original content once with compact
    recording disabled and exposes the complete canonical holder.

    This changes object ownership, not PDF semantics or scheduling. There is
    no filename rule, object-count threshold, Kotlin classification, second
    bitmap, global cache, lock, or UI-thread operation.

    Device proof must use:

    ```text
    revision=r25-3-0117 event=compile
    mode=parser_compact_ordered_program
    omittedPageObjects=... retainedPageObjects=...

    revision=r25-3-0117 event=replay
    mode=parser_compact_ordered_backend
    omittedPageObjects=... retainedPageObjects=...
    ```

    Q16 should retain the same `commands=3165420`, native opcode counts,
    visibility-run count, spatial postings, exact-no-op count, replay draw
    counts, and pixels while omitting most of its approximately 2.94 million
    exact native lines. The retained count should be dominated by fills,
    complex paths, text, and other canonical barriers. 11.pdf's Darken paths
    and EP23's fills are intentionally retained, so their 0116 performance and
    canonical ownership remain unchanged. Acceptance requires materially
    lower Q16 `compileWindowMs`, `acquireMs`, launch-to-first-tile time, and
    peak holder memory, plus exact normal-page, 11.pdf, Q16, and EP23 pixels.

    Device results show the ownership change worked but did not remove most
    parser work. Q16 omitted 2,940,123 of 3,165,420 source objects and reduced
    `compileWindowMs` from 12,162 to 9,888 ms, `acquireMs` from 13,267 to
    10,955 ms, and preview total from 15,089 to 12,986 ms. Dense tile replay
    still selects about 2.2 million genuinely intersecting commands, so that
    cost is separate from parser construction. 11.pdf and EP23 omitted no
    objects, as designed; their one-run timing variation requires standalone
    cold-process confirmation rather than a new routing rule.

14. **r25-3-0118: direct parser line emission.** The parser no longer creates
    a temporary `CPDF_PathObject`, copies all graphics-state snapshots into
    it, calculates its bounds, and immediately decodes it back into the same
    owned native line. For an already identified normal, stroke-only,
    two-point path, it passes the two endpoints, matrix, and PDFium's immutable
    copy-on-write graphics state and content marks directly to the shared
    native-line lowerer.

    The direct and page-object entries use the same exact eligibility, state
    and matrix interning, clip and visibility retention, 96 MiB budget,
    spatial insertion, source ordinal, and native-run append rules. A shared
    two-point stroke-bounds primitive is also used by ordinary
    `CFX_Path::GetBoundingBoxForStrokePath()`, so direct and canonical
    construction cannot drift. Darken bypasses direct emission and preserves
    its existing owned-path executor. Any unsupported state, unknown bound,
    budget failure, or non-line shape creates and retains the ordinary PDFium
    object at the same ordinal.

    Successfully omitted, non-clipping lines do not allocate a `CPDF_Path` or
    `CPDF_PathObject`. Lines that also update the clip still materialize the
    path once because PDF clip state requires it. No parser-state generation,
    classifier, threshold, cache, lock, second pass, or Kotlin work is added.

    Device proof must use:

    ```text
    revision=r25-3-0118 event=compile
    mode=parser_direct_line_program
    parserDirectLineAttempts=...

    revision=r25-3-0118 event=replay
    mode=parser_direct_line_backend
    ```

    Q16 acceptance requires `parserDirectLineAttempts` near its exact opaque
    line count, unchanged command/opcode/omission/spatial/replay counters and
    pixels versus 0117, and materially lower `compileWindowMs`, `acquireMs`,
    preview total, and launch-to-first-tile time. Replay itself is intentionally
    unchanged. 11.pdf, EP23, and normal pages should have zero or few direct
    attempts unless they contain the same universally eligible line commands.

15. **r25-3-0119: payload-free exact no-op ranges.** Revision 0111 proved
    that a two-point stroke with identical endpoints and a butt cap paints no
    pixels, but 0118 still retained each such command as a 24-byte native line
    plus line-run, native-run, state, matrix, clip, visibility, and replay
    bookkeeping. Q16 contains 574,229 of these commands, so the representation
    was paying construction and retained-memory cost for operations already
    known to have no raster effect.

    Both parser-direct and materialized-object lowering already converge on
    `TryAppendNativeOpaqueLine()`. After that shared path has validated finite
    geometry and matrix, normal opaque paint, exact graph state, and bounded
    content marks, an identical-endpoint butt-cap line now emits only a source
    ordinal into `VeloceExactNoOpRange`. Consecutive ordinals coalesce into one
    8-byte range. No geometry, state, matrix, clip, visibility, bounds, spatial
    posting, native run, or raster payload is retained for those ordinals.
    Round and square caps remain drawable and follow the unchanged line path.
    Any failed proof remains canonical at the same source ordinal.

    The immutable program validates that no-op ranges are sorted, nonempty,
    bounded by the source command count, disjoint from native runs, and equal
    to the stored exact no-op count. Mandatory spatial ranges explicitly
    exclude them, so sparse replay performs no query or cursor work for them.
    Full replay advances over an entire consecutive no-op range in O(1).
    The old per-native-line `start == end` replay test is removed: construction
    owns this semantic fact once.

    The mechanism is bounded by the existing 96 MiB program ceiling and by
    at most 1,048,576 ranges. Pages with no exact no-op commands retain no
    no-op vector allocation. The 574,229 Q16 no-ops previously consume about
    13.8 MB of line geometry; the range table costs at most 4.6 MB and less
    when ordinals coalesce. Removing commands from otherwise consecutive
    native runs can add source-run boundaries, however, so worst-case total
    retained memory is bounded rather than guaranteed to fall. This revision's
    guaranteed gain is removal of no-op geometry construction and per-line
    replay work; memory reduction is measured, not assumed.

    Device proof must use:

    ```text
    revision=r25-3-0119 event=compile
    mode=payload_free_noop_program
    nativeOpaqueLines=... exactNoOpLines=... exactNoOpRanges=... bytes=...

    revision=r25-3-0119 event=replay
    mode=payload_free_noop_backend
    exactNoOpLinesSkipped=... exactNoOpRanges=... replayUs=...
    ```

    Q16 acceptance requires the same `commands=3165420`, omission count,
    retained canonical barriers, painter order, and pixels as 0118.
    `exactNoOpLines` should remain about 574,229 while `nativeOpaqueLines`
    falls by that count, `exactNoOpRanges` must not exceed it, and
    compile/acquire timing should fall materially. Retained bytes should fall
    when no-op ranges coalesce and must remain below the same 96 MiB ceiling
    otherwise. Full replay must report the same skipped count. 11.pdf, EP23,
    and normal documents without this exact operation should retain the 0118
    representation and performance within run-to-run noise.

    Device results rejected the range representation as the final design.
    Q16 retained 574,229 exact no-op commands as 364,160 ranges. Removing
    their line payload reduced logical bytes from 80,768,903 to 75,145,199,
    but actual retained bytes fell only from 95,660,975 to 94,481,359 while
    native runs increased from 320,091 to 647,871. Acquire time increased
    from 7,898 ms to 8,636 ms, preview total from 9,929 ms to 10,667 ms, and
    launch-to-first-visible from 10,379 ms to 11,101 ms. Full replay remained
    about 2.03 seconds. The semantic proof was sound; materializing each
    alternating no-op interval as a source-run boundary was not.

16. **r25-3-0120: ranked sparse line tape.** Exact zero-coverage commands
    remain part of the surrounding `kNativeOpaqueLine` source run. Their only
    representation is one bit per source ordinal in a lazily grown 64-bit
    mask. A prefix rank is retained once per 256 source commands so sparse
    replay can map a selected source ordinal to compact line payload in
    bounded time. Full sequential replay carries the payload cursor forward
    and therefore performs no rank lookup for consecutive commands.

    No-op commands retain no geometry, state, matrix, clip, visibility,
    bounds, spatial posting, or raster payload. They also do not split an
    otherwise consecutive native line run. Full replay skips consecutive set
    bits word by word; sparse replay never selects them because they have no
    spatial posting. Canonical barriers and different native opcodes remain
    hard source-order boundaries.

    The immutable program validates all of the following before replay:

    - the mask is bounded by source command count;
    - prefix ranks exactly match mask population counts;
    - every set bit is covered by a native line run;
    - native line payload counts equal run commands minus set bits;
    - no set bit can occur in a fill, path, or canonical span.

    Construction charges mask words and rank blocks to the existing 96 MiB
    ceiling. Pages with no exact no-op commands allocate neither structure.
    The mechanism adds no classifier, threshold, second pass, page-sized
    bitmap, global cache, lock, Kotlin work, or UI-thread work.

    Device proof must use:

    ```text
    revision=r25-3-0120 event=compile
    mode=ranked_sparse_line_program
    nativeRuns=... exactNoOpLines=...
    exactNoOpWords=... exactNoOpRankBlocks=... bytes=...

    revision=r25-3-0120 event=replay
    mode=ranked_sparse_line_backend
    exactNoOpLinesSkipped=... replayUs=...
    ```

    Q16 must preserve the 0119 command, omission, native opcode, spatial,
    draw, and pixel results. `exactNoOpLines` should remain about 574,229 and
    `nativeOpaqueLines` about 2,365,894, while `nativeRuns` should return near
    the pre-range level rather than 647,871. Mask storage is bounded by one
    bit per covered source ordinal and rank storage by one 32-bit value per
    256 ordinals. Acceptance requires lower retained bytes and compile/acquire
    time than 0119. Full replay may improve from fewer run transitions, but
    the remaining roughly 2.36 million drawable lines still define its raster
    floor. 11.pdf, EP23, and pages without exact no-ops must report zero mask
    and rank entries and remain within timing noise.

    Device results confirmed that the ranked tape removed 0119's Q16
    fragmentation: `nativeRuns` fell from 647,871 to 320,091, actual retained
    bytes from 94,481,359 to 82,373,915, and logical bytes from 75,145,199 to
    67,432,547. Preview replay fell from about 2.03 seconds to 1.79 seconds;
    acquire remained effectively unchanged at 8,708 ms versus 8,636 ms.
    Launch-to-first-visible improved slightly from 11,101 ms to 10,823 ms,
    which is not a material first-render improvement.

    The build also exposed a deterministic validation defect for programs
    without exact no-ops. Validation initialized the consumed rank-block
    cursor to one even when both the mask and rank table were empty, then
    rejected the valid `1 != 0` final state. Consequently 11.pdf discarded its
    3.7 MiB Darken program and preview regressed from 691 ms to 8,868 ms.
    EP23 discarded its 10.3 MiB fill program and page-2 preview regressed from
    1,293 ms to 26,532 ms. These were canonical fallback costs, not raster or
    memory limits.

17. **r25-3-0121: empty ranked-tape validation correction.** The validated
    rank cursor now starts at zero when the optional rank table is empty and
    at one only when the required zero-prefix entry exists. A dedicated
    no-noop program test requires a valid program with zero mask words, zero
    rank blocks, and its native payload intact.

    This revision changes no command representation, eligibility, painter
    order, pixel operation, spatial index, cache, memory ceiling, or thread.
    It advances the revision because 0120 was already built and tested. Device
    acceptance requires:

    - 11.pdf logs `event=compile`, not `event=discard`, with the prior Darken
      counts and preview/tile timing restored near 0119;
    - EP23's 28,071-fill Form logs `event=compile` and form-cache
      store/hit/replay, not discard, with timing restored near 0119;
    - Q16 retains the exact 0120 tape counts, `nativeRuns=320091`, and retained
      bytes within allocation noise;
    - pages with no no-ops log `exactNoOpWords=0` and
      `exactNoOpRankBlocks=0` while remaining valid.

    Device results confirmed the correction restored 11.pdf and EP23 to their
    native programs. Q16 retained its 0120 representation
    (`commands=3165420`, `nativeRuns=320091`, `bytes=82373915`) but exposed the
    next first-render bottleneck: preview acquisition took about 9.42 seconds
    of an 11.48-second render, while replay took about 2.07 seconds. The
    builder performed 2,940,123 parser-direct line attempts and 2,590,767
    incremental budget checks even though the final program had one paint
    state, one clip run, and seven visibility runs. Spatial replay was already
    effective; repeated exact parser-state lowering dominated construction.

18. **r25-3-0122: exact parser-state line sink.** The parser remains the only
    producer and the immutable RenderProgram remains the only accelerated
    representation. The builder remembers the last successfully lowered line
    context using PDFium's copy-on-write backing identity for general, color,
    and graph state, plus the exact affine transform class and the existing
    exact clip/visibility scopes.

    A cache hit is not a page classification or semantic approximation. The
    cached state objects keep the old backing stores alive, so any mutation
    forces PDFium copy-on-write and changes identity. A mismatch returns to the
    complete lowerer before recording the command. Only a complete exact
    lowering may refresh the context. Matching commands still compute their
    transformed stroke bounds and append their source ordinal, spatial
    posting, line payload, and ordered run membership. Painter order, optional
    content, clipping, no-op rank mapping, canonical barriers, and pixels are
    unchanged.

    Repeated per-line retained-memory arithmetic is replaced only on exact
    context hits by bounded logical reservation packets: 4,096 line payloads,
    4,096 native runs when needed, and 1,024 line-state runs when needed.
    Reservations are charged to the existing 96 MiB ceiling before use; final
    actual-capacity validation remains authoritative. The context is one
    builder-local optional value. There is no global cache, lock, page-sized
    allocation, second pass, classifier, threshold, JNI/Kotlin work, or
    UI-thread work.

    Device proof must use:

    ```text
    revision=r25-3-0122 event=compile
    mode=exact_parser_state_line_sink
    parserDirectLineAttempts=...
    parserLineContextHits=...
    parserLineContextBuilds=...
    parserLineBudgetRefills=...
    compileWindowMs=...
    ```

    For Q16, `parserLineContextHits` should approach parser-direct attempts,
    context builds should track actual immutable-state changes rather than
    object count, reservation refills should stay near fixed packet count, and
    `incrementalBudgetChecks` should fall from about 2.59 million toward the
    roughly 221,000 non-line native commands.
    Command, omission, opcode, no-op, state, clip, visibility, spatial,
    actual retained-byte, replay, draw, and pixel results must match 0121.
    Logical charged bytes may differ by at most the bounded unused tail of the
    current reservation packets. Acceptance requires a material reduction in
    preview `acquireMs` and launch-to-first-visible time. 11.pdf, EP23, and
    ordinary pages without parser-direct lines do not enter this sink and must
    remain within measurement noise.

    Device results confirmed that the exact state sink was effective but did
    not remove the remaining parser allocation cost. On Q16,
    `parserLineContextHits=2940118` of 2,940,123 direct attempts, context builds
    fell to five, reservation refills to 613, and incremental budget checks
    fell from 2,590,767 to 224,878. Preview acquisition improved from 9,418 ms
    to 7,789 ms and total preview rendering from 11,484 ms to 9,602 ms.
    Actual retained program bytes stayed exactly 82,373,915; logical charge
    increased by only the bounded packet tail, from 67,432,547 to 67,496,923.
    11.pdf remained stable and did not enter the parser-direct line sink. EP23
    also reported zero direct-line attempts; its observed page-to-page timing
    variance did not identify a 0122 native-path regression.

19. **r25-3-0123: bounded omitted-path parser scratch.** `AddPathObject()`
    previously swapped `path_points_` into a temporary vector before exact
    lowering. Even when the lowerer returned `kOmitRecorded`, destruction of
    that temporary discarded the two-point allocation. A Q16-class stream
    therefore performed about 2.94 million small allocation/free cycles after
    0122 had already removed most repeated semantic and budget work.

    The parser now attempts the same exact two-point line lowering while the
    points remain in parser-owned scratch. Only `kOmitRecorded` with no clip
    clears and reuses that vector. `kRetainRecorded`, a clip consumer,
    unsupported state, or any failed exact lowering transfers ownership through
    the unchanged canonical `CPDF_Path` path. There is no second semantic
    predicate and no approximate fallback.

    Scratch retention is explicitly memory-bounded. Each parser may retain at
    most 16 `CFX_Path::Point` slots; larger capacity is released immediately.
    This is a storage ceiling, not a document classifier or an eligibility
    threshold. It adds no global cache, page-sized allocation, bitmap, lock,
    JNI/Kotlin work, or UI-thread work. RenderProgram format version 21 and its
    96 MiB retained-memory ceiling are unchanged.

    Device proof must use:

    ```text
    revision=r25-3-0123 event=compile
    mode=bounded_parser_path_scratch
    parserDirectLineAttempts=...
    parserLineContextHits=...
    parserLineContextBuilds=...
    parserLineBudgetRefills=...
    compileWindowMs=...
    ```

    Q16 must preserve 0122 command, opcode, omission, state, clip, visibility,
    spatial, no-op, actual/logical retained-byte, replay, draw, and pixel
    results. The acceptance signal is materially lower preview `acquireMs` and
    `compileWindowMs`; replay time should remain within noise because replay is
    unchanged. 11.pdf, EP23, and ordinary pages with zero parser-direct line
    attempts should remain within timing noise.

    Device results satisfy the acceptance criteria. On Q16, acquisition fell
    from 7,789 ms to 5,536 ms (-28.9%), bitmap replay fell from 1,812 ms to
    1,434 ms (-20.9%), total render fell from 9,602 ms to 6,971 ms (-27.4%),
    and launch-to-first-visible fell from 9,976 ms to 7,231 ms (-27.5%).
    Relative to 0121, Q16 acquisition is down 41.2%, total render 39.3%, and
    first visible 38.6%.

    The Q16 representation remained exact and byte-identical to 0122:
    3,165,420 commands, 225,297 retained commands, 2,940,123 exact no-op
    omissions, 2,365,894 native opaque lines, 320,091 native runs, 535 spatial
    cells, 2,662,901 spatial postings, 82,373,915 actual bytes, and 67,496,923
    logical retained bytes. The parser scratch therefore removed allocation
    churn without changing lowering, ordering, replay, or retained memory.

    11.pdf improved from 304 ms to 221 ms total, EP23 p2 from 1,181 ms to
    493 ms, and EP23 p3 from 1,168 ms to 355 ms. All three report
    `parserDirectLineAttempts=0`, so these gains are runtime, filesystem-cache,
    or device variance rather than causal 0123 gains. They establish no
    regression; they must not be credited to the new scratch path.

    Acquisition still consumes 5,536 ms, or 79.4% of Q16 total render time.
    Inspection identifies the next general redundant work: `ParsePathObject()`
    discovers a path paint terminator, rewinds, and lets the outer parser read
    and dispatch that operator again. Dense `m l S` streams repeat this once
    per exact line. The next revision should consume a recognized terminator
    once inside the path parser and invoke the same existing exact handler.
    Complex, clipping, or unsupported paths must preserve the canonical path;
    the optimization needs only fixed parser-local state and no classifier,
    command threshold, cache, or approximate semantics.

0105 remains separate because it repairs an already-shipped traversal invariant
without changing the pixel path. Combining that correction with new fill
semantics would make a correctness failure impossible to attribute. The former
fill-lowering and mixed-executor tasks were merged into 0106. The
former line-compaction and spatial-index tasks share one memory budget and were
merged into 0107.

Each revision must ship a real behavioral change plus the counters needed to
prove it. Telemetry-only builds are not part of this sequence. A revision does
not advance until its patch applies cleanly, unit tests build, normal-page pixel
tests pass, and its stated device counter moves in the expected direction.

### Separate Android Scheduling Patch

Native executor work and viewport admission remain separate concerns. After
0105 establishes predictable native tile cost, the Android layer will use one
immutable viewport generation to admit only current visible tiles, prioritize
the anchor/zoom-center region, replace queued obsolete generations, and cancel
obsolete running native work at existing cancellation boundaries. It will not
clear valid prior-scale coverage before replacement tiles arrive.

The scheduling proof is lower admitted work, bounded queue depth, and removal
of the 11.pdf 916 ms queue wait. It must not change PDFium opcodes, page
classification, bitmap ownership, or canonical/native pixel semantics.

These revisions extend the same sidecar and executor. They do not add a page
classifier, Kotlin preprocessing pass, second bitmap owner, or alternate
renderer.

## Invariants

- The original PDF content stream is authoritative; complete canonical PDFium
  page objects materialize before editing, enumeration, or canonical fallback.
- One source order, one destination bitmap, one pixel owner at each ordinal.
- Native execution is allowed only when exact lowering succeeds.
- Unknown or unsupported semantics remain canonical at the same ordinal.
- No native-to-canonical whole-holder restart after native pixels.
- Mutation, unsupported context, invalid structure, or budget failure fails
  before program pixels; per-operation device rejection falls back before that
  operation's first pixel.
- Fixed retained-memory ceilings and stack-bounded replay packets.
- Holder-space indexes are built once and queried with one bounded,
  render-status-local reusable bitset.
- No filename rule, count threshold, page classification, UI-thread rendering,
  worker pool, global render lock, or tile scheduler is introduced.
