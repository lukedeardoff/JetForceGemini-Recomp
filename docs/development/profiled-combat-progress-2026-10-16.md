# Profile-directed combat optimization, 2026-10-16

The no-20-FPS goal remains open. The installed profiled-math preview already contains the earlier compact timing and bounded resident-math improvements. This continuation adds qualified diagnostics and tests further candidates; it does not declare all remaining drops fixed.

## What the measurements establish

Game updates and synchronous graphics submission can execute on different host threads; some recorded/Original paths instead nest submission on the root owner. A later native capture shows both thread-cycle totals increasing in slow updates. Renderer fence/texture/queue waits explain only part of submission time. Wall durations include scheduling and blocking; nested costs are never added twice. Windows OS CPU-time deltas are coarse here, so use aggregate observations and preserve cycle units.

The permanent profiler now emits snapshot/decode/writeback timings, six renderer wait totals/counts, and actual submission-owner cycle/CPU-time observations. Its analyzer pairs repeated scene keys in order and reports incomplete or invalid joins. Ordinary F8 FPS logging leaves this expensive detail disabled. [Controls and interpretation](function-profiling.md); [qualification](../../evidence/high-refresh-submission-profile.json).

## Candidate decisions

| Candidate | Correctness evidence | Decision |
|---|---|---|
| Direct snapshot memcpy | Four exact recorded state comparisons | Unselected: about 0.1 ms per-update component saving, no repeatable overall improvement |
| Direct point-in-region predicate | 53,272 shadow answers; 76,699 ABI calls; 200,000 synthetic cases | Unselected: modest native saving, slow updates remain |
| Direct line-intersection predicate | 971,701 supported synthetic cases; 179,256 recorded ABI calls | Unselected: native overall improvement inconsistent |
| Closed collision BSP/arithmetic operation | 50,000 raw and 50,000 mixed-fallback fixture cases; 256 live shadows; exact recorded and long Original fallback checks | Private bounded native candidate; live timing comparison and selection tracked below |
| Windows thread-dispatch profiling | Owned-thread fixture succeeds without hardware counters or driver changes | Private diagnostic; runtime validation tracked separately |

Real-time comparisons share a verified initial state where stated, but their later paths and host scheduling can diverge. Exact recorded comparisons verify preserved behavior and same-workload costs; replaying an old timing transcript is not native FPS acceptance.

Private artifacts, generated code and captures remain on D:. Failed and inconclusive attempts are preserved. The production guard remains enabled with the same investigation identity and counters. The eight-hour extension changes only admitted allowance.

Further details and historical evidence: [combat slowdown](issues/goldwood-combat-slowdown.md), [frame diagnostics](issues/frame-diagnostics.md).


## Later scheduling and collision checks
The corrected private Windows dispatch collector accumulates wait-reason bits through nested readers. Recorded capture preserves exact state/instructions/interpolation; a native 120 run has 1,250 selected updates, 20.667 ms median, 24.732 ms p95 and 27.808 ms maximum active duration, with all 1,249 measured update intervals at two VIs and no audio or unsupported-access errors. This bounded run is not universal no-dip acceptance. Earlier slow captures remain relevant. Dispatch-cycle observations closely track QueryThreadCycleTime in this run; wait bits/counts do not provide wait duration or a CPU-time percentage. Evidence: thread-dispatch-profile-review-nested.
The first larger collision shadow runtime builds after suppressing the generated code's existing unreachable-path warnings in that private translation unit only. Live shadow comparison then detects a difference and fails closed. No replacement is enabled. The initial in-memory trace was lost on fail-closed exit; the next diagnostic explicitly drains it and retains original/candidate CPU, stack, collision globals and floating environments plus private entry memory for isolated reproduction. This failure is retained, not treated as a qualified candidate. Evidence: native-bsp-shadow-build, native-bsp-shadow-build-fixed, bsp-shadow-recorded-fixed, native-bsp-shadow-diagnostic-build and dispatch-and-bsp-shadow-review.


### Bounded collision candidate reaches live comparisons
The corrected live shadow comparison passes 256 sampled calls and exact final replay state. The earlier mismatch lay outside the helper's stack write footprint; the first changed byte was 1,925 bytes above entry SP. The mixed native/original fixture also passes all 50,000 cases, exercising 121,613 native nodes and 37,156 fallbacks with varied budgets.
The private runtime admits only the selected overlay in native elapsed-time mode, with Original, recorded-clock prefixes and instruction observers retaining reference execution. Each entry allows up to 32 nodes/128 edges, at most 16 edges per leaf and 24 recursion levels; a 50-microsecond deadline is checked before admitting each node. That check is not a hard wall-time guarantee, especially across OS preemption or original fallback. Any native work invalidates the clock fast path on return so the next generated instruction refreshes clocks/events. The reference fallback begins at the current node and preserves partial parent work. The exact replay and long 17,000-VI Original checks pass before live comparisons. Evidence: native-bsp-bounded-build, native-bsp-bounded-recorded, native-bsp-bounded-prefix120 and native-bsp-bounded-review.


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

The explicit level/death markers distinguish those calls from an unclassified gameplay hitch. A cold-overlay correlation alone is not assigned a cause. Late crate/light/bridge and shadow visual acceptance remain pending. The candidate is qualified for local preview packaging, not released or claimed as an all-level no-dip fix. [Compatibility and fixture](native-collision.md); [final binary evidence](../../evidence/high-refresh-native-collision.json).


### Bounded collision preview delivered locally
Both High Refresh Preview and Jitter Capture desktop shortcuts now select source commit 8f94f148ec367e4a221679a40255f12e6353af07 and runtime SHA-256 34ce90c9a11bba75a9ae47f3ae3d8a40d635b20357bf8c52391f4bd015a6bff1. Package and launcher/runtime setup checks pass; shortcut arguments and all 40 checked profile files are preserved. Previous package and links remain available for rollback. The qualified collision optimization, expanded submission diagnostics, cadence analyzer and explicit transition markers are included. Detailed function profiling remains off for ordinary play. This is a local preview, not a release or universal no-20-FPS claim. Late crate/light/bridge jitter and shadow cadence still require owner visual review. [Current evidence](../../evidence/high-refresh-native-collision.json).

The production guard remains enabled with all investigation counters and earlier evidence retained. Further gameplay optimization should be guided by a fresh owner capture; hidden playback does not settle the remaining visual reports. The full profiler now identifies pathfinding and the point-in-region wrapper as remaining expensive groups, but its per-call overhead biases small-function percentages. Previously rejected isolated-predicate experiments remain documented; they are not silently promoted to fixes.


### Final delivery audit

The final binary also completes a muted capture-mode comparison with actor, interpolation, poll and progress logging enabled (recorded inputs replace live input recording). Its combat median active duration is 20.392 ms; extended update intervals are {'2': 1229}. No new audio-buffer errors or unsupported accesses occur. This bounded comparison does not remove the observer-cost caveat for full function profiling.

The cadence analyzer now includes transition calls anywhere between consecutive update returns, including calls outside either measured root. Final normal-Play gap associations: [{"vi": 6588, "delta_vi": 64, "transition_kinds": [], "latest_transition": {"stamp_us": 109416093, "vi": 6494, "poll": 3170, "kind": "death_restart", "age_ms": 1568.366}}, {"vi": 6591, "delta_vi": 3, "transition_kinds": [], "latest_transition": {"stamp_us": 109416093, "vi": 6494, "poll": 3170, "kind": "death_restart", "age_ms": 1611.178}}]. These are actual observed transition entries, not causes inferred from a level number alone. Further owner visual review is still required for the reported late scenery/shadow jitter; a fresh route is also needed to judge combat under live inputs.
