# Production PDFium patches (ship-quality)

The active RenderPlan architecture, evidence, revision sequence, and proof
gates are locked in
[`RENDERPLAN_FIRST_PRINCIPLES_DENSE_TILE_PLAN_2026-07-14.md`](RENDERPLAN_FIRST_PRINCIPLES_DENSE_TILE_PLAN_2026-07-14.md).

This directory contains the **production PDFium patch series**
used by the Veloce PDFium build. Unlike the `experiments/` directory,
these patches:

- Are **complete and audit-ready**. Patches 01-04 apply independently to a
  clean PDFium HEAD. Patch 05 applies to clean PDFium HEAD. Patches 06 and
  07 are intentionally layered after patch 05 because they extend the same
  Veloce render-loop/export surface. Patch 08 is layered after the Veloce
  fpdfview export patches and adds first-render page classification.
- Are applied in numeric order by the Veloce release workflow.
- Contain **no ATrace markers** and no Android-specific includes. Patches
  01-04 are production-safe by default. Patch 05 is diagnostic/default-off:
  it only changes rendering when `FPDFEx_SetSkipRasterization()` is
  explicitly enabled by a measurement harness.
- Are **bit-identical output** versus baseline PDFium where it matters
  (patches 02 and 03 are pure speedups; patch 01's libjpeg scale path
  produces visually-equivalent output via libjpeg's documented API; patch
  04 adds exports only and does not change renderer behavior; patch 05
  preserves normal output while its toggle remains disabled).
  Patches 06 and 07 also preserve normal output while the abort flag remains
  disabled.

## The patches

| # | File | Effect |
|---|---|---|
| 01 | `01-jpeg-downscale-on-decode.patch` | Embedded JPEG images decode at the embedder's requested target size via libjpeg's `scale_num/scale_denom`, instead of always decoding at native size and downscaling the resulting bitmap. ~5-64× decode-time speedup at small render sizes. |
| 02 | `02-indexed-separation-fast.patch` | Adds fast `TranslateImageLine` overrides for `CPDF_IndexedCS` and `CPDF_SeparationCS` via precomputed BGR lookup tables. Replaces per-pixel virtual `GetRGB()` dispatch + float math with a 1-byte-index → 3-byte memcpy. Bit-identical output. ~50-200× per-scanline speedup. |
| 03 | `03-devicen-fast.patch` | Same fast-table pattern for `CPDF_DeviceNCS` (N=1 and N=2 component variants; N≥3 falls through to the existing slow path). Bit-identical output. ~50-200× per-scanline speedup. |
| 04 | `04-veloce-internal-access.patch` | Adds Veloce internal-access exports for graphics-state proof, rectangular clip detection, Form transparency groups, fast object iteration, and ABI sentinels. No rendering semantics change. |
| 05 | `05-veloce-skip-rasterization-probe.patch` | Adds the R12 diagnostic export `FPDFEx_SetSkipRasterization()`. When enabled, `FPDF_RenderPageBitmap()` walks page objects and cheap visibility rejection but skips `RenderSingleObject()`, producing a blank/unchanged bitmap for rasterizer-cost measurement. Default disabled. |
| 06 | `06-veloce-render-abort-probe.patch` | Adds the R18/R19 diagnostic export `FPDFEx_SetRenderAbort()`. When enabled, `CPDF_RenderStatus::RenderObjectList()` returns early at page-object boundaries so a cancelled progressive render can release xPDFSDK's single render lane. Default disabled. |
| 07 | `07-veloce-render-abort-deeper.patch` | Adds deeper abort checkpoints inside `RenderSingleObject()`, progressive image continuation, Form XObject recursion, transparency rendering, Type3 Form rendering, and soft-mask rendering. Default disabled through patch 06's abort flag. |
| 08 | `08-load-page-with-classification.patch` | Adds `FPDFEx_LoadPageWithClassification()` plus the fixed 64-byte `FPDFEx_PageClassification` ABI so xPDFSDK can route path-dense pages on the first render. No rendering semantics change. |
| 09 | `09-render-callbacks-scoped-cancel.patch` | Adds `FPDFEx_SetRenderCallbacks()` and a per-render scoped cancellation callback table layered on the existing 06/07 abort checkpoints. Existing `FPDFEx_SetRenderAbort()` remains supported. |
| 0011 | `0011-veloce-form-skip-mask.patch` | Adds `FPDFEx_SetRenderSkipMask(FPDFEX_RENDER_SKIP_FORM)` so callers can render a first pass that omits Form XObjects, then run a full pass later. Default mask is zero, so normal rendering is unchanged. |
| 0012 | `0012-veloce-image-skip-mask.patch` | Adds the `FPDFEX_RENDER_SKIP_HUGE_IMAGE` bit to the same skip mask plus `FPDFEx_SetHugeImagePixelThreshold()`. When set, any non-mask image whose decoded pixel count exceeds the threshold (default 4,000,000 px, ~2000×2000) is skipped and replaced with a flat gray placeholder rect, at any Form-recursion depth. Default mask is zero, so normal rendering is unchanged. |
| 0013 | `0013-veloce-path-display-list-form.patch` | Adds an experimental `FPDFEX_FEATURE_PATH_DISPLAY_LIST_FORM` feature bit. When set, eligible fill-only path Form XObjects are compiled into a compact internal display list and replayed into the current `CFX_RenderDevice`; unsupported content falls back before drawing. Default feature flags are zero, so normal rendering is unchanged. |
| 0014 | `0014-veloce-path-display-list-cache.patch` | Extends 0013 with a bounded process-local cache for compiled Form path display lists, keyed by document, holder, holder dictionary object number, and path-smoothing mode. Repeated renders of the same eligible Form replay from cache and log `cache=hit|miss`; compile cancellation returns `kCancelled` instead of falling back to legacy rendering. |
| 0015 | `0015-veloce-path-display-list-noop-clip.patch` | Extends 0014 with compile-time no-op clip normalization. If an eligible node's clip is one rectangle that contains the object's bounds, the display list stores `kNoClipKey` instead of replaying that clip state and logs `noopClips`. |
| 0016 | `0016-veloce-path-display-list-stacked-rect-noop-clip.patch` | Extends 0015 to support stacked rectangle-like clip components. It intersects all rectangle clips and elides the clip if that intersection contains the path object's bounds. |
| 0017 | `0017-veloce-holder-root-page-path-display-list.patch` | Extends the same native path display-list renderer from Form holders to root page holders. Path-only dense root pages such as `11.pdf` can now use native Veloce instead of the app-level Skia replay path, while Form-heavy pages such as `error.pdf` `/Meta20` continue through the same implementation. Adds `holderKind=root_page|form` telemetry. |
| 0018 | `0018-veloce-root-holder-context-layer.patch` | Corrects the root-page hook to identify top-level render-context layer holders instead of relying on `pObjectHolder->IsPage()`. This preserves the Form hook while allowing root path-only pages such as `11.pdf` to emit `holderKind=root_page`. |
| 0019 | `0019-veloce-progressive-root-path-display-list.patch` | Adds the missing root-page hook to `CPDF_ProgressiveRenderer::Continue()`, which is the Android `FPDF_RenderPageBitmap()` root-page path. This lets `11.pdf` emit `holderKind=root_page` instead of bypassing Veloce and walking every root page object. |
| 0020 | `0020-veloce-path-display-list-allow-fill-alpha.patch` | Allows ordinary fill alpha on fill-only path nodes. PDFium's normal path carries this through `GetFillArgb()` and `DrawPath()` rather than `ProcessTransparency()`, so Veloce can replay it while still rejecting soft masks and non-normal blend modes. |
| 0021 | `0021-veloce-path-display-list-stroke-replay.patch` | Adds native stroked-path replay. Stroke color and graph state are deduplicated in side tables and replayed through the same `DrawPath()` call PDFium uses for `ProcessPath()`. |
| 0022 | `0022-veloce-path-display-list-split-transparency-rejects.patch` | Splits the broad `transparency` reject telemetry into `soft_mask` and `blend_mode`. Rendering behavior is unchanged. |
| 0023 | `0023-veloce-path-display-list-darken-blend-replay.patch` | Allows `/BM /Darken` path nodes in the native Veloce path display list by rendering each blended node into a temporary BGRA bitmap and compositing it back through PDFium's existing `SetDIBitsWithBlend()` path. |
| 0024 | `0024-veloce-path-display-list-gate-dense-scratch-blend.patch` | Adds a fail-closed gate for dense root-page Darken scratch replay. If Veloce would route more than 512 blended nodes through the temporary BGRA bitmap path, it returns `not_eligible` before drawing so legacy PDFium/app routing can avoid the r17 `11.pdf` regression. |
| 0025 | `0025-veloce-path-display-list-mask-blend-replay.patch` | Adds a native mask-composite replay path for fill-only blended path nodes. Veloce renders the path coverage into an 8-bit mask and composites that mask through PDFium's existing scanline blend compositor, avoiding per-node BGRA scratch bitmaps for `11.pdf`-style fill-only `/BM /Darken` paths. Adds `scratchBlendNodes` telemetry. |
| 0026 | `0026-veloce-path-display-list-mask-blend-fill-stroke.patch` | Extends 0025's mask-composite replay to fill+stroke blended path nodes when fill and stroke colors are identical. This keeps correctness for mixed-color fill/stroke paths while allowing `11.pdf`-style Darken paths parsed with stroke state to avoid BGRA scratch replay. |
| ~~0027~~ | ~~`0027-veloce-path-display-list-batched-mask-blend.patch`~~ | **DEPRECATED — do not apply.** Groups nodes by `(paint_key, clip_generation)` across the whole clip group and flushes in paint-key order, violating PDF painter order when different paint keys interleave in the display list. Produces wrong output on PDFs with interleaved blend nodes of different paint keys. Validated on `11.pdf` only (single-paint-key workload; reordering had no visible effect there). Superseded by 0029. |
| ~~0028~~ | ~~`0028-veloce-path-display-list-packed-path-run.patch`~~ | **DEPRECATED — do not apply.** Inherits 0027's painter-order flaw and adds clip-group mask reuse on top of the same incorrect grouping. Superseded by 0029. |
| 0029 | `0029-veloce-path-display-list-consecutive-run-blend.patch` | Consecutive-run mask batching. Batches only adjacent blend nodes that share the same `paint_key`. Flushes the run on any painter-order boundary (paint/clip/normal/scratch-blend change). Allocates one mask per run, draws all nodes in original display-list order, composites once. Adds `maskRunComposites`, `maskRunNodes`, `maxMaskRunNodes`, `runFlushPaint/Clip/Normal/Scratch/Cancel/End` telemetry. Has three correctness issues fixed by 0030. |
| 0030 | `0030-veloce-path-display-list-run-correctness-fixes.patch` | Three correctness fixes to 0029. **P1:** adds a conservative bbox overlap gate (later superseded by 0031's group-buffer approach). **P2:** removes the flush-on-cancel path — cancelled output is discarded, so writing partial content to the bitmap is wasted work; the run is now just reset. **P3:** counts the end-of-loop drain as `runFlushEnd` instead of `runFlushPaint` so flush-reason telemetry is unambiguous. |
| 0031 | `0031-veloce-path-display-list-group-buffer-blend.patch` | Replaces the 8-bit mask + `SetMaskBitsWithBlend` flush with a BGRA group buffer + `SetDIBitsWithBlend`, matching MuPDF's transparency group model. Each run allocates one BGRA bitmap covering the union rect of all node rects; paths rasterize into it with normal blend (as MuPDF does inside `fz_draw_begin_group`); the completed group composites onto the destination once with `SetDIBitsWithBlend(blend_mode)` (as MuPDF does at `fz_draw_end_group`). Overlapping paths are correct because they accumulate under normal blend inside the group before the single blend-mode composite. Removes the P1 bbox overlap gate (no longer needed — overlaps are safe in the group buffer). Renames telemetry to `groupRunComposites`, `groupRunNodes`, `maxGroupRunNodes`. |

| 0032 | `0032-veloce-path-display-list-stroke-run-packing.patch` | Stroke run packing. Consecutive stroke-only normal-blend nodes sharing the same `paint_key` and path matrix are appended into one `CFX_Path` via `CFX_Path::Append` and drawn with a single `DrawPath` call using the preserved matrix. This keeps stroke width, dash, and stroke adjustment tied to the original CTM. Targets CAD/engineering PDFs (e.g. DWG exports from Bentley InterPlot) with hundreds of thousands of single-line-segment stroke paths. Adds `strokeRunDraws`, `strokeRunNodes`, `maxStrokeRunNodes` telemetry. |
| ~~0033~~ | _(not shipped)_ | **REVERTED — do not apply.** Reserved for the failed group-buffer resolution-cap experiment. It downscaled large group buffers and stretched them back, but regressed `11.pdf` replay time severely because it added per-composite stretch work while preserving thousands of composites. Kept as a reserved number to avoid patch-history confusion. |
| 0034 | `0034-veloce-path-display-list-ordered-text-passthrough.patch` | Ordered segmented acceleration. Converts the path-only display list into an ordered segment plan so text objects can remain in painter order between accelerated path runs instead of rejecting the whole holder. P1 supports `PathRun` + text passthrough only; image/Form/shading passthrough still rejects before drawing. Adds `segments`, `pathSegments`, `pathSegmentNodes`, `maxPathSegmentNodes`, `textPassthroughObjects`, and `unsupportedPassthroughObjects` telemetry. |
| 0035 | `0035-veloce-path-display-list-stroke-run-flush-telemetry.patch` | Stroke-run flush telemetry. Adds reason counters for existing stroke-run flushes (`strokeRunFlushPaint`, `strokeRunFlushColor`, `strokeRunFlushGraphState`, `strokeRunFlushMatrix`, `strokeRunFlushPathStyle`, `strokeRunFlushFillMode`, `strokeRunFlushClip`, `strokeRunFlushBlend`, `strokeRunFlushSegment`, `strokeRunFlushCapacity`, `strokeRunFlushEnd`). Telemetry-only: replay order and draw decisions are unchanged. |
| 0036 | `0036-veloce-path-display-list-nonoverlap-fill-barrier-stroke-packing.patch` | Non-overlap fill-barrier stroke packing. Keeps a pending stroke run open across a normal non-stroke path only when expanded clipped device-space bounds prove that the barrier is disjoint from the pending stroke run. Overlapping or unknown barriers still flush. Adds `strokeRunFillBarriersCrossed` and `strokeRunFillBarriersBlocked` telemetry. |
| 0037 | `0037-veloce-path-display-list-holder-space-spatial-index.patch` | Holder-space spatial index. Builds a cached 32x32 grid over path node bboxes in holder/page coordinates, transforms the device tile clip back to holder space at replay time, queries candidate bins, sorts candidate node ids back into display-list order, and then reuses existing segment replay. Broad preview clips fall back to the old full scan. Adds `spatialIndex*` telemetry. |
| 0038 | `0038-veloce-path-display-list-disable-text-passthrough-cache.patch` | Text-passthrough cache lifetime fix. Display lists containing ordered text passthrough segments still replay for the current holder, but are not stored in the process display-list cache because those segments hold raw `CPDF_PageObject*` pointers whose lifetime is only the live holder replay. Prevents r31 cache-hit use-after-free crashes. |
| 0039 | `0039-veloce-path-display-list-text-passthrough-index-cache.patch` | Text-passthrough index cache fix. Replaces cached raw text passthrough pointers with holder object indexes, resolves live text objects from the current holder before replay, and restores display-list cache insertion for Q16-style text-barrier pages. |
| 0040 | `0040-veloce-path-display-list-fill-barrier-telemetry.patch` | Fill-barrier telemetry. Adds compact `VelocePathDLFill` logs for normal barriers that are not pure stroke-run nodes: fill-only/fill+stroke shape, fill rule, rect-like/thin/empty device bounds, path point counts, and pending-stroke overlap/disjoint/unknown counts. Telemetry-only; draw order and packing decisions are unchanged. |
| 0041 | `0041-veloce-path-display-list-blend-group-cancellation.patch` | Blend group cooperative cancellation. Adds cancellation checkpoints inside `BlendGroupRun` group-buffer flushes before allocation, during group rasterization, before final composite, and after composite. Successful renders remain visually unchanged; cancelled renders return `kCancelled` and are discarded by callers. |
| 0042 | `0042-veloce-path-display-list-blend-run-widening-telemetry.patch` | Blend-run widening telemetry. Adds group-buffer pixel cost metrics (`groupRunPixels`, `groupRunNodePixels`, `maxGroupRunPixels`) and paint-change barrier opportunity counters (`blendPaint*`) plus a compact `VelocePathDLBlend` log. Telemetry-only; it does not merge or reorder blend runs. Use this to decide whether a future multi-paint `BlendGroupRun` can safely reduce `11.pdf` successful-tile cost. |
| 0043 | `0043-veloce-path-display-list-primitive-run-telemetry.patch` | Primitive/run telemetry. Precomputes path point/subpath counts per cached path and logs candidate/culled/drawn primitive totals during replay through `VelocePathDLPrimitive`. Telemetry-only; it does not change spatial-index selection, replay order, path packing, blend grouping, cancellation, or drawing semantics. Use this to decide whether the next MuPDF-style step should split dense path nodes into smaller indexed primitives. |
| 0044 | `0044-veloce-path-display-list-fill-barrier-proof-telemetry.patch` | Fill-barrier proof telemetry. Classifies stroke-run-blocking normal fill barriers by no-pixel, thin, rect-like, same-color, and coarse device-rect containment predicates through `VelocePathDLFillProof`. Telemetry-only; it does not cross additional barriers or change drawing. Use this to decide whether r41/r42 can safely promote a narrow fill-barrier crossing rule. |
| 0045 | `0045-veloce-path-display-list-same-argb-fill-barrier-crossing.patch` | Same-ARGB fill-barrier crossing. Keeps a pending normal stroke run open across a fill-only normal barrier when the barrier fill ARGB exactly matches the pending stroke ARGB. This promotes the r40 same-color proof into behavior using source-over commutativity, without crossing segment, text, clip, blend, pattern, soft-mask, or fill+stroke barriers. Adds `strokeRunFillBarriersSameArgbCrossed` / `sameArgbCrossed` telemetry. |
| 0046 | `0046-veloce-path-display-list-stroke-run-compact-telemetry.patch` | Stroke-run compact telemetry. Adds a short `VelocePathDLStroke` log that survives Android log truncation, records actual stroke-run flush reasons, buckets run lengths, and splits matrix flushes into same-linear/translation-only versus linear-change cases. Telemetry-only; rendering order, segment barriers, spatial selection, blend behavior, and draw decisions are unchanged. |
| 0047 | `0047-veloce-path-display-list-translation-normalized-stroke-run-packing.patch` | Translation-normalized stroke-run packing. Keeps stroke-only normal-blend runs open across same-linear translation-only matrix changes by appending each path through a relative coordinate-space translation and drawing the packed run with the base matrix. A fixed packed-point cap bounds temporary path memory and uses the existing capacity flush. Does not cross paint, graph-state, path-style, clip, blend, text, Form, image, or segment barriers. |
| 0048 | `0048-veloce-path-display-list-safe-blend-paint-widening.patch` | Safe blend paint widening. Lets a `BlendGroupRun` contain multiple paint keys only when the adjacent paint change keeps the same final blend mode, the current group/device rect and next node device rect are disjoint, and the union rect is memory-bounded. Each entry still draws with its own paint inside the isolated group; overlapping different-paint blend objects still flush. Adds `paintSwitches` / `maxPaints` telemetry to `VelocePathDLBlend`. |
| 0049 | `0049-veloce-path-display-list-effective-blend-paint-widening.patch` | Effective blend paint widening. Lets overlapping blend paint-key changes stay inside the current `BlendGroupRun` only when every current group entry has the same render-affecting blend paint as the next node. For fill-only blend paths, stroke-adjust is ignored because no stroke is drawn; non-equivalent overlapping paints still flush. Adds `paintEquivalent` / `paintEquivalentCrossed` telemetry. |
| 0050 | `0050-veloce-path-display-list-blend-shape-telemetry.patch` | Blend shape telemetry. Adds a compact `VelocePathDLBlendShape` line that classifies existing `BlendGroupRun` composites by blend mode, path paint shape, same source ARGB, and simple Darken candidate cost. Telemetry-only; it does not change grouping, painter order, cancellation, allocation policy, or pixels. |
| 0051 | `0051-veloce-page-dimensions-no-parse.patch` | Page dimensions no-parse export. Adds fixed 32-byte `FPDFEx_PageDimensions` and `FPDFEx_GetPageDimensions()` so embedders can read page width/height, effective CropBox, and rotation from the page dictionary without `FPDF_LoadPage()` or `ParseContent()`. No rendering semantics change. |
| 0052 | `0052-veloce-path-display-list-same-source-darken-widening.patch` | Same-source Darken blend widening. Lets an adjacent Darken `BlendGroupRun` paint-key change stay inside the current ordered segment/clip when every pending entry has one source ARGB, the next blend node has that same source ARGB, and the union group rect stays memory-bounded. Each entry still draws with its own path, matrix, graph state, and fill/stroke options inside the isolated group. Adds `sameSourceDarken*` telemetry. |
| 0053 | `0053-veloce-render-plan-interface.patch` | RenderPlan holder interface. Adds `veloce_render_plan.{h,cpp}` as the single renderer-facing boundary between PDFium's baseline pipeline and Veloce acceleration. Behavior-preserving on r25: it delegates to the existing path-display-list backend and only maps result/kind enums. Future acceleration patches should compose behind this fail-closed interface instead of adding direct hooks to `CPDF_RenderStatus` or `CPDF_ProgressiveRenderer`. |
| 0054 | `0054-veloce-render-plan-skeleton.patch` | RenderPlan ordered segment skeleton. Adds `VeloceRenderPlanSegmentKind`, `VeloceRenderPlanSegment`, `VeloceRenderPlan`, and `VeloceBuildRenderPlanSkeletonForHolder()`. The skeleton stores holder object indices only, groups consecutive path objects into `PathRun` segments, and emits non-path objects as ordered passthrough barriers. Behavior-preserving: the render facade still delegates to the r25 path-display-list backend and does not build the skeleton on the render hot path. |
| 0055 | `0055-veloce-render-plan-segmented-text-passthrough.patch` | RenderPlan segmented text passthrough. Adds range-based path-display-list compile/replay handles, preflights every `PathRun` before drawing, and consumes ordered `PathRun` + text passthrough segments behind the `VeloceTryRenderPlanForHolder()` facade. Non-text passthrough, blend barriers, unsupported barriers, and any ambiguous replay requirement still fail closed before drawing. All-path holders continue through the existing whole-holder cached backend. |
| 0056 | `0056-veloce-render-plan-bounded-cache.patch` | Bounded RenderPlan skeleton cache. Caches immutable ordered segment metadata only, keyed by document pointer, live holder pointer, holder dictionary object number, and holder kind. Values store holder object indices/counts, never raw page-object pointers or compiled path-display-list handles. Cache is bounded to 64 entries with simple LRU eviction; PathRun compile/replay validation remains per-render. |
| 0057 | `0057-veloce-render-plan-holder-space-spatial-index.patch` | Holder-space spatial index for compiled PathRun replay. Builds a bounded 32x32 grid over node holder-space bboxes for large path lists, transforms each device tile clip back to holder space, queries candidate bins, sorts node ids into original display-list order, and then reuses the existing replay body and device-clip culling. Broad preview clips and unsafe matrices fall back to the full scan. The index only selects candidates inside an already ordered PathRun; it never crosses RenderPlan barriers or changes eligibility, paint, clip, blend, or draw semantics. |
| 0058 | `0058-veloce-render-plan-facade-telemetry.patch` | RenderPlan facade telemetry. Emits a compact Android-only `VeloceRenderPlan` line showing holder kind, plan shape, segment counts, segmented result, fallback result, and whether the facade rendered via ordered segmented replay or the legacy whole-holder backend. Behavior-preserving: no eligibility, drawing, cache, allocation, or fallback semantics change. Use this to prove whether `11.pdf` and `error.pdf` p2/p3 are entering RenderPlan and where native completeness work should continue. |
| 0059 | `0059-veloce-render-plan-whole-path-run-backend.patch` | RenderPlan whole-path-run backend. Treats a single-`PathRun` RenderPlan as a complete native plan and invokes the existing cached whole-holder path-display-list implementation as `path=whole_path_run`. The old generic holder call remains only as `path=legacy_holder_fallback` for non-single-PathRun shapes. Behavior-preserving: eligibility, draw order, cache keys, allocation, cancellation, blend handling, and fallback semantics are unchanged. |
| 0060 | `0060-veloce-render-plan-cost-telemetry.patch` | RenderPlan cost telemetry. Adds stage timing and cache-hit fields to the existing `VeloceRenderPlan` log line: `planCache`, `planLookupMs`, `segmentedMs`, `wholePathRunMs`, `legacyFallbackMs`, and `totalMs`. Behavior-preserving: it does not add a holder scan, new eligibility rule, draw change, cache allocation, or fallback behavior. Use this with `VelocePathDL` compile/replay/spatial-index logs to identify whether Q16-like slow tiles are dominated by RenderPlan lookup/build, segmented preflight, whole-path-run replay, or late fallback. |
| 0061 | `0061-veloce-render-plan-compiled-pathrun-cache.patch` | RenderPlan compiled PathRun cache. Caches immutable compiled path-display-list handles per ordered PathRun segment, keyed by document, live holder, holder dictionary object number, holder kind, segment range, and path smoothing option. Cache hits still run replay validation before drawing; text passthrough is still resolved from the live holder; RenderPlan barriers remain hard ordering boundaries. Cache is bounded by entry count and compiled node count, and `VeloceRenderPlan` logs `compiledPathRunCacheHits`, `compiledPathRunCacheMisses`, and `compiledPathRunNodes` to prove whether Q16-like tiles are replaying cached segments rather than rebuilding them. |
| 0062 | `0062-veloce-render-plan-live-passthrough-table.patch` | RenderPlan live passthrough table. Replaces repeated linear holder walks for text passthrough segments with a per-render bounded table containing only the live objects referenced by passthrough indices in the cached RenderPlan. The cached plan remains pointer-free and index-based; the temporary table is bounded by passthrough segment count, built from one forward holder scan, and discarded after the render. Adds `livePassthroughObjects` and `livePassthroughTableMs` telemetry to prove whether Q16-like segmented replay is still spending time resolving barriers. |
| 0063 | `0063-veloce-render-plan-clip-bounded-pathrun-chunks.patch` | RenderPlan clip-bounded PathRun chunks. Stores holder-space bboxes on ordered RenderPlan segments, splits long consecutive path streams into 8192-object PathRun chunks, and skips chunks whose bbox is outside the current holder-space clip before compile/replay. Painter order is preserved because chunks remain adjacent ordered PathRun segments and never cross passthrough/blend/unsupported barriers. Raises the compiled PathRun cache entry cap to 512 while keeping the 4M compiled-node memory bound, and logs `clipSkippedPathRuns` / `clipSkippedPathObjects` to prove whether visible clips avoid whole-page replay work. |
| 0064 | `0064-veloce-render-plan-chunk-cancellation-checkpoints.patch` | RenderPlan chunk cancellation checkpoints. Adds cooperative `ShouldCancelRender()` checks at RenderPlan segmented replay boundaries: before chunk compile/validation, before chunk replay, after passthrough rendering, and before the final success return. Successful renders remain exact; cancellation returns `kCancelled` only when PDFium's existing abort bridge says the bitmap will be discarded. Logs `renderPlanCancelChecks` so broad/cold Q16-style renders can prove they stop promptly when superseded by newer visible tiles. |
| 0065 | `0065-veloce-render-plan-spatial-replay-metrics.patch` | RenderPlan spatial replay metrics. Reuses the 0057 path-display-list holder-space index as the single clipping source of truth and exports value-only replay metrics from each compiled PathRun into the facade log: `replayVisited`, `replayCulled`, `replayDrawn`, `spatialIndexQueries`, `spatialIndexQueryBins`, `spatialIndexCandidates`, `spatialIndexSkippedByTile`, `spatialIndexFallbackFullScan`, and `spatialIndexQueryMs`. Behavior-preserving: no new index, object scan, candidate allocation, eligibility rule, draw-order change, or pixel change. It distinguishes unnecessary replay from genuinely dense visible content before changing the clipping algorithm. |
| 0066 | `0066-veloce-render-plan-bounded-spatial-candidate-mask.patch` | RenderPlan bounded spatial candidate mask. Replaces the coarse "more than half the bins" full-scan heuristic with an exact holder-clip containment gate. Partial clips collect candidates into a bounded one-bit-per-node mask, deduplicate without sorting, and replay set node IDs in original painter order. The mask is at most 1 KiB for an 8192-node RenderPlan chunk and is allocated only for partial-clip queries. Full-page renders retain sequential replay when the clip provably contains the complete chunk bounds. |
| 0067 | `0067-veloce-render-plan-visible-passthrough-lookup.patch` | RenderPlan visible passthrough lookup. Removes the per-render full-holder scan, temporary raw-pointer table, index vector, and binary searches introduced by 0062. Cached plans continue to store object indices only. Preflight and replay skip passthrough segments whose cached holder-space bbox is outside the current clip; visible segments resolve their object through the live holder's bounds-checked `GetPageObjectByIndex()` immediately before validation or painting. Adds `livePassthroughLookups` and `clipSkippedPassthroughObjects` telemetry. |
| 0068 | `0068-veloce-render-plan-fail-closed-spatial-culling.patch` | Makes spatial culling conservative: every node is represented by bins or an always-replayed overflow list, uncertain segment bounds disable segment skipping, and query clips expand by one device pixel before inverse transformation. Prevents successful but incomplete tiles. |
| ~~0069~~ | `0069-veloce-render-plan-bounded-disjoint-stroke-batching.patch` | **REPLACED BY 0070.** Attempted bounded geometry merging only for disjoint adjacent strokes. Telemetry showed no useful dispatch reduction; retained in revision history but superseded by exact command batching. |
| 0070 | `0070-veloce-render-plan-exact-agg-stroke-command-batching.patch` | Replaces geometry merging with bounded AGG command packets. Every path remains a separate raster and destination composite; only immutable setup is shared for adjacent same-paint, same-matrix strokes. |
| 0071 | `0071-veloce-render-plan-reuse-agg-batch-scratch-storage.patch` | Reuses path, rasterizer, and scanline storage inside a bounded packet, resetting logical state between exact per-path operations. |
| 0072 | `0072-veloce-render-plan-device-owned-agg-scratch.patch` | Moves bounded AGG scratch to the single-owner device so capacity is reused across dispatches; also separates compile and replay timings. |
| 0073 | `0073-veloce-render-plan-indexed-range-compile.patch` | Compiles each RenderPlan range by direct holder object index instead of repeatedly scanning every preceding holder object. |
| 0074 | `0074-veloce-render-plan-per-matrix-agg-command-batching.patch` | Carries one exact matrix per command so adjacent same-paint strokes stay in a bounded packet across matrix changes. Each command is still transformed, rasterized, and composited separately in painter order. Adds `strokeMatrixChangesBatched` proof telemetry. |
| 0075 | `0075-veloce-render-program-v2-ownership-boundary.patch` | Starts the replacement RenderProgram v2 line from the correctness baseline (`r25 + 0051`), not from experimental patches `0053-0074`. Adds an optional holder-owned `unique_ptr<const VeloceRenderProgram>` and narrow install/reset accessors. The program remains unpopulated and has no parser or renderer hook, so this revision adds no scan, allocation, cache, lock, log, public API, or pixel change. |

## Why this directory exists

During development the patches went through many iterations under
`patches/experiments/` (numbered 0001..0010). Many of those were
diagnostic-only (trace markers, ICC bypass, etc.) and depend on each
other. The `ship/` directory is the **clean, audit-ready, upstreamable**
subset.

## How to apply

```bash
cd /path/to/pdfium
git apply patches/ship/01-jpeg-downscale-on-decode.patch
git apply patches/ship/02-indexed-separation-fast.patch
git apply patches/ship/03-devicen-fast.patch
git apply patches/ship/04-veloce-internal-access.patch
git apply patches/ship/05-veloce-skip-rasterization-probe.patch
git apply patches/ship/06-veloce-render-abort-probe.patch
git apply patches/ship/07-veloce-render-abort-deeper.patch
git apply patches/ship/08-load-page-with-classification.patch
git apply patches/ship/09-render-callbacks-scoped-cancel.patch
git apply patches/ship/0011-veloce-form-skip-mask.patch
git apply patches/ship/0012-veloce-image-skip-mask.patch
git apply patches/ship/0013-veloce-path-display-list-form.patch
git apply patches/ship/0014-veloce-path-display-list-cache.patch
git apply patches/ship/0015-veloce-path-display-list-noop-clip.patch
git apply patches/ship/0016-veloce-path-display-list-stacked-rect-noop-clip.patch
git apply patches/ship/0017-veloce-holder-root-page-path-display-list.patch
git apply patches/ship/0018-veloce-root-holder-context-layer.patch
git apply patches/ship/0019-veloce-progressive-root-path-display-list.patch
git apply patches/ship/0020-veloce-path-display-list-allow-fill-alpha.patch
git apply patches/ship/0021-veloce-path-display-list-stroke-replay.patch
git apply patches/ship/0022-veloce-path-display-list-split-transparency-rejects.patch
git apply patches/ship/0023-veloce-path-display-list-darken-blend-replay.patch
git apply patches/ship/0024-veloce-path-display-list-gate-dense-scratch-blend.patch
git apply patches/ship/0025-veloce-path-display-list-mask-blend-replay.patch
git apply patches/ship/0026-veloce-path-display-list-mask-blend-fill-stroke.patch
git apply patches/ship/0029-veloce-path-display-list-consecutive-run-blend.patch
git apply patches/ship/0030-veloce-path-display-list-run-correctness-fixes.patch
git apply patches/ship/0031-veloce-path-display-list-group-buffer-blend.patch
git apply patches/ship/0032-veloce-path-display-list-stroke-run-packing.patch
git apply patches/ship/0034-veloce-path-display-list-ordered-text-passthrough.patch
git apply patches/ship/0035-veloce-path-display-list-stroke-run-flush-telemetry.patch
git apply patches/ship/0036-veloce-path-display-list-nonoverlap-fill-barrier-stroke-packing.patch
git apply patches/ship/0037-veloce-path-display-list-holder-space-spatial-index.patch
git apply patches/ship/0038-veloce-path-display-list-disable-text-passthrough-cache.patch
git apply patches/ship/0039-veloce-path-display-list-text-passthrough-index-cache.patch
git apply patches/ship/0040-veloce-path-display-list-fill-barrier-telemetry.patch
git apply patches/ship/0041-veloce-path-display-list-blend-group-cancellation.patch
git apply patches/ship/0042-veloce-path-display-list-blend-run-widening-telemetry.patch
git apply patches/ship/0043-veloce-path-display-list-primitive-run-telemetry.patch
git apply patches/ship/0044-veloce-path-display-list-fill-barrier-proof-telemetry.patch
git apply patches/ship/0045-veloce-path-display-list-same-argb-fill-barrier-crossing.patch
git apply patches/ship/0046-veloce-path-display-list-stroke-run-compact-telemetry.patch
git apply patches/ship/0047-veloce-path-display-list-translation-normalized-stroke-run-packing.patch
git apply patches/ship/0048-veloce-path-display-list-safe-blend-paint-widening.patch
git apply patches/ship/0049-veloce-path-display-list-effective-blend-paint-widening.patch
git apply patches/ship/0050-veloce-path-display-list-blend-shape-telemetry.patch
git apply patches/ship/0051-veloce-page-dimensions-no-parse.patch
git apply patches/ship/0052-veloce-path-display-list-same-source-darken-widening.patch
git apply patches/ship/0053-veloce-render-plan-interface.patch
git apply patches/ship/0054-veloce-render-plan-skeleton.patch
git apply patches/ship/0055-veloce-render-plan-segmented-text-passthrough.patch
git apply patches/ship/0056-veloce-render-plan-bounded-cache.patch
git apply patches/ship/0057-veloce-render-plan-holder-space-spatial-index.patch
```

> **Note:** patches 0027 and 0028 are deprecated and must NOT be applied.
> They violate PDF painter order. Apply 0026 → 0029 directly.

Patches 06 and 07 depend on patch 05 and must be applied after it.
Patch 07 also depends on patch 06. Patch 08 depends on the fpdfview export
surface after patches 05 and 06. Patch 09 depends on patches 06 and 07 because
it reuses their render-abort checkpoints. Patch 0011 depends on patch 09
because it adds a sibling `CPDF_RenderStatus` snapshot and is authored against
the post-09 render-status layout. Patch 0012 depends on patch 0011: it adds a
second bit (`FPDFEX_RENDER_SKIP_HUGE_IMAGE`) and threshold setter to the same
`render_skip_mask_`/`veloce_render_skip_mask.{h,cpp}` surface that 0011
introduced, and is authored against the post-0011 `cpdf_renderstatus.{h,cpp}`
layout. Patch 0013 depends on patch 09's render callback feature flags and
patch 0012's post-skip-mask render-status layout. It adds a default-off
path-only Form XObject display-list fast path for Meta20-class workloads.
Patch 0014 depends on patch 0013 and avoids repeated compile cost for cached
eligible Forms. Patch 0015 depends on patch 0014 and avoids repeated clip setup
cost for path nodes whose rectangular clip contains the node bounds.
Patch 0016 depends on patch 0015 and handles inherited rectangle clip stacks
by intersecting their bounds before deciding whether the clip is no-op.
Patch 0017 depends on patch 0016 and extends the display-list path from Form
holders to root page holders. Patch 0018 depends on patch 0017 and corrects
the root holder gate to use render-context layer identity. Patch 0019 depends
on patch 0018 and adds the same root holder attempt to PDFium's progressive
root-page renderer. Patch 0020 depends on patch 0019 and narrows the
transparency reject rule so normal fill alpha remains eligible. Patch 0021
depends on patch 0020 and adds stroked-path replay using graph-state side
tables. Patch 0022 depends on patch 0021 and only splits reject telemetry.
Patch 0023 depends on patch 0022 and allows the specific `/BM /Darken` blend
mode by routing blended path nodes through temporary bitmap compositing while
leaving other blend modes and soft masks in the legacy fallback path.
Patch 0024 depends on patch 0023 and prevents dense root-page workloads from
using that temporary bitmap path when it is predictably slower than fallback.
Patch 0025 depends on patch 0024 and replaces the common fill-only blended
node case with 8-bit mask compositing through PDFium's scanline compositor.
Patch 0026 depends on patch 0025 and broadens that mask path to same-color
fill+stroke nodes while leaving different fill/stroke colors on fallback.
Patches 0027 and 0028 are deprecated (painter-order violation) and are
skipped by the release workflow. Do not apply them.
Patch 0029 depends on patch 0026 and replaces the r20 per-node mask path
with consecutive-run batching. Patch 0030 depends on patch 0029 and fixes
three correctness issues: a bbox overlap gate (P1), no flush on cancel (P2),
and a separate runFlushEnd counter for the end-of-loop drain (P3).
Patch 0031 depends on patch 0030 and replaces the 8-bit mask approach with
a BGRA group buffer matching MuPDF's transparency group model. The P1 bbox
overlap gate from 0030 is removed — overlapping paths accumulate correctly
inside the group buffer under normal blend before the single blend composite.
Patch 0075 starts a separate RenderProgram v2 continuation from the r25 + 0051
correctness baseline. Its workflow applies `01..09`, `0011..0026`,
`0029..0031`, `0051`, and `0075`; it deliberately excludes experimental
RenderPlan v1 patches `0053..0074`. Revision numbering continues for audit
history, but patch dependency does not. Patch 0075 only establishes immutable
holder ownership; patch 0076 will add parser-time recording.
Patch 0053 can also be applied directly on the rel-260701 stable line after
patch 0051. It is behavior-preserving and only inserts the `VeloceRenderPlan`
facade above the existing r25 path-display-list backend.
Patch 0054 depends on patch 0053 and adds only the ordered segment data model
and skeleton builder. The render facade deliberately does not call the builder
yet, so there is no per-render scan, allocation, or pixel behavior change.
Patch 0055 depends on patch 0054 and is the first segmented consumer. It
precompiles and validates every path-run segment before drawing, permits only
text passthrough in this phase, and returns `kNotEligible` before drawing for
images, Forms, shadings, blend barriers, unsupported barriers, or ambiguous
state. All-path holders keep the existing r25 cached whole-holder backend.
Patch 0056 depends on patch 0055 and caches only RenderPlan skeleton metadata.
It does not cache compiled path lists or page-object pointers; replay still
resolves indices against the live holder and preflights every PathRun before
drawing.
Patch 0057 depends on patch 0056 and adds only holder-space candidate selection
inside compiled path display lists. It is deliberately narrower than the
rolled-back r31/r40 spatial-index line: it does not introduce text passthrough
inside the path-display-list cache, stroke-run widening, fill-barrier crossing,
or blend widening. Broad clips fall back to the old scan, and every candidate
still passes the existing device-clip test before drawing.
Patch 0032 depends on patch 0031 and adds stroke-only run packing for the
normal-blend path. Consecutive stroke-only nodes are accumulated via
CFX_Path::Append only when they share paint and path matrix, then drawn with one
DrawPath call using that preserved matrix. This is safe for stroke-only paths
(no winding interaction) while preserving stroke CTM semantics for line width,
dash, and stroke adjustment, and is the primary optimization for CAD/DWG-export
PDFs with hundreds of thousands of single-line-segment stroke operations.
Patch 0033 is intentionally absent from the apply list. It was the reverted
group-buffer resolution-cap experiment and should remain reserved.
Patch 0034 depends on patch 0032 and adds ordered path/text segmentation.
Instead of rejecting an entire holder when a text object appears between path
runs, Veloce emits `PathRun` and text-passthrough segments and replays them in
painter order. The text passthrough calls PDFium's normal `RenderSingleObject()`
and then clears clip state so the next accelerated path segment installs its
own clip. Image, Form, and shading passthrough remain unsupported in this P1
patch and reject before drawing.
Patch 0035 depends on patch 0034 and is telemetry-only. It records why existing
stroke runs flush, including matrix/CTM changes, while preserving r28's hard
segment barriers and all replay ordering. Use it to decide whether the next
optimization should target color alternation, graph state/style variation,
matrix variation, fill-mode barriers, clip changes, blend barriers, segment
boundaries, or capacity.
Patch 0036 depends on patch 0035. It reduces fill-mode stroke-run fragmentation
without violating painter order by crossing only disjoint normal non-stroke path
barriers. The proof is device-space and conservative: expanded barrier bounds
must not intersect the expanded pending stroke-run bounds. Segment, text, clip,
blend, and overlapping fill barriers remain hard flush boundaries.
Patch 0037 depends on patch 0036. It adds a holder-space spatial index to the
cached native display list and uses it only to choose tile candidates before
the existing replay path. Candidate nodes are sorted by original node id before
replay, so painter order is preserved inside each `PathRun` segment. The index
is not built in device space and is not rebuilt per zoom. Broad preview clips
fall back to full sequential scan to avoid sort/dedup overhead when there is
little tile culling to gain.
Patch 0038 depends on patch 0037. It fixes the text-passthrough lifetime
contract introduced by ordered segmentation: text passthrough segments contain
raw live `CPDF_PageObject*` pointers, so lists with text passthrough are replayed
from the freshly compiled list for that render but are not retained in the
process cache. Path-only lists, including `11.pdf`, remain cacheable.
Patch 0039 depends on patch 0038. It replaces the temporary r32 cache disable
with pointer-free text passthrough replay tokens: cached segments store holder
object indexes, replay resolves those indexes from the live holder in one
forward pass before drawing, and cache insertion is restored for text-barrier
pages. If the current holder cannot resolve the expected text barrier indexes,
Veloce returns `not_eligible` before drawing.
Patch 0040 depends on patch 0039. It is telemetry-only and emits a compact
`VelocePathDLFill` line when normal non-stroke-only path barriers are
encountered during replay. It does not change path eligibility, replay order, stroke-run
packing, or fallback/cancel behavior.
Patch 0041 depends on patch 0040. It keeps successful `BlendGroupRun` output
unchanged, but checks `CPDF_RenderStatus::ShouldCancelRender()` inside the
group-buffer blend flush before allocation, around each group path draw, before
the destination composite, and after composite. If cancellation is observed,
Veloce returns `kCancelled`; the partially rendered tile is discarded by the
existing progressive render caller. Adds `blendCancel*` telemetry to the main
`VelocePathDL` line.
Patch 0042 depends on patch 0041 and is telemetry-only. It measures successful
`BlendGroupRun` cost in device pixels and records whether paint-change barriers
between adjacent blend runs are disjoint, overlapping, unknown, or same-blend
merge candidates. It emits a compact `VelocePathDLBlend` line for `11.pdf`-style
analysis. It does not change grouping, painter order, cancellation, or blend
semantics.
Patch 0043 depends on patch 0042 and is telemetry-only. It records compact
per-path primitive stats at display-list compile time, then reports how many
path points/subpaths were candidate, culled, and drawn during replay. It is the
measurement bridge for a future MuPDF-style primitive spatial index; replay
order, barrier semantics, stroke packing, blend grouping, cancellation, and
fallback behavior are unchanged.
Patch 0044 depends on patch 0043 and is telemetry-only. It classifies the
normal fill barriers that still block stroke-run packing, especially Q16-style
fill-only barriers, without changing crossing behavior. The invariant remains:
a later patch may cross a fill barrier only when the final pixels are provably
identical to original painter order.
Patch 0045 depends on patch 0044 and promotes the r40 same-color proof into a
strict behavior rule. Within the existing ordered segment and clip boundaries,
a fill-only normal source-over barrier may be drawn before the pending stroke
run only when its fill ARGB exactly matches the pending stroke ARGB. Same-source
source-over operations commute per pixel, including antialias coverage, so this
reduces fill-barrier stroke-run fragmentation without introducing page-specific
logic or crossing text, image, Form, clip, blend, pattern, soft-mask, or
fill+stroke barriers.
Patch 0046 depends on patch 0045 and is telemetry-only. It restores actual
stroke-run flush-reason accounting by recording the flush reason at the single
stroke-run drain point, emits a compact `VelocePathDLStroke` line that is not
hidden by the long main telemetry line, buckets stroke-run lengths, and splits
matrix flushes into same-linear/translation-only versus linear-change cases.
It does not change eligibility, draw order, segment boundaries, spatial-index
candidate selection, blend grouping, cancellation, or stroke packing behavior.
Patch 0047 depends on patch 0046 and promotes the r42 translation-only matrix
diagnosis into behavior. Within an existing ordered path segment, a pending
stroke-only normal-blend run may absorb a later stroke node with the same paint
key and identical matrix linear part by appending that node's path through a
relative coordinate-space translation and then drawing the whole packed path
with the base run matrix. This preserves stroke CTM semantics because stroke
width, dash, cap, join, miter, and stroke adjustment still see the same linear
matrix. Singular linear matrices, linear-part changes, paint/graph-state/style
changes, clips, blend nodes, text passthrough, non-stroke fills, image/Form
objects, and segment boundaries still flush. Temporary packed paths are capped
by point count and flush through the existing capacity reason, so the mechanism
is memory-bounded.
Patch 0048 depends on patch 0047 and promotes only the safe part of the r38
blend-run widening analysis. A pending `BlendGroupRun` may absorb an adjacent
different-paint blend node only when both nodes use the same final blend mode,
their clipped device rects are disjoint, and the union group rect does not add
more than 2x pixel area versus the two separate rects. Each group entry stores
its own paint key and draws with that paint inside the isolated BGRA group.
Overlapping different-paint blend nodes still flush because merging them into
one isolated group is not generally equivalent to per-object PDF blend order.
The mechanism stays inside the current ordered segment and clip run and does
not cross normal nodes, text passthrough, unsupported blend, image/Form, or
segment barriers.
Patch 0049 depends on patch 0048 and promotes a second safe blend-widening
case: overlapping paint-key changes whose render-affecting blend paint is
identical. The run tracks whether all accumulated entries still share one
effective paint in O(1) time. For fill-only blend paths, `adjust_stroke` is
ignored in the effective-paint comparison because `DrawPath()` does not draw a
stroke. Fill type, anti-aliasing, fill ARGB, stroke ARGB, blend mode, and
stroke graph state remain part of the comparison where they affect pixels.
Non-equivalent overlapping paint changes still flush, and r44's disjoint
bounded crossing rule remains unchanged.
Patch 0050 depends on patch 0049 and is telemetry-only. It classifies the
already-flushed `BlendGroupRun` composites by final blend mode, fill/stroke
paint shape, same source ARGB, and simple Darken candidate pixel cost through
`VelocePathDLBlendShape`. It piggybacks on the existing per-entry stats loop
after a group composite and does not change grouping, painter order,
cancellation, allocation, or rendering behavior.
Patch 0051 depends on the public extension surface established by patch 08 and
is independent of the path display-list renderer. It exposes dictionary-only
page geometry through `FPDFEx_GetPageDimensions()` and intentionally does not
call `ParseContent()`, `AddPageImageCache()`, or any render entry point. The
app/JNI layer must resolve this symbol optionally and fall back to the existing
slow `FPDF_LoadPage()`-based page-size path when the symbol is absent.
Patch 0052 depends on patch 0050 and promotes the same-source Darken proof into
behavior. A pending `BlendGroupRun` may absorb an adjacent paint-key change
when the pending run is entirely `/BM /Darken`, every accumulated entry has one
source ARGB, the next Darken node has that same source ARGB, and the union
device rect remains bounded by the existing 2x sparse-area policy. This is an
ordered-segment optimization only: normal objects, text passthrough, clip
changes, scratch/unsupported blends, image/Form/shading objects, cancellation,
and segment boundaries still flush. Each entry keeps its own paint key, path
matrix, graph state, and fill/stroke options when rasterized inside the BGRA
group.
The release workflow applies the numeric order shown above, skipping 0027/0028.

## Cumulative effect (measured)

On a representative image-heavy picture-book PDF, applying patches 01-03:

| Phase | Per-page render | Δ |
|---|---|---|
| Baseline (vanilla PDFium HEAD) | ~5,000 ms | — |
| With patches 01-03 | ~135 ms | **~37× faster** |

Visual quality: bit-identical output for patches 02 and 03; libjpeg
scale_denom path (patch 01) produces standard downsampled output per
the documented libjpeg API. Patch 04 does not affect rendering output.
Patch 05 does not affect rendering output unless its diagnostic toggle is
explicitly enabled. Patches 06 and 07 do not affect rendering output unless
the abort flag is explicitly enabled. Patch 08 does not alter
`FPDF_LoadPage()` behavior; callers must opt in by resolving and calling the
new `FPDFEx_LoadPageWithClassification()` symbol.
Patch 0051 also does not affect rendering output; callers must opt in by
resolving and calling `FPDFEx_GetPageDimensions()` for page-size enumeration.
Patch 0052 changes only the grouping of already eligible same-source Darken
blend paths inside one ordered segment/clip run. It preserves painter-order
barriers and rasterizes each entry with its original paint state inside the
isolated group.

## Files changed by each patch

- **01 JPEG downscale**: `core/fpdfapi/page/cpdf_dib.{h,cpp}`,
  `core/fpdfapi/page/cpdf_streamparser.cpp`,
  `core/fxcodec/jpeg/jpegmodule.{h,cpp}`,
  `BUILD.gn`.
- **02 IndexedCS + SeparationCS**: `core/fpdfapi/page/cpdf_indexedcs.{h,cpp}`,
  `core/fpdfapi/page/cpdf_colorspace.cpp` (SeparationCS only).
- **03 DeviceNCS**: `core/fpdfapi/page/cpdf_colorspace.cpp` (DeviceNCS only).
- **04 Veloce internal access**: `public/fpdf_edit.h`,
  `fpdfsdk/fpdf_editpage.cpp`.
- **05 Veloce skip rasterization probe**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`,
  `core/fpdfapi/render/veloce_skip_rasterization.h`,
  `fpdfsdk/fpdf_view.cpp`,
  `public/fpdfview.h`.
- **06 Veloce render abort probe**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`,
  `core/fpdfapi/render/veloce_render_abort.h`,
  `fpdfsdk/fpdf_view.cpp`,
  `public/fpdfview.h`.
- **07 Veloce deeper render abort checkpoints**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`.
- **08 Veloce load-page classification**:
  `fpdfsdk/fpdf_view.cpp`, `public/fpdfview.h`.
- **09 Veloce scoped render callbacks**:
  `BUILD.gn`, `core/fpdfapi/render/BUILD.gn`,
  `core/fpdfapi/render/cpdf_renderstatus.{h,cpp}`,
  `core/fpdfapi/render/veloce_render_callbacks.{h,cpp}`,
  `fpdfsdk/BUILD.gn`, `fpdfsdk/fpdfex_render_callbacks.cpp`,
  `public/fpdfex_render_callbacks.h`.
- **0011 Veloce form skip mask**:
  `BUILD.gn`, `core/fpdfapi/render/BUILD.gn`,
  `core/fpdfapi/render/cpdf_renderstatus.{h,cpp}`,
  `core/fpdfapi/render/veloce_render_skip_mask.{h,cpp}`,
  `fpdfsdk/BUILD.gn`, `fpdfsdk/fpdfex_render_skip_mask.cpp`,
  `public/fpdfex_render_skip_mask.h`.
- **0012 Veloce image skip mask**:
  `core/fpdfapi/render/cpdf_renderstatus.{h,cpp}`,
  `core/fpdfapi/render/veloce_render_skip_mask.{h,cpp}`,
  `fpdfsdk/fpdfex_render_skip_mask.cpp`,
  `public/fpdfex_render_skip_mask.h`.
  No `BUILD.gn` changes — all touched files were already registered as build
  sources by patch 0011.
- **0013 Veloce path display-list Form fast path**:
  `core/fpdfapi/render/BUILD.gn`,
  `core/fpdfapi/render/cpdf_renderstatus.{h,cpp}`,
  `core/fpdfapi/render/veloce_path_display_list.{h,cpp}`,
  `public/fpdfex_render_callbacks.h`.
  The implementation is isolated in separate display-list files and is gated
  by `FPDFEX_FEATURE_PATH_DISPLAY_LIST_FORM`.
- **0014 Veloce path display-list cache**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds a bounded internal cache and `cache=hit|miss` telemetry. No public API
  or build file changes.
- **0015 Veloce path display-list no-op clips**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds no-op rectangular clip normalization and `noopClips` telemetry. No
  public API or build file changes.
- **0016 Veloce path display-list stacked rectangle no-op clips**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Extends no-op clip normalization to inherited rectangle clip stacks. No
  public API or build file changes.
- **0034 Veloce ordered text passthrough**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds ordered `PathRun` / text-passthrough segments, text barrier replay via
  `RenderSingleObject()`, and segment/passthrough telemetry. No public API or
  build file changes.
- **0035 Veloce stroke-run flush telemetry**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds stroke-run flush reason counters only. No rendering behavior, public
  API, or build file changes.
- **0036 Veloce non-overlap fill-barrier stroke packing**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Allows stroke runs to cross disjoint normal non-stroke barriers, with
  crossed/blocked telemetry. No public API or build file changes.
- **0037 Veloce holder-space spatial index**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds cached holder-space binning, tile candidate query, ordered candidate
  replay, and `spatialIndex*` telemetry. No public API or build file changes.
- **0038 Veloce text-passthrough cache lifetime fix**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Skips process-cache insertion for display lists containing text passthrough
  raw page-object pointers. No public API or build file changes.
- **0039 Veloce text-passthrough index cache fix**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Stores text passthrough holder object indexes in cached display lists,
  resolves live text objects at replay, and restores process-cache insertion
  for text-passthrough lists. No public API or build file changes.
- **0040 Veloce fill-barrier telemetry**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds compact telemetry for fill-barrier shape and pending-stroke overlap
  classification. No rendering behavior, public API, or build file changes.
- **0041 Veloce blend group cooperative cancellation**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds cancellation checkpoints and `blendCancel*` telemetry inside group-buffer
  blend flushes. Successful render output is unchanged; cancelled replay returns
  `kCancelled`. No public API or build file changes.
- **0046 Veloce stroke-run compact telemetry**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds compact `VelocePathDLStroke` logging, real stroke-run flush reason
  recording, run-length buckets, and matrix-flush classification. No rendering
  behavior, public API, or build file changes.
- **0047 Veloce translation-normalized stroke-run packing**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Packs same-linear translation-only stroke matrices into the existing stroke
  run by appending paths through a relative translation. Adds point-cap
  telemetry and `matrixPacked` counters. No public API or build file changes.
- **0048 Veloce safe blend paint widening**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Allows disjoint, bounded same-blend paint changes inside a `BlendGroupRun`
  while drawing every entry with its own paint. Adds `paintSwitches` and
  `maxPaints` telemetry. No public API or build file changes.
- **0049 Veloce effective blend paint widening**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Allows overlapping blend paint-key changes inside a `BlendGroupRun` only when
  the effective render paint is equivalent for all entries. Adds
  `paintEquivalent` and `paintEquivalentCrossed` telemetry. No public API or
  build file changes.
- **0050 Veloce blend shape telemetry**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds compact `VelocePathDLBlendShape` telemetry for blend mode, path paint
  shape, same source ARGB, and simple Darken candidate cost. No rendering
  behavior, public API, or build file changes.
- **0051 Veloce page dimensions no-parse export**:
  `fpdfsdk/fpdf_view.cpp`, `fpdfsdk/fpdf_view_c_api_test.c`,
  `public/fpdfview.h`.
  Adds fixed 32-byte `FPDFEx_PageDimensions` and
  `FPDFEx_GetPageDimensions()` for dictionary-only page geometry lookup. No
  rendering behavior or path display-list behavior changes.
- **0052 Veloce same-source Darken widening**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds run-level source-ARGB tracking and allows bounded same-source Darken
  paint-key changes to remain in one `BlendGroupRun`, reducing repeated group
  composites on blend-heavy path pages. No public API or build file changes.
- **0053 Veloce RenderPlan holder interface**:
  `core/fpdfapi/render/BUILD.gn`,
  `core/fpdfapi/render/cpdf_progressiverenderer.cpp`,
  `core/fpdfapi/render/cpdf_renderstatus.cpp`,
  `core/fpdfapi/render/veloce_render_plan.{h,cpp}`.
  Adds the renderer-facing Veloce RenderPlan facade and delegates to the
  existing r25 path-display-list backend. Behavior-preserving; no public API
  changes.
- **0054 Veloce RenderPlan ordered segment skeleton**:
  `core/fpdfapi/render/veloce_render_plan.{h,cpp}`.
  Adds ordered segment data structures and a builder that emits path runs plus
  passthrough barriers by holder object index. Behavior-preserving; the builder
  is not called by the render facade yet and no public API changes.
- **0055 Veloce RenderPlan segmented text passthrough**:
  `core/fpdfapi/render/veloce_path_display_list.{h,cpp}`,
  `core/fpdfapi/render/veloce_render_plan.cpp`.
  Adds object-range path display-list compile/replay APIs and consumes ordered
  path/text segments in the RenderPlan facade. Fails closed before drawing for
  unsupported passthrough or replay requirements; no public API changes.
- **0056 Veloce RenderPlan bounded cache**:
  `core/fpdfapi/render/veloce_render_plan.cpp`.
  Adds a bounded cache for immutable RenderPlan skeleton metadata only. The
  cache stores no page-object pointers and no compiled display-list handles; no
  public API changes.
- **0057 Veloce holder-space spatial index**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds bounded holder-space grid candidate selection for large compiled path
  lists. Replay order and rendering semantics remain owned by the existing
  path-display-list replay body; no public API changes.
- **0058 Veloce RenderPlan facade telemetry**:
  `core/fpdfapi/render/veloce_render_plan.cpp`.
  Adds Android-only `VeloceRenderPlan` facade logs that distinguish segmented
  RenderPlan execution from legacy holder fallback, including plan shape and
  holder kind. Behavior-preserving; no public API changes.
- **0059 Veloce RenderPlan whole-path-run backend**:
  `core/fpdfapi/render/veloce_render_plan.cpp`.
  Makes single-`PathRun` plans use the existing cached whole-holder path
  display-list implementation as the RenderPlan `whole_path_run` backend,
  leaving only non-single-PathRun compatibility fallback under
  `legacy_holder_fallback`. Behavior-preserving; no public API changes.
- **0017 Veloce holder-level root page path display list**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`,
  `core/fpdfapi/render/veloce_path_display_list.{h,cpp}`.
  Adds the root-page `RenderObjectList()` hook and holder-kind telemetry. No
  public API or build file changes.
- **0018 Veloce root holder context layer gate**:
  `core/fpdfapi/render/cpdf_renderstatus.cpp`.
  Changes the root-page hook gate from `IsPage()` to render-context layer
  holder identity. No public API or build file changes.
- **0019 Veloce progressive root path display list**:
  `core/fpdfapi/render/cpdf_progressiverenderer.cpp`.
  Adds the root-page display-list attempt before the progressive renderer's
  root page-object loop. No public API or build file changes.
- **0020 Veloce path display-list fill alpha eligibility**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Allows normal fill alpha because replay uses `GetFillArgb()` and
  `DrawPath()`, while keeping soft-mask and non-normal blend fallback. No
  public API or build file changes.
- **0021 Veloce path display-list stroke replay**:
  `core/fpdfapi/render/cpdf_renderstatus.h`,
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Adds stroke colors plus deduplicated `CFX_GraphStateData` side-table replay,
  and removes the incorrect stroke-alpha-as-transparency reject. No public API
  or build file changes.
- **0022 Veloce path display-list reject diagnostics**:
  `core/fpdfapi/render/veloce_path_display_list.cpp`.
  Splits broad transparency rejection into `soft_mask` and `blend_mode`
  telemetry. No rendering behavior, public API, or build file changes.
- **0023 Veloce path display-list Darken blend replay**:
  `core/fpdfapi/render/veloce_path_display_list.{h,cpp}`.
  Allows `BlendMode::kDarken` in eligibility, stores blend mode in the paint
  side table, logs `blendNodes`, and replays blended path nodes through a
  temporary BGRA render device plus `SetDIBitsWithBlend()`. No public API or
  build file changes.

Patches 02 and 03 both touch
`cpdf_colorspace.cpp`, but in different sections (SeparationCS vs.
DeviceNCS), so they coexist cleanly.
Patches 05, 06, and 07 touch `cpdf_renderstatus.cpp`. Patch 06 is authored
against the post-05 source, and patch 07 is authored against the post-06
source. Patch 05 skips rasterization for measurement; patch 06 exposes the
abort flag and checks top-level object boundaries; patch 07 checks deeper
phase boundaries inside `RenderSingleObject()`.
Patch 08 touches `fpdf_view.cpp` and `fpdfview.h`; it is authored after the
Veloce fpdfview exports from patches 05 and 06.
Patch 09 and 0011 both extend the render-status surface and add public
extension headers/symbols. Patch 0011 is authored after patch 09.
Patch 0012 extends 0011's skip-mask surface with a second bit
(`FPDFEX_RENDER_SKIP_HUGE_IMAGE`) for per-image (rather than per-Form)
deferral, and is authored after 0011.
Patch 0013 extends patch 09's callback feature flags with
`FPDFEX_FEATURE_PATH_DISPLAY_LIST_FORM` and hooks only `ProcessForm()`. If a
Form contains anything outside the v1 eligibility envelope (non-path objects,
strokes, pattern paint, transparency, complex clips, or forced-color render
mode), it returns to the existing `RenderObjectList()` path before drawing.
Patch 0014 is authored after 0013 and only changes
`veloce_path_display_list.cpp`; it keeps the same feature flag and fallback
contract while avoiding repeated compile cost for cached eligible Forms.
Patch 0015 is authored after 0014 and only changes
`veloce_path_display_list.cpp`; it keeps the same feature flag and fallback
contract while avoiding repeated clip setup cost for path nodes whose clip
rectangle contains the node bounds.
Patch 0016 is authored after 0015 and only changes
`veloce_path_display_list.cpp`; it keeps the same feature flag and fallback
contract while handling multi-component rectangle clips created by inherited
Form/page clipping.
Patch 0017 is authored after 0016 and changes `cpdf_renderstatus.cpp` plus
`veloce_path_display_list.{h,cpp}`; it keeps the same feature flag and
all-or-nothing fallback contract while extending the holder fast path from
Form holders to root page holders.
Patch 0018 is authored after 0017 and changes only `cpdf_renderstatus.cpp`;
it keeps the same all-or-nothing fallback contract while making the root
holder check match the actual `CPDF_RenderContext::Render()` layer boundary.
Patch 0019 is authored after 0018 and changes only
`cpdf_progressiverenderer.cpp`; it keeps the same all-or-nothing fallback
contract while covering the Android `FPDF_RenderPageBitmap()` root render path.
Patch 0020 is authored after 0019 and changes only
`veloce_path_display_list.cpp`; it keeps the all-or-nothing fallback contract
for real unsupported transparency while allowing ordinary fill alpha.
Patch 0021 is authored after 0020 and changes only
`cpdf_renderstatus.h` plus `veloce_path_display_list.cpp`; it keeps the same
fallback contract while allowing dense path-only pages whose paths are stroked.
Patch 0022 is authored after 0021 and changes only
`veloce_path_display_list.cpp`; it preserves fallback behavior while making
the next unsupported feature visible in logs.
Patch 0023 is authored after 0022 and changes only
`veloce_path_display_list.{h,cpp}`; it keeps `kNotEligible` as a pre-draw
fallback result and returns `kCancelled` for any replay-time blend failure so
callers do not draw legacy content over partial Veloce output.

## Upstreaming

Patches 01-03 are independently suitable for upstream PRs to the PDFium
project. Patch 04 is intentionally xPDFSDK/Veloce-specific because it
exports internal PDFium state for a local rendering accelerator.
Patches 05, 06, and 07 are intentionally xPDFSDK/Veloce-specific and
diagnostic-only.

The patches in `experiments/` (0001–0009) are **NOT** upstreamable —
they include Android-specific ATrace dependencies and ad hoc diagnostic
debug toggles. The Veloce workflow uses this `ship/` directory because
patch 05 is a narrow, named, default-off probe with explicit artifact
provenance and symbol verification.
