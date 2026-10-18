# RENDER-006: Late tutorial objects jitter; live cadence drops

Status: **deforming bridge remedy verified in bounded rendering tests; owner visual acceptance and broader object coverage remain open** (2026-10-16 UTC).
Severity: high; the full tutorial does not meet smooth-play acceptance.

## Report and affected build

The owner played the tutorial with log-only diagnostics at Custom 120, VSync on.
Near the end, wooden crates, the exit light and a bridge visibly stuttered or
jittered, in the later cave and when leaving the bridge area. Exact room IDs,
input sequence and symptom timestamps were not recorded. This is broader than
the earlier gem/drone report; do not assume those prior causes explain it.

Package: `high-refresh-combat-spans-20261013`, implementation
`a93faa5ef61b86536327ffa83d48fc95835a21ff`, runtime SHA-256
`e7e3af29fd9aeec9e037da93a27d0e50272ba7bbb6b4ac77eb5f6536dd2d6fc5`.
The launcher support log confirms the runtime hash, and the native log confirms
native elapsed timing and instruction spans. Both exit codes are zero.

## Recorded evidence

Private capture `frames-20261013-145735-36772-0.csv` contains 85,946 presentation
rows, from elapsed 1.990 to 733.829 seconds. SHA-256:
`34dc7c0519297f267800720d3c86ec3b5c8f45eaae9fed95f35308f8c340fb36`.
Queue drops, render-counter discontinuities and timestamp/interval mismatches
are all zero. The overlay was off. Raw logs and reviewed support files remain
on D:; [aggregate evidence](../../../evidence/tutorial-owner-timing-review.json)
records formulas, windows, provenance and limits without personal save data.

Times below are minutes:seconds since logger initialization, not since gaining
control of Juno. Rates use counter deltas divided by the sampled wall duration.

| Window | Rendered output/s | New scene submissions/s | Worst present interval |
|---|---:|---:|---:|
| 0:30-3:30 (may include initial waiting) | 119.66 | 29.92 | 15.71 ms |
| 7:15-7:35 | 110.05 | 20.16 | 70.52 ms |
| 10:40-10:45 | 119.66 | 26.66 | 10.90 ms |
| 10:45-10:50 | 116.45 | 21.61 | 34.41 ms |
| 10:50-10:55 | 114.85 | 24.85 | 31.78 ms |
| 11:55-12:00 | 119.94 | 30.03 | 8.66 ms |

The final two minutes average 118.56 output/s and 28.34 scene submissions/s,
with 25 present intervals over 20 ms. Approximate reconstructed scene intervals
include 336 above 40 ms and 167 above 50 ms. The ordinary target is about
33.3 ms per scene; these are actual source-cadence irregularities, not merely a
rounded HUD reading. The last section is intermittent, not continuously slow.

There is a 594.43-ms present-return gap ending at 11:52.947 and a 1,564.36-ms
gap ending at 10:02.762. Pause, focus and room-transition markers are absent,
so these gaps cannot be labelled unprompted gameplay stalls from this log.
Earlier near-20-Hz windows above do not depend on those long gaps.

## Interpretation and remaining work

- Source-cadence drops are demonstrated. The earlier bounded combat replay
  passes remain valid for those fixtures, but do not establish full-tutorial
  acceptance. See [PERF-001](goldwood-combat-slowdown.md).
- The reported object-specific jitter could also involve interpolation identity,
  coverage, camera-relative transforms or visibility. Those are hypotheses;
  this CSV contains no object identities, transforms, interpolation phases or
  pictures. It cannot prove whether a crate or bridge interpolated correctly,
  or whether all visual jitter coincides with the measured cadence drops.
- SCENES measures screen-scene submissions, not every simulation tick. OUTPUT
  measures successful present returns, not unique poses or physical scanout.
  Reconstructed scene timestamps use independently sampled atomic fields and
  are approximate. No CPU/GPU cost profile or audio-underrun counter was captured.
- The prelaunch check found no other games; competing processes were not
  monitored throughout the owner run. External load attribution remains open.

This was an owner-requested review of existing files and persistent issue
documentation. No new game, replay, build or optimization worker was run; no
runtime remedy was attempted. The production guard remains enabled and the
same investigation remains shelved with 23 attempts and 22 supporting jobs.
No counters or allowances changed. Further autonomous work must use the same
guarded investigation and receive an authorized continuation.

Transferable lesson: qualify a complete live route and short-window source
cadence, alongside per-object visual continuity. A near-120 presentation average
and a successful short combat fixture cannot establish smooth full-level play.

## Approved two-hour continuation

Owner approved two hours on 2026-10-13 to investigate these findings. The same
production investigation and root retain all previous counters; guard enabled.
Admitted one additional attempt and supporting job, plus 7,200 seconds. Tests
will be muted and use copied saves. No current fix or reproduced object-specific
cause is claimed. Competing controls tests must finish before timing comparisons.
Large evidence is retained under the private tutorial-investigation-20261013 ID.

### Rigid-object candidate

The supported US camPushModelMtx consumes an ObjectTransform pointer and emits
a transient combined matrix. The existing semantic path recognizes skin palettes
and sprites, but not this rigid path. Candidate binds emitted matrices to live
actor/model/camera identities and retains exact geometry/motion checks. Matrix
bytes, actor lifetime, free/reuse, and task age validate bindings. A diagnostic
shadow history measures old versus new matching only when tracing is requested.
This is implemented candidate code; qualification and actual affected-room
coverage remain pending. No object-specific visual fix is claimed yet.

The first source-edit step stopped on a nonunique text anchor after writing the
tracker and boot observer. It was not built. The renderer-only continuation
uses a unique struct anchor; the failed step is retained in private evidence.

All four focused suites passed in the first rigid build. Its final diff check
then rejected one whitespace-only blank line in the new test, so the dependent
game step correctly did not launch. Whitespace is repaired; the qualified binary
is unchanged. State and scene coverage qualification follows.

Step07 preserved exact Original and recorded-native-clock state, then stopped on
an overbroad comparison of Original versus Custom raw RAM. Different rendered
framebuffers are expected across those modes; step09 checks all other bytes
explicitly and preserves the initial failure. No performance run was launched
by step08 because its dependency gate failed.

The first timing comparison overlapped a separate launcher compiler, was
explicitly excluded by process monitoring, and did not advance to the candidate.
The copied-save coverage probe is functional evidence, not performance.

### Candidate findings and rejected projection rule

The initial identity candidate preserves exact Original and recorded-native-clock
states, and all non-framebuffer bytes across Original/Custom. Two compiled
entry-metadata checks, 17 normalizer tests and four focused C++ suites pass.
The first Goldwood rigid trace has zero improved or regressed matches. This
establishes coverage, not a reproduced remedy for the owner's late cave.

Detailed camera-turn/walk geometry traces show screen-motion, depth and camera-
plane rejections on longwoodbridge, ForestCrate and exit markers. An offline
prototype ignoring corners outside both endpoint views produced no newly matched
visible samples and some visible regressions. It was rejected before editing
production logic; the paired-game step therefore did not run. These observations
do NOT prove that off-screen vertices caused the owner's visible jitter.
The temporary geometry dumper is removed. Its overhead/underruns are diagnostic
artifacts, not clean runtime performance measurements.

One clean 120/VSync comparison reduced median update wall time excluding explicit
clock wait from 25.245 to 22.914 ms after direct entry metadata plus rigid identity.
Both had zero audio underruns; source cadence still had occasional late updates.
The repeated baseline overlapped a launcher build and was excluded. An earlier
comparison also overlapped that task. A quiet preflight that counted idle MSBuild
pool nodes was cancelled; the corrected gate checks pooled-node CPU activity.
No full-tutorial or late-cave slowdown resolution is claimed.

Owner then explicitly requested merging current develop. This candidate and
evidence are checkpointed before that merge; the combined source must be rebuilt
and qualified before any preview replacement. See
[aggregate investigation](../../../evidence/tutorial-rigid-entry-investigation.json).

## Develop integration (2026-10-13, qualification pending)

Owner requested merging develop `63e49a7a71c3a914d2adc1d91fdcf00bef30d38e`
into high-refresh checkpoint `60cb549bceb023d416dc7721793e809d33ac0699`.
The merge preserves the high-refresh recovery policy, semantic/rigid identities,
video dialog and render diagnostics alongside develop camera, controls, native
audio sample source, profiling and render-viewport fixes. The newer diagnostic
clock-stall environment name accepts the previous high-refresh name as a fallback;
existing high-refresh trace event names remain stable. Both issue histories are
retained. Build and replay results must be recorded before delivery; a successful
merge alone does not establish late-cave or full-tutorial acceptance.

The second merged combat comparison overlapped a separate Scooby-Doo launcher
build. Its first window fell to 27.92 scenes/s and is explicitly excluded from
acceptance. The dependent camera step did not launch. After the live-window
regression, the remaining combat/360/pause cases use a fresh quiet preflight;
the first clean merged combat case remains valid. No foreign work was stopped.

## Develop merge qualification (2026-10-13)

Develop `63e49a7a71c3a914d2adc1d91fdcf00bef30d38e` is integrated with the
high-refresh candidate. Fresh generation has identical game files, and 25
Python checks, 13 focused C++ suites, viewport tests, 120 video RmlUi checks and
546 modern-controls UI checks pass. Original/Custom preserve every non-render-
buffer guest byte. A new native-clock recording with the native audio source
replays exactly. Older pre-merge raw hashes differ after the imported behavior
changes; no claim of cross-revision bitwise equivalence is made.

Review caught a real message-number collision missed by the initial non-live
dialog test. The new live test reproduces swallowed widescreen relayout before
the fix, then passes after the video-state query moves to its own message.
Apply, pause/resume and aspect changes are checked on an isolated desktop.

merged-combat-120-a: 119.7/29.81, 120.0/29.79, 119.8/29.84, 119.6/29.80 OUTPUT/SCENES; merged-combat-120-final-b: 119.7/29.85, 119.9/29.74, 119.9/29.84, 119.8/29.80 OUTPUT/SCENES; merged-combat-360: 352.0/29.79, 356.4/29.75, 357.3/29.83, 352.1/29.76 OUTPUT/SCENES. All three clean combat runs have zero audio underruns and
no foreign compiler/game overlap. Pause/resume also passes. The full-circle
trace has no backward presentation phases, and the reconstructed area-47 pond
retains full north-facing camera distance. These are bounded muted tests,
not a full tutorial or physical-display acceptance.

The owner-reported later-cave objects and full-route cadence drops remain open.
Rigid identity is qualified as a candidate; no visual remedy for that exact
unreproduced report is claimed. The rejected projection rule remains absent.
See [merge qualification](../../../evidence/high-refresh-develop-merge.json)
for rates, phase counts, provenance and limits. Preview packaging follows.

## Local merged-preview delivery (2026-10-13)

The qualified combined runtime/launcher is delivered as
`high-refresh-develop-merged-20261013`. The existing desktop **JFG - High Refresh
Preview** shortcut now selects it; close and reopen an older launcher to use
the new package. Existing save, video/audio/control settings and log-only
performance preferences are preserved byte-for-byte. Package validation and
isolated setup initialization pass. Source merge: `f70b513c896df107163f2d72abed344a87dc2517`.
Runtime SHA-256: `1381e018fa2ec2cc8cc0e9c64c785753485b4068a81a08b7f29f8ca0cee2e4f8`.

This is a local preview, not a public release. Full-tutorial performance and
the exact later-cave crates/exit-light/bridge jitter still require live coverage;
their issue records remain open. All runtime tests were muted. Large private
evidence and the new package stay on D:. The production guard remained enabled
under the same investigation and counters throughout.

## Expanded owner route capture (2026-10-13, preparation)

The owner clarified that the earlier recording includes a long wait at the
start because recording had begun without their realizing it. That waiting
period must be excluded from active-route reports and review playback. The
old timing-only capture has no exact input boundary, so no precise idle cutoff
is invented. Retain the original evidence and timestamps for reproducibility;
label old elapsed windows as session time, not active gameplay time.

The owner approved one guarded hour to prepare and collect a fresh route.
The explicit launcher diagnostics option now also enables the existing RT64
interpolation trace (display phase and actor/model matching), alongside inputs,
actor/camera events and timing. Ordinary Play still strips inherited diagnostic
variables. A fresh capture starts on Play; waiting in the launcher is excluded.
The status states that diagnostics are recording. Initial save/settings are
snapshotted for replay. Frame CSV logging is enabled without an overlay in the
dedicated muted copied profile, with all capture artifacts on D:.

This is observation plumbing, not a jitter fix. Expanded synchronous traces
can affect timing; observed stalls must later be compared to a normal run.
Initial in-game waiting must be trimmed only in derived analysis/viewing, with
raw VI/input ordering preserved. Setup validation passes; owner route collection
and visual acceptance remain pending.

Validation: 640 launcher checks (including explicit trace paths, inherited-variable
stripping and duplicate rejection), 15 setup checks, six graphics-policy checks
and viewport checks pass. A muted live smoke writes all eight requested streams,
a v8 modern-input recording, rendered/source frame CSV, final image and guest RAM.
It includes 10,485 display phases, 8,592 rigid-object samples and 2,638 semantic
samples. The smoke close harness initially selected the runtime wrapper PID,
not its render child; descendant lookup recovered and closed the owned game
normally. The attempt to isolate that helper did not propagate the desktop to
the render child (it used Default); do not treat this as isolated-desktop or
clean performance evidence. No timing or visual-fix claim relies on this smoke.
The delivered runtime is unchanged from the qualified develop merge.

## Owner repeat-route report (2026-10-13, review in progress)

The expanded capture closed normally. The owner reports a roughly one-second
laggy response when Juno is hit, and the exit light marker and bridge jittering
again near the end. "Jumping" means visual jitter, not actors actually jumping.
Juno's ground shadow also appears to update sluggishly; this last symptom is
subjective and not yet established as a separate regression. Preserve the
separation between whole-frame stalls, low source cadence, object history
rejections and shadow-mask updates. No cause or fix is confirmed yet.

Affected package: high-refresh-route-capture-20261013, launcher source
9407ccd726001bad4e2372af3bdfb94688394209, unchanged merged runtime
1381e018fa2ec2cc8cc0e9c64c785753485b4068a81a08b7f29f8ca0cee2e4f8.
Input v8, camera/actor trace, interpolation trace, frame CSV, initial profile and
final RAM/image are retained privately on D:. Initial launcher waiting was not
recorded. Exclude initial in-game waiting from derived active-route reports;
retain raw VI ordering. Expanded logging overhead is a possible confounder,
not a demonstrated explanation for the reported symptoms.

The first replay fixture rejected repeated controller polls sharing a VI because
it selected retrace-range input ordering; the raw recording is retained and
valid under recorded-poll ordering. A second, muted hidden replay using recorded
VI timing runs cleanly with no audio underruns, but never leaves area 21, while
the owner reaches areas 16, 26 and 157. It is NOT accepted as an exact route or
late-cave performance comparison. Native elapsed-clock state divergence and
replay-input correspondence must be checked before making a causal comparison.

### Completed framebuffer-write window candidate

The muted replay's extra trace identifies 297 completed-sequence events with a
CPU write to the active screen framebuffer (0x10ff00 or 0x135700); all replayed
an entire finite output window. The existing path already selects the latest
interpolated image, so those outputs repeat one endpoint rather than new motion.
Candidate presents that completed endpoint once per write event while retaining
operation execution, pause, capture and unlimited handling. This does not yet
correct the existing mismatch between CPU changes on the base target and the
selected interpolated target. Route visual acceptance and timing comparison are
pending. The focus fix passes input tests but did not restore owner-route
correspondence by VI 7000; later-room reproduction remains unresolved.

### Stationary rigid object camera candidate

A fresh bounded bridge orbit again reproduces history rejection when a bridge
vertex approaches/crosses the camera plane. Independent matrix reconstruction
shows a stationary bridge and small camera steps (an example is about 8.55
world units and axis cosine 0.9988), while the projected-vertex test rejects the
whole shared transform. The previously rejected generic offscreen-vertex rule
remains absent.

Candidate captures immutable world/view/projection operands at the already
validated US camPushModelMtx return, keeps actor/model/camera lifetime binding,
and checks that their product describes the actual emitted fixed matrix.
Unchanged world/projection plus a bounded rigid camera step may interpolate a
stationary object across the camera plane. Unknown objects, changing poses,
changed projection, cuts, conflicting bindings and invalid matrices retain the
existing conservative path. No guest state is changed. Synthetic near-plane,
teleport, model-change, projection-change, singular and invalid-value checks
are added. Build, exact endpoint-state and rendered-image checks are pending.

The first camera-candidate build exposed a compatibility constraint: RT64's
adapter translation units use C++17, so a shared header cannot default a C++20
comparison operator. Explicit array comparisons restore C++17 compatibility;
the blocked image test did not run against a stale binary. Evidence is retained.

The first immutable-pose candidate passes its unit tests, accepts all 3,732
observed matrix/operand products, and preserves every non-framebuffer guest
byte in the bounded orbit. It does not yet improve bridge matching (2,610 of
3,500 before and after). Therefore it is not qualified as a bridge fix. The
captured VI 4056 images face the pond and do not visibly show the rejected bridge;
a visible rejected sample and the failing pose-continuity condition are being
checked before further changes. Full poll-order native routes also diverge:
the baseline reaches area 47, while the candidate reaches areas 16 and 26.
Their whole-route FPS averages are not a controlled performance comparison.

The missing pose acceptance is explained by guest lookup-table trigonometry:
retained view axes have squared lengths near 0.99869, just outside the initial
0.001 orthonormal tolerance. The revised check bounds near-unit axes, divides
by their measured squared lengths when recovering the camera eye and normalizes
orientation comparisons. Tests retain rejection of actual scale/cuts. A
vertex-inside-screen test alone is insufficient for large bridge triangles;
homogeneous polygon clipping is used to choose visible rejection captures.

## Review qualification (2026-10-13)

The completed-window pacing defect is reproduced and corrected in tests.
A VI with CPU framebuffer writes was presenting the already completed endpoint
for another whole 4/6-output interval. It now presents that endpoint once.
Framebuffer operations, pause, final captures and the Unlimited path remain.
In the bounded recorded-time comparison, repeated presentation phases fall
from 995 to 35, with zero backwards phases. These runs do not establish exact
whole-route input correspondence. The existing base-versus-interpolated target
CPU-write limitation described above remains open.

The stationary-object camera extension now accepts all 3,732 captured operand
products and improves bridge matching from 2,610/3,500 to 3,500/3,500; the exit
arrow remains 230/232. It preserves every non-framebuffer guest byte. However,
homogeneous triangle clipping finds that all newly accepted bridge samples in
this orbit are offscreen. This is tested coverage, NOT proof that the owner's
visible bridge/light jitter is fixed. Camera cuts, lifetime changes, changing
world/projection, invalid matrices and geometry keep conservative rejection.
The header must remain compatible with RT64's C++17 translation units.

A further native-clock tail attempt spent its entire gameplay section in area
21 rather than reproducing the late bridge. Its 1,176 geometric frames produce
no newly matched visible objects. Preserve this informative failure; do not
repeat full-route FPS comparisons without clock/input correspondence. Available
owner inputs lack the exact simulation-clock history needed for faithful route
recreation, and poll-order baseline/candidate paths diverge.

Final clean runtime SHA-256:
`f6c420fdb4ef5c7b4c361e334ca86e91a7ca94126c866e5645ab5f2db5d7546c`.
Ten focused C++ suites and four presentation-analyzer tests pass. Temporary
geometry dumping is removed. Original/Custom 120 have identical save hashes
and zero differing guest bytes outside the known rendering buffers. These
readback-heavy state probes report 2/3 audio underruns; they are not clean
performance evidence. The separately measured native Play tests below have zero.

At 1164x873, isolated muted Goldwood combat with normal and expanded logging
holds 119.65-119.95 OUTPUT and 29.76-29.86 SCENES across the measured windows.
The expanded trace contains 18 damage events. All live runs have zero audio
underruns and zero dropped diagnostic rows. A 360-cap run delivers
349.89-354.92 OUTPUT / 29.73-29.82 SCENES, so it does not qualify sustained 360.
Pause/resume passes; its deliberate 1.25-second pause is excluded from a
slowdown claim. These are bounded checks, not the owner's full-route resolution.

Active owner-route reporting now starts at first non-neutral gameplay input,
VI 2010. The earlier VI 1978 position change was entry settling, not player
input. Raw recording order/timestamps remain intact. The active route averages
113.73 OUTPUT / 28.14 SCENES over 225.85 seconds, with 211 intervals over 20 ms
and 67 over 40 ms. Its 1,466 ms maximum includes room-transition coverage and
must not be attributed to a hit without correlation.

The exact visible crate/light/bridge report, full-route damage slowdown and
[shadow cadence](shadow-cadence.md) remain OPEN. Lessons: completed framebuffer
writes do not authorize a fresh animation interval; immutable transform
operands must be tied to emitted matrix bytes and object lifetimes; matching
counters and offscreen geometry cannot establish a visible fix. Aggregate
[evidence](../../../evidence/high-refresh-route-review.json) preserves the
qualification scope without ROM-derived meshes or save files. Raw evidence is
retained privately on D:. No public release or full-game acceptance is claimed.

## Local route-review candidate delivered (2026-10-13)

Implementation commit `34c9986224c0508a45d1f7d1caa10f2b6b377526` is installed as
`high-refresh-route-review-20261013` on D:. Both **JFG - High Refresh Preview**
and **JFG - Jitter Capture** desktop shortcuts select it, preserving their
existing profile and recording arguments. Close and reopen an older launcher
to load it. All 14 package entries verify by size/hash, isolated setup resolves
the new runtime, and both existing save/settings profiles are unchanged.
The launcher remains the already qualified expanded-capture build.

A packaging invocation initially omitted a PowerShell switch prefix and stopped
before changing either shortcut. The corrected guarded continuation revalidated
the staged package and old shortcut backups, then completed delivery. This was
a delivery-script error, not a game test failure; evidence is preserved.

This is a partial local correction, not a public release or a declaration that
the tutorial is fixed. Completed-write pacing and hidden replay input are
corrected; stationary camera coverage is extended. Exact full-route damage
slowdown, visible late bridge/light jitter and shadow cadence remain open.
Qualification and runtime identity are in
[the delivery evidence](../../../evidence/high-refresh-route-review.json).

## Four-hour guarded continuation (2026-10-13)

Owner approved four more hours after the route-review candidate delivery. The
same production investigation, root and counters are preserved, with the guard
still enabled. Focus: isolate the damage-correlated source-rate drop, obtain a
visible reproducible later-room bridge/light fixture, and qualify shadow/body
cadence. Prior offscreen matching improvement is not a visible fix. Use copied
profiles, muted hidden owned windows and D: evidence; do not repeat mismatched
whole-route averages as a causal comparison. Work remains open.

The stationary-object subset regression is now reproduced in a focused test:
changing the culled vertex/triangle key discards history despite an unchanged
owned model pose and continuous camera. A candidate retains a separate immutable
owner/pose endpoint across subset changes. It still rejects recycled or ambiguous
owners, camera cuts, invalid operands/vertices and changing model/projection.
This test addresses a concrete coverage hole; visible late-room qualification
is pending. Diagnostic function self-time collection is also added under the
existing finite CPU-cost probe option to distinguish increased guest work.


A direct camera candidate binds world-space batches to a camera/room generation
and immutable emitted view/projection operands, independent of the culled
triangle set. Its history regression fails before the change and passes after;
camera cuts, inconsistent claims, nonfinite geometry and level changes reject.
The new identity test initially left a shared test fixture in a different room;
restoring that fixture fixes its later unrelated sprite assertion. Both focused
suites now pass. Runtime visible coverage still needs review.

A CMake composition defect also dropped the regular-workload GPU timestamp
observer when interpolation replaced the generated source. The candidate now
composes the audited copies. A zero workload submission count in an affected
profile means absent instrumentation, not zero GPU cost. The temporary private
shadow probe's initial getenv warnings-as-errors failure was corrected with the
Windows environment API; no game launched from that failed build.

The explicit camera prototype was **rejected before delivery**: exact recorded
clock/input coverage found 70,851 formerly matched batches rejected, and zero
additional matches. Multiple emitted cameras reused a logical identity with
different operands. Synthetic identity tests alone did not establish coverage.
The prototype and its tests were removed; existing camera interpolation remains.
Private evidence: route-fix-20261013/camera-coverage-review.json.

A separate completed-presentation correctness defect is now reproduced in the
trace: CPU writes target the displayed framebuffer address, but the ordinary
write pass only updates its base render target. Completed high-refresh scenes
then select the retained interpolation endpoint, hiding those changed pixels.
The candidate applies the same CPU changed-pixel mask to that retained endpoint
under the existing workload and pool ownership, without replaying old phases.
This is a framebuffer update correction, not yet proof of the reported bridge
or exit-light jitter. Paired presented-frame and guest-state checks follow.

The completed-write paired clock replay preserves every guest byte and the save,
with zero backward presentation phases in both cases. The corrected run applies
234 completed CPU-write operations to the retained endpoint.
The bounded image sequence is separately reviewed; matching game state alone
does not establish the owner's exact bridge/light visual acceptance.
Private evidence: route-fix-20261013/endpoint-write-review.json.

The bridge fixture emits many batches for one rigid model. The first subset
candidate required a single batch per owner, so it contributed no matches there.
A revised candidate accepts several batches only when every claim has the same
validated immutable model/view/projection and matrix; a contradictory or invalid
claim rejects secondary owner history. It can preserve camera phase when all
culled subsets change together. The new split-model regression fails before and
passes after, including identical-shape batches. Invalid poses, conflicting valid
poses, camera cuts and changing model transforms retain conservative handling.
Visible bridge/light qualification remains pending.

## Variable scene cadence (2026-10-13 continuation candidate)

The owner's later-cave VI 13600-15200 window includes five four-VI scene
intervals, four one-VI intervals and one ten-VI gap. The old continuity policy
accepted only transitions between two and three VIs; four-to-two transitions
discarded interpolation for both the delayed scene and its recovery. Twenty-one
scene records have a zero source interpolation rate in that window, with whole
camera groups held at endpoints. That is a measured timing-policy defect, not
proof that all individual bridge/light matching faults have been resolved.

The candidate accepts actual bounded one-to-six-VI intervals regardless of the
prior interval. Geometry/lifetime, pose and motion rejection remain unchanged;
long gaps, repeated timestamps, invalid data and camera cuts still hold.
The added delayed-update/recovery test fails before and passes after. Source
FPS continues to report real scene production; this does not claim to remove
CPU combat drops or manufacture extra simulation updates. Replay qualification
and live acceptance remain separate.

The secondary owner-history experiment was rejected after the bridge orbit
and three recorded-clock fixtures produced zero additional subset matches.
Consistent duplicate batches passed synthetic tests, but there is no measured
benefit in these real samples. That additional map/pose work is removed from
the delivered candidate; existing exact rigid identity/camera matching remains.
The prior direct-camera experiment likewise remains rejected. Preserve these
failed attempts as evidence rather than labeling them visible jitter fixes.

### Procedural exit-light candidate, 2026-10-13

US behavior 68 calls beamDraw (overlay 76 in the pinned decomp), which generates two alternating eight-vertex buffers in actor control data, outside standard model geometry. Added host-only logical vertex ownership using actor lifetime, control allocation, shape flags and the verified emitted model/view/projection. Buffer changes retain logical vertex identity; invalid flags, ambiguous owners and stale matrices fall back. Synthetic buffer/lifetime validation passes. Actual visible-light coverage and full-route visual improvement are pending; this is not a resolved report.

## Continuation qualification, 2026-10-13

The selected correction keeps bounded one-to-six-VI scene intervals continuous
instead of discarding the delayed update and its recovery. The exact-clock
cadence comparison removes 12 unnecessary holds with no new rejected matches,
backward presentation phases, guest-memory differences or save differences.
Completed CPU framebuffer writes now update the retained displayed endpoint
with the same changed-pixel mask; 234 operations were applied in the paired
fixture without changing guest state.

The procedural LightBeam path now owns logical vertices across its two DMA
buffers and uses verified actor lifetime, control allocation, shape flags and
model/view/projection operands. Ownership lookup is per DMA batch. Synthetic
buffer/lifetime/ambiguity tests and real non-target beams exercise it; quiet and
dense clock replays preserve all guest bytes, saves and instruction counts.
This remains a candidate for the owner's exit marker, not a verified visual fix.

The controlled room captures are **not visible bridge/exit-light acceptance**.
Input/level correlation found movement and images before room 26 entry in the
earlier attempts; beams recorded there include other scenes. The landmark-based
attempt and a generation-stability retry remained in room 21. Room-26 endpoint
metadata in other attempts showed the target beams disabled by their draw flags.
Preserve those failures; final player location or actor matching counts alone
cannot qualify an earlier screenshot. The exact crate/bridge/exit-light report
is still open.

The direct-camera and secondary rigid-subset prototypes remain rejected and
are absent from the selected source. See [aggregate evidence](../../../evidence/high-refresh-route-fix.json).
No full-route or physical-display resolution is claimed.

## Local continuation delivered, 2026-10-13

Implementation commit aaae085c7be7d6ce140cde06c683ef19082470ee is installed in
high-refresh-route-fix-20261013 on D:. Both **JFG - High Refresh Preview**
and **JFG - Jitter Capture** desktop shortcuts select the new package, with
their profile/recording arguments preserved. Close and reopen an old launcher
to load it. All 14 package entries verify by size and hash; isolated muted
setup selects the correct runtime. Both existing save/settings profiles are
unchanged. The launcher remains the qualified expanded-capture build.

This is a partial correction. Delayed-scene holds and retained CPU-written
endpoints are corrected in bounded tests, timing/history overhead is reduced,
and logging drains correctly. Procedural beam ownership has synthetic and
non-target coverage. The native Unlimited scene-age candidate was rejected
for worse frame spacing and is absent.

The owner's exact damage/combat slowdown, visible late crates/exit marker/
bridge jitter and shadow cadence remain open. Unlimited still has source
starvation/audio errors. Initial waiting was excluded from gameplay analysis;
the original capture was preserved. Neither a build nor hidden-window FPS
establishes visible resolution. Runtime identity and measured limits are in
[the delivery evidence](../../../evidence/high-refresh-route-fix.json).

## Five-hour guarded continuation, 2026-10-14 UTC

Owner approved five hours after the route-fix preview. The same operational
investigation/root and all prior charges are retained; the guard stays enabled.
Selected baseline is implementation aaae085 with documentation e51b0af and runtime
1b0bdd0f. Work prioritizes actual combat source-rate cost and a visibly qualified
late-room fixture. Preserve original game timing, muted owned tests, copied saves,
and raw evidence on D:. The scene-age, direct-camera and secondary subset
experiments remain rejected unless new evidence supports a distinct correction.
No bounded replay, synthetic match or build alone closes the owner's reports.

The initial 2026-10-14 stage/alias renderer builds retained stale generated
layout code. Their visual/performance acceptance is invalid and is being rerun;
see [BUILD-001](stale-generated-renderer.md). Guest-state equality alone did not
detect this private build error. The delivered preview is unaffected.

The coherent near-beam capture now visibly includes the room-26 exit light;
all 48 logical vertices match during the captured pan. The far-exit-facing
sequence also visibly includes the bridge. Whole-orbit telemetry still has
isolated rigid-pose validation failures. Source inspection found the validator
reassociates world/view/projection using double arithmetic, whereas the guest
computes single-precision world * (view * projection) with a specific paired
sum order. A bounded matching diagnostic records rejected operands to determine
whether this numeric mismatch causes the remaining isolated holds. This is a
new concrete hypothesis; no broad tolerance relaxation or jitter resolution is
claimed. Raw captures remain on D: under combat-focus-20261014.

## Captured numeric validation defect, 2026-10-14 UTC

The near-beam pan captures 19 rejected rigid poses, including ten for the
actual room-26 bridge. Repeating the guest's single-precision operation order
matches every rejected endpoint within one signed 16.16 unit. The prior
double-precision reassociation differs by enough to trigger conservative
rejection near cancellation. This is a demonstrated unnecessary hold, not a
reason to weaken topology, lifetime, finite-value or camera-cut checks.

The candidate computes view * projection, then world * that product, preserving
the guest's (t1+t2)+(t0+t3) single-precision sum and preventing FMA contraction.
The captured cancellation regression fails before and passes after; an altered
endpoint and nonfinite input remain rejected. Live coverage and performance are
still required; this does not prove every late object or shadow is fixed.

The corrected camera orbit records zero rejected rigid operands, versus nineteen
poses before the arithmetic correction. During VI 3300-3950 all 3,240 bridge
batches match (the preceding run rejected twenty batches around one bad pose).
The runs differ slightly in native update ordinals; this is bounded coverage,
not an identical wall-clock or full-route comparison. The visible exit light
continues to use logical beam ownership. An exploratory report initially
queried the wrong event tag; its beam field was empty and is not evidence.
The corrected report uses the actual beam-owned producer tag.

The complete 17,000-VI Original reference preserves every guest RAM byte, save,
instruction count and reference framebuffer hash. The matrix-only dense replay
also preserves state and matching decisions in that combat fixture. This
establishes numeric/state safety in those cases, not physical-display smoothness
or a solution for every crate, bridge, marker or shadow.

## Combat-camera hold lead, 2026-10-14 UTC

The newer camera-coverage analysis finds no held world-camera vertices in the
two controlled late-room pans, but 120 of 1,220 combat scenes at 120 output FPS
and 74 of 1,185 at 360 have camera holds. Some are consecutive; other actors
remain interpolated. These are matching observations, not uncontended FPS
comparisons. The earlier direct-camera candidate yielded zero extra matches
and remains unselected.

A new diagnostic distinguishes triangles that cross genuinely different
transforms from triangles whose RT64 matrix indices differ while their numeric
matrices are identical. The current eligibility gate rejects either case and
can block an entire shared camera cohort. This is a hypothesis until captured
hold events establish the correlation. The private poll-quantum experiment is
preserved but parked while another game's tests prevent clean FPS measurement;
ordinary code is restored to its qualified 128-instruction quantum.

## Camera conflict observer follow-up, 2026-10-14 UTC

A normal Play capture to VI 6500 recorded 2,236 scenes and one camera hold
at VI 6450. The held camera cohort had zero ineligible vertices and zero
numerically equal cross-matrix triangles; 140 other cross-matrix triangles
were present. This capture does not support relaxing the existing cross-matrix
triangle safety rule. The earlier sustained camera holds remain under review;
native reruns can take different paths, so disappearance on a new run alone is
not proof of a fix. Evidence: combat-focus-20261014/camera-conflict-live-review.json
and camera-hold-room-correlation.json.

The earlier sustained holds are not room transitions: the 120-FPS capture holds
world-camera geometry for 120 scenes at VI 5028-5272 within room 21, while
semantic actors continue matching. A 360-FPS capture similarly holds within
room 21. The one hold in the newer capture occurs around the room change.
Control-event correlation includes both aimed and orbit modes; aim transitions
alone do not explain all holds. A bounded opt-in per-mesh reason observer now
distinguishes absent geometry history, rejected motion, cohort conflict and
ineligible geometry without changing the matching decision.

## Equal matrix slots poison world-camera interpolation, 2026-10-14 UTC

The 360-FPS diagnostic reproduces a sustained world-camera hold starting at
VI 4814. At that frame six cross-slot triangles use numerically identical
matrices; their 18 vertices are marked ineligible, blocking the entire shared
camera cohort even though the other scenery meshes have valid prior anchors.
The per-mesh reasons confirm cohort rejection, including the two affected slots.
This is distinct from the rejected explicit camera-identity prototype.

The candidate treats equal finite matrices as one triangle space. Cross-slot
meshes have incomplete local topology, so they cannot anchor shape history;
they may only follow an existing complete, unambiguous cohort anchor.
Different transforms, nonfinite values, contradictory history, absent anchors
and invalid cadence retain endpoint rejection. Opt-in same-input comparison
runs the old eligibility policy alongside the candidate to measure gains and
any lost/changed anchors. Synthetic policy checks and native qualification follow.

The first policy-test build stopped on warnings-as-errors because new fixture
variables shadowed earlier locals. Renaming those test locals resolves the build;
no game was launched by the dependent qualification steps before that check passed.

The same-input old/new comparison now restores 219,538 vertex samples at 360
and 436,504 at 120 across 3,117 scenes in each run. It loses zero formerly valid
matches and changes zero valid prior anchors. There remain 15/17 camera-hold
frames; restored samples alone do not establish the owner's full-route visual
acceptance. The dense exact-clock replay preserves every guest byte, save and
per-update instruction count. The 17,000-VI Original reference also preserves
all guest/reference-framebuffer hashes and probe counters. Qualification and
remaining hold details are retained in combat-focus-20261014/shared-matrix-*
artifacts; this is a bounded verified correction, not an all-jitter resolution.

Long exact-clock captures after the equal-slot fix still include isolated and
short consecutive scenery-camera holds: 14/6,934 scenes in one 120-FPS route
and 49/5,902 in a different 360-FPS route. The first report used an incorrect
room event tag; its unknown-room values are not evidence of room transitions.
The corrected room report and opt-in per-mesh reasons now distinguish missing
history, motion rejection, ambiguous shapes and blocked cohorts on those exact
recordings. No new camera policy is selected from hold counts alone.

## Missing camera anchors in recorded routes

The new exact-clock observer reproduces scenery holds with reason 136: every
visible batch lacks its former geometry key and the shared camera has no anchor.
This is new causal evidence distinct from the earlier candidate that added no
coverage. With equal-slot safety and guest-ordered matrix validation now fixed,
a candidate binds world-camera endpoints to the captured camera generation and
mode/index. Emitted bytes and immutable view/projection operands validate the
binding; geometry changes alone no longer erase it. Contradictory claims, cuts,
projection changes, invalid coordinates and stale generations remain rejected.
Validation is shared once per camera owner. Same-input old/new counts and exact
game-state replay are required; no full-route visual resolution is claimed.

The first camera-identity takeover is rejected: it restores 4,299/33,019 vertex
samples in the 120/360 recorded routes, but loses 2,806,533/3,800,423 samples
that the geometry matcher already accepts. All guest RAM/save and common
instruction records remain exact, demonstrating why state equality is insufficient
for rendering acceptance. The revised candidate supplies only missing cohort
anchors and preserves independently validated geometry history. Conflicting
additional camera claims cannot create an anchor. Camera validation diagnostics
separate missing endpoints, changing projection and other motion rejection.
The rejected patch/binaries and comparisons remain on D:.

The supplement-only camera candidate passes both long exact-clock routes with
zero lost vertices and zero changed existing anchors. It restores 4,299 samples
in one 120-FPS scene and 33,019 samples in forty 360-FPS scenes. Camera holds
fall from 14 to 13 and 49 to 9 respectively; these recordings follow different
routes, so their counts are not an FPS comparison. All guest RAM/save bytes and
common complete update-work records agree at the same deliberate transcript
limit. Remaining projection/motion rejections require actual operand review.
Evidence: camera-supplement-replay-review.json; candidate runtime 95b73d0a.
No new preview delivery or full-route visual acceptance is claimed.

## Recorded projection and motion rejection

Both full operand captures preserve guest RAM/save and common complete update
work. Every changed projection alters only its two positive lens scales;
maximum relative change is 1.174%. A candidate permits at most 2% common scale
change for explicitly owned cameras, preserving all other projection fields,
view rotation/translation limits, generation, byte binding and cohort rules.
Aspect-ratio/depth/reflection changes and larger zoom jumps still reject.
This narrowly targets five remaining otherwise-safe camera holds in the two
recordings, not every observed jump. Other holds include actual view jumps up
to 148 degrees and about 1,276 world units, or scene-generation transitions.
Their presence does not justify weakening camera-cut protection. Full replay
qualification is required before selecting the zoom candidate.
Evidence: camera-operand-review.json and remaining-camera-motion-review.json.


## Camera supplement and zoom qualification

Runtime `914cc5d6185d6e2be7e3a9b1b970745c5d03b635859418064773eef9d05d2bbe` passes both complete recorded-clock
comparisons at their existing explicit limits, preserving all guest RAM/save
bytes and common complete instruction records. Across the two different routes,
120 restores 9,378 vertex samples in 3 scenes and 360 restores
36,463 in 43 scenes. Both lose zero previous matches and change zero existing
anchors. Remaining camera holds are 11/6; these route-specific counts are not an FPS comparison.
The bounded zoom rule restores VIs 7733/8560 and 6540/10273/13179 respectively.
The 17,000-VI Original reference preserves all guest/reference-framebuffer hashes,
save and probe/instruction results. These qualify the targeted history remedy,
not full-route physical-display acceptance. The native-stability blocker remains
open and the desktop package is unchanged. Evidence: camera-zoom-replay-review.json
and camera-zoom-full-original-review.json; failed takeover evidence is retained.


The bounded, mod-warped late-cave capture visibly contains the exit marker and
surrounding terrain. Its eight distinct presented images cover two consecutive
source scenes at phases 0.25/0.5/0.75/1.0, with 30 SCENES / 120 OUTPUT in the
matching trace. This verifies the target is visible and intermediate images are
produced; eight images do not establish an all-route jitter remedy or physical
monitor pacing. Evidence: camera-zoom-near-beam-phase-review.json and its raw PPMs.


## Current-package target capture and coverage, 2026-10-15 UTC

The post-lifetime-fix source is captured again with the controlled level-26 warp,
near-exit movement and camera orbit fixture. The exit marker and cave terrain
are visibly present in all eight images, covering two source scenes (VIs 3501
and 3505), each with phases 0.25/0.5/0.75/1.0 at 30 source / 120 output. Every
image hash differs. The bridge and wooden crates are not visible in this fixture.
This qualifies visible intermediate images for the exit marker only; readbacks,
mod warp and infinite health prevent ordinary-play performance acceptance.
Neither eight images nor a hidden window establish physical-display pacing.

The extended native owner-input replay remains in level 21 and reaches death/
respawn. It does not cover the full recorded later-cave path; see PERF-001's
transition/health qualification. RENDER-006 stays open for exact owner-visible
bridge/crate/marker jitter and full-route verification. Evidence:
lifetime-late-cave-review.json, lifetime-late-cave-sequence and
transition-call-native-review.json.


A separate current-source far-exit orbit produces eight distinct intermediate
images showing the cave opening and distant wooden span. Actual actor labels
identify all 3,000 observed longwoodbridge batches as matched in VIs 3800-4400.
The outgoing arrow has 600/600 matches. Two isolated holds belong to the incoming
arrow and five to a RegionID object, not the bridge; they are retained for review.
No wooden crates are visible in this capture. This is bounded matching/image
coverage with a mod-warped route and readback, not full-route visual or physical
display acceptance. Evidence: lifetime-visible-bridge-review,
lifetime-visible-bridge-actor-review and the retained image sequence.

## Owner review of optimized preview (2026-10-16 UTC)

The latest owner session confirms runtime 34ce90c9a11bba75a9ae47f3ae3d8a40d635b20357bf8c52391f4bd015a6bff1.
Gameplay felt substantially more stable, but the late bridge still visibly strobes
at a 120 FPS target, described as if it only appears at 30 FPS. This is an
owner-reproduced visual defect; previous numeric tests did not resolve it.

Separately, jumping on the late bridge did not produce the physical response the
owner expected. **Original-game behavior remains unconfirmed**. The owner
explicitly requests an original-ROM EmuHawk comparison before classifying this
as a recomp physics defect. Retained EmuHawk copies are available. Do not alter
physics based only on this expectation or conflate it with the rendering defect.
New capture ID: 20261016-062617-3fd7bf54efdd4eab861c018a60fcc3c5.
Related frequent audio interruptions are tracked in [AUDIO-002](walking-audio-interruptions.md).

The first original-ROM setup reused the earlier lighting fixture save and loaded room 92, not the owner late bridge in room 26. No bridge physics conclusion was drawn. Corrected setup uses a disposable copy of the owner capture initial save and identifies the playerBoy actor explicitly. Original files and failed-target evidence are preserved.

## Owner verifies original bridge physics (2026-10-16 UTC)

The owner independently checked the original game and confirmed: our bridge
physics are correct. The game requires the bridge to stop bouncing completely
before another bounce can start. This resolves the reported expectation mismatch;
no bridge physics repair is required or authorized by that symptom. Preserve the
original cooldown/settling behavior. The separate visible 120 FPS strobing remains
an open rendering issue. Emulator setup captures do not supersede this owner
original-game comparison; no further physics investigation is needed.

## Bending-geometry candidate (2026-10-16 UTC)

The bridge behavior (55 / hitBridgeControl) rewrites model vertex heights.
Rigid transform matching alone does not interpolate that bending. A default-off
candidate gives its vertices explicit actor-lifetime/model/logical-index keys.
Identity and interpolation tests pass; initial visible capture exposes additional
semantic history rejection and is not a completed fix. New optional per-actor
rejection diagnostics distinguish invalid samples, cadence, duplicate conflicts,
missing history and projected-motion cuts. A private landing fixture changes only
the initial test player placement, then runs the original physics; this fixture
is removed from the clean build. Evidence and failures remain under private
route-audio-bridge-20261016. Desktop preview has not changed.

## Owned-camera continuity candidate (2026-10-16 UTC)

The stationary bridge capture reported zero invalid samples, duplicate conflicts,
missing endpoints or cadence breaks in the late interval, but 2,897 projected
motion rejections. A near-plane/edge vertex can move far in screen coordinates
during a continuous camera move, incorrectly holding the whole bridge. The
candidate reuses bounded owned camera/model/zoom validation for bridge vertices,
with a separate 128-unit bound on physical vertex movement; character joint
rules remain strict. Tests cover near-plane camera movement versus actual cuts,
large deformation and character exclusions. The first placement fixture landed
at the bridge anchor/end and produced an invalid visible view; it is not bridge
rendering acceptance. A normal-input walk/jump fixture follows the exit warp.
Private evidence: bridge-audio-factor-review.json under route-audio-bridge-20261016.
No live owner verification or release is claimed.

## Deforming bridge capture and regression checks (2026-10-16 UTC)

The qualified visible walk/jump fixture reaches the bridge with ordinary
movement/jump inputs after test-only exit warps. It records 251,856 bridge
vertex samples, 250,668 matched endpoints and 7,968 samples with actual
world-space deformation. The camera-continuity candidate retains over 99% of
bridge samples; the eight captured images show all four phases at 120 FPS.
The disabled-candidate walk/jump control reaches the same final player position
in room 26. Its source-scene ordinals differ slightly, so do not claim identical
frame-by-frame game state from that control. This is bounded render coverage,
not ordinary-play performance or owner visual acceptance.

The candidate now defaults on in the development build, with the private
JFG_DIAGNOSTIC_BRIDGE_VERTICES=0 comparison override. Actor lifetime/model/range
and camera-cut checks remain in force; behavior 55 alone gets deforming bridge
vertex identities. Semantic, original timing and native event-batch tests pass.
Evidence: bridge-walk-visible, bridge-walk-baseline and bridge-default-build in
private route-audio-bridge-20261016. Desktop preview is not updated yet.


## Qualified bridge geometry remedy (2026-10-16 UTC)

The clean default-build walk/jump capture has 249,876 bridge samples, 248,688
matched endpoints and 7,968 actually deforming samples. The visible bridge is
present through all captured intermediate phases. Actor/model ownership and
camera continuity tests pass. This addresses missing bending interpolation;
the owner-confirmed settling restriction remains original game behavior.
The final native audio tail-overflow correction does not modify renderer code.
[Qualification](../../../evidence/high-refresh-audio-bridge.json) binds captures,
source hashes and tests. Owner visual verification, other late objects and
shadow cadence remain separately open.


## Local preview delivery (2026-10-16 UTC)

Both desktop shortcuts now select source ca6ef7a883b2568da9df2ff0665ec7169c9f4be1 and runtime SHA-256 8dbe521304348ecda93c9f153fbd15f95486b2050b456e37ee221562e30f6a56. Package/launcher setup checks and copied-profile migration pass; all 40 checked live profile files and shortcut arguments are preserved. The previous package and links remain available for rollback. Owner listening and bridge visual review remain pending. [Qualification and delivery](../../../evidence/high-refresh-audio-bridge.json) record the exact scope. This is a local preview, not a public release or completion of the broader no-dip goal.


The High Refresh Preview and Jitter Capture desktop links now select the native-ROM package: source 8277a80611e73c0dc57285e32e0761e9b7519e41, runtime SHA-256 5f902049bd3e4a991a020f17e63a51d0291f28709057a9aa7efc03097692764b. Launcher/setup checks pass; all 40 checked profile files and shortcut arguments are preserved. Earlier audio and bridge remedies are included. Owner listening/visual acceptance, later renderer/load hitches and the broader no-20-FPS goal remain open. [Qualification and delivery](../../../evidence/high-refresh-native-rom.json).


Local delivery: both High Refresh Preview and Jitter Capture desktop links select source 27ddef0a5ed4afd1f1f9c26a5590b590cda5fe8e, runtime SHA-256 db65b460f399b550ca8a3ad0d98f5374d5e00a95dc4a7073edfa2649fa6772c6. Manifest and launcher/profile checks pass; all 40 checked profile files and shortcut arguments are preserved. Includes native audio, bending bridge interpolation and ROM loading improvements. Owner live acceptance and broader restart/load gaps remain open. [Current qualification](../../../evidence/high-refresh-draw-key.json).
