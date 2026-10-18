# Plans and acceptance contracts

The [roadmap](JFG_RECOMP_MASTER_PLAN.md) defines product goals and acceptance
gates; [project status](../status.md) records what has been demonstrated.

## Open work

- [Campaign slice (Phase 9)](phase9-acceptance.md): remaining G6 route,
  recovery and regression coverage.
- [Automated gameplay exploration (Phase 9.5)](phase9-5-autonomous-exploration.md):
  scenario, checkpoint and recovery requirements.
- [Automation delivery plan](autonomous-full-scope-execution.md): the remaining
  engineering-loop outcomes.
- [Selected-state boundary at update 1909](phase9-execution-frontier-1909.md):
  the recorded native/reference comparison limit.
- PC enhancements: [experimental PC controls](../features/pc-controls.md) and
  [navigation](../features/navigation-mod.md) list their remaining work.

## Operational requirements

- [Production progress guard](autonomy-progress-guard.md), including the
  ledger location and trusted continuations.
- [Automation guide](../development/automation.md) and [AGENTS.md](../../AGENTS.md).

## Runtime timing contracts

These notes define the original-OS timing implementation; boot provenance
cites them by path. [Runtime architecture](../development/architecture.md#boot-os-and-timing)
summarizes them.

- [Execution clock](phase9-execution-clock-implementation.md)
- [OS and device timing](phase9-os-clock-qualification.md)
- [Serial-interface completion](phase9-si-completion-timing.md)
- [CPU-visible framebuffer writeback](phase9-renderer-cpu-writeback.md)
- [Instruction-effect observation](phase9-instruction-effect-observation.md)

## Retained acceptance contracts

| Area | Contract |
|---|---|
| CPU generation | [Phase 4](phase4-acceptance.md) |
| Deterministic kernel | [Phase 5](phase5-acceptance.md) |
| Native boot | [Phase 6](phase6-acceptance.md), [native record](phase6-native-boot-addendum.md), [completion ledger](phase6-native-boot-remaining.md) |
| Rendering | [Phase 7](phase7-acceptance.md) |
| Interactive runtime | [Phase 8](phase8-acceptance.md) |

Contracts keep their phase names and historical wording because tests read
them and signed manifests bind some of their bytes (Phase 5, Phase 6 and
[ADR 0003](../adr/0003-phase4-dependency-architecture.md)). Changing a contract
requires an explicit version and evidence decision; documentation cleanup must
not rewrite them. Their historical license and publication wording does not
describe the current project: original work uses [MIT](../../LICENSE). See the
[evidence guide](../../evidence/README.md).

## High-refresh rendering

[60-360+ FPS rendering investigation](high-refresh-rendering.md) preserves guest
timing and records the RT64 integration path, working-build evidence and acceptance
gates. The [opt-in prototype](../features/high-refresh-rendering.md) now measures
60/240 FPS in one moving-camera fixture with matching guest state; broader
visual acceptance and 360+ FPS qualification remain open.
