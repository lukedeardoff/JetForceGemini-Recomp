# AUTO-001: Withdrawn queue request aborts a guarded session

- Severity: medium, investigation tooling only.
- Status: cause identified; recovery worker corrected, recovery in progress.
- Discovered: 2026-10-14 UTC.

The private combatfocus worker snapshots pending request filenames before running
them. The host withdrew request 20 while request 19 was executing. After 19
completed, opening the now-renamed pathname raised FileNotFoundError. The job
failed and was graded against the existing investigation; no prior result was
lost, and requests 21-23 did not execute in that attempt.

The owner approved one recovery within the unchanged deadline. The guard remains
enabled; all spent time, attempt, no-progress and activity counters are preserved.
The replacement worker verifies the completed manifest, request hashes and
results before resuming. It accepts a missing queued pathname only when the
corresponding immutable withdrawn request exists and its script hash verifies.
Unexpected missing files still fail closed. The host will avoid withdrawing
requests after submission. The original failed job and withdrawal receipt remain
private evidence; this is not a game runtime regression.

The queue is private trusted-host orchestration, not production guard code.
A resumed job is not fresh investigation allowance and a tooling failure must
not be hidden by resetting counters or bypassing the supervisor.

The 715-file generated-code rebuild exceeded its initially estimated duration.
Before the private step deadline, the trusted host canceled only that step's
owned compiler tree and recorded the nonzero step result. The enclosing guarded
job stayed active and retained all charges. Immutable dependent requests were
withdrawn and requeued in the same session; the replacement step resumes cached
objects with a longer bounded build time inside the original approved window.
No guard denial, job budget or completed result was overridden.

The recovery now safely observes explicitly withdrawn immutable requests
34-39 from its cached queue list and continues to later checks. Request hashes,
withdrawal receipts and the completed/failed results remain intact. A queued
matrix-test executable path was corrected before execution. The shadow-image
analysis initially tried to parse the contact-sheet name as a frame number;
a filtered retry passes and preserves the failed analysis. Neither setup
correction changes game code or establishes visual acceptance. Recovery startup
validation still needs explicit handling of cancelled manifest entries before
a future, separately authorized resume; no such new job is launched here.

A prepared successor worker now validates both legacy cancelled entries without
result files and newly persisted cancellation results. Eight isolated fixture
executions cover normal resume, both cancellation formats, cached withdrawal
and subsequent resume, plus rejection of tampered scripts, recreated requests
and missing ordinary results. Tests use private fixture queues only, with no
production ledger access. The active worker and immutable job identity are
unchanged; a future admitted job can use the verified source. No new operational
attempt or counter reset was performed.
