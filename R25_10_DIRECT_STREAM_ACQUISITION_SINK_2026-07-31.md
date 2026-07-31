# r25-10 Direct Stream Acquisition Sink

**Locked:** 2026-07-31 (Asia/Taipei)

**Status:** implemented, pending Android build and device acceptance

**Extends:** r25-8-0138

**First revision:** r25-10-0140

**Excluded:** r25-9-0139, which did not pass its replay-performance gate

## Measured Boundary

The 0138 Q16 cold preview established:

```text
parseUs=3184549
finishUs=21528
acquireMs=3185
bitmapRenderMs=1465
directLineRuns=187859
spatialLineRunCommands=2940110
```

Final sealing is below one percent of acquisition. Replay was already improved
by 0138. The remaining dominant cold cost is parsing and constructing the
exact RenderProgram.

The accepted parser run is bounded to 256 commands, but progressive parse
budgets make the observed average only about 15.65 commands:

```text
2940110 commands / 187859 runs = 15.65 commands per run
```

Before 0140, every run created and initialized:

```cpp
std::array<VeloceParsedTranslationOriginLine, 256>
std::array<uint32_t, 256>
```

The fixed arrays reserve roughly 5 KiB of stack for each short progressive
run. Their source-level initialization could represent close to 960 MiB over
the observed run count, but an optimizing compiler may eliminate unused tail
initialization, so that figure is not treated as measured memory traffic.

The unavoidable duplicate work is smaller but still structural: about 2.94M
decoded 16-byte lines are written to staging, reread for payload/run
construction, and reread for spatial construction. Unit-start entries are
also written for every staged command even though only the first uncommitted
position is needed on rollback.

## MuPDF Architectural Lesson

MuPDF does not require a temporary page-object-shaped batch between content
interpretation and its display list:

- `source/pdf/pdf-interpret.c` dispatches PDF operators through one processor
  interface.
- `source/pdf/pdf-op-run.c` maps interpreted operators directly to device
  operations.
- `source/fitz/list-device.c` records a stateful, packed, variable-length
  command stream. Unchanged state remains implicit and the list grows in one
  authoritative order.

The transferable principle is:

> Decode an operator once and immediately submit it to the one authoritative
> rendering representation. Do not materialize an intermediate batch that is
> copied and rescanned before it reaches that representation.

0140 applies that principle without copying MuPDF's object model, rasterizer,
or renderer. PDFium remains responsible for parsing semantics, canonical page
objects, editing, fallback, rendering, clipping, and compositing.

## r25-10-0140 Mechanism

The exact translation-origin parser and RenderProgram builder now form a
bounded pull stream:

```text
PDF bytes
  -> existing transactional exact tokenizer
  -> one 16-byte parsed line
  -> existing exact RenderProgram builder
       -> payload or exact no-op ordinal
       -> native run
       -> aggregate bounds
       -> 32-command spatial block
  -> next PDF operator
```

For each accepted line, one builder pass now performs all representation work
in source order. The parser no longer allocates or initializes the two
256-entry staging arrays, and the builder no longer performs a later spatial
scan of that batch.

The existing span-based builder API remains as a compatibility adapter and
feeds the same stream sink. There is still one authoritative lowering path.

## Correctness Invariants

0140 changes no eligibility proof and no retained format:

1. The existing PDFium transactional tokenizer recognizes the same exact
   grammar and uses the same number conversion.
2. The same matrix arithmetic and evaluation order produce each endpoint and
   translation.
3. The same immutable graphics-state, clip, visibility, transform, finite
   value, command-count, and memory checks must succeed.
4. Every committed command keeps the same source ordinal, opcode, payload
   rank, exact no-op bit, native run, bounds, and spatial candidate bit.
5. On the first reader or builder failure, the parser rewinds to the first
   uncommitted PDF operator and canonical PDFium resumes there.
6. Painter order, replay, clipping, rasterization, blending, and pixels remain
   those of 0138.
7. RenderProgram format remains version 24.

The callback is synchronous and stack-owned. It cannot escape parse lifetime
or retain PDFium pointers.

## Resource and Thread Bounds

- Existing parser work quantum remains at most 256 commands.
- Transient line storage falls from two fixed arrays to one 16-byte line.
- Existing 3M-line and 96 MiB RenderProgram limits remain unchanged.
- Existing 32-command spatial blocks remain unchanged.
- No persistent allocation, second representation, cache, classifier,
  threshold, scheduler, thread, lock, JNI/Kotlin path, or UI-thread work is
  added.
- Canonical-only pages never call the direct stream sink.
- Cancellation and progressive parse boundaries remain unchanged.

## Device Proof

The compile log adds:

```text
parserStreamSinkLines
```

Expected Q16 invariants relative to 0138:

```text
parserStreamSinkLines == spatialLineRunCommands
commands, omittedPageObjects, retainedPageObjects unchanged
compactTranslationLines, exactNoOpLines, nativeRuns unchanged
spatialBlocks, spatialCoveredCommands, bytes unchanged
replay counters and pixels unchanged
```

## Acceptance

Accept 0140 only when repeated same-device cold medians show:

1. Q16 exact representation, memory, spatial, replay, and pixel counters match
   0138.
2. `parserStreamSinkLines` covers the same roughly 2.94M accepted run
   commands.
3. Q16 `parseUs` and `acquireMs` improve by at least 10 percent from the 0138
   median. Twenty percent, or at most about 2.55 seconds from the measured
   3.18 seconds, is the target rather than a promised result.
4. Q16 `finishUs` and bitmap/replay time remain within controlled noise.
5. Q16 preview and zoom table lines remain complete.
6. 11.pdf, EP23, Study Notes, 6Steps, and ordinary canonical pages remain
   pixel-correct and within controlled timing noise.
7. First-visible latency improves consistently without extra memory, lock
   contention, UI-thread work, or cancellation delay.

If acquisition improves by less than 10 percent in controlled A/B/A testing,
stop this staging-removal direction. The next cold-path project would require
a broader direct typed content interpreter with the same fail-closed canonical
boundary, not another run-size adjustment or file-specific policy.
