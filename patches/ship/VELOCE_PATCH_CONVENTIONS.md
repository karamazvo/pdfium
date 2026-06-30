# PDFium Veloce Patch Conventions Handoff

Date: 2026-06-30

Purpose: shared context for Codex and Claude when continuing the native PDFium
Veloce performance patch stack. This document records the current worktree
layout, patch naming rules, workflow conventions, architecture invariants, and
current revision status so the next patch can be contributed without reopening
old decisions.

## 1. Repository And Worktree Layout

Native PDFium patch repo:

```text
/Users/shchao/Code/xPDFSDK/android/meta/libs/pdfium
```

This repo owns:

- `patches/ship/*.patch`
- `.github/workflows/pdfium-android-arm64-rXX-*.yml`
- patch-stack documentation in `patches/ship/README.md`
- commits for native PDFium patch revisions

Android app, JNI, Kotlin, and UI layer worktree:

```text
/Users/shchao/Code/xPDFSDK.worktrees/codex-use-r7-pdfium/android
```

Use this worktree only for app/JNI/Kotlin changes. Do not mix app-layer changes
into the PDFium patch repo.

Current native PDFium HEAD after r44:

```text
e79b17afd Add r44 safe blend paint widening patch
```

Known untracked files currently visible in the native PDFium repo are generated
source snapshots and one unrelated patch file. They were intentionally not
staged in r44:

```text
core/fpdfapi/render/veloce_path_display_list.cpp
core/fpdfapi/render/veloce_path_display_list.h
core/fpdfapi/render/veloce_render_*.{h,cpp}
fpdfsdk/fpdfex_render_*.cpp
public/fpdfex_render_*.h
patches/ship/10-page-dimensions-no-parse.patch
```

Do not stage these unless the next task explicitly requires them.

## 2. Patch Naming And Revision Rules

Native patch files live under:

```text
patches/ship/
```

Use numeric patch order for the patch stack:

```text
0049-veloce-path-display-list-effective-blend-paint-widening.patch
0050-<next-name>.patch
```

Use revision labels for user/build tracking:

```text
r45 effective blend paint widening
r46 <next revision name>
```

The next native patch after r45 should be:

```text
patch file: patches/ship/0050-<short-kebab-name>.patch
workflow:   .github/workflows/pdfium-android-arm64-r46-<short-kebab-name>-path-display-list.yml
commit:     Add r46 <short readable name> patch
```

Keep deprecated patch numbers and names in place. In particular:

- `0027` is deprecated and must not be applied.
- `0028` is deprecated and must not be applied.
- Workflows intentionally apply `0026 -> 0029` directly.
- Do not rename old patch files just to make names prettier. Stable names are
  more important than perfect taxonomy.

Workflow `name:` and artifact path must start with the revision label:

```yaml
name: r46 <revision name> path display-list - Build patched PDFium Android arm64
DIST_DIR: .../libpdfium-android-arm64-r46-<revision-name>-path-display-list
```

This is required so the GitHub Actions UI shows the new revision immediately.

## 3. Patch Authoring Principles

The rule is mechanism first, file-specific behavior never.

Do:

- Solve a class of rendering workloads, not a single PDF file.
- Preserve PDF painter order unless there is a proof that operations commute.
- Keep segment boundaries, text passthrough, clip changes, unsupported blend,
  image/Form/shading, and normal-object barriers as hard barriers unless a patch
  explicitly proves a narrower crossing rule.
- Keep changes memory-bounded.
- Keep replay visible-first where possible.
- Prefer telemetry before behavior when the cost model is unclear.
- Add compact Android log lines when long `VelocePathDL` lines can be truncated.
- Add workflow guards that prove the new mechanism compiled in and old unsafe
  mechanisms did not reappear.

Do not:

- Add a fast path for `11.pdf`, `Q16*.pdf`, or `error.pdf` by filename or
  document-specific shape.
- Reintroduce Kotlin `classifyAllPages`; huge path detection and acceleration
  moved into native PDFium/Veloce.
- Add UI-thread zoom-continuity hacks to compensate for native render cost.
  A previous snapshot/continuity framework created visual artifacts and was
  reverted conceptually.
- Merge overlapping different-paint blend objects into one group unless there
  is a formal pixel-equivalence proof.
- Store raw `CPDF_PageObject*` pointers in cached display lists. Use holder
  object indexes and resolve live objects at replay time.
- Cross segment or passthrough barriers for batching.

## 4. Current Architecture Invariants

The central architecture is ordered segmented replay:

```text
PathRunSegment -> TextPassthrough -> PathRunSegment -> ...
```

Inside a path segment, Veloce may optimize replay. Across segment boundaries,
passthrough objects, clip changes, unsupported objects, and non-proven barriers,
it must preserve original PDF order.

Current native mechanisms:

- Cached native path display list for root pages/forms.
- Text passthrough by holder object index, not raw pointer.
- Holder-space spatial index for visible tile candidate selection.
- Stroke-run packing inside a segment.
- Same-ARGB fill-barrier crossing for normal stroke runs.
- Translation-normalized stroke-run packing for same-linear CTMs.
- Blend group replay through BGRA isolated group buffers.
- Cooperative cancellation in blend group flushes.
- Conservative safe blend paint widening for disjoint same-blend paint changes.

The display-list cache is native owned. Replay must tolerate cache hits across
renders without retaining stale page-object pointers.

## 5. Current Revision Status

Recent revisions:

| Revision | Patch | Status | Purpose |
| --- | --- | --- | --- |
| r39 | `0043-veloce-path-display-list-primitive-run-telemetry.patch` | committed | Primitive/run telemetry for dense tile analysis. |
| r40 | `0044-veloce-path-display-list-fill-barrier-proof-telemetry.patch` | committed | Proof telemetry for fill barriers blocking stroke runs. |
| r41 | `0045-veloce-path-display-list-same-argb-fill-barrier-crossing.patch` | committed | Cross same-ARGB fill-only barriers for normal stroke runs. |
| r42 | `0046-veloce-path-display-list-stroke-run-compact-telemetry.patch` | committed | Compact stroke telemetry and restored flush accounting. |
| r43 | `0047-veloce-path-display-list-translation-normalized-stroke-run-packing.patch` | committed | Pack same-linear translation-only stroke matrices. |
| r44 | `0048-veloce-path-display-list-safe-blend-paint-widening.patch` | committed | Conservative disjoint same-blend paint widening for blend groups. |
| r45 | `0049-veloce-path-display-list-effective-blend-paint-widening.patch` | in progress | Overlapping blend paint-key widening when effective render paint is equivalent. |

r44 commit:

```text
e79b17afd Add r44 safe blend paint widening patch
```

r45 current files:

```text
patches/ship/0049-veloce-path-display-list-effective-blend-paint-widening.patch
.github/workflows/pdfium-android-arm64-r45-effective-blend-paint-widening-path-display-list.yml
```

## 6. Current Performance Reading

Q16 after r43:

- `flushMatrix=0`
- `matrixPacked` in the millions
- full render stroke draw calls collapsed to hundreds
- small and medium tiles are mostly fast
- large visible regions are still expensive because millions of nodes are truly
  visible and large packed paths are rebuilt/rasterized per replay

Q16 current remaining bottleneck:

```text
rebuilding and rasterizing huge packed stroke paths for large visible regions
```

Likely future Q16 class improvement:

```text
cached packed stroke chunks with holder-space bboxes, inside existing segment,
paint, graph-state, path-style, clip, blend, and passthrough barriers
```

11.pdf after r43/r44:

- r43 stroke packing does not materially affect it because the stroke path is
  trivial in the log.
- The dominant cost is still `BlendGroupRun`: many composites, high group pixel
  area, high node pixel area.
- r44 is intentionally conservative. Because the r43 logs showed most paint
  barriers overlap, r44 may be modest for 11.pdf. It is still safe and useful
  telemetry for `paintSwitches` / `maxPaints`.
- r45 attempts the next safe overlapping case: paint keys that differ but are
  equivalent for rendering. The first expected win is fill-only blend paths
  whose keys differ only by stroke-adjust state that does not affect fill
  pixels.

11.pdf current remaining bottleneck:

```text
overlapping Darken blend group cost
```

Likely future 11.pdf class improvement:

```text
proof-backed Darken/span/mask optimization or another equivalent reduction in
group-buffer composite/pixel work
```

Do not implement unsafe overlapping multi-paint group merging. It is not
generally equivalent to PDF per-object blend order.

## 7. Workflow Requirements

Every new native revision needs a workflow file:

```text
.github/workflows/pdfium-android-arm64-rXX-<revision-name>-path-display-list.yml
```

Workflow requirements:

- `name:` starts with `rXX`.
- `WORK_ROOT` uses `/tmp/pdfium-rXX`.
- `DIST_DIR` and upload artifact include `rXX-<revision-name>`.
- Patch apply list includes the new numeric patch.
- Artifact copy step includes the new patch.
- `build-info.txt` patch list includes the new patch number.
- Verification step has revision-specific `grep` guards for the new mechanism.
- Verification step keeps safety guards against deprecated symbols and known
  reverted approaches.

For r46, copy the r45 workflow and update:

```text
r45 -> r46
0049 -> 0050 in apply/copy/build-info
revision name and guards
```

Be careful with mechanical replacement. In r44, a Perl replacement briefly
damaged `$GITHUB_WORKSPACE` references; always inspect for broken strings such
as:

```text
""/repo
"/patches/"
```

## 8. Validation Checklist For New Patches

Before committing a new native patch:

1. Generate the patch against the current patched base, not against untracked
   generated snapshots in the patch repo.
2. Verify:

```text
git apply --check patches/ship/00XX-<name>.patch
```

against a clean temp tree that has the previous patch applied.

3. Parse the workflow YAML.

4. Run local grep guards for the new mechanism.

5. Confirm staged files include only:

```text
patches/ship/00XX-<name>.patch
.github/workflows/pdfium-android-arm64-rXX-<name>-path-display-list.yml
patches/ship/README.md
```

6. Keep generated source snapshots, unrelated patches, and app worktree changes
   out of the native patch commit unless explicitly requested.

## 9. Commit Convention

Native patch commits use:

```text
Add rXX <revision name> patch
Fix rXX <specific workflow/build issue>
```

Examples:

```text
Add r43 translation-normalized stroke run packing patch
Fix r43 workflow stroke append guard
Add r44 safe blend paint widening patch
```

If a workflow build fails because a guard is stale but the patch is correct,
commit the workflow fix separately as:

```text
Fix rXX workflow <short cause>
```

## 10. Next Patch Direction

The next patch should be chosen by 80/20 value:

- For 11.pdf market-facing blend cost: a future patch should target a proof-backed Darken
  blend cost reduction. Start with telemetry/proof if equivalence is not yet
  clear.
- For Q16 large visible-region cost: a future patch should target cached packed stroke
  chunks. The invariant is to cache reusable packed geometry inside existing
  ordered segment/style/clip/blend barriers and replay only visible chunks.

Do not mix both tracks in one patch. One mechanism per patch.
