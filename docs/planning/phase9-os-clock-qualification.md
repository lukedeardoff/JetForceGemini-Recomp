# OS and device timing

This is a retained implementation reference for source provenance. The latest
recorded selected-state comparison in this investigation is the
[1,908-update prefix](phase9-execution-frontier-1909.md). Earlier trial-by-trial
results are available in [Git history](../README.md#historical-material).

## Guest OS integration

Original guest queue routines require compatible guest thread/sentinel lists
and context-switch state. Native waiting-list bookkeeping cannot be substituted
blindly. Preserve guest registers, CP0 status, interrupt masks, and exception
return semantics across scheduler handoffs.

The execution-driven profile models guest work and separate device deadlines.
It must not substitute host elapsed time, generated function count, or a guessed
OS-call cost for guest execution. Blocked-call wall time includes scheduling
and interrupt work and cannot be charged as uninterrupted CPU work.

## Device timing

`include/jfg/boot/reference_device_timing.hpp` records the reference-specific
device profile. Keep PI/SI completion, VI service, SP/DP events, AI behavior,
and FlashRAM semantics explicit. Reference-profile observations do not by
themselves establish physical-N64 behavior.

The supporting microtests cover context switches, masks, queue operations,
translation/cache behavior, device completion, and FlashRAM cases. Their
recorded passing cases are bounded contracts, not proof of a complete hardware
timing model. See [execution clock](phase9-execution-clock-implementation.md)
and [SI completion](phase9-si-completion-timing.md).

## Remaining acceptance

The recorded pacing mismatch precedes the selected actor/camera divergence.
Qualify each observer against unchanged traces and focused state, identify
the causal operation, and prove a focused repair before retesting the route.
Do not install offsets or timing constants solely to move the comparison boundary.
The [production guard](autonomy-progress-guard.md) controls further execution.
