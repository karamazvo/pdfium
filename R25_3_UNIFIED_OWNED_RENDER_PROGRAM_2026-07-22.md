# r25-3 Unified Sparse RenderProgram

**Locked:** 2026-07-22 (Asia/Taipei)
**Updated through:** r25-3-0109 on 2026-07-23 (Asia/Taipei)

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

- PDFium page objects are the single source of truth for fidelity and editing.
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
