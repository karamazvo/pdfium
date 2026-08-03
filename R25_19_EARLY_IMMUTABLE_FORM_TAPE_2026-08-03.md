# r25-19-0149 Early Immutable Form Tape

**Locked:** 2026-08-03 (Asia/Taipei)

**Revision:** `r25-19-0149`

**Performance parent:** accepted `r25-8-0138`

**Excluded:** rejected `0139` through `0147` and diagnostic-only `0148`

## Measured Root Cause

The `0148` phase profile showed that allocation, render-lane waiting, and JNI
coordination are not the dominant native costs:

- 11.pdf spent about 99.6% of active bitmap time in its accelerator.
- Q16 cold preview spent about 3,677 ms acquiring its root program and about
  1,630 ms replaying it.
- EP23 page 2 acquired its Form program in about 852 ms. Page 3 reported a
  Form-program cache hit but still spent about 844 ms in acquisition.
- The repeated EP23 Form stream contained 28,071 opaque fills and 707,225
  points; its immutable program retained about 10.3 MiB.
- Typical render-session waits and bitmap allocation were below one
  millisecond.

The EP23 cache hit was therefore too late. `CPDF_Form::ParseContentInternal()`
looked up the cached program before parsing, but it still ran the complete
content parser and materialized every child `CPDF_PageObject`. Only after that
work did it attach the cached program.

## Invariant

An immutable Form program may replace repeated child materialization only when
the program proves that every source command is native and replay cannot look
up a canonical child object. Canonical PDFium objects remain the source of
truth and must be reconstructible before enumeration, editing, printing,
Type3 rendering, mutation fallback, or any failed native preflight.

## Mechanism

The existing document-owned Form cache now stores one bounded immutable value:

```text
exact invocation key
  -> shared all-native RenderProgram
  -> exact content bounding box
  -> background-alpha metadata
```

On an exact cache hit, before constructing `CPDF_ContentParser`:

1. Copy the invocation graphics state and optional parent matrix into the
   Form holder. These inline optionals are used only if canonical children are
   later requested.
2. Attach the cached immutable program.
3. Set the holder's source-command count and omitted-object count to the exact
   program command count while retaining zero child objects.
4. Reuse the cached exact content bounds and alpha metadata for the parent
   Form object.
5. Render from the program only if `CanReplayWithoutPageObjects()` proves that
   the valid program has one native command for every source command and no
   mandatory canonical range.

If canonical child access is required, `CPDF_Form::EnsureCanonicalPageObjects`
uses the original PDF stream and saved invocation state to run PDFium's normal
parser once with native recording disabled. The cached bounds are discarded,
the canonical list becomes authoritative, and normal editing APIs continue
unchanged.

## Correctness Boundary

The early path is fail-closed:

- Type3 Forms never use the cache.
- Forms that perform any resource lookup are never cached.
- Mixed native/canonical programs are never detached from their child table.
- Cache keys retain the stream generation, Form dictionary state, initial
  graphics state, parent matrix, transparency flags, and mutation epoch.
- A global object mutation invalidates replay before pixels and forces
  canonical materialization before bounds are reused.
- Legacy PathDL sees omitted objects and declines before scanning the holder;
  the ordered RenderProgram executor remains the only accelerated pixel owner.
- Painter order, clipping, visibility, blend behavior, and native geometry are
  unchanged from `0138`.

## Bounds

- The existing Form cache remains capped at 16 entries and 96 MiB.
- No second program cache, classifier, threshold, scheduler, thread, lock,
  JNI path, Kotlin path, or UI-thread work is added.
- Each early-adopted live Form retains two inline optional invocation values
  and one exact rectangle; it does not allocate thousands of child objects.
- Cancellation cadence and the accepted `0138` replay bounds are unchanged.

## Expected Device Result

This revision targets repeated exact Form acquisition:

- EP23 page 3 should log `result=early_hit` and `skippedObjects=28071` (or the
  exact command count in that build). Its roughly 844 ms repeated acquisition
  should collapse to cache lookup plus holder setup.
- EP23 page 2 remains the cold producer and is expected to remain close to its
  existing first-use compile time.
- Q16 cold root acquisition and dense replay are not changed by this revision.
- 11.pdf Darken replay is not changed.
- Canonical normal pages such as 6Steps do not create or adopt this Form tape
  unless an exact resource-independent all-native Form has already succeeded.

Relevant Android logs:

```text
revision=r25-19-0149 event=form_cache result=store
revision=r25-19-0149 event=form_cache result=early_hit ... skippedObjects=N
```

## Acceptance

Retain `0149` only if all of the following hold in repeated cold runs without
warmup:

1. EP23 cache-hit pages show early hits and materially lower acquisition.
2. EP23, 11, Q16, Study Notes, 6Steps, and the correctness corpus are
   pixel-identical to accepted `0138`.
3. Editing or enumerating an early-adopted Form reconstructs the exact
   canonical child count and clears the omitted-object state.
4. Normal-page, Q16, and 11 timings remain within controlled run noise.
5. Memory remains within the existing 16-entry/96-MiB cache bounds.

If EP23's first use is still too slow, the next work must target cold Form
compilation itself. If Q16 is still too slow, it requires a root-stream
acquisition or dense ordered raster mechanism; broadening this cache's
eligibility would not solve that cost and would weaken correctness.
