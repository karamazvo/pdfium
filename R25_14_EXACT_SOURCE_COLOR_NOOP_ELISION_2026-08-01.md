# r25-14 Exact Source-Color No-Op Elision

**Locked:** 2026-08-01 (Asia/Taipei)
**Status:** performance rejected by device result; keep for traceability and do not extend
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

## Device Result

The 2026-08-01 Samsung run rejected the performance hypothesis:

| Workload | 0144 result | Interpretation |
|---|---|---|
| 11.pdf | acquire 278 ms, bitmap 170 ms, total 449 ms; 5,396 pixels tested and zero skipped | Dominant work is 28,727 direct Darken paths. The 0144 mechanism is inactive; the good timing cannot be attributed to it. |
| Q16 | acquire 3,826 ms, bitmap 1,658 ms, total 5,484 ms, first visible 5,835 ms; 3,768,260 of 4,183,709 tested pixels skipped (90.1%) | Versus accepted 0138 at 3,185/1,465/4,651 ms and 4,899 ms first visible, acquisition regressed 20.1%, bitmap 13.2%, total 17.9%, and first visible 19.1%. A very high hit rate with no speedup proves destination channel arithmetic/write is not dominant. |
| EP23 human p2/p3 | p2 416/135/552 ms; p3 316/134/450 ms; dense Form hit ratio 103/296,507 (0.035%) and outer fill zero | Faster than the prior session, but 0144 is effectively inactive. Form caching and run variance explain the result, not source-color elision. |
| 6Steps | page 0 acquire 7 ms, bitmap 266 ms, total 273 ms, first visible 638 ms; later previews roughly 34-277 ms | Pages remain `canonical_no_exact_candidate`, so 0144 does not execute. The 20 ms fast-scroll goal remains unmet. |

The Q16 result is decisive even though this is one run: the proof counter shows
that the intended shortcut covered nearly all eligible destination pixels, but
the measured bitmap path still became slower. Do not add another destination
compositing shortcut on top of 0144.

The remaining Q16 replay cost is the 2,590,767 independent AGG raster passes,
including rasterizer reset, geometry-to-cell conversion, scanline generation,
and per-operation dispatch. The next exact experiment must remove raster passes
or prove device-space zero coverage before AGG, rather than optimize the final
channel write.

## Next Revision

The next unused revision is `0145`. It must restart from accepted `0138`,
exclude `0139`-`0144`, and target exact device-space zero-coverage elimination
before AGG. It must not add a telemetry-only build or another pixel-composite
micro-optimization.
