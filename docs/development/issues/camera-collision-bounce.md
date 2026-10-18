# CAMERA-005: modern camera bounces upward over terrain

## Report, status and scope

Status: **reconstructed Goldwood pond regression fixed in tests; revised local
preview installed; owner live verification pending**, 2026-10-13. Severity: high
usability impact. See the follow-up below for the current runtime and evidence.

The owner reports sudden upward camera jumps while walking, looking around,
or using a low camera angle, described as hitting ground speedbumps. A later
report identifies apparent false contact above a raised pond lip directly in
front of Goldwood's spawn/entrance. The camera appeared above the lip but
responded as though blocked. The original report lacked a complete input trace.
The follow-up recovered area 47 and reproduced false contacts at that pond with
a reconstructed route; the reported exact movement sequence remains unrecorded.

This is separate from the three owner-validated transition/jump/focus fixes,
committed as 7b1ea4ba6228c126757b2b2e966ed7c0e1de8ca4. It overlaps the broader
CAMERA-002 scenery-driven pull-in investigation. Automated pond qualification
does not establish that all scenery cases or owner live feel are resolved.

Reproduced baseline: local modern-controls-state-fixes-20261013, runtime SHA-256
b2a9a6c6d3ccd706482e688f2bb784c2a548e171daad6ad87fb7815d7e23bb0e.
Private evidence ID: camera-collision-evidence-20261013. ROM bytes,
generated game code and personal saves remain private.

## Initial cause and pond hypothesis (first candidate)

The modern shoulder camera writes the native collision-resolved position
directly to avoid lagging raw look. The native normal-camera helper can lift
that endpoint away from the requested viewing ray to clear the floor. Its
floor/ceiling query includes a 16-unit margin. Separately, added visibility
probes start near Juno's feet and to either side; hiding his lower body can
retract the camera almost to him, even when the head-height view is clear.
Immediate changes in these two corrections cause sharp vertical/position snaps.

The installed baseline reproduces this on a copied Goldwood entry save:
lower the view, walk forward, strafe onto adjacent terrain, reverse, then turn
while stationary. Native height changes and feet-probe contractions are
separately recorded. In the mouse route, unchanged input orientation still
produces a maximum relative vertical step of 45.668 world units and a relative
position step of 174.106 units.

For the pond, distinguish a blocked focus-to-camera ray, endpoint floor/ceiling
clearance, and the old feet-only visibility test. A clear endpoint alone does
not prove that the view segment is clear; equally, hidden feet should not force
a camera move above a harmless low lip. Do not reduce the collision margin or
disable scenery checks without identifying the actual contact. The pond's
precise cause was not independently confirmed at this first stage; the follow-up
below records the subsequent geometry evidence.

## Reference principles

Unity Cinemachine documents a collision radius, separate tracking/collision
damping and different inward/outward recovery in
[Third Person Follow](https://docs.unity3d.com/Packages/com.unity.cinemachine@3.1/manual/CinemachineThirdPersonFollow.html).
Its [Deoccluder](https://docs.unity3d.com/Packages/com.unity.cinemachine@3.1/manual/CinemachineDeoccluder.html)
documents pulling forward, preserving height and holding a contracted distance
to reduce obstacle-driven oscillation. These are design references, not imported
engine code or proof that another engine's implementation fits this game.

Preserve input orientation; resolve obstructions along the camera boom when
possible; retain clearance; separate environmental response from raw rotation;
use elapsed simulation time for recovery. Do not smooth through solid geometry.

## Candidate implementation

[PcCameraBoom](../../../include/jfg/runtime/pc_input.hpp) separates the desired
view from the collision-constrained distance. The
[runtime adapter](../../../src/boot/native_boot.cpp) retains the game's scenery,
moving-model and floor/ceiling query rather than introducing a second incomplete
collision database.

- Keep the accepted camera point on the input-owned focus-to-camera ray.
- Remove feet-visibility probes only from Modern Controls' shoulder camera.
  The focus ray and native floor/ceiling clearance remain active.
- Derive a shorter candidate distance from the native correction, then validate
  it with the native query. Accept only a position that the query leaves
  unchanged within 0.01 world unit. Retain the inward near-plane margin.
- Contract immediately when safety requires it. Hold outward recovery for 80 ms
  after contraction, then recover using a 180 ms exponential time constant.
  This acts on distance; mouse/stick rotation is not filtered.
- Check recovery on the current ray: a turned view or moving object can
  invalidate last frame's safety. An intermediate distance can also cross a
  ridge even when the farther endpoint is clear.
- Bound the search to eight correction attempts plus a checked near-pivot
  fallback. If no acceptable point exists there, retain the native correction
  and reset distance history. This exceptional path can leave the ray; it is
  traced explicitly and was not reached in the qualifying routes.
- Preserve area/view lifetime, focus retention, scripted locks and player
  physics. Controller Mapping keeps its existing camera path.

This changes collision response and its modern-input integration. It is
**not a new continuous sphere/capsule sweep** and does not establish that every
native collision margin is correct. The bounded search may choose a
conservative distance rather than the farthest mathematically clear position.
A real nearby obstruction can still require a large immediate contraction.

## Attempts and results

| Attempt | Result / interpretation |
| --- | --- |
| Baseline low-view Goldwood replay | Reproduced off-ray height jumps and feet-driven contraction. |
| Full-range bisection | Vertical/position steps fell to 7.811/28.719 units, but median/p95 camera work reached 8.590/12.981 ms on reference timing. Rejected; not delivered. |
| Correction-derived bounds | Native-validated candidates usually require one extra query; removed full-range bisection cost. |
| Margin review | Avoided applying the inward margin twice: use raw native correction as the constraint input; retain legacy margin handling for exceptional fallback only. |
| Normal-height entrance pass | Full 200-unit distance across 176 samples; zero height/body correction or extra query. Checks the open entrance path, not the exact pond. |
| Adjacent depression/rim route | Final frame identifies a hollow stump, not the owner's pond. Retained as another obstacle check, not mislabeled as pond reproduction. |
| Geometry plot helper | Matplotlib unavailable; used existing Pillow for a private diagnostic diagram. No product dependency added. |

Final focused low-view results (relative to player motion, with unchanged input
orientation for the step metrics):

| Route / runtime | Samples | Max vertical step | Max position step | Camera work median / p95 |
| --- | ---: | ---: | ---: | ---: |
| Baseline mouse, reference timing | 514 | 45.668 | 174.106 | 1.045 / 2.899 ms |
| Candidate mouse, reference timing | 446 | 6.952 | 20.587 | 1.096 / 10.151 ms |
| Baseline dual stick, native timing | 581 | 45.130 | 172.387 | 0.768 / 1.254 ms |
| Candidate dual stick, native timing | 599 | 5.139 | 15.209 | 0.255 / 0.514 ms |

These are focused replay observations, not a universal performance guarantee.
The routes use corresponding input sequences but are not identical wall-clock
state trajectories. The reference-timing tail remains slower with extra native
queries; the preview's normal Play uses native timing. Do not claim a speedup on
every clock policy.

The final native dual-stick route has median one extra query, p95 two, maximum
six; zero exceptional fallbacks; ray error below 0.00005 unit; and zero idle
player displacement. The additional hollow-stump route has 307 samples,
zero fallbacks, ray error below 0.00003 and median/p95 camera work 0.290/0.659 ms.
Its distance can contract to four units near the obstruction: this is not
evidence that large scenery pull-ins are universally solved.

## Verification and delivery

[PC input tests](../../../tests/pc_input_tests.cpp) cover floor/wall constraints,
near-plane margin, inward response, outward hold/recovery, a ridge crossing an
intermediate recovery point, immediate input rotation, exceptional fallback,
near-vertical views and equivalent 30/60/120-update recovery. PC input, modern
controls, native audio source and audio DMA suites pass.

The final runtime also passes suspension and directional-jump regressions:
55 locked-view samples retain orientation and reject movement/jump input;
fresh look resumes; three takeoffs and 60 airborne samples retain expected
visual facing and native airborne steering. Original Controller Mapping's
final RDRAM is byte-identical to the previous qualified jump replay.

A hidden, owned-window normal-Play smoke passes with native timing and the
repaired audio source enabled through the normal default path. Saves are copied
for tests. No global input injection, desktop focus changes or unrelated-process
shutdown were used.

The existing **JFG Modern Controls Preview** desktop shortcut now targets the
local v1.1 package build/modern-controls-camera-collision-20261013.
The launcher is unchanged; the runtime and verified package manifest are updated.
The ROM, saves, mappings and 70 other profile files were verified unchanged.
The previous package and activation backups are retained. Runtime SHA-256:

24baa16f86dc927f80b92a991cc15e7158936609291fd81b98df95f5e50d07f1

Receipts include camera-qualification.json, final-camera-qualification.json,
camera-state-regressions.json, original-mapping-verification.json,
entrance-route-qualification.json and camera-preview-activation.json.
This is a tested local candidate, not a public release or owner acceptance.

## Live checks and further collision work

Use a freshly launched Modern Controls Preview. Check the reported Goldwood
pond lip, previous low-angle speedbumps, walls, narrow corners and the bridge.
Rotate while stationary and while moving; repeat with mouse and XInput.
Confirm that the camera preserves the chosen angle, does not pull in merely
to show feet, and returns smoothly after clearing an obstacle. Check that
nearby scenery does not clip through the view.

If the pond still produces a false contact, capture the requested endpoint,
native result, accepted distance, focus, scene and input orientation together.
Identify whether the contact is the focus segment, clearance footprint, or
moving-model collision before changing policy.

A broader camera collision engine would need a swept camera volume/near-plane
contract, triangle and edge contacts, initial-overlap handling, collision-surface
filtering, moving-model transforms, area invalidation and equivalent performance
qualification. Replacing a native query with a static-mesh-only ray or lowering
margins without contact evidence is insufficient. That work remains distinct
from the tested distance-response correction.

## Lessons and future high-refresh compatibility

Track desired view, physical obstruction and body visibility separately.
Smoothing a final off-ray position hides the cause, adds look lag and can pass
through geometry. Test raw input responsiveness, environment-induced movement
and collision-query cost independently.

Recovery uses elapsed simulation time and is tested at 30/60/120 updates per
second. It adds no render-frame movement multiplier. Future render interpolation
must not run gameplay/input adaptation twice or interpolate the camera through
an obstruction. Dynamic collision, unusual camera ownership transitions,
no-feasible-pivot configurations and the exact pond still need broader coverage.

The production guard remained enabled under the existing owner-approved
continuation, with investigation identity and all previous charges preserved.

## Owner follow-up: remaining inward jumps

The owner reports improved feel with the delivered candidate but still sees
unnecessary forward camera jumps, including at the end of the next session.
The new session report verifies the candidate runtime hash, but ordinary
launcher logs are not a full input/collision trace. Preserve the final map
snapshot and locate any separate recording; do not claim an exact replay from
basic logs alone. A further one-hour investigation was explicitly approved,
using the same guard/root and retaining all previous charges. The issue remains
open. Evaluate native clearance versus conservative distance selection before
adding further smoothing.


## 2026-10-13 follow-up: false inward contacts at the actual pond

### Report, recovered scene and recording limitation

The owner found the first camera candidate better, but still saw abrupt inward
jumps when the camera visibly cleared scenery. Their latest run ended near the
Goldwood landing-area pond. The ordinary preview had not been configured to
record detailed input/gameplay traces: its launcher report and final map/actor
snapshots cannot reconstruct the whole route. This was a capture-setup omission,
not a missing file the owner should have had to find.

The final snapshot identifies **area 47**, not area 21 used in the first
candidate's entrance/depression tests. The preceding snapshot also contains an
approximately 115-unit contracted camera near the actual pond. A copied save,
the recovered mesh and a reconstructed walk from the forest exit to the pond's
north rim reproduce the problem. This is the same location and obstacle, not
a replay of the owner's exact input sequence.

Private evidence: camera-inward-evidence-20261013, including owner-session,
pool-clearance-triangle-check.json, pool-boundary-comparison.json,
final-geometry-qualification.json and camera-clearance-qualification.json.
Personal saves, traces, collision meshes and images remain private.

### Two confirmed defects and an informative partial remedy

1. The first camera distance search accepted a halved seed when the native
   result only lifted the endpoint. A tiny height correction at a horizontal
   camera angle could therefore turn a 200-unit boom into 100 units in one
   update. The new shallow-lip unit case fails against that implementation.
2. The native height correction can report contact where the finite scene
   triangles do not intersect the camera clearance sphere. On the reconstructed
   pond route, the two baseline 200-to-100 pull-ins start at native lifts of
   approximately 1.77 and 21.07 units. The nearest finite terrain is respectively
   24.47 and 27.42 units away, beyond the native 16-unit clearance. The focus ray
   is also clear. This establishes a false height contact. Infinite-plane
   extrapolation is a plausible explanation for the guest height routine;
   its precise internal arithmetic has not been fully isolated, so do not
   present that narrower explanation as proven.

Refining only the accepted/rejected distance bracket reduced the worst fixed-view
pond step from 100 to about 23.57 units, but still honored false contacts and
could remain unnecessarily close. That build was retained as evidence and was
not installed for the owner. Damping inward movement would conceal this defect
and could allow clipping; it is not the remedy.

### Current implementation

[PcCameraBoom](../../../include/jfg/runtime/pc_input.hpp) now refines the
height-only accepted/rejected distance bracket. The halved point is a search
seed, not the final distance. The bounded refinement returns an explicitly
validated point near the clearance boundary rather than an arbitrary half.

[PcCameraBroadphase](../../../include/jfg/runtime/pc_camera_visibility.hpp)
adds a finite sphere-to-triangle clearance test, including edges and corners,
with a conservative numerical allowance. A triangle AABB rejection keeps distant
degenerate faces from defeating otherwise valid clearance proofs. Degenerate
geometry near the sphere, invalid coordinates, incomplete batch/face coverage,
unknown moving-model registration and possible moving-model bounds retain the
native response. Scene geometry and registered model bounds are read fresh for
the solve, not cached across level changes.

The [modern runtime adapter](../../../src/boot/native_boot.cpp) rejects a
native **height-only** correction only when both are established:

- The requested endpoint's 16-unit clearance sphere is clear of the complete
  decoded static mesh and conservative registered moving-model bounds.
- The torso-focus-to-endpoint segment is clear.

Corrections that change horizontal position still use the native response.
Possible floor, ceiling, wall, edge and moving-model contacts keep it too.
The native query remains the authority when geometry cannot establish clearance.
This is a finite-volume validation of a false height result, not collision
disablement, a reduced clearance radius, extra inward smoothing or a complete
swept-sphere replacement. The existing time-based outward recovery is unchanged.
The original Controller Mapping path is unchanged.

### Verification and performance limits

| Check | Result |
| --- | --- |
| Reconstructed area-47 pond, modern stick input | Baseline had two 200-to-100 pull-ins. Final candidate remains at 200 units across 244 north-pond samples (float minimum 199.999954); no fixed-view inward step. |
| Same pond route, scripted modern mouse input | Full distance during the flat north-rim crossing; view-ray preservation and zero exceptional fallback checked. This is runtime input replay, not a physical-device test. |
| Real obstruction response | Entrance portion still contracts below 100 units; collision response was not globally disabled. |
| Low-angle terrain regression | 599 final samples, no exceptional fallback, maximum deviation from the requested ray about 0.000031 units. |
| Finite geometry and search tests | Shallow lip, changing clearance, actual pond triangle, triangle face/edge/corner contact, nearby and distant degeneracy, unknown geometry, and existing floor/wall/recovery tests pass. |
| Four portable suites | PC input, modern controls, native audio source and audio DMA broker pass. |
| Original Controller Mapping | Complete final RDRAM matches the prior qualified reference byte-for-byte: 1243885c7b66a45c8778fbcea6cbcce9436018342ee8bf97a9516fe5b9b69c9e. |
| Normal Play path | Hidden owned-window smoke passes with native timing and the normal synchronous audio sample provider, without private enable flags or global desktop input. |
| Profile preservation | Activation verifies the launcher, ROM settings, saves, bindings and other profile files; only the runtime selection/shortcut changes. |

On the final stick pond run, camera work is 0.428 ms median, 2.512 ms p95 and
9.257 ms maximum; the old candidate measured 0.541/2.290/7.921 ms. These
native-clock runs share scripted inputs but not identical scheduling/positions
or machine load; they are cost observations, not proof of a tail-latency
improvement. Extra native query counts are median 0, p95 1, maximum 12.
The low-angle final run is 0.255/0.660/4.289 ms. A broad scene and frame-time
qualification is still required before treating the collision engine as complete.

### Installed preview and next live check

Installed local v1.1 package: build/modern-controls-camera-clearance-20261013.
Runtime SHA-256:
15c40b95d95cf48a388f652419efe7bedd0f42f6d6b641059a1ff65498241613.
The v1.1 launcher binary is unchanged. JFG Modern Controls Preview points to this
package; the previous camera-collision package and activation backups are retained.
This is local preview delivery, not a public release or an owner-validated fix.

Use the new Desktop **JFG Camera Capture** shortcut for the next pond check.
It resolves the current Modern Controls Preview target, copies the initial save
and control settings, and starts the launcher's explicit diagnostics path.
Captures go under Desktop/JFG Camera Captures with a timestamped session folder.
The game still uses the normal preview profile; close it after the reproduction.
The helper does not rely on inherited JFG variables that the launcher sanitizes.

Pending: owner live verification at the reported pond/end-of-run route, broad
scenery and moving-model coverage, swept camera volume/near-plane behavior,
initial overlaps and other camera ownership modes. Keep CAMERA-002 open.
At 60+ simulation updates, clearance stays a geometric distance and recovery
continues using elapsed simulation time; do not substitute render-frame counts.
Higher update rates still need cost/temporal qualification and do not make this
a swept collision solver.

Transferable lesson: instrument the desired endpoint, raw native correction,
accepted distance, finite nearest geometry, view ray and cost separately.
Prove actual contact before tuning damping. An endpoint height solver can disagree
with finite-volume geometry, and a coarse search can amplify a tiny correction
into a large visual jump. Validate the recording pipeline before asking an owner
for another capture; a final snapshot is useful context but is not a replay.
