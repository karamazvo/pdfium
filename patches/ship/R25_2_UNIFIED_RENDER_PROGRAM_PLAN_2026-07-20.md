# r25-2 Unified RenderProgram Plan

Date: 2026-07-20
Updated: 2026-07-21 (Asia/Taipei)
Current revision: `r25-2-0100`

## Decision

Generation 1 ended at `r25-1-0091`. Its measurements proved that indexing live
PDFium objects removes sparse scans, but it does not remove the dominant dense
cost: per-object lookup, state setup, rasterization, compositing, and fallback.
The 0091 build admitted 28,727 clipped Darken commands on `11.pdf`, dispatched
zero directly, and rendered all 28,727 canonically. Q16 tiles still visited up
to 2.9 million live commands.

Generation 2 therefore compiles an immutable renderer-ready program once from
PDFium's resolved page model. PDFium objects remain the editing and fidelity
source of truth; the derived program owns only rendering data.

The first 0093 device trace established a failed coverage gate, not an
executor result: Q16 recorded 3,165,420 commands and zero native lines because
all candidate paths were inside marked-content scopes. Its raw streams also
use many translation-only path matrices. 0094 corrected both representation
gaps: the device admitted 2,937,165 native lines, or 92.8% of path commands,
through seven visibility runs and no matrix-table entries or budget fallback.

0095 now measures how much of that renderer-ready stream is relevant to each
real tile. It adds conservative bounds over ordered ranges rather than a
per-line bounds array because the 0094 Q16 program already retains 82.54 MiB
of its 96 MiB budget.

0097 made the program reachable from Android's progressive root renderer. Its
device results separated the next two bottlenecks: Q16 executes 2,937,165
owned lines but still scans the 3,165,420-command stream, while `11.pdf`
compiles 28,806 canonical paths because clip rejection masks its Darken blend
state. 0098 expands the exact command vocabulary without changing that
ordering model: normal-blend multi-segment strokes own their geometry, and all
native strokes consume retained PDFium clip snapshots in ordered runs.

0099 attacks the two remaining measured costs without adding a second
pipeline. For Q16 tiles, bounded ordered command blocks can reject whole
off-tile ranges before live object lookup. For `11.pdf`, owned opaque
stroke-only Darken commands rasterize once and composite their coverage
directly through PDFium's existing integer Darken compositor. Both operations
preserve command order and fail closed to canonical replay.

The 0099 device result proved that its representation and sparse culling work,
but it did not pass the performance gate. On `11.pdf`, all 24,701 owned Darken
paths fell back because the root page is an outer non-isolated transparency
group, producing zero direct draws and 8.8-15 second replays. On Q16, sparse
tiles skipped about 3.15 million commands and replayed in 2-20 ms, while one
dense tile still independently dispatched and rasterized 2,653,167 visible
lines in 7.49 seconds. That dense task then caused 9.5-11.2 seconds of
single-lane queue delay for otherwise fast tiles.

0100 removes those measured compile and executor costs. One exact
previous-state entry bypasses repeated hashing and table probes in uniform CAD
streams. A fixed 256-entry stack packet shares AGG renderer, scanline,
path-storage, and rasterizer setup while keeping every line independently
rasterized and composited in painter order. The outer group Darken path enters
direct coverage only after an allocation-free scan proves the current clip
backdrop is opaque; transparent destinations continue through canonical
PDFium.

## Baseline

Apply exactly:

```text
r25 patches 01..09, 0011..0026, 0029..0031
+ 0051 page dimensions without page parse
+ 0075 holder-owned immutable program lifetime
+ 0076 parser-time exact painter order
+ r25-2-0092 and later generation-2 patches
```

Do not apply `0053..0074`, `0077..0091`, or an invented `0084`. Historical
patch files stay unchanged for audit and A/B comparison.

## Architecture

```text
PDF content
    |
    v
PDFium parser + CPDF_PageObject model  (canonical editing/fidelity truth)
    |
    | one forward parser-owned compilation
    v
holder-owned immutable RenderProgram
    +-- versioned packed bytecode
    +-- owned geometry arrays
    +-- interned immutable graphics states
    +-- ordered optional-content visibility scopes
    +-- ordered clip/group/canonical barriers
    +-- conservative primitive/chunk bounds
    +-- bounded page-space index
    |
    | clip query, painter-order ranges only
    v
bounded native executors
    +-- opaque fill/stroke spans
    +-- exact clip-aware blend spans
    +-- canonical PDFium barrier executor
    |
    v
existing CFX_RenderDevice and destination bitmap
```

There is one PDF semantic model, one derived render program, one candidate
index, one executor boundary, and one destination bitmap. Kotlin owns viewport
intent and scheduling; native PDFium owns parsing and pixel execution. There is
no Kotlin page classification, filename/page rule, warmup, native tile
scheduler, second bitmap, or UI-thread compilation.

## Invariants

1. Program order is exact PDF painter order.
2. Native commands own every geometry and graphics-state scalar needed for
   replay; they do not call a live page object to rediscover them.
3. Canonical barriers and dynamic visibility scopes retain only holder object
   ordinals. Visibility is resolved through PDFium's current canonical render
   options, while the holder owns the immutable program.
4. Structural holder mutation invalidates the complete program in O(1);
   attached-object mutation invalidates block culling by epoch and makes the
   dirty command fall back canonically at its ordinal.
5. Bounds are conservative. Unknown or overflowed bounds are never culled.
6. Unsupported semantics remain ordered canonical barriers.
7. A native executor either validates before its first pixel or does not run.
8. Cancellation occurs between bounded chunks; partial cancelled tiles are not
   published.
9. Program, index, geometry, state, and scratch storage have hard byte budgets.
10. No render work or program compilation runs on the Android UI thread.

## Revisions

| Revision | Scope | Pixel behavior |
| --- | --- | --- |
| `r25-2-0092` | Versioned packed bytecode, canonical opcodes, fixed summary, holder ownership, legacy PathDL disabled | Canonical PDFium only |
| `r25-2-0093` | Owned two-point opaque stroke geometry, exact interned path state/matrices, bounded chunk storage, shadow telemetry | Canonical PDFium only; native data is not executed |
| `r25-2-0094` | Pointer-free visibility runs, exact inline translation matrices, fail-closed rejection telemetry, 96 MiB retained-program ceiling | Canonical PDFium only; corrected native data remains shadow-only |
| `r25-2-0095` | Conservative 64-line leaves, 64-leaf coarse ranges, allocation-free holder-clip query telemetry, actual-capacity budget gate | Canonical PDFium only; bounded visible candidates are measured |
| `r25-2-0096` | Format-5 single-owner ordered replay, bounded opaque-line executor, live dirty-object fallback, 64-command cancellation | Exact supported opaque lines plus canonical barriers, but initially reachable only from `RenderObjectList()` |
| `r25-2-0097` | Shared fail-closed executor entry from ordinary and root progressive rendering | Android progressive root pages can execute the existing exact program |
| `r25-2-0098` | Format-6 owned multi-segment stroke geometry, exact retained PDFium clip runs, bounded path-point storage | Exact supported normal-blend stroke paths and lines with arbitrary PDFium clip paths; unsupported semantics remain canonical |
| `r25-2-0099` | Format-7 conservative 256-command block index, exact side-stream jumps, attached-object mutation epoch, and clip-aware direct Darken coverage | Off-tile ordered ranges avoid object work; supported opaque stroke-only Darken objects avoid temporary BGRA buffers; all unsupported or stale cases remain canonical |
| `r25-2-0100` | Fixed 256-line ordered AGG packets and opaque-backdrop proof for outer non-isolated Darken | Lines retain independent raster/composite semantics while sharing setup; proven outer-group Darken avoids per-object temporary bitmaps; rejection remains canonical before pixels |

Revision numbers remain globally monotonic. Failed or superseded revisions are
not renamed, deleted, amended after push, or reused.

## 0092 Contract

0092 is a foundation and correctness build, not a speed claim:

- one canonical opcode is appended during the existing `AppendPageObject()`;
- command ordinal is the implicit holder object index;
- no raw page-object pointer or duplicate index array is retained;
- one fixed six-counter summary is updated in the same append operation;
- format header records version, command count, bytecode bytes, and native
  command count;
- bytecode is limited to 32 MiB and abandoned on overflow;
- `native_command_count` is zero;
- legacy PathDL returns `kNotEligible` before cache lookup or drawing;
- no generation-1 executor is compiled into the workflow stack.

Expected performance is canonical r25 PDFium. `11.pdf` and Q16 are expected to
be slower than successful experimental fast paths until 0095-0096 execute
owned geometry. The purpose of 0092 is to prove lifetime, format, memory, and
pixel ownership before acceleration resumes.

## 0093 Contract

0093 adds the first renderer-ready command without changing pixel ownership:

- exact two-point, stroke-only, normal-blend paths may become
  `kNativeOpaqueLineShadow` commands;
- the first 4096 holder commands remain canonical; shadow compilation starts
  only for the suffix of a large holder, without a prefix rescan, so ordinary
  pages pay no native eligibility, hashing, or payload-allocation cost;
- endpoints, object matrix, resolved source color, line width, miter, dash,
  cap, join, and stroke-adjust state are copied during the existing parser
  append; no second holder scan or page-object pointer is retained;
- state and matrices use bitwise-exact hash interning with collision checks;
- native lines use fixed 4096-command chunks and are capped at 3 Mi commands;
  bytecode, state, matrix, and dash tables have independent hard ceilings;
- clips, marked content, fills, transparency, transfer functions, overprint,
  patterns, non-finite values, unsupported geometry, and capacity overflow
  remain canonical path opcodes at the same painter-order position;
- legacy PathDL stays disabled and no render call site consumes native lines.

Large holders emit one `VeloceRenderProgram2` line with `nativeOpaqueLines`,
`canonicalPaths`, intern-table sizes, retained bytes, budget fallbacks, and an
elapsed shadow window. The Q16 result was `nativeOpaqueLines=0`, proving that
the marked-content rejection made the primitive representation unusable for
that class. This build measures coverage and compile/memory cost only.

## 0094 Contract

0094 fixes the failed representation gate without taking pixel ownership:

- marked content is no longer treated as a generic rendering rejection;
- exact content-mark identity changes produce ordered four-byte visibility-run
  ordinals, with no retained page-object or mark pointer in the sealed program;
- a future executor must resolve each run through PDFium's current canonical
  optional-content check, so view/print/layer state remains dynamic;
- source endpoints and translation components are stored separately, allowing
  the exact source matrix to be reconstructed without changing floating-point
  composition order or consuming one matrix-table slot per CAD line;
- arbitrary affine matrices continue through bitwise-exact interning;
- first-failure counters classify path/activity, paint, clip, geometry,
  matrix, transparency, color, graph state, visibility, and budget rejection;
- retained program data has a 96 MiB logical ceiling in addition to bytecode,
  line, state, matrix, dash, visibility-run, and mark-count ceilings;
- format version advances to 3, legacy PathDL remains disabled, and no render
  call site consumes the shadow commands.

The 0094 acceptance gate is nonzero Q16 native coverage without matrix-table
exhaustion, bounded retained bytes, unchanged canonical pixels, and no material
normal-page acquisition regression. Bounds/index work starts only after this
gate passes.

The 0094 device result passed that gate:

- Q16 admitted `2,937,165 / 3,164,996` path commands (`92.8%`);
- all admitted lines used seven pointer-free visibility runs and exact inline
  translations, with one state, zero matrices, and zero budget fallback;
- retained data was `82.54 MiB`, or `86%` of the hard ceiling;
- acquisition was `10.07s` versus `9.99s` in 0092, a non-material `0.8%`
  difference in one trace;
- pixels and render time remained canonical by design;
- 11.pdf admitted zero lines because all post-threshold candidates were
  rejected by clip (`24,702`) or paint (`8`) semantics.

## 0095 Contract

0095 adds one conservative, bounded query structure without taking pixel
ownership:

- native line bounds reuse the parser-computed `CPDF_PageObject::GetRect()`;
  no geometry, holder, or page-object rescan is added;
- each 32-byte leaf covers 64 consecutive native lines and records its native
  and command ranges in exact painter order;
- each 28-byte coarse range covers 64 leaves, allowing 4,096 lines to be
  rejected with one comparison when their union misses the tile;
- non-finite or empty bounds mark a range unbounded and always candidate;
- the inverse device clip already used by canonical `RenderObjectList()` is
  queried without a candidate vector, sort, cache, lock, or second scheduler;
- full-page clips containing the complete native bounds return all lines in
  O(1);
- actual retained vector capacity is checked against the same 96 MiB ceiling
  before the program is installed;
- Android telemetry reports candidates, culled lines, tested/rejected ranges,
  fail-open state, retained index bytes, and query microseconds;
- format version advances to 4, legacy PathDL remains disabled, and the
  unchanged canonical object loop still paints every pixel.

0095 is accepted only if representative sparse Q16 tiles reject substantial
native work at low query cost, no query fails open unexpectedly, retained data
stays below the existing ceiling, and canonical rendering remains unchanged.
If ordered leaves remain effectively page-wide, 0096 still applies exact live
object bounds before rasterization, preserving correctness and avoiding a
second index. That result would block any claim that leaf culling is the main
gain and would motivate a more selective compact index before native batching.

## 0096 Contract

0096 is the first generation-2 revision that changes pixel execution. It does
not add another renderer or scheduler. `CPDF_RenderStatus` consumes the
immutable program in the same holder call that previously ran the canonical
object loop:

- one bytecode pass preserves every PDF object ordinal and painter-order
  barrier;
- canonical opcodes still call `RenderSingleObject()` with live PDFium objects;
- `kNativeOpaqueLine` reconstructs only previously proven two-point,
  stroke-only, solid, opaque, normal-blend geometry from owned data;
- the 0095 leaves and exact live object bounds reject native lines before AGG
  rasterization, without a candidate vector or second scan;
- matrix, width, dash, cap, join, miter, stroke adjustment, render-option color
  translation, optional-content visibility, and device clip reset retain
  PDFium semantics;
- graph state, fill options, stroke color, and one two-point `CFX_Path` buffer
  are reused across matching lines rather than rebuilt or allocated per draw;
- a live dirty object is rendered canonically at its exact command position,
  so PDFium page objects remain the editing source of truth;
- cancellation is checked every 64 commands and after canonical or background
  fallback work;
- malformed holder/program preconditions return to the untouched canonical
  loop before drawing; after execution starts, the holder is never restarted
  through a second pixel owner;
- the existing feature value `0x1` now names
  `FPDFEX_FEATURE_RENDER_PROGRAM`; the old Form PathDL name remains an ABI
  compatibility alias while legacy PathDL stays disabled.

0096 deliberately retains one raster operation per native PDF line. Combining
independent antialiased strokes can change overlap coverage, so batching is not
part of this correctness milestone. Its real performance gain must come from
off-tile raster rejection and removal of generic PDF object/transparency/path
dispatch for supported visible lines.

Expected scope:

- Q16-like pages with high native coverage should log nonzero `nativeDraws`,
  substantial `leafCulled + boundsCulled`, and materially lower replay time;
- ordinary pages below the 4,096-command threshold remain wholly canonical;
- mixed pages preserve canonical text, images, fills, forms, shading, clipped
  paths, transparency, and unsupported geometry in the same ordered replay;
- 11.pdf remains canonical in 0096 because 0094 observed zero admitted native
  lines (`rejectClip=24702`, `rejectPaint=8`). Its blend/clip representation is
  a later proof task, not something 0096 guesses around.

## 0097 Contract

The first 0096 device run proved that representation and execution reachability
must be measured separately:

- Q16 compiled `3,165,420` ordered commands and `2,937,165` native lines in
  `8.3s`, retaining `88.7 MB`;
- the same log contained zero `event=replay` records despite the app supplying
  feature flags `0x1`;
- Android region and preview rendering enters
  `CPDF_ProgressiveRenderer::Continue()`, whose root object loop calls
  `ContinueSingleObject()` directly and does not call `RenderObjectList()`;
- therefore 0096 paid program compile and retention cost but did not execute
  the program for the measured root-page renders.

0097 closes that architectural entry gap without creating another executor:

- `CPDF_RenderStatus::TryRenderVeloceProgram()` is the single public,
  fail-closed holder entry used by both ordinary and progressive rendering;
- the existing private replay implementation remains the only program pixel
  executor;
- progressive rendering attempts the program exactly once when a layer starts,
  after the disabled legacy PathDL entry and before canonical iteration;
- rejection returns before pixels change and leaves the existing progressive
  iterator untouched;
- a rendered program consumes and finalizes the complete layer, so canonical
  progressive replay cannot restart the same holder;
- cancellation after replay starts marks the render status stopped, restores
  the device state, and prevents partial output publication through the
  existing caller contract;
- no cache, bitmap, scheduler, candidate vector, lock, or UI-thread work is
  introduced.

Acceptance requires Q16 logs to contain both
`revision=r25-2-0097 event=compile` and `event=replay`, with nonzero native
draw/cull counts. Absence of replay remains an activation failure, regardless
of elapsed-time variation. 11.pdf is not expected to improve in 0097 because
its currently compiled program has no admitted native commands; clip/blend
representation remains separate work.

## 0098 Contract

0098 removes two representation blockers without weakening PDF semantics:

- `kNativeOpaqueStrokePath` owns the complete `CFX_Path` point/type/close
  sequence for finite, multi-segment, stroke-only, normal-blend paths;
- the existing two-point line opcode and 28-byte chunked line storage remain
  unchanged, so Q16 does not pay per-line path-object or clip-index inflation;
- every native command is covered by an ordered `VeloceClipRun`; each run
  retains PDFium's own copy-on-write `CPDF_ClipPath`, including path and text
  clip semantics, rather than encoding a rectangle approximation;
- replay installs a run through canonical `ProcessClipPath()` and reapplies it
  after every canonical barrier, preserving the device clip stack and painter
  order;
- owned path bounds are conservative holder-space bounds. Unknown bounds fail
  open; known off-tile paths skip rasterization without changing command
  order;
- path count, total owned path points, clip runs, bytecode, state tables,
  line storage, and actual retained capacity are all bounded by the existing
  96 MiB program ceiling and independent hard limits;
- dirty objects render canonically at their original ordinal; optional-content
  visibility and render-option color translation continue through live PDFium
  state;
- fill paths, mixed fill/stroke paint, patterns, alpha, soft masks, transfer
  functions, overprint, non-normal blend modes, invalid geometry, and budget
  overflow remain canonical barriers before native pixels are written.

0098 is not the Darken optimization. On `11.pdf`, removing the earlier clip
rejection is expected to expose `rejectTransparency` for the same objects;
that is correct fail-closed behavior until 0099 proves exact clip-aware Darken
execution. Q16 should retain roughly the same line coverage and memory shape,
with only a small number of clip runs. The decisive 0098 log fields are
`nativeOpaquePaths`, `clipRuns`, `ownedPathPoints`, `pathDraws`, and
`pathBoundsCulled`.

## 0099 Contract

0099 combines two executor improvements because they remove independent costs
from the same immutable ordered program:

- the parser's existing append pass builds one conservative holder-space bound
  for each consecutive 256-command suffix block;
- every block records exact command, native-line, and native-path counts, so a
  rejected block advances all streams without reordering or candidate storage;
- unknown/empty/non-finite bounds fail open, stop-object rendering disables
  block skipping, and an attached page-object mutation epoch mismatch disables
  all stale block skips for that replay;
- the first 4096 commands remain canonical and receive no block metadata;
- `kNativeDarkenStrokePath` owns finite stroke-only geometry already proven to
  use solid color, alpha 1, Darken, no soft mask/transfer/overprint, and exact
  retained clip state;
- AGG rasterizes each Darken object separately and passes its coverage and
  current antialiased clip coverage directly to `CFX_ScanlineCompositor`;
- group, isolated-group, type-3, printer, unsupported destination, changed live
  semantics, or driver rejection falls back canonically at the same ordinal
  before any direct destination mutation;
- no image-sized scratch bitmap, candidate vector, cache, scheduler, lock, JNI
  policy, or UI-thread work is added;
- command blocks, owned geometry, clip runs, states, and indexes remain inside
  the existing 96 MiB retained-program budget.

Acceptance is based on removed work. Q16 tile logs should show nonzero
`blocksSkipped` and `commandsSkipped`, with `commandsVisited + commandsSkipped`
equal to the complete command count. `11.pdf` should show nonzero
`nativeDarkenPaths` at compile and `darkenDraws` at replay; a high
`darkenFallbacks` count means the runtime proof remains blocked and no speed
claim is valid. `blocksCurrent=0` is a deliberate correctness fallback after
attached-object mutation.

The actual 0099 result failed the second gate and exposed the dense executor as
the next dominant cost:

- `11.pdf`: `nativeDarkenPaths=24701`, `darkenDraws=0`,
  `darkenFallbacks=24701`, replay `8.807s` for preview and up to `15s` for
  regions;
- Q16 sparse tiles: about `3.15M` commands skipped and `2-20ms` replay;
- Q16 dense tile: `2,653,167` lines drawn in `7.485s`, followed by seconds of
  head-of-line delay for sparse tiles on the single PDF session lane.

## 0100 Contract

0100 changes execution rather than adding a new index or telemetry pass:

- before hashing an eligible command's graphics state, the compiler compares
  it with the immediately preceding interned state and reuses the exact index
  on equality; this adds one optional scalar and no allocation, approximation,
  or second pass;
- consecutive eligible lines with identical exact state and clip are collected
  into a fixed 256-entry stack packet; canonical commands, dirty objects, clip
  or state changes, owned paths, Darken paths, capacity, and cancellation are
  hard flush boundaries;
- each packet entry owns its source endpoints and object-to-device matrix;
  AGG still rasterizes and composites every line separately in original order,
  so antialias overlap and painter-order pixels are unchanged;
- the packet shares only immutable renderer configuration and bounded AGG
  scratch, removing one `CFX_Path`, render-device dispatch, renderer, scanline,
  path-storage, and rasterizer construction per line;
- the driver validates the whole packet before its first pixel; unsupported
  drivers return false and the executor canonically replays the same holder
  ordinals in order;
- an outer non-isolated group may use direct Darken only if one allocation-free
  scan proves every destination pixel in the current clip has alpha 255;
  non-group Darken remains exact, while transparent, isolated, nested, printer,
  Type-3, changed-live-state, and unsupported-device cases remain canonical;
- no command format change, page-sized cache, candidate allocation, second
  bitmap, scheduler, lock, JNI/Kotlin policy, or UI-thread work is added.

Device acceptance requires real dispatch, not just compilation:

- Q16: `lineBatchCommands > 0`, `lineBatchDispatches` materially smaller than
  `lineBatchCommands`, `maxLineBatch > 1`, and `lineBatchFallbacks=0` on AGG;
- `11.pdf`: `darkenDraws > 0` and `darkenFallbacks` no longer approximately
  equals `nativeDarkenPaths` on the normal opaque Android page target;
- normal and transparent-output corpus pages remain pixel-identical to the
  canonical baseline.

## Proof Gates

Every generation-2 behavior patch must pass:

- patch-stack apply check from the exact baseline;
- normal-page canonical pixel comparison;
- painter-order tests with path/text/image/Form/shading barriers;
- clip, transparency, soft-mask, and group fallback tests;
- mutation invalidation and holder teardown tests;
- cancellation with no partial tile publication;
- memory budget and overflow tests;
- cold preview, sparse tile, dense tile, zoom, and pan benchmarks;
- external MuPDF comparison on the same device, viewport, scale, and bitmap
  dimensions.

The performance target is demonstrated by work removed, not only elapsed time:

- Q16 dense tiles must stop resolving/rasterizing millions of irrelevant live
  objects;
- `11.pdf` must stop allocating per-object transparency buffers for supported
  exact blend spans;
- normal pages must remain on canonical PDFium unless a complete ordered native
  subset is proven equivalent.
