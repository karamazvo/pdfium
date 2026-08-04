# r25-normal-exp-0003: Direct Image-Mask Composite

Date: 2026-08-04 (Asia/Taipei)

Status: isolated native performance experiment, not an accepted release revision

Base: `r25-8-0138` ordered stroke renderer tape

Artifact: `libpdfium-android-arm64-r25-normal-exp-0003-direct-image-mask-composite`

## Evidence

The `r25-normal-exp-0002` profile attributed about 78% of sampled 6Steps
native active time to image work:

- image device work: about 41%
- image loading/realization: about 29%
- masked-image paint outside nested device draws: about 8%

Experiment 0001 did not prove a useful decoder-demand win. Experiment 0003
therefore leaves decode demand, image caching, and scaling unchanged. It
targets a concrete redundant pass in PDFium's masked-image paint path.

## Root Cause

For an explicit image mask, canonical AGG rendering currently:

1. renders the transformed color image into an opaque BGRx bitmap;
2. renders the transformed mask into an 8-bit bitmap;
3. allocates/materializes BGRA by multiplying the mask into the BGRx image;
4. scans the BGRA bitmap again to composite it into the destination.

The existing scanline compositor can already composite an opaque source using
an 8-bit clip mask as source coverage. Materializing the equivalent BGRA
intermediate is unnecessary for the exact subset implemented here.

## Invariant

The direct path is used only when this operation is equivalent to canonical
PDFium before any destination pixel changes:

```text
Composite(destination, BGRA(color, mask), normal blend)
  == Composite(destination, opaque BGRx color, coverage = mask, normal blend)
```

Eligibility is exact and fail-closed:

- global image alpha is exactly `1.0`;
- blend mode is `Normal`;
- source format is opaque `Bgrx`;
- mask format is `k8bppMask`;
- source and mask dimensions match exactly;
- group knockout is inactive;
- the AGG device has no soft clip mask.

Rectangular device clipping and Android reverse-byte-order compositing remain
inside the existing DIB scanline compositor. Any failed predicate returns
before pixels and executes the unchanged BGRA construction and canonical
`SetDIBitsWithBlend()` fallback.

## Architecture

```text
CPDF_ImageRenderer::DrawMaskedImage
  -> existing transformed BGRx color bitmap
  -> existing transformed 8-bit mask bitmap
  -> backend-neutral SetBitsWithMask capability
       -> AGG exact direct composite: existing CompositeBitmap + mask coverage
       -> unsupported: false before pixels
  -> unchanged canonical MultiplyAlphaMask / MultiplyAlpha / SetDIBits fallback
```

This is a device capability, not a page policy. It has no filename, page
number, page classification, command-count threshold, or document-specific
routing.

## Cost

- No new cache or retained representation.
- No new heap allocation on the direct path.
- No persistent memory.
- No JNI, Kotlin, scheduler, lock, thread, or UI-thread change.
- No extra source decode or image transform.
- One exact predicate block per explicit masked-image paint.
- On a match, removes BGRx-to-BGRA alpha materialization and its full-bitmap
  pass; destination compositing uses the existing scanline compositor.

The existing `CompositeBitmap()` mask parameter becomes const/read-only. This
matches its actual behavior and avoids a new pixel kernel.

## Correctness Gate

`OpaqueBitmapMaskMatchesCanonicalBgraComposite` compares the direct operation
with canonical BGRA construction across:

- BGR, BGRx, and BGRA destinations;
- transparent, translucent, and opaque destination pixels;
- mask coverage 0, 1, 64, 127, and 255;
- normal and Android reverse byte order.

The workflow compiles `pdfium_unittests` after linking the Android artifact.
The normal-page corpus still requires device pixel comparison because the
cross-compiled Android tests are not executed by GitHub Actions.

## Expected Result

This experiment is intentionally narrower than a decoder replacement. It can
remove most of the measured masked-image paint overhead when the exact path is
eligible, but it does not remove image decode, scaling, or the final
destination composite. Based on the 0002 sample, the realistic cold-preview
gain is single-digit to low-double-digit percent, not a guaranteed reduction
from roughly 100 ms to 20 ms.

It should have no measurable effect on text/vector pages, 11.pdf, Q16, or EP23
unless those renders contain an eligible normal-blend opaque masked image.

## Acceptance Gate

Compare repeated cold and warm runs directly against `r25-8-0138` on the same
device, viewport, bitmap size, and scroll sequence.

Accept only if all are true:

1. 6Steps repeated-median preview latency improves beyond run-to-run noise.
2. The 6Steps, Study Notes, `disquisitionesa00gaus.pdf`, 11, Q16, and EP23
   corpus remains pixel-correct, including masks, matte colors, alpha, and
   reverse-byte-order output.
3. Normal text/vector pages remain within timing noise.
4. Peak memory does not increase.
5. Unsupported alpha, blend, knockout, and soft-clip cases visibly match the
   accepted base through canonical fallback.

Reject the experiment if the win is not repeatable, if partial mask coverage
rounds differently, or if any page content changes.

## Files

- Patch: `patches/experiments/normal-page/0003-direct-image-mask-composite.patch`
- Workflow: `.github/workflows/pdfium-android-arm64-r25-normal-exp-0003-direct-image-mask-composite.yml`
