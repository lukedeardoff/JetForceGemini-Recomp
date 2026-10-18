# Investigation-level anti-stall guard

The production progress guard bounds wasted autonomous work across an entire
investigation. It must stay enabled at all times ([AGENTS.md](../../AGENTS.md)).
It is a bounded-waste control in the supervisor, not completion of the
autonomous repair loop or game parity. Implemented 2026-09-27 in
`scripts/autonomy/progress_guard.py`; operator guidance is in
[automation](../development/automation.md).

## Production ledger

The authoritative operational ledger is
`D:/JFG-private/autonomy-production/jobs.sqlite`. In the current main checkout,
`tools/private/autonomy` is a junction to that directory. On 2026-10-15 the owner
requested preservation before deleting the original checkouts. The ledger was
moved using a verified SQLite backup after all jobs stopped; its logical
contents, enabled guard, investigation roots and counters were preserved. The
original operational filename was retired to prevent accidental duplicate use.
The private receipt is
`D:/JFG-retained/legacy-preservation-20261015/production-ledger-migration.json`.

Before autonomous work, recover this ledger, verify its guard is enabled and
recover the existing investigation state and remaining budget. Relocation does
not reopen a shelved investigation or grant new allowance. Historical supervisor
scripts may still refer to the retired location; do not run them unchanged.
Never create a replacement ledger. Guard-disabled tests use isolated fixture
ledgers only.

The 2026-09-27 rollout backed the ledger up to
`jobs.pre-progress-guard-20260927.sqlite`, then migrated and enabled it while
no jobs were active; the continuation update moved it to schema 3 with
protection still enabled. `native-prefix-parity` is anchored to the completed
observation `instruction-observe-e0fadbd8b74d8f92c69a99b3`, so its descendants
share one budget; execution before the rollout was not retroactively charged
or credited. Read the ledger itself, not this page, for current state.

## Enforcement rules

- The guard is enforced at lease acquisition, below individual job dispatchers.
  With the guard enabled, unregistered roots cannot launch.
- Descendants inherit their investigation across job IDs, source revisions and
  worker sessions. Mixed or unregistered ancestry is denied rather than creating
  a fresh allowance.
- Only a trusted host registers roots or admits continuations. Workers get no
  ledger write access and no host service that exposes admission; a worker
  cannot grant itself budget through its output. Worker isolation is essential:
  the guard does not defend against arbitrary ledger writes.
- Each investigation is serialized through independent grading. Supporting work
  can succeed without progress credit; failed or expired work counts against the
  stall allowance. Claimed progress, file counts, test counts and reworded
  reports never reset counters.
- The process runner checks remaining allowance before launch and on one-second
  heartbeats, and terminates the guarded child tree when the budget runs out.
  A live attempt or pending grade is not an opportunity to launch duplicate
  work. Expiry, failure and restart keep their charges; decisions cannot be
  rewritten into successes.
- Never disable the guard, reset counters, rename or re-root an investigation
  for a fresh allowance, or execute directly after a denial.

Default limits per investigation:

- 12 normal attempts and 1,800 seconds of leased execution.
- Two no-progress outcomes, or six completed supporting jobs without a
  registered progress proof, trigger recovery or shelving.
- At most one fresh read-only review (300 seconds) and one pre-admitted
  alternative (600 seconds); these are bounded extras, not resets.
- A verified progress credit resets only the local stall and activity counters,
  never elapsed execution or the cumulative attempt count.

The production progress adapter independently checks the candidate, review and
replay lineage and input correspondence before crediting a later selected-update
frontier; a semantic fingerprint and numeric high-water mark reject repeated or
lower frontiers, even under renamed baselines. Other job types count as
supporting activity until they have their own progress verifier.

## Recovery and stopping

Recovery uses trusted, pre-admitted jobs. The fresh reviewer is read-only and
earns no credit; one alternative may then run, after which the investigation is
shelved with its result preserved. Independent admitted work stays runnable.
With no runnable work and a denied queue, the service returns
`progress-stopped` and writes `investigation-progress.json` instead of
replanning or idling. That is not goal completion, a Goal pause or a budget
extension. No production recovery job is registered yet, so an exhausted
investigation is shelved rather than retried.

## Trusted continuations

An exhausted job retry budget does not necessarily exhaust its investigation.
A trusted host can attach a fresh queued job to a completed failed or blocked
normal-work attempt while the same investigation remains active:

```python
from scripts.autonomy import progress_guard

receipt = progress_guard.admit_continuation(
    store, "synthetic-problem",
    predecessor_job_id="failed-job", job_id="fresh-job",
    reason="Test the distinct explanation recorded in the failed attempt",
)
```

After enqueueing the fresh job, the equivalent host CLI is:

```text
python -m scripts.autonomy.progress_guard --state tools/private/fixture-state --admit-continuation synthetic-problem --predecessor-job failed-job --job fresh-job --reason "Test the recorded explanation"
```

Both return the durable admission receipt. Reasons are 1-500 printable
characters without surrounding whitespace. There is no automatic proposal
generation or progress classifier.

- **Requirements.** The guard is enabled; the investigation exists and is active
  with time, attempt, no-progress and activity allowance; no investigation
  attempt is live or ungraded; the predecessor's latest attempt is completed and
  graded as normal work; the child is queued, unattempted and has no
  conflicting membership. Unknown IDs, mixed or unregistered ancestry, recovery
  roles, reused children and second forks from one attempt fail closed.
- **Ancestry versus prerequisites.** Accounting ancestry is separate from
  execution prerequisites: the child may omit the failed predecessor, but every
  listed dependency must pass the ordinary success checks and belong to the
  same investigation.
- **Record.** Schema 3 adds `problem_continuations` (problem, predecessor job,
  exact attempt token, child, reason, admission time) with one continuation per
  predecessor attempt and per child. Versions 1 and 2 migrate transactionally
  without rewriting specs, seals, receipts, definitions, roots, the guard
  setting or counters; version 1 stays disabled until explicitly enabled.
- **Semantics.** Validation, admission and membership share one
  `BEGIN IMMEDIATE` transaction. Exact repeats return the original receipt while
  the investigation is eligible; conflicting repeats cannot rewrite it; shelved,
  exhausted and recovery-state investigations reject even repeats. Admission
  awards no progress and changes no counter. The child's lease charges the same
  investigation and its deadline is lease start plus the remaining cumulative
  execution time; grading, stall, heartbeat, pause and resource gates still apply.

Continuations never reopen shelved investigations. The one bootstrap admission
the owner approved on 2026-09-27, made with the original budget and charges,
is not a standing authorization.

## Acceptance

```powershell
python -m unittest discover -s tests -p 'test_autonomy_progress_guard.py'
python -m scripts.autonomy.progress_guard_smoke tools/private/progress-guard-smoke-NEW
```

The output directory must be new. The smoke builds an original fixture
repository with fake CLI workers (no game, no model, no API billing) and uses
real supervisor dispatch, detached worktrees, guarded subprocesses, sealed
results, SQLite restart and independent problem selection. It asserts that two
failed attempts run and a third never launches, exactly one review and one
alternative run, independent work finishes, a hung worker is terminated by the
investigation budget, self-reported progress earns no credit, a restart cannot
reopen the alternative, and the exhausted service stops.

Recorded acceptance: producer-pinned smoke
`tools/private/progress-guard-smoke-20260927e/result.json` passed all eleven
checks (SHA-256 `796690b6bb292e1f4b25cd127912f178516aa565d170fb7bc39eeca42b366fbe`),
terminating its stalled 90-second fixture after about 1.2 seconds of leased
execution. After the continuation update, all 463 automation tests passed twice
independently with a high-confidence review approval.

## Scope and remaining work

The guard covers supervisor-launched work, not arbitrary shell commands or the
foreground chat; autonomous project work must enter the queue to be governed.
A Codex Goal is an evidence-based continuation contract, separate from this
project's investigation controller; resuming a Goal does not reset budgets or
override supervisor decisions.

Still unproved: evidence-grounded recovery experiments, independent verifiers
for hypothesis elimination and repaired microcases, general alternative
generation and selection, real-model recovery effectiveness and the complete
autonomous repair and integration loop.
