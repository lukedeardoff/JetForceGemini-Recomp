# Detailed update and rendering profiler

This opt-in developer profiler records function costs and caller relationships for individual game updates, alongside graphics, GPU and scheduling observations. It is disabled by default. It does not repair combat slowdowns.

## Capture controls

Set these variables in the environment of a developer runtime process, using a new private output path. Automated tests use copied profiles and muted audio.

| Variable | Meaning |
|---|---|
| JFG_GAMEPLAY_TRACE | Private gameplay TSV output path. |
| JFG_GAMEPLAY_TRACE_MEMORY=1 | Buffer up to 256 MiB in RAM; write the TSV on normal finish. Required for this profiler. |
| JFG_DIAGNOSTIC_FUNCTION_PROFILE=1 | Major update stages: entities, drones, pathfinding, animation and level drawing. |
| JFG_DIAGNOSTIC_FUNCTION_PROFILE=2 | Every dispatch reached on the root update's host thread, aggregated by function and caller per update. |
| JFG_DIAGNOSTIC_PROFILE_FIRST_VI | First VI at which to start profiling a root update; default 3300. |
| JFG_DIAGNOSTIC_PROFILE_LAST_VI | Exclusive start cutoff; default 4500. At most 3600 VIs per capture window. |
| JFG_DIAGNOSTIC_UPDATE_CYCLES=1 | Independent root-thread cycle observation paired with update-work. |
| JFG_DIAGNOSTIC_GPU_COSTS=1 | Framebuffer and regular-workload GPU timestamp queries; requires function profiling. |

A root that starts inside the window is measured through its return. The VI window includes elapsed game/menu time; select it from a prior route capture. Function recording includes native dispatch paths as well as generated bodies. It covers the owning update thread, not all renderer/audio threads. Process-cycle deltas and guest-baton wake measurements provide separate wider observations.

Normal finish writes the memory trace. A crash/forced termination can lose that buffer. Overflow writes an explicit limit marker and stops recording; it never silently wraps. The optional memory mode opens/truncates the selected output at startup and defers gameplay rows until finish. Other diagnostics such as the FPS CSV can still use their own buffered writer.

## Records and analysis

All gameplay payload fields remain hexadecimal; timestamps are decimal microseconds. New records are additive to the existing TSV format:

- function-profile: end VI/poll, start VI/poll, host thread, target, selected parent, call count, inclusive/exclusive wall nanoseconds, inclusive/exclusive thread cycles, known wait microseconds, observed instruction delta, maximum call wall/cycles, validity.
- function-profile-end: the five identity values, mode, validity, reached call count, emitted edge count and capacity-overflow flag.
- function-profile-host: identity, process-cycle validity/delta, guest handoffs, baton wakes and accumulated wake latency in nanoseconds.
- function-profile-emission: identity, validity, record-emission wall nanoseconds and thread cycles. This emission occurs after the measured root interval.
- submit-detail: VI/poll/scene sequence, history preparation and RT64 RDP synchronization microseconds, five history-stage microsecond totals, loaded vertices and drawn triangles.
- Existing submit, update-work, update-thread-cycles, gpu-framebuffer-time and gpu-workload-time records provide correlated observations.

The five history stages cover geometry keys, matrix/rigid/shape history, semantic history, previous-world endpoint conversion, and coverage diagnostics. Their sum can be smaller than total preparation because initial setup, publication and unmeasured boundaries remain outside those intervals.

Run:

    python scripts/analyze_function_profile.py path/to/trace.tsv --first-vi 3500 --last-vi 6000 --output profile.json

An optional --symbols JSON file maps hexadecimal addresses to function names. The report includes every measured function in its aggregate and the 20 slowest updates with caller edges, CPU/wall costs, graphics stages and host/GPU records. It verifies that exclusive CPU and wall totals sum to each root. Missing, truncated, invalid or capacity-limited observations are reported explicitly. Repeated graphics scene keys are paired by emission order.

## Interpretation and qualification

Inclusive costs overlap. Exclusive CPU percentages describe only the profiled thread and include instrumentation overhead. Wall durations include blocking, preemption and other threads' work while blocked. Known wait is a measured subset of waiting, not a complete OS scheduling classification. Process cycles include all process threads and are not added to root-thread cycles. Never convert QueryThreadCycleTime counts into milliseconds.

GPU figures describe completed timestamp observations during the update window; the associated work can start earlier. They are not added to CPU wall time. RT64 full-sync wall time includes history preparation; submit-detail separates that preparation from the subsequent RT64 synchronization operation.

Full mode has substantial observer cost. In one exact recorded workload the disabled control measured roughly 25 ms median update work, stage mode roughly 26 ms, and full mode roughly 36 ms. Tiny frequently called functions are especially sensitive to measurement overhead. Use full mode to locate call/algorithm costs, then confirm optimization savings and native frame pacing with profiling disabled. Native runs using the same input file can still follow different timing-dependent game paths.

Fixtures cover nested/recursive scopes, repeated calls, counter validity, thread mismatch, depth overflow, exact exclusive totals, memory buffering and limits, and CPU/host/GPU/submission correlation. Recorded comparisons verify RAM/save/instruction/interpolation agreement; an Original-mode comparison also checks the disabled final build. See [PERF-001](issues/goldwood-combat-slowdown.md), [PERF-002](issues/frame-diagnostics.md), and the [qualification receipt](../../evidence/high-refresh-function-profile.json).


The qualified native preview also has bounded resident-math dispatch. Function scopes include this path, and resident-math rows count angle/difference/trig admissions per update. The final combined build passes recorded correspondence with full profiling enabled and disabled, plus native 120/360 observations. See [native math compatibility and validation](native-resident-math.md). The earlier profiler-only receipt remains historical; the [combined-build receipt](../../evidence/high-refresh-resident-math.json) identifies the newer binary.


## Submission cost breakdown
Function-profile modes also emit three records for every graphics submission, without enabling the broad interpolation-comparison trace. Repeated VI/poll/scene keys are correlated in event order.

- submit-host: VI/poll/scene sequence, snapshot-import, display-list submission, and CPU-writeback nanoseconds.
- submit-sync: VI/poll/scene sequence, six wait nanosecond totals followed by their six call counts. Order: draw upload, transform upload, render upload, framebuffer fence, texture upload, workload queue.
- submit-thread: VI/poll/scene sequence, owning host thread, validity, three stage cycle totals, and three OS CPU-time deltas in 100-nanosecond units. Stage order matches submit-host. Invalid observations carry zero counters.

The analyzer groups submission CPU observations by actual host thread and marks when that thread is also the root update owner. Those costs then overlap the root's CPU totals. Other thread totals are separate observations, not extra wall time. OS CPU-time accounting can be coarse for individual short stages; aggregate it across several updates and retain cycle counts as cycles. The six waits are subsets of submission/RDP synchronization and must not be added to enclosing time. All original waits remain in place. Collection is off in ordinary FPS logging.

Source integration and its final qualification are tracked in PERF-002; earlier private-capture evidence does not automatically qualify a rebuilt executable.


## Update and presentation cadence report
The lightweight route trace can be reviewed without full per-function profiling:

    python scripts/analyze_update_cadence.py trace.tsv --first-vi 3540 --frames frames.csv --target-fps 120 --output cadence.json

The report retains every observed interval above the selected update period (two VIs by default), with active/wait time, root thread cycles when available, nearby level/state and cold-overlay activity, plus native collision counters. Loading evidence is a correlation, not an automatic exemption or cause. Missing counters remain unknown. Frame CSV segments report present-return intervals, scene age, queue drops and missing rows separately; they do not substitute for game-update timing or verify monitor scanout. Repeat --frames for rotated segments. Partial or limited traces cannot prove that no slowdown occurred.


Route traces also emit route-transition at the existing mainChangeLevel and mainRestartAfterDeath dispatch observations: VI, controller poll, target function, first two raw argument words, and that transition's cumulative call count. The cadence report correlates these explicit calls with long updates. A death-restart entry identifies reload work more directly than cold overlays alone; it is not a per-hit or damage-severity log. Collection is conditional on an existing gameplay trace.

Transition correlation covers the complete interval between consecutive update returns as well as the measured root body. A transition dispatched between roots therefore remains visible. These calls are recorded separately rather than assigned to a root that did not execute them.

Each update also carries the most recent transition call and its age. This preserves delayed restart context without claiming that a prior call occurred inside the measured update or caused every later stall.


ROM loading observations are available without full function profiling. The rare rom-dma record reports per-update reads, copied bytes, reference bus ticks, scheduled ticks and native-read count after VI/poll. It distinguishes synchronous host copying from deferred emulated cartridge completion; durations are scheduling counters, not thread CPU measurements. See [native ROM policy](native-rom-loading.md).


Private RT64 stage instrumentation altered a padding-dependent cache defect and hid its hitch. A clean rebuild restored the event; external thread sampling located hash-table growth without modifying the function. Treat instrumentation-induced disappearance as an observer effect until an independently failing regression and unprofiled corrected run support a remedy. See [PERF-003](issues/renderer-cache-key-instability.md).
