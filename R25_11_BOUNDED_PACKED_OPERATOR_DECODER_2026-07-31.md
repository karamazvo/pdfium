# R25-11 Bounded Packed Operator Decoder

**Locked:** 2026-07-31 (Asia/Taipei)

**First revision:** r25-11-0141

**Extends:** accepted r25-8-0138

**Excludes:** unaccepted r25-9-0139 and r25-10-0140

## Status

r25-11-0141 is the first acquisition patch in the new order-of-magnitude
performance series. It is experimental until device measurements pass the
acceptance gates below.

It is a real implementation change, not telemetry-only work. It does not by
itself claim or guarantee a 10x improvement.

## Root Cause

Q16 cold preview still processes roughly 2.94 million exact repeated line
operators. The accepted representation already stores each retained compact
line in 16 bytes, but acquisition repeatedly enters the general token helpers
for every fixed operator and constant in:

```text
q 1 0 0 1 tx ty cm
0 0 m
dx dy l
S
Q
```

r25-10-0140 attempted to remove staging by interleaving parser callbacks with
RenderProgram mutation. That destroyed the useful phase boundary between
sequential byte scanning and sequential program construction. Its device run
also had broad runtime variance, and it did not pass the performance gate.

MuPDF's useful principle is not merely "call a sink for each operator." Its
display list is a compact append-only representation consumed sequentially.
The corresponding PDFium mechanism must preserve locality on both sides of the
parser/builder boundary.

## Invariant

The ordinary PDFium parser remains authoritative. Native acquisition may
consume a source prefix only when every byte, number, operator, content-stream
boundary, immutable state, budget check, and source ordinal is represented
exactly. Any mismatch resumes canonical parsing at the first unconsumed byte.

## r25-11-0141 Mechanism

1. Start only after the existing exact semantic line context has been proven.
2. Match one common exact byte layout without a document or page classifier.
3. Decode at most 256 commands into the existing 16-byte worker-stack records.
4. Stop at the current content-stream boundary.
5. Fold the two matrices over the bounded batch in the content parser.
6. Submit the batch once to the existing authoritative RenderProgram builder.
7. Rewind to the first uncommitted source command on a lowering or budget
   boundary.
8. Fall back immediately to the general grammar when the exact byte layout
   does not match.

The packed path adds no persistent allocation, cache, thread, lock, JNI/Kotlin
work, UI-thread work, document threshold, or second rendering representation.
The existing 3M-line limit and 96 MiB RenderProgram cap remain authoritative.

## Why This Is Different From 0140

```text
0140:
tokenize one line -> callback -> mutate several builder structures -> repeat

0141:
sequentially decode <=256 lines -> sequentially commit one bounded batch
```

0141 preserves cache locality and keeps ownership simple. The parser owns its
transaction and the builder owns RenderProgram mutation.

## Correctness Gates

- `sourceCommands`, retained/omitted objects, native commands, native runs,
  compact line counts, no-op counts, spatial counts, and `programBytes` match
  0138.
- Replay counters and pixels match 0138.
- A byte-layout mismatch leaves the parser position unchanged.
- A content-stream boundary cannot be crossed.
- Partial builder commitment rewinds to the first uncommitted command.
- Normal pages continue to report `canonical_no_exact_candidate`.

## Performance Gates

Use cold runs only; do not add warmup.

- `packedLineRunCommands` must cover most Q16 direct line commands.
- Repeated cold Q16 `parseUs` and `acquireMs` must improve by at least 20%
  versus controlled 0138 runs to retain this mechanism.
- 11.pdf is the environmental control because the packed decoder is inactive
  there. Compare Q16 only when 11.pdf timing differs by no more than 10-15%.
- EP23 and ordinary pages must remain within timing noise.

If the decoder coverage is high but acquisition improves by less than 20%,
the fixed-token parser is no longer the dominant acquisition cost. Do not add
another token micro-optimization.

## Remaining X10 Work

0141 addresses cold acquisition only. Order-of-magnitude performance also
requires a later exact executor change that reduces the remaining per-line
raster work without merging overlapping PDF paint operations. Previous 0131
data proved that pairwise-disjoint coalescing removes less than 1% of Q16 full
replay raster passes, so that experiment must not be revived as the primary
executor strategy.

The next executor revision is justified only after 0141 measurements identify
the new dominant cost. It must consume immutable packed spans directly, remain
clip/visibility/state ordered, provide bounded cancellation points, and fall
closed before its first pixel.
