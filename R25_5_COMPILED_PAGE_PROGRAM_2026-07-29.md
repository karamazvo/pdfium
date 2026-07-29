# r25-5 Compiled Page Program

**Locked:** 2026-07-29 (Asia/Taipei)
**Extends:** accepted r25-4-0130
**First revision:** r25-5-0131

## Objective

Approach MuPDF-class rendering cost without replacing PDFium's parser, page
objects, editing model, graphics device, or bitmap. The authoritative PDF
content stream is interpreted once into an immutable, ordered program. Exact
native packets remove repeated interpretation and raster setup; operations that
cannot be lowered exactly remain canonical PDFium operations at the same source
ordinal.

This series targets order-of-magnitude gains only where the document contains
enough exactly lowerable repeated work. It does not claim a universal 10x gain.
Canonical-only pages must retain native PDFium behavior and cost.

## Architecture

```text
authoritative PDF content stream
              |
              v
one ordered semantic interpreter
       | exact lowering     | lowering fails
       v                    v
native render packet    canonical PDFium operation
       \                    /
        \ exact source order
         v
immutable compiled page program
         |
         v
ordered holder-space block directory
         |
         v
visible block executor
         |
         v
same PDFium render device and destination bitmap
```

The compiled page program is a derived sidecar, never a second document model.
PDFium remains the source of truth for parsing, editing, saving, resources,
color, transparency, clipping, and unsupported operations.

## Non-Negotiable Invariants

1. Every source operation has one position in painter order.
2. Native lowering commits only after exact semantic validation.
3. A failed or unsupported operation remains canonical at the same position.
4. No executor may restart the whole holder after writing native pixels.
5. Spatial data may remove invisible candidates but never reorder operations.
6. Raster operations may share a pass only with a proof of identical pixels.
7. Bounds are conservative; false positives are allowed, false negatives are
   forbidden.
8. Cached programs contain immutable owned values, never live page-object
   pointers.
9. All caches and packet scratch have explicit byte or entry ceilings.
10. Canonical-only pages do not allocate, compile, index, or schedule native
    program work.
11. Rendering remains off the UI thread and introduces no new global lock.
12. Editing or mutation invalidates the sidecar before later rendering.

## Revisions

### r25-5-0131: Exact Disjoint Raster Packets

Root cause addressed: r25-4 amortizes packet dispatch and transform setup, but
Q16 still performs about 2.59 million AGG raster passes for 2.59 million native
line draws.

0131 combines direct butt-cap line polygons in one AGG raster pass only when
their conservative device-pixel support rectangles are pairwise disjoint.
Disjoint destination pixels prove that source-over order cannot change the
result. A fill, overlap, render-option change, unsupported stroke, or fixed
packet-capacity boundary flushes before the next operation.

The proof uses each exact four-vertex stroke polygon transformed to device
space and inflated by a two-pixel anti-alias halo. Scratch is a fixed array of
16 rectangles on the render stack. There is no heap allocation, page
classification, document threshold, cache, thread, lock, JNI, Kotlin, or
UI-thread work.

Acceptance:

- disjoint line packets match independent canonical `DrawPath()` pixels;
- overlapping translucent lines remain separate raster passes and match
  canonical source-over pixels;
- `lineRasterPasses <= nativeDraws`;
- `coalescedSourceDraws > coalescedRasterPasses` on suitable dense geometry;
- Q16 replay improves only in proportion to the proven coalescing ratio;
- 11.pdf, EP23, Study Notes, and 6-Steps remain pixel-correct;
- canonical-only pages do not execute this code.

0131 is a real executor optimization, but it does not remove Q16's cold stream
acquisition cost. It is not by itself the complete 10x architecture.

### r25-5-0132: Self-Contained Ordered Program

Move canonical fallback ownership out of the transient page-object holder and
into an immutable program-owned representation. Native packets and canonical
barriers then have one lifetime and one ordered owner. The page holder can
materialize ordinary PDFium objects from the authoritative stream on editing or
enumeration.

This phase is accepted only if replay no longer depends on cached raw object
pointers and concurrent renders cannot mutate shared program state.

### r25-5-0133: Bounded Compiled-Program Cache

Cache complete immutable programs by exact document, page-stream generation,
resources generation, and render-program format. A cache hit skips content
interpretation; it never skips resource validation or mutation invalidation.
Use a strict process byte ceiling and LRU eviction.

This is the phase capable of an order-of-magnitude warm-render gain. It must not
cache live PDFium pointers or retain unbounded page state.

### r25-5-0134: Single-Pass Typed Stream Compiler

Replace duplicated generic-token and path-local handling with one exact typed
interpreter that emits native packets or canonical barriers directly. It must
preserve PDF number conversion, graphics-state semantics, source ordinals, and
stream-boundary behavior.

This is the remaining cold-acquisition change. The target is Q16 cold
acquisition below two seconds, but acceptance is based on measured end-to-end
cost and pixel equivalence rather than the target alone.

## Measurement Contract

For 0131, Android replay logs expose:

```text
revision=r25-5-0131
mode=exact_disjoint_raster_packets
lineBatchCommands
lineRasterPasses
coalescedSourceDraws
coalescedRasterPasses
replayUs
```

The number of eliminated passes is:

```text
coalescedSourceDraws - coalescedRasterPasses
```

A build is not considered improved because the new path executed. It must show
lower end-to-end acquisition or replay time in controlled repeated runs, stable
command and omission counts, and correct pixels across dense and canonical
pages.
