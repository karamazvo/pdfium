# r25-3 Unified Owned RenderProgram

**Locked:** 2026-07-22 (Asia/Taipei)

## Decision

Generation 3 uses one ordered rendering program for every parsed holder. Page
size, path count, filename, and Kotlin classification do not select a rendering
approach. Each object is compiled once, in source painter order, into either:

- an exact owned native opcode, or
- a canonical PDFium barrier at the same ordinal.

Unsupported semantics are not approximated. They remain canonical barriers.
The native executor uses PDFium's existing render device and writes into the
same destination bitmap as canonical PDFium.

## r25-3-0103: Owned Unchanged-Holder Replay

Patch: `patches/ship/0103-veloce-render-program-owned-unchanged-holder-replay.patch`

This is a behavioral performance patch, not telemetry-only work:

1. Native opcode compilation starts at command zero. The old 4096-command
   page-routing threshold is removed.
2. Replay checks the source mutation epoch once before any program pixel. A
   mismatch returns to canonical PDFium before drawing.
3. Native commands read owned geometry, transform, graph state, color, clip,
   and bounds. They do not fetch or inspect their live page object.
4. Optional-content visibility is evaluated once per exact recorded visibility
   run. Canonical barriers continue to use normal PDFium visibility handling.
5. Live object lookup is restricted to canonical barriers, one representative
   per visibility run, and exceptional device fallback.
6. Top-level non-group pages admit exact Darken commands. PDFium sets a page's
   isolated bit by default even when no transparency `/Group` exists; that bit
   is no longer mistaken for an isolated group. Real groups and nested contexts
   remain conservative.
7. The existing 96 MiB retained-program limit remains. No per-page-object
   field, image-sized scratch bitmap, candidate allocation, scheduler, lock, or
   UI-thread operation is added.

## Expected Proof

For Q16-class CAD pages, the old executor performed live object lookup,
dirty/active/bounds inspection, and optional-content evaluation for roughly
2.94 million native lines. The measured page has seven visibility runs. In a
successful 0103 replay:

```text
revision=r25-3-0103
mode=owned_ordered_program
visibilityRunChecks=7
nativeObjectLookups=7
lineBatchCommands~=2937165
```

Exceptional driver fallback can increase `nativeObjectLookups`; ordinary
native replay must not make it track `nativeDraws`.

For 11.pdf, the measured program contains 24,701 exact native Darken paths. A
top-level render should report:

```text
darkenContextDirect=1
darkenDraws~=24701
darkenFallbacks~=0
```

Pixel comparison remains mandatory. Performance acceptance requires lower
`replayUs` and end-to-end visible render latency, not counters alone.

## Remaining Cost, In Order

0103 removes object-model overhead but does not eliminate Q16's 220,968
canonical even-odd fill barriers or the raster work for 2.94 million visible
lines. The next revisions must extend the same program, not add another page
path:

1. **r25-3-0104: exact owned fill opcode.** Own simple solid fill geometry,
   fill rule, color, matrix, clip, and visibility under the same fail-closed
   predicates. Execute it in the ordered stream. This removes canonical object
   machinery for Q16's `f*` barriers and creates a complete native operation
   stream without crossing painter-order barriers.
2. **r25-3-0105: ordered AGG operation executor.** Send bounded consecutive
   owned operations to one render-device executor. Preserve each operation's
   rasterization and composite order while reusing AGG state and scratch across
   stroke/fill boundaries. This targets the current 167K render-status packet
   dispatches without merging objects or changing pixels.
3. **r25-3-0106: compile-time spatial hierarchy over all owned operations.**
   Extend conservative holder-space blocks/leaves to fills and paths. Query
   visible tiles without candidate allocation; unknown bounds stay visible.

These are opcode and executor capabilities, not page classifiers. A normal
page and a huge-path page use the same representation and entry point; they
only contain different exact opcodes.

## Invariants

- One source order, one destination bitmap, one pixel owner at each ordinal.
- No native-to-canonical restart after native pixels.
- No heuristic equivalence, barrier crossing, filename rule, or object-count
  routing.
- Unknown semantics remain canonical in place.
- Mutation, unsupported context, or invalid program shape fails before pixels.
- Fixed retained-memory ceilings and stack-bounded replay packets.
- No UI-thread rendering, new worker pool, global render lock, or tile flood.
