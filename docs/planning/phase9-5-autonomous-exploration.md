# Phase 9.5: Autonomous gameplay exploration

Status: acceptance requirements open. Bounded capabilities exist; see
[project status](../status.md) for their limits.

## Purpose

Remove the maintainer as the routine gameplay driver: a game-specific bot
that navigates, fights, explores alternate branches, recovers from failure and
generates reproducible native/reference tests. Repeating a fixed recording is
not completion, and no LLM inference is needed during gameplay. This enabling
workstream supports the open [Phase 9 slice](phase9-acceptance.md) and later
campaign expansion without weakening any gate. Missing map or mechanic
knowledge is an engineering task, not a routine request for manual play.

The source already has input recording/playback, native replay, determinism
comparison, state observation, checkpoint tools, generated navigation and
bounded failure triage; the launcher's [navigation mod](../features/navigation-mod.md)
adds live exploration. Still required: general varied-path exploration,
combined recovery coverage, frontier scheduling, a qualified failure-reduction
and sanitizer workflow, and native snapshot equivalence.

## Architecture

| Component | Responsibility |
|---|---|
| Oracle adapter | Read BizHawk state, apply input at defined polls, advance bounded execution, save/restore reference checkpoints. |
| World model | Decode and validate player position/facing, camera/control mode, health/ammo, actors, enemies, pickups, exits and progression; reject stale observations. |
| Navigation model | Build a graph of rooms, connections, hazards, jumps, one-way links, doors and prerequisites from decoded data, measured exploration or authored waypoints. |
| Action controller | Closed-loop move, aim, fire, jump, interact, collect, pause and retry, with calibrated camera-relative movement. |
| Exploration manager | Choose uncovered reachable objectives, keep checkpoints and route history, detect stuck states and bound retries. |
| Verification worker | Replay generated routes natively, compare reference checkpoints, classify failures and keep regression artifacts. |

Start with a small Lua emulator adapter and a testable Python planner over a
versioned protocol with sequence numbers, acknowledgments, timeouts and
neutral input on planner failure. Benchmark the transport before choosing
files, pipes or sockets. Track unknown connections explicitly and do not promise
discovery of hidden paths without an independent coverage denominator.

## Correctness and replay contract

- Observe memory but progress only through ordinary controller input:
  acceptance routes must not teleport or write health, ammo, progression or
  physics. Fault-injection experiments are labeled and excluded.
- Pin ROM, emulator/core/settings, native binary and source, initial saves and
  accessories, planner/map version and seed. Keep concrete input-poll samples
  as well as semantic actions and outcomes.
- Strict native/reference replay uses the same declared input schedule; report
  scheduling divergence when load cadence makes a recording nonportable. An
  adaptive bot running natively is a robustness test, not differential evidence.
- Compare validated canonical fields at completed-update boundaries. Never skip
  mismatches, offset traces or mask RNG differences.
- Classify reference-bot failure, harness failure, input timing mismatch,
  native crash, native divergence and incomplete evidence separately. A timeout
  or clean exit never proves objective completion.

Reference checkpoints include emulator, planner, frontier, RNG, held-input,
counter and pending-action state, and must prove restore-and-replay
equivalence. BizHawk snapshots cannot load natively, so native verification
replays from the pinned initial state; report that prefix cost. Native
snapshots need guest execution, scheduler, queues, timers and device state;
RDRAM alone is insufficient. Each run bundle holds a manifest, completion
predicate, initial-state digests, inputs, actions and observations, checkpoint
lineage, traces, coverage, logs, exit classification and first-failure
details, persisted incrementally. ROM-derived bodies stay private.

A supervisor owns only its launched processes and supplies heartbeat, stop,
time/disk/retry limits and isolated workers. Stop on stale state, incompatible
checkpoints, exhausted budget, missing heartbeat or user pause, keeping
evidence. Never terminate unrelated sessions.

## Milestones and gates

**A. Reliable control.** Define observation/action schemas with provenance;
reach an explicit Goldwood gameplay predicate from a fresh game through
state-aware actions; calibrate movement and aim; prove checkpoint restore
equivalence. *Gate:* ten repeat runs reach the declared state without human
input, and restored continuations reproduce approved state.

**B. Goldwood proof (first useful deliverable).** Before running, name a bounded
area with one encounter, one pickup and two reachable destinations with
completion predicates. Navigate, fight with normal health and ammo, collect
the pickup, reach each destination from a checkpoint, and exercise natural
death and retry. *Gate:* ten seeded jobs complete their objectives within
predeclared retry budgets with zero intervention, covering both destinations
and death/retry; one frozen route reproduces three times in BizHawk with
matching checkpoints; a native replay and differential report are generated
automatically. A native divergence remains a blocker. Report elapsed time,
retries, interventions, coverage and native prefix cost. The deliverable is a
command that starts or resumes the scenario, plays it and leaves a
reproducible bundle without steering.

**C. Growing frontier.** Extend through the Phase 9 slice: transitions,
cutscenes, save/relaunch, death/retry, pause, controller disconnect and overlay
reloads, tracking reachable, covered, blocked and unknown branches with
prerequisites and character/inventory variants (room visits are not
progression coverage). *Gate:* every declared slice objective has a generated
reproducible route or an explicit blocker.

**D. Unattended regression factory.** Archive failures, find the earliest
reproducible failing prefix, reduce without assuming monotonic failure, keep
original inputs, rerun under a pinned ASan build (a symptom alone is not a root
cause), add seeded perturbations and run restartable resource-limited batches.
`scripts.phase95_native_failure_triage` already retains failing native replays
with bounded prefix probes; seeded faults, restartable reduction, automatic
ASan reruns and perturbation coverage remain. *Gate:* a seeded fault yields a
reproducible report, an interrupted batch resumes, and Phase 9's 100
consecutive deterministic slice completions pass. Varied exploration and
startup repeats are reported separately and never substitute.

**E. Native resume.** Inventory all native execution state, then prove
uninterrupted-versus-restored equivalence across combat, loading, saving and
retry with incompatible-build rejection. This does not block milestone B.

## Order of work

1. Finish milestone A: control and aim observations, input isolation and
   watchdog behavior, then ten fresh autonomous startups.
2. Define the Goldwood scenario manifest and decode missing health/ammo,
   hostility, dialogue/progression and map fields; proximity to an NPC is not
   a completed interaction.
3. Implement closed-loop navigation, combat, interaction and natural
   death/retry, keeping checkpoints and lineage at each verified frontier.
4. Run milestone B and integrate native replay; finish input/update-boundary
   validation before approving parity.
5. Expand the frontier and regression factory, then native resume.

Do not expand to whole-game exploration before the bounded loop works. This
plan does not itself start implementation or unattended batches; execution
runs under the [production guard](autonomy-progress-guard.md). Full-game
acceptance is governed by the [roadmap](JFG_RECOMP_MASTER_PLAN.md).
