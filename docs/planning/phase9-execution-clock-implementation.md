# Execution-driven clock

The opt-in original-OS diagnostic uses `include/jfg/boot/execution_clock.hpp`.
Its recorded selected-state limit is documented in the
[boundary record](phase9-execution-frontier-1909.md).

## Contract

- Monotonic 64-bit virtual time with absolute device deadlines, cancellation,
  one outstanding event per source, and stable simultaneous-event ordering.
- Periodic deadlines anchored to the previous deadline; missed periods remain visible.
- Explicit overflow handling and idle advance only when no guest thread is runnable.
- Pending interrupts retained while masked, disabled, in exception state, or
  at an unsafe boundary. Acknowledgement clears only selected sources.
- Per-context retirement: retire prior work, drain events, preempt if allowed,
  and begin the next operation after resumption. In-flight work is not shared
  between guest threads.

Callers supply qualified costs and device behavior. Host instruction counts
and wall-clock delays are not a general guest timing model. Events retain
their original timestamps when atomic work crosses a deadline.

## Verification boundary

Focused tests cover clock, deadline, and interrupt behavior. Full-game timing,
asynchronous devices, and complete parity remain separate acceptance work.
Historical prototype trials are in [Git history](../README.md#historical-material); consult
[OS timing](phase9-os-clock-qualification.md) for integration constraints.
