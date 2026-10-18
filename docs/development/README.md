# Development guide

To play, start with [getting started](../getting-started.md); to report a
problem, use [playtesting](../playtesting.md). Code changes follow
[CONTRIBUTING.md](../../CONTRIBUTING.md); automated work follows
[AGENTS.md](../../AGENTS.md).

## Build, test and release

- [Development setup](setup.md): build and test the source without a ROM.
- [ROM build](rom-bootstrap.md): produce the playable Windows runtime (shipped in the release ZIP).
- [Launcher guide](launcher.md): the player-facing guide shipped as the ZIP's `README.md`.
- [Releasing](releasing.md): package the launcher and publish a versioned release.
- [Support diagnostics](support-diagnostics.md): crash/freeze capture, symbols and the gameplay trace.
- [Test quarantine](test-quarantine.md): the documented Linux CI exclusion.

## Engineering reference

- [Experimental high-refresh rendering](../features/high-refresh-rendering.md): implementation, preview launchers, 60/240 FPS replay evidence and remaining limits.
- [Runtime architecture](architecture.md): build pipeline, overlays, timing, graphics, audio and saves.
- [Issue investigations and resolutions](issues/README.md): ongoing reports, attempted remedies and verified outcomes.
- [Runtime fix log](boot-gameplay-fix.md): confirmed defects with cause, fix and validation.
- [Feature pages](../features/): PC controls, live map and navigation mod, frontend.
- [Automation](automation.md): the guarded supervisor and diagnostic workflow.
- [Plans and acceptance contracts](../planning/README.md) and [project status](../status.md).

## Source layout

| Location | Purpose |
|---|---|
| `src/`, `include/jfg/` | Native runtime, boot, rendering, audio, frontend (`src/app`) and host interfaces |
| `launcher/` | Frontend build and RmlUi resources; embedded Windows helper (C#, PowerShell setup) |
| `scripts/` | Build orchestration, validation and diagnostic tools |
| `scripts/autonomy/` | Guarded job scheduling and execution |
| `tests/` | Source tests and synthetic fixtures |
| `config/`, `schemas/`, `examples/` | Versioned contracts and validation fixtures |
| `patches/` | Maintained dependency patches and upstream notices |
| `evidence/` | Milestone aggregates and signatures |
| `docs/` | Documentation ([index](../README.md)) |

Generated output, dependencies, local inputs and job results use ignored
directories (`tools/`, `build/`, local caches). Files under `examples/` are
used by validation, and phase-numbered scripts can still be part of the build.
