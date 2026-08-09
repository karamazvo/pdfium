# r25-jpx-probe-0001: Bounded Fused-Output Activation Probe

Locked: 2026-08-09 CST

## Scope

- Diagnostic revision: `r25-jpx-probe-0001`
- Functional parent: `r25-jpx-3-0004`
- Accepted performance baseline: `r25-cancel-1-0003`
- Incremental patch:
  `patches/experiments/jpx/probe-0001-fused-output-activation.patch`
- Workflow:
  `.github/workflows/pdfium-android-arm64-r25-jpx-probe-0001-fused-output-activation.yml`

This probe does not consume functional revision `r25-jpx-3-0005`. It changes
no pixels, decode eligibility, decode area, resolution, allocation, cache,
thread, lock, cancellation, scheduling, or fallback behavior.

## Why It Is Necessary

`r25-jpx-3-0004` emitted only the first process-wide successful activation.
The supplied device trace began after process startup, so a missing marker
could mean either no activation or an activation that occurred before Logcat
capture. That marker cannot distinguish codec rejection, PDFium bridge
fallback, or successful sink use.

Local tests against both real page-one JPX resources from
`disquisitionesa00gaus.pdf` proved byte-identical fused output at reduction
levels 0, 1, 2, and 3, including the existing non-cancelling callback path.
The static codestream and PDF `/DeviceRGB` predicates are supported. The
remaining question is runtime path selection in the installed Android build.

## Bounded Evidence

For successful JPX decodes with a PDFium output sink, the probe logs the first
64 results and then one result per 128 decodes:

```text
PdfJpxFusedProbe revision=r25-jpx-probe-0001 event=decode_result \
  ordinal=<n> candidate=<0|1> callback=<not_called|fallback|error|ready> \
  used=<0|1> reduce=<n> colorspace=<n> components=<n> \
  firstPrec=<n> firstSigned=<n> firstDx=<n> firstDy=<n>
```

Interpretation:

- `candidate=0 callback=not_called used=0`: CJPX exact eligibility rejected.
- `candidate=1 callback=fallback used=0`: PDFium destination conversion
  rejected.
- `candidate=1 callback=ready used=0`: OpenJPEG declined the exact fused path
  after accepting the destination.
- `candidate=1 callback=ready used=1`: the fused sink executed.

The atomic ordinal is diagnostic-only. This build must not be used for final
performance acceptance because logging perturbs measurements.

## Next Decision

1. Install the probe artifact and capture the `PdfJpxFusedProbe` lines while
   opening and scrolling `disquisitionesa00gaus.pdf`.
2. Correct only the proven rejection in functional revision
   `r25-jpx-3-0005`.
3. Remove the probe from the functional build.
4. Re-run pixel equivalence, repeated cold render, first-visible, warm render,
   cancellation, memory, and normal-page regression gates.

If the probe reports `used=1`, no bridge correction is needed. Benchmark
`0004` directly against `cancel-1-0003`; reject the direction if repeated cold
gain remains below 15%.
