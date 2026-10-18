# Jet Force Gemini PC roadmap

Build a complete, faithful PC version of Jet Force Gemini, then add optional
modern enhancements. Compatibility and reliable progression come first.
This roadmap defines goals; [project status](../status.md) records
what has actually been demonstrated. No release dates are committed.

## Product goals

1. A finishable single-player campaign with all characters, required progression,
   boss encounters, ending, credits, and reliable saves.
2. Original multiplayer, optional content, menus, controller configurations,
   accessories, and recovery behavior within the declared release scope.
3. Correct original in-game widescreen behavior, including menu selection and
   persistence. PC ultrawide is a separate enhancement.
4. Ultrawide presentation with correct UI, HUD, camera, and culling.
5. Higher display rates with interpolation and pacing that preserve simulation.
6. Stable mod interfaces and documented tools. Readable-source replacement
   requires differential evidence and is separate from the mod interface.
7. A dependable PC experience: setup, controls, diagnostics, packaging, and support.

Windows x64 is the released v1 platform. See the [dashboard](../dashboard.md) for current feature availability. Linux source CI does not establish
a supported Linux or Steam Deck game package. Additional ROM regions and
platforms require their own explicit support and verification decisions.

## Execution and contribution model

[Execution profiles](../adr/0002-execution-profiles.md) separate compatibility
behavior, optional enhancements, deterministic execution, and presentation.
Changing aspect ratio, frame rate, or rendering must preserve compatibility
simulation checkpoints. Required fixes and optional enhancements stay distinct.

Playtesters play normally and [report problems](../playtesting.md). They do not
need to run the internal acceptance program or report successful sessions.
Code changes use focused, reviewed [pull requests](../../CONTRIBUTING.md).

## Delivery sequence and acceptance gates

Completed milestone records retain their original contracts. The broader
requirements below remain obligations where those records provide less coverage.
Historical estimates, staffing assumptions, and day-by-day schedules are in
[Git history](../README.md#historical-material).

| Gate | Required outcome |
|---|---|
| G0: inputs and dependencies | Supported ROM checked locally; data-handling rules, dependency terms, notices, and corpus ownership recorded. |
| G1: reproducible reference | Two clean upstream builds with matching normalized output; CPU/overlay/relocation/RSP denominators; timestamped reference observations. |
| G2: architecture | Every executable CPU section attempted and failures classified; difficult overlay lifecycle and indirect calls; classified RSP programs; real graphics and audio tasks; save/accessory round-trip; owned mitigations for direct hardware and unusual control flow. |
| G3 / M1: CPU link | Every expected executable symbol generated or explicitly excluded with evidence; direct and observed indirect calls resolved; complete overlay inventory; no unexplained boot-path stub. |
| G4 / M2: deterministic boot | Original entry reaches scheduler/VI checkpoints; **50 deterministic boot repeats** agree; reference checkpoints agree; no unhandled call, unexplained failure, silent stub, or unbounded loop. |
| G5 / M3: rendered and interactive foundation | Title and difficult gameplay scenes; **10 consecutive stable reference captures**; renderer-independent simulation; input, audio, save/accessory, filesystem, and restart integration. |
| G6 / M4: complete campaign slice | **100 consecutive deterministic slice completions** with identical approved checkpoints and no crashes; verified gameplay endpoint, reference graphics/audio, overlay coverage, death/retry, pause, controller loss, save/exit/relaunch/resume, and varied-seed/transition tests. |
| G7 / M5: finishable campaign | Independently replayable required segments; **three clean new-game runs to credits**; progression, boss retry, major-transition save/reload, ending and post-game behavior; zero route blockers. |
| G8 / M6: complete compatibility | Passing scenarios for all release-scoped content, original multiplayer, menus, controller counts, saves, and recovery; original widescreen in both states; overlay/branch coverage; **eight-hour soak**, fuzz/stress coverage, no release-blocking parity defect or flaky required test. |
| G9 / M7-M8: enhanced release | Ultrawide/HFR preserve the compatibility baseline; declared platform/feature matrix; package tests, dependency inventory, notices, rollback, and support plan; explicit release acceptance. |

At M4, the active-subsystem corpus must include at least **100 captured
functions**. Earlier risk-weighted kernel coverage is a foundation, not a
substitute. A run ending at its input limit is not a gameplay completion.
The [slice tracker](phase9-acceptance.md) owns the detailed open requirements.

The proposed release floor remains zero open severity-1/2 defects, zero flakes
in required tests, 50 automated route-hours, 20 human play-hours, and a two-week
release-candidate freeze. These are planning criteria pending a declared
release matrix, not measurements already achieved or obligations for each tester.

## Engineering workstreams

- Runtime, overlays, scheduling, device timing, input, audio, and saves.
- Rendering correctness and presentation independent of simulation.
- Replay, first-divergence diagnosis, representative subsystem corpora, and recovery.
- [Automated exploration](phase9-5-autonomous-exploration.md) and
  [bounded engineering automation](autonomous-full-scope-execution.md).
- Installer reliability and [support diagnostics](../development/support-diagnostics.md).

Native snapshots must restore scheduler, thread/continuation, device, and
external state as well as RDRAM, with restored/uninterrupted equivalence and
incompatible-build rejection. Until proved, native recovery uses replay.

## Change and evidence rules

Keep exact build/input/profile identities with results. Diagnose the earliest
qualified divergence; do not insert offsets, remove compared fields, regenerate
goldens, or change tolerances to conceal a failure. A fix needs focused regression
coverage and a fresh relevant replay. Human review remains required for scheduler
semantics, hash exclusions, floating-point policy, tolerances, golden updates,
save semantics, and game-visible production/test differences.

[AGENTS.md](../../AGENTS.md) and the [production guard](autonomy-progress-guard.md)
govern automated work. Preserve investigation accounting and stop at a denial.
Neither this roadmap nor a resumed task reopens a shelved investigation.
Release, publication, and external messages require their own authorization.
