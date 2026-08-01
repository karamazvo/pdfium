# r25-15-0145 Exact Zero-Coverage Stroke Elision

Status: implemented, awaiting CI build and controlled device validation

Locked: 2026-08-01 15:07 CST

Base: accepted `r25-8-0138`

Excluded experiments: `0139` through `0144`

## Problem

The accepted Q16 control still performs 2,590,767 independent AGG raster
passes for one full preview. `0144` skipped 90.1% of eligible destination
pixel composites without improving bitmap time, proving that the remaining
cost is before final channel composition.

`0145` tests whether a meaningful subset of those source operations becomes
mathematically empty at the target device scale and can be removed before AGG
cell generation.

## Correctness Invariant

An operation may be skipped only when the exact four-vertex polygon already
used by the accepted direct butt-stroke renderer becomes two retraced edges
after AGG's own signed 24.8 `poly_coord()` conversion.

The accepted polygon order is:

```text
start-left -> start-right -> end-right -> end-left -> close
```

Zero coverage is proven only when either:

```text
start-left == end-left AND start-right == end-right
```

or:

```text
start-left == start-right AND end-left == end-right
```

where equality is exact equality of both AGG fixed-point coordinates. The
closed path then contains only an edge and its exact reverse, so it contributes
no area, coverage, destination write, or painter-order effect.

Any unsupported cap, dash, path shape, non-finite geometry, or non-collapsed
polygon follows the existing `0138` renderer unchanged.

## Execution

```text
ordered RenderProgram command
  -> accepted direct butt-stroke four-vertex construction
  -> transform four vertices to device space once
  -> convert each vertex with AGG poly_coord()
       exact retraced collapse: count and skip before rasterizer
       otherwise: submit the same device vertices to AGG
  -> independent immediate source-order composite
```

The transformed vertices are reused for the non-empty AGG path. The patch does
not duplicate matrix transforms or create a second geometry representation.

## Resource Policy

- Eight fixed-point integers and four device vertices are stack-local.
- No persistent allocation, page cache, bitmap, index, lock, thread, JNI,
  Kotlin, or UI-thread work is added.
- Existing maximum-256 command packets, memory ceilings, spatial culling,
  cancellation cadence, clip handling, and canonical fallback are unchanged.
- Canonical normal pages do not enter this executor.
- There is no page classifier, document name, page number, content threshold,
  or approximate width test.

## Tests And Contract

`OrderedFixedPointCollapsedButtStrokeSkipsRasterPass` proves that a stroke
which collapses below the AGG fixed-point boundary produces the same untouched
bitmap as canonical PDFium while reporting zero raster passes.

`OrderedButtStrokeAboveFixedPointCollapseStillRasterizes` proves that a nearby
non-collapsed stroke still executes one raster pass and remains byte-identical
to canonical PDFium.

Every accepted packet enforces:

```text
raster_passes + zero_coverage_strokes == source_draws
```

The replay log adds:

```text
revision=r25-15-0145
mode=exact_zero_coverage_stroke_elision
zeroCoverageStrokes=<raster passes removed before AGG>
```

## Acceptance Gate

Accept `0145` only when all conditions hold:

1. Patch application, Android library build, and `pdfium_unittests` compilation
   pass in the workflow.
2. Lossless bitmap comparisons match `0138` for normal pages, 11, Q16, EP23,
   and 6Steps.
3. Q16 reports a material `zeroCoverageStrokes` count and satisfies
   `lineRasterPasses + zeroCoverageStrokes == lineBatchCommands`.
4. Repeated cold Q16 median bitmap time improves by at least 20% from `0138`
   without acquisition regression.
5. 11, EP23, Study Notes, 6Steps, cancellation, memory, and publication remain
   within normal variance and preserve correctness.

Reject the performance hypothesis if the skip count is small or the added
fixed-point comparisons do not produce the required bitmap gain. This patch
does not claim an order-of-magnitude improvement in advance.

## Device Result - 2026-08-01

Reject `0145` as a performance base. The exact predicate did not match a
single command in the supplied device runs:

| Workload | Observed replays | `zeroCoverageStrokes` | Cold acquire / bitmap / total |
| --- | ---: | ---: | ---: |
| 11 | 40 | 0 in every replay | 218 / 150 / 369 ms |
| Q16 | 51 | 0 in every replay | 3,942 / 1,967 / 5,910 ms |
| EP23 | 64 | 0 in every replay | p2 390 / 377 / 767 ms; p3 840 / 160 / 1,000 ms |
| 6Steps | 11 accelerated micro-runs | 0 in every replay | ordinary-page previews ranged from 39 to 290 ms in the captured scroll |

Q16 still executed 2,590,767 raster passes for its full preview. Compared with
accepted `0138` at 3,185 / 1,465 / 4,651 ms, this run was slower in acquisition,
bitmap replay, and total latency. Since `zeroCoverageStrokes` was zero, no
timing movement in any workload can be attributed to elision. The faster 11
sample and slower EP23 samples are run variance or unrelated pipeline work.

The negative result is architecturally useful: after the existing exact no-op
mask removes 574,229 Q16 commands, the remaining direct strokes do not collapse
to empty AGG fixed-point polygons. Their scan conversion is real rendering
work. Broadening this rule to a width threshold, estimated coverage, or
file/page classifier would sacrifice exactness and risks repeating the missing
line regression seen in `0130`.

Keep `r25-8-0138` as the accepted experimental base. Do not extend `0145` and
do not reuse revision number `0145`.

## Separate Scheduling Issue

Q16 zoom logs also showed seconds of render-lane queue wait around native tile
work that often completes in tens to hundreds of milliseconds. `0145` changes
native raster execution only. Latest-visible request replacement and bounded
pending work remain a separate app-layer requirement.
