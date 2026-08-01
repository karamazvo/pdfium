# r25-14 Exact Source-Color No-Op Elision

**Locked:** 2026-08-01 (Asia/Taipei)
**Status:** implemented; CI build, pixel validation, and device performance acceptance pending
**Extends:** accepted `r25-8-0138`
**Excludes:** performance-rejected `0139`, `0140`, `0141`, `0142`, and `0143`
**Revision:** `r25-14-0144`

## Why This Revision

`0143` proved that a large orientation-specific raster shortcut was not the
remaining Q16 bottleneck. It handled 1,553,522 axis-aligned line draws but left
Q16 bitmap time effectively unchanged:

| Build | Acquire | Bitmap | Preview total |
|---|---:|---:|---:|
| accepted `0138` | 3,185 ms | 1,465 ms | 4,651 ms |
| `0143` device run | 3,978 ms | 1,478 ms | 5,456 ms |

`0143` is therefore rejected and is not in this build. `0144` returns to the
accepted renderer and targets repeated destination compositing work directly.

## Exact Invariant

For an opaque source color `S`, opaque destination pixel `D`, Normal blend,
and any exact AGG coverage `c`:

```text
D == S  =>  source_over(S, D, c) == D
```

The rasterizer, coverage, clip, operation count, and source order remain
unchanged. Only the final destination write and channel arithmetic are skipped
when the destination already has exactly the source color.

## Eligibility

The no-op check is enabled only inside the existing ordered AGG packet when all
of these are true:

1. Source alpha is exactly 255.
2. Blend mode is exactly Normal.
3. No backdrop/group-knockout bitmap participates.
4. Destination RGB or gray bytes exactly equal the source bytes.
5. A BGRA destination also has alpha exactly 255.

Any failed predicate executes the unchanged canonical compositor. Translucent,
Darken, backdrop, and unsupported formats cannot enter the shortcut.

## Architecture

```text
immutable RenderProgram
  -> ordered maximum-256 command packet
  -> independent AGG raster pass per source operation
  -> exact clip and coverage
  -> exact source-color destination predicate
       hit: preserve destination bytes
       miss: existing PDFium composite
  -> next operation in painter order
```

This is not a page classifier, paint merge, geometry union, alternate renderer,
or file-specific route. It applies to any already exact-lowered ordered stroke
or fill and fails closed at the pixel operation.

## Resource Policy

- No page-sized bitmap, cache, index, or retained representation is added.
- No heap allocation, lock, thread, JNI/Kotlin work, or UI-thread work is added.
- No additional path parsing, bounds calculation, or geometry calculation is
  added.
- The existing 256-command packet and cancellation cadence are unchanged.
- Memory growth is two counters per render packet/status, independent of page
  size.
- The normal canonical PDFium route remains unchanged.

## Proof Telemetry

The Android replay line adds:

```text
revision=r25-14-0144
mode=exact_source_color_noop_elision
sourceColorTestedPixels=<eligible destination pixels examined>
sourceColorNoOpPixels=<exact destination operations skipped>
```

The ratio `sourceColorNoOpPixels / sourceColorTestedPixels` measures useful
coverage. A high hit ratio with no timing gain proves channel compositing is not
dominant; a low ratio rejects the mechanism for this workload.

## Acceptance Gate

Accept `0144` only if all conditions hold:

1. Android PDFium and `pdfium_unittests` compile in CI.
2. Unit tests prove opaque repeated strokes are pixel-identical and produce
   no-op hits; translucent strokes remain on the existing compositor and are
   pixel-identical.
3. Lossless bitmap comparisons match `0138` for the correctness corpus,
   including normal pages, 11, Q16, EP23, and 6Steps.
4. Repeated cold Q16 median bitmap time improves by at least 20% from `0138`
   without acquisition regression.
5. 11, EP23, Study Notes, and 6Steps remain within normal run variance.
6. Cancellation, visible publication, memory bounds, and canonical fallback
   behavior remain unchanged.

Reject the performance result if hit ratio is low, bitmap gain is below 20%,
or the added comparison branch regresses ordinary or non-overlapping content.
Correctness alone is not sufficient to extend this experiment.

## Next Revision

The next unused revision is `0145`. It must be selected from measured `0144`
results and must not reintroduce `0139`-`0143` without new proof.
