# r25-6 Single-Pass Exact Line Run

**Date:** 2026-07-30 (Asia/Taipei)
**Revisions:** r25-6-0133 through r25-6-0136
**Base:** r25-4-0129
**Excluded:** r25-4-0130 and r25-5-0131

## Decision

0130 and 0131 are experiments, not the base of this series. Q16 showed missing
lines with 0130 in both preview and zoom rendering. 0133 therefore returns to
the 0129 representation and executor.

The invariant is:

> An optimization may remove parser dispatch, but it may not create a second
> semantic lowerer or change the command representation, painter order, matrix
> arithmetic, clipping, visibility, fallback, or raster execution.

## Root Cost

After one exact compact line activates the RenderProgram lowerer, 0129 returns
to the outer content-parser loop for every following line. Each command repeats
the same parser eligibility checks, stream-index lookup, and general operator
dispatch even when the next bytes are another exact balanced line unit.

The repeated control flow is redundant. Numeric conversion, matrix composition,
budget accounting, and command emission are not redundant and remain. Exact
tight stroke bounds are also unnecessary: the spatial index only requires a
conservative rectangle that never excludes visible coverage.

## 0133 Mechanism

0133 adds one bounded run loop inside `CPDF_StreamContentParser`:

1. It starts only after the existing exact lowerer has a valid parsed-line
   context.
   It is folded into the existing exact-line probe, so canonical pages do not
   pay for a second speculative parser call.
2. It accepts only the exact grammar
   `q 1 0 0 1 tx ty cm 0 0 m dx dy l S Q`.
3. It does not cross a PDF content-stream boundary.
4. Every command is submitted through the existing
   `RecordVeloceParsedLine()` path. There is no duplicate chunk lowerer.
5. The first token, state, boundary, budget, or lowering mismatch stops the run.
6. A parsed command rejected by native lowering is materialized immediately as
   a canonical PDFium path at the same source ordinal.
7. A run is bounded by the remaining progressive parse budget, or 256 commands
   when the caller supplies no budget.
8. For the exact translation-origin grammar, bounds are built from the
   transformed endpoints and inflated by the full affine transform of a
   conservative local margin. This removes the former per-command `hypot` and
   four-corner rectangle transform while preserving a superset of the former
   tight bounds under scale, rotation, and shear.

No new retained command format, cache, heap buffer, candidate index, raster
batch, classifier, document rule, or page threshold is introduced.

## Expected Result

This revision targets cold acquisition only. Replay pixels and replay cost
should match 0129.

For dense homogeneous streams, acquisition logs should show:

```text
revision=r25-6-0133
mode=single_pass_exact_line_run
directLineRuns=...
directLineRunCommands=...
```

`directLineRunCommands` should cover most compact translation lines. Compare
`parseUs`, `finishUs`, and total first render against 0129. The change removes
outer dispatch overhead and tight-bound math, but it does not remove essential
numeric conversion, matrix composition, storage, or raster work. A 20-40%
acquisition improvement is a reasonable target for streams dominated by this
exact grammar, not a guaranteed x10 result.

## Acceptance

1. Q16 table lines match canonical PDFium in preview and zoom tiles.
2. Normal-page output and first-visible latency do not regress.
3. 11.pdf and EP23 output remain unchanged.
4. The workflow proves that 0130 chunk-lowering and 0131 disjoint-raster symbols
   are absent.
5. The patch contains no filename, page number, document classifier, or command
   count threshold.
6. `TranslationLineConservativeBoundsContainPriorTightBounds` proves that the
   fast conservative bounds contain PDFium's previous tight line bounds.

## 0134 Result

0134 fused fixed-token recognition and exact numeric decoding into the
transactional scanner. Q16 preserved all representation and replay counts, but
the measured improvement was small:

- `parseUs`: 4,980,142 to 4,821,771, 3.18% faster.
- `acquireMs`: 4,980 to 4,823, 3.15% faster.
- Total preview: 6,958 to 6,854 ms, 1.5% faster.
- First visible: 7,327 to 7,269 ms, 0.8% faster.
- Replay: 1,977 to 2,031 ms, within adverse run variance.

This fails the documented 10% acquisition criterion. Numeric decoding is not
the dominant Q16 acquisition cost.

## 0135 Final Acquisition Experiment

0135 replaces per-command parser-to-holder-to-builder submission for the exact
translation-origin grammar with one bounded authoritative bulk lowerer:

1. The parser stores at most 256 decoded commands in worker-stack arrays.
2. Matrix linear terms are composed once. Per-command translation arithmetic
   uses the same operation order as PDFium's prior matrix multiplication.
3. The builder validates immutable general/color/graph state identity, clip,
   visibility, and transform class once for the batch.
4. It pre-reserves one exact payload prefix under the existing 3M-line and
   96 MiB caps before changing source ordinals.
5. The prefix is committed directly to the existing compact line payload,
   native run, line run, bounds, and bounded spatial structures.
6. A non-finite command, exact butt-cap no-op, state mismatch, transform
   mismatch, or exhausted budget stops before that command. The parser rewinds
   to its source byte and canonical PDFium resumes there.
7. Scalar translation-origin lowering delegates to the same bulk primitive
   with a one-command span whenever its exact preconditions hold.

There is no document classifier, threshold, second retained representation,
heap batch, cache, lock, JNI/Kotlin change, UI-thread work, painter-order
change, or raster change.

### Hard Decision Gate

Accept 0135 only if repeated same-device measurements show:

1. More than 20% lower Q16 `parseUs` and `acquireMs` versus 0134.
2. Identical source command, omitted/retained object, native opcode, compact
   payload, native-run, spatial, program-byte, and replay-work counts.
3. Pixel-identical Q16 preview and zoom tiles, including table lines.
4. No material 11.pdf, EP23, Study Notes, or 6Steps regression.

If criterion 1 fails without an implementation defect, stop this acquisition
micro-optimization direction. The remaining costs are structural: per-command
bounds and spatial construction during acquisition, and independent line
raster passes during replay. Any further performance work must start as a
separately reviewed architecture with its own correctness proof and baseline.

## 0135 Device Result And Root Cause

0135 failed because its implementation violated the single-pass invariant, not
because the bulk-line model was disproved:

- Q16 `parseUs` regressed from 4,821,771 to 15,818,458.
- Total preview regressed from 6,854 ms to 17,640 ms.
- Replay improved slightly from 2,031 ms to 1,821 ms.
- Final commands, compact payloads, exact no-ops, program bytes, and replay
  shape remained identical.
- `directLineRuns` grew from 187,860 to 507,173.

The bulk builder treated every exact zero-coverage butt-cap line as a packet
terminator. Q16 contains 574,229 such commands. The parser therefore rewound at
each no-op, parsed it through the scalar path, and then restarted the following
suffix. This repeated parser/operator work while producing the same final
RenderProgram.

## 0136 Single-Pass Correction

0136 restores the actual invariant:

> Every accepted command in a bounded homogeneous packet is interpreted once,
> in source order. An exact no-op remains an ordered command in that packet,
> but consumes only its existing rank bit and no geometry payload.

The correction:

1. Keeps finite exact no-ops inside the current 256-command worker-stack
   packet.
2. Calls the existing `AppendExactNoOp()` representation at the original
   command ordinal.
3. Continues lowering later drawable lines without rewinding or reparsing the
   packet suffix.
4. Preserves the existing compact payload, native-run, line-run, spatial,
   painter-order, cancellation, and canonical fallback contracts.
5. Uses the existing cached byte reservations, 3M-line ceiling, and 96 MiB
   retained-program ceiling. It adds no allocation, cache, classifier, thread,
   lock, JNI/Kotlin path, or UI-thread work.
6. Fails closed at the first invalid or over-budget command and returns only
   the committed prefix, so canonical PDFium resumes at exactly the first
   uncommitted source byte.

The unit test proves a drawable/no-op/drawable packet is consumed in one call,
the no-op ordinal remains set, and the two drawable commands map to adjacent
payload ranks across that no-op.

### 0136 Acceptance

1. Q16 `directLineRunCommands`, commands, compact payloads, exact no-ops,
   native runs, program bytes, replay work, and pixels match 0134/0135.
2. Q16 `directLineRuns` falls materially below 0135; packet count is governed
   by the 256-command bound plus real stream/state boundaries, not no-op count.
3. Repeated-median Q16 `parseUs` and total preview recover to at least the 0134
   range and improve by more than 20% versus 0135.
4. 11.pdf, EP23, Study Notes, and 6Steps remain within normal run variance.
5. Any later x10 work starts a new structural series; 0136 closes this
   acquisition regression and does not claim to remove raster cost.
