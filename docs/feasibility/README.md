# Feasibility and G2 record

Phase 3 feasibility spikes established whether the game could be recompiled
and what the native runtime had to provide. Their blockers were closed for the
G2 architecture gate, which the signed [Phase 4 aggregate](../../evidence/phase4-completion.json)
records as a go decision. This page is the historical record; current
component behavior is in [runtime architecture](../development/architecture.md).
The full spike reports are in [Git history](../README.md#historical-material).

The tracked public aggregate for spikes A and B is
[phase3-cpu-overlay-evidence.json](phase3-cpu-overlay-evidence.json). Its path
is fixed by its schema and validators. Only aggregate counts, pins and digests
are tracked; ROM bytes, generated code, per-entry data and local paths stay
under ignored `tools/`.

## Spike results

Pinned inputs: JFG decomp `b49aa4791e8f`, N64Recomp `ffb39cdad1da`.

- **A, CPU metadata.** The ELF was a usable start but not a complete
  recompiler input: no usable entry point, six uncovered zero-size functions
  needing size overrides, no standard relocation sections, 7,414 unresolved
  `jal` placeholders in 970 functions and 205 register-indirect candidates.
  N64Recomp's jump-table analysis could read past the ROM buffer; the
  maintained patch series bounds word reads and analysis (patches 0006-0007).
  A conservative fallback with 1,091 stubs proved the rest could generate.
- **B, overlays.** The complete overlay table and all 28,035 custom relocation
  records parse with in-bounds, non-overlapping spans and paired HI16/LO16
  records. The loader copies code and patches existing words; no generated
  instruction streams were observed. Three exceptional reference-table entries
  must stay fail-closed. The custom records must be translated into recompiler
  metadata rather than patched into native code at runtime.
- **C, RSP, graphics and audio.** RSPRecomp generated compilable code for the
  custom audio microcode (one slot, two permutations). Pinned RT64 had no
  handler for the game's F3DDKR graphics family, so the project wrote its own
  bounded binding. GLideN64 was a research reference only; its GPL source was
  not copied or linked.
- **D, runtime gaps.** The candidate upstream runtime covered queues, timers,
  standard PI transfers and FlashRAM but not direct hardware registers, address
  aliases, TLB, code-cache coherency, raw PI transfers, the game decompressor,
  Controller Pak files or trap/self-check paths
  (`config/runtime-capability-matrix.json`). The project implemented those
  boundaries itself, including the accessory and FlashRAM models.

## G2 closure

G2 required, with authenticated executable evidence: every executable CPU
section attempted; the hardest overlay loaded, relocated, looked up, unloaded
and reloaded through the native runtime; reachable traps classified with
non-stub dispositions; one real graphics task and one real audio task through
the proposed native paths; and a save/accessory round trip.

- The overlay lifecycle and trap-classification requirements are fixed in
  `config/overlay-runtime-g2-contract.json` and checked by
  `scripts/validate_overlay_runtime_g2_contract.py`.
- Graphics and audio evidence came from one approved private task each,
  matched against independent oracles; the public manifests record only
  aggregates (`scripts/validate_graphics_task_bridge.py`,
  `scripts/validate_audio_task_bridge.py`).
- Every executable mapping is a byte-pinned producer built twice,
  byte-identically, from a reviewed clean checkpoint (WSL Ubuntu 24.04,
  GCC 13.3). The dispatcher runs only staged producer copies with fixed
  arguments, a fresh nonce and before/after hash checks; on Windows it uses the
  fixed system WSL boundary. Pins live in `scripts/g2_production_evidence_harness.py`
  and `scripts/validate_g2_private_evidence.py`.

## Regenerating signed Phase 4 evidence

The Phase 4 claim is fail-closed: it needs the signed public manifest, the
ignored private bodies and local digest recomputation. The construction order
is deliberately acyclic:

1. Commit the production harness (`scripts/phase4_evidence_harness.py`) with
   its pin in `scripts/validate_phase4_private_evidence.py`; worktree, index and
   `HEAD` bytes must match.
2. Produce all eleven execution product sets (generation; Clang, GCC and MSVC
   compiles; forced-object link audit; Clang static analysis; ASan; UBSan;
   reproducibility runs A and B; configuration mutation) from the same private
   inputs, then package them with `scripts/prepare_phase4_products.py`.
3. Build the private body and non-completion G3 projection with
   `scripts/build_phase4_private_evidence.py`, which reruns every pinned audit.
4. Derive the G2 CPU case (`scripts/prepare_g2_cpu_case.py`) and close every
   G2 requirement with its own private evidence.
5. Require [ADR 0003](../adr/0003-phase4-dependency-architecture.md) to be
   accepted, then sign with `scripts/build_phase4_completion_manifest.py`,
   which refuses to overwrite an existing claim.
6. Verify with `scripts/validate_phase4_manifest.py` against the manifest, both
   schemas, and the public and private G2 and Phase 4 bodies.

Signing uses `ssh-keygen -Y` from fixed system locations with a staged,
rechecked copy, the anonymous public key in
`config/phase4-completion-signing.json`, and the ignored private key under
`tools/`. Never re-sign a historical claim or replace a pinned digest during
cleanup.

## ROM-free checks

```sh
python scripts/validate_project.py
python scripts/validate_runtime_capabilities.py
python scripts/validate_overlay_runtime_g2_contract.py
python -m unittest tests.test_phase3_cpu_overlays tests.test_runtime_capabilities tests.test_overlay_runtime_g2_contract
```
