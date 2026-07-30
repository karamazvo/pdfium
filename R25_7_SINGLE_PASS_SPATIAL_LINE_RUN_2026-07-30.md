# r25-7 Single-Pass Spatial Line Run

**Locked:** 2026-07-30 (Asia/Taipei)
**Status:** implemented, pending Android build and device acceptance
**Extends:** accepted r25-6-0136
**First revision:** r25-7-0137

## Root Cause

The 0136 Q16 result separates the remaining cold cost:

```text
parseUs   4,295,680
finishUs     32,448
replayUs  2,082,000
```

Final sealing is below one percent of acquisition. Deferring only the completed
spatial-index object would therefore save little and require a later scan of
roughly three million commands. That would duplicate work and move latency into
the first zoom tile.

The actual acquisition redundancy is inside the accepted translation-origin
line run. For every drawable line, 0136:

1. creates an identity-plus-translation matrix;
2. transforms the endpoint;
3. creates and normalizes a rectangle;
4. recalculates the same stroke inflation;
5. unions the rectangle into page bounds;
6. dispatches the rectangle into the spatial block builder.

The compact parser run already owns the endpoint, translation, common line
width, source ordinal, and exact no-op decision. Those operations do not need a
second per-line object-shaped path.

## Invariant

> Every accepted command is consumed once in source order. The same pass must
> produce its compact payload, ordered native/no-op representation, aggregate
> line bounds, and spatial block membership. No later phase may rescan the tape
> to recover information available during that pass.

## r25-7-0137 Mechanism

For the existing bounded translation-origin line run:

- calculate the conservative stroke inflation once from the shared exact line
  width;
- calculate each endpoint directly as `translation + local endpoint`;
- update aggregate native bounds with scalar min/max;
- update the active 32-command spatial block directly with the same values;
- preserve exact no-op source ordinals without geometry or candidate bits;
- continue using the existing 256-command parser-stack bound;
- abandon only the spatial index if its existing block ceiling is exceeded,
  while continuing to calculate complete native bounds for executor preflight.

The union is mathematically identical to the previous representation because
all accepted commands have identity linear transforms and the same line width:

```text
union(expand(line_i endpoints, shared_margin))
  = expand(union(all line_i endpoints), shared_margin)
```

The implementation still updates the active block in source order so block
boundaries, no-op positions, and candidate masks remain byte-for-byte
compatible with format 24.

## Deliberately Not Added

- no deferred spatial-index build;
- no second command scan;
- no mutable lazy sidecar;
- no cache or cache lock;
- no page/document classifier or threshold;
- no additional heap allocation;
- no JNI or Kotlin policy;
- no UI-thread work;
- no raster batching or pixel change.

Canonical-only pages never enter the translation-origin run and execute the
same PDFium path as 0136.

## Acceptance

Accept 0137 only when repeated same-device medians show:

1. Q16 `spatialLineRunCommands` covers the accepted bulk line commands.
2. Commands, retained/omitted objects, compact translation lines, exact no-ops,
   native runs, spatial blocks, spatial covered commands, unbounded blocks,
   program bytes, replay work, and pixels match 0136.
3. Q16 `parseUs`, `acquireMs`, and first-visible time improve materially.
4. Q16 preview and zoom table lines remain complete.
5. 11.pdf, EP23, Study Notes, and 6Steps remain within controlled timing noise
   and retain correct pixels.
6. Memory remains under the existing 96 MiB program cap and cancellation
   behavior is unchanged.

## Next Gate

If 0137 does not reduce Q16 acquisition by at least 15 percent in repeated
medians, stop acquisition micro-optimization. The next revision must target
the measured 2.08-second replay cost through an exact dense ordered raster-tape
executor. A bounded immutable compiled-program cache follows only after the
representation and executor stabilize; it targets warm acquisition and must
not be used to hide unresolved cold cost.
