# r25-normal-2-0008: Unified Destination-Row Image Executor

Date: 2026-08-05 (Asia/Taipei)

## Scope

This is a normal-page image-rendering experiment. It does not apply `0151` or
any `0139..0151` heavy-path experiment. Its reproducible workflow stack is the
accepted `r25-8-0138` base plus normal-page experiments `0005`, `0006`, `0007`,
and this patch. The accepted base remains present for controlled comparison,
but `0008` changes only PDFium's canonical image, stretch, mask, and AGG device
boundaries. It adds no page classifier and no RenderTape behavior.

Patch:

```text
patches/experiments/normal-page/0008-unified-destination-row-image-executor.patch
```

Workflow:

```text
r25-normal-2-0008 unified destination-row image executor - Build libpdfium Android arm64
```

## Root Cost

`0005` bounds the image stretch intermediate. `0006` lets canonical decoder
rows feed that stretcher while filling the existing page cache. `0007` removes
the full destination-sized color offscreen for exact masked images, but it
still renders the soft mask into a complete destination-sized bitmap before
the color image can begin its final pass.

The remaining eligible path is:

```text
mask decode -> mask stretch -> full destination mask
color decode -> color stretch -> read destination mask -> destination
```

`0008` removes the destination mask allocation and the serial phase boundary.

## Architecture

```text
canonical color source -> bounded color stretch ----+
                                                   +-> exact alpha/clip blend
canonical SMask source -> bounded mask stretch -----+   -> destination row
```

The two existing PDFium stretch engines are initialized before the first
pixel. They use the same destination transform, clip, resampling options, and
fixed-point weight tables. Replay advances them one output row at a time:

1. Produce one exact mask row into one destination-width buffer.
2. Produce the corresponding color row.
3. Multiply mask coverage by global alpha and device clip in PDFium's existing
   integer order.
4. Composite the color row once into the AGG destination.
5. Reuse the mask-row buffer for the next row.

DeviceGray soft masks are exposed as mask rows only when their 256-entry
palette is the exact identity grayscale mapping. Inverted, indexed, ICC,
malformed, or otherwise non-identity masks remain canonical.

## Correctness Contract

Activation requires all of the following before either stretcher consumes a
source pixel:

- opaque color image;
- Normal blend;
- no group knockout or backdrop bitmap;
- no matte correction;
- axis-aligned non-degenerate image transform;
- exact 8-bit mask source, or exact identity-gray mask view;
- destination mask rectangle equal to the clipped image rectangle;
- both bounded stretchers validate and allocate successfully.

Any failed predicate returns to the unchanged canonical masked-image path
before output. Painter order, decoder output, image cache ownership, weight
tables, interpolation, alpha equations, device clip, byte order, and final
bitmap ownership remain canonical PDFium.

The unit comparison intentionally uses different source and destination mask
dimensions so the new path proves exact mask scaling rather than consuming an
already-rendered destination mask.

## Resource and Threading Contract

- Two bounded stretch rings, each capped by the existing 1 MiB limit.
- One reusable destination-width mask row.
- No destination-sized color or mask temporary for the eligible path.
- No persistent cache, map, index, thread, lock, JNI call, Kotlin policy, or
  UI-thread work.
- Existing page-image cache remains the single decoded-image owner.
- Existing progressive pause reaches both mask and color source-row admission.
- If cancellation interrupts color production, the completed mask row remains
  locally owned and is consumed exactly once after resume.

## Expected Result

This targets ordinary pages containing images with soft masks. It should
reduce allocation, memory traffic, and serial image passes. It does not
accelerate unmasked images, text, paths, affine image transforms, or codec
entropy decoding, so a 16-33 ms average is an acceptance target rather than a
claimed result.

Compare the same cold fast-scroll sequence at `756x978` against `0006` and
`0007`. Record `bitmapRenderMs`, `engineRenderMs`, queue wait, visible publish
latency, cancellation latency, and output resolution. Accept only when:

1. complete output buffers remain pixel-equivalent on the correctness corpus;
2. normal motion-preview median improves beyond run variance;
3. no page is blank, lower resolution, delayed until zoom, or partially
   published;
4. p50 ordinary pages and cancellation do not regress;
5. 11, Q16, EP23, Study Notes, and scanned pages remain unchanged within
   controlled noise.
