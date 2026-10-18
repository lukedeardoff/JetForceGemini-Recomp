# RENDER-003: Jitter with steady 120 OUTPUT / 30 SCENES

Status: capped presentation-window correction verified in bounded tests and delivered locally; owner visual verification pending. Source-workload and Unlimited limits remain open.
Severity: high; smooth high-refresh rendering remains incomplete.

## Report and reproduction

The owner reports visible jitter even in a quiet scene while the counter holds
120 OUTPUT / 30 SCENES with no visible dips. It occurs both without input and
during motion, but is most obvious while moving Juno and turning the camera
together. The reported profile requests Custom 120 with VSync on. The affected desktop
preview was `high-refresh-native-timing-preview-20261012`, implementation
`5c7df1bdee61778590fb3b7baa281958a9167870`, runtime SHA-256
`a14d9d77fc98ef6be1999a3ea9492d60467a4b107e376c66d431fbfd70231259`.
Confirm the running executable separately; the owner report is not an automated
reproduction. The owner explicitly requests interpolation around stable actor,
skeleton/joint and camera identities, beyond matching graphics batches.

## Existing evidence and hypotheses

- [PERF-002](frame-diagnostics.md) corrects the reproduced capped-play bridge's
  source-clock fallback. That does not establish visually continuous motion.
- OUTPUT counts successful present returns, including repetitions; SCENES counts
  new submissions. Neither establishes unique images, physical scanout or smooth
  camera/animation progression. A stable average can conceal uneven intervals.
- [Presentation history](../../../include/jfg/renderer/interpolation_history.hpp)
  matches exact geometry and blends combined transforms. Unmatched/ambiguous
  geometry falls back to endpoints; animated geometry need not retain its key.
- Incomplete animation coverage, camera/player phase mismatch, coefficient
  blending and presentation/display pacing are competing hypotheses, not a
  diagnosed cause. The 30-Hz source rate alone does not make jitter inevitable.
- Combined movement and camera rotation need the same presentation timestamp.
  Stable semantic identities must include lifetime/reset handling so reused
  actors, camera cuts, teleports and mode transitions do not blend unrelated data.

## Planned validation and acceptance

Compare stationary, movement-only, camera-only and simultaneous-motion fixtures
using copied profiles/saves and muted, owned hidden windows. Measure per-frame
spacing and actual intermediate camera/body motion, not counters alone. Keep
heavy GPU screenshot captures separate from pacing benchmarks. Preserve the
original-clock reference and guest update/movement rules. Test actor reuse and
animation boundaries before claiming complete coverage. Native monitor/focus
behavior and owner visual confirmation remain necessary where hidden tests cannot
establish them. Do not infer that this report shares the heavier combat cause.

## Modern-engine references

Fixed simulation with interpolated rendering is a standard design:
[Godot's interpolation overview](https://docs.godotengine.org/en/stable/tutorials/physics/interpolation/physics_interpolation_introduction.html).
Camera handling can combine an interpolated follow target with per-render mouse
rotation; camera and target sampling must agree:
[Godot camera guidance](https://docs.godotengine.org/en/stable/tutorials/physics/interpolation/advanced_physics_interpolation.html).
These are design references, not evidence of implementation parity in JFG.

## Resolution and compatibility

The changing-geometry history gap is established and addressed by semantic
vertex ownership. It is not established as the sole cause of all reported jitter.
The candidate preserves gameplay state and the terrain correction. See the
qualification below and its later delivery record for the tested artifact. The
same guarded investigation retains all charges; unrelated PC/audio work is
unchanged. Never feed interpolated visual positions back into simulation.

## Diagnostic interruption and storage recovery

The identity diagnostic was interrupted by a full C: drive before runtime source edits or a game run. The guard retained the failed charge and stopped at 18 attempts. The owner explicitly approved one additional guarded retry within the existing five-hour deadline, preserving all counters and the investigation root.

Large private captures were moved to `D:/JFG-private/high-refresh-20261011` with SHA-256 verification of every file before removing the original copies; directory junctions preserve existing evidence references. New large outputs use D:. The production ledger and source checkout remain in place. The private relocation receipt records 16 directories, 6612 files and 8036767841 bytes.

Informative setup failures: the inventory initially used Path.is_junction, unavailable in the installed Python; Windows reparse attributes replaced it. The first cleanup script encountered PowerShell array wrapping before deleting anything; direct JSON array assignment corrected it. An automatic command review had rejected the combined relocation command without a specific reason; copying/hash verification and path-checked cleanup were subsequently separated. These failures establish no rendering result.

## Semantic interpolation candidate

The bounded identity trace ties animated matrix DMA addresses to two alternating
palettes owned by a model instance, with a stable bone index. Exact-shape matching
had left changing body geometry without history. The candidate carries actor
lifetimes and camera ownership with each immutable graphics task, and matches
logical model vertices by actor lifetime, instance, model, camera and joint.
Free requests, missing actors, level/model/camera changes and discontinuities
invalidate history. UV clones retain their original logical vertex identity.

Character vertex endpoints interpolate in world space before applying the same
camera phase. This avoids separately blending deforming local positions and
joint transforms, which can pull shared edges apart. It is presentation-only:
guest simulation and input timing are unchanged. It is not a reconstructed local
skeletal hierarchy with quaternion blending or per-render mouse look. Unsupported
draw paths retain the previous conservative matcher.

Six focused identity/interpolation/clock tests pass. The first muted moving-route
run matched 517,189 of 524,527 tagged vertices (98.6%); that is coverage evidence,
not visual acceptance. The same run averaged 119.22 output FPS and 27.29 scenes/s
in VI 4000-4400, including a 33.104-ms output interval. Pacing therefore remains a
qualification concern, and the installed preview has not been replaced. Paired
runs of the accepted runtime and candidate, without interpolation trace output,
are retained in private artifact `semantic-interpolation-v1/pacing-comparison.json`.

Implementation setup failures: a patch selector initially matched both a real
task and a skipped-task placeholder; it was narrowed before building. The first
build used RT64 interop matrix member syntax on an hlslpp matrix; row indexing
and explicit construction corrected it. Subsequent review guarded live-only
identity state for non-live builds, removed a per-UV-clone context mutex lookup,
cached joint inverse validation per DMA and inverses per transform, and skipped
semantic vertex collection for offscreen masks. The refined binary passed the route qualification below. Final review bounded
byte-swapped reads for truncated RAM spans, rejected signed-negative bone counts,
and kept unknown-camera paths on the existing conservative matcher.


## Bounded qualification (2026-10-13 UTC)

Seven focused tests pass, including retirement/address reuse, missing actors,
level/camera changes, stale buffers, ambiguous UV duplicates, actor-wide fallback,
and coincident shared-edge vertices under different joint rotations and camera
motion at five interpolation weights. F3DDKR texture tests cover the modified UV
clone path. These checks preserve endpoints; they do not establish raster coverage
of every reported thigh pose.

Without detailed interpolation tracing, the initial paired route averaged
119.85 OUTPUT / 29.78 SCENES for the candidate versus 119.90 / 29.79 for the
installed build. Refined idle, movement-only and camera-only cases held roughly
120 output FPS and 29.6-29.8 scenes/s. Combined movement/camera input was verified
in guest position/yaw samples, not merely assumed from the replay file.

The combined route exposed the same 31-update three-VI recovery episode in both
old and new builds: about 119.5-119.6 output FPS / 27.5 scenes/s over VI 4000-4400.
This remains an open source-cadence limitation. Custom 360 averaged 350.1 output
FPS / 27.4 scenes/s and included a 41.5-ms output interval. Neither observation
supports a claim of completely stable 30-Hz scenes or steady 360-FPS output.
All six normal-play cases recorded zero audio underruns and dropped log rows,
and preserved the copied save. The hidden-window tests measure successful present
returns; they do not qualify physical VSync/monitor delivery.

The original-clock reference retained exact guest/RT64 memory and copied-save
hashes. Paired original-clock visual runs likewise retained identical final
guest and RT64 memory. The first selected screenshot window was after movement
had stopped and was inconclusive for animation. A second bounded window targets
active simultaneous movement. Swapchain readback captures deliberately stall
rendering and are excluded from performance claims. Capture counts and hashes,
tested runtime identities and route results are in
[qualification](../../../evidence/semantic-interpolation.json).

In the active window, the baseline repeated one complete image across six
subframes, while the candidate produced six different images and changed dark
blue character pixels in each adjacent pair. The capture start can land on a
different presented group, so this is sampled motion evidence, not an exact
paired per-frame trajectory or proof that every output image is unique. Visual
inspection shows the walking pose progressing; the automated body mask excludes
the surrounding bright water but is not a complete character segmentation.

The remedy is host presentation metadata and world-space vertex interpolation;
it does not increase simulation/input frequency, reconstruct a local skeleton
hierarchy, interpolate lighting attributes, or implement per-render mouse look.
Unsupported paths keep conservative transform matching. Teleports and large
projected discontinuities hold an endpoint; every small cinematic cut and model
variant is not yet covered. Reported Original-mode thigh gaps, delayed lighting,
combat slowdown, Unlimited saturation and long-session/audio behavior remain
separate open work. Owner live confirmation is required before resolving this
jitter report.

Transferable lesson: constant output counters do not establish unique, coherent
animation samples. Carry stable ownership with the immutable render task, make
all parts of an actor share the same fallback decision, and measure screenshot
motion separately from timing to avoid diagnosing readback overhead as normal
play stutter. Large evidence remains on D: with verified stable references.


## Local preview delivery (2026-10-13 UTC)

Implementation commit `01a35a13539a89e8842b8491eb37565d42b03de8` is packaged as
`high-refresh-semantic-20261013` on D:. The existing **JFG - High Refresh Preview**
desktop shortcut now selects that package. Close an older launcher and reopen
the shortcut to test walking while turning the camera. Your existing profile,
save and settings were preserved; all 14 package manifest entries and launcher
initialization passed. F8 and **JFG - Performance Logs** remain available.
No public release or push was made.

The exact delivered runtime SHA-256 is `71f59d66fc20e62f85cc3c2c280e72a85b02aee61e0de5a87adf45fe80f906b4`.
Final combined-motion qualification averaged 119.87 output FPS /
27.49 scenes/s at Custom 120 with requested VSync, and
345.38 / 27.44 at Custom 360 with VSync off.
Output p99/max intervals were 8.637/10.200 ms and
4.228/42.520 ms respectively.
Both normal-play runs recorded zero audio underruns/dropped log rows; the exact
original-clock memory/save reference still matches. The existing three-VI
recovery episode remains visible; these are not steady-30/steady-360 claims.
Detailed results are in the qualification JSON. Owner visual review, full-game
coverage, physical monitor pacing and the separate seams/lighting issues remain
open. The previous package is retained for comparison.


## Owner-approved continuation (2026-10-13 UTC)

The owner approved five further hours to continue the open pacing, thigh-gap and
delayed-lighting issues after local semantic-interpolation delivery. The same
production guard remains enabled, retaining 19 attempts, 18 activities and
33222.14147090912 seconds before the new lease. One additional guarded session
is authorized until 05:29:07 UTC; no counters or roots are reset. New large
artifacts use private storage on D:. Tests remain muted with owned hidden windows
and copied profiles/saves. The installed semantic preview remains the comparison
baseline until a new candidate is independently qualified.

First step: correlate retained present intervals, guest update work and graphics
submission waits around the combined-motion three-VI recovery episode. Reuse
existing seam/lighting captures to define an exact pixel or spatial boundary;
avoid repeating broad captures without a new discriminating measurement.


The first new pacing fixture exited at startup: it requested CPU-cost diagnostics
with --play, which the runtime deliberately rejects. The wrapper then reported
a dead-window ownership assertion while the process was closing. No gameplay or
pacing evidence came from that run. Corrected tests keep CPU-cost sampling on
the supported finite native-clock probe; ordinary play uses the existing light
trace/counter only. The failed fixture and its environment are retained.

Owner follow-up after reopening the semantic preview: "No seems at 120". Record
this as live confirmation that the reported thigh lines are absent at 120 FPS
in that test, not acceptance of every pose, frame-rate mode or lighting state.


### Owner follow-up: stationary camera spin (2026-10-13 UTC)

The owner reports only movement/camera jitter remains in the delivered semantic
preview at 120 FPS. The thigh lines and delayed lighting are absent in that
live check; this is scoped owner confirmation, not a claim of all-scene parity.
A more precise jitter reproduction is to stand still and rotate the camera in a
full circle. The view jumps and looks blurred despite a steady 120 OUTPUT /
30 SCENES counter. Present frequency alone is therefore insufficient acceptance:
measure the camera/geometry displacement of consecutive presented images.

Current hypotheses include camera/terrain history falling back as visibility
changes, skipped or repeated interpolation samples, and camera sampling phase.
Separate the broad 20-Hz recovery event from this stable-counter reproduction.


### Capped sequence truncation candidate

The stationary-spin trace confirms nonuniform displayed interpolation progress
even though the output and source counters remain approximately 120/30. The
finite-rate workload loop treats the next queued shadow-only task as a new scene
and abandons remaining intermediate frames. This priority test already filters
shadow tasks in Unlimited mode; capped modes had retained the broader test.

Candidate: apply the same scene-aware queue check at capped rates. A real next
scene or unknown workload still preempts; the existing time budget still sheds
work on overloaded machines. Record actual displayed scene/alpha identities
through the optional trace, and separately measure ordinary uncaptured play.
This is a candidate until paired spin and combined-motion tests pass.


The first instrumented spin held 119.74 output / 29.78 scenes per second, but
162 of 743 displayed samples repeated their predecessor's interpolation phase.
All 185 measured camera cohorts matched: scenery history loss did not explain
that interval. The optional output trace captures source tick and alpha alongside
the actual selected target before its GPU read, rather than inferring motion
from the render queue or FPS counter.

The scene-aware queue check alone still leaves duplicates. A second cause is the
completed-sequence VI path: it schedules another entire source-frame window of
identical endpoint images. That can hold an endpoint for 33 ms and defer the next
scene, while the output counter continues to report 120. Candidate correction:
acknowledge an already-consumed finite scene immediately and let the next source
scene own its output window. Original mode, paused debugger work and Unlimited's
stream handling retain their own paths. Qualification is still pending.


The second candidate produces 740 strictly advancing samples (every step 0.5 VI)
across 185 scenes in the original camera-pan fixture, with zero repeats or
backward samples. The wider nominal-circle fixture traversed only 314.3 degrees:
its real applied look samples, rather than intended input rows, exposed incomplete
coverage. The first qualification script correctly stopped before broader tests.
Increase the turn rate and verify actual accumulated yaw exceeds 360 degrees.
The wider trace crossed a normal 30-to-20 Hz recovery interval; phase analysis
must use each scene's actual source interval, not assume two VIs for every scene.
Earlier repeat counts are unaffected, but the reported step-size anomalies at
20 Hz were an analysis error and are superseded by interval-aware analysis.
The retained pre-fix wide sweep also had eight audio underruns during its warmup;
it is not clean audio acceptance. Muted normal-play qualification remains pending.

Optional screenshots can now select a bounded source-scene interval, with a hard
64-image limit. Scene selection requires the optional output trace, which supplies
the image's source identity. Readback timings are never used as a benchmark.


### Capture regression caught before delivery

Eight ordinary-play mode/pause fixtures completed, but the final finite reference
probe failed closed with renderer `present-10` (frame capture failed). The new
duplicate-VI skip had also skipped a specifically armed diagnostic screenshot.
Its renderer RAM still matched the reference, but the early trap invalidated
the final guest-state check. No parity pass is claimed for that failed run.
Dependent qualification/commit preparation stopped without edits.

Correction: the single active capture owner exposes an atomic pending flag,
set when armed and cleared on completion, exception or deactivation. A requested
capture presents one latest image even for a consumed scene; ordinary duplicate
VIs still yield immediately. Explicit CPU framebuffer operations also preserve
their existing presentation path. Re-run the exact reference and camera-phase
checks before local delivery. Preserve the failed probe and all step receipts.


## Confirmed cause and qualified correction (2026-10-13 UTC)

Two queue policies prevented uniform presentation progress. Shadow-only jobs
could truncate a capped scene before all intermediate images were generated.
Separately, an extra VI referring to a consumed scene scheduled a full second
window of its final image. The next scene then arrived visually late despite a
steady present counter. Both policies are corrected in
[the RT64 queue adapter](../../../cmake/rt64_video_settings.cmake).

A muted stationary pan changes from 162 repeated phases in 743 displayed
samples to 740 samples with 739 equal 0.5-VI steps and no repeats or rewinds.
The corrected full-circle fixture verifies 419.0625 degrees of actual applied
camera yaw and 1,702 displayed samples with 1,701 equal steps, including the
31-update 20-Hz recovery interval. All 410 measured scenery camera cohorts
remain matched. Separate screenshots of actual swapchain images demonstrate
intermediate scenery motion; they are not a timing benchmark.

Normal hidden play results over VI 4000-4400 follow. These measure successful
present returns, not physical scanout. The pause case intentionally includes
a 1.25-second pause and must not be compared as ordinary pacing. The reference
case uses the retained diagnostic clock and is for exact state comparison.

| Fixture | Output FPS | Scenes/s | p99 ms | Maximum ms | Audio underruns |
|---|---:|---:|---:|---:|---:|
| normal-circle-120 | 119.68 | 29.77 | 9.000 | 10.298 | 0 |
| normal-combined-360 | 335.83 | 27.44 | 9.496 | 34.097 | 0 |
| normal-circle-144 | 143.85 | 29.82 | 7.278 | 8.066 | 1 |
| normal-circle-60 | 59.82 | 29.84 | 17.583 | 18.187 | 0 |
| normal-circle-480 | 458.79 | 29.78 | 3.851 | 16.875 | 0 |
| normal-circle-unlimited | 1274.77 | 19.13 | 3.071 | 5.427 | 413 |
| normal-original | 27.89 | 27.59 | 51.511 | 52.271 | 0 |
| pause-120 | 100.73 | 25.09 | 9.215 | 1260.765 | 0 |
| reference-state | 119.65 | 24.56 | 8.974 | 15.801 | 0 |

The finite reference run retains the exact guest RAM, renderer RAM and copied
save hashes. Focused timing, interpolation, texture and video-setting tests
pass. Full measurements, source/runtime identities, artifact hashes and limits
are in [qualification](../../../evidence/presentation-pacing.json).

Transferable lesson: qualify monotonic source-time progress and actual images,
not just output/submission rates. An already-consumed VI does not authorize a
second animation interval. Small offscreen tasks must preserve the owning
scene's interpolation window; new scenes and actual overload still take priority.

This verifies the reproduced duplicate-window defect. Native workload spikes,
physical display smoothness, Unlimited saturation, the heavier combat report
and complete scene/animation coverage remain separate limits.


The source-controlled [trace analyzer](../../../scripts/analyze_presentation_trace.py)
reproduces these phase counts. Four regression tests cover mixed 30/20-Hz source
intervals, steady-timestamp freezes/jumps, backward samples and invalid input.
For example, after capturing the optional TSV, run:

```text
python scripts/analyze_presentation_trace.py interpolation.tsv --from-vi 3900 --to-vi 4750 --expected-fps 120
```

Use the effective output cap after VSync; omit the expected rate for Unlimited.
The display row records source VI, source rate, requested target, render index,
sequence length, alpha, present ID and target slot after its monotonic timestamp.
A uniform phase does not establish physical scanout or unique pixels: retain
camera-coverage and actual-image checks alongside it.


The capture-compatible build now passes the specifically armed final screenshot
and exact guest/renderer/save hashes. A fresh full-circle trace still advances
strictly at the expected phase with no repeats or rewinds. Normal combined
360-cap play was repeated separately. The final compatibility proof and runtime
identity are in qualification; the earlier failed capture remains preserved.
One 144-cap fixture reported one audio underrun, and Unlimited reported 413 with
about 19 scenes/s under saturation. Those are limitations, not audio acceptance;
the 120-cap and capped combined-motion audio results are recorded separately.


## Local delivery

Implementation commit `b9c1aeee7d259f358a5a76ac22b9ba6c9b7644d6` is committed on the
research branch. The desktop **JFG - High Refresh Preview** shortcut now targets
`high-refresh-pacing-20261013` on D:. Close the game and launcher before reopening.
The delivered runtime SHA-256 is `8bd6b7635ec71a70c735055af97df5119df9f8a0b00d942825b226259ffe9cf7`.
All 14 package entries verify, an isolated launcher-init fixture resolves that
exact runtime, and the owner's profile/settings/save hashes are unchanged. The
older semantic package is retained. This is a local preview, not a release.

The exact delivered binary passed nine focused C++ checks, four analyzer checks,
the corrected full-circle trace (1,700 samples / 1,699 uniform steps), fresh
combined 360-cap play, and the armed screenshot / exact reference-state test.
The latest combined-motion run averaged 351.62 output FPS over its active
window, with the existing 31-update source-cadence recovery still present.
No audio underruns were recorded in those final circle/combined replays.
The rebuilt committed executable is byte-identical to that qualified binary.

The owner has been asked to retest a stationary full-circle camera rotation at
Custom 120 / VSync on. Monitor scanout and owner visual acceptance are pending;
this record does not mark all high-refresh limitations resolved.

## Completed CPU-write windows (2026-10-13)

The expanded owner route exposes another completed-sequence case: framebuffer
writes bypass the earlier empty-operation skip and repeat one endpoint for a
full finite interval. The correction displays that completed endpoint once,
without discarding CPU writes or changing game timing. Bounded repeated phases
fall from 995 to 35 with no backward phases. Pause and final captures pass.
The base/interpolated CPU-write target mismatch remains a separate limitation.
See [the route review](tutorial-late-object-jitter.md#review-qualification-2026-10-13)
and [qualification](../../../evidence/high-refresh-route-review.json).
This does not establish a fix for the owner's exact late light/bridge jitter.

## Variable-cadence continuation, 2026-10-13

The scene-history policy now retains safe interpolation across actual bounded
one-to-six-VI intervals, including delayed-scene recovery. It preserves lifetime,
geometry, camera-cut and invalid-data rejection. Exact-clock tests remove 12
unnecessary holds without backward phases or guest-state differences. Completed
CPU writes also reach the retained displayed endpoint. These measured corrections
do not establish that all movement/camera or late-room visual reports are fixed.
See [route qualification](tutorial-late-object-jitter.md#continuation-qualification-2026-10-13)
and [evidence](../../../evidence/high-refresh-route-fix.json).


## Finite clock and queued scene investigation, 2026-10-15 UTC

Normal Play uses 46,875,000 Count ticks per second and a 783,520-tick VI
period in the qualified NTSC mode: about 59.82617 VIs/s. Renderer source labels
remain nominal 30/20. Counting exactly four images per nominal 30-Hz scene at
120 FPS therefore underfills the output clock. In a stable 1,800-VI window,
current 120/360 captures repeat 8/9 endpoints respectively; all observed repeats
coincide with completed CPU framebuffer-write presents. This correlation alone
does not establish the cause of every owner-visible jitter event.

A private rational-resampling experiment uses the reduced 78,352/78,125 duration
ratio without changing Count, guest scheduling or simulation. Repeated endpoints
fall to 2 in both runs, but whole rendered scenes are then skipped: at 120 FPS,
VI 4890 is fully rendered after a five-image sequence and never displayed. The
present queue drops the current event whenever another event is pending, then
marks its interpolation sequence consumed. The equivalent 360-FPS case follows
a thirteen-image sequence. The initial ratio-only candidate is not selected.

A second private experiment retains an unconsumed finite scene when exactly one
later present is queued; deeper backlog, pause, empty swap chains and Unlimited
keep the existing skip behavior. Its captured results are retained separately.
Neither private experiment is in the desktop package at this checkpoint. These
are hidden-window phase observations, not physical-display acceptance. Original
reference timing, framebuffer writes and original guest update speed remain
required compatibility checks before selection. Evidence: finite-phase-repeat-
context, finite-clock-ratio-native-review, finite-clock-gap-context and
clock-queue-retain-review. Large/private captures remain on the evidence volume.

Transferable lesson: nominal VI/source labels are not elapsed-time rates; a
fractional output schedule must also be qualified against queue admission and
scene consumption, rather than evaluated from FPS averages alone.


The presentation analyzer now accepts an explicit --vi-hz value for elapsed
phase-spacing checks, retains nominal source labels for VI-span reconstruction,
and reports up to 32 anomaly contexts with source VI, present ID, interpolation
weight and sequence length. Selected-window source rates are separate from the
whole-file inventory. Relative phase-versus-wall-time error exposes accumulated
lag that an average FPS counter can hide. Eight focused tests cover native and
nominal clocks, cadence changes, freeze/jump context, bounded output, corrupt
records and drained/truncated logs. This is analysis tooling, not a renderer fix.
For qualified native NTSC Play use --vi-hz 59.82616908311211; leave the default
for nominal/reference captures. A log alone does not prove monitor scanout.


The broad one-pending-event candidate fails 360-FPS qualification: 53 audio
underruns in its first run and 169 in a repeat. The subsequent unchanged baseline
has zero underruns, despite a higher sampled whole-host CPU average (33.4% vs
23.0%). Native paths can diverge, so this is an acceptance failure rather than an
exact-state causal benchmark. No broad queue change is selected or installed.
A production draft was prepared but never admitted/executed. The prior generated
sources and normal runtime are restored; failed evidence is retained.

A narrower private variant additionally requires both queued presents to name
the same graphics workload. Its 360-FPS run reports 0
audio underruns, 0 unsupported accesses and
6006 VIs. Results and scene/guest wait distributions are in
queue-outcomes-review. This observation alone does not qualify installation.
The first nested request for this variant was withdrawn before execution to
correct string escaping; its immutable draft is retained. No counters reset.


The context limit retains the 32 largest phase errors, listed in capture order,
so a small persistent nominal/elapsed clock mismatch cannot crowd out a later
whole-scene jump. A ninth regression and the real-capture CLI verify that case.
The CLI still rejects incomplete traces with --require-complete. This changes
only offline analysis; the logging runtime and desktop package are unchanged.


The same-workload-ID restriction fires zero times in its capture: offscreen
workloads make raw graphics IDs insufficient for logical-scene equivalence.
A separate private observer retains the ratio-only behavior and records current/
next workload IDs, pending-event count, active interpolation VI and consumed
count at each skip. In VIs 4000-5800 it classifies fresh finite skips as
{'different_workload_same_scene': 3} and finds 2 rendered scene VIs with no display sample.
The explicit per-scene contexts are preserved in queue-scene-ownership-review.
This remains a bounded hidden trace; no scene queue correction is selected.
The observer is removed and the normal generated sources/runtime restored.


## Scene-owned queue candidate checkpoint, 2026-10-15 UTC

A private bounded registry assigns one logical scene token to the scene workload
and its following offscreen shadow workloads. Unknown metadata breaks the token
chain; reused slots require exact workload IDs. Retention requires exactly one
later event, matching nonzero scene ownership, fresh finite output, an active
swap chain and unpaused presentation. Deeper backlog and Unlimited retain their
existing policy. The private candidate also includes the elapsed-clock ratio.

Current status: **bounded candidate; full qualification pending**. Its 120/360 observations respectively report
[0, 0] audio underruns,
[0, 0] repeated phases and
[0, 0] rendered scene VIs
without a display sample in the selected window. Those values qualify only the
recorded hidden runs. See [the checkpoint](../../../evidence/high-refresh-finite-pacing.json)
for exact hashes, intervals, failures and remaining acceptance work.

No clock/queue candidate is installed. The private registry, generated renderer
patches and observer are removed after testing; the verified history-storage
preview remains on the desktop. Before selecting a production version, scope the
ratio to the native clock, retain Original reference timing, prove guest-state
and matching equality, test registry lifecycle/reuse and pause, and check further
caps and the actual owner route. This checkpoint does not resolve RENDER-003 or
PERF-001. The offline analyzer change is the only selected source implementation
from this final pacing continuation.


The final private prototype completes these exact-state stages:
['Original RAM/save/renderer/probe/instruction equality', 'Recorded-clock dense RAM/save/instruction/matching equality'] (step exit code 0). The saved reports
compare physical RAM, save, renderer/probe state where captured, complete guest
instruction/update records and dense matching decisions against their recorded
baselines. Separate C++17 registry tests pass scene/offscreen ownership, unknown
metadata, slot eviction, long offscreen chains and workload-ID restart. Those
checks do not establish production readiness or physical-monitor smoothness.
The normal runtime is restored after the binary comparisons. Production clock
scoping, integration, pause/other caps and full-route/live checks remain pending;
this prototype is still not in the desktop package.


The Original instruction-clock/reference run reports 2 audio underruns
with final readback. Its exact-state pass is not native-play FPS/audio acceptance;
the separate 120/360 native prototype runs each report zero audio underruns.
The owner route, damage/combat consistency, shadows and Unlimited remain open.


The dense recorded-clock equality fixture also has 98 displayed phase steps
larger than one VI in its analyzed interval. Its pass qualifies guest state and
matching only; it is not native pacing acceptance. These gaps and their complete
trace remain preserved, alongside the separate normal-Play 120/360 results.
No all-captures-smooth claim follows from the private prototype checks.

### 2026-10-15 production integration candidate

The owner approved a six-hour continuation with all production-guard charges
preserved. The previously private finite-clock/scene-ownership prototype is being
integrated into the pinned build-tree patches. The production candidate scopes
ownership to each RT64 State/shared queue lifetime, clears it on teardown/reuse,
and fails closed for unknown or evicted workloads. Only qualified native NTSC
pacing uses the 78352/78125 duration ratio; instruction-clock and unknown startup
modes retain 1/1. A clock-mode change resets the finite tick accumulator.

Build, lifecycle/concurrency tests, exact-state comparisons and native presentation
runs are pending for this integration. The installed preview remains unchanged.
This does not resolve native combat-update drops, the dense recorded-clock pacing
gaps, shadow-mask cadence or the unverified late tutorial route.

The first integrated 120-FPS native capture has no repeated/backward phases,
missing rendered scenes or audio underruns in the reviewed window. The following
360-FPS run fails acceptance: 49 audio underruns and seven rendered-but-undisplayed
scene VIs, despite no repeated/backward phase values. Its combat windows vary
around 24.7-28.6 scenes/s. Native input paths differ, so this is a candidate
qualification failure, not yet a causal attribution. Preset/pause request 354 was
withdrawn before execution. Delivery is withheld while a shared recorded-prefix
comparison distinguishes candidate behavior from starting-state/host-load changes.
Evidence: production-scene-owned-native-review.

The shared-prefix candidate run completed, but its report formatter expected a Play-only audio field in a probe result and raised KeyError. Raw results were already saved. The correction reuses that run and collects only the missing baseline; no successful game test is discarded or rerun. Request 356 stopped at its prerequisite check without analysis.


### Queue pressure in the integrated candidate

All focused tests pass. The integrated candidate preserves the dense reference's
physical RAM/save, 1,847 complete update records and 17,715 matching decisions.
The 17,000-VI Original comparison preserves RAM, save, renderer snapshot, probe
counters and 4,754 complete guest-update records. These are state comparisons;
neither substitutes for native presentation or audio acceptance.

A 360-FPS shared-prefix comparison verifies identical release state. The candidate
records zero audio underrun events, three repeated phases and one missing rendered
scene; the installed build records zero underrun events, five repeated phases
and two missing scenes. Native suffixes differ, so these are observations, not
strict per-frame causal measurements. Candidate scene queue waits total 279.1 ms
over the selected window versus 11.5 ms in the installed build, with p99 2.745
versus 0.218 ms. Ordinary shadow passes have negligible queue waits. The separate
failed normal-native 360 capture has substantially longer waits and 49 underruns.
The source candidate remains unselected and the desktop package is unchanged.

A second private candidate coalesces an older empty presentation event into its
newer same-scene event while preserving the unconsumed interpolation sequence.
Events containing CPU framebuffer writes retain their existing delivery. Both
shared-prefix and normal-native 360 captures have no audio underrun events and
no repeated/backward phases, but still omit three and two rendered scene VIs.
The normal capture maintains two-VI game updates throughout VIs 4000-5800; it is
not proof of combat performance on every route. The prototype is not selected;
all generated source is restored afterward. Further work must explain the
remaining scene skips and bound queue latency without dropping CPU writes,
weakening synchronization or changing guest simulation.

Private evidence: production-scene-owned-native-review, production-prefix360-review,
production-pacing-cost-review, production-queue-owner-cost-review and
coalesced-scene-present-review. Original instruction-clock fixture coverage
remains in level 21; the earlier recorded native clock reaches levels 16 and 26,
which may provide a bounded starting prefix for future late-route qualification.


The empty-event observer identifies a further failure: a forwarded fresh sequence
can be skipped by its receiving notification when newer work is queued. Seven
other observed fresh skips have two pending notifications despite the immediate
next event sharing the same scene. An explicit one-event handoff prototype
preserves that receiver's obligation to show the untouched finite sequence.
Its normal 360-FPS capture has no missing/repeated/backward scenes in the selected
window, but has 32 audio underruns and source-cadence drops. It is rejected for
delivery pending queue-latency correction; zero missing scenes alone is not
acceptance. The detailed observer itself had ten underruns and is not a normal
performance baseline. All generated observers/prototypes are restored.

A follow-up instrument measures PresentQueue submission-slot waits independently
from WorkloadQueue waits. See sequence-handoff-queue-costs for the measurements.
The next candidate must reduce producer blocking without discarding framebuffer
writes, invalidating active Present storage, or increasing latency unchecked.
No public or desktop delivery has occurred.


### Bounded native presentation window candidate (2026-10-15)

The private active-Present copy frees the notification slot early but moves
backpressure into WorkloadQueue: about 3.2 seconds of aggregate wait in the
reviewed window. Notification-ready to first output increases from 42.3 to
48.9 ms median in separate native suffixes; this is not a deterministic latency
A/B. The normal run has 18 audio underruns and four missing scenes. Rejected;
all generated sources are restored. Evidence: owned-present-storage-review
and owned-present-storage-cost-review on D:.

A separate bounded-window prototype combines explicit adjacent same-scene
handoff with a maximum two-retrace finite presentation window. The retail
scheduler's two-retrace completion threshold is verified in pinned assembly
at 0x800506E4 and __scTaskComplete; no simulation or guest clock is changed.
Slow intervals still supply their actual interpolation span and source-rate
label. The prototype starts later in a long interval instead of extending
the renderer backlog to replay the entire interval. Reference clocks and
Unlimited are unchanged. Nonempty CPU-write events retain only with one
pending notification; deeper backlogs preserve the original skip policy.

Both shared-prefix and normal 360/no-VSync captures have zero missing
presentation scenes, backward phases, dropped trace chunks and audio-underrun
events (the normal Play device counters also report zero underruns/overruns).
The shared-prefix scene windows are 29.4-29.7/s, with 13 repeated phases;
normal Play has 54 repeated phases and still falls below 30 scenes/s. This
is a private candidate, not stable-30 acceptance or a delivered fix.
Slow-source transitions reset the current rational remainder, requiring a
further continuous-clock check. A wrapper assembly attempt (step 369) failed
before any source mutation/build; the readable replacement (370) completed
and restored/rebuilt both generated sources. Evidence: bounded-scene-window-
review and bounded-scene-window-cost-review on D:.

Compatibility lesson: releasing a queue slot can merely move producer stalls;
measure game-update cadence and both queue waits, not just output FPS or one
queue. Bounding presentation delay trades interpolation coverage during a
real slow update for less backlog and requires visible cadence qualification.


The unselected native scene-retention/clock-scale source is preserved in research
commit 53527b626bbd14c10ff2ed151454a8bb585aae39 (branch research/native-scene-retention-candidate-20261015).
It is removed from the active high-refresh working source before targeted CPU
optimization because its earlier native 360 FPS run had audio underruns. The
private bounded/common-clock alternatives remain retained in prior evidence;
neither is silently treated as a delivered fix. The active baseline is rebuilt,
and the installed desktop preview remains unchanged.
