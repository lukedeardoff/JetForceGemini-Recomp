# Documentation

## Play and report problems

- [Project dashboard](dashboard.md): current release, completed features and active enhancements.
- [Getting started](getting-started.md): requirements, downloads and installation.
- [Launcher guide](development/launcher.md): setup, controls, saves, live tools and mods.
- [Known issues](known-issues.md): current open limitations.
- [Playtesting](playtesting.md): reporting problems and the support ZIP.
- [Release notes](releases/): curated notes for published versions.

## Features

- [Experimental PC controls](features/pc-controls.md): mouse/keyboard and dual-stick play.
- [Live map and navigation mod](features/navigation-mod.md): map views, exports and automatic navigation.
- [Unified frontend and live inventory](features/frontend.md): launcher architecture and ROM-derived artwork.

## Develop

- [Development guide](development/README.md): building, testing, releasing and diagnostics.
- [Issue investigations and resolutions](development/issues/README.md): symptoms, attempts, evidence and actual fixes.
- [Runtime architecture](development/architecture.md) and [runtime fix log](development/boot-gameplay-fix.md).
- [Project status](status.md), [roadmap](planning/JFG_RECOMP_MASTER_PLAN.md) and
  [plans and acceptance contracts](planning/README.md).
- [Contributing](../CONTRIBUTING.md); automated work follows [AGENTS.md](../AGENTS.md).

## Reference

- [Architecture decisions](adr/): project boundaries, execution profiles, dependency architecture.
- [Completion evidence](../evidence/README.md) and the [feasibility record](feasibility/README.md).
- [Pinned upstream versions](upstream/pinned-version.md) and the
  [upstream reference review](upstream/reference-review-2026-10-01.md).
- [Third-party notices](../THIRD_PARTY_NOTICES.md), [dependency inventory](legal/dependency-license-inventory.md),
  [data handling](legal/rom-and-assets-policy.md) and [test corpus policy](legal/test-corpus-policy.md).
- [Security reporting](../SECURITY.md) and [community credits](../CREDITS.md).

## Where documents belong

| Location | Contents |
|---|---|
| `docs/` | Player-facing guides and project status |
| `docs/features/` | Design, usage details, validation and limits of optional features |
| `docs/development/` | Build, test, release, diagnostics, architecture and fix records |
| `docs/planning/` | Roadmap, acceptance contracts, automation plans and timing contracts |
| `docs/adr/`, `docs/legal/`, `docs/upstream/` | Decisions, licensing and data policy, upstream references |
| `evidence/` | Milestone aggregates and signatures |

Some paths are fixed even where another folder would fit better:

- Signed evidence pins the bytes of the Phase 5 and Phase 6 contracts and
  [ADR 0003](adr/0003-phase4-dependency-architecture.md), and those files name
  other documents by path (the Phase 4 contract, the dependency inventory).
  Never edit or move them.
- [Boot provenance](../src/boot/PROVENANCE.md) cites the runtime fix log and the
  four `phase9-*` timing notes in `planning/` by path.
- Published launchers and release packages link `getting-started.md`,
  `known-issues.md`, `development/launcher.md` and `development/rom-bootstrap.md`
  on `main`; the last two ship in the release ZIP and use absolute links.
- The autonomy supervisor protects the roadmap path, and AGENTS.md names the
  progress-guard contract and upstream reference review.
- The Phase 1 and Phase 3 aggregates' paths are fixed by their schemas and validators.

## Historical material

Superseded plans, experiment journals, per-run validation notes and
review-only artifacts are removed from the working tree once their conclusions
are captured in the pages above. The
[source snapshot before the first cleanup](https://github.com/TK22-26/JetForceGemini-Recomp/tree/2c55b0f082395e2be4407958637dbb4effb3c43d)
keeps the original documents; in a clone, use `git log --all -- path/to/file`
and `git show <commit>:path/to/file` to read any earlier version.

Historical results apply to their recorded source, tools, inputs and execution
profiles. They do not certify the current build or reopen a stopped investigation.
