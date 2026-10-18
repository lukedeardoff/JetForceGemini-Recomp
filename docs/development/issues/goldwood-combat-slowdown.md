# PERF-001: Goldwood combat slows game updates

- **Severity: critical. Status: open; native ROM loading improvement qualified after collision/math/timing optimizations. Tested combat windows retain two-VI updates; the repeated renderer hitch is corrected in bounded tests; cold-load/restart hitches and full-route live acceptance remain open.**
- Reported: 2026-10-11 UTC. Last updated: 2026-10-16 UTC.
- Affected: modern-controls v1.1 local preview with original instruction-clock runtime;
  source capture runtime SHA-256 `de2737396c3f840291de8a9155eaed415f6076beaf4d27200be6505cde11c538`.
- [Implementation investigation and research](../../features/pc-controls.md#2026-10-11-critical-goldwood-timing-investigation).
- Related: CAMERA-001 small boom-distance steps; CAMERA-002 large collision pull-ins.

## Symptoms and reproduction

The owner reported lag while fighting Goldwood drones and crossing the bridge after
the first enemy section. Rapid mouse turns made the issue noticeable. The worst
combat window in capture `20261011-011404-f9cc9def804541998e2a5fa4d250df0b` is
approximately 190-210 seconds. The late bridge segment is approximately 237-268
seconds. A steady 30 Hz camera has inherent latency, but it cannot explain the
measured combat collapse to roughly 15 game updates per second.

## Evidence and cause

Host VI ticks continue near 16.7 ms while game updates reach about 67 ms median,
with 85-98 ms gaps in the source capture. CPU graphics submission is about 2.6-3 ms;
this is not GPU completion time and does not rule out all graphics bottlenecks.

The reference execution policy charges Count for modeled N64 instruction and cache
costs, then paces the host to that timeline. It therefore reproduces the modeled
console CPU throughput limit and also spends host CPU time evaluating that model.
A light-trace run measured about 62.3 ms modeled non-idle work per update in the
worst window. This is a demonstrated contributor. It does not by itself explain
all residual host execution or scheduling costs.

Actor trace rows are changes, not complete snapshots. Reconstruct by slot and
apply removals. Corrected source combat counts are 44-58 total table entries and
1-7 enemy entries; these are not visible-on-screen counts. Earlier counts derived
from individual delta rows are invalid.

## Attempts and results

| Experiment / hypothesis | Result | Decision |
|---|---|---|
| Fixed modern desired camera distance | Removed 186 unobstructed fixed-angle relative steps over five units in comparable replay pairs | Separate camera fix delivered; does not repair combat update rate |
| Full versus light capture logging, same runtime/input | Slowdown remains; final RDRAM identical (`757b7094a9d6fabd671e4c90ba7d50e54a52741d3e7ff3ffb57df3f062b93d99`) | Logging is not the sole cause |
| Extra modern camera visibility-query cost | About 0.14 ms modeled / 0.52 ms host per traced invocation | Too small to explain the measured deficit |
| Replace modeled CPU clock with elapsed host clock | Removes modeled per-instruction/cache charges, preserves guest OS/device deadlines | Promising diagnostic candidate, default off |
| Replay old recording strictly by poll number | Different clock changes real input durations | Invalid performance acceptance comparison |
| Replay by absolute recorded VI | Startup menu readiness differs | Invalid route comparison |
| Anchor timed replay at first gameplay pose | Start matches, long open-loop route later diverges into a different level | Do not claim the full-route FPS comparison as Goldwood proof |
| Load the same Goldwood checkpoint saved by the game | Same initial pose, 47 actor entries, two enemy entries, 27 shots each; median 49.96 to 33.61 ms across movement/firing, candidate p95 50.19 ms | Improvement, residual slow intervals remain |
| Change clock only after identical combat prefix | Full RDRAM, Count, VI and input poll identical at boundary; both remain level 21 | Stronger comparison below; still below target |
| Instrument native idle deadlines and wake-up times | Ordinary sleeps overshot by 1.247 ms median / 1.890 ms p95 over 1,868 observed gameplay waits; run then trapped | Scheduling cost demonstrated; incomplete run is not acceptance |
| High-resolution Windows waitable timer | Built after correcting Windows header-order macro pollution; native run trapped at VI 4098 on equal PI/SP/DP deadlines | Timer candidate remains diagnostic; event-order qualification is required |
| Inline native ordinary-instruction bookkeeping | Original 250,000-transition regression plus native no-time-charge / bounded-safepoint / hardware-exception boundary checks passed; runtime rebuild in progress | Avoid a full C++ callback per ordinary native instruction without dropping clock safepoints |

For seconds 1-21 after the identical combat boundary, reference median/p95 are
**67.88/97.78 ms**; candidate median/p95 are
**49.89/51.82 ms**. Median enemy-table count is seven in both.
Later state can diverge as changed cadence changes sampling; identical initial
memory does not imply identical future AI or frame-by-frame behavior.

## Actual resolution

**No final resolution yet.** The local Modern Controls Preview now uses the
qualified native elapsed-time clock and contiguous instruction spans. Its v1.1
launcher and saved profile are unchanged. See the [final candidate measurements
and acceptance](#contiguous-candidate-qualification-and-handoff) below.

`HostCountClock` in
[`include/jfg/boot/host_count_clock.hpp`](../../../include/jfg/boot/host_count_clock.hpp)
and its integration in [`native_boot.cpp`](../../../src/boot/native_boot.cpp)
remove modeled console CPU/cache costs from the private interactive build's
timeline. The explicit `JFG_EXPERIMENTAL_NATIVE_PLAY_TIMING` build option remains
off by default; reference probes retain their deterministic instruction clock.
No release or fix commit is claimed. Residual slow updates and live acceptance
remain open.

The separate high-refresh preview first enabled elapsed timing during play on
2026-10-12; see [PERF-002](frame-diagnostics.md#local-preview-delivery-2026-10-12).
Historical results below identify their specific build/profile. They do not
prove a full-tutorial fix or reopen a shelved investigation.

The candidate uses a steady clock at 46.875 MHz, preserves fractional timer units,
handles guest wrap with a 64-bit count, excludes frontend pauses, and services
normal hardware/OS deadlines. It retains game simulation and actor processing.
It does not multiply world speed, skip actors, or conceal drops with smoothing.

## Initial verification and acceptance history

These initial results and outstanding checks describe the first clock candidate.
Later sections record subsequent checks, failures and current acceptance.

- 148 clock checks and 18 recorded-input timeline checks passed; the nine targeted
  timing/input test executables passed together.
- Controlled movement is identical under both clocks: 243.04 world units/second
  forward/back and both strafe directions in the tested fixture.
- Held aim survives firing; shot counts agree in movement and Goldwood fixtures.
- Native Save changes only a copied test save; reload restores the saved 95-round
  pistol state. Source/live saves remain unchanged.
- Default reference-clock replay remains byte-identical to the retained baseline.
- Still required: consistent combat cadence, startup/cutscene/audio timing,
  overload and stall recovery, broader controller/input coverage, frontend pause
  and physical focus behavior, and owner verification of the fight and bridge.
- Large collision-driven camera pull-ins and unqualified pivot animations remain
  separate issues. A passed timing test does not resolve them.

Private artifact root: `combat-critical-evidence-20261011`, within the existing
`pc-input-aiming-20261008` guarded investigation. Preserve scripts, runtime hashes,
input/profile provenance and reports; do not publish ROM or personal save data.

## Lessons for other recomp projects and future 60+ FPS work

Distinguish elapsed real time, guest timer units, guest simulation ticks, VI,
rendered frames and presentation. An emulated CPU/cache cost model is a valuable
deterministic reference, but retaining its throughput ceiling as the native game
clock can preserve original slowdown despite abundant host performance.

A host-time clock is compatible with future higher-refresh rendering, but does
**not** itself make the simulation run correctly at 60+ Hz. Keep the game-speed
contract independent of render/present rate. Audit movement, animation, weapon
cadence, AI, audio and input for per-update assumptions before changing simulation
frequency. A possible later design renders interpolated transforms between fixed
simulation steps; camera collision, input latency, discontinuities and state
ownership still require their own design and tests. Do not blindly double ticks
or halve deltas. No interpolation or 60 Hz simulation is implemented here.

Use bounded catch-up and explicit pause/stall policy, not an unbounded backlog.
Timing-policy changes also invalidate poll-count-based real-time comparisons;
use matched checkpoints, time-based input with conserved mouse displacement and
short button edges, and report route divergence. Measure presentation/GPU and
actual update cadence independently; an FPS counter alone does not locate a cause.

Primary architectural references: [N64ModernRuntime timer](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/ultramodern/src/timer.cpp),
[VI scheduler](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/ultramodern/src/events.cpp),
[Perfect Dark timing](https://github.com/perfect-dark-pc-port/perfect_dark/blob/32a1cb9f268dd3ac73016801025c6bbbfa20130f/src/game/timing.c),
and [Fix Your Timestep](https://gafferongames.com/post/fix_your_timestep/).
These guide architecture; they are not drop-in fixes for this runtime.


### Follow-up: simultaneous device events and timer precision

The high-resolution wait run stopped at VI 4098: PI, SP and DP all had absolute
Count deadline 3243790045; current Count was 3243822106. This is an exact deadline
tie, not merely several different deadlines becoming overdue. The original
reference selector only qualifies SP-before-DP ties and deliberately rejects
other simultaneous-source combinations. The failure check remains in place.
Changing clock sampling exposes a native-runtime scheduling requirement that
must be resolved with an explicit, validated event contract. Arbitrary priority
changes or offsetting one deadline to avoid a tie would conceal the issue.

The Windows wait candidate uses a high-resolution one-shot waitable timer,
rounds the relative timeout up to 100 ns units, rechecks steady time after wake,
and keeps the original device deadline. It does not busy-spin or change game
speed. The API requires Windows 10 version 1803 or newer; unsupported creation
currently fails the diagnostic mode rather than silently claiming high precision.
See [CreateWaitableTimerExW](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw)
and [SetWaitableTimerEx](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setwaitabletimerex).
Both recorded wait probes stopped before the route finished. Their partial
observations cannot qualify either a final performance improvement or a release.

The generated native inline rebuild did not complete within this build step; continuation required. Its focused tests passed; this
is not a performance acceptance result. The installed runtime hash was checked
and remains unchanged. Additional owner-approved work continues under the same
production guard and investigation with all prior charges retained.


### Continuation: native scheduling qualification

The owner approved two additional hours in the same investigation, preserving
all guard counters. The first generated inline rebuild exceeded its eight-minute
command budget. The continuation rebuild has its own bounded command inside the
same guarded investigation; this is not a new investigation or a counter reset.

The native event candidate commits all independent device completions sharing the
earliest due timestamp before the guest CPU observes an interrupt. Guest PI/SI
transfers already make their data visible at launch in the retained device
profile; their completion changes status/interrupt state. SP completion finishes
its owned task before the CPU runs again. The original reference policy remains
unchanged, including its rejection of unqualified ties. This is a separate native
elapsed-time contract, not a new claim of cycle-exact N64 timing parity.

Tests cover all nonempty device subsets and their input permutations, duplicate
sources, future/earlier deadlines, integer limits, actual PI/SI/SP completions in
all 24 orders, interrupt masking/acknowledgement, and unchanged reference tie
rejection. Integration and performance outcomes follow below.

- `finish-native-inline-runtime-build`: process exit 127 (zero alone does not establish performance acceptance).

- `native-inline-clock-goldwood-before-batch`: process exit 125 (zero alone does not establish performance acceptance).

- `native-event-batch-candidate`: process exit 125 (zero alone does not establish performance acceptance).

- `native-event-batch-test-build`: process exit 125 (zero alone does not establish performance acceptance).

- `native-event-batch-tests`: process exit 125 (zero alone does not establish performance acceptance).

- `native-event-batch-runtime-build`: process exit 125 (zero alone does not establish performance acceptance).

- `native-event-batch-goldwood-checkpoint`: process exit 125 (zero alone does not establish performance acceptance).

- `native-event-batch-combat-boundary`: process exit 125 (zero alone does not establish performance acceptance).

- `reference-event-batch-combat-boundary`: process exit 125 (zero alone does not establish performance acceptance).

- `compare-event-batch-combat-boundary`: process exit 125 (zero alone does not establish performance acceptance).

No completed matched combat comparison for this revision; acceptance remains pending.


### CPU versus GPU work

RT64 already renders on the GPU. The diagnosed clock charges, ordinary instruction
callbacks and device waits are CPU-side work. Moving AI/physics to GPU compute
would require data/layout and synchronization changes; it is not a demonstrated
remedy for this timing defect. Graphics submission timings do not establish GPU
completion latency. If residual stalls implicate rendering, measure GPU timestamps,
queue/fence waits and presentation separately before proposing further offload.

The guest stack transport also uses parked host OS threads with a single execution
baton. Context-switch overhead is a possible additional cost, not a confirmed
bottleneck in this recording. Do not replace it merely because fibers might be
faster; quantify the cost and preserve context, exceptions, sanitizer support,
thread-local state and guest scheduler behavior before changing the transport.


### Completed scheduling replay pass

The incremental build completed after the command timeout; completed objects
were retained and no original/generated game bodies were changed. The native
inline build without the event batch reproduced an exact PI/SP/DP deadline tie
at VI 3816. After adding the explicit native event batch, its 41,370 focused
checks passed and the saved Goldwood movement/firing replay completed.

Matched combat prefix memory, Count, VI and poll equality: **True**.

- Reference seconds 1-21: update median **67.79 ms**, p95 **98.69 ms**; final audio counters [19124224, 19117568, 8, 0] (queued/consumed/underflows/overflows).

- Native seconds 1-21: update median **50.01 ms**, p95 **54.91 ms**; final audio counters [19124224, 19113728, 2, 0] (queued/consumed/underflows/overflows).

Windows timer deadline/reuse/lifetime check process exit: 0.


The owner approved six additional hours on the same investigation. This does not
reset counters, disable the production guard or change the installed preview.
Critical acceptance remains open until residual costs and the listed regressions
are qualified. Supporting replays alone are not a release claim.

## High-refresh visibility preview combat follow-up (2026-10-13)

The owner frequently sees 20-25 SCENES while fighting Goldwood drones despite
120 OUTPUT in visibility preview `c9189c1` (runtime SHA-256
`cb3de8f6593fa2abb0a12e7a2ea7f98ecf23ea817ad247505c1338a39dcd2292`). Drones and gems
now look good, but this scene-cadence drop destroys the smooth result. Critical
combat acceptance remains open. This report is in the current elapsed-clock
high-refresh build, so the original modeled-clock contributor alone cannot
explain it. Measure host work, renderer waits and the game's 31-update recovery
rule without forcing simulation speed or hiding drops in the counter.

One owner-approved guarded continuation in the existing high-refresh investigation
runs within the prior deadline. This does not reopen the PC/audio investigation.
Current before/after evidence will be recorded here; no new fix is yet claimed.

The normal 120-cap copied-checkpoint combat replay reproduces 19.8-22.1 scenes/s
through the fight. Diagnostic elapsed-clock samples show repeated genuine fast
frames while the guest remains in its 31-fast-frame recovery hold. Candidate:
seed only the native 20-Hz recovery counter to its threshold before viFrameSync;
the original late-frame test, VI waits and returned elapsed interval stay intact.
This allows the next genuinely fast update to restore 30 Hz and still preserves
real overload. Reference instruction-clock execution remains unchanged.

The first CPU-cost wrapper incorrectly combined the diagnostic option with Play,
which the runtime rejects (exit 3). Its ownership assertion masked that status;
a repaired wrapper recorded it. The corrected finite native-clock diagnostic
omits screenshot/memory captures. These failed requests are retained, not passes.


The recovery-only ordinary Play replay improves the four measured windows from
19.8/19.8/22.1/19.6 to 27.4/26.9/23.9/25.2 scenes/s, with zero audio underruns.
This is a partial improvement, not sustained-30 completion. Source diagnostics
also observe roughly 4,600-6,000 full CPU/device probes per update because native
elapsed time is sampled every 128 ordinary guest instructions. A controlled
candidate tests a 512-instruction quantum. Per-instruction IO revision, exception,
interrupt and deadline checks remain; no modeled clock or guest timestep is
changed. Reference-mode and both sampling-boundary policies need qualification.


The clean recovery/particle runtime with the existing 128-instruction quantum
measures 29.47/29.78/29.83/27.31 scenes/s across the same four windows. The 512
experiment measures 29.38/23.49/24.78/28.01; it does not establish a benefit and
is rejected. Host-load/route variability limits attribution. Restore 128 for
delivery; retain the boundary tests and failed-hypothesis evidence. Do not claim
that reducing clock polling fixed combat. Real overload and full-game live
acceptance remain open, even though removing the sticky recovery helps.


## Retained correction and final qualification (2026-10-13 UTC)

Native elapsed play now primes only the 20-Hz recovery counter before the
supported US viFrameSync function. The original function still measures queued
VIs, rejects late work, waits for its deadline and returns real elapsed time.
The 31-consecutive-fast-frame hold was a demonstrated contributor to prolonged
20-Hz episodes. This does not force 30 Hz or accelerate the simulation clock.
The original instruction-clock reference path is unchanged. Genuine overload
remains visible. The 512-instruction sampling experiment was rejected; 128 stays.

Final runtime `14746717614559b93fa535f9caf0a02fce50f23d2df3ab23aa6e9302607f30aa`. Ordinary muted Play at Custom 120/VSync:

| VI window | Before SCENES/s | Final SCENES/s | Final OUTPUT/s |
|---|---:|---:|---:|
| 4000-4500 | 19.83 | 26.79 | 117.92 |
| 4500-4800 | 19.79 | 25.18 | 117.32 |
| 4800-5400 | 22.14 | 23.98 | 112.34 |
| 5400-5800 | 19.62 | 24.33 | 111.61 |

These bounded copied-checkpoint results are not a sustained-30 guarantee or
full-combat acceptance. Different native timings can affect an open-loop fight,
and host-load variation was significant. Nine focused C++ suites pass. Final
Original guest/renderer/save state matches exactly, native timing begins after
an identical reference prefix, and a deliberate 250-ms overload remains visible.
Pause/resume is qualified. Normal final audio underruns (120/360): 8/62.

The first recovery test build stopped on narrowing warnings; explicit bounded
casts corrected the test. A particle build stopped on local shadowing and was
corrected. A restored-source executable hash assertion also stopped a request;
no binary identity was assumed afterwards: all final replays identify the actual
rebuilt artifact. These informative failures remain in the private manifest.

Transferable lesson: distinguish mandatory elapsed-time handling from historical
console recovery hysteresis. Raising OUTPUT cannot compensate for delayed scene
production, and dropping polling frequency is not a demonstrated optimization.
**Status remains critical/open, partial correction qualified; owner live review
and further genuine host-cost work remain.** No public release.
See [qualification](../../../evidence/combat-particles.json) and
[particle issue](pickup-particle-desync.md). Large private evidence stays on D:.


### Performance acceptance remains open

The final combat runs record **8 audio underruns at 120 and 62 at a 360 target**.
These are observed failures of sustained smooth playback, not passing performance
acceptance. The earlier recovery runs had zero; the initial old preview had five.
A separate modern-controls-camera-collision runtime was observed active at
10:46-10:47 UTC. Earlier overlap was not fully recorded, so its contribution to
all timing samples is unproven. No owner process was stopped. A controlled
single-runtime comparison remains necessary before attributing the late audio
difference to this change or claiming sustained 30/360 performance.

Functional/state/pixel checks pass, while combat remains critical/open and this
preview is a partial correction. Preserve both successful and failed measurements.


## Delivery held for performance verification

The implementation is ready for review, but the desktop shortcut and installed
visibility preview remain unchanged. Final combat has real drops and 8/62 audio
underruns at 120/360; late concurrent-runtime load prevents clean attribution.
The post-pause trace has 988 samples, no repeats/rewinds, and three skipped phase
steps. Functional/state/pixel verification does not establish smooth performance.
Do not replace the working preview or call combat fixed until a controlled
single-runtime comparison resolves this. Both candidate sources and all evidence
are preserved. The next bounded run should first coordinate exclusive runtime
testing, compare unchanged/candidate builds on the same copied checkpoint, then
profile remaining host execution/renderer costs if genuine drops persist.

Implementation commit: 83cd8af0bad951d27939adadb74f374840d1303d. The committed build retains
qualified runtime 14746717614559b93fa535f9caf0a02fce50f23d2df3ab23aa6e9302607f30aa.
A copy is retained in the private D: evidence directory. Desktop delivery is
held pending the controlled performance comparison above; no release was made.

## Three-hour combat continuation (2026-10-13 11:47 UTC)

Owner approved three hours to fix combat drops. Same production investigation,
root and accumulated charges retained, guard enabled. Candidate 83cd8af remains
held for performance acceptance. First compare installed preview and candidate
with no other JFG runtime, monitoring process overlap throughout. Large evidence:
D:/JFG-private/high-refresh-20261011/combat-isolated-20261013. Tests muted, hidden,
1600x900, copied Goldwood save and identical input/settings. This records work in
progress, not a completed fix or delivered result.

First isolated runs: installed preview 24.74/29.27/29.84/25.01 and
27.97/26.66/29.86/27.43 SCENES/s; held candidate 29.77/29.46/29.71/29.63.
All had zero audio underruns. The fourth comparison overlaps a separate
modern-controls-camera-clearance runtime and is excluded from performance
acceptance. No foreign process was stopped. Dependent requests stopped on the
failed isolation prerequisite; no source import or build ran. Resume source
integration using the three valid runs, repeat performance after isolation.

Ported CPU eligibility caching, bounded native instruction spans and exact
clock transcript support from develop 9a6e6781975610ef59811f29355eba01d2d07d19. All original guest
operations and 128-instruction clock safepoints remain. This is a candidate;
the new integration still requires differential tests and replay qualification.
The import script assumed a blank line after the final CMake test target and
stopped after writing source; a follow-up added the target using its actual EOF.
No partial import was built. Dependent builds correctly stopped. Entry-offset
lookup metadata remains a separately measured optimization to consider.

The full CPU rebuild was canceled before its resource limit. A frozen copy of
the prior completed archive supplied 2,909 unchanged generated bodies, after
matching generated source/header text, recorded compiler flags/toolchain and
dependency ages. Wrappers/runtime use this checkout; normal object-shape and
link audits stay enabled. First cache scripts stopped on a concurrently changed
project, path-comparison error, replaced wrapper log, then an MSBuild output-log
filename variation. No failed cache request counted as validation. The repaired
build and two focused timing suites pass; game/performance validation follows.

## Native CPU candidate qualification

The imported cache/span optimization removes repeated host bookkeeping around
ordinary generated instructions. Groups contain at most 16 safe operations;
dynamic RAM checks, branches, device accesses and observers retain the checked
fallback. All guest operations and the 128-instruction clock sampling quantum
remain. The 31-fast-frame recovery correction and particle identities are kept.
This is a CPU optimization, not a forced game timestep or changed FPS counter.

Ten focused C++ suites pass, as do six generator tests including 4,096 compiled
RAM/MMIO pairs. Original guest/renderer/save state matches the reference exactly.
Native transcript replay matches 2,606,049 clock observations and the final
guest/renderer/save state exactly, with 145,817,191 instructions using spans.
Those transcript runs are not performance evidence; recording adds overhead
and the replay deliberately follows recorded time rather than wall time.
Pause/resume preserves the clock boundary, and injected 250-ms overload remains
visible. The post-pause trace has 1,000 samples, no repeats, backward phases or
uneven phase steps. These are bounded software checks, not monitor scanout.

The MSVC generator-test fixture initially failed Windows batch quoting after
its source edit; the repaired invocation ran all six tests without skips.
An initial performance preflight treated idle MSBuild node-reuse helpers as
active load. Its owned waiting process was canceled before any benchmark.
The replacement blocks actual compiler/linker/game processes and retains all
guard charges. A read-only job-accounting query reports the current innermost
Windows job; it is not proof of the supervisor's aggregate remaining allowance.

**Candidate functionally qualified; isolated performance, delivery and owner
live acceptance remain pending.** A separate controls task is running replays;
no foreign process was stopped. See [candidate evidence](../../../evidence/combat-native-spans.json).
Keep the production guard enabled and retain the same investigation/root and
counters on handoff. Large private artifacts stay on D:.

Build compatibility: native preview builds now require a generated span root;
ordinary/reference builds retain their default behavior. After fresh private
normalization and original timing setup, call
scripts.original_timing_generation.finish_native_spans(root) once before
building with JFG_EXPERIMENTAL_NATIVE_PLAY_TIMING=ON. Never apply it twice or
copy a header over stale generated bodies. Frozen-object reuse here separately
verified all 2,909 body sources, their included headers, compiler flags/toolchain
and dependency ages; fresh wrappers/support and normal object audits were kept.
Transferable lesson: separate game-state/clock equivalence from performance,
and never compare frame rates while another game or compiler is competing.

## Controlled combat qualification and local delivery

| Run | SCENES/s by combat window | OUTPUT/s by combat window | Audio underruns | Median update wall time excluding clock wait |
|---|---|---|---:|---:|
| prespan-120-a | 29.83 / 29.78 / 29.84 / 29.77 | 119.67 / 119.94 / 119.88 / 119.68 | 0 | 22.522 ms |
| spans-120-clean-a | 29.82 / 29.79 / 29.83 / 29.75 | 119.76 / 119.95 / 119.83 / 119.60 | 0 | 21.945 ms |
| spans-120-clean-b | 29.83 / 29.79 / 29.84 / 29.80 | 119.91 / 119.95 / 119.87 / 119.65 | 0 | 21.442 ms |
| spans-360-clean | 29.80 / 29.73 / 29.81 / 29.68 | 358.13 / 359.55 / 355.88 / 352.77 | 0 | 22.058 ms |

Controlled performance qualification completed after the competing controls
tests ended. Two clean 120/VSync runs and one 360/no-VSync run each maintained
29.68-29.84 SCENES/s across the measured combat windows, with zero audio
underruns. Output was 119.60-119.95 at the 120 target and 352.77-359.55 at the
360 target. One 3-VI update occurred in the first 120 run and one in the 360
run; all other measured updates used 2 VIs. No sustained 20-25-Hz episode
recurred. This qualifies bounded local combat cadence, not perfect pacing,
physical scanout, all computers, every encounter or sustained exact 360 FPS.

The original installed preview's valid comparison windows ranged from 24.74
to 29.86 SCENES/s. The recovery-only build already held about 29.8 in its clean
repeat. Native span batching reduced median update wall time excluding explicit
clock wait from 22.522 ms there to 21.945 / 21.442 ms in the two 120 runs.
That is modest observed headroom, not a claim that batching alone removed the
20-Hz hold. These times also include renderer/OS work; open-loop fights can
diverge with native timings. The demonstrated recovery hysteresis fix is the
main explanation for avoiding prolonged 20-Hz episodes.

The first new-CPU comparison overlapped another controls replay and was excluded,
even though its audio counter was zero. Its process evidence and result remain.
Two-minute quiet preflight and continuous JFG/compiler/linker process sampling
qualified the final three runs; this is not whole-system load control.

Delivered locally as high-refresh-combat-spans-20261013 on D: through **JFG - High Refresh Preview**.
Implementation a93faa5ef61b86536327ffa83d48fc95835a21ff; runtime e7e3af29fd9aeec9e037da93a27d0e50272ba7bbb6b4ac77eb5f6536dd2d6fc5.
All 14 manifest files and isolated muted launcher initialization verify.
The existing owner save/settings and shortcut arguments are preserved. Tests
were muted and used copied saves. The earlier pickup-particle correction is
included. No public release. **Owner live verification and broader combat/game
coverage remain open.** See [qualification](../../../evidence/combat-native-spans.json).

## Owner tutorial capture after delivery

The 2026-10-13 manual log-only Custom 120/VSync run still contains genuine scene
cadence drops: 20.16 submissions/s over elapsed 7:15-7:35, and 21.61 then 24.85
over 10:45-10:50 and 10:50-10:55. Output averages 110.05, 116.45 and 114.85
respectively. The owner reports jitter on crates, an exit light and a bridge
in the later cave/bridge area; exact room-to-log timing is unavailable.
The qualified short combat comparisons are not withdrawn, but they do not
generalize to this route. No new cause or remedy has been established.
See [RENDER-006](tutorial-late-object-jitter.md) for evidence, limits and the
unchanged production guard state. This review ran no new gameplay experiments.

The separate entry-offset optimization is now imported from 9a6e6781975610ef59811f29355eba01d2d07d19.
The controls investigation measured 666,854 lookup probes and 2.661 ms per
combat update before its remedy, falling to 57 probes and 0.0028 ms there.
Those are prior measurements, not new high-refresh performance results. Here,
three dispatch paths now use offsets published with the same function pointer
and overlay lifetime, instead of scanning preceding instructions. Fresh private
normalization and unchanged game-body verification precede build acceptance.

## Controls investigation history imported from develop

The following measurements belong to the separate controls investigation and
its documented fixtures; they are not qualification of the combined build.

### Instruction-call overhead and GPU measurement audit

The first scheduling revision completed both Goldwood tests. The simpler fixture
held roughly 33.3 ms per frame body, but the matched fight still had a 50.01 ms
median / 54.91 ms p95 update interval (reference 67.79 / 98.69 ms). About 13.0% of
the measured combat wall-time window was spent inside graphics submission; its
full-sync portion was about 6.5%. Those nested values must not be added together.

An opt-in CPU-cost trace then measured 56.74% of guest instructions entering an
out-of-line runtime callback, while only 0.80% entered the full clock/device
handler. Most of those calls therefore performed ordinary instruction bookkeeping.
These are instruction fractions, not fractions of runtime. Whole-process CPU
time includes renderer/helper threads and is quantized; use aggregate values.

The instrumented run itself had a 33.87 ms median / 50.54 ms p95 interval. This
variation from the preceding uninstrumented run means a single favorable replay
cannot prove a performance fix. Compare independent runs with matched initial
state, and compare uninstrumented behavior with instrumented behavior.

The ordinary RT64 workload-profiler snapshot returned no available samples.
Missing samples are not zero GPU cost. The synchronous framebuffer render-to-RAM
path has a separate graphics worker; the regular workload profiler is insufficient
to characterize it. Direct measurement must put timestamps on that worker and
resolve them after its existing fence, without adding another GPU wait.
CPU full-sync timing includes submission, synchronization and readback and is
not interchangeable with GPU execution duration.

The next native candidate inlines the compact bookkeeping path in large generated
functions as well as small ones. It retains the same bounded host-clock sampling,
hardware revision, interrupt, exception, observer and instruction-budget checks.
It does not omit guest instructions or actor work. The original cache/Count model
and its selector remain available for reference comparison.

The new path passed 65,536 differential native transitions alongside the 250,000
reference transitions. A warnings-as-errors failure for a shadowed variable in
the new test was corrected before the runtime rebuild was admitted. The generated
support-header and previous executable hashes were archived; game code bodies
were not edited.

All compared combat prefixes equal: **True**.

- native-event-batch-boundary: median 50.01 ms, p95 54.91 ms, 335 updates over 40 ms / 430 samples.

- native-cpu-gpu-cost-boundary: median 33.87 ms, p95 50.54 ms, 206 updates over 40 ms / 497 samples.

- native-all-inline-boundary: median 50.13 ms, p95 54.16 ms, 398 updates over 40 ms / 399 samples.

- native-all-inline-cost-boundary: median 50.14 ms, p95 54.02 ms, 395 updates over 40 ms / 402 samples.

Reference full-replay RDRAM SHA-256: `757b7094a9d6fabd671e4c90ba7d50e54a52741d3e7ff3ffb57df3f062b93d99`.

The critical issue remains open; installed preview unchanged. Pause/stall, audio, startup/cutscene, movement, Save/reload and owner live verification remain part of promotion acceptance.


### Scheduling phase handoff and bridge comparison

The all-function inline candidate reduced out-of-line instruction callbacks from 56.74% to 0.924%, but both new combat replays remained near 50.1 ms per update (about 20 Hz). It did not solve the remaining slowdown. Reduced callback count is not a demonstrated performance improvement. Preserve the result and measure remaining handler, renderer and execution costs before choosing the next remedy.

Hidden runs so far used a 640x480 surface. The owner recording ended at 1164x873; subsequent GPU comparisons must match that size. Hidden rendering still does not qualify visible presentation/VSync latency.
The late-bridge comparison did not complete; preserve the failed/skipped command outcomes and do not claim acceptance.

The installed preview hash remains `059328317aeedccf1342156b884a6b8221573acd7f669636744649d8669f3cdd`. The native timing policy is diagnostic and default off. The next authorized phase measures the synchronous GPU framebuffer queue and validates pause/resume and host stalls before any promotion. The critical issue remains open.


### Hidden pause fixture correction

The first hidden pause attempt received no acknowledgement: the ordinary non-play probe does not service frontend window commands. This is a harness limitation, not evidence of a broken interactive pause. The existing hidden real-time replay flag was restricted to the older cooperative runtime. Its validation now also admits the unified guest OS when that runtime owns pacing; the shared window service returns before the cooperative frame pacer, preventing double pacing. The corrected bounded fixture remains hidden and uses copied saves, without foreground or physical input changes. Acceptance awaits the corrected run.

The reference bridge replay reached its requested ending, then failed at `guest-capture/final-image`; the paired comparison was skipped. This diagnostic capture failure is retained and not counted as gameplay/timing acceptance.


### Direct GPU measurements and timing qualification

The direct synchronous framebuffer GPU measurement returned valid timestamps:
1,195 submissions over 19.98 seconds occupied 42.37 ms of GPU execution in total
(0.21% of wall time), averaging 0.0355 ms per submission. The remaining combat
update interval was 50.14 ms median / 54.23 ms p95. This rules out that measured
GPU command interval as the dominant cost; it does not measure the separate
regular workload/presentation queue. CPU graphics submission, including its
existing waits, remained about 5.59 ms per update. The native clock/device handler
prefix contributed about 2.68 ms per update after excluding measured waits and
graphics submission. These nested measurements must not be added indiscriminately.

The corrected hidden real-time pause fixture held the guest clock at the same VI
for a 1.263-second frontend pause (four Count ticks of measurement separation),
then resumed at 30 updates in the first second. Held aim survived firing five
shots; saving modified only the copied fixture. Reload is checked separately.

Injected 250 ms and 2,000 ms host stalls exposed overdue video-interrupt delivery
in a burst: respectively 74 and 179 VI events in the following second, with many
within the first millisecond. Gameplay completed without a fatal error, but this
is not acceptable missed-deadline qualification. Native timing remains default
off until the overdue-event policy is corrected and rechecked.

The first capture-size tests stopped before gameplay because Windows produced a
1160x863 client rectangle from an estimated 1164x873 request. The harness now
corrects dimensions using the measured owned hidden window rectangle, without
showing/focusing it. Failed cases remain archived; only a confirmed final surface
size will count as a size-matched GPU comparison.

Source inspection found three dispatch paths that scan every preceding instruction
address to identify a generated relocation's alternate entry point. Per-call
lookup cost is being measured before selecting a remedy. Generated lookup tables
change at overlay lifecycle boundaries, so any cached resolution must preserve
publication, rebasing and invalidation semantics; a blind address cache is unsafe.


### Generated entry lookup cost and remedy

Diagnostic-only measurements found a median 666,854 table probes across 54
relocation-entry resolutions per combat update, costing 2.661 ms (5.3% of the
20-second measured wall-time window). Each dispatch searched every preceding
instruction address to discover whether its target was an alternate entry into
a generated function. The generator already knows that identity.

The candidate now emits each continuation offset with its function-table entry.
That offset is published, rebased, invalidated and collision-selected together
with the corresponding function pointer. Dispatch retrieves the metadata instead
of searching prior addresses. This removes repeated work without changing guest
code, instruction costs, timers or actor simulation. It is one measured overhead,
not a complete explanation of the remaining 50 ms update interval.

The compiled synthetic test exercises normal and alternate entry execution,
1,000 rebase/unload cycles, exact address collisions, misses and reload. It and
the 17 existing generator tests passed. The stale-normalizer build check first
rejected an incomplete incremental support update. Full regeneration then used
the original qualified recompiler header; 5,821 game and shared-header files
were verified byte identical before support/inventory synchronization. The
normalizer revision check remains enabled. Matched performance and full reference
RAM parity checks are separate acceptance requirements.

The pause Save reload also passed: all 250 observed post-load weapon states
retained the saved 95-round count; the source save remained unchanged.

The second surface-size attempt was still one pixel short because the inspecting
process used a different DPI-awareness context from the owned runtime window.
The revised fixture uses that window's DPI context for both sizing and
measurement. These failed size checks did not count as GPU evidence.


### Entry metadata validation, CPU samples and missed video deadlines

The entry metadata remedy passed the full 17,000-VI reference replay with the
same final 4 MiB RAM hash as the earlier reference:
`757b7094a9d6fabd671e4c90ba7d50e54a52741d3e7ff3ffb57df3f062b93d99`.
Candidate executable: `266aa9d0b459e80b36c73bc550877fd2b304490134d146c5cb044b9d03b5e67f`.
Median resolution cost fell from 2.6605 ms / 666,854 probes per update to
0.0028 ms / 57 probes. The combat update median still measured 49.991 ms
(p95 51.308 ms); this is a verified local optimization, not resolution of PERF-001.
Both comparisons began with the same RAM hash at the clock switch. Subsequent
native-time trajectories may differ, so later work totals are not identical inputs.

The generator's object-shape audit initially rejected two exported functions in
one support object. The compatibility lookup wrapper now occupies its own
support source/object; the entry table has one exported metadata accessor.
The audit accepts only the specifically named new accessor in addition to the
two existing lookup roles, while retaining the one-function-per-object rule.
Synthetic tests cover both old and new inventories and reject unrelated extras.
The stale-normalizer, header-lineage, line-ending and object-shape failures are
retained as evidence; no qualification check was bypassed to obtain a build.

The corrected DPI fixture confirmed a 1164x873 hidden client surface and final
capture. Native synchronous framebuffer GPU work occupied 44.91 ms over 19.969 s
(0.225% of wall time; 0.0351 ms/submission). Reference was 27.95 ms over 19.949 s
(0.140%; 0.0348 ms/submission). These measure that command list only. Regular
rendering/presentation GPU cost and visible VSync latency remain separate.

Owned-process CPU sampling collected 1,089 passes over 18 seconds without a
failed sample. On the hottest game thread, 248 of 298 samples attributed to
generated source landed on instruction timing hooks (83.2%). The thread used
6.531 s of user CPU and 0.344 s of kernel CPU. Waiting samples are not CPU cost;
optimized PDB attribution and sampling cadence can bias source attribution.
This is evidence to investigate per-instruction bookkeeping, not proof that
83% of whole-process time is spent there. GPU offloading is not indicated by
these CPU bookkeeping measurements.

The next candidate caches the metadata-independent eligibility predicate at
the full timing handler. Every instruction still checks live CPU Status,
hardware revision, bounded poll budget, FPU availability and metadata boundaries.
CPU register writes and ERET explicitly invalidate it; instruction-limit
remaining allowance caps the next poll budget. It keeps instruction/branch
accounting and the reference cache model, and never skips game logic. Differential
tests, full reference parity and matched combat measurements are required before
calling it a performance improvement.

Native periodic VI scheduling now latches one pending video interrupt and
advances its next deadline over all elapsed edges while preserving phase. It
does not advance Count, replay missed game updates or change reference timing.
VI_CURRENT uses bounded periodic phase even before an overdue event is serviced.
88,126 unit checks passed, including overflow, phase, missed periods and the
existing equal-time event-batch checks. These rules also avoid a burst of old
video messages after a future higher-render-rate build stalls; they do not
implement 60 Hz game simulation or interpolate animation/camera poses.

Private evidence: `native-coalesced-stall-250`; verification result remains in the guarded command spool.

Private evidence: `native-coalesced-stall-2000`; verification result remains in the guarded command spool.

Both injected stalls passed: 250 ms resumed with 60 video events in the next
second; 2,000 ms resumed with 61, compared with 74 and 179 before the fix.
Each had one video event within the first 2 ms. Count continued to track actual
elapsed time to within 0.002 ms of the logged wall interval. The 2-second fixture
is allowed to underflow audio while the entire host thread is deliberately
stopped; this is not a claim of uninterrupted audio through a frozen process.

All fixtures used copied profiles/saves and hidden owned windows. The installed
preview remains unchanged and native timing remains diagnostic/default off.
PERF-001 remains CRITICAL OPEN; final controls, startup/audio, pause/save,
stall regressions and owner live verification are still required for promotion.


### Cached eligibility qualification and rejected Count-store experiment

The cached eligibility candidate built and passed 250,000 reference and 65,536
native transition checks, including Status/I/O invalidation, FPU and metadata
boundaries, bounded clock sampling and instruction-limit refusal. Its full
17,000-VI reference replay still produced RAM SHA-256
`757b7094a9d6fabd671e4c90ba7d50e54a52741d3e7ff3ffb57df3f062b93d99`.
Runtime SHA-256: `2e52d24adc2e5f5707b06d13e66059e757bc20dd97ea71bb7172f9420c478fcb`.

This change did **not** resolve the remaining combat slowdown. The unprofiled
matched-boundary run measured 50.115 ms median / 54.590 ms p95, with 382 of 408
updates exceeding 40 ms. The instrumented sibling measured 49.094 / 53.829 ms.
Both began with the qualified identical combat prefix. Two- and three-VI update
intervals remain; later native-time AI/input trajectories can diverge.

Two Clang synthetic benchmarks had substantially different results: a 32-hook
same-translation-unit body improved about 1%, while a 1,024-hook body with state
and fallbacks compiled separately improved about 73.5%. Both body size and
compilation boundaries changed, and the actual runtime uses MSVC. Those figures
are not gameplay speedup evidence. The actual replay result takes precedence.

A separate, isolated MSVC /O2 experiment compared the current native helper with
removing its redundant Count mirror write. Across eight alternating pairs of
64 million synthetic hooks, medians were 227.563 ms and 229.964 ms (candidate
1.01 times baseline). There was no demonstrated benefit; the runtime source
retains that write. Do not convert a plausible optimization into an accepted fix
without relevant measurements.

The 200-second cold-start/title/attract fixtures completed under both clocks.
Both produced nonzero 22,018 Hz PCM, no recorded host-audio underruns or empty
queue pauses, and preserved the source save. Native PCM duration was 200.446 s;
reference was 200.463 s. This verifies delivery/content presence, not audible
quality or visually judged cutscene synchronization.

Next diagnostics distinguish the generated-dispatch prefix, repeated overlay
inventory validation and regular GPU workload from the previously measured
synchronous framebuffer GPU queue. Regular-workload observations read RT64's
existing timestamp results after its existing fence on the owning worker;
a synchronized snapshot avoids racing its profiler. No extra GPU wait or guest
workload is introduced. Prefix measurements are nested and must not be summed
as independent costs. The separately recorded final-image probe failure is
[DIAG-001](final-probe-frame-capture.md).

PERF-001 remains **CRITICAL OPEN**. The installed preview is unchanged and the
native timing policy remains diagnostic/default off. Hidden qualification does
not replace the owner's reserved physical-input/focus and fight/bridge checks.


### Separate regular GPU and dispatch observations

Runtime `00c1dfaeae5d2f4e12242a69326c531df5edadb010127d5a461d9506a4318434`
measured both existing GPU command intervals. Over the matched 20-second combat
window, 981 synchronous-framebuffer submissions totaled 32.790 ms (0.164% of
wall time), and 650 regular workloads totaled 16.189 ms (0.081%). Mean command
intervals were 0.0334 and 0.0249 ms. These measurements do not include the wait
before a GPU's first marker, visible VSync or desktop presentation latency.
They provide no reason to move CPU bookkeeping to the GPU.

Generated-dispatch prefix timing measured 3.309 ms median per update across
16,756.5 calls. Overlay-inventory validation is nested within that prefix:
0.769 ms and 3,640 checks. Never add those nested measurements. Instrumentation
and host scheduling affect this run, which measured 64.64 ms median updates;
unprofiled repeats take precedence for gameplay-cost claims. The next unprofiled
run measured 45.860 ms median / 54.338 ms p95, with 256 of 470 updates above 40 ms.
The matched reference sibling measured 67.883 / 98.999 ms, with all 267 updates
over 60 ms. Later native/reference actor and RNG paths can differ. This remains
partial improvement, not a fixed 30 Hz combat update rate.

Private evidence: `native-remaining-costs-boundary-detailed-costs.json`,
`certified-combat-comparison.json`, and `certified-bridge-comparison.json`.
The final-image failure is independently verified under [DIAG-001](final-probe-frame-capture.md).

### Control verification corrections and bounded native span candidate

Initial qualification checks made two invalid assumptions: positive expected yaw
for rightward stick input (the implementation uses negative yaw), and that VI
messages delivered by an update's completion equal the movement time consumed
inside that update. Both clocks had the same turn sign. Completion can straddle
an asynchronous video interrupt, and elapsed video edges can be coalesced.
Preserve those failed checks as test defects, not evidence of changed speed.
The trace now records the actual tick argument consumed by `controlPlayer`.
Replacement checks fit displacement and turn angle against accumulated movement
ticks, while separately reporting wall-time and delivered-VI rates. Save/reload
qualification previously skipped because of the failed prerequisite; it must
actually run and pass before being claimed.

The next candidate consolidates native timing bookkeeping across at most sixteen
straight-line instructions. Every guest operation and every original timing hook
remain in generated source; the reference path uses its original hook sequence.
Native admission reserves the full span within the existing sampling quantum.
Branches, labels/alternate entries, CPU register operations, callbacks, observers,
CACHE and exception boundaries end a span. Each memory operation dynamically
checks the full-width address, cached RDRAM extent and alignment. MMIO, uncached
aliases or other addresses first commit the executed prefix, then use individual
hooks before the access, preserving trap PC/Count/branch metadata. Hardware I/O
revision changes originate in that guarded MMIO path, not asynchronous render
callbacks. FPU admission retains the live availability check.

This is an internal opt-in experiment, requiring both native timing and generated
span support; release generation and the preview do not enable it. Synthetic
helper and compiled RAM/MMIO fallback equivalence, original hooks/labels inventory,
full reference RAM parity and same-binary alternating combat runs are required.
Building it is not proof of a performance gain. No actor, collision, animation,
movement or camera work is skipped. Future 60+ Hz rendering still needs separate
simulation/interpolation work; bounded instruction safepoints remain required.


### Conditional native-span qualification (2026-10-11)

Candidate runtime `43e65779b7cbd5da3dbcb5d61f392b57d0675dcd69165c473d08bb63f2f65b2b`
passed the complete 17,000-VI reference replay with unchanged final RAM
`757b7094a9d6fabd671e4c90ba7d50e54a52741d3e7ff3ffb57df3f062b93d99`.
Generated spans retained all 424,827 original hooks and existing labels;
46,829 spans grouped 237,742 instructions with 110,267 dynamic memory guards.

Same-executable ABBA combat measurements, over 20 seconds after the identical
qualified initial state:

| Spans | Updates | Median ms | p95 ms | Updates over 40 ms |
| --- | ---: | ---: | ---: | ---: |
| Off | 466 | 49.427 | 53.759 | 267 |
| On | 557 | 33.585 | 50.287 | 83 |
| On | 502 | 34.001 | 50.515 | 193 |
| Off | 454 | 49.825 | 53.094 | 289 |

The optimization reduces repeated host bookkeeping; no game work is skipped.
About 56.6% of executed combat instructions used admitted spans. Results remain
variable: the repeated enabled run still spent part of combat near 50 ms per
update. The median is not an average frame rate. Changed clock cadence can
alter later input sampling, AI and RNG; these are performance comparisons, not
proof of identical later gameplay state. PERF-001 remains **CRITICAL OPEN**.

A qualification instrumentation error was also caught: the first replacement
movement checks expected a fifteenth trace field that an earlier build did not
actually emit. The source edit had been appended to a private installer after
it ran. Those checks failed and dependent save/reload checks skipped. This
candidate actually includes the consumed movement-tick field; its replacement
checks must pass independently. Failed attempts remain in the private evidence.

A diagnostic clock transcript records exact native-clock observations keyed by
instruction ordinal, PC/opcode and idle deadline. Replaying the transcript with
spans enabled must consume every observation and produce identical final RAM,
Count, instruction total and save. Replay rejects mismatched keys, truncation,
backward time and unused tail records. Transcript replay skips host idle waits
and must never be reported as a performance measurement. The utility is opt-in
and restricted to finite recorded-input probes; it does not change play timing.

Private evidence: `span-qualified-combat-comparison.json`,
`span-qualified-reference-parity`, and the guarded qualification spool.
Installed preview unchanged; no live verification or release claimed.


The native transcript comparison passed: 8,589,363 clock observations were
consumed exactly, with identical final Count (10,200,870,393), instruction total
(11,629,409,591), save and RAM
`98e92f42129b1b7f6f8dd14b1b84280bfecf8814d94ef4efc0045b99153f2331`.
The optimized execution included 508,191,887 grouped instructions. This proves
that recorded sequence, not arbitrary gameplay or a performance claim.

The source audit checked all 115,571 eligible memory expressions against their
decoded base register and signed offset, with no mismatches. Its initial parser
flagged 84 zero-register forms incorrectly; the corrected audit and initial
report are both preserved. The next generator also rejects mismatched,
relocated, compound or unknown operands conservatively instead of assuming
its decoded address matches emitted C. Compiled fallback tests cover a base
register changing between accesses, including RAM-to-MMIO and MMIO-to-RAM.

Dual-stick/reference/native movement qualification and KBM aiming, frontend
pause, native Save and reload all passed. Movement remained 4.0625 world units
per consumed game tick in all four directions; half-stick yaw was -128/+128
angle units per tick. Firing retained aim, and the saved 95-round ammunition
state reloaded. A 1.262-second frontend pause advanced Count by only 0.0006 ms,
kept VI unchanged, and resumed with 29 updates in the next second. These hidden
fixtures do not replace the owner's physical-input/focus verification.

With actor logging disabled, the 20-second comparison completed 480 updates
without spans and 562 with spans; medians were 37.968 and 33.507 ms, and p95
remained 50.699 and 50.205 ms. The residual slowdown is not explained by actor
logging alone. Private evidence: `span-light-combat-comparison.json`,
`native-span-transcript-parity.json`, `native-span-memory-expression-audit.json`
and the `span-tick-*` fixtures.


### Contiguous native instruction candidate

The next candidate keeps admitted native operations together in generated code,
followed by the exact original fallback sequence. Unsafe memory access commits
the executed prefix and jumps to that instruction's original hook. This removes
per-instruction fast/fallback joins from the admitted block and lets the compiler
optimize consecutive operations without repeated opaque observer checks. Every
original operation, hook and alternate-entry label remains. No enemy, collision,
animation or input work is omitted.

The stricter generator proves each allowed memory expression has the decoded
base/offset and access width. Unknown expressions end a span. All 46,829 spans,
237,742 grouped instructions and 110,267 memory guards survived this stricter
classification. Six generator tests passed, including compiled RAM/MMIO
transitions and rejection of changed base, offset, width and relocation forms.
A quoting error in the newly added test initially stopped installation before
compilation. The failed command and skipped dependencies were retained; the
corrected tests passed before generation resumed under the same guard.

A default-off `JFG_EXPERIMENTAL_NATIVE_PLAY_TIMING` CMake option makes an explicit
private interactive preview use native elapsed timing and batching without
inherited environment flags. It requires the live runtime and a span-generated
root, identifies itself as `NativeTimingPreview`, and emits a support-log policy
marker. Finite reference probes keep original timing unless explicitly requested
otherwise. This engine policy applies to both control styles; it is not a new
controller setting or a released default.

This build must still pass full reference RAM parity, the existing exact-clock
transcript, same-binary alternating combat measurements, movement/aiming,
pause/Save/reload, bridge, audio and stall checks. An owned hidden interactive
startup must confirm activation without diagnostic clock environment variables.
Physical input/focus and visible/audible verification remain reserved for the
owner. A successful compile does not resolve PERF-001.

Candidate binary SHA-256: `251700d98118a137fdcbae8af50bdc4351c0276e9c4d0d1c525a65a18505f554`. Installed preview unchanged at build completion.


### Contiguous candidate qualification and handoff

Runtime SHA-256: `251700d98118a137fdcbae8af50bdc4351c0276e9c4d0d1c525a65a18505f554`.

The complete reference replay retains RAM SHA-256
`757b7094a9d6fabd671e4c90ba7d50e54a52741d3e7ff3ffb57df3f062b93d99`.
The previously recorded 8,589,363 native-clock observations replayed exactly
across the changed binary, ending with identical RAM, Count, instruction total
and save. This isolates code-generation semantics from host scheduling changes.

Same-binary alternating combat results (20-second qualified window):

| Spans | Updates | Median ms | p95 ms | Updates over 40 ms |
| --- | ---: | ---: | ---: | ---: |
| Off | 503 | 33.840 | 50.593 | 192 |
| On | 553 | 33.533 | 50.295 | 93 |
| Off | 476 | 46.798 | 50.626 | 247 |
| On | 594 | 33.440 | 37.513 | 12 |

The qualified bridge window measured 33.448 ms median / 33.865 ms p95 across 594 updates.
Its initial memory, Count and input poll match the previously qualified reference.
Later trajectories can diverge as cadence changes; the clock transcript supplies
the separate exact-state proof. Median update time is not average frame rate.

The interim analysis wrapper initially rejected a UTF-8 byte-order mark before
launching the second enabled replay. That failed command is retained. Reading
its source as UTF-8 with BOM support fixed the wrapper; the four completed runs
were executed off/on/off/on. No runtime replay failure was hidden or discarded.

Dual-stick movement/turning, KBM aiming/fire, frontend pause, native Save and
reload passed. The new binary preserved movement per consumed game tick and
reloaded the 95-round ammunition state. Source saves stayed unchanged.
The frontend pause lasted 1.260 seconds, advanced Count by 0.000085 ms,
and resumed with 30 updates in the next second.
A deliberate two-second host stall resumed with 61 video events in the next second
and 1 within the first 2 ms, preserving the no-catch-up-burst contract.

The native startup/attract fixture delivered the expected nonzero PCM duration
without reported underruns or empty-queue pauses. Its reference comparison used
the previously qualified reference binary, explicitly recorded in the evidence.
Both produced 200.463 seconds of PCM at 22,018 Hz. The longest interval between
queue submissions was 384.694 ms for this candidate versus 192.072 ms for the
reference fixture; there were no recorded underruns or empty-queue pauses.
These counters do not establish audible quality or visible cutscene sync.
An owned hidden interactive window also started with the preview policy without
diagnostic timing environment variables and closed cleanly through its window.
No desktop focus, global mouse/keyboard input or user window was changed by these
checks. They do not verify audible quality, visible latency or physical devices.

**PERF-001 remains CRITICAL OPEN, with the candidate verified in the stated hidden
fixtures and owner live verification pending.** Remaining slow updates are not
a claimed stable 30 Hz result. This work does not provide 60+ Hz simulation or
animation/camera interpolation. It removes modeled CPU cost from native elapsed
timing and reduces host bookkeeping without skipping simulation work.

For future recomp implementations: keep hardware time separate from host
instruction throughput, retain bounded safepoints and forced hardware boundaries,
preserve phase while coalescing a pending periodic interrupt after a host stall,
and use an exact clock transcript for optimizer parity. Prove effective memory
operands before bypassing per-instruction observers; uncertain forms must fall
back. A faster binary or completed replay alone is not acceptance evidence.

Diagnostic limit: ordinary live captures retain inputs and timing traces, but
do not record the exact Count transcript. Native elapsed-time runs can therefore
diverge on replay even with identical input recordings. The exact transcript
utility remains restricted to finite offline probes; RAM alone is not a complete
CPU/scheduler snapshot. Future deterministic live replay needs an explicit,
bounded clock-recording design rather than assuming input records are sufficient.

Private evidence: `native-preview-qualification.json`,
`contiguous-combat-comparison.json`, `contiguous-native-span-transcript-parity.json`,
`contiguous-bridge-comparison.json`, and the linked qualification fixtures.

The local Modern Controls Preview now uses this verified runtime with the
existing v1.1 launcher. Its previous runtime is backed up, package file hashes
were verified, and profile/settings/save hashes are unchanged. No public release
or Git commit is claimed. The desktop preview and capture shortcuts retain their
existing targets. Owner acceptance remains the drone fight, bridge, physical
KBM/XInput and focus, and audible/visible timing checks.


### Remaining costs in the final candidate

The final candidate received a separate CPU sampling and detailed timing pass
from the same qualified combat boundary. Profiling suspends/resumes owned
process threads and adds counters; these timings locate remaining costs and
are **not** performance acceptance measurements. Parked-thread samples are not
CPU usage, and nested counters must not be added together.

| Instrumented host work | Median per update (ms) | p95 (ms) |
| --- | ---: | ---: |
| Generated dispatch prefix | 1.887 | 2.339 |
| Overlay inventory (inside dispatch prefix) | 0.502 | 0.679 |
| Native handler prefix | 2.671 | 3.302 |
| CPU graphics submission | 5.343 | 6.820 |
| Explicit idle waits | 6.874 | 20.514 |

| Existing GPU timestamp interval | Completed submissions | Total GPU ms | Fraction of measured wall interval |
| --- | ---: | ---: | ---: |
| Framebuffer work | 1704 | 58.956 | 0.295% |
| Regular graphics work | 1123 | 28.337 | 0.142% |

These GPU queries reuse completed timestamps and add no new GPU wait. They do
not measure visible presentation/VSync latency or queue delay before the first
GPU marker. Rendering already runs on the GPU; these measurements do not justify
moving game logic, input or scheduler bookkeeping to GPU compute.

The owned-thread sampler completed 1,085 passes with no failed samples. It
recorded 109 instruction-pointer samples in `memcmp`, but flat samples do not
identify its callers or establish that these comparisons caused slow updates.
Before optimizing that work, attribute call sites and distinguish required
code/overlay integrity checks from avoidable repetition. Do not remove checks
or cache guest memory without a complete invalidation/lifetime contract.

The remaining work is to locate the CPU execution and scheduling boundaries in
slow updates on the final candidate, while retaining the same correctness
contracts. Broader pure-operation span eligibility is one possible experiment,
but excluded helpers must first be proven free of hardware callbacks and other
observable effects. It is not an implemented fix or a measured explanation.
Do not trade deadline checks, collision tests, actors or animation updates for
a higher reported frame rate.

Private evidence: `contiguous-remaining-costs-detailed-costs.json` and
`contiguous-remaining-costs-cpu-sampling.json`. The latter retains source locations
and owned-thread CPU accounting for subsequent investigation.


### Owner live feedback after native preview

The owner reports very smooth gameplay on 2026-10-11, but intermittent crackle
in background music. This is encouraging gameplay feedback, not a blanket timing
acceptance. Track the audible problem as [AUDIO-001](intermittent-music-crackle.md).

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

## Expanded route: damage-correlated drops (2026-10-13)

The owner reports about a second of lag on damage. In capture 20261013-172104,
health drops near VI 3742-3912 (session 64-67 seconds) coincide with roughly
20-22 new scenes/s while output remains about 111-114/s. This is a real source
cadence reduction, not just counter text. The window includes dense combat and
expanded synchronous tracing; damage is correlated, not yet a proved cause.
The later light/bridge segment mostly has even presentation phases, so its
object matching failures must be considered separately. Initial menu/idle time
is excluded from active-route summaries; raw timestamps are retained.

The full owner route has not yet replayed faithfully using elapsed-VI input
scheduling; the replay remains in the first room. Hidden recorded precise-stick
fraction accumulation had a real focus-reset bug and now passes targeted input
tests, but that change alone did not restore route correspondence. Do not infer
a full-route fix from these partial replays.

## Damage comparison qualification (2026-10-13)

The final candidate corrects redundant completed framebuffer-write holds, and
preserves non-render-buffer game state in Original/Custom comparison. A bounded
Goldwood native Play fixture with 18 actual damage events maintains about
120 OUTPUT / 29.8 SCENES with both light and expanded logging, no audio underruns
and no diagnostic queue drops. Logging alone does not explain a slowdown in
that fixture. 360-cap output averages 350-355 with about 29.8 SCENES, not a
sustained 360. Deliberate pause/readback probes are reported separately.

The owner's 20-22 SCENES damage-correlated window remains real and unexplained
in the exact route. Replaying available inputs does not preserve the full
route under timing changes, so unmatched baseline/candidate routes cannot
prove a whole-route performance improvement. Keep PERF-001 open. See
[review findings](tutorial-late-object-jitter.md#review-qualification-2026-10-13)
and [aggregate qualification](../../../evidence/high-refresh-route-review.json).

The continuation audits pure relocation-address arithmetic inside native timing
spans. The two relocation macros read the host section table and have no device
callbacks; calls that can alter that table remain boundaries. Relocated memory
operands are still excluded unless their actual address is proved. The candidate
preserves every guest operation and the 128-instruction clock quantum. The new
eligibility regression fails before and passes after; compiled differential
cases exercise relocation-table changes between invocations alongside RAM/MMIO
and clock-boundary fallback. Runtime replay/performance qualification is pending.

The pure-arithmetic change preserves every replayed guest byte, save, instruction
count and logical matching decision, but live dense combat still reaches 24-27
SCENES/s. No combat resolution is claimed. A follow-up candidate validates the
actual signed relocated memory address before batching; it never substitutes the
unrelocated ROM immediate. Unknown expressions, MMIO and unaligned accesses keep
the precise fallback. The compiled differential test includes relocated loads
and stores, changing section tables, RAM/MMIO and all nearby poll boundaries.
Runtime qualification follows.

A second CPU candidate checks the existing pending-device certificate after
each normal 128-instruction Count sample. If no device deadline has arrived and
Status/I/O revision are unchanged, it avoids rewalking every inactive device.
The Count sample, instruction boundary, guest operation and deadline remain
identical. CP0/CACHE, idle/park sites, observers, device errors, flash mutations
and invalidated certificates keep the complete handler. This is a candidate
until exact recorded-clock state and native Play comparisons pass. A duplicated
certificate refresh left by integration is also removed.

Test review found that the first randomized relocated-memory fixture combined
its section stride and base-selection parity so relocated cached-RAM cases were
always misaligned. The test is corrected to exercise both admitted and rejected
relocated accesses, with explicit minimum hit counts for each and all 4,096
original/optimized architectural comparisons. The earlier eight-test pass alone
did not establish that admitted branch; this limitation is recorded explicitly.

A dense recorded-clock fixture now contains 1.50-1.70 million instructions per
update, with about 3.8-4.3 ms in interpolation preparation. A storage candidate
replaces ordered semantic-vertex lookup with full-key hashed lookup, avoids
allocation for repeated UV clones, and records corrupt owners in the existing
first pass. Identity equality, duplicate rejection, whole-character decisions,
cut handling and endpoint arithmetic remain unchanged. A private 180-frame,
7,200-sample differential harness compares every output field to the previous
algorithm; full real-scene replay and native performance qualification follow.

Review caught a NaN/culling edge case in the first-pass corruption audit: the old
second pass rejects even a unique NaN endpoint because its copy compares unequal.
A new regression removes that corrupt vertex in the following scene and requires
all remaining actor parts to hold. It fails before the correction and passes
afterward; the differential harness also includes this sequence. The initial
private benchmark launch failed to locate the compiler by bare filename; using
its explicit developer-environment path corrected setup before any game launch.

## Continuation outcome, 2026-10-13

The selected CPU changes admit proven relocation arithmetic and actual signed
relocated RAM operands into existing bounded instruction spans, and reuse an
unchanged pending-device certificate after the normal Count sample. The
128-instruction clock quantum, maximum 16-instruction span, device deadlines,
MMIO/alignment rejection, CP0/CACHE/observer paths and guest operations remain.
Quiet and dense recorded-clock comparisons preserve every guest byte, save and
guest instruction count.

Semantic history uses full-key hashed lookup and avoids duplicate UV-clone
allocation while preserving whole-actor corruption rejection. The dense
same-workload comparison reduces median history cost from 3.90/4.09 ms to
3.34/3.59 ms in the two busy windows. The NaN-then-culling regression and 4,096
compiled RAM/MMIO/relocation/clock-boundary comparisons pass. One generic test
invocation lacked a compiler on PATH and skipped that integration test; the
developer-environment rerun executes all eight tests without skips.

Native combat still has real scene-rate drops. Different native timing paths
do not prove an end-to-end FPS uplift, and the owner's damage-correlated
20-25 FPS full-route slowdown remains open. Unlimited's additional queue-window
delay is measured in PERF-002; its candidate was rejected for worse presentation.
It is not the sole cause of capped combat load. Final rates, audio and source identities are in
[aggregate qualification](../../../evidence/high-refresh-route-fix.json).

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

## Five-hour guarded continuation, 2026-10-14 UTC

Owner approved five hours after the route-fix preview. The same operational
investigation/root and all prior charges are retained; the guard stays enabled.
Selected baseline is implementation aaae085 with documentation e51b0af and runtime
1b0bdd0f. Work prioritizes actual combat source-rate cost and a visibly qualified
late-room fixture. Preserve original game timing, muted owned tests, copied saves,
and raw evidence on D:. The scene-age, direct-camera and secondary subset
experiments remain rejected unless new evidence supports a distinct correction.
No bounded replay, synthetic match or build alone closes the owner's reports.

The new dense-stage observer preserves all guest bytes, instructions and matching
decisions. Semantic history is the largest measured preparation stage (1.51 ms
median), followed by geometry-key construction (0.81 ms). A candidate reuses the
continuity result for exactly equal UV-cloned vertices while retaining full-key
identity, conflicting/invalid actor-wide rejection and NaN/culling behavior.
The old algorithm is the differential oracle; real replay and native checks
remain required. This does not change game timing or count extra simulation.

The initial 2026-10-14 stage/alias renderer builds retained stale generated
layout code. Their visual/performance acceptance is invalid and is being rerun;
see [BUILD-001](stale-generated-renderer.md). Guest-state equality alone did not
detect this private build error. The delivered preview is unaffected.

A diagnostic-only replay-prefix release is being added to establish identical
combat starting state before real native timing measurements. At a complete
update boundary it fingerprints RAM, Count, input and instruction ordinals,
then releases the checked Count transcript and reanchors only the host clock.
It requires finite non-Play replay with an explicit boundary before the probe
end; normal replay still requires EOF. This is test infrastructure, not a
performance fix, and does not change the launcher or default gameplay policy.

Coherent renderer requalification preserves guest state and logical matching.
The alias reuse lowers dense history preparation by roughly 0.2-0.3 ms, while
busy updates still exceed 33 ms. The current semantic stage alone is 1.26 ms
median. A further candidate retains two contiguous endpoint tables, with full-key
collision checks and unchanged invalid-owner rejection, instead of allocating
individual hash nodes every scene. It must pass the prior implementation oracle,
real-state replay and native-prefix performance review before selection.

The flat-table candidate preserves the dense replay byte-for-byte and all
matching decisions. Dense semantic cost falls from about 1.26 to 0.97 ms.
One native-prefix comparison improves the busy window from 23.3 to 28.6 source
FPS; later paths differ, so this is encouraging rather than isolated causal
proof or a stable-30 guarantee. Detailed CPU profiling itself adds cost, but
its disabled control still reproduces the real slowdown.

A preparation candidate reserves mesh storage from actual per-matrix vertex
counts, and binds the F3DDKR context only for the owning synchronous interpreter
call. The latter replaces repeated global mutex/map lookup while keeping the
fallback registry for other states/threads and restoring the binding on return
or exception. Neither change alters geometry, identities or GPU resource waits.
State, rendering and native timing qualification follow.

A concrete CPU omission is found in the span generator: MUL_S and MUL_D expand
to plain multiplication in the pinned generated header, but were absent from
the pure-helper allowlist. Matrix loops therefore fragment around each multiply.
The candidate adds only those two macros; it keeps the original arithmetic,
FPU/Status checks, 16-instruction maximum, 128-instruction clock quantum, MMIO
fallback and all control/callback boundaries. The added eligibility regression
fails before and passes after. The compiled 4,096-case differential now also
compares float/double output bits and FP exceptions in four rounding modes,
including signed zero, subnormal, overflow and infinity inputs. Exact recorded
clock and native performance qualification remain required.

The multiplication-span candidate passes exact-clock dense replay with unchanged
guest bytes, save, instruction totals and logical matching. In a combined
comparison against the delivered 1b0bdd0f runtime, the busiest median update falls
from 37.872 to 35.626 ms and history preparation from 3.735 to 2.633 ms. The
earlier busy window changes only 32.915 to 32.730 ms. These are profiled,
identical-workload costs, not native FPS promises. Native released-prefix
measurements still include sub-30 source rates; the combat report remains open.

Another measured hypothesis concerns the generated function table: multiplying
an aligned address then masking only its low bits does not mix overlay/section
bits. A probe model and compiled original-versus-mixed benchmark use the actual
inventory while checking identical function/continuation resolution. This is
distinct from the already removed continuation metadata scan. No lookup change
is selected until semantic, lifecycle and native-game checks qualify it.

The actual-inventory compiled lookup benchmark preserves every queried function,
continuation and miss. Mixing the whole 32-bit address reduces lookup time by
about 33-51% in resident, sampled-hot-section and all-section models. This saves
nanoseconds per call and does not explain the full combat deficit. The generator
candidate retains full-address equality, first-published collision precedence,
lazy rebuilds and lifecycle invalidation. Existing compiled rebase/unload/reload
and generator checks pass; regenerated game bodies and headers are byte-exact.
Native replay qualification is still required before delivery.

A foreground process-status check at 03:01 UTC found a concurrent
Scooby-Doo diagnostic plus unrelated host activity. The owner was asked to pause
its game tests. The later lookup native comparison is not accepted as isolated
FPS evidence; earlier native runs without a full process-load monitor also
cannot rule out this contention. Exact recorded-state and Original reference
comparisons remain valid for correctness, not uncontended performance.

The older 512-instruction poll candidate remains rejected. A distinct private
experiment will compare 128 versus 256 only after releasing the identical
recorded combat prefix; ordinary Play and the entire prefix retain 128.
Repeated reversed-order runs with host-load monitoring are required before
considering a policy change. No default clock or game-speed change is selected.

## Monitored polling comparison, 2026-10-14 UTC

After the owner paused the other task and its diagnostic exited, four reversed-
order 128/256/256/128 runs began at the same recorded combat RAM/Count/input/
instruction boundary. No foreign game or compiler was observed by the two-second
monitor. Source windows ranged 29.21-29.92 updates/s with 128 and 29.52-29.91
with 256. The later native paths differ and the effect is small; these observations
do not justify changing the established clock policy. The private 256 probe is
not selected. Ordinary Play retains 128 and the existing 16-instruction spans.
Both earlier attempts that detected the open foreign process deferred without
launching a game. Evidence: combat-focus-20261014/poll-clear-abba-review.json,
its per-run host-load records and poll-decision.json.

A second monitored reversed-order comparison uses the pre-flat-table baseline
and the qualified CPU/matrix/lookup candidate, with identical recorded release
state in all four runs and no observed foreign game/build activity. The busiest
window is about 28.51 updates/s in both baseline runs; the candidate produces
29.62 in one run and 28.51 in the other. Early active work is slightly lower,
but changing later instruction workloads and run variability prevent a stable
30-FPS guarantee. Select the exact-state-preserving CPU reductions for measured
work savings; keep PERF-001 open. Evidence: cpu-clear-abba-review.json and its
host-load records. Neither the 256-instruction probe nor scene-age scheduling
experiment is included.

## Severe 360-FPS qualification hitch remains under review

The selected equal-slot candidate's later normal Play run completes and drains
its logs, but VI 4000-4800 averages only about 11 scene updates/s and 181-189
presentations/s, with 34 audio underruns. This is a failed performance check,
despite successful state/phase/completion checks. Earlier same-runtime dual-policy
captures were substantially smoother, so neither a new rendering regression nor
external host contention is ruled out. Do not deliver based on a generic
"completed successfully" assertion. The actual work/submit/history costs and a
monitored identical-prefix old/new comparison are required. Evidence:
shared-native-play-review.json and 360-slow-run-cost-review.json.

The subsequent monitored 360-FPS before/after/after/before comparison preserves
all four release RAM/Count/input/instruction/tape boundaries and observes no
foreign game/build activity. It does not reproduce the 11-FPS collapse or audio
underruns. Both versions still reach approximately 26-27 scene updates/s in
combat windows. Per-process CPU deltas are retained; changing native paths
prevent attributing the previous severe hitch or claiming a stable-30 remedy.
Evidence: shared360-abba-review.json and its four host-load records.

A static 32-versus-16-instruction span model reduces span count by only about
5.3%, with 0.11% more instructions covered. The sampled dense runtime averages
about 5.7 guest instructions per span commit. No 32-span runtime is selected
or benchmarked; the established 16-instruction bound and 128-instruction polling
quantum remain unchanged. Evidence: span-length-potential.json.

## Measuring host-thread transport separately

Function self time includes scheduling latency and cannot identify how much CPU
is spent switching guest stacks. An opt-in CPU-cost diagnostic now measures
baton publication-to-receipt latency on each side of a transfer; it excludes
execution of the intervening guest slice. Ordinary transport remains unmeasured.
Per-update counts/totals and the cumulative maximum accompany the existing
instruction/handler/graphics counters. This is observation only: the scheduler,
context-transfer rules, thread implementation and game timing are unchanged.
A transport optimization requires measured cost and exact-state qualification.


The measured baton wake totals are approximately 0.31-0.39 ms median per update
in four dense-route windows, about 1% of active update wall time. The larger
__osEnqueueAndYield self-time label includes other work and is not evidence that
thread switching causes the combat deficit. No scheduler replacement is selected.
A second same-clock capture disables the extra comparison histories and actor
trace while keeping cost counters; its RAM/save/instruction equality passes.
This separates ordinary history work from expensive diagnostic comparison passes.
These instrumented captures are cost observations, not uncontended FPS acceptance.
Evidence: transport-timing-review.json and single-policy-cost-review.json.


## Checked overlay-identity index candidate

The single-policy capture attributes about 0.58-0.66 ms per busy update to repeated
157-record overlay identity scans. A private byte-checked index passes 18,004
oracle checks (duplicates, changed identities, absent values, moved tables and
invalid extents) and reduces lookup time about 35-40% in reversed-order synthetic
workloads. It compares the complete validated inventory bytes on every call,
rebuilds on any difference, resolves hash collisions with the full identity and
rejects duplicates. Allocation bases are still read live; guest load/resume,
publication and lifecycle handling are unchanged. No unchecked lifetime cache
is introduced. This is a small CPU-work candidate, not a combat-FPS resolution.
Actual replay and native comparison are required before selection. Evidence:
overlay-inventory-cost-review.json and inventory-index-prototype/result.json.


The byte-checked overlay index passes the dense recorded combat replay and full
17,000-VI Original replay: RAM, save, complete instruction trace and reference
framebuffer agree. Profiled busy-update inventory work changes from 0.656 to
0.526 ms median, a small saving. The 120/360 native comparisons were interrupted
by foreign compiler activity and are not accepted as FPS qualification. Keep the
candidate separate from any stable-30 claim. Evidence: inventory-index-dense,
inventory-index-full-original-review.json, indexed-inventory-cost-review.json,
inventory-native120-review.json and inventory-native360-review.json.

A private compiler prototype added restricted aliases only inside admitted
16-instruction spans, leaving all hook and fallback boundaries intact. Its real
matrix leaf differential failed byte-exact RAM equality on synthetic edge cases.
It is rejected before changing the generator or rebuilding game functions;
normal generated code is unchanged. Preserve span-alias-prototype/decision.json
and mismatch-details.log. Compiler hints require state-level differential proof,
even when a pointer-separation argument appears sound.


The rejected restricted-alias leaf's first difference is a NaN representation
at RAM offset 0x32c: baseline 0xffc00000 versus candidate 0x7fc00001. CPU snapshot
equality alone would miss this memory-result difference. The prototype remains
unselected; the production generator has no restricted-alias change.

The owner authorized continuing under the existing unrelated host load. The
new inventory-loaded120/360 reversed-order runs therefore retain all process
observations instead of stopping when another compiler appears. They are loaded
host observations, not isolated performance acceptance or proof that a slowdown
is caused by this candidate. Exact release state is checked independently.


The corrected 360 FPS comparison verifies saved `fps=360` and VSync disabled.
Both runs share the exact release point; output averages roughly 354-359 FPS,
while combat scene windows still vary around 25-30 FPS. This single native pair
does not establish stable 30 FPS or a general gameplay speedup. The indexed
inventory remains a small CPU-cost reduction supported by exact replay, not a
resolution of PERF-001. Incorrectly labelled private runs are documented in
[frame diagnostics](frame-diagnostics.md); their raw evidence is retained.

A temporary full-sync stage probe passes the same dense exact-state checks and
is removed/rebuilt afterward. In the busy window, texture upload wait is about
0.34 ms per scene, GPU fence wait 0.70 ms, queue wait 0.006 ms, and history 2.68 ms.
Other renderer work is distributed across several stages. No dominant queue
wait was found, and synchronization is retained. Evidence: sync-stage-probe-normalized.


## Post-lifetime-correction native observations, 2026-10-15 UTC

The non-observer candidate completes the extended owner-input route at both
120/VSync and 360/no-VSync without crashes, invalid accesses or audio underruns.
Reviewed combat windows run near 119.9 output/29.8 scenes at 120, and 358-360
output/29.2-29.8 scenes at 360. Longer individual updates still occur. Four
minimal/expanded-capture observations do not reproduce a consistent large
logging penalty. These are current-build observations under the allowed host
load, not a controlled causal speedup or stable-30 guarantee. The captured
path-cache lifetime fix addresses memory corruption; PERF-001 remains open.
Evidence: overlay-cold-native120/360, overlay-lifetime-cadence-review and
capture-overhead-native-review. Preserve source-update intervals alongside
output averages; level/mode stability alone cannot identify damage events.


## Replay coverage and health-event qualification, 2026-10-15 UTC

A private transition-call observer resolves the long native replay gap: the
original mainRestartAfterDeath routine at 0x80046378 executes at VI 7155. Juno's
sampled health reaches zero at VI 6933 and resets at VI 7250; the largest update
interval is 1,139.504 ms across VIs 7182-7250. After entering Goldwood, this replay
stays in level 21. Therefore the extended native input replays do not reproduce
the owner's later-cave route. Earlier references to an extended owner-input
route describe replay duration/input provenance, not complete location coverage.
The observer is removed and normal source rebuilt before further qualification.

The owner's retained recording instead has approximately 104 ms update intervals
near nonfatal health losses in levels 21 and 16. A new 120-FPS native capture has
a largest interval of 48.616 ms in its sampled nonfatal-loss windows. These are
different paths and events, not a matched A/B or a damage-hitch resolution. Health
is sampled from playerBoy properties+6; actor initialization and lifetime changes
must be distinguished from damage. Windows overlap and are not independent trials.
The misleading hitPlayer symbol is a proximity query, not an actual damage hook.

Evidence: transition-call-native-review.json, recorded-player-health-v2-review.json
and health-window-cadence-review.json. PERF-001 remains open. Transferable lesson:
record actual location, transition identity and health state before attributing
a long update interval to combat or accepting a nominal full-length input replay.


## Shape-history storage candidate, 2026-10-15 UTC

Current captures attribute about 0.5 ms per scene to shape/rigid ownership and
history. A private dual-history comparison feeds identical meshes to the prior
ordered-map implementation and a contiguous probing table, alternating call
order. Across 1,337 combat scenes, the measured history call changes from
460.5 to 305.9 microseconds median (total cost ratio 0.664), with identical
matching decisions. This is a small CPU-cost saving, not game-FPS acceptance.

The candidate stores complete keys and rejects duplicate identities; hashes
only select lookup slots. Two tables retain storage between scenes. All 3,600
independent prior-implementation differential frames match, including an isolated
variant that forces every full hash to collide. Existing history cases and a
new growth/reuse/duplicate regression pass, as does C++17 header compatibility.
Production exact-state and native qualification remain pending; the desktop
package is unchanged. Evidence: shape-history-paired-cost, shape-history-oracle
and shape-history-candidate. No lifetime/camera/geometry gate is relaxed.


The first production shape-history build stops on a warnings-as-errors test
variable shadowing an existing local. Source is restored and rebuilt before the
replacement renames only that local; dependent replay requests are withdrawn
without execution. This is a test-source compile issue, not a running-game
regression. All failed logs and unchanged counters are retained.


The production shape-history candidate passes its recorded-clock dense replay:
all physical guest RAM and save bytes, 1,847 complete guest instruction/update
records and 17,715 interpolation decisions match the prior source. The full
17,000-VI Original comparison also preserves the renderer snapshot, physical
RAM, save, probe counters and all complete per-update instruction records against
the qualified cold-overlay baseline. Both measurement orders favor the new
lookup implementation. Normal 120/VSync and 360/no-VSync captures each complete
over 6,000 VIs without invalid accesses, audio underruns/overruns, or dropped
frame rows. These starting-area captures are not full owner-route traversal.

The approximately 0.15 ms median stage saving qualifies this small CPU-work
correction. It does not establish stable 30-Hz combat, solve the exact damage
hitch, or verify physical-display pacing. The source change is selected after
these checks; local package qualification follows. Evidence:
shape-history-dense-review, shape-history-full-original-review and
shape-history-candidate-native-review. Compatibility lesson: replacing a lookup
container must preserve full identity equality, duplicate rejection and scene
lifetime rules, including collision and storage-reuse behavior.


The local high-refresh preview and jitter-capture desktop links now include the
qualified shape-history storage change (80293adb), alongside the
prior overlay-lifetime and optional formatter fixes. Installed runtime:
e73bd0c1b6377d5bea0d941e6fa13854f8f56b54bea31a2a58b8f4a136d0f478. Both links retain arguments, and all 40 checked
profile files remain unchanged. The original package/links remain available.
The instruction-clock Original reference reports 2 audio underruns; it is a
state/readback comparison, not normal-play performance or audio acceptance.
The separate native 120/360 captures each report zero audio underruns.
Full-route damage/combat consistency and the remaining visual limitations stay
open. This is local delivery, not a public release or all-issues-fixed claim.


### 2026-10-15 finite-presentation queue investigation

The integrated finite-clock/scene-ownership candidate preserves both exact-state
references but increases measured scene queue waits at 360 FPS. One normal
native run has 49 audio underruns and 24.7-28.6 scenes/s windows. A shared-prefix
comparison and a private empty-event coalescing alternative are recorded in
[RENDER-003](steady-fps-jitter.md#queue-pressure-in-the-integrated-candidate).
The coalescing normal capture holds two-VI updates in the reviewed window, but
still has missing presentation scenes. Neither variant is delivered. Queue
latency, native combat consistency and damage timing remain open; a successful
exact-state replay does not establish performance acceptance.


The private bounded two-retrace finite-window test removes normal-run audio
underruns seen with the unbounded sequence handoff, but still has slow game
updates and repeated phases. The active-Present storage experiment is rejected
because it moves stalls to WorkloadQueue and increases measured latency.
Details and failed-attempt evidence are recorded in RENDER-003's bounded native
presentation window section. Both remain unselected; desktop package unchanged.


Owner acceptance clarified on 2026-10-15: eliminate combat drops to 20 source
FPS, retaining the original 30-FPS simulation target. A high interpolated
output count, or a 29.x window average containing 50-ms updates, does not meet
this target. The remaining slow updates are being compared by actual active
work, idle waits and time outside main update; no counter smoothing or game
clock acceleration is accepted as a substitute. Evidence: bounded-source-
critical-path-review on D:.


The bounded normal capture's 66 three-retrace updates have 34.24 ms median
active update work, compared with 26.88 ms for its two-retrace updates. Median
idle work is 12.44 ms in the slow group; the missed two-retrace completion
then waits for the third retrace. Ordinary wake overshoot is about 0.4 ms in
the slow group and submission-slot waits are negligible after the bounded
window change. This narrows the residual investigation to active work and
its scheduling, not a cosmetic FPS label. Active wall time can include host
preemption; CPU profiling is being considered alongside graphics preparation.
The source remains unselected and no stable-30 resolution is claimed.


### Parallel graphics-history prototype and rejected CPU leaf, 2026-10-15

A private native-count publication experiment removes two redundant native
bookkeeping stores in an isolated real generated matrix leaf. Its synthetic
byte-exact differential fails at case 21: RAM offset 0x130 changes a NaN payload
from 0x7fc00001 to 0xffc00000. It is rejected before changing any production
header/generated game code. Compiler scheduling can change exceptional float
representations even when the edited stores concern integer timing metadata.
Evidence: native-count-store-prototype/result.json on D:.

The user asked about multithreading. Rendering is already threaded; gameplay
shares ordered mutable guest state. A separate private experiment overlaps
semantic character/particle history with shape/camera geometry preparation.
The semantic worker owns a copied sample vector and its own history, and joins
before its output is consumed or Context can be destroyed. It uses the same
cadence gate and captures/merges the host floating-point environment. Gameplay
updates remain ordered. This initial std::async prototype measures potential;
it is not a selected per-frame thread architecture. Normal native observations
and a same-recorded-clock RAM/save/instruction/matching differential are kept
in parallel-semantic-review and parallel-semantic-dense-review. Source and
normal runtime are restored after capture; desktop package unchanged.


### Controlled parallel cost and entity-count question, 2026-10-15

The owner reports that slowdown increases with things on screen, asks about
multithreading, and requests more detailed profiling. Entity AI, collision,
animation and graphics can all scale together; visibility correlation alone
does not identify their relative contributions. Existing wall-time attribution
includes runtime handlers and synchronous graphics inside guest scheduler calls.
The no-20-FPS acceptance target remains unchanged.

Four identical recorded-clock/input captures in serial/parallel/parallel/serial
order preserve RAM, save, 1,847 complete update/instruction records and all
17,715 interpolation matching decisions. Busy history preparation medians are
2.477/2.725 ms serial and 1.756/1.799 ms parallel. This is a stage-cost saving,
not stable-30 qualification; whole-update costs and host load are retained in
parallel-semantic-paired-secure-env/review.json and its host-load files. The first
comparison failed at compile time on the deprecated getenv API; its replacement
uses _dupenv_s. Source is restored after both attempts. The installed desktop
remains unchanged. Local MSVC future implementation uses Concurrency::create_task;
no claim that this creates a fresh OS thread per frame is established.

The pure-integer restricted-span leaf passes 32,768 differential cases after
correcting its fixture to use addresses in its actual allocation. The earlier
fixture's unmapped uncached alias caused an access violation; that result is not
a proven candidate defect. Alternating copy benchmarks show no compelling gain
(roughly 159-162 ns median), so this compiler hint remains unselected. Both
integer-span-alias-prototype and integer-span-mapped-fixture evidence remain on D:.

A new private full-function profiler separates nested instruction, native-handler,
graphics and dispatch-prefix counters before reporting each function's owned
work. It records validated actor-slot/drone counts and cumulative geometry counts.
All rows go through the existing buffered trace writer. Wall residuals still
include host preemption and instrumentation, so they must not be labelled pure
on-CPU time. Negative residuals must be reported, never silently converted into
a precise percentage. Detailed state and matching checks are recorded in
detailed-function-cost-profile; its source/runtime are restored after capture.


### Detailed stage profiling and direct MI-mask candidate, 2026-10-15

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


### Integer entity-span compiler candidate, 2026-10-15

After sparse profiling identified entity/pathfinding CPU cost, a private compiler
hint is limited to admitted integer-only spans in nine measured functions. CPU
context and cached RAM use distinct restricted aliases; floating spans and all
original fallbacks remain intact. The first build rejects shadowed local variable
names under warnings-as-errors and performs no replay. The replacement uses
distinct names and matching private memory macros, preserving that warning gate.
Evidence, exact-state results and paired costs: integer-entity-span-alias-paired
and integer-entity-span-distinct-paired/review.json on D:. No selection or stable
30 FPS claim follows from compilation alone; private sources are restored.


The isolated mask-access benchmark measures about 13.9-14.9 microseconds for
a guarded read/write pair versus 18-21 nanoseconds for direct model calls.
This establishes an access-cost reduction, not a whole-game speedup; the
production callback bridge adds a small lifetime-check cost absent from that
prototype. A source candidate now adds optional MI-mask CPU-bridge operations
with exact mapped-memory fallback, callback lifetime checks and rejection-value
preservation. Native callback handling retains journal, device and I/O-revision
effects; reference timing remains on the original access path. Generated-site
rewriting, artifact inventory updates and runtime qualification are pending.
Evidence: native-mi-bridge-implementation. No new desktop package is delivered.


The first integration build stops at the generated-root freshness check because
the normalizer changed. This produces no replay result. The private incremental
migration updates the new runtime exports, symbol inventory, link registry and
qualified instruction sites together, then records the new generator digest.
Local declarations avoid changing the shared generated header. The guard and
build freshness checks remain enabled. Evidence: native-mi-bridge-generation.


The broader integration build exposes missing original-timing exports in the
link-only smoke executable. Its registry deliberately does not run guest
bodies. The harness now supplies a null timing state and an aborting hook,
so accidental guest execution still fails instead of receiving fake timing.
The runtime executable itself linked in the failed broad build; qualification
waits for the repaired full build and focused tests. The initial migration
wrapper also stopped on an existing evidence directory; no evidence was erased.

The timing-hook link stubs are shared by all three link-only audit executables;
the first harness adjustment fixed smoke linking but baseline/patch audits
exposed the same missing exports. No runtime timing code is stubbed.


The integrated MI-mask candidate passes the full Release build, 12 focused
CTest checks, exact 4,000-VI recorded-state comparison and exact 17,000-VI
Original comparison (including renderer snapshot). Native 120 completes with
zero audio underruns but retains actual 3-VI/20 Hz updates. Native 360 fails
near VI 5737 with Rt64ShellError::renderer_exception (submit-15), 125 audio
underrun trace events and heavy cadence loss beforehand. Its host CPU samples
reach 99.6% (median 51%); that observation alone does not assign causality.
The candidate is held, not qualified or delivered. A same-binary mapped-access
control and exception-detail capture will distinguish the new register path
from existing native 360 pacing/resource problems. Evidence:
native-mi-bridge-qualification, mi-bridge-native360 and mi-bridge-native360-control.
A review/commit wrapper had a Python syntax error and committed nothing.


The 360 FPS comparison wrapper restored both diagnostic source edits, but its
final rebuild failed with LNK1180 (insufficient disk space). Two dependent
checkpoint jobs then stopped at their prerequisite assertions without editing
source. This was a build-environment failure, not new gameplay evidence.
Recovery relocated only the investigation's build tree to D: after a complete
file-hash comparison and retained the original C: path through a junction. The
ordinary candidate rebuild succeeded; earlier failure records remain intact.
Evidence: build-drive-recovery-20261015. The desktop preview is unchanged.


### MI-mask source checkpoint; qualification remains incomplete

The same diagnostic binary completes a 360 FPS mapped-access control with four
audio underruns and a direct-access run with zero. Both retain actual 3-VI
updates; their native instruction/timing paths differ, so this is not a causal
whole-combat speedup measurement. Neither run repeats the earlier renderer
exception, and its cause remains open. The diagnostic exception capture and
control switch are removed afterward and the ordinary candidate is rebuilt.
Evidence: mi-bridge-native360-control and mi-bridge-source-checkpoint-recovered/review.json.

This source checkpoint preserves the narrow register-access candidate and its
reproducible validation, not a completed no-20-FPS remedy or release. The exact
recorded/Original state results and 120 FPS audio result remain valid. Native
360 robustness requires further work; no desktop launcher/package changes.
Fresh generated roots include the two runtime exports and qualified site pass;
older generated inventories require regeneration or the documented complete
incremental migration, not a digest-only freshness override.

{"case": "mi-control360-original-access", "audio_underruns": 4, "host_cpu_median": 45.75, "top_observed_cpu": [{"name": "jfg-native-boot.exe", "mean_one_core_percent": 157.25311629869427}, {"name": "wezterm-gui.exe", "mean_one_core_percent": 90.29440400852674}, {"name": "python3.11.exe", "mean_one_core_percent": 67.1250767350496}, {"name": "explorer.exe", "mean_one_core_percent": 60.396829252577014}, {"name": "dwm.exe", "mean_one_core_percent": 42.53977885100147}, {"name": "System", "mean_one_core_percent": 37.263308917025206}, {"name": "msedge.exe", "mean_one_core_percent": 15.898587273822951}, {"name": "OneDrive.Sync.Service.exe", "mean_one_core_percent": 13.836089880972068}]}

{"case": "mi-control360-direct-access", "audio_underruns": 0, "host_cpu_median": 34.8, "top_observed_cpu": [{"name": "jfg-native-boot.exe", "mean_one_core_percent": 157.7145628791479}, {"name": "wezterm-gui.exe", "mean_one_core_percent": 82.17584582959971}, {"name": "python3.11.exe", "mean_one_core_percent": 62.78593839091493}, {"name": "dwm.exe", "mean_one_core_percent": 34.02654898415906}, {"name": "System", "mean_one_core_percent": 32.37957178027382}, {"name": "python.exe", "mean_one_core_percent": 19.58336000021389}, {"name": "msedge.exe", "mean_one_core_percent": 16.559118079025712}, {"name": "explorer.exe", "mean_one_core_percent": 16.28926111976895}]}


The integrated parallel semantic-history candidate restores each worker's prior
floating-point environment on normal return and exception unwinding. The worker
owns its input copy and separate history, and joins before consumption. A shared
rate predicate preserves offscreen/reset/repeat/cadence behavior. The first build
failed warnings-as-errors on the old predicate's now-unused local; removing that
local retains the warning gate. Three focused tests then pass, including pooled
worker restoration and cadence agreement. Exact-state and native FPS
qualification remain pending. Evidence: parallel-semantic-scoped-environment-recovered
and parallel-fenv-unused-local-recovery. No desktop package is changed.


The renderer submission boundary now preserves bounded exception text and source
VI in captured stderr while retaining its existing error code. Previously, the
catch-all erased the original exception behind submit-15, preventing diagnosis
of the unreproduced native 360 failure. This is observability, not a crash fix.
No allocation is required on this error path, and normal frames add no logging.
Build and existing renderer-shell tests pass; repeated native checks follow.
Evidence: renderer-submit-exception-context.


### Post-move continuation and parallel candidate qualification

The owner approved another six hours after the legacy-checkout preservation.
The same production investigation resumes against the migrated ledger, with
its guard and all prior counters retained. One bounded continuation is admitted;
this does not erase the exhausted preceding attempt. No desktop package changes.

The parallel-history candidate matches the dense recorded-clock state and
17,000-VI Original RAM/save/renderer snapshot. Native 120 completes without
audio underruns but still has three-VI updates. Native 360 completes without
the earlier submit exception, yet logs 20 audio-underrun events and a combat
scene rate around 25 Hz. Correctness correspondence is established only for
the compared paths; neither native result meets the no-20-FPS goal. Evidence:
parallel-fenv-qualification and postmove-reconciliation/qualification-review.json.

The preservation handoff incorrectly described step 427 as unexecuted: the
worker had accepted and completed its exception-context change before stopping.
Its passing build/shell-test result and source edit are retained. Only steps
428 and 429 were cancelled before execution. Corrected private handoff:
legacy-preservation-resume-correction-20261015.json. Historical evidence is
unchanged; new requests will qualify the remaining candidates.


A distinct private rounding-control fixture targets repeated CFC1/CTC1
rounding changes in the measured drone/pathfinding routines. It compares the
existing generated helpers with direct MSVC-x64 MXCSR round-bit access, retaining
all other MXCSR bits. This is a local prototype, not a selected runtime change.
Validation includes existing sticky exception flags, all rounding directions,
DAZ/FTZ combinations, arithmetic output bits, complete CRT environment snapshots
and errno. Evidence: rounding-control-fixture/result.json and run.log.
The x64-specific scope follows Microsoft's documented SSE2 control environment;
other architectures retain the portable helpers. Whole-game correspondence
and performance remain prerequisites even if the isolated fixture passes.


A private dispatch experiment gates temporary counter snapshots to the scopes
that consume them. Actual counters, timing hooks and observations remain active;
this only avoids reading unused baselines on unrelated guest calls. Identical
recorded inputs/timing are compared in alternating order with heavy profiling
off. Source and the ordinary candidate executable are restored afterward.
Evidence: dispatch-snapshot-gating. Performance selection requires review;
this is not a completed combat remedy.


The rounding-control fixture passes 4,096 control-state and 6,400 arithmetic
comparisons. A save/set/get/restore microbenchmark drops from about 75 ns to
35 ns; this is not the number of calls or savings in a game update. A second
fixture measures the actual float-to-int conversion. Only finite normal values
strictly inside the int32 range use the hardware intrinsic; denormals, zeros,
NaNs, infinities and out-of-range operands retain the CRT implementation.
Evidence: rounded-conversion-fixture/result.json. Runtime selection remains
pending exact-state and measured whole-update comparisons.


### Controlled parallel-history cost and rejected micro-optimizations

Four alternating same-binary runs preserve exact RAM, save, update instructions
and interpolation decisions. In the recorded busy window, serial history costs
2.545 / 2.491 ms median; parallel history costs 1.919 / 1.814 ms. This repeatable
roughly 0.6-0.7 ms history reduction is a qualified component improvement, not a
no-20-FPS resolution. Whole-update medians vary from 28.793 to 30.950 ms without
a similarly strong separation. Two native 360 runs complete with zero audio
underruns and no renderer exception, but retain three-VI updates (one and 23 in
the combat window respectively). Their different native instruction paths do
not support a causal whole-combat comparison. Evidence:
parallel-fenv-controlled-costs/runs.json and retained host-load observations.

Gating unused dispatch-counter snapshots preserves all recorded comparisons
but yields no clear repeatable combat benefit (baseline medians 28.791 / 28.567
ms, candidate 28.466 / 28.718 ms). It is not selected; source and runtime were
restored. Evidence: dispatch-snapshot-gating.

The rounded-conversion fixture passes 1,048,576 edge-case comparisons, including
value bits, MXCSR, CRT environment and errno. Its nanosecond-scale conversion
savings do not explain the multi-millisecond combat excess. The later hot-function comparison is recorded below; no rounding-helper change is selected.
The next investigation targets native instruction locations rather than
promoting isolated microbenchmark gains. Evidence: rounded-conversion-fixture.

The parallel-history source checkpoint preserves its existing serial cadence
rules, copied input ownership and worker floating-point environment, restoring
the pool thread on success or exception. Focused cadence/environment/shell
tests, dense recorded comparison and 17,000-VI Original comparison passed as
recorded above. Native 360 robustness and the owner's full route remain open.
This checkpoint does not update the installed desktop preview.


### October 15 reference and native instruction sampling

The owner-requested [VibinToast review](../../upstream/reference-review-2026-10-15-vibintoast.md) distinguishes its real 60 Hz scheduler, disabled matrix interpolation path and unqualified release timings. Its operating-system replacement architecture is a useful performance lead; no claim of eliminating combat drops is accepted without measurement.

The scoped native sampler now follows the actual update thread and preserves exact recorded RAM, save, instruction and interpolation comparisons. Combat samples include generated drone/path work, dispatch and lookup bookkeeping, rounded conversion, and a large wait-state share. Wait samples and sampler observer effects prevent treating these proportions as CPU-cycle percentages. The earlier all-thread sample limit expired before combat, so that capture cannot support combat attribution. Evidence: pc-sampling-update-thread, including timer/thread-selection fixtures and restored.json. No sampling hooks remain in the committed runtime. The native sampled run retained three-VI updates and one audio underrun; it is diagnostic evidence, not FPS acceptance.


A private dispatch-cache experiment combines immutable target-to-mapping and linked-section metadata in a bounded tagged array. Dynamic generated-function lookup, overlay allocation/publication, relocation and instruction boundaries remain unchanged. Collision replacement only causes metadata recomputation. Alternating recorded-clock comparisons require exact RAM/save, per-update instructions and interpolation decisions. Evidence: dispatch-metadata-cache-paired. The source/runtime are restored after this experiment; selection requires measured whole-update benefit and broader qualification.


A private native-only rounding/conversion comparison modifies only the nine
previously measured entity/pathfinding functions that actually call the relevant
helpers. It retains all instruction hooks, guest FCR31 writes and original-clock
fallbacks; unusual float conversion inputs keep the CRT operation. Separate
noinline helper boundaries retain the existing generated math context. Four
recorded-clock captures alternate baseline/candidate/candidate/baseline and
require exact RAM/save, per-update instructions and interpolation decisions.
Evidence: rounding-hot-functions-paired. Source/runtime restoration occurs even
on failure; any performance choice requires review, not a build-only claim.


The private Clang hot-function comparison compiles nine unchanged generated CPU bodies and links them ahead of the normal archive in an isolated executable. It retains every timing hook and dynamically checked operation. The special-value leaf fixture differs in NaN payloads; the separate finite fixture and exact recorded-route comparison do not erase that limitation or authorize delivery. Evidence: clang-hot-functions-paired. All source is unchanged and the ordinary runtime is restored after the experiment.


### October 15 controlled compiler and helper comparisons

All four runs of each comparison preserve exact recorded RAM/save, per-update instruction counts and interpolation decisions. These use the same recorded guest timing, so the following numbers measure host update cost, not newly demonstrated game FPS. Every experiment restores the ordinary runtime; the desktop preview remains unchanged.

| Experiment | Baseline combat median, ms (A / B) | Candidate combat median, ms (A / B) | Result |
|---|---|---|---|
| Immutable dispatch metadata cache | 30.079 / 29.165 | 30.548 / 28.102 | Mixed whole-update results; not selected. |
| Native rounding/conversion helpers in nine hot functions | 29.402 / 28.040 | 28.824 / 29.123 | No consistent benefit; not selected. |
| Clang, nine unchanged hot functions | 29.223 / 29.315 | 27.412 / 28.505 | Lower median cost in both pairs; exceptional-value compatibility remains unqualified. |

Clang reduces combat median host work by roughly 0.8-1.8 ms in these pairs, but candidate 95th-percentile work remains 41.5-43.9 ms, above the 33.3 ms budget. An 8,192-case finite-input matrix fixture matches the baseline, while the exceptional-input fixture differs in NaN payload bits. Real-route correspondence does not establish equivalence for those inputs. No compiler candidate is selected or delivered. Evidence IDs: clang-generated-leaf-toolchain, clang-finite-leaf-cost, and the three experiment directories named above.


A broader private Clang comparison compiles the unchanged generated bodies observed at least five times in the pinned combat samples (capped at 64 functions) and links them ahead of the normal archive in an isolated executable. It retains every timing hook and dynamically checked operation. The special-value leaf fixture differs in NaN payloads; the separate finite fixture and exact recorded-route comparison do not erase that limitation or authorize delivery. Evidence: clang-sampled-functions-paired. All source is unchanged and the ordinary runtime is restored after the experiment.


A private grouped-memory-validation prototype combines repeated bounds/alignment predicates for stable base registers inside existing native spans. Every original fallback and timing hook remains, and a failed combined predicate uses the existing individual checks. Boundary/random address proofs precede four exact recorded-clock comparisons. Evidence: span-range-guards-paired. This experiment restores the ordinary runtime; no generator change or preview is selected by this test alone.


The broader private Clang candidate also receives four alternating real-time 120 FPS route runs with muted audio and copied initial state. Actual VI intervals and underruns are counted independently of rendered FPS. Native timing changes can produce different game paths, so these are repeated robustness observations, not exact-state CPU comparisons. Evidence: clang-sampled-native120. The ordinary runtime and desktop preview are preserved; special-value compiler compatibility remains open.


The broader 62-function Clang comparison preserves all recorded comparisons. Combat medians are 29.464/30.005 ms for baseline and 26.444/26.511 ms for candidate; candidate p95 remains 44.700/44.197 ms. This supports a component throughput improvement, not a no-dip claim. Four real-time route comparisons are tracked separately. The inspected baseline executable contains about 589.6 MiB of machine code versus 13.5 MiB in the independently built reference release. Different runtimes/features prevent treating that ratio as a CPU speed ratio. It motivates a private experiment sharing repeated fallback timing code, with every observation preserved. No candidate is selected from file size alone.

The inline-symbol resolver now passes its forced-inline fixture (24 of 54 outer-function bytes resolve to the nested helper) and loads the matching pinned PDB. In combat it resolves inline attribution for only 52/1088 and 65/1095 executable-address observations. This sparse subset cannot establish a timing-bookkeeping CPU percentage. Prior fixture attempts failed symbol loading; loading the live fixture module and supplying the captured module size resolves that harness problem. Evidence: inline-pc-attribution-loaded-module. No new game sampling was needed.


Existing sparse stage captures are additionally partitioned by measured active update duration above/below 33.33 ms, preserving complete nested scopes and exact root-to-update joins. This asks whether tail updates have different CPU work or renderer/wait costs rather than relying on whole-window averages. Instrumented wall durations remain observations, not uninstrumented FPS acceptance. Evidence: slow-update-stage-attribution. No new game logging or runtime mutation is needed for this analysis.


### Real-time compiler comparison and unselected memory predicates

The four 120 FPS/VSync runs use the same input recording and copied initial save, with no expensive function profiler. Native timing can change later instruction paths, so they supplement the exact recorded-clock comparison rather than replace it. All remain private experiments.

| Run | Combat three-VI / four-VI updates | Extended three-VI / four-VI updates | Extended active median, ms |
|---|---|---|---|
| clang-native120-baseline-a | 11 / 0 | 127 / 1 | 28.593 |
| clang-native120-candidate-a | 11 / 0 | 28 / 0 | 21.572 |
| clang-native120-candidate-b | 7 / 0 | 69 / 2 | 26.222 |
| clang-native120-baseline-b | 70 / 0 | 254 / 5 | 29.645 |

The compiler candidate still produces three-VI updates. Lower median CPU work is not the requested no-20-FPS result. Encounter coverage is recorded separately in clang-sampled-native120/encounter-coverage.json; failed coverage analysis, if any, cannot qualify the route. The installed preview remains unchanged.

The stable-base memory-predicate prototype passes 87,516 address-boundary/random checks and all four exact recorded comparisons. Combat medians are 30.282/29.055 ms baseline versus 29.755/28.793 ms candidate, with no consistent quiet-scene advantage. The small variable saving does not justify selecting the generated-source change. Ordinary source/runtime are restored. Evidence: span-range-guards-paired.

The shared fallback wrapper's complete leaf fixture returned 3. Evidence: outlined-native-fallbacks/fixture/result.json. The candidate stops before whole-runtime testing; no generator change is selected.


The private fallback-outlining matrix fixture failed special-value RAM comparison immediately (NaN payloads at offsets 0x118 and 0x134), so no whole-runtime outlined candidate was built. Clang's additional classification also finds a CPU-state mismatch at case 112; treating RAM NaNs as one class does not establish equivalence. Both broad candidates remain unselected. The integer-only compiler comparison was withdrawn before execution to keep the owner's reference tutorial capture free of concurrent game/build work. Evidence: outlined-native-fallbacks/fixture, clang-float-payload-classification, and immutable request withdrawal records. The unchanged preview remains installed.


Owner tutorial follow-up: severe stalls are confirmed in the recorded Original-mode route, including 15.0-update/s diagnostic windows. The capture inherited debug cheats, forced pistol selection and inactive dual-stick controls; it is not a clean default-game comparison. See [DIAG-002](reference-owner-capture-contamination.md) for the full audit, per-level timing and prepared correction. Missed-input causation and matching original-game targeting remain open; no auto-aim cheat was identified among the enabled overrides.


A narrower compiler comparison replaces only eleven sampled generated bodies with no floating-point operations or register access. Every called function, timing hook, memory check and original source remains. The complete RAM, save, instruction counts and interpolation decisions must match across the recorded route. Evidence: clang-integer-functions-paired. This does not qualify compiler-wide exceptional floating behavior. The ordinary executable is restored afterward.


The floating-state diagnostic classifies differing 32-bit words only in RAM and floating registers; all integer/control state, timing structures, counters, hook calls and exception flags must remain exact. Classification preserves the earlier strict failures and does not redefine exact equivalence or select a runtime. Evidence: floating-state-difference-classification/result.json.


The follow-up cold-original-helper performance diagnostic runs only after the complete 8,192-case classification found differences confined to NaN sign/payload representations. All finite values, integer/control state, timing state and exception flags matched in that fixture. Strict equivalence remains false; the diagnostic must preserve exact recorded game RAM/save/instruction/interpolation decisions and does not select or install a runtime. Evidence: cold-original-classified-paired.


The resumed integer-only compiler comparison passed all four exact recorded-route checks. Combat medians were 38.148/32.236 ms baseline and 29.443/33.681 ms candidate; host contention and inconsistent pair results prevent a repeatable benefit claim. It remains unselected. The complete exceptional-value classification found only NaN sign/payload changes in the matrix fixture (Clang: 1,009 affected cases; cold original helper: 204), with all 8,192 cases matching finite/non-floating state, timing, hooks and exception flags. This clarifies the prior CPU mismatch but does not make either candidate byte-exact or qualified for delivery. Evidence: clang-integer-functions-paired and floating-state-difference-classification. The owner ended upstream debugging; performance work continues solely on our runtime.


### Owner-requested pause and compiler attribution limit

The owner requested a pause after the current comparison to take stock. The four cold-helper/build comparisons finished and preserved exact RAM, save, per-update instruction counts and interpolation decisions. The ordinary executable was restored; no preview change occurred.

| Run | Combat active median, ms | Combat active p95, ms |
|---|---|---|
| baseline-a | 29.361 | 42.847 |
| candidate-a | 27.601 | 43.872 |
| candidate-b | 27.571 | 43.71 |
| baseline-b | 37.753 | 55.585 |

These are recorded-clock CPU-cost observations, not new native FPS acceptance. The slower tail still exceeds the 33.3 ms update budget. Audit of the original compiler command revealed a confound: production generated C uses /fp:precise and /std:c11, while the manual candidate commands use /fp:strict and omit /std:c11. Consequently the measured effect cannot be attributed solely to helper outlining or solely to changing the compiler. Existing exact-route matches remain valid for the tested binaries; causal optimization claims require matched compiler/language settings. The prepared one-factor size audit and production-mode compatibility fixture were withdrawn before execution at the owner's pause request, along with additional transition tests. No results are claimed for them.

See [the pause checkpoint](../high-refresh-checkpoint-2026-10-15.md) for verified improvements, open issues and recommended next steps. The production guard remains enabled; pausing preserves its existing investigation, attempts and charges.


### Owner-approved optimized implementation direction

After the pause checkpoint, the owner explicitly prioritized optimized implementations over instruction-for-instruction authenticity, including asymptotically faster algorithms when AI behavior remains equivalent, and approved a five-hour continuation. The existing investigation and all counters remain; the production guard stays enabled.

For native optimization candidates, fewer guest instructions, different scratch/register state or a different internal algorithm are not automatic failures. Preserve observable AI decisions, RNG effects where they influence gameplay, actor motion, collision/visibility outcomes, save state, input responsiveness and elapsed game time. Validate replacements against captured function inputs/outputs and relevant game behavior; keep the original path for unsupported preconditions and explicit reference/observer modes. Exact recorded-clock replay remains appropriate for compiler-only changes, but an instruction-indexed clock recording cannot itself qualify an algorithm that executes fewer guest instructions. No behavior difference is silently declared equivalent.

Initial work isolates the compiler/code-size confound, then examines measured drone/pathfinding costs for native equivalents or algorithm improvements. No external recomp debugging is resumed. The desktop preview is unchanged until a candidate is qualified.


A bounded native angle-lookup candidate targets the sampled Arctanf movement/collision helper. The private independent implementation reads the existing guest lookup table and preserves finite-input return angles rather than substituting host atan2. Boundary, random, signed-zero, subnormal and four-rounding-mode checks compare its return value, original path length, final timing boundary and floating exception flags with the generated reference. Nonfinite inputs retain the original fallback. Evidence: native-angle-leaf-fixture/result.json. This is a local fixture candidate, not yet a game-tested or delivered optimization. Fixture result: 0.


The angle candidate additionally preserves the complete guest register result and original timing count, permitting stronger exact replay qualification despite the owner allowing ABI-only equivalence. A bounded 37-operation entry certificate, finite-input and floating-register-layout checks retain the generated fallback for unsupported cases. A separate 100,000-case fixture varies poll budgets and fallback preconditions and compares full CPU/timing/FENV state, excluding only batching diagnostic counters. Evidence: native-angle-context-fixture/result.json; fixture return code 0. No runtime candidate is selected by this fixture alone.


All 2,909 compact-helper bodies compiled successfully. The initial link command exceeded the Windows linker response-file per-line limit (LNK1170 at 131,071 characters); dependent game runs were refused by their prerequisites and did not execute. The link is retried with one object per line, reusing the verified objects and preserving the failed response/log. Angle fixture results independently passed 993,888 finite/boundary/rounding cases and 100,000 complete-context/timing cases (57,317 optimized, 42,683 fallback). This is build/fixture progress, not FPS acceptance. The attempted QueryInformationJobObject(NULL) resource report covers an inner Python job (only six processes), not the production supervisor tree, and must not be used as remaining-budget evidence.


The controlled compiler audit reproduces the baseline machine-code hash with its actual compiler settings. For the sampled routine, C11 versus legacy language mode does not change machine code; precise versus strict floating mode changes only a few bytes. Outlining the original timing helper reduces that routine from 3,324,678 to 46,483 bytes with production options held constant. The all-function private candidate retains those options across all 2,909 generated bodies, then compares the recorded combat route. Evidence: compiler-size-factor-audit and compact-clock-all-functions. This demonstrates a code-generation effect, not by itself a no-dip fix; real-time and broader route qualification remain required.


The all-function compact timing-helper candidate receives four alternating real-time 120 FPS route runs with muted audio and copied initial state, after the exact recorded comparisons. Actual VI intervals and underruns are counted independently of rendered FPS. Native timing can change later game paths, so these are repeated robustness observations, not exact-state CPU comparisons. Evidence: compact-clock-native120. The ordinary runtime and desktop preview are preserved; no no-dip claim follows solely from lower code size.


The bounded native angle candidate is compared against the compact-helper build using four alternating exact recorded combat replays. Both sides share the same compiler settings and all other generated functions. Full RAM/save, per-update original instruction counts and interpolation decisions must match; CPU costs are recorded independently. Evidence: native-angle-paired/runs.json. Ordinary runtime/source and the desktop preview are preserved. This is a component optimization comparison, not a no-20-FPS acceptance result.


The compact-helper candidate also receives the retained 17,000-VI Original-mode comparison and a muted native 360 FPS stress route. Original RAM/save, complete update instruction records, renderer hashes and probe output must match the retained reference. Native mode reports actual update intervals and audio underruns separately. Evidence: compact-clock-original-and-360/runs.json. These are private qualification captures; the installed preview remains unchanged.


A private native Count-publication candidate removes redundant volatile stores from instructions and spans that cannot advance time. Certificate refresh publishes Count once; all runtime native clock mutations already publish before recertification, and the Original path remains. The invariant is tested at certificate refresh, with native differential comparisons starting from the published value. Exceptional matrix outputs are classified without suppressing non-NaN or FENV differences, then the 62 previously sampled hot bodies use matched production compiler settings for four exact recorded-route cost comparisons against the compact-helper baseline. Evidence: native-count-publication-sampled. The earlier strict NaN-payload failure remains historical evidence; source and the ordinary runtime are restored. No performance or delivery selection follows from compilation alone.


The compact native captures receive an encounter check that carries unchanged actor state forward and respects removals. It records level/active drone counts, proximity, and every extended-window update longer than two VIs with its measured work and wait. This avoids mistaking sparse changed-actor rows for a complete actor census. Native paths still differ and are not exact workload pairs. Evidence: compact-clock-native120/encounter-and-slow-update-review.json.


### Compact timing helper selected for source integration

The repeated exact combat comparison reduces median active work from 29.486/29.509 ms to 25.852/25.765 ms with matched production compiler options. The executable shrinks from 627,766,272 to 146,052,096 bytes. The retained Original-mode comparison and native 120/360 robustness checks qualify the component, while native slow updates remain. The public timing header now uses a portable noinline declaration for the original cache-model helper; native checks and algorithms remain. The generation pipeline already copies this header, and the active private generated header is refreshed from the same source. Focused timing-model and generation tests pass. This source commit is a component improvement, not a resolved PERF-001 or a preview deployment; the ordinary runtime rebuild and package qualification follow separately. Earlier NaN representation differences in synthetic matrix fixtures remain documented, and do not become a universal bit-equivalence claim.


### Compact code and native angle review, 2026-10-15

The private all-function build retains production compiler/language settings. Machine-code size is measured from executable code sections, independently of file size. The compact helper is selected for source integration separately; the native angle prototype is not selected because its repeated cost results are mixed. Live acceptance remains pending.

| Build | Executable bytes | Machine-code bytes |
|---|---:|---:|
| baseline | 627766272 | 618275867 |
| candidate | 146052096 | 137739115 |

Recorded-clock combat cost (milliseconds); these preserve the captured workload and are not real-time FPS acceptance.

| Run | Median | p95 | RAM/save, instruction and interpolation agreement |
|---|---:|---:|---|
| compact-all-baseline-a | 29.486 | 43.861 | True |
| compact-all-candidate-a | 25.852 | 41.086 | True |
| compact-all-candidate-b | 25.765 | 39.916 | True |
| compact-all-baseline-b | 29.509 | 43.821 | True |
| native-angle-baseline-a | 26.53 | 44.579 | True |
| native-angle-candidate-a | 25.821 | 41.512 | True |
| native-angle-candidate-b | 26.913 | 42.541 | True |
| native-angle-baseline-b | 26.546 | 42.478 | True |

Native 120 FPS/VSync repetitions use the same input recording but can follow different timing-dependent paths. Counts below are from the extended VI 3500-6000 window.

| Run | Median active ms | Two-VI updates | Three-VI updates | Longer updates | Audio underruns |
|---|---:|---:|---:|---:|---:|
| compact-native120-baseline-a | 26.954 | 1192 | 38 | 0 | 0 |
| compact-native120-candidate-a | 23.204 | 1247 | 1 | 0 | 0 |
| compact-native120-candidate-b | 24.954 | 1220 | 19 | 0 | 0 |
| compact-native120-baseline-b | 26.909 | 1194 | 37 | 0 | 0 |

native-angle-leaf-fixture: return code 0. PASS cases=993888 maximum_instructions=37 angles_counts_lastpc_fenv_exact=1 ram_unchanged=1; COUNT 9 16; COUNT 25 124256; COUNT 27 124088; COUNT 28 124348; COUNT 30 124000; COUNT 31 124820; COUNT 32 124548; COUNT 33 123952; COUNT 34 123804; COUNT 37 56

native-angle-context-fixture: return code 0. PASS cases=100000 admitted=57317 fallback=42683 full_context_timing_fenv_exact=1

Evidence: compact-and-angle-evidence-review/review.json links the private fixture, recorded-cost, native cadence, Original/360 qualification and nested-job accounting results (not production budget evidence). No preview deployment or resolved no-dip status is implied. All captures were muted. The 32-entry path search is not prioritized: prior combat profiling observed about 3.6 calls/update, versus about 2,421 calls/update to Arctanf; these older instrumented counts guide target choice, not speedup acceptance.


Four alternating native 120/VSync comparisons additionally remove actor snapshots, interpolation comparison tracing and PCM capture, retaining the basic update timeline and buffered frame timings. Both sides use the same reduced instrumentation. This checks whether slow updates persist closer to the normal FPS-logging configuration; hidden windows still do not qualify monitor scanout or visual smoothness, and native paths may differ. Evidence: compact-minimal-native120.


A controlled Clang follow-up compiles only the 62 pinned sampled bodies against the compact header, with production precise/C11 options on both sides. The other 2,847 bodies are identical. A classified special-value matrix comparison precedes four exact recorded RAM/save/instruction/interpolation comparisons. This distinguishes compiler effects from the newly isolated inline-policy effect; no compiler or preview is selected from this private experiment alone. Evidence: compact-clang-frontend-options. The preceding compact-clang-sampled compilation refused the MSVC-only /Gm- option under warnings-as-errors; no game ran in that attempt. The retry removes that obsolete driver option and uses the Clang warning equivalents already recorded by the repository. Optimization, floating mode and C11 remain unchanged.


Four alternating native 120/VSync comparisons additionally remove actor snapshots, interpolation comparison tracing and PCM capture, retaining the basic update timeline and buffered frame timings. Both sides use the same reduced instrumentation. This checks whether slow updates persist closer to the normal FPS-logging configuration; hidden windows still do not qualify monitor scanout or visual smoothness, and native paths may differ. Evidence: native-count-minimal120. The baseline is the compact-helper build, and the candidate changes Count publication only in the 62 pinned sampled bodies; any all-body source integration needs a coherent rebuild and qualification.


### Narrow update-thread diagnostic for remaining hitches

A reduced-instrumentation repeat still records an 83.409 ms active-wall update with about 1.54 million guest instructions, a workload count also seen in normal updates. Instruction count alone cannot distinguish slower CPU operations, renderer work, scheduling delays or contention. Host records include other graphics/build activity; no isolated performance claim follows from those runs.

The optional JFG_DIAGNOSTIC_UPDATE_CYCLES=1 observation adds only two QueryThreadCycleTime queries per measured game update and one buffered update-thread-cycles row (VI, input poll, validity, host thread ID, cycle delta, guest instructions). It requires gameplay tracing and stays off by default. The same-thread validity check prevents attributing a different thread; cycles are not converted to milliseconds and exclude other guest/renderer threads. Unlike the full function profiler it does not time every dispatch. This is diagnostic source awaiting build/correspondence qualification, not a hitch remedy. Evidence: update-thread-cycle-source.


### Ordinary build consumer check and deferred Count-store candidate

The new callable helper exposed MSVC warning C4505 in a C++ consumer that uses the native timing refresh but never calls the Original helper. A C++-only maybe_unused declaration resolves that diagnostic without disabling warnings or changing generated C execution. The W4/WX consumer compile, focused timing tests and generation tests pass after the correction. Evidence: timing-header-consumer-build (failure), timing-header-consumer-build-fixed and compact-header-consumer-fix.

The sampled native Count-store reduction is not selected. Four reduced-instrumentation native 120 runs recorded extended-window median work of 24.660/22.800 ms for the compact baseline and 25.393/24.200 ms for the Count candidate; three-VI updates were 9/8 versus 21/11. The earlier recorded-clock tail improvement did not establish a real-time benefit. Its source stays private; existing native Count publication remains unchanged. Evidence: native-count-minimal120/runs.json. Clang remains unselected after consistently slower matched compact runs. The ordinary preview rebuild therefore includes only the confirmed compact helper and the opt-in update-cycle diagnostic from this round.


### Candidate: consolidate repeated stable-base RAM guards

A private generator experiment groups four or more cached-RAM checks sharing an unchanged base register inside an already admitted native span. Endpoint bounds and the widest compatible alignment prove the covered addresses. Unknown relocation forms, intervening base writes and incompatible alignment residues retain the existing checks. On preflight failure, the candidate commits the already executed prefix and resumes the unchanged original instruction sequence from the first check. All actual memory operations, timing admission/counts, callbacks and reference fallback remain. A bounded address/alignment property fixture and mutation rejection checks precede compilation. Evidence: native-span-range-certificate. This is an unselected candidate; no source/runtime or preview change is implied by the fixture.


### Compact-timing standard build qualified and delivered locally

The standard runtime build at 73b8b977b3e940728efa6c5fa0efab2ad741bfd5 passes the exact recorded combat comparison with optional update-cycle logging off and on, including RAM/save, original update-instruction counts and interpolation decisions. The retained 17,000-VI Original comparison also passes. Final muted native 120/VSync and 360/no-VSync captures complete with valid update-cycle records, no unsupported accesses and zero audio underruns. These native routes can diverge; they are robustness checks, not identical-workload performance comparisons or physical-display acceptance.

| Final native case | Extended active median (ms) | Three-VI updates | Other >3-VI updates |
|---|---:|---:|---:|
| compact-final-native120-cycles-a | 27.279 | 79 | 17 |
| compact-final-native120-cycles-b | 24.394 | 8 | 0 |
| compact-final-native360-cycles | 23.708 | 1 | 0 |

The High Refresh Preview and Jitter Capture desktop links now select the compact-timing local package. Runtime SHA-256: f0bf36e7be10a47504fee4444f8cc979172448430b96deb94ae3786a12039c46. Both links preserve arguments and all 40 checked profile files. Prior package/links are retained. This is a measured component improvement, not completion of the no-20-FPS goal, a public release or full-route visual acceptance. [Qualification and delivery](../../../evidence/high-refresh-compact-timing.json) records the final binary and checks.

The transferable compiler lesson is to reproduce production flags and isolate one factor before attributing a speedup. Removing duplicated cold timing code produced a repeatable recorded-workload improvement; changing the compiler or reducing an isolated operation did not reliably improve real-time cadence. Use native cycle/submit observations to identify remaining long updates before another implementation change.


The stable-base guard candidate compiles the 51 changed sampled bodies using matched production options, with the ordinary compact runtime as control. Four alternating recorded-clock combat comparisons test RAM/save, instruction counts and interpolation decisions while measuring both active wall time and root-thread cycles. The remaining generated bodies and public source are unchanged, and the ordinary runtime is restored afterward. Evidence: native-span-range-paired. This remains unselected until the repeated costs and qualification limits are reviewed.


Two further muted native 120/VSync observations use only the ordinary FPS CSV and its cached overlay, with gameplay/actor/interpolation tracing, cycle queries, PCM/RAM capture and the all-process host monitor disabled. The retained input replay is still automated. Scene submission times are reconstructed from scene_age_us; inconsistent atomic snapshots are reported and unobserved scene gaps are not silently treated as one update. SCENES are submissions, not an independent simulation-tick counter. These observations are closer to normal FPS logging, but differing native paths and host load prevent attributing a difference solely to instrumentation. Evidence: compact-fps-only-native120. They do not establish full-route or physical-display acceptance.


### Detailed profiler requested by owner

The owner requested hyper-detailed profiling because root update duration alone cannot identify the remaining spikes. An opt-in bounded function profiler records every reached dispatch edge in mode 2, or major stages in mode 1, on the root update's host thread. It reports per-update call counts, inclusive/exclusive thread cycles and wall duration, maximum call cost, known wait and observed guest instructions. Child costs are subtracted once; invalid counters, capacity limits and cross-thread samples invalidate precise attribution. Other guest threads and renderer execution are not silently counted as this thread's CPU work. Profiling defaults off and initially covers VI 3300-4500. A separate 256 MiB bounded memory trace defers gameplay TSV file writes until finish; overflow is explicit and crash-before-finish loses buffered evidence. This is instrumentation awaiting fixtures, build and same-workload overhead/correspondence checks, not a slowdown fix. Evidence: hyper-profile-source.


### Later comparison decisions and detailed attribution

The stable-base RAM-check consolidation passes its 661,374 boundary/alignment checks and all four exact recorded comparisons. Combat medians are 26.537/26.273 ms for the compact control and 25.793/26.471 ms for the candidate. The small inconsistent gain is not selected; ordinary source/runtime were restored and the installed preview is unchanged by that candidate.

The ordinary FPS-only observations retain slow scenes with no full gameplay tracing or process monitor: the first extended window has 94 three-VI and 20 longer scene intervals, while the second has four three-VI intervals. Scene submissions are not independent simulation ticks, and differing native paths prevent causal claims about instrumentation. Evidence: native-span-range-paired and compact-fps-only-native120.

The owner requested hyper-detailed profiling. The implemented profiler now captures all reached function/caller edges or selected stages, plus same-update root/process cycles, wake latency, graphics submission, GPU completed timestamps and history/RDP-sync timings. Exact recorded state checks and the disabled Original comparison pass. Full profiling materially raises update cost, so its induced 20-FPS updates are not a normal-play regression claim. The lower-overhead native captures still contain genuine budget overruns. Earlier correlated captures show 10-16 ms of graphics submission in several slow updates, alongside substantial entity/pathfinding CPU costs; the recorded GPU timestamp totals are much smaller but not a complete root-owned GPU interval. This motivates splitting CPU-side interpolation preparation from RT64 synchronization; it does not yet establish one universal slowdown cause. See [profiler guide](../function-profiling.md), [PERF-002](frame-diagnostics.md), and [qualified diagnostics receipt](../../../evidence/high-refresh-function-profile.json). PERF-001 remains open.


### Profiler-directed resident math dispatch experiment

The full capture finds roughly 2,435 Arctanf and 1,335 mathDiffAngle calls per combat update. An earlier angle-body replacement did not prove a gain. A distinct private candidate now tests whether bounded native equivalents can also avoid generic generated-dispatch metadata and scope setup. Dispatch history, MMIO failure checks, generated-call counts and the outer function profiler remain. Focused/instruction observers and Original timing retain fallback; admission preserves clock sampling boundaries. C++ helper fixtures compare 200,000 complete CPU/timing/FENV states against the generated C reference. The candidate is unselected pending repeated exact-workload comparisons. Evidence: native-pure-dispatch-fixtures and native-pure-dispatch-build.


The first private dispatch build failed because its insertion placed the helper under the unused fallback-header branch and duplicated CRLF line endings. Its dependent comparison correctly refused to run. The corrected private build normalizes only the candidate text and places helpers after the selected timing header. Original source bytes/runtime are restored in both attempts; no failed executable was delivered. Evidence: native-pure-dispatch-build and native-pure-dispatch-build-fixed.


The first native math comparison completed its muted control capture, then the review rejected an expected ai_submission_overflows field absent from this play-mode summary. The corrected review explicitly checks the present unsupported-access/audio underflow/overflow fields and reuses the exact saved first capture; it does not rerun or replace that evidence. Evidence: native-pure-dispatch-native.


The profiler also identifies sine/cosine table lookups as frequent CPU calls. A private extension retains the existing guest table, exact register effects and counted paths, requires a bounded 20-instruction native certificate, and falls back for unsupported/nonfinite table entries. It changes no guest memory or lookup approximation. A further 200,000 C++ versus generated-C context/timing/FENV comparisons precede the combined four-helper build and exact recorded comparisons. Evidence: native-trig-dispatch-fixtures and native-four-math-build. This remains a candidate, not a no-dip remedy.


The shared-prefix review initially requested play-only audio fields from the finite original-OS probe summary and refused that analysis. The corrected review uses the existing 14-field tick records for audio underflow/overflow counters and reports both release-boundary totals and native-suffix deltas. Recorded-prefix audio overflows are preserved and separated from new native-suffix errors. It reuses the already completed first capture and does not replace evidence. Evidence: native-four-math-prefix-native.


### Selected resident-math source candidate, final build awaiting qualification

The four resident math replacements preserve the original guest table and full observed CPU/timing/FENV effects, with 400,000 differential checks against locally generated C. Four exact recorded combat comparisons preserve RAM/save/instructions/interpolation and show about 1.1-1.5 ms lower median update work within the two pairs. The source enables these bounded helpers only in the experimental native preview; Original timing, insufficient timing budget, unsupported inputs and explicit instruction/focused observers retain generated execution. JFG_DIAGNOSTIC_RESIDENT_MATH=0 selects the reference dispatch path for comparison. A reusable validation script and original synthetic fixture permit local reproduction without publishing generated game code. Shared-prefix/native evidence remains separate from final-build qualification and the no-20-FPS goal remains open. Evidence: native-four-math-recorded, native-four-math-prefix-native and resident-math-source-build.


The first public helper fixture build correctly refused an invalid include-contract test: the selected timing header undefines its internal inline macro at its end. Removing that macro check keeps the documented include order and the compiler type checks; it changes no math or timing logic. The corrected 400,000-case local reference comparison and build are recorded separately. Evidence: resident-math-source-build (failed header contract) and resident-math-source-build-fixed. No failed executable was delivered.


### Resident-math and profiler final qualification

The source-integrated four-helper build passes all 400,000 generated-reference cases; exact recorded combat with helpers disabled/enabled; the retained 17,000-VI Original comparison; full per-function profiling with exact recorded state; and muted native 120/VSync and 360/no-VSync runs. The final recorded confirmation measures 26.786 ms disabled versus 25.682 ms enabled in combat. This is additional to the earlier compact-helper improvement, not a claim that independently measured percentages add directly.

Four shared-prefix tests begin from exactly equal recorded state, then run in real time. Extended three-VI update counts are 1/20 for controls and 0/2 for the candidate; their subsequent native paths and host timings differ. The recorded prefix has its own preserved audio totals; no new audio underflows/overflows occur in those native suffixes. These observations do not establish a zero-dip guarantee.

| Final capture | Median active work (ms) | Three-VI intervals | Longer intervals |
|---|---:|---:|---:|
| resident-final-profile-full | 35.356 | 94 | 0 |
| resident-final-native120 | 24.140 | 86 | 0 |
| resident-final-native360 | 24.802 | 8 | 0 |

The full-profile row includes substantial observer overhead; it must not be read as ordinary gameplay FPS. Default profiling is off. Source and reproducible fixtures are integrated, with package delivery tracked separately. The no-20-FPS goal, late-route visual acceptance and shadow cadence remain open. See [native math notes](../native-resident-math.md) and [combined qualification](../../../evidence/high-refresh-resident-math.json).


### Combined preview delivered locally

Both High Refresh Preview and Jitter Capture desktop shortcuts now select the profiled-math package, runtime source commit 4a54d903853471bfd248cfd2da12ac97f14df1cb and SHA-256 3e898fc20e96d4c189e4645807f5a439bf01c0d84f684ea3bd1dc1f7d02186ec. Manifest and launcher/runtime setup protocol checks pass; both shortcut arguments and all 40 checked profile files are preserved, and the previous package/links remain available for rollback. Full profiling stays off in ordinary play. This is an additional measured component improvement with diagnostic tooling; occasional three-VI game updates and the no-20-FPS goal remain open. See [combined qualification and delivery](../../../evidence/high-refresh-resident-math.json).


### Eight-hour profiled continuation: geometry and submission attribution
The final default-on resident math capture verifies over one million helper uses without an enabling environment variable, zero audio under/overruns and zero unsupported accesses. At 120/VSync it still records 39 three-VI and four four-VI intervals. Stage profiling measures 24.537 ms median active work and a 55.930 ms maximum; this remains an instrumented observation, not visual acceptance.
The new per-update review finds roughly 28% of root-thread cycles in SmartMover and its unselected descendants. This supersedes the earlier call-count-only decision to deprioritize pathfinding: few calls do not imply low cost. Above-budget updates also have longer graphics submission (12.063 ms median versus 8.172 ms for within-budget updates), with both history preparation and RDP synchronization contributing. These nested costs must not be summed into root time twice.
A private shadow geometry implementation computes point-in-region results alongside the unchanged generated routine, restoring the host floating-point environment. A bounded capture will compare its answers and measure repeated collision/path queries before proposing an algorithm or cache. No geometry replacement is enabled or delivered. Evidence: resident-default-stage-profile, resident-final-slow-update-review, resident-profiled-stage-review, geometry-query-shadow-build.


The first geometry shadow capture preserves exact RAM/save/instructions/interpolation and identifies many repeated query arguments, but its predictor rejects all calls because the two overlay globals live in the checked synthetic overlay mapping rather than cached RDRAM. This is a diagnostic address-range defect, not a game failure or verified predicate. The correction allows only the two pinned globals of the already leased overlay; geometry pointers still require aligned cached RDRAM. Failed evidence is retained. Evidence: geometry-query-shadow-recorded and geometry-query-shadow-build-fixed.


### Geometry predicate and ABI candidate
The corrected shadow calculation matches all 53,272 point-in-region returns in the recorded combat window, with zero unsupported queries and exact final RAM/save/instruction/interpolation correspondence. Of those calls, 35,208 repeat the recorded arguments within a VI interval; the collision traversal similarly has 149,940 repeats among 229,818 calls. These figures establish repeated work, not permission to cache mutable state.
A private next candidate tests a direct geometry implementation that preserves the input context except the return value and reproduces the original private stack saves. The ABI test first executes the unchanged guest routine and then restores that proposed register contract, retaining original guest timing and memory effects; exact downstream replay correspondence is required before enabling the replacement. The actual native replacement skips emulated instruction accounting and therefore requires separate real-time behavior/performance qualification. It is opt-in, unselected, and unavailable from the desktop preview. Evidence: geometry-query-shadow-recorded-fixed and native-region-abi-build.


The point-query ABI replay passes 76,699 calls with exact final RAM/save/instructions/interpolation. A separate 200,000-case generated-C differential fixture covers randomized BSP chains, zero/degenerate edges, vertex-adjacent points and four rounding modes; it verifies return values, private stack saves, the proposed preserved-register contract and eight fallback cases. Initial real-time control/candidate medians are 23.022/22.887 ms, while three-VI intervals are 3/11 plus one four-VI candidate interval. Native paths and host timings differ; this does not select the candidate or establish a cadence improvement. A bounded revision limits traversal to 64 nodes/edges, rejects noncanonical or unsupported inputs, and retains generated execution during recorded-clock prefixes and explicit instruction observers. Evidence: native-region-abi-recorded, native-region-reference-fixture, native-region-native120 and native-region-bounded-build.


### Shared-prefix region candidate result
All four bounded-region comparisons start from an exactly matching recorded release fingerprint and complete without new native-suffix audio under/overruns or unsupported accesses. Extended median active work is 19.754/20.925 ms for the controls and 19.057/20.212 ms for the candidates. The candidate pair suggests a modest component saving, but its second run has 30 three-VI and three four-VI updates, versus zero/two three-VI control updates. The native paths and host timing diverge after release; these results do not establish a robust cadence improvement. The candidate remains private and unselected while submission costs are examined. Evidence: native-region-prefix120 and submit-sync-breakdown-review.
A local standard-library source check also confirms that this MSVC build implements std::async through Concurrency::create_task. It must not be described as necessarily creating a new operating-system thread for every scene. A dedicated worker proposal would require measured queueing evidence, not that assumption.


### Snapshot-copy and synchronization breakdown
The six-site submission probe preserves exact recorded RAM/save/instructions/interpolation and completes a native 120 capture without unsupported accesses or audio errors. Normal update medians include 7.380 ms submission, 1.151 ms snapshot preparation, 1.200 ms interpolation history, and 2.963 ms RDP synchronization. Above-budget updates have 12.751 ms submission; the worst 55.073 ms update has 33.618 ms submission. Fence/texture waits explain only part of that time, with large CPU-side or scheduling residuals. Nested costs are not additive to root work.
A private comparison replaces block-compare-then-copy with ordinary memcpy only for the renderer's full snapshot import, using the same checked buffer sizes and preserving the identical-pointer no-op. Gameplay's earlier snapshot creation and GPU/CPU ownership remain unchanged. Exact replay and repeated timing comparisons will decide whether this is useful; it is not delivered. Evidence: submit-sync-breakdown-captures, submit-sync-breakdown-review and snapshot-direct-copy-build-fixed. The first build rejected deprecated getenv under warnings-as-errors; it restored all source/runtime and never ran a candidate.


### Native line-intersection candidate
DoLinesIntersect accounts for about 2.1 percent of root-thread cycles in the retained full-profile capture. A private direct implementation uses the same 32-bit wraparound arithmetic and signed division/rounding, including degenerate/collinear results and optional short-coordinate outputs. It rejects unsupported pointers, overlap with live input stack arguments and signed-division overflow before mutation. Differential tests compare the original generated routine's return and all touched memory while checking the proposed preserved-register ABI; game-callsite validation is still required. Evidence: native-line-reference-fixture-fixed. The initial fixture omitted the reference break-handler link stub and never executed; the correction aborts on any unexpected reference break and retains that failure evidence. It is unselected and does not change the desktop preview.


The line-intersection differential fixture evaluates one million randomized/degenerate/extreme inputs: 971,701 admitted cases match the original return and all touched memory; 28,299 unsupported cases are identified before native execution, and eight explicit invalid-context/pointer cases delegate. The next opt-in runtime ABI check executes the original routine, verifies predicted outputs and restores only the proposed caller-visible register contract. Direct execution remains disabled during a recorded-clock prefix, and Original/instruction observers retain reference execution. Evidence: native-line-reference-fixture-fixed and native-line-abi-build.


The four alternating direct-copy recorded comparisons preserve RAM/save/instructions/interpolation. Per-snapshot medians improve from 0.518/0.548 ms to 0.472/0.478 ms, but whole combat-update medians are 26.676/27.259 ms for controls and 27.373/27.858 ms for direct copy. The small component change does not establish a robust update improvement, so direct copy is not selected. Evidence: snapshot-direct-copy-recorded.
The next private diagnostic records the actual graphics-submission owner's thread cycles and operating-system CPU-time counters around snapshot/decode/writeback. This distinguishes elapsed blocking/preemption from CPU work without attributing renderer work to the guest update thread. OS CPU times can be coarse per sample, and cycle counts cannot be converted to milliseconds. Evidence: submit-thread-attribution-build.


The line helper ABI check passes all 179,256 reached recorded calls with exact RAM/save/instruction/interpolation correspondence. Four native shared-prefix runs have equal release state and zero new audio errors/unsupported accesses. Controls measure 19.860/20.159 ms extended medians; native line replacement measures 19.712/21.172 ms, with 519,572/605,305 helper calls and no fallback. Three-VI intervals are 0/0 for controls and 0/1 for candidates. This is not a repeatable overall gain, so the line helper remains private and unselected. Evidence: native-line-abi-recorded and native-line-prefix120.
The newer submission-owner capture confirms the renderer executes on a different host thread from the root game update. Slow native updates contain increased CPU-cycle totals on both: median root cycles 36.0 million within budget versus 74.1 million above budget, and submission cycles 24.4 versus 39.9 million. This capture's longest update is 72.194 ms active with 16.821 ms submission and 2.427 ms measured renderer waits. These are observations, not a complete universal cause; nested wall costs are not added, and cycles are not converted to milliseconds. OS CPU-time counters are quantized in 15.625 ms steps here, making per-frame medians unhelpful. Evidence: submit-thread-attribution-review.


### Broader collision closure, isolated feasibility fixture
The two standalone predicate replacements produced insufficient repeatable benefit. A private next experiment translates the collision BSP helper and its two arithmetic callees as one closed CPU-only operation, retaining the generated arithmetic/register/memory behavior while removing per-instruction timing from the isolated fixture. The owner explicitly allows native implementations that preserve observable AI behavior rather than original instruction accounting. This is not runtime admission: bounded traversal, memory ownership, clock-poll responsiveness and actual-call differential checks still need proof before enabling it. The original fallback implementation is retained. Synthetic checks compare complete CPU context, touched stack/globals, host floating environment and periodic full RAM, with generated sources kept private. Evidence: native-bsp-closure-fixture.



### Collision shadow runtime rejection
The raw closed collision operation passes 50,000 isolated inputs but its first sampled game run fails a live comparison. Native execution remains unselected; the original routine still supplies every gameplay result. The first trace was buffered and lost at fail-closed exit, so a bounded diagnostic follow-up drains it and retains private entry state and detailed original/candidate differences. Generated C's unreachable paths initially triggered the host C++ warnings-as-errors policy; the diagnostic build confines suppression to its private translation unit and restores normal source/runtime after execution. The exact mismatch and remedy remain under investigation. Evidence: bsp-shadow-recorded-fixed, bsp-shadow-recorded-diagnostic, dispatch-and-bsp-shadow-review.


### Live shadow qualification and bounded traversal fixture
The live rejection was an overbroad test range: CPU context, collision globals and floating environment matched, while 244 changed bytes lay above the helper's stack arguments. The first difference was 1,925 bytes above entry SP, outside its write footprint. Source analysis bounds the helper/callee stack below SP with at most 16 argument bytes above it; a 4,112-byte comparison preserves all of that space for the already bounded 32-level shadow tree. The corrected capture passes all 256 sampled game calls, zero skips/mismatches, and exact final RAM/save/instructions/interpolation.
A separate private mixed-execution fixture now constrains native work to 32 nodes, 128 edges, 16 edges per leaf and 24 recursion levels, with a 50-microsecond admission deadline checked at node entry. It falls back at the current node, never restarts a partly executed parent, and retains every original arithmetic/register/stack effect. Tests deliberately exhaust different budgets and compare full CPU, floating environment, stack/globals and periodic full RAM. This is not a delivered optimization; runtime clock refresh and live performance still require qualification. Evidence: native-bsp-shadow-recorded-footprint and native-bsp-bounded-fixture.


The bounded BSP runtime candidate preserves the 4,000-VI recorded route and 17,000-VI Original reference, including RAM/save/instructions/interpolation and Original probe/renderer hashes. Live alternating comparisons retain the same verified release state; results and limitations are in native-bsp-bounded-review. It remains private and opt-in pending the comparison decision. Native instruction accounting is intentionally not 1:1 under the owner authorization, while Original and observers retain the reference path.


A second private collision variant annotates only the generated CPU-context parameter as non-aliasing and permits the two arithmetic leaves to inline. Guest RAM remains unrestricted because timing/fallback callbacks also access it through runtime state. The CPU context is distinct from mapped guest memory, and child calls receive the same derived context pointer. Full mixed-fallback differential checks are required before runtime comparison; this is an optimization candidate, not a selected change. Evidence: native-bsp-restrict-fixture.


### Source integration candidate
Repeated private native-collision comparisons reduce combat update medians from 21.743/22.174 ms to 20.468/20.045 ms, with all measured 120 FPS update intervals at two VIs and no audio or unsupported-access errors. The paired 360 target captures also keep two-VI game updates; rendered output is about 356-359 FPS in the sampled candidate bins. These captures share a verified release state but their later timing-dependent paths can diverge.
Identical synthetic workloads show roughly 69-70 ms native collision execution versus 114-119 ms reference execution over 50,000 cases with full runtime budgets. Reference timing callbacks are inert in that fixture, so this is a component result, not an absolute gameplay saving. Extra inlining/non-alias annotations do not show a material independent benefit; source integration retains the ordinary generated context parameter and leaf attributes.
The candidate moves bounded traversal into a separate host module, generates its three qualified bodies only inside the private build tree, rejects unqualified source digests/callees, and preserves complete reference fallback. It is a rebuild/qualification candidate until the integrated executable passes its own checks. Evidence: native-bsp-restrict-prefix120, native-bsp-restrict-prefix360, native-bsp-full-budget-benchmark, native-bsp-restrict-full-route and native-collision-source.


Both long private comparison runs consume all 7,276 saved input samples by 17,000 VIs with zero new audio errors and unsupported accesses. The native candidate has two isolated three-VI update intervals later in the route, plus two long gaps of 67 and 37 VIs. These are being correlated with game state, level changes, cold overlays and graphics submission before classifying them as gameplay drops or loading transitions. The combat window remains at two VIs per update. Evidence: native-bsp-restrict-full-route and native-bsp-full-route-review. Input consumption alone does not prove identical navigation after timing-dependent AI diverges.


The longer native route exposed two cold-overlay stalls, including a same-level reload that cannot be identified as damage versus death from generic state alone. Added rare trace markers at the existing level-change and death-restart counters so the next full route can establish whether a stall contains an explicit transition call. Historical captures without these markers remain ambiguous; they are not retroactively labeled.


### Integrated bounded collision and cadence diagnostics qualified

The separate production module passes 50,000 mixed-budget original-C comparisons, 18 admission rejections and cancellation cleanup. The final executable passes exact recorded combat and the 17,000-VI Original comparison. Detailed profiling passes on both recorded and native execution, with submission owner/wait records present and valid; full profiling remains off during ordinary play.

| Final native comparison | Combat median active work (ms) | Combat p95 (ms) | Extended VI intervals |
|---|---:|---:|---|
| collision-final-prefix120-off-a | 20.349 | 24.367 | {'2': 1229} |
| collision-final-prefix120-on-a | 19.892 | 24.233 | {'2': 1229} |
| collision-final-prefix360-on-a | 19.931 | 23.883 | {'2': 1229} |

The repeated integration comparisons used the same compiled arithmetic before rare transition markers were added. Enabled 120 medians were 19.796, 19.713 ms; controls were 20.467, 20.797 ms. These are component observations, not universal or identical-path native acceptance.

The final normal Play run consumes all saved route inputs, admits 2,124,488 native nodes and reports no unsupported memory accesses or audio under/overruns. Its post-prefix update intervals are {'2': 6700, '64': 1, '3': 1}. The following gaps are all retained, including transition-adjacent ones:

| VI | Delta VIs | Active / known wait (ms) | Explicit transition calls | Cold overlay activity |
|---:|---:|---|---|---|
| 6588 | 64 | 811.905 / 264.957 | [] (prior []) | current=True, prior=False |
| 6591 | 3 | 18.049 / 24.755 | [] (prior []) | current=True, prior=True |

The explicit level/death markers distinguish those calls from an unclassified gameplay hitch. A cold-overlay correlation alone is not assigned a cause. Late crate/light/bridge and shadow visual acceptance remain pending. The candidate is qualified for local preview packaging, not released or claimed as an all-level no-dip fix. [Compatibility and fixture](../native-collision.md); [final binary evidence](../../../evidence/high-refresh-native-collision.json).


### Bounded collision preview delivered locally
Both High Refresh Preview and Jitter Capture desktop shortcuts now select source commit 8f94f148ec367e4a221679a40255f12e6353af07 and runtime SHA-256 34ce90c9a11bba75a9ae47f3ae3d8a40d635b20357bf8c52391f4bd015a6bff1. Package and launcher/runtime setup checks pass; shortcut arguments and all 40 checked profile files are preserved. Previous package and links remain available for rollback. The qualified collision optimization, expanded submission diagnostics, cadence analyzer and explicit transition markers are included. Detailed function profiling remains off for ordinary play. This is a local preview, not a release or universal no-20-FPS claim. Late crate/light/bridge jitter and shadow cadence still require owner visual review. [Current evidence](../../../evidence/high-refresh-native-collision.json).


### Final delivery audit

The final binary also completes a muted capture-mode comparison with actor, interpolation, poll and progress logging enabled (recorded inputs replace live input recording). Its combat median active duration is 20.392 ms; extended update intervals are {'2': 1229}. No new audio-buffer errors or unsupported accesses occur. This bounded comparison does not remove the observer-cost caveat for full function profiling.

The cadence analyzer now includes transition calls anywhere between consecutive update returns, including calls outside either measured root. Final normal-Play gap associations: [{"vi": 6588, "delta_vi": 64, "transition_kinds": [], "latest_transition": {"stamp_us": 109416093, "vi": 6494, "poll": 3170, "kind": "death_restart", "age_ms": 1568.366}}, {"vi": 6591, "delta_vi": 3, "transition_kinds": [], "latest_transition": {"stamp_us": 109416093, "vi": 6494, "poll": 3170, "kind": "death_restart", "age_ms": 1611.178}}]. These are actual observed transition entries, not causes inferred from a level number alone. Further owner visual review is still required for the reported late scenery/shadow jitter; a fresh route is also needed to judge combat under live inputs.

## Owner review after collision preview delivery (2026-10-16 UTC)

The owner reports substantially improved stability and no noticeable FPS drops
during the latest recorded tutorial route. This is useful live evidence, not
proof that every update met 30 FPS. Frequent audio interruptions and late bridge
strobing remain, tracked in AUDIO-002 and RENDER-006. Do not claim complete
smooth-play acceptance from this report.


The audio/bridge qualification retains a repeatable ordinary Goldwood update gap: poll 2387 (VI 4935 at 120 output, VI 4932 at 360 output) expands from 45 to 48 actors and takes about 32.27 ms of measured active work plus 15-16 ms of known wait. No cold overlay or level/death transition occurs in that interval. This is a remaining 3-VI update, not a no-dip pass. A narrow full-function capture around that event is being reviewed; profiler overhead must be separated from ordinary-play timing. The new audio/bridge package stays selected while this follow-up remains private. The first capture timed out because its stop monitor read the memory-buffered trace before shutdown; it produced no usable trace and is excluded. The recovery monitors the independent FPS CSV for its stop target, then closes normally to flush the trace. Evidence: residual-cadence-roots and residual-goldwood-profile-recovery under route-audio-bridge-20261016.


A narrower full-function capture reproduces the poll-2387 explosion: shockwave/expsprite creation loads 11 texture frames. texLoadSprite includes 11.13 ms and its texture ROM section reads include 8.35 ms; decompression itself is about 1.95 ms. These inclusive profile times overlap and are not summed. A private opt-in candidate lets a validated immutable-ROM PI read complete on the next Count tick after its host copy, retaining normal register/interrupt delivery, validation and unchanged FlashRAM timing. This removes the simulated cartridge-bus delay only in native elapsed-time mode; Original retains its existing model. It is not selected or delivered until unprofiled comparisons and compatibility checks establish the result. Evidence: explosion-decompression-profile and native-rom-completion-build.

The first ROM-completion fixture build stopped on signed/unsigned optional-value comparisons under warnings-as-errors; no capture ran and source/runtime were restored. The corrected unsigned comparisons pass. Four alternating unprofiled, muted native replays compare the opt-in next-tick ROM completion with the existing bus duration using the same compiled candidate. All preserve copied profiles and the selected desktop package. Their post-start native paths can diverge, so route/event identity and every missed update interval are retained rather than treating a lower median as complete acceptance. Evidence: native-rom-completion-abba and native-rom-completion-review.

The second control stayed at two VIs for the explosion despite a 36.815 ms root duration, so the initial all-controls-miss promotion assertion refused before changing source. Preserve that mixed outcome. Both candidate explosion roots are shorter (33.198/33.974 ms versus 48.161/36.815 ms), all four have the same 45-to-48 actor-count change, and both candidate combat windows have no three-VI updates or audio errors. Native paths and scheduling phases differ; these results admit integration/compatibility qualification of a loading-latency improvement, not a universal no-dip claim. Final default-policy and Original comparisons are still required.


## Native ROM completion qualified (2026-10-16 UTC)

The final integrated binary passes Original and recorded-clock correspondence, including an explicit fast-ROM request that stays on reference timing during recorded replay. Final native 120/360 combat windows each contain 749 two-VI intervals and no missed intervals; audio under/overruns, rejected submissions and payload mismatches are zero. The six-minute capture remains in Goldwood after a death restart rather than reproducing the owner late-cave route. It retains three gaps: death/reload, a 67.115 ms interval containing 40.72 ms graphics submission/39.60 ms full synchronization, and a 49.96 ms cold-overlay load. The 67 ms update performs no ROM reads, so its renderer path is separate work. The native ROM improvement is qualified for local packaging, not a universal no-20-FPS resolution. [Policy, measurements and compatibility](../native-rom-loading.md); [binary qualification](../../../evidence/high-refresh-native-rom.json).


The High Refresh Preview and Jitter Capture desktop links now select the native-ROM package: source 8277a80611e73c0dc57285e32e0761e9b7519e41, runtime SHA-256 5f902049bd3e4a991a020f17e63a51d0291f28709057a9aa7efc03097692764b. Launcher/setup checks pass; all 40 checked profile files and shortcut arguments are preserved. Earlier audio and bridge remedies are included. Owner listening/visual acceptance, later renderer/load hitches and the broader no-20-FPS goal remain open. [Qualification and delivery](../../../evidence/high-refresh-native-rom.json).


The qualified native-ROM binary reproduces the later hitch at VI 15347/poll 7620: 67.307 ms between update returns, 41.834 ms graphics submission, 0.409 ms interpolation preparation and 40.371 ms inside RT64 fullSync. The six measured waits total only 0.7247 ms; the submission owner records 153,826,280 decode-stage thread cycles. This narrows the repeated hitch to CPU-side RT64 work rather than interpolation preparation, without excluding unmeasured locks or scheduling. A private stage-attribution build is being captured; the desktop package remains unchanged. Evidence: late-renderer-attribution and rt64-stage-candidate.

The exact installed binary repeats the late poll-7620 hitch (66.941 ms interval; RT64 synchronization 38.059 ms), while two stage-instrumented builds do not. All use the same input/save but native timing can diverge. A clean rebuild with all added stage timers removed is being compared before attributing the difference to instrumentation or build provenance. No additional renderer optimization is selected. Evidence: late-renderer-cross-capture-review and clean-renderer-rebuild.


External CPU sampling reproduced the renderer hitch without source instrumentation and observed unsigned-64/bool unordered-map growth. The pinned DrawCallKey hashes all 88 object bytes, although its members occupy only 84; fromDrawCall does not initialize the four tail bytes. A regression deliberately varies those bytes and fails against the original dependency. Its equality also returns raw memcmp, reversing logical equality and reading the same padding. The candidate hashes/compares the checked member range and avoids negative cache entries when the material-preset library is empty. Dependency source remains pristine through a hash-guarded build-tree patch. Runtime hitch and reference comparisons are pending. Evidence: external-renderer-sampling-review, sampled-hitch-time-correlation and draw-key-fix.


The deterministic draw-call key correction passes failing-then-passing regression checks, renderer interpolation tests, exact Original/recorded comparisons and final 120/360 native checks. The previously repeated poll-7620 renderer hitch is absent in the unprofiled corrected replay; restart/load gaps and full-route acceptance remain open. [PERF-003](renderer-cache-key-instability.md) records the cache defect, observer effects and bounded remedy.


Local delivery: both High Refresh Preview and Jitter Capture desktop links select source 27ddef0a5ed4afd1f1f9c26a5590b590cda5fe8e, runtime SHA-256 db65b460f399b550ca8a3ad0d98f5374d5e00a95dc4a7073edfa2649fa6772c6. Manifest and launcher/profile checks pass; all 40 checked profile files and shortcut arguments are preserved. Includes native audio, bending bridge interpolation and ROM loading improvements. Owner live acceptance and broader restart/load gaps remain open. [Current qualification](../../../evidence/high-refresh-draw-key.json).


## Longer owner acceptance capture (2026-10-16 UTC)

The owner reports that dips are no longer visually apparent in the draw-key preview and requests a longer recorded measurement. This is subjective improvement, not a clean quantitative result. A fresh owner-driven capture uses the delivered runtime unchanged (SHA-256 db65b460f399b550ca8a3ad0d98f5374d5e00a95dc4a7073edfa2649fa6772c6), 120 FPS with VSync, existing controls/save, and asynchronous frame/VI/update/audio diagnostics. Full function profiling remains off. Startup/idle, loading/death transitions and active gameplay will be reported separately; every observed gap remains in the evidence. The detailed gameplay stream has an existing 256 MiB ceiling; unbounded game-update timing, final counters and rotating frame CSVs must be checked for full-run coverage before any clean result is claimed. Private evidence: final-measurement-20261016. Recording/review pending; no release claim.


## Longer owner capture completed (2026-10-16 UTC)

The owner reports no perceptible gameplay slowdown, with choppiness and audible interruptions during the Goldwood exit cutscene. This is not a full clean acceptance. The 21-minute recording retains all 131,288 presentation records with no queue drops, but the detailed gameplay/audio trace reached its 256 MiB cap at about 18:27. No unbounded game-update rows or final audio-health totals were written in the live path. The earlier expectation of full-run update/audio coverage was incorrect. Thus later cutscene audio cannot be cleared using the earlier zero counters. Preserve the final report as open; review timing and reproduce the exit under bounded muted tests. Private evidence: final-measurement-20261016, measurement-review.json. The native cutscene/bridge physics conclusions are unchanged.
