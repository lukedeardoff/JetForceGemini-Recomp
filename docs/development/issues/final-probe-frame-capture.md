# DIAG-001: Bounded replay ends without a captured final image

- Severity: low; diagnostic failure that blocks performance comparisons.
- Status: verified in hidden reference/native replays; not released.
- Reported / updated: 2026-10-11 UTC.
- Affected: Windows original-OS bounded replay with frame capture enabled.
- Related: [PERF-001](goldwood-combat-slowdown.md).

## Symptoms and evidence

The reference Goldwood bridge replay completed its requested 17,000 video
messages, wrote RAM, then failed at `guest-capture/final-image`. Its last
presentation was VI 16,997. Native gameplay had not crashed.

The capture condition requested GPU readback only within two VIs of the stop
target. A three-VI presentation interval can miss that entire window. Extending
the fixed window would still assume a maximum presentation interval.

## Cause and remedy

At a non-play probe's stop boundary, if frames were presented but no image was
captured, request readback of the already selected display buffer through the
existing RT64 presentation/capture path. Do not execute more guest instructions
or silently discard capture errors. This extra diagnostic presentation occurs
after the measured replay, and is marked `final-frame-capture` in the trace.
RT64 also deduplicates unchanged VI/framebuffer presentations. For an explicit
screenshot request that queues no new present, drain its queues and repeat the
last presentation using RT64's paused-presentation mechanism. This repeats the
display/readback only, not a guest workload or simulation update. Normal rendering
and probes without image capture keep their current path.

## Verification and remaining limits

Both reference and native 17,000-VI bridge cases now produce valid 640 x 480
images. The reference final RAM remains
`65030211bfc31fb2b85ff4c13198f7b35d792f9a9a4abb94f660f94a1cffa939`,
exactly matching the earlier run that failed only at final-image capture. Its last
guest presentation remains VI 16,997; the final diagnostic capture occurs at
17,000 without executing additional guest instructions. Both comparisons begin
at the same VI, Count, controller poll and RAM hash. Source saves are unchanged.
Runtime: `00c1dfaeae5d2f4e12242a69326c531df5edadb010127d5a461d9506a4318434`.
Private evidence: `certified-bridge-comparison.json` and its paired replay cases.
This verifies the reported bounded-probe failure, not visible presentation latency
or an installed/released preview change.

## Lessons

Simulation ticks, video messages and rendered frames are distinct clocks. A
bounded test should capture at its actual stop boundary rather than assuming a
frame will arrive within a fixed number of simulation ticks. This also matters
when testing future interpolation or higher presentation rates.
