# High-refresh checkpoint, October 15, 2026

Status: paused at the owner's request after completing the current comparison. No new gameplay/build experiment is left queued. This is a checkpoint, not a completed performance fix.

## What is established

- High-refresh presentation preserves the original game-time model. Rendered FPS and game-update FPS are measured separately; 120 rendered frames do not prove 30 stable game updates.
- The owner reported Juno's thigh seams/delayed dark lighting absent at 120 FPS, and subsequently reported gems and the first Goldwood ground drones looking good. Those observations apply to their recorded previews, not all scenes or rates.
- Native elapsed-time scheduling removed the earlier modeled console-CPU cap. Reduced timing bookkeeping and renderer/history work improved performance. Parallel semantic history at runtime source checkpoint e90731b78dc145854e9d87b86958756659e84cc0 preserves the compared state and saves roughly 0.6-0.7 ms in its measured component.
- That source checkpoint is distinct from the installed desktop preview. The preview remains the previously installed high-refresh-history-storage-20261015 package. None of today's compiler/helper experiments was installed.
- Detailed combat profiling identifies substantial drone, pathfinding and other entity CPU work. Renderer submission/history and waits also contribute. Sample/cycle percentages are not wall-clock milliseconds.

## What remains open

1. **PERF-001: real 20 FPS combat updates.** Native runs still contain three-VI updates. The goal is not achieved. Current controlled tails are around 43-44 ms against a 33.3 ms update budget; medians alone hide the problem.
2. **RENDER-006: late bridge, exit-light and crate jitter.** Bounded camera/matching corrections pass recorded comparisons, but the owner's full physical tutorial route and crates are not fully qualified.
3. **RENDER-007: shadow cadence.** Clipped receiver/mask geometry lacks a qualified smooth interpolation fix.
4. Damage hitches, broader 360+ FPS robustness, and the complete owner route remain acceptance gaps. Pickup-particle correction was delivered locally but still requires its recorded live follow-up.

## Last completed comparison

The cold-helper candidate and baseline each ran twice using the same recorded guest clock and input route. All four preserved exact game RAM, save, instruction counts and interpolation decisions. Ordinary source/runtime were restored.

| Run | Combat active median, ms | Combat active p95, ms |
|---|---|---|
| baseline-a | 29.361 | 42.847 |
| candidate-a | 27.601 | 43.872 |
| candidate-b | 27.571 | 43.71 |
| baseline-b | 37.753 | 55.585 |

These numbers describe measured host update cost under a recorded clock. They do not prove real-time game FPS or eliminate the slower tail.

The exceptional floating-point classification ran 8,192 cases for each candidate. Differences were confined to NaN sign/payload bits; other numeric and integer/control state, timing state, hook counts and floating exception flags matched within that fixture. This clarifies prior strict failures; it does not establish byte equivalence or qualify a release.

A new build audit found that manually compiled candidates changed floating-point and C-language options as well as the intended optimization. The normal command uses /fp:precise and /std:c11; the candidates use /fp:strict and omit /std:c11. Large machine-code-size reductions are real observations, but their cause remains unisolated. Prior combined-build throughput results must not be described as a pure compiler or pure helper improvement. Microsoft's [floating-point option documentation](https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170) explains that these modes provide different floating-environment guarantees; this is a compatibility consideration as well as a performance variable.

## Reference review disposition

VibinToast's release is not a validated fix donor. Our owner capture inherited debug gameplay overrides and classic controls, so it cannot support a fair default-build quality comparison. The owner explicitly ended upstream debugging; its build was canceled and the clean replacement capture remains unexecuted. No source transplant is selected. See [DIAG-002](issues/reference-owner-capture-contamination.md) and the [pinned review](../upstream/reference-review-2026-10-15-vibintoast.md). Any future reference idea must prove benefit in our implementation.

## Recommended next decision

Prioritize our generated-code/runtime overhead, beginning with a controlled compiler-factor audit. Reproduce the original compiler command, vary only one setting at a time, inspect code size, and compare the same recorded workload. A fixture for the production floating-point settings is also prepared. Neither was executed before this pause.

Then measure the updates that exceed 33.3 ms, separating active CPU execution, renderer waits and host scheduling delays. Use those results to choose a structural reduction in bookkeeping or submission cost. Do not infer a bottleneck solely from mean FPS, a smaller executable, or a different native route. Preserve timing, input, audio and interpolation correctness; finish with native-time combat and full-route verification.

The immediate objective remains stable game updates through combat. Another round of broad compiler swaps or unrelated upstream debugging is not justified without this isolation.

## Resumption and evidence

The operational progress guard must remain enabled. Recover the existing high-refresh-rendering-20261011 investigation and its current production-ledger state before resuming; do not reset counters, create a fresh root or bypass a denial. This pause grants no new attempt or time budget.

Private evidence IDs: cold-original-classified-paired; floating-state-difference-classification; clang-integer-functions-paired; owner-pause-stocktake-20261015. Prepared requests for cold-original-transition-checks, compiler-size-factor-audit and production-fp-mode-classification were withdrawn before execution. Their scripts and existing evidence remain preserved on the evidence volume. Public records contain no ROM, generated game code or personal save data.

Persistent issue records remain the source of symptoms, attempts, verification limits and eventual resolution: [issue index](issues/README.md), [combat](issues/goldwood-combat-slowdown.md), [late objects](issues/tutorial-late-object-jitter.md), and [shadows](issues/shadow-cadence.md).


## Subsequent owner direction

The owner subsequently approved five hours of resumed work and explicitly authorized faster implementations/algorithms even when they are not instruction-for-instruction authentic, provided AI and gameplay behavior remain equivalent. The pause record above remains historical. The guarded continuation preserves the same investigation and counters. Current validation and implementation direction is recorded in PERF-001; the compiler-factor and floating-mode fixtures may now be executed under this new authorization.


The resumed investigation selected the compact original-timing helper for source integration after four exact recorded comparisons, the retained Original reference, and native 120/360 checks. Median recorded combat work improves by about 3.7 ms; remaining slow native updates keep the no-20-FPS goal open. Standard runtime rebuild/package qualification remain separate from this source checkpoint. See PERF-001 for evidence and limits.


The compact-timing standard build was subsequently qualified and delivered to both high-refresh desktop shortcuts. The expanded function/graphics profiler is now a separately qualified source change, disabled by default; it is not installed on those shortcuts. Controlled median improvement remains about 13%, while native slow updates keep PERF-001 open. Current diagnostic controls and limitations are in [function profiling](function-profiling.md).


The detailed profiler led to qualified bounded resident-math dispatch. Exact recorded comparisons show about 1.1-1.5 ms lower median combat work in the two controlled pairs, with a separate integrated confirmation and preserved tested state. The final combined binary passes the Original reference, full diagnostic correspondence and native 120/360 captures; local package delivery is separate. Remaining occasional three-VI updates keep PERF-001 open. The production guard and existing counters remain authoritative; this checkpoint grants no new budget.


The combined profiled-math package has now been qualified and delivered to both desktop links, retaining profiles, arguments and the previous package. Detailed profiling is off by default. PERF-001 remains open; subsequent work should use the new per-update attribution to target collision/pathfinding and CPU-side graphics submission, retaining reference fallbacks and uninstrumented checks. No new investigation root, counter reset or budget extension follows from delivery.
