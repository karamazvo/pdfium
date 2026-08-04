# r25-normal-exp-0004: Exact Opaque SMask Elision

Date: 2026-08-04 (Asia/Taipei)

Status: isolated native performance experiment, not an accepted release revision

Base: `r25-8-0138` ordered stroke renderer tape

Artifact: `libpdfium-android-arm64-r25-normal-exp-0004-exact-opaque-smask-elision`

## Decision From 0003

Experiment 0003 was stable in its first device run, but the measured normal-page
render average improved only about 3-4% and the log did not prove how often its
direct compositor executed. It remains isolated and is not a parent of 0004.

Experiment 0004 restarts from accepted `r25-8-0138`. It targets an earlier and
larger exact redundancy in canonical image realization.

## Evidence

The 0002 profile attributed about 78% of sampled 6Steps native active time to
image work. A direct corpus inspection found 32 explicit soft masks in 6Steps.
Thirteen masks, covering 6,395,955 decoded mask pixels, contain only coverage
value 255.

Canonical PDFium nevertheless retains, scales, and composites those masks on
every render. For a fully opaque mask all that work is the identity operation.

## Root Cause

`CPDF_PageImageCache::Entry::ContinueGetCachedBitmap()` realizes the image and
its mask, then caches both without reducing constant masks. A cached all-255
8-bit mask therefore forces every later image render through `DrawMaskedImage`:

```text
decode/realize color and mask
  -> scale color into an offscreen BGRx bitmap
  -> scale mask into an offscreen 8-bit bitmap
  -> apply mask / composite
```

For coverage `255` at every logical mask pixel, PDF alpha composition requires:

```text
sourceAlpha * 255 / 255 == sourceAlpha
```

The mask does not affect color, alpha, blend order, clipping, matte correction,
or destination pixels.

## Invariant

The mask is removed only after realization and only if all of these exact facts
hold:

- format is `FXDIB_Format::k8bppMask`;
- width and height are positive;
- every logical scanline contains at least `width` bytes;
- every logical coverage byte is exactly `255`.

Pitch padding is deliberately ignored. Any unsupported format, malformed row,
empty mask, or non-255 byte retains the unchanged canonical mask path.

This is value canonicalization, not page classification. It has no filename,
page number, content-count threshold, visual approximation, or device-scale
policy.

## Architecture

```text
CPDF image and SMask decode
  -> existing cache-time mask realization
  -> allocation-free exact coverage scan
       all 255 -> omit identity mask from cache
                  -> ordinary canonical image renderer
       otherwise -> retain canonical mask unchanged
```

The proof happens once at cache insertion. The cache remains the single source
of truth; renderers do not repeat the scan or maintain a second property table.

## Cost And Bounds

- One sequential scan of each realized 8-bit mask at first cache insertion.
- Early exit at the first non-255 byte.
- No allocation, retained metadata, new cache, lock, thread, JNI, Kotlin, or UI
  work.
- Opaque masks reduce cache memory by the decoded mask size.
- Every render after proof avoids mask scaling and the masked-image offscreen
  route.
- Non-mask pages pay zero cost.

The worst case is a non-opaque mask whose first differing byte is near the end;
it receives one additional linear read and then behaves canonically. This is an
experiment acceptance risk and must be measured on the corpus.

## Expected Result

This should help pages containing exact opaque soft masks, including a material
subset of 6Steps. It should improve both cold rendering after mask realization
and repeated rendering because the retained mask is removed.

It will not improve pages whose masks contain transparency, pages without
masks, text/vector-only pages, 11.pdf, Q16, or EP23 unless they independently
contain an exact all-255 8-bit image mask.

It is not expected to reduce every 6Steps page to 20 ms. The remaining color
image decode and AGG scale/composite work stays canonical.

## Acceptance Gate

Compare repeated cold and warm runs directly with `r25-8-0138` on the same
device, viewport, bitmap sizes, document state, and scroll sequence.

Accept only if all are true:

1. 6Steps repeated-median preview latency improves by at least 10% on pages
   containing exact opaque masks, beyond run-to-run noise.
2. 6Steps, Study Notes, `disquisitionesa00gaus.pdf`, 11, Q16, and EP23 remain
   pixel-correct.
3. Alpha edges, matte images, inverted decode arrays, image masks, soft masks,
   and transparent destinations remain visually identical.
4. Normal pages without opaque masks stay within timing noise.
5. Peak memory does not increase and cancellation behavior is unchanged.

Reject if the one-time scan outweighs the removed work, any decoded coverage is
misinterpreted, or any page differs visually.

## Files

- Patch: `patches/experiments/normal-page/0004-exact-opaque-smask-elision.patch`
- Workflow: `.github/workflows/pdfium-android-arm64-r25-normal-exp-0004-exact-opaque-smask-elision.yml`
