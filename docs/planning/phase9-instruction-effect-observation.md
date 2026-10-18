# Instruction-effect observation

N64Recomp patch 0021 adds opt-in offline-C `after_vram` hooks, independent of
existing before hooks. Ordinary-operation hooks follow the emitted operation.
Control-transfer hooks follow condition/target evaluation and link updates,
before the delay slot; they do not wait for a generated callee to return.
Annulled likely slots emit neither entry nor effect events.

Duplicate hooks of one kind and invalid addresses are rejected. After addresses
must be nonzero, aligned, inside the function, and exclusive of `before_vram`
within the same configuration entry. Hook-free generation retains its behavior.

## Qualification

The fixture in `tests/fixtures/instruction_effects.S` and executable checks in
`tests/generated_instruction_effects.cpp` exercise the compiler boundary.
Native effect streams and exception-return observers have separate runtime tests.

A textual hook after a callback does not prove completion of ERET, syscall,
thread handoff, or an exception. Unsupported operations and backends must fail
closed. Observers must be bounded and non-mutating; preserve full update traces,
inputs, and focused state before relying on their measurements.

Paired instruction/device observations do not establish a common raw clock
or a causal gameplay fix. The recorded selected-state frontier remains in the
[boundary record](phase9-execution-frontier-1909.md). Detailed qualification
and rejected cases remain in [Git history](../README.md#historical-material).
