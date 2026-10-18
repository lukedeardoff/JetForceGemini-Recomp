# Serial-interface completion timing

The original investigation identified VI-gated SI completion as a source of
controller-read timing differences. The bounded deadline experiment kept PIF
payload handling separate from delivery timing and extended selected-state
agreement without shifting input or update indices.

## Required behavior

- Preserve busy rejection and no completion before the qualified deadline.
- Deliver completion exactly once; support rearming and reject overflow.
- Keep controller query/initialization state explicit.
- Preserve uninstrumented behavior when diagnostic flags are disabled.
- Separate measured reference latency from a claim about physical hardware.

Tests and recorded experiments cover these boundaries. The SI repair did not
prove full runtime parity; subsequent differences required independent diagnosis.
See the later [comparison boundary](phase9-execution-frontier-1909.md) and
[OS timing contract](phase9-os-clock-qualification.md). Detailed trial evidence
remains in [Git history](../README.md#historical-material).
