# Engineering automation

The local supervisor in `scripts/autonomy/` owns durable jobs, prerequisites,
leases, heartbeats, attempts, and sealed results. Its mutable state belongs in
ignored `tools/private/autonomy/`; the operational ledger is in the original
checkout ([location](../planning/autonomy-progress-guard.md#production-ledger)).
Read [AGENTS.md](../../AGENTS.md) before running automated project work.

## Mandatory guard

The [production progress guard](../planning/autonomy-progress-guard.md) must stay
enabled. Recover the existing investigation and remaining budget before work.
Run experiments, builds, replays, and coding workers through guarded execution.
Foreground orchestration is not a replacement for a guarded child process.

Never reset counters, rename or re-root the same problem for another allowance,
or run directly after a denial. Preserve original failures, evidence, and user
changes. Stop and report exhaustion. A resumed conversation does not reset the
ledger. Guard-disabled tests use isolated fixture ledgers only.

[Trusted continuations](../planning/autonomy-progress-guard.md#trusted-continuations)
attach a fresh job to an eligible completed attempt while the same investigation
still has allowance. They do not reopen shelved investigations or erase charges.

## Execution model

- `job_store.py`: immutable job specifications, dependency checks, resource
  leases, restart/expiry accounting, and artifact seals.
- `supervisor.py`: isolated source worktrees, scoped workers, bounded commands,
  heartbeat enforcement, validation, and result capture.
- `progress_guard.py`: investigation membership, cumulative budgets, outcome
  grading, progress evidence, and durable stop decisions.
- `scheduler.py`: evidence-dependent task selection and continuation.

Each job pins source, producer tools, applicable inputs, and execution profile.
Queued work is not permission to execute. Workers must not receive ledger write
access, publishing credentials, or an unbounded admission service. One writer
owns each worktree and evidence lineage. Preserve complete original failures
before reduction and keep mutable results outside tracked source.

## Diagnostic and repair flow

1. Establish a reproducible baseline and qualify the observer against unchanged
   input, full update traces, and focused state.
2. Choose a bounded hypothesis supported by retained evidence.
3. Capture and independently measure the relevant word, entry, instruction,
   interval, or device boundary; distinguish raw observations from causal claims.
4. Retain qualified negative results. Follow-up plans must add information and
   must not repeatedly capture the same disproved prediction.
5. Produce a focused regression and candidate fix, then independently review,
   rebuild, and replay it under the same input/profile contract.
6. Grade actual progress. A successful diagnostic or larger report is not a
   gameplay repair or an automatic progress credit.

Existing adapters cover source baselines, candidate feedback, bounded observation,
and several repair/research transitions. The complete self-directed repair loop
and full-game parity remain unaccepted. See the [delivery plan](../planning/autonomous-full-scope-execution.md)
and [project status](../status.md).

## Maintenance and verification

Read-only status checks may inspect the ledger. Do not infer current job state
from dated reports. Use the actual supervisor CLI definitions and `--help` for
task arguments; old run packets are tied to their recorded inputs.

Relevant tests are `tests/test_autonomy_*.py`. The progress-guard acceptance
recipe is retained in its [contract](../planning/autonomy-progress-guard.md).
All agent-launched validation still runs through the supervisor; test fixtures
must not mutate the production ledger.

### Private investigation storage

Owner instruction (2026-10-12): keep new large experiment output, replay captures and private evidence on D:, under `D:/JFG-private/<investigation>`. Preserve source and the existing production guard ledger locations. Relocating retained evidence requires verified copies and stable references; high-refresh history uses directory junctions and per-file SHA-256 manifests. A storage move never creates a new investigation budget or replaces the operational ledger.
