# r25-17 Exact Reverse-Byte Direct Spans

**Locked:** 2026-08-02 (Asia/Taipei)

**Revision:** r25-17-0147

**Extends:** accepted r25-8-0138

**Excludes:** rejected r25-9-0139 through r25-15-0145 and integration-invalid
r25-16-0146

**Status:** implemented; workflow and device validation pending

## Root Cause

0146 attempted to combine exact axis-aligned scan conversion with direct
destination compositing, but its eligibility predicate required
`!rgb_byte_order_`. Android creates an `FPDFBitmap_BGRA` destination and passes
`FPDF_REVERSE_BYTE_ORDER` so that the four bytes map to Android RGBA memory.
Consequently, every 0146 replay reported `directAxisAlignedSpanDraws=0`.

0147 restarts from accepted 0138. It does not patch or apply 0146. The single
change in execution policy is that the exact four-byte compositor understands
both PDFium byte layouts:

- standard BGRx/BGRA memory: blue, green, red, alpha;
- reverse-byte RGBx/RGBA memory: red, green, blue, alpha.

## Exact Mechanism

The existing 0138 direct butt-line lowerer constructs the same four vertices
as AGG's one-segment butt-cap stroke. Direct span execution is allowed only
when all of these conditions are exact:

- the resulting four-vertex polygon is axis aligned by float equality;
- destination format is four-byte `kBgrx` or `kBgra`;
- source alpha is 255 and normal source-over rendering is active;
- there is no backdrop, clip mask, or `full_cover` mode;
- cap is butt, dash is empty, and geometry is finite and nondegenerate.

The format bit selects source channel positions once before the pixel loops.
Coverage uses AGG's signed 24.8 `poly_coord()` conversion, 8-bit area product,
and aliased threshold. BGRx uses canonical `AlphaMerge()` source-over. BGRA
uses the canonical destination-alpha and alpha-ratio equations, including
zero-alpha destinations. Any unsupported condition falls back before writing
pixels to the unchanged independent 0138 AGG raster path.

The mechanism adds no classifier, threshold, page-specific routing, geometry
merge, persistent allocation, cache, lock, thread, JNI/Kotlin change, or UI
work. Source operations remain independent and publish in painter order.

## Correctness Contract

```text
lineRasterPasses + directAxisAlignedSpanDraws == lineBatchCommands
```

Unit tests compare the accelerated destination byte-for-byte with canonical
`DrawPath()` for:

- standard BGRx;
- standard BGRA with nonzero and zero destination alpha;
- reverse-byte RGBx;
- reverse-byte RGBA with nonzero and zero destination alpha;
- translucent and round-cap fail-closed fallback.

The workflow also rejects any reintroduction of `!rgb_byte_order_` in the
direct-span eligibility block.

## Expected Device Result

Q16 should report nonzero `directAxisAlignedSpanDraws`; 0143 previously proved
about 1.55 million exact axis-aligned operations in the same workload. Those
operations should no longer count as `lineRasterPasses`. Pixel output must
match accepted 0138.

Accept only if repeated measurements show all of the following:

1. Q16 bitmap replay materially beats the accepted 0138 result of about
   1,465 ms, with no acquisition regression.
2. Q16 direct-span coverage is substantial and the accounting invariant holds.
3. 11, EP23, Study Notes, 6Steps, cancellation, memory, and publication remain
   correct and within timing noise.
4. Normal-page routing remains canonical and sidecar-lazy.

This revision cannot reduce Q16's approximately 3,185 ms accepted cold
acquisition time. If direct coverage is high but bitmap time does not improve
materially, reject 0147 and close this direct primitive executor direction.
