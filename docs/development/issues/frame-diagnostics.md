# PERF-002: Frame diagnostics and reported stutter

Status: native scene timing and present logging verified in bounded capped-play tests; local preview delivered, owner verification pending. Unlimited throughput and broader audio remain open.
Severity: high owner-reported stutter; presentation defects reproduced and corrected in the candidate.
Owner reports slowdowns while testing the high-refresh preview but has no visible
counter or convenient frame log. Slowdown cause and location remain unconfirmed.
The separate combat slowdown record is PERF-001; do not assume the same cause.

Candidate: F8 toggles a swapchain-only counter and asynchronous per-render CSV.
Count render intervals and screen-scene submissions separately, excluding small
offscreen shadow tasks. The observer never writes simulation memory. Disk writes
run on a separate thread; an 8192-entry bounded queue counts overflow rather than
blocking rendering. Four rotating 16 MiB files bound each session's storage.
Local preview defaults can be enabled through performance.ini next to video.ini.
Tests must start muted and use copied profiles/saves. No slowdown fix claimed.


Initial build caught a misspelled configuration field, corrected before replay.
The first enabled replay failed before gameplay with 0xc0000409 in NVIDIA's
nvwgf2umx.dll (Windows Application events 1000/1001, retained crash dump).
The large glyph rectangle batch is a candidate trigger. The next candidate
limits clear calls to sixteen rectangles and keeps non-Windows renderer builds
behind a no-op diagnostics adapter. Failed run remains preserved; no delivery.


The bounded rectangle batches pass subsequent Original and Custom 360/VSync
replays. The initial driver failure did not recur in these checks; the exact
driver-internal failure is not diagnosed. The counter is Windows-only; other
renderer builds receive a no-op adapter rather than Windows dependencies.

Validation: three muted 4,500-VI modern-route runs (Original diagnostics on,
Custom 360/VSync on, Custom 360/VSync off) preserve exactly the same guest RDRAM,
renderer snapshot and save as the prior clean fixture. CSV frame numbers and
timestamps are monotonic, queue drops are zero, and Custom records intermediate
output frames rather than merely counting game submissions. The disabled run
writes no frame log. A screenshot verifies counter text and placement. F8 uses
foreground game-window ownership and rising-edge detection; physical-key/live
fullscreen verification remains for owner testing. Automated tests never take
desktop focus. Rotation/error paths were reviewed, not disk-exhaustion tested.

FPS measures swapchain render-hook cadence, not physical monitor scanout. GAME
counts screen-scene submissions, not every guest simulation tick; 64x64 shadow
tasks are excluded. MS is the last half-second's mean render interval; MAX is
its worst interval. Logging records each raw interval and the latest scene/VI
counters. Pausing, minimizing, loading and diagnostics toggling can introduce
gaps; these must be distinguished from gameplay stalls. GPU utilization and a
precise slowdown cause are not inferred from these counters alone.

Delivery uses the same preview profile and saves in a new immutable package.
The preview defaults to diagnostics enabled through `performance.ini`, with
F8 toggling the counter and recording for the current game session. CSV files
are under `performance` beside `video.ini`; a desktop **JFG - Performance Logs**
shortcut opens that directory. Stop the game to finish the capture, then share
the CSV files with the same session prefix. Four rotating 16 MiB segments retain
bounded recent history per session; old sessions are retained until removed by
the owner. Files contain timing/counter data, not game memory, ROMs or saves.
The observation tool has some rendering cost; compare with F8 off when needed.

Private evidence: `fps-logging-qualification-v2/result.json` and per-mode CSV,
frame captures, stdout/stderr; initial failed run and build receipts retained.
This adds observability and does not claim to fix the newly reported slowdown.


Implementation commit: `930c57517f31f624d2a05527cdcc1b5264ff4c72`.
Machine-local package: `C:/JFG-builds/high-refresh-diagnostics-preview-20261012`.
The existing preview shortcut now targets this package with the same profile;
the new Performance Logs shortcut opens that profile's `performance` folder.
All fourteen manifest entries and isolated launcher package resolution passed.
Runtime SHA-256: `9fc3233e51b0436f2675362bf9c803a81ee0a94a5a43cd1d9ee68612e2caa0f4`.
Old package and shortcut backup remain available; no public release was made.

The Original run recorded 3,218 render rows and Custom recorded 16,132, with
2,056 scene submissions in each. Custom diagnostics-on/off took 76.829/76.812
seconds in this capped replay; this does not establish overhead at unlimited
FPS, different hardware or long sessions. [Aggregate evidence](../../../evidence/frame-diagnostics.json)
links the observations to the qualified runtime. Implementation:
[counter and bounded writer](../../../src/renderer/frame_diagnostics.hpp),
[scene counters](../../../include/jfg/renderer/frame_diagnostics_state.hpp).


## Owner live regression, 2026-10-12

Owner reports stutter only with the newly delivered diagnostic build, with
GAME showing 20 while output shows 120 (Custom 120, VSync on). This supersedes
the narrow muted replay qualification as evidence of live usability. Severity:
high, reported regression plus misleading counter labels; the timing causes require comparison.
Affected runtime: `9fc3233e51b0436f2675362bf9c803a81ee0a94a5a43cd1d9ee68612e2caa0f4`.
Private owner timing capture: `frames-20261012-062848-6524-0.csv`; summarized
in `fps-regression-owner-capture.json`. No reproduction or fix is claimed yet.

Suspects: the overlay emits many rectangle-clear commands every output frame;
we must compare this separately from asynchronous logging. The GAME counter is
actually screen-scene submission cadence, not a verified simulation counter.
The output counter includes interpolated and repeated frames and cannot establish
120 unique game poses or smooth delivery. Original 30 versus observed 20 requires
comparison with the pre-diagnostics runtime and scheduling evidence.

Approved bounded work retains the production guard, investigation root and all
existing counters. Automated tests remain muted and use isolated copied saves.


### Owner F8 comparison and cadence-transition candidate

Owner confirms the stutter continues with F8 off. The raw comparison repeats
a large output gap at scene VI 4113 with legacy/cached/log-only instrumentation.
Pre-diagnostics RT64 trace has four interpolated frames at VI 4110, a single
unmatched scene at 4113 (source_rate=0), then six matched frames at 4116 (20 Hz).
The history policy invalidates every change between two- and three-VI intervals;
normal 30/20 Hz transitions therefore disable interpolation despite valid motion.
This is a specific presentation defect distinct from source simulation slowdown.

Candidate allows ordinary two/three-VI transitions to use their actual source
interval and existing motion/geometry continuity checks. Camera jumps, unsafe
mixed-transform geometry, long gaps and other cadence changes retain their
existing rejection. Focused transition tests and muted native comparisons are
pending. This candidate does not change guest timers or force 30 Hz simulation.


### Completed-sequence replay candidate

The cadence candidate restores six moving intermediates at VI 4113 and retains
the same guest/renderer/save hashes, but a roughly 33 ms output gap remains.
Do not credit it with eliminating the reported timing stutter.

A separate capped-rate defect is visible in the baseline and candidate trace:
VI 4498 is rendered once into four intermediates, then present events 3216,
3217 and 3218 each present the entire 0..3 sequence again without a new scene.
That replays motion backward at the start of each duplicate group. The previous
repeat-latest guard applied only to Unlimited. Finite counters also count extra
targets while their count includes the base frame, so completion was not marked
as the complete sequence.

Candidate marks completion after the final output and extends repeat-latest to
completed capped sequences, with finite-array versus unlimited-ring indexing.
No source updates or scene timing change. Native trace/pixel verification and
regression checks are pending; this candidate is not yet delivered.


### Capped streaming experiment

The completed-sequence correction verifies 505 repeat groups with at most one
output each and preserves all fixture hashes. It removes sequence rewind, but
the fixed-batch 120 FPS moving-window p99 worsens from roughly 10 to 25 ms.
This is not an accepted final performance fix and has not been delivered.

Next candidate reuses the existing two-target, time-sampled producer/consumer
ring for capped interpolation as well as Unlimited. The consumer still enforces
the requested cap and VSync; the producer uses elapsed source-frame time and
yields to new scene work. This avoids finite-batch restart and rate-transition
phase resets without changing guest simulation. Tests and measurements pending.


The initial capped-streaming replay preserves hashes and avoids sequence rewind,
but its moving-window p99 is about 23 ms. Source review also identifies a capped
producer/consumer mismatch: an unpaced producer can fill the ring with multiple
nearly identical early-time poses before the capped consumer displays them.
The next bounded variant paces pose sampling to the cap while retaining the
existing presentation cap and yielding at source-window end/new scene work.
No streaming variant is qualified or delivered yet.


### Separate capped-sequence buffers

The paced streaming variant reduces the moving-window maximum to 10.9 ms, but
replay duration grows from about 76.8 to 81 seconds and wall-time scene cadence
falls. Rejected: smoothing output by making the game wait violates the task.
Neither capped streaming variant will be delivered in this form.

Next candidate restores finite interpolation and gives consecutive sequences
separate alternating target banks. The old shared target array forced the
producer to wait for the previous sequence's consumer, then reduced/skipped
frames because that intentional ownership wait counted against its render
time budget. Two banks avoid cross-sequence overwrite/wait while existing
present-ID ordering protects bank reuse. Unlimited retains its two-slot ring.
Completion/repeat-latest and normal 30/20 Hz transition corrections remain.
Build, replay, memory/scene-speed and presentation checks are pending.


The first two-bank replay preserves baseline wall time/state and holds about
60 outputs/second with a moving-window maximum of 19.6 ms. Its 120/VSync request
was actually clamped to 60: the desktop now exposes one 1920x1080 display and
RT64 reports 60 Hz, whereas earlier runs detected 240 Hz. The test's obsolete
assumption of more than 100 duplicate groups failed (74 actual groups); this
is not a 120 Hz performance pass. Preserve that failed assertion and evidence.

Subsequent qualification will use explicit effective-rate checks: VSync-on at
the currently reported 60 Hz and capped 120/360 with VSync off, without changing
the owner's desktop settings. The old delivered runtime is rerun under those
same current conditions. Target-bank allocation and consumer lookup share a
mutex so vector growth during cadence transitions does not race presentation.


The two-bank candidate alone fails qualification: 120/VSync-off moving-window
p99 is 25.05 ms versus the delivered build's 9.82 ms. Unlimited also fails
the scene-progression gate. No desktop update was made. Actual swapchain
readback proves the previous runtime replays the same four distinct images
for consecutive VI presentations; the candidate holds only the final image.
The next experiment preserves a finite repeat output window while repeating
that final image, so removing rewinds does not introduce idle present gaps.
Readback runs are visual evidence, not timing benchmarks.


## Qualified correction, 2026-10-12

The owner's F8-off comparison confirms that disabling the counter does not stop
stutter. The retained pre-logging runtime also produces 20 Hz source scenes on
this route. Do not call that counter reading an invented conversion, force the
guest simulation to 30 Hz, or confuse 120 outputs with 120 unique authored poses.

Three defects are addressed in the final candidate:

- The bitmap counter emitted hundreds of tiny rectangle clears each output.
  It now uses one cached texture draw, uploaded when displayed text changes
  (normally twice per second). Labels read OUTPUT and SCENES, with an explicit
  INTERPOLATED / REPEATED line. Neither counts physical monitor scanout or
  simulation ticks. `overlay=0` in performance.ini enables log-only isolation.
- Ordinary two-to-three-VI scene intervals (30/20 Hz) invalidated interpolation
  for one frame. These transitions now retain valid transform history; the
  existing geometry, shared-matrix, projected-jump and other cadence rejection
  checks remain. VI 4113 now has six moving intermediates at 120 and eighteen
  at 360, rather than a single unmatched endpoint.
- A later VI restarted an already completed finite interpolation sequence.
  Actual GPU readback shows the old runtime displaying the same four distinct
  images again. The correction repeats its latest image through the paced
  output window. Presenting only one repeat per VI was rejected because it
  introduced idle gaps. Alternating finite GPU target banks prevent the next
  producer sequence from overwriting a target still being presented. Their
  vector allocation/lookup is synchronized; intermediate-target storage is
  roughly doubled. Unlimited retains its bounded two-target ring.

The 120/VSync-off moving window changes from 119.43
outputs/s, p99 9.823 ms and maximum
29.735 ms to 120.00, p99
9.725 ms and maximum 10.892 ms. At 360 the candidate
measures 359.80 outputs/s, p99 3.494 ms and maximum
7.110 ms. These are bounded fixture observations, not a claim that
all owner-visible stutter is fixed.

Original, VSync (currently detected 60 Hz), 120, 360, log-only, 1000 and Unlimited
runs preserve the expected guest RAM, renderer snapshot and save hashes, with
2,056 source scenes and zero CSV queue drops. Capped replay duration and source
progression match the baseline. Unlimited's strict near-30-Hz progression gate
failed in both old and new traced builds; compare its no-trace results separately.
Its high output count does not establish full-speed gameplay under saturation.
No game clock/input/audio scheduling change is made. Focused interpolation-history
and RT64 interpolation tests pass. Test windows are hidden and muted; automated
checks never take desktop focus. Current 60-Hz desktop VSync is not evidence of
physical 120/360-Hz scanout; those rates were tested with VSync off.

Actual swapchain readback verifies completed groups contain only repeated latest
pixels. Readback is excluded from timing benchmarks. Source/aggregate evidence:
[comparison receipt](../../../evidence/frame-diagnostics-regression.json),
[history](../../../include/jfg/renderer/interpolation_history.hpp),
[presentation adapters](../../../cmake/rt64_video_settings.cmake),
[counter](../../../src/renderer/frame_diagnostics.hpp). Private attempts and
failed gates remain under `fps-regression-*`; no ROM, generated code or save is
published. Dependency inputs remain pinned, with patches in build-tree copies.

The local preview remains experimental. Owner live testing, other scenes,
fullscreen/high-refresh display behavior and long sessions remain outstanding.
Original-mode joints/lighting and conservative actor interpolation remain in
[RENDER-002](character-seams-lighting.md); combat throughput remains PERF-001.
The original logging-build claim is not treated as proof that instrumentation
alone caused every hitch. No public release or blanket stutter resolution.

Transferable lessons: a steady FPS counter can conceal backward animation;
validate image order as well as timing. Identical guest memory does not prove
wall-time speed. Check source-scene throughput, compare observation on/off,
verify effective VSync targets, and retain failed candidates and their costs.


### Local delivery receipt

Implementation commit: `19fde3c7c0d5b102e4dd2c2d1fa6e0c9af4fbd67`.
Runtime SHA-256: `b3e871f0cde15a90263f5e691932cf026e6cef182c42f3592ca86023e3fa3a50`.
Package: `C:/JFG-builds/high-refresh-diagnostics-v2-preview-20261012`.
All 14 manifest file sizes/hashes and isolated managed
launcher resolution passed. The existing desktop **JFG - High Refresh Preview**
shortcut targets the new package with its original arguments/profile. The previous
package and shortcut backup remain available. Audio, video, controller, modern
controls, diagnostics preference and save hashes were unchanged by delivery.
Close the existing launcher and reopen the desktop shortcut to load the candidate.
No public release, push, owner desktop input or game launch occurred during
handoff. The tests were muted; owner live acceptance remains pending.


## Source scene-rate investigation, owner approved 2026-10-12

The owner explicitly asks to fix the remaining 20-Hz scene-update drop, with up
to five hours and commits of this work. All prior HFR changes are already committed
(implementation 19fde3c, delivery notes 0fac3b6); this research worktree starts
clean. The existing high-refresh investigation/root and guard retain every charge.
The unrelated main checkout has uncommitted timing/audio/control work and is
preserved. The older PC/audio investigation is shelved and is not resumed here.
Its retained source/evidence can inform this narrow scene-cadence investigation.

Reproduction: the existing muted copied-save Goldwood bridge fixture changes
from two to three VI intervals during movement (30 to 20 screen scenes/sec),
including VI 4113 and following scenes. This occurs before logging was added and
with Original presentation. Presentation-only fixes do not restore source cadence.
Investigate modeled CPU Count cost, real host work/scheduling and explicit guest
frame-selection behavior. Preserve elapsed movement/animation/weapon timing,
input, audio deadlines and save correctness; do not accelerate game speed merely
to raise the counter. Related prior combat work: [PERF-001](goldwood-combat-slowdown.md).
All experiments use muted isolated profiles and guarded execution; no owner input.


The original-policy bridge replay with low-volume update/cost tracing and input
recording retains the exact baseline RAM/renderer/save hashes and 2,056 scenes.
A second recorded-poll replay also preserves them. The next comparison switches
only clock policy after an identical VI-3900 prefix; this is a diagnostic, not
a delivered change. Retained source/evidence paths are recorded privately in
`scene-rate-retained-timing.json`; no unrelated main files are changed.


The baseline charges about 34.1 ms median modeled non-idle CPU work in the
20-Hz bridge interval, crossing a 33.4 ms frame budget. Retained guest source
`gameVi.c::viFrameSync` sets LOGIC_20FPS after a late frame and waits for more
than 30 faster updates before restoring LOGIC_30FPS. The native elapsed-clock
comparison starts from identical VI/poll/Count/RAM at VI 3900 and keeps two-VI
updates (33.44 ms median) through that interval. It is an existing clock policy,
not a guest frame-rate patch or forced time multiplier.

The diagnostic native process reaches VI 4500 cleanly, but its final presentation
is too old for the existing optional renderer snapshot, so the wrapper fails on
missing rdram.bin.rt64. This is a capture limitation, not a qualified full pass;
retain it and compare boundary/trace evidence separately.

Candidate: opt-in native elapsed timing for interactive preview; finite reference
probes keep the original instruction clock. Reuse the existing independently
unit-tested periodic deadline/scanline phase helper to coalesce overdue video
interrupts after a host stall without accelerating catch-up. Add bounded offline
stall and pause observations for qualification. No guest frame-rate clamp, CPU
span generation, unrelated main edits or audio DSP changes are imported.

The first policy-check wrapper incorrectly expected a plain-text `retraces=4500` marker in JSON output. Its reference process exited normally and preserved all three reference hashes. Correct the parser in a new retained step; this is a harness failure, not a runtime failure.



Qualification harness failures retained: the live trace-tail reader attempted to parse a partially flushed hexadecimal field; its hidden play child was stopped by the guarded worker. The follow-on analysis could not find the unfinished play window. The first startup probe requested an incompatible realtime flag and was rejected before starting. Fix the wrappers and rerun; these are not passing runtime checks. The interpolation-trace run overlapped the failed play child and cannot be used as an uncontended performance benchmark.

Native timing restores about 29.8 screen scenes/s in the boundary replay, but the 120-FPS candidate has recurring ~16-ms output gaps. The 360-FPS candidate reaches ~354.6 output FPS with one ~25.9-ms gap. Treat presentation pacing as an unresolved candidate regression and investigate before delivery. Detailed qualification is retained in scene-rate-policy-analysis.json.

The paired stage trace explains the apparent 120-FPS regression: draw-hook
intervals reached 16.4 ms, while actual present-return intervals stayed near
8.34 ms (p99 8.62, maximum 9.14 in that diagnostic interval). The counter sampled
before RT64's limiter, so it measured preparation scheduling, not output pacing.
This diagnostic overlapped a failed wrapper's hidden child and is not a final
performance benchmark. Candidate correction: sample successful swapchain returns,
keep the cached overlay drawing separate, and label CSV schema 2 explicitly.
Monitor scanout/visible fluidity still require owner verification.

The first final play wrapper successfully paused/resumed and requested close, but checked window ownership again after the runtime destroyed its window. That harness assertion terminated the waiting outer process before a complete receipt. Correct the closed-window polling condition in a new immutable wrapper and rerun. The initial startup play attempt shares that wrapper flaw; keep its evidence separate from successful tests.

Unlimited qualification with the new timing boundary stops producing frames at
VI 3900 and reaches the wall-clock watchdog (exit 4). Preserve the failed run:
`scene-rate-final-compatibility/native-unlimited`. Capped modes complete this
boundary. The unlimited ring waits for a free target/previous consumer without
a deadline, despite checking scene expiry only after waking. Candidate: bound
that wait by the existing scene time window, then mark generation complete and
wake the consumer. This imposes no numeric FPS cap and does not overwrite an
occupied target. The candidate needs replay qualification before delivery.

The normal --play path with native timing from boot still has recurring long updates and audio starvation, unlike the identical-prefix boundary replay. The 1.25-second pause itself freezes Count correctly, but this candidate does not yet fix normal-play scene cadence. Startup audio qualification also fails its no-underrun requirement. Do not ship the boundary result as a completed fix; compare the normal-play stalls and preserve scene-rate-live-stall-analysis.json.

The normal-play comparison enabled JFG_PHASE8_FRAME_CAPTURE. In --play this
forces a synchronous GPU readback every 60 submitted presentations, waiting for
the whole presentation queue to idle. The long trace gaps end in present(),
not CPU computation or idle timer oversleep. Ordinary launcher Play strips this
option; it belongs to the explicit heavy diagnostic capture. Correct the timing
fixture to omit periodic screenshots in --play, preserving lightweight CSV and
muted PCM recording. Do not treat screenshot-enabled results as ordinary-play
performance or import unrelated CPU optimizations to compensate for readback.
Heavy capture remains a documented source of stalls and audio starvation.

Bounding the Unlimited wait removes the watchdog, but Unlimited still spends
up to 33 ms inside scene full-sync submission, drops to 20-Hz updates and starves
audio while emitting about 1,390 output frames/s. Candidate: give each in-flight
sequence its own two-slot ring (four targets total), as finite sequences already
have separate banks. Remove the previous sequence's target-reuse wait while
retaining the current ring's slot ownership and deadline. This is bounded GPU
storage, not a numeric FPS limit. Validate before retaining or delivering.

The separate Unlimited banks did not recover source cadence (~19 scenes/s) or
audio continuity; revert that experiment and its extra allocation. The renderer
still ignores queued off-screen shadow passes until the old scene's time window
ends. Those passes can contain CPU-visible readback, so queued work can hold up
the next game update. Candidate: yield Unlimited interpolation to any queued
workload, preserving the bounded slot wait and original two-target ring. There
is still no numeric FPS cap. Check both scene rate and output spacing rather
than crediting a high repeated-frame count.


### Accepted implementation and remaining limits

The private build enables JFG_EXPERIMENTAL_NATIVE_PLAY_TIMING for --play.
Elapsed steady time drives Count at 46.875 MHz; guest viFrameSync, update steps,
movement scaling and save logic remain unchanged. Finite reference probes and
other builds retain the original instruction clock by default. The native policy
removes modeled CPU penalties as the trigger for the reproduced bridge's 20-Hz
fallback when the host has enough time. The game's real-overload fallback and
31-update recovery rule remain intact.

Keep the original two-target Unlimited ring with a bounded scene-window wait,
which fixes the transition watchdog. Extra banks did not recover cadence and are
reverted. Eagerly yielding to all queued work recovered 30 scenes/s but produced
~145 output FPS with ~41-ms gaps, so that experiment is reverted too. Unlimited
can still delay game-dependent rendering and reduce source updates to ~20 Hz.
Use Custom for this scene-rate correction; rates above 360 remain hardware and
workload dependent. Neither rejected change is in the accepted source.

CSV schema 2 measures successful present returns after pacing, retaining its
columns. OUTPUT includes interpolated/repeated images; SCENES counts submissions.
Neither measures monitor scanout. The cached overlay remains one texture draw.
Normal 120-FPS play without periodic screenshots holds two-VI updates (~33.4 ms)
and records zero audio underruns on the bridge. The 1.25-second frontend pause
freezes Count and resumes without catch-up. Boundary comparisons preserve the
route endpoints and copied save. Original-clock probes retain exact RAM/renderer/
save hashes; native final RAM differs and is not claimed as reference parity.

A 200-second startup comparison produces ~200.46 seconds of PCM in the reference
and ~200.51 in native play, with ~98.8% nonzero samples. Native reports three
underruns/one empty pause versus zero in this reference. This fails the strict
zero-underrun startup gate; broad audio continuity and audible quality remain
open. Explicit diagnostic screenshots impose synchronous GPU waits and must not
be used as ordinary-play performance benchmarks. No audio DSP changes or CPU
span optimizations were imported; the shelved audio investigation stays shelved.

Other levels, combat, long sessions, full actor interpolation and reported seams/
lighting remain outside this demonstrated correction. Tests were muted, hidden,
and used copied saves. Every failed wrapper/candidate is retained privately.


### Committed runtime qualification

Implementation `5c7df1bdee61778590fb3b7baa281958a9167870`; rebuilt runtime SHA-256 `a14d9d77fc98ef6be1999a3ea9492d60467a4b107e376c66d431fbfd70231259`.

| Normal play setting | Output FPS | New scenes/s | Update median ms | Output p99 ms | Output max ms |
|---|---:|---:|---:|---:|---:|
| play-120-vsync | 119.66 | 29.76 | 33.428 | 9.019 | 9.534 |
| play-360 | 351.99 | 29.78 | 33.451 | 3.157 | 28.819 |

These are hidden, muted, copied-save tests at 1600x900. The VSync row records the
requested setting; it is not monitor-scanout verification. Both normal-play
runs record zero audio underruns and CSV queue drops. Original-clock replay
retains all three reference hashes. Five focused clock/scheduler/interpolation
tests pass. The longer startup/audio and Unlimited limits above remain open.
[Aggregate qualification](../../../evidence/native-scene-timing.json) records
artifact identities and distinguishes checks made on earlier binaries from the
final runtime. No full-game parity or blanket stutter resolution is claimed.


### Local preview delivery (2026-10-12)

The desktop **JFG - High Refresh Preview** shortcut now selects
`C:\JFG-builds\high-refresh-native-timing-preview-20261012\JFG-Launcher.exe`.
Close the old launcher and reopen that shortcut. Use a Custom cap for this
scene-rate correction; F8 toggles the counter/log, and **JFG - Performance Logs**
opens the existing log folder. The launcher initialization check selected the
new runtime, all 14 manifest entries passed hash/size verification, and the
existing profile/save/settings hashes were unchanged. The old package remains
available. This is a local preview, not a public release.

The implementation commit and exact delivered runtime are recorded above and
in the qualification JSON. Owner live confirmation of the bridge and overall
stutter remains pending. Unlimited source cadence, startup audio continuity,
combat, other levels, seams and lighting are not declared resolved.

## Hidden controller replay fractional-motion regression (2026-10-13)

The extended owner route exposed a replay limitation: physical focus gating
cleared precise stick/scope fractional motion even when the recorded input path
correctly bypassed the physical gate. Hidden replays could therefore diverge
from focused live input. Candidate uses the already computed effective input
block state for both suspension and fractional resets; live focus loss and
explicit diagnostic replay suspension still reset. A fractional slow-stick
regression check is added. Build and owner-route correspondence are pending;
this is not presented as a damage-hitch or scenery-jitter fix.

The hidden fractional-motion correction now passes focused input and replay
checks: recorded low-rate input retains its accumulated fraction while live
focus loss and explicit replay suspension still clear it. This removes one
source of replay divergence; it does not restore full owner-route equivalence.
Final native Play compares log-only and expanded tracing at 120/30 with 18
actual damage events and no diagnostic drops. The owner's damage-correlated
source-rate loss remains open. See [route qualification](tutorial-late-object-jitter.md#review-qualification-2026-10-13).

## Workload GPU observer composition regression (2026-10-13)

In the high-refresh/develop combined profile, the interpolation CMake layer
read the original pinned workload source after the GPU timing layer had emitted
its instrumented copy. The later source substitution discarded the workload GPU
timestamp callback. Missing workload GPU submissions were not zero-cost GPU
execution; earlier conclusions must not interpret them that way.

The correction composes interpolation onto the already hash-validated generated
GPU timing source. The combined runtime builds and its generated workload retains
both the GPU completion observer and presentation changes. Native runtime
verification is tracked with the route-fix evidence; the original dependency pin
is unchanged. This instrumentation repair is separate from a combat FPS remedy.

Expanded interpolation tracing still performed synchronous file writes under a
mutex shared by the submit, render and present paths. It also computed a second
full geometry-history pass solely for a legacy comparison. These are avoidable
observer costs, not established sole causes of the owner's combat drop.

The candidate queues complete formatted rows through the existing bounded
diagnostic writer, drains at normal runtime exit, and appends a ten-column
trace-end footer (last fields: dropped chunks, incomplete flag). Missing footer
or nonzero status invalidates completeness claims. A 4,000-row concurrent test
covers field integrity, final drain and rejection of writes after shutdown.
The legacy comparison is now opt-in with JFG_RT64_COMPARE_MATCHING=1; final
rigid-owned, semantic and camera-coverage observations remain in ordinary expanded
captures. Performance and exact-state replay qualification follow. The temporary
shadow geometry probe has been removed from the deliverable source.

Review also found that normal runtime exits use _Exit, bypassing the frame CSV
writer destructor. Up to its buffered tail could therefore be absent even after
a normal close. The candidate explicitly detaches the presentation observer,
waits for any in-flight enqueue, then drains the writer before exiting. Late
callbacks remain safe while the renderer object is alive. A completed comment
records the final queue-drop/error status; missing completion marks an incomplete
capture. Final runtime close/drain verification remains pending.

The continuation reproduces Unlimited source-rate loss and audio underruns in
both the prior delivered preview and the candidate. Screen submission waits
rise to about 14-17 ms in the candidate, versus sub-millisecond mask full sync.
Additional asynchronous trace events distinguish draw/transform upload waits,
render upload waits, the framebuffer fence and texture upload completion. Every
original wait remains; no synchronization is removed on timing evidence alone.

### Unlimited queue-window candidate, 2026-10-13

The queue-stage trace separates the modes: capped 120 waits about 0.004 ms for a workload slot; Unlimited waits about 9.84-10.01 ms (95th percentile 24-26 ms). Unlimited starts a full interpolation window at worker consumption, although that scene has already waited for submission/queue ownership. The candidate timestamps host scene preparation and uses elapsed age for Unlimited phase, expiry and ring-slot deadlines. Capped scheduling and all synchronization remain. Unknown/future timestamps retain the local clock, and old scenes expire instead of receiving a new window. Native rates, audio, phase continuity and exact-state replays must qualify this candidate.

## Continuation qualification, 2026-10-13

Normal-close frame CSV and interpolation traces now finish with explicit
completion records and zero queue-drop/write-error status in the qualified
runs. The concurrent 4,000-row test, five analyzer tests, capped/Unlimited Play,
pause/resume and Original/Custom replay checks pass. A footer proves records
accepted through shutdown were drained; it does not prove logging stayed
enabled for an entire play session. Older logs without a footer remain
analyzable but cannot satisfy --require-complete.

Expanded tracing queues complete rows instead of writing them synchronously
on renderer paths. The unused second geometry matcher runs only with
JFG_RT64_COMPARE_MATCHING=1. Workload GPU timing now composes with interpolation
instead of being overwritten by the later CMake source substitution.

The scene-age experiment below was rejected after the final native rendering review.
Unlimited remains open; earlier provisional qualification is superseded by the rejection.

## Unlimited scene-age candidate rejected, 2026-10-13

The final native review rejects the prepared-scene-age candidate. It reduces
queue delays and audio underruns (368 in the preceding native sample to 2 in the
candidate sample), and the identical-clock busiest update falls from 49.0 to
39.0 ms with identical guest memory, save and instruction counts. However its
Unlimited presentation p99 expands from about 2.75 ms to 31.0 ms, with output
bursts separated by long gaps. Those native paths are not workload-equivalent,
but the rendering regression is sufficient to reject the scheduling change.
Preserve all evidence. Do not accept lower CPU cost or fewer audio errors alone
as proof of smooth rendering.

All four scene-age source edits are reverted to the tested pre-candidate source.
The selected binary is the queue-observer build 1b0bdd0f, whose exact-clock
guest bytes, save, instruction counts and logical matching equal its preceding
1609c019 build. All original resource waits remain. Unlimited's existing
starvation/pacing issue stays open. No prepared-age correction is delivered.

Final selected-runtime qualification: muted native 120/360 and pause/resume
pass without audio underruns; complete traced runs have no backward phases.
Unlimited remains functionally available but has source starvation/audio errors.
The selected runtime is 1b0bdd0fbfe25eacbf3ed0417c1607a70eed49d4198a6add62f3fba12900f0db; the aggregate evidence supersedes
provisional scene-age results. Physical scanout and the exact late-room visuals
remain unverified. The Original/Custom state pair is on the functional predecessor;
a separate selected-observer pair preserves every guest byte and matching decision.

## Five-hour guarded continuation, 2026-10-14 UTC

Owner approved five hours after the route-fix preview. The same operational
investigation/root and all prior charges are retained; the guard stays enabled.
Selected baseline is implementation aaae085 with documentation e51b0af and runtime
1b0bdd0f. Work prioritizes actual combat source-rate cost and a visibly qualified
late-room fixture. Preserve original game timing, muted owned tests, copied saves,
and raw evidence on D:. The scene-age, direct-camera and secondary subset
experiments remain rejected unless new evidence supports a distinct correction.
No bounded replay, synthetic match or build alone closes the owner's reports.

## Internal Count-transcript bound, 2026-10-14 UTC

A requested 6,500-VI exact-clock capture stopped at VI 4384 after reaching the
existing 16,000,000-sample CountSampleTape bound. The retained tape is exactly
512,000,016 bytes (header plus bounded rows). The caller reports the overly broad
native-clock/non-monotonic-time trap for a null sample, so that text alone does
not prove the host clock went backward. This was an exhausted internal proof
transcript, not a demonstrated game-clock regression. Preserve the partial
capture; it is not a completed replay. Normal Play and ordinary FPS/input/
interpolation logging do not use this Count tape and retain their own limits.
The camera-matching investigation continues with a normal Play capture and
same-frame baseline matching; the transcript limit is not disabled or raised.

The long-camera replay review initially compared the full `update` telemetry
row and rejected matching RAM because host audio consumption differed. The
producer identifies fields 10-13 as host queued/consumed audio bytes and
under/overrun counters; playback progresses differently during a faster Count
replay. This is an analysis error, not a demonstrated gameplay regression.
The corrected review separately compares all guest update fields and every
common complete `update-work` VI/input/Count/idle/instruction record, plus full
RAM/save hashes. It records any terminal incomplete-row difference explicitly.
Original failed reviews and raw telemetry remain preserved on D:.


The explicit CPU-cost trace additionally reports `transport-wake-cost`: VI, input
poll, number of baton receipts, summed publication-to-receipt nanoseconds, and
cumulative maximum wake nanoseconds. It excludes guest-slice execution and is
configured before participants exist. Ordinary Play performs no clock sampling
for this diagnostic. Unit transport checks and exact recorded-state replay pass.


## Private comparison-rate label correction, 2026-10-14

The inventory-loaded360 captures actually saved fps=120 in video.ini. A nested
script-source replacement changed the case label but did not reach the fixture's
FPS argument. The first inventory-final360 request inherited the same error and
is also excluded from 360 qualification. Preserve the captures as loaded 120-FPS
observations only. This is a private harness error, not a video-settings/runtime
regression. A replacement materializes one script and asserts its FPS argument
and saved video configuration. The separate inventory-native360 cases are audited
from their configuration independently. Evidence: inventory-rate-label-correction.json.

Windows CPU sampling could not start: WPR returned 0xc5585011 when enabling its
system-performance profiling policy. No recording was started or cancelled and
no permission changes were attempted. Continue using the in-process stage timers.
The first finer sync-stage probe stopped on a source-pattern assertion before
writing/building code because its input retained CRLF line endings. The bounded
replacement normalizes text before matching and restores the original generated
source in a finally block. Preserve combat-cpu-sampling and sync-stage-probe.


## Logging-cost comparison, 2026-10-15 UTC

After the stale overlay-cache correction, four reversed-order normal-clock
observations compare frame-CSV-only recording with gameplay/actor/interpolation/
PCM capture. Both modes mostly produce 119.7-120.0 output and 29.5-29.8 scene
frames/s in the reviewed combat windows. One expanded window falls to 28.79;
there is no consistent large slowdown across both repetitions. Host CPU averages
15-16%, with none of the monitored foreign game/compiler/find processes seen.
Native paths can differ, so this is neither an exact workload A/B nor proof of
zero recording cost. Evidence: capture-overhead-native-review and per-run loads.

The first attempted comparison exits before gameplay because recorded-clock
boundary/release diagnostics require gameplay tracing. The replacement uses
fully native clocks in both variants and leaves that validation intact. A private
formatter prototype initially could not locate cl.exe despite its child environment;
the replacement resolves the compiler from the imported toolchain PATH. Failed
requests/evidence remain retained; production behavior was unchanged.


## Optional trace formatting candidate, 2026-10-15 UTC

The gameplay TSV formatter uses integer to_chars in place of one printf call
per value. A standalone reversed-order microbenchmark and 12,000 independent
old-format differential cases cover signed timestamp extremes, full-width hex,
name truncation, poisoned buffers and record limits. Existing byte-limit/drain
tests pass. Private capture performance and exact replay are still pending;
this does not establish a fix for combat FPS or change ordinary Play.


The formatter candidate passes runtime qualification: all 4 MiB of physical game
RAM, save data, 1,847 complete guest update records and 17,715 interpolation
matching records equal the prior candidate under the same recorded clock/input.
The trace includes 36,634 actor rows. The first review requested an RT64 snapshot
that neither dense fixture captured; its failed request is retained. No renderer
snapshot equality is claimed for this pair. Normal 120/VSync and 360/no-VSync
expanded captures each complete over 6,000 retraces with no audio underruns,
invalid accesses, dropped frame rows or failed interpolation footer. This is
recording qualification, not a stable-combat-FPS fix. The standalone format
benchmark takes roughly 30% of the old formatter time; ordinary Play is unchanged.
Evidence: trace-format-candidate, trace-format-dense-review and
trace-format-native-review. Local delivery follows package qualification.


## Display-statistics availability probe, 2026-10-15 UTC

A temporary read-only D3D12 probe records 7,603 successful Present returns.
GetLastPresentCount advances, while all successful GetFrameStatistics samples
have zero displayed-present, refresh and synchronization counters. The window
is intentionally hidden, so no physical-monitor cadence is established. A
successful statistics call with zero counters must not be interpreted as either
a displayed-FPS measurement or proof that the game drops every frame.

The query costs 2.1 microseconds median / 4.6 at p99 in this hidden fixture, but
that is not visible-mode overhead qualification. No Present sync interval,
waitable-object behavior, DwmFlush call, clock or limiter is changed. The private
observer is removed and normal source rebuilt; the installed desktop is unchanged.
Microsoft documents that submitted and displayed counts can differ and notes
limitations for multiple monitors/fullscreen interactions and hardware queues:
[frame statistics](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_frame_statistics),
[GetFrameStatistics](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-getframestatistics).

The first private probe retained rows until a static destructor, which the
runtime's intentional _Exit shutdown does not execute. No statistics file was
produced, so that attempt gives no statistics evidence. The replacement uses
a bounded private mapped file that survives process cleanup; existing production
logs already use explicit finish/drain calls. Dependent bridge capture did not
run after the first failed observer and was correctly resubmitted after restore.
Evidence: private-dxgi-statistics, private-dxgi-statistics-mapped and
dxgi-statistics-mapped-review.json. Neither private observer ships.


## Owner-requested deeper bottleneck profiling, 2026-10-15

The owner requests finer cost percentages even if capture adds overhead. A
private diagnostic extends function self timing with owned instruction counts,
native-handler time, synchronous graphics time and dispatch-prefix time,
subtracting nested generated calls. It emits every measured function at update
completion through the buffered writer, plus actor/drone counts and cumulative
command/triangle counts. These are wall-time categories; scheduling and probe
overhead remain visible limits, and nested/inclusive figures must not be summed.

Evidence and exact-state check result: combat-focus-20261014/
detailed-function-cost-profile and parallel-and-detailed-cost-decision.json on D:.
The public desktop remains unchanged. Ordinary Play does not enable CPU_COSTS.
This profiling is investigation support, not a fix or native FPS acceptance.


The first normal-Play profiling attempt exits during setup because production
CPU_COSTS intentionally rejects Play. It performs no gameplay and supplies no
performance result. A private replacement permits profiling only with an input
replay and gameplay trace, adds selected-function thread CPU-cycle measurements
and update service totals, and restores the original gate/source after capture.
It keeps the separate bounded/common-clock renderer candidate explicit.
Thread-cycle counts are inclusive on one host thread; overlapping entries must
not be summed. The same-clock dense state check precedes normal-timing captures.
Evidence: detailed-native-cost-captures, detailed-native-service-profile and
its service-review.json. No public profiling setting or FPS fix is delivered.


A lighter private observer measures each guarded MMIO page from guard-handler
entry through single-step completion and guard rearm. It excludes the initial
Windows exception delivery, includes scheduler interruption, and is not a pure
CPU percentage. It avoids per-function tracing; the recorded-clock dense replay
retains exact RAM/save/update/interpolation correspondence. Normal 120/VSync and
360/no-VSync each finish over 6,000 retraces without audio underruns or unsupported
accesses, but still contain three-VI (approximately 50 ms) game updates.
The 120 FPS combat capture has median active update time 30.304 ms and
median measured MMIO fault time 1.771 ms. Per-page counts and aggregate
wall-time fractions are in mmio-fault-cost-probe/cost-review.json on D:.
The original source and runtime were restored; this observer does not ship.


### Sparse CPU/wait attribution and controlled optimization comparison, 2026-10-15

A lighter observer identifies roughly 1.8 ms/update in measured MMIO exception
handling during one native 120 FPS combat segment (about 6% of active wall time).
It excludes initial exception delivery and includes observer/preemption costs.
The MI interrupt-mask page is its largest component. A private candidate routes
only osSetIntMask's two fixed-address accesses through the existing mask model,
retaining instruction probes, read/write journals and I/O revision effects.
Other addresses, unsupported commands and reference-clock execution fall back
to guarded accesses. A later guarded read refreshes the page mirror.

The first dense replay matches RAM, save, complete update/instruction records,
interpolation decisions and the runtime state hash. Native 120/VSync and
360/no-VSync finish without audio underruns or unsupported accesses, but still
contain three-VI updates. This is a candidate, not a stable-30 fix or delivery.

A sparse follow-up measures selected entity, pathfinding, animation, audio and
graphics scopes with both wall intervals and thread cycles. Its on/off/on/off
recorded-clock comparison uses one executable and exact state correspondence:

- stage-mi-dense-off-a: active median 32.037 ms; game-thread cycles 47.699 million/update.
- stage-mi-dense-on-a: active median 32.200 ms; game-thread cycles 51.823 million/update.
- stage-mi-dense-on-b: active median 31.999 ms; game-thread cycles 45.632 million/update.
- stage-mi-dense-off-b: active median 31.726 ms; game-thread cycles 48.139 million/update.

The initial extracted handler fixture had an unresolved save-device dependency;
its completed replacement is recorded below. Detailed qualification is in
direct-mi-mask-handler-fixture; paired costs and scope hierarchy are in
sparse-stage-direct-mi-paired/review.json, with host-load evidence. Source and
normal runtime are restored after each private experiment; desktop unchanged.

Percentages use CPU cycles on the same thread, not wall-time durations that
include another thread's work while blocked. Nested scope percentages overlap;
they are not a pie chart. Microsoft explicitly advises against converting
[QueryThreadCycleTime](https://learn.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-querythreadcycletime)
counts into elapsed time. Profiler results locate work; final FPS must be checked
with expensive instrumentation removed.


The reusable scripts/analyze_cpu_stage_trace.py analyzer reconstructs selected
scope trees by host thread, subtracts child cycles once, and reports exclusive
and inclusive CPU shares separately. Invalid child counters exclude an entire
update; negative residuals fail validation instead of being clamped. Other
threads, partial records and unfinished scopes are reported separately. Synthetic
regressions cover nested/recursive scopes, siblings, different threads, truncated
rows and VI-window boundaries. Real capture reports are retained under
stage-analyzer-validation on D:. The private stage observer remains opt-in and
is not part of the installed preview.


### Measured combat attribution and qualification limits, 2026-10-15

The sparse profiler's native 120 FPS control covers 233 completed combat updates.
Entity updates consume 75.05% of the game-update thread's measured CPU cycles.
The additive exclusive breakdown is 23.56% drone behavior, 23.34% pathfinding,
18.67% other object-update work, 11.10% level draw-command work, 10.04% animation
matrices, 9.13% other update work, 2.76% rcpFast3d and 1.40% rcpWaitDP. These are
CPU shares on that thread, not shares of total frame wall time or GPU load.
Animation includes work under multiple parents, so its full 10.04% should not
be added to the inclusive entity figure. Nested scopes are subtracted once.
Ten unfinished selected scopes at process close are reported and excluded;
233 complete update trees have valid counters, with no partial stage rows.

Separately, graphics submission consumes 9.389 ms/update of elapsed time in that
window, including 6.011 ms in full-sync/history. Scene history contributes 2.379
ms/update. Measured scene texture/fence waits contribute roughly 0.586/0.574 ms,
and offscreen fence waits 0.254 ms. Enclosing and child durations overlap.
Do not sum these wall figures with the CPU percentages above.

Direct MI-mask handling removes four Windows exceptions per selected read/write
pair. The exact extracted-handler fixture passes 46,656 valid mask/command cases
and 64 reference-clock fallback cases, preserving journals, device effects and
a subsequent guarded read. Two earlier fixture links failed because the full
handler references save-device code; linking the already built runtime resolves
those fixture dependencies. Those attempts produced no correctness result.
Evidence: direct-mi-mask-handler-runtime-fixture; the earlier fixture outputs
remain retained. The four recorded-clock on/off captures preserve state but
do not establish a repeatable whole-update saving amid the observed cost
variation. The native on/off routes also differ in instruction/pathfinding work,
so their different counts of slow updates are not a causal speedup measurement.

A separate private nested-dispatch lease retains binding lifetime across nested
calls and passes existing capacity/failure tests, concurrent nesting, stale
escaped-target rejection and exception/rejection cases. Its small benchmark
varies from about 18-25 ns/call versus roughly 22-23 ns for the control. This
uncertain, small saving does not justify selecting it as a combat remedy. No
production dispatch change is made. Evidence: nested-dispatch-lease-prototype.

The no-20-FPS target is still open. The detailed profiler and analyzer now
identify entity work, especially drone/pathfinding routines, as substantial
CPU targets alongside renderer submission. Heavy profiling is removed for final
FPS acceptance. All captures here were muted and hidden; desktop unchanged.


A private periodic instruction-pointer sampler is qualified with a known busy
integer loop and a sleeping thread. It records thread identity, instruction
address and cycles advanced between checks; symbolization happens after target
sampling, using local pinned binaries/PDBs. Suspension encloses only kernel
context operations and is balanced before allocation, I/O or symbol lookup.
No guest RAM, stack contents or register dump is written. Evidence:
pc-sampler-fixture/result.json. This locates host-code hotspots without adding
per-function callbacks, but its time-based observations are not precise CPU
percentages and its captures cannot be used for uninstrumented FPS acceptance.


The first combat sampler attached to JFG's waiting watchdog parent, collecting
only five samples. Its sample-count assertion correctly rejects attribution;
the game route itself completed, and source plus runtime restoration succeeded.
The corrected capture requires the authenticated child-mode argument and the
owned process ancestry, without logging its token. It reuses the exact retained
anchor runtime/PDB and qualified sampler, then restores the ordinary runtime.
Evidence: pc-sampling-combat (rejected parent capture) and pc-sampling-child
(corrected child capture). No kernel permissions, guest memory changes, game
function timing hooks or public profiler option are introduced.


The all-thread child sampler reached its 300,000-observation limit before combat.
Suspending sleeping threads also advanced their cycle counts, defeating the
activity filter; those captures provide no combat attribution. The replacement
records the update thread identity once, samples only that owned thread and
requires combat-window coverage. Its known-loop fixture excludes the sleeping
thread. Exact recorded-state comparison and restoration remain mandatory.
Evidence: pc-sampling-update-thread. Periodic RIP observations still include
waiting and observer effects, so they are not exact CPU percentages or FPS
acceptance. Earlier incomplete captures remain preserved.


### Optional per-update thread-cycle observation

JFG_DIAGNOSTIC_UPDATE_CYCLES=1, together with JFG_GAMEPLAY_TRACE, requests one update-thread-cycles TSV row per measured game update. Fields are VI, controller poll, valid flag, host thread ID, cycle delta and guest-instruction delta. Correlate with update-work by VI/poll. This measures only the host thread that runs the root game update; it excludes renderer/other guest threads and must not be converted to CPU milliseconds. Invalid thread/query results report valid=0 and zero cycles. Ordinary FPS logging does not enable it. Source and replay qualification are tracked with PERF-001; this diagnostic does not establish a slowdown cause by itself.


The repository analyzer scripts/analyze_update_cycle_trace.py reads the optional update-cycle timeline and reports the slowest updates with known wait, thread cycles, instruction counts, enclosed graphics submissions and time outside the measured root update. Outside-update time may contain both work and waits, so it is not automatically attributed to CPU execution. Example: python scripts/analyze_update_cycle_trace.py trace.tsv --first-vi 3500 --last-vi 6000 --output update-costs.json. It handles repeated VI/poll keys in event order, rejects invalid cycle joins, reports missing/truncated observations, and excludes boundary-crossing submissions from nested cost totals. Focused fixtures cover those cases; the final native capture has complete valid observations. Wall time is not CPU time; nested submission/full-sync costs are never added together. Evidence: update-cycle-analyzer. Hidden-window capture remains distinct from physical-display acceptance.


The detailed profiler correlation fixture initially failed because its synthetic baseline timestamp used a double-escaped tab; no real capture was reinterpreted as passing. Correcting that fixture passes the CPU/host/GPU join checks. The dependent review refused to run until that check passed. Evidence: hyper-profile-analyzer-correlations (failure), hyper-profile-correlation-fix and hyper-profile-correlated-review.


The renderer-stage profiling build initially exposed a missing array include, omitted private/public diagnostic-field propagation, and MSVC getenv deprecation under warnings-as-errors. The corrected build retains strict warnings and copies the timing fields explicitly. Evidence: hyper-profile-renderer-stages (failed build) and hyper-profile-renderer-build-fixed. No failed executable was delivered.


### Qualified bounded function, graphics and scheduling profiler

Owner-requested full function/caller profiling is implemented with a lower-overhead stage mode, per-update CPU/wall totals, maximum call cost, process cycles, guest-baton wake latency, GPU timestamps and interpolation-preparation/RDP-sync sub-stages. The memory trace is bounded to 256 MiB and drains at normal finish. Default behavior remains disabled. Full recorded comparisons preserve RAM/save, instruction counts and interpolation decisions; the final disabled runtime also passes the retained Original comparison. Native 120/360 diagnostic captures complete, but profiling changes frame cadence and does not qualify a slowdown fix. Full mode adds roughly 11 ms median cost in the recorded comparison, whereas stage mode adds roughly 1 ms in that observation. Captures and immutable failed/repaired fixture/build evidence remain on D:. The desktop preview still selects the earlier compact-timing package. See [capture controls and interpretation](../function-profiling.md) and [binary-bound evidence](../../../evidence/high-refresh-function-profile.json).


The combined resident-math/profiler runtime also passes the full instrumented recorded comparison and profiler-disabled Original/native checks. Detailed logging remains opt-in and its overhead remains explicit. The final binary and captures are linked in [combined qualification](../../../evidence/high-refresh-resident-math.json); this does not mark the underlying combat slowdown resolved.


The combined profiled-math binary is now installed on both high-refresh desktop links. Detailed profiling remains an explicit developer-runtime option; ordinary F8 FPS logging does not activate per-function queries. Logs and captures remain private on D:. The delivered binary and source commit are in [the combined receipt](../../../evidence/high-refresh-resident-math.json).


The first private synchronization patch rejected its single-site assertion because framebuffer fence waits occur at more than one pinned site. It changed no source/runtime and its dependent capture refused to run. The corrected patch accounts for every occurrence, retains all waits, and records per-kind call counts alongside durations. Snapshot, display-list submission and CPU-writeback timings accompany the six wait kinds without enabling broad interpolation tracing. Exact-state validation remains required. Evidence: submit-sync-breakdown-build (refused) and submit-sync-breakdown-build-fixed.


The submission-owner diagnostic also preserves exact recorded RAM/save/instructions/interpolation. Native collection records valid same-thread cycles and OS CPU-time deltas for snapshot/decode/writeback, correlated by event order with six wait kinds. CPU-time tick quantization makes individual tiny substage deltas noisy; aggregate counters and wall/cycle patterns must be interpreted together. Evidence: submit-thread-attribution-captures and submit-thread-attribution-review.


### Submission profiler source integration
The private six-wait and submission-owner diagnostics are now integrated as an opt-in, thread-local collector. The analyzer joins repeated submission keys in order, distinguishes root/other CPU owners, excludes boundary-crossing work, and reports unpaired, missing or invalid records. Nested collector scopes and exception restoration retain the previous state; worker counters remain separate. All waits and ordinary default behavior remain unchanged. Fixtures, rebuild, exact recorded correspondence and disabled runtime checks are required before delivery. Evidence: submission-profile-source.


### Private Windows thread-scheduling attribution
An owned-thread fixture successfully enables Windows dispatch profiling without hardware counters or driver/system changes. It records context-switch counts, wait-reason bits and cycles excluding interruptions, and disables profiling before thread exit. A private runtime experiment samples only root-update and synchronous submission intervals. Actual thread identities remain explicit; wait-reason bits are since the last read, so any nested reader would make them incomplete and must be rejected in analysis. No hardware PMU access is claimed. Exact recorded validation and native correlation remain pending. Evidence: windows-thread-dispatch-fixture and thread-dispatch-profile-build-fixed. The first runtime build placed its private header before Windows declarations; it failed at compile time and restored source/runtime. Its dependent captures refused execution.
API contracts: [EnableThreadProfiling](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-enablethreadprofiling), [ReadThreadProfilingData](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readthreadprofilingdata), [PERFORMANCE_DATA](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-performance_data).


### Submission profiler qualification checkpoint
The integrated runtime passes all 13 analyzer fixtures, the nested/exception/thread-isolation C++ collector fixture, exact recorded combat with profiling disabled/enabled, and the retained 17,000-VI Original comparison. All final native 120/360 captures have valid submission-owner joins and zero audio under/overruns or unsupported accesses. The observed source cadence is zero three-VI intervals in the 120 stage capture and one in the 360 stage capture; different native paths and observer cost prevent a no-dip claim. Runtime SHA-256: 699502f5f4118366b9af349056ddc5eaa7d78f5e1986eb67e70674947280cb67. The desktop package remains the earlier profiled-math build. [Qualification receipt](../../../evidence/high-refresh-submission-profile.json).


### Nested scheduling readers corrected in private experiment
The first runtime dispatch capture passed execution/state checks but rejected attribution: root and submission readers can share a thread. Windows resets the wait bitmap on each read. The corrected private collector accumulates every read into all enclosing scopes, while retaining cumulative cycle/context-switch deltas and RAII cleanup. Analysis keeps nested CPU cost inclusive and avoids adding same-thread submission cycles to root cycles. The failed capture and refusal remain preserved; this is not a gameplay regression. Evidence: thread-dispatch-profile-captures-fixed and thread-dispatch-profile-build-nested.


A separate cadence analyzer now retains every missed update interval from ordinary route traces and separately reports rendered present-return stalls. It correlates root work/waits/cycles, native collision counters, level state and cold overlays without classifying correlations as proven causes. Missing, partial and queue-dropped evidence stays explicit. Synthetic tests and private route analyses are recorded under native-collision-cadence-analysis; full profiler overhead remains unnecessary for this report.


The final collision preview integrates and qualifies the expanded submission profiler, update/present cadence analyzer and rare level/death transition markers. The analyzer retains every missed interval and flags partial evidence. Exact recorded, native profiled and default-Play captures are identified by binary hash in [combined qualification](../../../evidence/high-refresh-native-collision.json); delivery is a separate step.


### Bounded collision preview delivered locally
Both High Refresh Preview and Jitter Capture desktop shortcuts now select source commit 8f94f148ec367e4a221679a40255f12e6353af07 and runtime SHA-256 34ce90c9a11bba75a9ae47f3ae3d8a40d635b20357bf8c52391f4bd015a6bff1. Package and launcher/runtime setup checks pass; shortcut arguments and all 40 checked profile files are preserved. Previous package and links remain available for rollback. The qualified collision optimization, expanded submission diagnostics, cadence analyzer and explicit transition markers are included. Detailed function profiling remains off for ordinary play. This is a local preview, not a release or universal no-20-FPS claim. Late crate/light/bridge jitter and shadow cadence still require owner visual review. [Current evidence](../../../evidence/high-refresh-native-collision.json).


### Final delivery audit

The final binary also completes a muted capture-mode comparison with actor, interpolation, poll and progress logging enabled (recorded inputs replace live input recording). Its combat median active duration is 20.392 ms; extended update intervals are {'2': 1229}. No new audio-buffer errors or unsupported accesses occur. This bounded comparison does not remove the observer-cost caveat for full function profiling.

The cadence analyzer now includes transition calls anywhere between consecutive update returns, including calls outside either measured root. Final normal-Play gap associations: [{"vi": 6588, "delta_vi": 64, "transition_kinds": [], "latest_transition": {"stamp_us": 109416093, "vi": 6494, "poll": 3170, "kind": "death_restart", "age_ms": 1568.366}}, {"vi": 6591, "delta_vi": 3, "transition_kinds": [], "latest_transition": {"stamp_us": 109416093, "vi": 6494, "poll": 3170, "kind": "death_restart", "age_ms": 1611.178}}]. These are actual observed transition entries, not causes inferred from a level number alone. Further owner visual review is still required for the reported late scenery/shadow jitter; a fresh route is also needed to judge combat under live inputs.


A private full-function capture timed out because its stop monitor waited for VI rows in a trace intentionally buffered until shutdown. The forced stop lost that buffer; no measurements from that attempt are accepted. The corrected helper monitors the independent FPS CSV, requests normal window closure at its VI target, then analyzes the flushed trace. The recovery and narrower explosion capture pass profiler accounting checks. Evidence: residual-goldwood-profile-recovery and residual-goldwood-profile-late. This is a capture orchestration correction, not a runtime FPS fix.

The first private RT64 stage replay exited before window creation with STATUS_DLL_NOT_FOUND because only the executable had been copied to its isolated evidence directory. It produced no performance evidence. The recovery uses the original build runtime directory and checks the executable hash against the candidate; the failed capture is retained. Evidence: late-renderer-stages and late-renderer-stages-v2.

The runtime-directory stage replay reached normal window closure and flushed its memory trace, but the parent rejected extra private stderr output. Treat the diagnostic protocol failure separately from gameplay timing; do not promote this as a clean qualification run. Subsequent private stage output must use a dedicated file. Evidence: private-stage-protocol-review.

The first unprofiled draw-key replay supplied FUNCTION_PROFILE=0 and retained dependent profile flags; the parser rejected that unsupported configuration before gameplay. The recovery unsets the profiler, window and GPU-profile variables together. A nested-string setup was withdrawn before execution and replaced with direct setup. Failed setup is excluded from acceptance. Evidence: draw-key-native-late and draw-key-native-late-v2.


## Longer measurement coverage regression (2026-10-16)

The owner trace capped at 256 MiB before Goldwood exit. Full frame records survived, but game-update timing was incorrectly gated by optional semantic hashing, and normal live exit health totals were only printed to stdout, which the launcher filters. Prior expectations of continuous game/audio coverage were false. Candidate remedy emits compact game-update rows whenever timing is enabled, includes cumulative audio counters in progress snapshots, and writes play-health on clean live exit. These use the existing asynchronous timing writer; no hash stream is enabled. Test verification and final long-run audio acceptance remain pending.

The candidate passes interpolation and asynchronous writer checks; the latter initially lacked its required output-file argument and was rerun with the CTest argument. A muted live-mode replay with detailed trace and semantic hashing both disabled retained all 2,068 compact update rows, matching final play-health and native stdout audio counters (zero underruns/overruns), plus progress audio fields. This verifies independent compact coverage; it does not retroactively repair the owner capture or qualify the exit cutscene. Runtime candidate remains undelivered pending renderer qualification.


### Periodic screenshot contamination (2026-10-16)
The owner long capture enabled a framebuffer readback every 60 native presentations. The first 18:27 contain 607 such calls: median 37.300 ms, p99 54.197 ms; 604 exceed 25 ms. The remaining 35,821 presentation calls have median 0.024 ms, p99 0.620 ms, maximum 9.538 ms. Of 357 coalesced-VI events, 355 are within 10 ms of the screenshot's completion. Present event counts agree with all 55,347 tick counters, so the periodic classification is independently checked. This is strong correlation, not yet a causal A/B proof, and does not cover the later exit cutscene. Candidate removes automatic screenshots from launcher route diagnostics while retaining inputs, compact timing, interpolation, actor traces and final RAM. Explicit developer framebuffer probes remain available. Private evidence: shuriken-reticle-20261016/capture-overhead-correlation.json.


The controlled live-mode comparison confirms the periodic-readback cost: identical recorded inputs/profile, same binary and diagnostic streams, with only screenshots enabled/disabled. In the measured VI 3,500-4,500 window, screenshots produce 11 presentation calls above 25 ms (maximum 40.912 ms) and six three-VI/20-FPS updates. Without screenshots, all 501 game intervals are two VI/30 FPS and presentation calls stay below 0.095 ms. Both complete with zero audio underruns/overruns. This verifies the capture hitch remedy; it does not clear the Goldwood exit, which still needs a new capture with complete compact health coverage. Launcher checks pass with route screenshots disabled. The private analysis script first rejected the trace header as a number; its corrected event filter preserves the failed attempt and all raw evidence.


The final reticle runtime repeats the normal Play-mode coverage check: 2,070 consecutive compact game-update rows, final play-health counters matching the native exit record, and progress audio fields, with detailed gameplay tracing and semantic hashing disabled. Both audio counters are zero. The revised launcher passes 641 managed checks and 15 setup checks. [Qualification](../../../evidence/high-refresh-reticle.json) retains the screenshot readback A/B result; a new long exit-cutscene capture is still required.


The compact health logging and screenshot-free route capture are delivered locally with source `26295c75dd43848f59cdd1268518d7131861dbee` through both preview shortcuts. Earlier capture evidence remains intact; this does not retroactively supply missing final counters or resolve the Goldwood exit-cutscene report.


### Ichor route live-prefix coverage (2026-10-18)

The official v1.2.0 owner route reaches the existing detailed gameplay/actor
trace limit at 949.097411 seconds (VI 55915, update 27461), after the complete
Vela rescue sequence. Compact timing, FPS/output, input and renderer streams
continue independently; the earlier compact-coverage remedy remains in force.
A read-only existing-live-export observer retains subsequent room/control-lock
context for the boss report. It cannot restore the truncated detailed actor
records. Keep this coverage limit explicit in later replay qualification.
On Windows, file directory metadata for these active writers can report zero
or stale length while the files are readable: use an opened handle's end offset
for live prefix bounds and reject incomplete last rows. No game restart or
change to the active capture was made. Private evidence:
issue20-ichor-controls-20261018/live-rescue-review-01 and live-rescue-sync-03.


## Multipass scene counter correction (candidate)

The closed Ichor boss capture contains two screen-sized tasks for every game
update: world rendering and motion blur. The old SCENES counter increments on
both, yielding about 60 while compact simulation timing remains 30 updates/s.
The candidate counts distinct source VI ticks, including a valid first tick of
zero, and leaves output FPS independently measured at presentation. Additional
passes no longer reset the age of that source scene. This corrects the counter;
it does not increase output FPS or repair interpolation/compositing.

A regression check replays the observed two-pass/two-VI pattern and checks
duplicate scene age. Native scene/output verification remains pending.

The unique-VI counter check passes in the focused preview, and the CSV header now describes that counting rule. This diagnostic correction does not qualify blur-scene output pacing.
