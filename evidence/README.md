# Completion evidence

This directory retains milestone aggregates and signatures at their recorded
source revisions. They describe bounded results; they do not certify later
builds, a complete campaign or a release. Only public-safe aggregates (counts,
pins, digests, gate booleans) are tracked.

| Record | Purpose |
|---|---|
| [Phase 4](phase4-completion.json) | CPU generation and G2/G3 completion (signed) |
| [Phase 5](phase5-completion.json) | Deterministic test kernel completion (signed) |
| [Phase 6](phase6-completion.json) | Native-boot M2 completion (signed), with its [boot summary](phase6-native-boot-summary.json) and [stub ledger](phase6-native-stub-ledger.json) |
| [Phase 7](phase7-completion.json) | Rendering acceptance, with [boot/title](phase7-boot-title.json), [gameplay](phase7-gameplay.json) and the earlier [RT64 task smoke](phase7-rt64-task-smoke.json) |
| [Phase 8](phase8-completion.json) | Interactive-runtime acceptance |
| [Opening asteroid textures](opening-asteroid-texture-reset.json), [lens flare](opening-lens-flare-depth-bounds.json), [cutscene audio](opening-cutscene-audio-continuity.json) | Renderer and audio fixes in the opening cinematic |
| [Unified original timing](unified-original-timing-validation.json) | Opening and Quarry runs on the integrated original-timing runtime |

The Phase 1 build aggregate ([docs/upstream](../docs/upstream/phase1-build-evidence.json))
and Phase 3 CPU/overlay aggregate ([docs/feasibility](../docs/feasibility/phase3-cpu-overlay-evidence.json))
stay beside their records because their schemas and validators fix those paths.
The fixes behind the opening and timing records are described in the
[runtime fix log](../docs/development/boot-gameplay-fix.md).

## Rules

Some Markdown contracts and source files are bound by digest or read directly
by validators (see the [planning index](../docs/planning/README.md)). Preserve
their bytes when reorganizing documentation, and never re-sign historical
claims or replace pinned digests to make a cleanup pass.

`examples/` contains unsigned synthetic validation fixtures, not completion
claims; keep them and the signing public key. Detailed inputs, generated
bodies, local evidence and private signing material stay outside tracked
source under the [corpus policy](../docs/legal/test-corpus-policy.md). The
signed Phase 4 pipeline is described in the
[feasibility record](../docs/feasibility/README.md#regenerating-signed-phase-4-evidence).

## Validation

```sh
python scripts/validate_project.py
python scripts/validate_phase7_acceptance.py
python scripts/validate_phase8_acceptance.py
```
