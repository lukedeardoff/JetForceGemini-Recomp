# Project status

[CI](https://github.com/TK22-26/JetForceGemini-Recomp/actions/workflows/ci.yml)
checks source, tools and tests without a ROM. A passing build does not mean
every level or feature works. To play, start with [getting started](getting-started.md);
to check a problem, see [known issues](known-issues.md).

## Completed v1 release

**v1.1.0 is fully playable from beginning to end**, with original local multiplayer
and optional content confirmed working by the maintainer. The Windows x64 ZIP
contains the prebuilt program and runtime libraries: import your supported US
ROM once and play. No ROMs or game assets are included, and no first-run build or
developer tools are required. See the [project dashboard](dashboard.md).

The latest development build adds the major controller/keyboard/mouse overhaul;
the new update checker is also in current source, awaiting release. See the
[controls guide](features/pc-controls.md) and [update-checker record](development/issues/launcher-update-discovery.md).

The retained campaign run used infinite-health/enemy auto-kill testing mods and
a save with all Tribals unlocked. Historical test conditions remain recorded;
specific bugs, broader hardware testing and original-console accuracy are still
tracked. These are distinct from the game's released and playable status.

## Engineering milestones

Records describe bounded results at their recorded revisions; they do not
certify later builds or close broader roadmap gates. See the
[evidence guide](../evidence/README.md).

| Milestone | Recorded result | Reference |
|---|---|---|
| Reproducible upstream builds | Two environments, repeated deterministic builds | [Phase 1 aggregate](upstream/phase1-build-evidence.json) |
| CPU and overlay feasibility | G2 go decision | [Feasibility record](feasibility/README.md) |
| CPU generation and link | Phase 4 complete (signed) | [Phase 4 aggregate](../evidence/phase4-completion.json) |
| Deterministic test kernel | Phase 5 complete (signed) | [Phase 5 aggregate](../evidence/phase5-completion.json) |
| Native boot | Phase 6 M2 complete (signed) | [Phase 6 aggregate](../evidence/phase6-completion.json) |
| First rendered scenes | Phase 7 accepted | [Phase 7 summary](../evidence/phase7-completion.json) |
| Interactive runtime | Phase 8 accepted | [Phase 8 summary](../evidence/phase8-completion.json) |
| Original timing in the unified runtime | Opening and Quarry runs with zero audio underruns | [Timing validation](../evidence/unified-original-timing-validation.json) |
| Deterministic campaign slice (G6) | Open | [Phase 9 tracker](planning/phase9-acceptance.md) |
| Automated exploration and repair | Partial capabilities; acceptance open | [Exploration plan](planning/phase9-5-autonomous-exploration.md) |

The formal gates are stricter than a manual playthrough: G4 needs fifty
deterministic boot repeats and ten reference captures, and G6 needs a hundred
consecutive deterministic completions of a slice. The only hundred-run batch
so far is a 1,794-retrace startup prefix, not a gameplay slice.

## Automated gameplay

Earlier work demonstrated startup, checkpoint restoration, generated
navigation, selected destinations and bounded combat/death/retry cases; ten
seeded jobs varied their objectives. The launcher's [navigation mod](features/navigation-mod.md)
adds live room-to-room exploration, jumps, chests and NPC rewards. None of this
proves general varied-path exploration, native snapshot resume or complete
native/reference parity. The recorded original-OS comparison matched 1,908
consecutive selected update states ([boundary record](planning/phase9-execution-frontier-1909.md)).

## Remaining work

- Complete the deterministic campaign slice and its recovery and parity coverage.
- Extend formal G7/reference-parity evidence beyond the retained campaign run.
- Resolve documented performance/camera issues and improve support diagnostics.
- Continue physical-device and fresh-PC compatibility testing of modern controls.
- Add ultrawide, higher frame-rate presentation and mod interfaces on a stable
  compatibility baseline.

The [roadmap](planning/JFG_RECOMP_MASTER_PLAN.md) defines the goals and gates.
