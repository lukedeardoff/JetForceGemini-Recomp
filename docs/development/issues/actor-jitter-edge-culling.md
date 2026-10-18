# RENDER-004: Gem/enemy instability and camera-edge culling

Status: **implemented, verified in bounded replays and delivered locally;
owner live acceptance pending** (2026-10-13 UTC). Severity: high visual
regression/coverage limitation. No save corruption reported.

The owner reports that life-restoring gems shake during combined walking and
camera movement, and the first Goldwood ground drones flash/shake during their
own movement or camera rotation. Level geometry disappears at the trailing
screen edge. This is distinct from character joint gaps. The owner paused live
testing while these corrections were developed.

Affected preview: pacing implementation `b9c1aeee7d259f358a5a76ac22b9ba6c9b7644d6`,
runtime `8bd6b7635ec71a70c735055af97df5119df9f8a0b00d942825b226259ffe9cf7`,
reported in the ongoing Custom 120/VSync context. Comparison of the owner's
exact symptoms in Original mode was not supplied.

## Reproduction, causes and remedy

Walk and turn the camera near Goldwood gems and ground drones; inspect their
motion and the trailing screen edge. Muted hidden replays use copied saves.
Three initially separate hypotheses were established in source and captures:

- **Drone reduced-detail geometry:** the compact BlueAnt model shares the main
  model's skeleton palette. The old resolver accepted only vertices owned by
  that main instance, so valid LOD vertices invalidated the actor's interpolation.
  Resolve palette ownership and geometry ownership separately within the same
  actor lifetime. Inline compact-instance vertices must not be read as private
  palette pointers, even when their bytes resemble RAM addresses.
- **Gems:** identical billboard quads had no stable actor identity. Ambiguous
  shape matching held their current poses while the scenery interpolated.
  Observe the supported `camDoSprite` actor/anchor/asset binding and include
  actor lifetime and camera in billboard transform identity. Preserve original
  texture-animation cadence and retire stale/free/level-change bindings.
- **Screen edges:** F3DDKR clipped triangles and display lists against the
  current endpoint camera before intermediate presentation. Retain this geometry
  for interpolated main targets and let the GPU clip at the displayed pose.
  Original mode and offscreen targets retain their prior clipping. Loaded-vertex
  validation remains enforced. Guest-level culling was not established as a
  cause in these fixtures and is unchanged.

Ambiguous palette/geometry ownership still holds endpoints conservatively.
Semantic provenance survives UV vertex clones. Guest simulation and input rates
are unchanged. See [identity tracking](../../../include/jfg/renderer/presentation_identity.hpp),
[guest observation](../../../src/boot/native_boot.cpp), and
[F3DDKR integration](../../../src/renderer/rt64_f3ddkr.cpp).

## Verification

Exact counts, runtime identities and private artifact hashes are in the
[qualification record](../../../evidence/actor-visibility.json). Large captures,
traces and memory dumps remain on D: in private artifact set
`high-refresh-20261011/visibility-20261013`; no ROM/generated game code/save is
published. Dependency pins are unchanged; the pinned upstream review/catalog
and supported game's source/disassembly informed the shared-palette and sprite
calling-convention analysis.

- LOD diagnostics: 23,030 valid LOD-source observations per drone were rejected
  by the old resolver. The candidate matches 47,061/47,439 vertices per drone
  over 251 observed scenes, with two conservative held scenes. The baseline's
  reduced-detail path matches none. Three gem billboards match 747/753 transforms.
- Actual distant-drone captures at source VI 5001 show the baseline's blue-pixel
  centroid held near x=756.33 across six presentations. The final runtime advances
  through x=757.85, 757.72, 757.11, 757.10, 756.75 and 756.30, with visible pose
  changes. The final crop is byte-identical to the baseline endpoint. A separate
  full-detail close-up at VI 4591 already interpolated in the old preview;
  all six poses remain pixel-identical before/after.
- Actual gem captures at VI 4201: baseline x=253.09 is held; the candidate
  progresses through 253.60, 253.35, 253.20 and 253.10 to the same endpoint.
- Edge captures at VI 4165/4169 restore 220/532 pixels at alpha 0.25 and 37/206
  at alpha 0.5. There are zero scenery differences at alpha 0.75/1.0. HUD
  presentation differences and diagnostic text are excluded; their cause is
  not established by this scenery comparison.
- Nine focused CTest suites pass. New tests cover shared palettes, compact
  instances, ambiguous/cross-actor/unaligned rejection, identical sprites with
  independent lifetimes, sprite-only actors, frees, expiry and level changes.
  Actual CPU decoder command tests verify main-target triangle/display-list
  retention, Original/offscreen clipping and unloaded-vertex rejection.
- Original-mode replay has byte-identical full guest RAM, renderer RAM and save.
  Custom-mode paired replays have identical non-framebuffer guest bytes. The
  diagnostic pair also matches all 25,525 actor, 11 removal, 249 camera and 249
  player-control observations, CPU Count, instruction count and input polls.
- Final 120-cap full-circle trace: 1,698 strictly advancing phases, no repeated,
  backward or uneven steps; median present-return interval 8.340 ms, p99 8.848 ms,
  maximum 22.749 ms. Final combined 360-cap moving window: 351.68 output FPS /
  29.77 scenes/s, median 2.780 ms, p99 4.048 ms, maximum 18.877 ms. Both normal
  final runs have zero audio underruns and zero diagnostic queue drops.

## Informative failures and qualification limits

The first RAM inspection assumed the wrong byte order and stopped; corrected
analysis reads canonical big-endian dumps. An initially overbroad full-RAM
equality assertion failed because 3,318 bytes changed entirely inside the two
guest framebuffers. All other bytes matched. Rendering changes can change
framebuffer contents; Custom mode is not claimed to preserve a full-memory hash.

Early camera routes projected drones onscreen but terrain/Juno occluded them.
Those captures did not qualify visible smoothness. A close-up exercised an
already-working full-detail path; a final clear distant view established the
LOD correction visually. Fixture compilation used an incorrect container API
once; changing `push` to `push_back` fixed it. Incorrect receipt names, a trace
header parse, nested script extraction and proof aggregation each stopped their
requests; corrected immutable requests preserve those failures and results.

Screenshot readback caused diagnostic audio underruns; capture runs are not
performance acceptance. Performance varied across repeated runs, so the final
360-cap result is not a sustained-360 guarantee. The baseline and candidate can
both enter the existing adaptive 30/20-Hz recovery under load. Existing Unlimited
saturation remains open. These fixes do not qualify every enemy, animation,
guest visibility path, texture frame or physical monitor scanout. Newly seen
objects, LOD changes and cuts can still conservatively hold an endpoint.

All temporary per-vertex/triangle diagnostics are removed from the qualified
candidate. Optional aggregate tracing remains disabled unless explicitly enabled.
Owner live acceptance and full-game coverage remain pending; this is a local
preview correction, not a public release or blanket elimination of all flashing.

## Compatibility and transferable lessons

Geometry ownership need not equal skeletal palette ownership. Repeated sprites
need lifetime identity, and camera interpolation requires geometry covering the
intermediate view. Uniform FPS counters and actor-table membership alone cannot
prove visible smoothness. Pair pixel captures by scene/phase, distinguish LOD
paths, and compare non-framebuffer state when rendering itself changes pixels.

The production guard remained enabled in the same investigation with all prior
charges/root retained. Earlier related work: [RENDER-003](steady-fps-jitter.md),
[RENDER-002](character-seams-lighting.md), and
[high-refresh feature notes](../../features/high-refresh-rendering.md).

## Local delivery

Implementation `c9189c1d1d2e356f53a9801c283a527620f71b9b` is committed on
`research/high-refresh-20261011`. The committed rebuild is byte-identical to the
qualified runtime `cb3de8f6593fa2abb0a12e7a2ea7f98ecf23ea817ad247505c1338a39dcd2292`.
Delivered on 2026-10-13 UTC as `high-refresh-visibility-20261013` on D: through
the existing **JFG - High Refresh Preview** desktop shortcut. Close the old game
and launcher before reopening it. The existing profile/save hashes are unchanged.
All 14 package files verify, and isolated launcher initialization selects the new
runtime. Prior preview packages are retained. Delivery does not change the live
acceptance or full-game limitations above, and no public release was made.

## Owner feedback after visibility delivery (2026-10-13)

The owner confirms that gems and drones now look good. This is live confirmation
of those corrections, not blanket acceptance of edge culling or all rendering.
Particles above gems appear to desynchronize from the gems; tracked separately
in [RENDER-005](pickup-particle-desync.md). Frequent 20-25 SCENES during drone
combat at 120 OUTPUT remains a critical smoothness issue under [PERF-001](goldwood-combat-slowdown.md).
