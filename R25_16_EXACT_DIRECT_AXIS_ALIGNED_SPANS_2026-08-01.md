# r25-16 Exact Direct Axis-Aligned Spans

**Locked:** 2026-08-01 (Asia/Taipei)

**Revision:** r25-16-0146

**Extends:** accepted r25-8-0138

**Excludes:** rejected r25-9-0139 through r25-15-0145

**Status:** device-invalidated because the production byte-order mode makes the
fast path unreachable; do not use as a performance base

## Root Cost

Q16's accepted full preview executes 2,590,767 independent raster passes.
`0143` generated exact axis-aligned coverage but still called the generic AGG
renderer for every row. `0144` optimized final color arithmetic but retained
AGG scan conversion. Neither isolated half improved bitmap time. `0146` tests
the only remaining useful combination: remove both layers for one exactly
provable destination/geometry class.

## Mechanism

For an existing accepted direct butt-stroke command, the executor constructs
the same four device-space vertices as `0138`. It enters the direct span path
only when all of the following are exact:

- the polygon is axis aligned by float equality;
- destination format is `kBgrx` or `kBgra`;
- source alpha is 255 and normal blending is in use;
- RGB byte order, backdrop, clip mask, and `full_cover` are absent;
- cap is butt, dash is empty, and geometry is finite and nondegenerate.

The rectangle is converted with AGG's signed 24.8 `poly_coord()` operation.
Edge and interior coverages use the same 8-bit area and aliased threshold. The
result is composited directly into the destination row with PDFium's exact
`AlphaMerge()` arithmetic. BGRx preserves its unused fourth-byte behavior;
BGRA uses the canonical destination-alpha and alpha-ratio equations, including
the transparent-destination case. This matters because Android page and tile
bitmaps are created as `FPDFBitmap_BGRA`. Every source operation remains
independent and is published immediately in source order.

Any mismatch returns to the unchanged `0138` four-vertex AGG raster path before
candidate pixels are written. The path has no page classifier, document name,
threshold, approximate angle, geometry merge, paint reorder, persistent cache,
heap allocation, lock, worker, JNI/Kotlin change, or UI-thread work.

## Accounting And Tests

The packet invariant is:

```text
lineRasterPasses + directAxisAlignedSpanDraws == lineBatchCommands
```

The replay log reports `directAxisAlignedSpanDraws`. Unit coverage compares the
direct opaque BGRx and BGRA results byte-for-byte with canonical `DrawPath()`
under fractional translation and rectangular clipping. The BGRA test exercises
both nonzero and zero destination alpha so both edge-pixel source-over branches
are covered. A translucent BGRx line must report zero direct spans and match
the unchanged AGG fallback.

## Acceptance

Accept only if:

1. The workflow applies exactly accepted `0138` plus `0146`; `0139-0145` are
   absent.
2. Android library and unit-test targets compile.
3. Corpus captures are pixel-identical to accepted `0138`.
4. Q16 reports material `directAxisAlignedSpanDraws` coverage and at least 20%
   lower repeated-median `bitmapRenderMs` without acquisition regression.
5. 11, EP23, Study Notes, 6Steps, cancellation, memory, and publication do not
   regress.

If coverage is high but bitmap time does not improve, reject `0146` and close
the per-primitive CPU-kernel direction. Do not broaden eligibility. Cold Q16
acquisition is unchanged by design.

## Device Result - 2026-08-02

Do not accept `0146`. The direct executor was inactive in every captured
replay:

| Workload | Replays | `directAxisAlignedSpanDraws` | Cold acquire / bitmap / total |
| --- | ---: | ---: | ---: |
| 11 | 26 | 0 in every replay | 287 / 387 / 677 ms |
| Q16 | 40 | 0 in every replay | 7,140 / 1,883 / 9,024 ms |
| EP23 | 101 in the dedicated capture | 0 in every replay | p2 1,060 / 381 / 1,441 ms; p3 821 / 360 / 1,182 ms |
| 6Steps continuation | 17 additional replays | 0 in every replay | first page 31 / 235 / 268 ms; common scrolling previews about 47-357 ms |

Q16 still executed 2,590,767 raster passes in its full preview. It reported
2,365,894 accepted direct butt strokes but zero direct spans, so geometry
acquisition and ordered replay were active while the new terminal executor was
not. The timing regression cannot be attributed to direct-span execution.

The root cause is an exact integration contradiction. Android creates an
`FPDFBitmap_BGRA` destination and always passes `FPDF_REVERSE_BYTE_ORDER` for
RGBA Android bitmap memory. That sets PDFium's `rgb_byte_order_`. The `0146`
eligibility predicate requires `!rgb_byte_order_`, so no production preview or
tile can enter the direct path. The BGRA support added by `0146` is therefore
insufficient for the app's actual BGRA-plus-reversed-byte-order contract.

This result invalidates the `0146` build, not the combined scan-and-composite
hypothesis. `0143` previously proved that 1,553,522 Q16 operations are exactly
axis aligned. A new revision may reproduce PDFium's canonical reversed-byte
RGBA channel and alpha equations directly and test them byte-for-byte. It must
still fail closed before destination mutation for every unsupported blend,
alpha, clip mask, backdrop, or geometry case. Do not amend `0146`, remove the
JNI reverse-byte flag, or weaken geometric eligibility.

Keep `r25-8-0138` as the accepted experimental base. The next unused revision
is `0147`.
