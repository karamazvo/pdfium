# rel-260810 Unified Rendering Release Handoff

**Locked:** 2026-08-10 17:28:16 CST (Asia/Taipei)

**Audience:** xPDFSDK maintainers, Codex, Claude Code, release engineering,
and future PDFium performance contributors

**Scope:** Android arm64 AGG `libpdfium.so` only. Kotlin/JNI scheduling changes
are tracked as integration requirements but are not contained in this native
artifact.

## 1. Release Identity

```text
release: rel-260810
artifact: libpdfium-android-arm64-rel-260810-unified-rendering
workflow: .github/workflows/pdfium-android-arm64-rel-260810-unified-rendering.yml
renderer: AGG
PDFium base: r25
correctness rollback: rel-260701
```

`rel-260810` promotes the exact integrated stack previously built as
`r25-jpx-5-0006`. It does not combine independent experimental heads or infer
that every patch present under `patches/` is safe.

The workflow name starts with `rel-260810` so the release action is immediately
visible in GitHub Actions.

## 2. Release Decision

This is a release candidate until the device acceptance matrix in section 9
passes. It contains only mechanisms that are part of the best integrated
working stack:

1. the r25 rendering foundation and no-parse page dimensions API;
2. the accepted `r25-8-0138` ordered RenderProgram executor;
3. the accepted normal-image bounded row pipeline;
4. exact mask execution used by the integrated image/cancellation path;
5. bounded native cancellation through the complete expensive image chain;
6. exact clipped JPX decoding plus the required window-validation correction.

It deliberately excludes later experiments that regressed, failed to activate,
changed pixels, lacked device proof, or only corrected an unaccepted build.

## 3. Exact Native Patch Stack

### 3.1 Foundation

```text
patches/ship/01..09
patches/ship/0011..0026
patches/ship/0029..0031
patches/ship/0051
```

Important effects:

- codec-native JPEG reduction for eligible small previews;
- exact Indexed, Separation, and supported DeviceN lookup-table conversion;
- scoped render callbacks and cancellation ABI;
- the root/Form path display-list and ordered Darken group foundation;
- `FPDFEx_GetPageDimensions()` without page-content parsing.

The classification ABI from patch `08` remains exported for compatibility.
The release architecture does not require app-wide `classifyAllPages`, and the
Android integration must not restore that deprecated eager policy.

### 3.2 Accepted ordered RenderProgram

```text
patches/ship/0075..0076
patches/ship/0092..0123
patches/ship/0125..0129
patches/ship/0133..0138
```

The important retained mechanisms are:

- one pointer-free source-ordered native/canonical command program;
- exact fail-closed lowering rather than document classification;
- lazy sidecar activation, so ordinary pages remain canonical without eager
  RenderProgram allocation;
- owned compact line/path payloads and stable source ordinals;
- exact canonical barriers for text, images, Forms, shading, unsupported
  transparency, clipping, and state;
- ranked payload-free exact no-op lines;
- bounded 32-command holder-space spatial blocks;
- proportional line storage and a 96 MiB program ceiling;
- bounded cancellation cadence;
- one reusable ordered AGG stroke renderer per homogeneous packet, while every
  PDF paint command retains its own raster/composite operation.

Measured reference result for Q16:

```text
accepted 0138 acquisition: approximately 3,185 ms
accepted 0138 bitmap replay: approximately 1,465 ms
accepted 0138 total: approximately 4,651 ms
0138 bitmap improvement over prior executor: approximately 21.7 percent
renderer constructions reduced by approximately 253x
```

This stack also retains the proven root/Form and Darken mechanisms that made
`11.pdf` materially faster and the bounded exact Form program used by EP23.

### 3.3 Normal image pipeline

```text
patches/experiments/normal-page/0005-bounded-streaming-image-renderer.patch
patches/experiments/normal-page/0006-canonical-decode-to-stretch-row-pipeline.patch
patches/experiments/normal-page/0011-exact-mask-execution-plan.patch
```

`0005` replaces a potentially large full stretch intermediate with a source-row
ring capped at 1 MiB. It uses PDFium's existing weight tables and fixed-point
accumulation order.

`0006` progressively realizes canonical decoded rows into the existing page
image cache while feeding the stretcher. It removes eager full realization and
avoids decoding a row twice. The canonical page cache remains the sole decoded
image owner.

`0011` learns exact opaque-mask state while rows are already being decoded. It
does not perform a second scan. Eligible variable masks are consumed directly
as AGG coverage instead of first materializing an equivalent BGRA mask image.
Unsupported or incomplete mask states remain canonical.

These patches reduce successful-render allocation and memory traffic. They do
not claim that codec entropy decoding disappears.

### 3.4 Bounded cancellation

```text
patches/experiments/cancellation/0001-bounded-native-image-codec-cancellation.patch
patches/experiments/cancellation/0002-bounded-affine-image-transform-cancellation.patch
patches/experiments/cancellation/0003-bounded-exact-masked-image-composition.patch
```

One render-status cancellation snapshot flows through:

```text
OpenJPEG/JBIG2 decode
  -> component/color conversion
  -> canonical row realization
  -> horizontal/vertical stretch
  -> affine transform
  -> soft-mask and matte processing
  -> final masked composition
```

Cancellation is checked at natural bounded work units. A cancelled result is
abandoned and cannot enter background redraw or be published as a completed
bitmap. No worker, lock, queue, bitmap cache, or second cancellation owner is
introduced.

This family improves stale-work release and next-visible admission. It is not
credited as uncancelled throughput improvement.

### 3.5 Exact clipped JPX decode

```text
patches/experiments/jpx/0005-exact-clipped-jpx-decode.patch
patches/experiments/jpx/0006-exact-clipped-jpx-window-validation.patch
```

The existing stretcher calculates the exact required source clip. PDFium maps
that clip into the OpenJPEG reference grid, decodes only the required JPX
window, and exposes it through a logical full-image view for the unchanged
stretcher and compositor.

`0006` is mandatory: it distinguishes a deliberately clipped output from an
invalid undersized full decode and validates that the decoded window completely
covers the required source rectangle.

Observed effect: zoom-region output improved materially, approximately 2x in
the recorded scanned-page case. A full-page preview still requests the whole
page and therefore remains a full decode.

## 4. Unified Runtime Architecture

```text
PDF content stream
        |
        v
canonical PDFium parser and exact lowering
        |
        +-------------------------+
        |                         |
        v                         v
ordered RenderProgram       canonical page objects
exact owned commands        unsupported/editing source of truth
        |                         |
        +-----------+-------------+
                    |
                    v
source-ordered replay
  path packet | canonical barrier | image | Form | text
                    |
                    v
PDFium AGG destination bitmap
```

Image execution is part of the same render status:

```text
codec/source rows
  -> canonical page-image owner
  -> bounded exact stretch rows
  -> exact mask/clip composition
  -> destination
```

The architectural invariant is:

> Every source ordinal has one authoritative rendering owner. Acceleration is
> used only after complete semantic eligibility succeeds before its first
> pixel. Unsupported work remains canonical in the same painter order.

## 5. Resource And Threading Contract

- All parsing, lowering, decode, transform, and raster work remains off the UI
  thread.
- PDFium retains one native render owner in the Android integration.
- No new global lock or wider critical section is introduced.
- RenderProgram retained memory is capped at 96 MiB.
- Normal stretch temporary memory is capped at 1 MiB per active stretch.
- Exact clipped JPX output is transient in this release; no JPX source-window
  or decoder-session cache from `0007/0008` is included.
- Cancellation observes one lock-free request snapshot and never publishes
  partial output as success.
- Canonical page objects remain available through fail-closed materialization
  for editing, enumeration, save, print, or unsupported execution.

## 6. Explicit Exclusions

### 6.1 Deprecated and correctness-failed generations

```text
0027, 0028
0032..0050 and 0052 from the expanded path-display-list generation
0053..0074 RenderPlan v1
0079..0091 RenderProgram generation 1
0124
0130, 0131
```

Reasons include painter-order violations, raw-pointer lifetime bugs, duplicate
execution paths, blank/zoom continuity failures, missing Q16 lines, and
performance regressions.

### 6.2 Post-0138 rejected executor experiments

```text
0139..0147
```

- `0140/0141/0142` reduced representation work but regressed acquisition and
  total rendering.
- `0143` executed the specialized path without improving bitmap time.
- `0144` skipped most tested destination channel writes but regressed, proving
  final channel arithmetic was not dominant.
- `0145` found zero exact zero-coverage opportunities.
- `0146` was unreachable under Android reverse-byte-order output.
- `0147` corrected integration and removed about 60 percent of Q16 raster
  passes, but bitmap improved only 11.7 percent and cold total regressed 9.2
  percent.

`0148` is diagnostic only and is not a rendering base.

### 6.3 Early Form tape

```text
0149
0150
```

`0149` is a valid architecture experiment: on an exact all-native Form cache
hit it adopts the immutable Form program before child-object materialization.
`0150` changes only a stale `nullptr` return to `std::nullopt` so the experiment
builds.

Neither patch is in `rel-260810` because the required device proof is not
locked: EP23 must show `result=early_hit`, a material acquisition reduction,
unchanged pixels, and no first-use or ordinary-page regression. Performance
must not be attributed to `0150` itself.

### 6.4 Parser-integrated ownership experiment

```text
0151
```

`0151` removes duplicate retained path ownership after exact lowering. It is a
useful memory/architecture experiment but has not demonstrated the broad
correctness and latency improvement required for this release.

### 6.5 Rejected normal-image experiments

```text
0001 transform-sized decode
0003 direct mask composite as an isolated experiment
0004 opaque SMask elision as an isolated experiment
0007/0008 destination-row fusion
0009 fixed-support resampler
0010 vector row kernel
```

The later resampler and fusion experiments did not improve aggregate 6Steps
latency and sometimes regressed median/P90. Their useful exact mask mechanism
was redesigned into included `0011` rather than stacking them.

### 6.6 Rejected or inactive JPX experiments

```text
0002 parallel OpenJPEG decode
0003 separate ARM64 color finish
0004 fused output sink
0007 retained source-window bitmap
0008 single-component reusable decoder session
```

Parallel decode and the separate SIMD finish regressed. The fused sink did not
become an accepted performance parent. Source-window retention did not match
the adjacent/disjoint pan workload. Decoder-session reuse did not activate for
the observed three-component RGB JPX resources.

## 7. Android Integration Requirements

The native artifact does not by itself guarantee visible-first behavior. The
Android worktree consuming `rel-260810` must retain:

- no eager `classifyAllPages` call;
- no hidden 540-pixel preview request;
- newest-visible request coalescing;
- cancellation of active obsolete work before admitting its replacement;
- one authoritative generation for publication;
- discarded cancelled/partial bitmaps;
- last complete page/tiles retained until complete replacements are ready;
- bounded live-page and bitmap ownership;
- no UI-thread document parsing or rendering.

Do not reintroduce page classification to select the RenderProgram. Exact
lowering success is the native capability decision.

## 8. Release Artifact Contract

The workflow must:

1. apply every patch explicitly in the frozen order;
2. fail if a patch id resolves to zero or multiple files;
3. compile the Android arm64 static and shared AGG libraries;
4. compile `pdfium_unittests` after shared-library linking;
5. verify public extension symbols;
6. verify structural contracts for `0138`, normal `0005/0006/0011`,
   cancellation `0001..0003`, and JPX `0005/0006`;
7. reject diagnostic ATrace markers and excluded generation symbols;
8. package the exact patches, public headers, `libpdfium.a`, `libpdfium.so`,
   SHA-256, this handoff, and machine-readable `build-info.txt`.

The workflow is the authoritative stack. Directory contents and patch-number
order are not sufficient evidence.

## 9. Device Acceptance Matrix

Use the same app commit, target device, viewport, gesture script, cold/warm
protocol, and log filters for the oracle and candidate.

| Corpus | Required proof |
| --- | --- |
| `The-6-Steps-...pdf` pages 7, 20, 24 and fast-scroll sequence | No blank/missing page; unchanged resolution and pixels; normal-page median/P90 no regression versus the accepted `0005/0006` base and meaningful improvement versus `rel-260701` |
| `Undergraduate Medicine Study Notes.pdf` | Canonical route without eager sidecar work; no fast-scroll regression or delayed publication |
| `11.pdf` | Correct Darken output; native accelerator remains materially faster than `rel-260701`; no zoom/pan blanking |
| `Q16BC-1-51-0-01101-01.pdf` | Table lines present; counters match accepted `0138`; acquisition/replay remain within controlled variance of `0138` |
| `error.pdf` human pages 2 and 3 | Correct repeated Form output; no regression versus `0138`; do not expect unvalidated early Form-tape behavior |
| `disquisitionesa00gaus.pdf` pages 10, 357, 700 | Correct color and mask at preview and zoom; clipped tiles materially faster than full decode; cancellation p95 below 50-75 ms |
| Broad correctness corpus | Pixel comparison against `rel-260701`, with any intentional libjpeg reduced-decode difference reviewed separately; no crash, UAF, missing object, stale tile, or partial publication |

Release only when:

- no correctness regression exists;
- ordinary-page median and P90 do not regress materially;
- uncancelled cancellation-check overhead remains below 3 percent;
- cancellation p95 is at most 75 ms;
- memory stays within documented caps;
- repeated close/reopen/scroll/zoom stress has no native crash.

## 10. Rollback

`rel-260701` remains the production correctness rollback:

```text
r25 rendering behavior + patch 0051 page dimensions API
```

If a broad correctness, lifecycle, or normal-page regression is found, replace
the native artifact with `rel-260701`. Do not attempt to fix a release failure
by activating a document-specific gate or stacking a corrective experiment
onto the shipped binary.

Bisect in this order:

1. JPX `0005/0006` for scan zoom/color/window failures;
2. cancellation `0003`, then `0002`, then `0001` for stale-work or partial
   masked-image behavior;
3. normal `0011`, then `0006`, then `0005` for ordinary image/mask issues;
4. `0138` integrated stack versus `rel-260701` for vector/Form issues.

## 11. Postmortem Conclusions

### What worked

- Remove duplicate or unnecessarily materialized work while preserving exact
  source order.
- Build immutable pointer-free render commands only after exact lowering.
- Keep ordinary pages canonical through success-lazy activation.
- Use holder-space bounds to avoid irrelevant tile commands.
- Reuse rendering context, clips, state, and resource programs without merging
  independent PDF paints.
- Stream image rows through the existing exact resampler instead of allocating
  a full intermediate.
- Push one cancellation owner into the actual indivisible codec/raster stages.
- Decode the exact JPX source window required by the visible destination.

### What failed

- Filename/page classification as an architectural decision.
- Multiple overlapping RenderPlan/path-display-list executors.
- Reordering or approximate merging of painter operations.
- Adding preflight, overlap grids, staging, or packet representations that cost
  more than the work removed.
- Optimizing final channel writes when edge generation, raster setup, or codec
  work dominates.
- Extra decoder threads and separate SIMD passes around bandwidth-bound JPX
  stages.
- Resolution reduction or visual continuity substitutions presented as exact
  rendering improvements.

## 12. Next Rendering-Pipeline Experiments

These are not part of `rel-260810`.

### A. Destination-banded ordered vector executor

Build directly on the accepted `0138` RenderProgram. Use existing spatial
blocks to create bounded destination-band command streams. Hoist clipping,
transform, edge storage, and scanline setup out of individual strokes while
preserving one ordered source-over result per command. Do not merge coverage or
reorder paints. This is the only remaining credible cold-Q16 multi-x replay
direction.

### B. Parser-to-tape single ownership

Continue the invariant explored by `0151`, but accept only a direct transfer of
completed exact parser geometry and immutable state into the same ordered
program. Unsupported operations remain canonical. Do not retain both a native
path and canonical path object, and do not add a second lowerer.

### C. Compiled destination-row ImageOp

Replace the separate decode, color, resample, mask, and composite orchestration
for an exactly eligible image with one bounded operation:

```text
codec rows -> colorspace -> exact weights -> mask/clip -> destination rows
```

The existing page-image owner, PDFium equations, and canonical fallback remain
authoritative. This targets actual cold image rendering rather than bitmap
reuse.

### D. Early immutable Form execution

Re-evaluate `0149/0150` as a separate experiment. Promote only after EP23
proves an early hit before child materialization and passes the full release
matrix. If accepted, integrate the semantic Form call into the ordered program,
not as a document-specific cache rule.

### E. Codec backend

JPX cold full-page rendering still contains unavoidable entropy and inverse
wavelet work. Continue only with a change that reduces component-plane
materialization or emits exact requested destination bands. Reject more wrapper
passes or worker scheduling without at least a 15 percent controlled gain.

## 13. Development Convention

- Patch repository: `/Users/shchao/Code/xPDFSDK/android/meta/libs/pdfium`
- Android integration worktree: `/Users/shchao/Code/xPDFSDK.worktrees/codex-use-r25-0053-pdfium`
- PDFium behavior is changed through patch files and workflows, not by
  committing generated applied-source modifications.
- New revision numbers are never reused, including failed builds.
- Do not amend a pushed revision; add a new correction commit/revision.
- Workflow and artifact names begin with the release/revision identifier.
- Every performance claim records parent artifact, app commit, device, corpus,
  cold/warm protocol, pixel result, median/P90/P95, memory, and cancellation.
- MuPDF reference: `android/meta/mupdf-1.27.2-source`.
- PDF.js reference: `android/meta/libs/pdf.js-5.7.284`.

## 14. Current Authority

Use these references together:

```text
REL_260810_UNIFIED_RENDERING_RELEASE_HANDOFF_2026-08-10.md
codex_pdfium_veloce_renderprogram_handoff_2026-07-31.md
PATCHES_STATUS.md
the rel-260810 workflow and packaged build-info.txt
```

When documents disagree, the release workflow's exact patch list determines
the binary, this handoff determines promotion/exclusion policy, and
`rel-260701` determines rollback correctness.
