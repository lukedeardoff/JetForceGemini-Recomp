# Phase 9 acceptance tracker

Status: open. Historical route evidence is described below; it does not certify the current preview.

Authority: the [project roadmap](JFG_RECOMP_MASTER_PLAN.md), G6/M4.
This tracker does not replace or weaken the roadmap requirements.

## Scope and evidence

The candidate is the recorded Goldwood route. Its preserved long replay ends
at retrace 40,452; reaching that input limit is not, by itself, slice completion.
An explicit in-game completion predicate and subsystem/overlay coverage matrix
are still required. Current runtime boundaries are summarized in
[project status](../status.md).

The preserved full-render route reached 40,452 retraces with zero unsupported
accesses and repeated state/journal hashes. It exercised shots, enemy kills,
death/restart, and persistence writes. Those counters establish exercised paths,
not complete gameplay or save/relaunch acceptance. Detailed run records remain
in [Git history](../README.md#historical-material).

| Requirement | Current evidence / remaining work |
|---|---|
| Control, enemies, weapons, collision, camera, animation, particles, audio, cutscene, level transition, save | Interactive Goldwood and recorded scenarios exist; bind each requirement to verified route checkpoints and enumerate missing coverage. |
| Native and emulator replays | Input v2, native runner, BizHawk Lua capture, and hash comparator exist; approved cross-runtime checkpoint parity is not established. |
| Canonical checkpoints | Per-retrace native hashes exist; raw pointer-bearing hashes alone are not canonical cross-runtime state. Resolve timing and representation differences without concealing behavioral divergences. |
| All active overlays instrumented and reloaded | Runtime diagnostics exist; route-specific overlay denominator and reload coverage remain to be published. |
| First-divergence fixing and documented patches | Diagnostic and hardening documents exist; retain a replay/checkpoint regression for each accepted fix. |
| Active-subsystem function corpora | Phase 5 boundary corpus exists; demonstrate slice subsystem coverage and the roadmap's cumulative minimum of 100 captured functions at M4. |
| Reset to slice completion | Long replay exists; certify the gameplay endpoint, not merely process exit at the requested retrace. |
| Mid-slice save, exit, relaunch, resume | Phase 8 persistence evidence exists; bind a passing recovery scenario to this slice's progression state. |
| Death/retry, pause, controller loss | Manual evidence and route generators exist; require passing automated slice-specific outcomes. |
| Valid varied RNG seeds and transition fuzz | No complete acceptance bundle established. |
| Visual and audio references | Phase 7/8 references do not substitute for designated slice checkpoints. |
| 100 consecutive deterministic completions | Only the 1,794-retrace startup batch has passed 100 runs. It is not a completed gameplay route and does not close G6. |
| Deliverables | Complete slice replay, subsystem corpora, route dashboard, and revised finishable-game estimate remain acceptance deliverables. |

## Execution order

Maintainer priority update (2026-09-06): begin the enabling
[Phase 9.5 autonomous exploration workstream](phase9-5-autonomous-exploration.md)
next, while this gate remains open. Its first useful deliverable is state-aware
Goldwood navigation/combat and alternate-branch exploration without human input,
not another fixed-recording replay. The acceptance obligations below are unchanged.

1. Preserve the current full-route baseline with exact input, initial save,
   binary identity, progress, and outcome; establish its actual gameplay coverage.
2. Resolve the earliest native/oracle disagreement using matched initial state
   and defined sampling points. Do not approve an offset merely to obtain a pass.
3. Define and verify the missing route endpoint and recovery scenarios, reference
   checkpoints, overlay coverage, and subsystem corpora.
4. Run perturbations and 100 complete replays; reject incomplete or malformed
   hash streams even when two such streams happen to match.
5. Publish the coverage result and revised Phase 10 estimate. Close Phase 9 only
   when all required evidence exists.

## Faster-loop work requested by the maintainer

Per-retrace dumps and a parallel determinism runner are implemented. Runtime
snapshot/resume, automatic crash minimization plus ASan replay, and checkpoint
perturbation soak are not claimed implemented. A resumable snapshot must include
native runtime execution state: copying RDRAM does not restore parked stackful
guest execution. Until that is solved, input replay remains the recovery method.
