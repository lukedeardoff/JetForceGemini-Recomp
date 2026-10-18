# High-refresh rendering references, 2026-10-11

This review compares implementation approaches for JFG's requested 60-360+ FPS
presentation while keeping game timing unchanged. These are pinned source
observations and proposed applications; another game's results do not verify
JFG. The [implementation plan](../planning/high-refresh-rendering.md) and
[RENDER-001](../development/issues/high-refresh-rendering.md) own current scope
and acceptance. The existing [reference catalog](reference-catalog.json) and
[2026-10-01 review](reference-review-2026-10-01.md) remain applicable.

## Reviewed revisions

| Project | Revision | Role in this review |
|---|---|---|
| RT64 | `5473732a822a4423b5696e7cb18fecc425a59875` | Our pinned renderer; authoritative for available APIs and caps |
| DK64: Recompiled / ReKONGpiled | `3b0c36d3fbfeb0b5d2c47923f35cf84b31edcbf7` | Released Rare-game recomp with explicit matrix and discontinuity patches |
| Banjo: Recompiled | `dbd52154bc1e00e56867b1fac94a8f907052f41b` | Rare-game camera, object/marker and vertex/graphics ownership patterns |
| Zelda64Recomp | `b65c482ed672258eb684ab94a4e8c8aa662615ee` | Transform identity lifecycle, camera cuts and per-category tagging |
| Starship | `3322643f0bbf80d2f8f7e1c0d2c4c578515f6c90` | Decompilation-based port; matrix-operation recording is an alternative architecture |
| DKR-R | `32bf1c7999c2ab55cb1ff5f3d1e34147a8dcd957` | Closest custom-microcode comparison; inspect its implementation independently |
| cyscott/dkr-recomp | `eb8151c05837359c9eca0ad006077a14f5bf72b8` | Experimental alpha comparator; not used as proof of completed HFR support |
| JFG decompilation | `b49aa4791e8fb1e7acd3bba10346876358d0a9a7` | Existing local pin; candidate draw/camera hooks only |

Default-branch revisions were fetched on the date of this review. Do not call
them permanent latest versions. Exact retrieved paths and SHA-256 hashes are
retained privately in `high-refresh-20261011/references/inventory.json`.

## DK64: most useful implementation lessons

DK64's matrix patch gives model draw groups stable IDs and offsets, explicitly
selects interpolation components, and selectively supports changing vertex
positions. It uses simple matrix interpolation in the reviewed model path;
decomposition is not a universal prerequisite for an initial experiment.
[Matrix implementation](https://github.com/Rainchus/Donkey-Kong-64-Recompiled/blob/3b0c36d3fbfeb0b5d2c47923f35cf84b31edcbf7/patches/patches_matrix.c).

Its interpolation patch treats camera changes, actor warps, model creation and
replacement as discontinuities. Timed invalidation and special-mode rules stop
unrelated states being blended. This is useful for JFG's room transitions,
character changes and scripted cameras, but its game-specific addresses and
thresholds cannot be reused as JFG facts.
[Discontinuity implementation](https://github.com/Rainchus/Donkey-Kong-64-Recompiled/blob/3b0c36d3fbfeb0b5d2c47923f35cf84b31edcbf7/patches/patches_interpolation.c).

The recomp links the [DK64 decompilation](https://gitlab.com/dk64_decomp/dk64)
as the source of reverse-engineered types and functions. The relevant lesson is
to recover JFG's semantic boundaries from its own decompilation and verify them
against working memory. This review did not build or run DK64 and does not claim
JFG compatibility from DK64's published feature list.

## Banjo: camera meaning and asynchronous graphics ownership

Banjo tags perspective and orthographic projections separately, exposes a view
matrix, and restores projection identity along with viewport state. Camera
translation has a specific representation in that game; copying only its
interpolation flags would omit the reason the transforms match correctly.
[Camera implementation](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/camera_transform_tagging.c).

Its marker/model tagging distinguishes the objects that own transforms, while
graphics changes address buffer capacity and completion ordering around vertex
modification. JFG already snapshots tasks; its interpolation metadata must share
the same lifetime. Appending extended commands to a fixed guest buffer would
also require a capacity audit.
[Marker tagging](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/marker_transform_tagging.c),
[graphics ownership](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/graphics_patches.c).

## Zelda: object lifetimes and explicit coverage

Zelda creates and destroys actor extension data at actor lifetime boundaries and
uses dedicated tagging for actors, terrain, effects, particles and UI. This is a
strong reference for avoiding pointer-reuse mismatches and for tracking coverage
per rendering category. A general matrix switch does not cover every effect.
[Actor lifetime/tagging](https://github.com/Zelda64Recomp/Zelda64Recomp/blob/b65c482ed672258eb684ab94a4e8c8aa662615ee/patches/actor_transform_tagging.c),
[ID domains](https://github.com/Zelda64Recomp/Zelda64Recomp/blob/b65c482ed672258eb684ab94a4e8c8aa662615ee/patches/transform_ids.h),
[particles](https://github.com/Zelda64Recomp/Zelda64Recomp/blob/b65c482ed672258eb684ab94a4e8c8aa662615ee/patches/particle_transform_tagging.c).

Camera tagging includes logic for skipping inappropriate interpolation. For JFG,
record native camera mode and transition state rather than importing Zelda's
heuristics or constants.
[Camera implementation](https://github.com/Zelda64Recomp/Zelda64Recomp/blob/b65c482ed672258eb684ab94a4e8c8aa662615ee/patches/camera_transform_tagging.c).

## DKR: close microcode comparison, with important limits

DKR-R maintains identity metadata per selectable combined-matrix slot and has
separate presentation scopes for shadows, vehicle parts, billboards and scenery.
Its documented architecture freezes semantic IDs with task snapshots rather than
putting additional commands into the fixed display-list buffer. This suggests a
host-side metadata path compatible with JFG's existing task snapshots.
[Matrix-slot metadata](https://github.com/ThatGuyMcd/DKR-R/blob/32bf1c7999c2ab55cb1ff5f3d1e34147a8dcd957/runtime-recomp/src/game/interpolation_state_policy.hpp),
[snapshot architecture](https://github.com/ThatGuyMcd/DKR-R/blob/32bf1c7999c2ab55cb1ff5f3d1e34147a8dcd957/docs/RENDER_SNAPSHOT_ARCHITECTURE.md).

That project also patches RT64's interpolation-target eligibility for
SkipBuffering. This identifies a boundary to investigate, not a patch to import:
JFG explicitly retains Console presentation for CPU overlays. Its feedback path
must be measured before changing eligibility or buffer selection.
[Eligibility patch](https://github.com/ThatGuyMcd/DKR-R/blob/32bf1c7999c2ab55cb1ff5f3d1e34147a8dcd957/patches/rt64/0001-allow-skip-buffering-interpolation-targets.patch).

The separate [cyscott DKR port](https://github.com/cyscott/dkr-recomp/blob/eb8151c05837359c9eca0ad006077a14f5bf72b8/README.md)
describes itself as experimental alpha. It is retained as a renderer-integration
reference, not counted as verified high-FPS evidence. No DKR binary was run here.

## Starship: an alternative with a larger JFG integration surface

Starship records matrix operations in a hierarchy, interpolates recordings and
uses camera epochs to break continuity. This can preserve more transform meaning
than interpolating final matrix coefficients. It requires broad coverage of the
game's draw/matrix call sites and is therefore a larger first step for JFG than
using its existing RT64 workload pipeline.
[Recording and interpolation](https://github.com/HarbourMasters/Starship/blob/3322643f0bbf80d2f8f7e1c0d2c4c578515f6c90/src/port/interpolation/FrameInterpolation.cpp),
[camera handling](https://github.com/HarbourMasters/Starship/blob/3322643f0bbf80d2f8f7e1c0d2c4c578515f6c90/src/engine/camera_interp_skip.c).

## RT64: available mechanism and constraints

RT64 keeps decoded draw/transform history and can generate extra scene frames
while preserving a separate native-resolution feedback path. Its matching and
interpolation are the foundation for the proposal.
[Architecture](https://github.com/rt64/rt64/blob/5473732a822a4423b5696e7cb18fecc425a59875/README.md#architecture).

At our pin, ignored transform IDs bypass explicit matching. Matched groups can
produce vertex and texture-coordinate velocities. Workload generation requires
a valid source cadence and an eligible framebuffer; presentation has its own
queue and pacing.
[Matching](https://github.com/rt64/rt64/blob/5473732a822a4423b5696e7cb18fecc425a59875/src/hle/rt64_game_frame.cpp),
[workload generation](https://github.com/rt64/rt64/blob/5473732a822a4423b5696e7cb18fecc425a59875/src/hle/rt64_workload_queue.cpp),
[presentation](https://github.com/rt64/rt64/blob/5473732a822a4423b5696e7cb18fecc425a59875/src/hle/rt64_present_queue.cpp).

Manual refresh is validated within 10-1000, then constrained by detected
swapchain refresh. A 360 target is supported by the configuration range but is
not evidence of 360 delivered frames. Source cadence also differs from VI rate.
[Configuration](https://github.com/rt64/rt64/blob/5473732a822a4423b5696e7cb18fecc425a59875/src/common/rt64_user_configuration.cpp),
[cadence inference](https://github.com/rt64/rt64/blob/5473732a822a4423b5696e7cb18fecc425a59875/src/hle/rt64_vi.cpp).

## Application boundary

These findings support an implementation strategy, not a dependency update or
an immediate runtime patch. Sources were read into ignored private storage;
this review adds original summaries and links, not upstream code or assets.
Existing project licensing and attribution rules remain in force. No ROM,
generated guest code or personal save is included in the research branch.

Retain the enabled production guard and existing investigation counters on
continuation. Record future positive and negative experiments in RENDER-001.
