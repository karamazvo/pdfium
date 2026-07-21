# r25-2 Unified RenderProgram Plan

Date: 2026-07-20
Current revision: `r25-2-0097`

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
4. Mutation invalidates the complete program in O(1).
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
| `r25-2-0098` | Owned multi-segment opaque path chunks | Exact supported opaque paths |
| `r25-2-0099` | Exact clip/group-aware blend spans | Exact supported blend commands |

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
