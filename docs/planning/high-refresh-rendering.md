# Smooth high-refresh rendering: investigation and implementation path

Updated 2026-10-11 UTC. Status: **opt-in prototype implemented and replay-qualified
at 60/240 FPS in one moving-camera fixture**. See the
[feature and preview instructions](../features/high-refresh-rendering.md).
Broader visual acceptance and 360+ FPS qualification remain in
[RENDER-001](../development/issues/high-refresh-rendering.md).

## Decision and scope

Use RT64 to render intermediate scene states while retaining the original guest
simulation, input-poll, VI, audio and save timeline. The owner selected smooth
rendering first, with 60 FPS as the initial target and 360+ FPS as the eventual
performance target on capable hardware. Those are output targets, not a request
to execute game logic six or twelve times faster.

This is technically plausible with our existing renderer: the pinned RT64 already
contains frame matching, transform/vertex interpolation and a separate presentation
queue. Successful ports supply the identities and discontinuity rules those
mechanisms need. JFG's default mode opts out; the new experimental path supplies
copied transform history and scene ownership. Semantic identity and broader
visual/performance qualification remain necessary.

The worktree baseline is `d649cf586b13d2bff7ab7258db9991ad554ae017`: merge the
owner's source checkpoint `66e14d60e7809eae12a65a5002240b10fe9707ae` with fetched
main `acccc55`. All newer runtime work continuing in the original checkout has
its own lineage. This investigation does not replace the active
[Goldwood timing work](../development/issues/goldwood-combat-slowdown.md).

## Baseline contracts established by the initial review

| Boundary | Observed source behavior | Implication |
|---|---|---|
| Refresh configuration | `Rt64Shell::create` leaves RT64's refresh setting at `Original`; configuration files are disabled | A display with a high refresh rate does not enable interpolation |
| F3DDKR matrix groups | `select_matrix` sets `G_EX_ID_IGNORE`, skips transform components and disables decomposition | RT64 intentionally has no eligible JFG transform history |
| Matrix representation | Object matrices can be multiplied by slot zero's combined camera matrix; the bridge supplies identity view/projection | Do not treat these matrices as independent affine world transforms or blindly enable decomposition |
| Vertex stream | Packed vertices are expanded through one scratch address; texture-coordinate changes can clone vertices | Scratch addresses and raw matrix-buffer positions are not semantic object IDs; animated geometry needs its own matching rules |
| Frame ownership | The shell uses Console presentation because the guest modifies completed framebuffers on the CPU | Advancing to the newest rendered buffer can lose or misorder CPU overlays |
| Snapshot ownership | Each queued graphics task already owns an 8 MiB private decode snapshot; device-owned writeback ranges are checked before committing | Host-only metadata can accompany that snapshot without storing interpolation state in the guest |
| Source cadence | RT64 infers logical cadence from VI history unless explicitly supplied; inconsistent factors return zero | Do not confuse every VI or offscreen task with a new scene frame |
| Output cap | Pinned RT64 clamps a manual target to detected swapchain refresh; configuration validation permits 10-1000 | 360 is representable, but a 360 request on a 144 Hz display is capped at 144 by this implementation |

Relevant local sources:
[shell](../../src/renderer/rt64_shell.cpp),
[F3DDKR decoder](../../src/renderer/rt64_f3ddkr.cpp),
[task snapshot, writeback and VI presentation](../../src/boot/native_boot.cpp),
[address contract](../../include/jfg/renderer/rt64_f3ddkr_address.hpp).
Upstream observations are pinned in the
[reference comparison](../upstream/high-refresh-reference-review-2026-10-11.md).

The working-build experiments also show why a graphics-task counter cannot be an
FPS counter: a scene submission is followed by a small offscreen shadow submission.
The scene buffers alternate. Pairing the last two arbitrary tasks would combine
different passes. The reference runtime's final `.rt64` dump can consequently be
an offscreen task snapshot; it is not a complete, timestamped history of presented
scene endpoints. See the issue record for measured results and their limits.

## Recommended architecture

1. **Retain the canonical game timeline.** Keep Count/VI, scheduler events,
   game updates, controller sampling, collision and audio untouched by the
   selected output rate. Renderer frames must not dispatch guest updates or
   write interpolated actor/camera values into guest RDRAM.
2. **Capture semantic identity while the game authors graphics.** Observe
   object draw scope, object lifetime, model/limb role, camera/viewport and scene
   generation. Keep metadata in host memory. Freeze it with the exact queued
   task snapshot and immutable task ID. The graphics worker must consume that
   task's metadata even when the game has already reused its frame heap.
3. **Teach F3DDKR to preserve that identity through its matrix slots.** Loading,
   selecting and restoring a matrix must restore its metadata together with its
   value. Billboards, projected shadows, limbs and static scenery need separate
   roles. Only known matches opt into RT64 interpolation; unknown, ambiguous or
   invalid identities display an authored endpoint.
4. **Qualify camera and model transforms separately.** Stable-ID simple matrix
   interpolation is a useful first controlled experiment for the current combined
   representation. It is not general visual acceptance. Determine whether JFG's
   camera hooks can expose separate view/projection information without changing
   guest rendering; this supports better rotation handling. Decomposition belongs
   only on transforms whose meaning and validity have been established.
5. **Select complete scene endpoints.** Associate scene tasks, offscreen passes,
   the VI-selected buffer and CPU edits. Preserve native-resolution readback for
   game-owned pixels. Add a presentation-only path for intermediate frames after
   the selected endpoint's CPU changes are accounted for. Console, SkipBuffering
   and PresentEarly are behaviors to qualify, not interchangeable performance flags.
6. **Let RT64 generate and pace extra frames.** Reuse its owned workload data and
   render/present queues. Add explicit original, display and manual rate settings
   only after eligibility and frame ownership are working. Preserve the current
   default until acceptance. Benchmark mode above display refresh, if desired,
   needs a distinct contract and a separately reviewed renderer change.

Candidate observation points in the pinned US decompilation include
`camSetProjMtx`, `camPushFloatModelMtx`, `camPushModelMtx`, matrix restore/pop,
`objPrintModelObject`, `objPrintObject`, model construction and object setup/free.
These are **hook candidates** from source/symbol evidence. Their call arguments,
nesting, lifetime coverage and correspondence to the supported generated runtime
must be established with a bounded trace before adding hooks. An address or name
alone does not establish a valid integration point.

An identity should describe `(scene generation, object lifetime, viewport,
draw role, limb/mesh ordinal)`, with separately managed static scenery and UI
identities. If narrowed to RT64's 32-bit ID, detect collisions and disable both
conflicting mappings. A pointer reused by a newly spawned object must acquire a
new lifetime. A matrix address reused by the next frame must not acquire a new
object identity merely because the heap flips.

## Discontinuities and special content

| Event/content | Proposed first behavior |
|---|---|
| New scene, load, respawn, teleport, camera cut or viewport change | Start a new history epoch; display the current endpoint |
| Object removal, spawn or model replacement | Invalidate that lifetime/model history; never match to a recycled slot |
| Missing endpoint, task-order mismatch, queue pressure | Present an authored endpoint; bound history and memory use |
| Camera collision correction | Observe the native resolved camera; avoid interpolating through an abrupt obstruction change |
| Pauses, focus loss, resize and settings transitions | Reset presentation timing/history as needed; do not accumulate a catch-up burst |
| Skinned/CPU-deformed meshes | Match stable vertex topology as well as object identity before computing velocities |
| Billboard, particle, shadow or trail | Use an explicit identity and per-component rule; do not inherit the previous object's matrix metadata |
| HUD, menus and CPU framebuffer effects | Preserve authored composition first; extend interpolation after their ownership is understood |
| Split screen | Separate camera/viewport identity domains and validate every viewport count |

Interpolation adds no new authoritative input samples. It can smooth camera
motion yet retain simulation-rate input latency. A later late-latched visual
camera would require its own collision, aiming and input-consumption design.
It is outside the selected first milestone.

## Experiments and acceptance gates

**A. Reference qualification and metadata capture.** Use one retained runtime,
private copies of the save/profile, identical input and explicit stop boundaries.
Repeat the same boundary and compare full guest RDRAM, state hash, input, task
descriptors and image. Record runtime/ROM/input/profile hashes. Then collect a
short ordered sequence of main-scene tasks with their own snapshots, VI records,
CPU overlay updates and semantic metadata. Existing end-of-run dumps do not
substitute for that sequence.

**B. Minimal scene experiment.** Replay those frozen scene endpoints through the
renderer at original and 60 Hz. First enable output scheduling only, then add
known transform IDs one category at a time. Count actual extra presentations and
distinct moving geometry states; repeated identical images do not qualify.
Require unchanged source snapshots and canonical writeback output. Preserve
before/after images and motion traces. Test camera cuts and a removed/reused
object before adding more categories.

**C. Live 60 Hz qualification.** Replay the same gameplay input under each output
mode. Require matching gameplay state at corresponding simulation boundaries,
with explained exclusions if asynchronous device state prevents a full byte
comparison. Compare guest timer progression, movement distance, shots/ammo,
animation phase, AI state, audio samples and save output. Input-by-poll and
input-by-time comparisons answer different questions and must not be mixed.
Test CPU overlays, shadows, loading, pause/resume, cutscenes and unsupported
geometry. Owner playtesting remains necessary for visual feel.

**D. Higher rates and performance.** Run 120, 144, 165, 240, 360 and an above-360
target where the display/output policy supports it. Report requested rate,
effective renderer target, actual presentations, unique rendered states,
frame-time median/p95/p99, drops, GPU time, render-thread time and memory growth.
Use the same scene, resolution and effects. Disable per-frame readback during
timing measurements and measure visual correctness separately. Report hardware
and display mode. The baseline replays in this investigation are not a GPU
throughput benchmark.

| Output target | Frame budget | Outputs per ideal 30 Hz simulation step |
|---:|---:|---:|
| 60 FPS | 16.67 ms | 2 |
| 144 FPS | 6.94 ms | 4.8; needs fractional scheduling |
| 240 FPS | 4.17 ms | 8 |
| 360 FPS | 2.78 ms | 12 |
| 480 FPS | 2.08 ms | 16 |

These are mathematical budgets, not measured JFG performance. CPU graphics
submission time is not GPU completion time. Interpolation does not repair a
slow game simulation: the separate combat-cadence problem remains relevant to
responsiveness and endpoint freshness even if presentation is smooth.

## Progress and continuation

The guarded implementation now builds and runs. Original, 60 and 360 requested
modes retain identical guest RAM, renderer snapshots, input, save and probe state
at VI 4100. The moving-camera fixture measures 59.98 and 239.81 presentations/s;
the 360 request is capped by a 240 Hz display. Actual consecutive presented-image
capture also verifies changing pixels. See the feature page for measurement scope,
preview launchers and remaining acceptance limits. No release or 360+ delivered
FPS is claimed.

Continue under investigation `high-refresh-rendering-20261011` in the original
production ledger. The owner approved 43,200 seconds; the guard remains enabled
with existing attempt/activity/stall limits. Recover ledger state after restart
or compaction, preserve counters and evidence, and stop on denial. Other active
investigations remain separate. Keep the issue record and known-issues page
updated, including negative results and outstanding live acceptance.
