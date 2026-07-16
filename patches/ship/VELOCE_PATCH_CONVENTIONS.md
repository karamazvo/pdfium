# PDFium Veloce Patch Conventions Handoff

Date: 2026-07-16

Current architecture and performance plan:

```text
/Users/shchao/Code/xPDFSDK/android/meta/libs/pdfium/patches/ship/UNIFIED_RENDER_PROGRAM_BACKEND_PLAN_2026-07-16.md
```

The r25-0074 measurements proved that dispatch batching was not the dense-tile
bottleneck. r25-0078 is now the frozen legacy-consumer reference build. The new
native line is **r25-1**, unified-backend generation 1. It starts from the r25 +
0051 correctness baseline and retains only the RenderProgram ownership/order
foundation in 0075-0076. It deliberately excludes experimental RenderPlan v1
patches 0053-0074 and the legacy path-display-list consumers 0077-0078.

Global patch numbering remains monotonic for audit history. The first r25-1
revision is `r25-1-0079`; no old or failed number is reused.

Purpose: shared context for Codex and Claude when continuing the native PDFium
Veloce performance patch stack. This document records the current worktree
layout, patch naming rules, workflow conventions, architecture invariants, and
current revision status so the next patch can be contributed without reopening
old decisions.

## 1. Repository And Worktree Layout

Native PDFium patch repo:

```text
/Users/shchao/Code/xPDFSDK/android/meta/libs/pdfium
```

This repo owns:

- `patches/ship/*.patch`
- `.github/workflows/pdfium-android-arm64-rXX-*.yml`
- patch-stack documentation in `patches/ship/README.md`
- commits for native PDFium patch revisions

Android app, JNI, Kotlin, and UI layer worktree:

```text
/Users/shchao/Code/xPDFSDK.worktrees/codex-use-r25-0053-pdfium/android
```

Use this worktree only for app/JNI/Kotlin changes. Do not mix app-layer changes
into the PDFium patch repo.

Current committed native PDFium reference HEAD:

```text
5a12ae941 r25-1-0082 add bounded holder-space candidate index
```

Known untracked files currently visible in the native PDFium repo include
generated source snapshots. They are intentionally not staged as generated
sources:

```text
core/fpdfapi/render/veloce_path_display_list.cpp
core/fpdfapi/render/veloce_path_display_list.h
core/fpdfapi/render/veloce_render_*.{h,cpp}
fpdfsdk/fpdfex_render_*.cpp
public/fpdfex_render_*.h
```

Do not stage the generated source snapshots.

The earlier page-dimensions proposal draft, `10-page-dimensions-no-parse.patch`,
has been deleted now that it was superseded by the adopted, fully-wired
`0051-veloce-page-dimensions-no-parse.patch` (in `README.md`'s patch table,
apply-sequence, and changelog). It no longer exists in this repo and should
not be referenced as provenance — `0051` is the sole source of truth for this
feature going forward.

## 2. Patch Naming And Revision Rules

Native patch files live under:

```text
patches/ship/
```

Use numeric patch order for the patch stack:

```text
0049-veloce-path-display-list-effective-blend-paint-widening.patch
0050-veloce-path-display-list-blend-shape-telemetry.patch
0051-veloce-page-dimensions-no-parse.patch
0052-veloce-path-display-list-same-source-darken-widening.patch
```

The unified-backend series uses this complete revision identity:

```text
r25-1-0079
|  |   |
|  |   +-- global monotonic patch number
|  +------ unified-backend generation 1
+--------- r25 correctness baseline
```

`r25-1` is a Veloce series identifier, not a PDFium upstream version number.
Use the full identity consistently:

```text
patch file:       0079-veloce-unified-render-program-backend-interface.patch
patch subject:    [PATCH] r25-1-0079: add unified RenderProgram backend interface
workflow file:    pdfium-android-arm64-r25-1-0079-unified-render-program-backend.yml
workflow name:    r25-1-0079 unified RenderProgram backend - Build libpdfium Android arm64
artifact prefix:  libpdfium-android-arm64-r25-1-0079-
runtime revision: r25-1-0079
```

Workflow and action names must begin with the complete revision so the newest
series build is immediately visible. Later revisions advance globally:

```text
r25-1-0080
r25-1-0081
r25-1-0082
r25-1-0083
```

Do not rename, delete, or reuse 0077/0078. They remain historical evidence for
the legacy consumer even though the r25-1 workflow does not apply them.

Use revision labels for user/build tracking:

```text
r45 effective blend paint widening
r46 blend shape telemetry
r47 page dimensions no parse
r48 same-source Darken widening
```

The r48 native patch after r47 is:

```text
patch file: patches/ship/0052-veloce-path-display-list-same-source-darken-widening.patch
workflow:   .github/workflows/pdfium-android-arm64-r48-same-source-darken-widening-path-display-list.yml
commit:     Add r48 same-source Darken widening patch
```

Keep deprecated patch numbers and names in place. In particular:

- `0027` is deprecated and must not be applied.
- `0028` is deprecated and must not be applied.
- Workflows intentionally apply `0026 -> 0029` directly.
- Do not rename old patch files just to make names prettier. Stable names are
  more important than perfect taxonomy.

Workflow `name:` and artifact path must start with the revision label:

```yaml
name: r46 <revision name> path display-list - Build patched PDFium Android arm64
DIST_DIR: .../libpdfium-android-arm64-r46-<revision-name>-path-display-list
```

This is required so the GitHub Actions UI shows the new revision immediately.

## 3. Patch Authoring Principles

The rule is mechanism first, file-specific behavior never.

Do:

- Solve a class of rendering workloads, not a single PDF file.
- Preserve PDF painter order unless there is a proof that operations commute.
- Keep segment boundaries, text passthrough, clip changes, unsupported blend,
  image/Form/shading, and normal-object barriers as hard barriers unless a patch
  explicitly proves a narrower crossing rule.
- Keep changes memory-bounded.
- Keep replay visible-first where possible.
- Prefer telemetry before behavior when the cost model is unclear.
- Add compact Android log lines when long `VelocePathDL` lines can be truncated.
- Add workflow guards that prove the new mechanism compiled in and old unsafe
  mechanisms did not reappear.

Do not:

- Add a fast path for `11.pdf`, `Q16*.pdf`, or `error.pdf` by filename or
  document-specific shape.
- Reintroduce Kotlin `classifyAllPages`; huge path detection and acceleration
  moved into native PDFium/Veloce.
- Add UI-thread zoom-continuity hacks to compensate for native render cost.
  A previous snapshot/continuity framework created visual artifacts and was
  reverted conceptually.
- Merge overlapping different-paint blend objects into one group unless there
  is a formal pixel-equivalence proof.
- Store raw `CPDF_PageObject*` pointers in cached display lists. Use holder
  object indexes and resolve live objects at replay time.
- Cross segment or passthrough barriers for batching.

## 4. Current Architecture Invariants

The r25-1 central architecture is one holder-owned ordered RenderProgram:

```text
live PDFium holder and editing objects
              |
              v
immutable ordered RenderProgram
              |
     bounded holder-space index
              |
       visible candidates only
              |
 exact bounded execution packets
              |
     current PDFium destination bitmap
```

This is one framework for normal, mixed-content, and huge-path holders. It has
no filename/page classification and no separate huge-path scheduler. Command
order, clip/group nesting, candidate selection, execution, cancellation, and
canonical fallback are represented by the same program contract.

Non-negotiable boundaries:

- PDFium page objects remain the fidelity/editing source of truth.
- The RenderProgram is immutable, holder-owned, and stores no long-lived raw
  page-object pointer.
- The complete ordered program is validated before any accelerated pixel is
  drawn.
- Candidate selection may remove only provably invisible commands and must
  return surviving commands in original painter order.
- Unsupported commands are ordered barriers or cause pre-draw canonical
  fallback; they do not create a second page model.
- The app remains the sole visible-region scheduler. Native code executes one
  requested bitmap/clip and never adds warmup or speculative rendering.
- Program, index, compiled ranges, caches, packets, and scratch storage have
  explicit byte/count ceilings.
- Compilation and replay run off the UI thread, outside cache locks, with
  bounded cancellation checkpoints.
- Normal and huge-path content use the same interface. Native structural cost
  and equivalence determine execution, never Kotlin classification.

The r25-0078 cached path display list remains a comparison implementation, not
the r25-1 source of truth. Useful algorithms may be re-derived behind this
single RenderProgram contract, but its legacy segmented backend must not be
installed as a parallel execution architecture.

## 5. Current Revision Status

Release workflows:

| Release | Workflow | Patch Contract | Purpose |
| --- | --- | --- | --- |
| rel-260701 | `.github/workflows/pdfium-android-arm64-rel-260701-r25-page-dimensions.yml` | r25 rendering stack (`01..09`, `0011..0026`, `0029..0031`) plus `0051` only | Correctness-stable release candidate: restore the last validated rendering behavior while keeping the no-parse page dimensions API. Excludes post-r25 render-behavior patches such as ordered text passthrough, spatial index, stroke-run widening, and blend widening. |
| r25-0075 | `.github/workflows/pdfium-android-arm64-r25-0075-render-program-v2-ownership-boundary.yml` | r25 rendering stack plus `0051` and `0075`; excludes `0053..0074` | Behavior-neutral start of RenderProgram v2. Adds holder-owned immutable program lifetime only; recording and replay remain absent. |
| r25-0076 | `.github/workflows/pdfium-android-arm64-r25-0076-render-program-parser-command-order.yml` | r25 rendering stack plus `0051`, `0075`, and `0076`; excludes `0053..0074` | Parser-time recording only. Stores one command-kind byte per object in exact painter order, bounded to 32 MiB, and seals it under holder ownership. No renderer consumes it yet. |
| r25-0077 | `.github/workflows/pdfium-android-arm64-r25-0077-render-program-exact-path-gate.yml` | r25 rendering stack plus `0051` and `0075..0077`; excludes `0053..0074` | First v2 consumer. Uses exact parser summaries to reject absent, stale-count, or mixed programs before legacy cache lookup/compile; all-path backend remains unchanged. |
| r25-0078 | `.github/workflows/pdfium-android-arm64-r25-0078-render-program-ordered-text-segments.yml` | r25 rendering stack plus `0051` and `0075..0078`; excludes `0053..0074` | First ordered mixed-command replay. Compiles path runs once, keeps text as live holder-index barriers, preserves painter order and clip state, and rejects image/Form/shading/unsupported streams before drawing. |
| r25-1-0079 | `.github/workflows/pdfium-android-arm64-r25-1-0079-unified-render-program-backend.yml` | r25 rendering stack plus `0051`, `0075`, `0076`, and `0079`; excludes `0053..0074` and `0077..0078` | Adds a behavior-neutral unified interface, but the artifact still exposes the older `0013..0031` executor when its feature bit is supplied. It is not a canonical-pixel baseline. |
| r25-1-0080 | `.github/workflows/pdfium-android-arm64-r25-1-0080-compact-command-summary.yml` | r25-1-0079 plus `0080`; excludes `0053..0074` and `0077..0078` | Adds fixed O(1) command-kind summaries and keeps the unified backend disabled. The older executor remains exposed, so this is also not a canonical-pixel baseline. |
| r25-1-0081 | `.github/workflows/pdfium-android-arm64-r25-1-0081-canonical-correctness-baseline.yml` | r25-1-0080 plus `0081`; excludes `0053..0074` and `0077..0078` | Disables the older `0013..0031` holder executor before cache/compile/draw and keeps the unified backend disabled, making canonical PDFium the sole pixel owner. |
| r25-1-0082 | `.github/workflows/pdfium-android-arm64-r25-1-0082-holder-space-candidate-index.yml` | r25-1-0081 plus `0082`; excludes `0053..0074` and `0077..0078` | Adds bounded immutable holder-space candidate metadata to huge RenderPrograms while keeping both accelerated executors disabled and canonical PDFium as sole pixel owner. |
| r25-1-0083 | `.github/workflows/pdfium-android-arm64-r25-1-0083-exact-path-text-candidate-executor.yml` | r25-1-0082 plus `0083`; excludes `0053..0074` and `0077..0078` | Enables the single unified boundary only for complete indexed path/text holders. Candidate omission is conservative, replay calls canonical PDFium object rendering, and dense/edited/unsupported requests fail closed before drawing. |

Recent revisions:

| Revision | Patch | Status | Purpose |
| --- | --- | --- | --- |
| r39 | `0043-veloce-path-display-list-primitive-run-telemetry.patch` | committed | Primitive/run telemetry for dense tile analysis. |
| r40 | `0044-veloce-path-display-list-fill-barrier-proof-telemetry.patch` | committed | Proof telemetry for fill barriers blocking stroke runs. |
| r41 | `0045-veloce-path-display-list-same-argb-fill-barrier-crossing.patch` | committed | Cross same-ARGB fill-only barriers for normal stroke runs. |
| r42 | `0046-veloce-path-display-list-stroke-run-compact-telemetry.patch` | committed | Compact stroke telemetry and restored flush accounting. |
| r43 | `0047-veloce-path-display-list-translation-normalized-stroke-run-packing.patch` | committed | Pack same-linear translation-only stroke matrices. |
| r44 | `0048-veloce-path-display-list-safe-blend-paint-widening.patch` | committed | Conservative disjoint same-blend paint widening for blend groups. |
| r45 | `0049-veloce-path-display-list-effective-blend-paint-widening.patch` | committed | Overlapping blend paint-key widening when effective render paint is equivalent. |
| r46 | `0050-veloce-path-display-list-blend-shape-telemetry.patch` | committed | Telemetry-only blend shape classification for future direct/simple Darken proof. |
| r47 | `0051-veloce-page-dimensions-no-parse.patch` | committed | Export dictionary-only page geometry for app/JNI page-size fast path. |
| r48 | `0052-veloce-path-display-list-same-source-darken-widening.patch` | in progress | Widen bounded same-source Darken blend groups inside ordered segment/clip barriers. |
| r25-0053 | `0053-veloce-render-plan-interface.patch` | committed | Behavior-preserving RenderPlan holder facade on the r25 + 0051 stable line. |
| r25-0054 | `0054-veloce-render-plan-skeleton.patch` | committed | Behavior-preserving ordered RenderPlan segment data model and skeleton builder. |
| r25-0055 | `0055-veloce-render-plan-segmented-text-passthrough.patch` | committed | Consume ordered RenderPlan path/text segments with preflighted range path display-list replay. |
| r25-0056 | `0056-veloce-render-plan-bounded-cache.patch` | committed | Cache immutable RenderPlan segment metadata with bounded LRU; no raw page-object pointers or compiled path-list handles. |
| r25-0057 | `0057-veloce-render-plan-holder-space-spatial-index.patch` | in progress | Add bounded holder-space candidate selection inside compiled PathRun replay. It preserves original node order, never crosses RenderPlan barriers, and falls back to full scan for broad clips or unsafe transforms. |
| r25-0075 | `0075-veloce-render-program-v2-ownership-boundary.patch` | committed | Start the clean RenderProgram v2 line from r25 + 0051 with immutable holder ownership and no runtime behavior change. |
| r25-0076 | `0076-veloce-render-program-parser-command-order.patch` | committed | Record bounded compact object-kind commands during the existing parser append path, seal exact painter order after parse completion, and keep replay disabled. |
| r25-0077 | `0077-veloce-render-program-exact-path-gate.patch` | committed | Gate the legacy path cache/compiler with exact parser-owned program presence, holder count, and all-path summary before any scan or allocation. |
| r25-0078 | `0078-veloce-render-program-ordered-text-segments.patch` | committed, frozen reference | Replay exact path/text programs as bounded ordered path runs and live text barriers without raw cached page-object pointers. |
| r25-1-0079 | `0079-veloce-unified-render-program-backend-interface.patch` | committed; native build/link verified; not a correctness baseline | Start the unified backend contract. Its own executor is disabled, but the artifact still permits the older holder executor. The first workflow run reached packaging; commit `48be9f3f3` fixed that packaging-only glob error. |
| r25-1-0080 | `0080-veloce-render-program-compact-command-summary.patch` | implemented; not a correctness baseline | Add fixed O(1) command-kind counts during the existing parser append and retain implicit live-object state identity. The unified executor is disabled, but the old holder executor remains available. |
| r25-1-0081 | `0081-veloce-disable-legacy-path-display-list.patch` | implemented, pending build | Establish the canonical correctness A/B baseline by disabling both old and new accelerated executors before any destination mutation. |
| r25-1-0082 | `0082-veloce-render-program-holder-space-candidate-index.patch` | implemented, pending build | Build a bounded ordered candidate index only for huge holders, with uncertain commands always replayed and no runtime consumer or pixel behavior change. |
| r25-1-0083 | `0083-veloce-render-program-exact-path-text-executor.patch` | implemented, pending build | Consume the one index for sparse complete path/text holders, validate candidates before drawing, replay with canonical `RenderSingleObject()`, and invalidate stale spatial metadata on owned object mutation. |

Historical native HEAD before r48 (not the active line):

```text
41ffb668d Fix r47 workflow page dimensions guard
```

r48 historical files:

```text
patches/ship/0050-veloce-path-display-list-blend-shape-telemetry.patch
.github/workflows/pdfium-android-arm64-r46-blend-shape-telemetry-path-display-list.yml
patches/ship/0051-veloce-page-dimensions-no-parse.patch
.github/workflows/pdfium-android-arm64-r47-page-dimensions-no-parse-path-display-list.yml
patches/ship/0052-veloce-path-display-list-same-source-darken-widening.patch
.github/workflows/pdfium-android-arm64-r48-same-source-darken-widening-path-display-list.yml
```

r47 is not a render-behavior patch. It adds `FPDFEx_GetPageDimensions()` so
the app/JNI layer can read width, height, effective CropBox, and rotation from
the page dictionary without `FPDF_LoadPage()` / `ParseContent()`. Claude should
adopt this API in the app worktree with optional `dlsym` resolution and a slow
path fallback for unpatched builds.

r48 is a render-behavior patch. It keeps a `BlendGroupRun` open across an
adjacent paint-key change only when the pending run is entirely same-source
`/BM /Darken`, the next blend node has the same source ARGB, and the union
device rect stays memory-bounded. It does not cross clip, segment, text,
normal-object, image/Form/shading, unsupported-blend, or cancellation barriers.

## 6. Current Performance Reading And r46 Findings

### Q16 / Large CAD Stroke Pages

Q16 after r43-r46:

- `flushMatrix=0`
- `matrixPacked` in the millions
- full render stroke draw calls collapsed to hundreds
- small and medium tiles are mostly fast
- large visible regions are still expensive because millions of nodes are truly
  visible and large packed paths are rebuilt/rasterized per replay
- r46 has no direct Q16 effect because `blendNodes=0`

r46 Q16 evidence:

```text
full render:
draws=203 nodes=2,944,028 matrixPacked=2,942,510
flushMatrix=0 fillBlocked=0 sameArgbCrossed=220,916
compileMs=2221 replayMs=2223

large visible region:
draws=95 nodes=2,739,884 matrixPacked=2,738,258
replayMs=1982

small visible tiles:
typical replayMs=5..66 when candidate/drawn node counts are small
```

Q16 current remaining bottleneck:

```text
rebuilding and rasterizing huge packed stroke paths for large visible regions
```

Likely future Q16 class improvement:

```text
cached packed stroke chunks with holder-space bboxes, inside existing segment,
paint, graph-state, path-style, clip, blend, and passthrough barriers
```

### 11.pdf / Darken Blend Pages

11.pdf after r44-r46:

- r43 stroke packing does not materially affect it because the stroke path is
  trivial in the log.
- The dominant cost is still `BlendGroupRun`: many composites, high group pixel
  area, high node pixel area.
- r44 safe disjoint blend paint widening is correct but modest because most
  paint barriers overlap.
- r45 effective-paint widening is safe but did not materially help 11.pdf:
  `paintEquivalent=0` and `paintEquivalentCrossed=0` in r45/r46 logs.
- r46 proves the real shape: almost all expensive blend work is same-source
  Darken even when paint keys differ.

r46 11.pdf evidence:

```text
full render:
groups=2991 darken=2991 nonDarken=0
sameSource=2987 simpleDarken=2987
simpleDarkenNodes=28,792 / 28,805
simpleDarkenPixels=224,969,571
replayMs=894

slow tile examples:
groups=2023 simpleDarken=2022 simpleDarkenPixels=483,164,153 replayMs=1348
groups=1758 simpleDarken=1758 simpleDarkenPixels=418,727,759 replayMs=1296
groups=1563 simpleDarken=1563 simpleDarkenPixels=401,351,730 replayMs=1197
```

11.pdf current remaining bottleneck:

```text
thousands of same-source Darken group composites and repeated group-pixel work
```

Likely future 11.pdf class improvement, now proof-backed by r46:

```text
same-source Darken BlendGroupRun widening across overlapping paint-key changes,
while keeping each entry's own graph state and fill options for coverage
```

Do not implement unsafe overlapping multi-paint group merging. It is not
generally equivalent to PDF per-object blend order.

## 6.1 80/20 Next Improvement Candidates

Ranked by current expected value:

1. r48 same-source Darken widening for 11.pdf-class pages.
   - Expected impact: likely >20% on slow 11.pdf tiles if group count drops
     without excessive union-rect growth.
   - Mechanism: allow overlapping paint-key changes to stay inside the current
     `BlendGroupRun` only when final blend mode is `Darken`, source ARGB is the
     same, entries remain mask/group eligible, and union rect remains
     memory-bounded. Each entry still draws with its own graph state and fill
     options inside the group, preserving coverage.
   - Invariants: no crossing segment, clip, passthrough, image/Form/shading,
     unsupported blend, or normal-object barriers. No filename-specific logic.

2. Cached packed stroke chunks for Q16-class huge stroke pages.
   - Expected impact: likely >20% on large visible-region Q16 renders; small
     sparse tiles are already fast and should not be the primary target.
   - Mechanism: cache reusable packed stroke geometry inside existing ordered
     segment/style/clip/blend barriers, with holder-space bboxes and strict
     memory caps. Replay only visible chunks.
   - Risk: higher than r48 because it adds a memory-owned geometry cache and
     invalidation/lifetime rules.

3. Direct simple-Darken compositor for 11.pdf-class pages.
   - Expected impact: potentially >30%, but higher proof burden.
   - Mechanism: specialize simple same-source Darken into a direct mask/span
     compositor instead of allocating/compositing many BGRA group buffers.
   - Recommendation: do this only after r48 unless r48 fails to reduce group
     count enough.

## 7. Workflow Requirements

Every new native revision needs a workflow file:

```text
.github/workflows/pdfium-android-arm64-rXX-<revision-name>-path-display-list.yml
```

Workflow requirements:

- `name:` starts with `rXX`.
- `WORK_ROOT` uses `/tmp/pdfium-rXX`.
- `DIST_DIR` and upload artifact include `rXX-<revision-name>`.
- Patch apply list includes the new numeric patch.
- Artifact copy step includes the new patch.
- `build-info.txt` patch list includes the new patch number.
- Verification step has revision-specific `grep` guards for the new mechanism.
- Verification step keeps safety guards against deprecated symbols and known
  reverted approaches.

For r25-0056 and later on the stable RenderPlan line, copy the previous
stable-line workflow and update:

```text
r25-00NN -> r25-00NN+1
00NN -> 00NN+1 in apply/copy/build-info
revision name and guards
```

For r25-0057 specifically, guards must require `PathDlSpatialIndex`,
`BuildPathDlSpatialIndex`, `QuerySpatialIndexForReplay`, and `spatialIndex*`
telemetry, while continuing to reject rolled-back post-r25 mechanisms such as
`StrokeRunFlushReason`, `sameArgbCrossed`, and `sameSourceDarken`.

Be careful with mechanical replacement. In r44, a Perl replacement briefly
damaged `$GITHUB_WORKSPACE` references; always inspect for broken strings such
as:

```text
""/repo
"/patches/"
```

## 8. Validation Checklist For New Patches

Before committing a new native patch:

1. Generate the patch against the current patched base, not against untracked
   generated snapshots in the patch repo.
2. Verify:

```text
git apply --check patches/ship/00XX-<name>.patch
```

against a clean temp tree that has the previous patch applied.

3. Parse the workflow YAML.

4. Run local grep guards for the new mechanism.

5. Confirm staged files include only native-patch files for the current task:

```text
patches/ship/00XX-<name>.patch
.github/workflows/pdfium-android-arm64-rXX-<name>-path-display-list.yml
patches/ship/README.md
patches/ship/VELOCE_PATCH_CONVENTIONS.md
```

6. Keep generated source snapshots, unrelated patches, and app worktree changes
   out of the native patch commit unless explicitly requested.

## 9. Commit Convention

Native patch commits use:

```text
Add rXX <revision name> patch
Fix rXX <specific workflow/build issue>
```

Examples:

```text
Add r43 translation-normalized stroke run packing patch
Fix r43 workflow stroke append guard
Add r44 safe blend paint widening patch
```

If a workflow build fails because a guard is stale but the patch is correct,
commit the workflow fix separately as:

```text
Fix rXX workflow <short cause>
```

## 10. Current And Next Patch Direction

The frozen reference line is RenderProgram v2 on the r25 + 0051 correctness
baseline:

```text
r25-0075: immutable holder ownership boundary
r25-0076: bounded parser-time command-order recording, no replay
r25-0077: exact O(1) path eligibility gate before legacy compile
r25-0078: ordered PathRun replay with live text passthrough barriers
```

0076's invariant is that command `i` describes live holder object `i` in exact
painter order. It obtains this order from the holder's existing central append
operation, not from a later classification scan. The program stores no page
object pointers, copied geometry, explicit indices, device-space state, cache,
or synchronization. It is sealed only after parsing and clip normalization and
is invalidated by every supported holder-list mutation. A 32 MiB command-kind
ceiling makes memory use fail-closed; an absent program always leaves the
canonical renderer authoritative.

0077 consumes only exact summary facts recorded by that same append operation.
0078 extends that gate from all-path to exact path/text programs. It compiles
the complete path display list and ordered segment table before drawing, stores
text barriers only as holder indices, resolves each barrier from the live
holder, and resets PDFium clip state at every path/text boundary. Missing
program ownership, count/type mismatch, image/Form/shading commands, unsupported
path state, or segment-budget overflow remains a hard pre-draw fallback.

The 0077 Q16 trace recorded 3,165,420 commands: 3,164,996 paths and 424
non-path commands. The O(1) gate itself cost zero compile/replay milliseconds,
but whole-holder rejection sent every tile through canonical PDFium. 0078
removes that poison-holder behavior when all 424 barriers are text. Expected
successful logs report `programTexts=424`, a bounded `segments` count,
`passthrough=424`, and `result=rendered`; `cache=hit` should dominate after the
first compile.

The r25-1 line starts cleanly from 0076 rather than extending the legacy 0078
consumer:

```text
r25-1-0079: behavior-neutral unified execution and benchmark interface
r25-1-0080: compact command summary and live-object state identity
r25-1-0081: canonical correctness isolation; legacy and unified executors disabled
r25-1-0082: bounded conservative holder-space candidate index
r25-1-0083: exact path/text vertical executor
r25-1-0084: dense path execution kernel with bounded reusable scratch
r25-1-0085: clip/image/Form/group/transparency completeness
r25-1-0086: proof-gated blend kernels
```

These are revisions of one framework, not separate fast paths. Every revision
extends the same holder-owned RenderProgram and writes into the same PDFium
destination bitmap. There is one command order, one bounds/index contract, one
candidate query, one cancellation contract, one byte-accounted cache policy,
and one canonical pre-draw fallback boundary.

Each r25-1 consumer must:

- validate the complete ordered program before drawing any accelerated pixel;
- represent path, text, image, Form, shading, clip, group, and transparency
  commands in the same ordered program as completeness advances;
- preserve unsupported commands and non-proven graphics state as hard barriers;
- use live PDFium objects as the fidelity and editing source of truth;
- never retain raw page-object pointers beyond the holder lifetime;
- fall back before drawing when equivalence cannot be proven;
- keep candidate data holder-space, immutable, and memory-bounded;
- keep all compilation and replay off the UI thread;
- avoid a second full-holder scan and avoid duplicating geometry;
- make native structural cost decide whether acceleration is profitable;
- use the same interface for normal and huge-path pages.

Do not reintroduce RenderPlan v1 patches 0053-0074 behind the new interface.
Useful algorithms must be re-derived against the v2 ownership, ordering,
correctness, and memory invariants rather than copied as accumulated behavior.
Do not call the 0077/0078 legacy path display list from r25-1. It remains an A/B
reference used to prove that the new backend removes work instead of moving it.
