# r25-6 Single-Pass Exact Line Run

**Date:** 2026-07-29 (Asia/Taipei)
**Revision:** r25-6-0133
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
