# r25-jpx-7-0008: Bounded Single-Tile JPX Decode Session

Date: 2026-08-10 (Asia/Taipei)

## Parent and revision policy

`r25-jpx-5-0006` is the direct performance parent. `r25-jpx-6-0007` is
retained for audit but excluded: device requests were adjacent or disjoint,
not contained, so retaining one decoded source-window bitmap did not remove
meaningful work.

`0008` advances the revision instead of amending or reusing `0007`.

## Root cause

`0006` sends the exact source rectangle needed by a visible region into
OpenJPEG. That reduces decoded pixels, but every region still constructs a
new decoder, reads the same JP2/J2K header, ingests the same codestream
metadata, decodes one window, and destroys all decoder state. Adjacent visible
regions therefore repeat source acquisition that is independent of the
requested rectangle.

OpenJPEG explicitly supports repeated
`opj_set_decode_area() -> opj_decode()` sequences for a single-tile image and
documents that this improves chunked reads. The reusable state belongs to the
source image, not to a viewport tile.

## Invariant

> Reuse only immutable codec/source state whose identity and decode parameters
> are exact; independently decode every requested source window through the
> unchanged PDFium conversion, mask, stretch, blend, and painter-order path.

If eligibility or any operation fails, destroy the session. The next request
starts from the canonical `0006` path.

## Mechanism

One `CPDF_JpxDecodeSession` can be owned by the existing
`CPDF_PageImageCache`:

- the compressed `CPDF_StreamAcc`, OpenJPEG codec, stream, header, and tile
  state share one lifetime;
- reuse requires the same image-cache entry, reduction level, and encoded
  PDFium color-space option;
- OpenJPEG codestream metadata must prove exactly one tile;
- the image must have exactly one component with supported reference-grid
  geometry;
- only exact clipped source-window requests retain the session;
- full-image, multi-tile, multi-component, mismatched, canceled, failed, or
  unsupported requests destroy the session and remain canonical;
- a page image cache owns at most one session;
- retained session memory is capped at 16 MiB and included in the existing
  cache-size and eviction accounting;
- cache reset, image eviction, page close, or document close destroys it by
  existing ownership.

This is not a second bitmap cache. Each source window still receives one new
transient result bitmap, and `0007` is not applied.

## Why single component is required

PDFium/OpenJPEG color conversion can mutate component/color-space state after
decode. Reusing that state across RGB, palette, CDEF, SYCC, or custom color
transforms would require a deeper immutable codec representation and new
correctness proofs. `0008` therefore enables only one-component images, which
covers the grayscale scanned-page class while leaving every ambiguous case on
`0006`.

This is a semantic predicate, not a document classifier or size heuristic.

## Correctness proof

The reusable session changes source acquisition only:

1. OpenJPEG's documented single-tile contract permits repeated exact area
   decodes.
2. The same compressed bytes, reduction level, color option, and source image
   are retained.
3. Every request still calls `opj_set_decode_area()` with its own exact
   reference-grid rectangle.
4. Each decoded window still passes through the existing `JpxDecodeConversion`,
   `SourceWindowDIB`, PDFium stretcher, mask, compositor, and painter order.
5. Cancellation or failure prevents publication and destroys reusable state.
6. The unit test decodes one disjoint window through a reused session and
   compares its complete byte buffer with a fresh canonical decoder.

No approximation, resolution substitution, overlap union, stale bitmap reuse,
document routing, new worker, lock, queue, or UI-thread operation is added.

## Expected performance

- Cold first window: mostly unchanged; it creates the reusable session.
- Second and later same-image, same-reduction disjoint windows: avoid decoder,
  stream, header, and codestream reconstruction.
- Full-page previews: unchanged unless they are emitted as clipped windows.
- Multi-component JPX and multi-tile JPX: unchanged.
- DCT/JBIG2/vector/11/Q16/EP23 and ordinary PDFium paths: unchanged.

The expected gain is workload-dependent and is not claimed until device data
shows repeated exact source windows using the same image and reduction level.

## Acceptance

Accept only if all conditions hold:

1. The Android PDFium build and `pdfium_unittests` compile.
2. `ReuseSingleTileGrayDecodeArea` proves reused disjoint-window bytes equal a
   fresh canonical decode.
3. Preview and zoom pixels match `0006`, including grayscale, masks, and page
   boundaries.
4. Second and later same-scale visible-region native times improve materially.
5. Cold preview, cancellation latency, normal pages, 11, Q16, and EP23 stay
   within measurement noise of `0006`.
6. Retained session memory remains at or below 16 MiB and follows existing
   page-cache eviction.

Reject the experiment if real documents are multi-component/multi-tile, the
session has no repeated-window benefit, or pixels differ. Do not broaden the
predicate heuristically.
