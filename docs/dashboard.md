# Project dashboard

Updated 2026-10-16. Jet Force Gemini Recomp is a **complete, fully playable v1
game for Windows x64**. The campaign can be played from beginning to end;
original local multiplayer and optional content work. Development now adds PC
quality-of-life features and addresses specific bugs and compatibility issues.

## Download and play

The published stable release is **v1.2.0**:
[download JFG-1.2.0-windows-x64.zip](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0).
Extract the whole ZIP, open `JFG-Launcher.exe`, import your supported North
American `.z64` ROM once, and click **Play**. No compiler, WSL or developer tools
are needed. Saves and settings stay in your local profile when you update.

The release includes the compiled program and runtime libraries. **No ROMs or
game assets are distributed.** The game reads those assets from your own local
ROM. Follow the [installation guide](getting-started.md).

## Feature status

| Feature | Status |
|---|---|
| Full campaign, original local multiplayer, optional content | Playable; completion/availability confirmed by the maintainer |
| Native rendering and audio; save/load | Implemented and playable |
| Prebuilt Windows x64 bundle and one-time ROM import | Released in v1.1.0 |
| Correct centered 4:3 game viewport in wide windows | Released in v1.1.0 |
| Unified launcher, original input mappings, live map and inventory | Available in the stable release |
| Major controller and keyboard/mouse overhaul | Included in v1.2.0; opt-in |
| Automatic release notifications and Help > Check for updates… | Included in v1.2.0; validation status in the [issue record](development/issues/launcher-update-discovery.md) |

## Controls: the major quality-of-life addition

The new **Modern Controls** style gives player 1 mouse look and WASD movement
or a dual-stick controller layout, with strafing while aiming. Camera, aim and
scope sensitivity are independent. Rebind gameplay actions, tune inversion,
deadzones and response curves, and see your assignments on controller or
keyboard/mouse diagrams. Xbox/XInput and compatible SDL controllers are supported.

Open **Controllers**, choose your device and select **Modern Controls**. Original
**Controller Mapping** remains available and keeps its saved bindings when you
switch styles. Original multiplayer uses the original mappings; modern gameplay
is currently opt-in, experimental and single-player. Supported native menus
accept mouse navigation; some subpages still need keyboard/controller input.
See [PC controls](features/pc-controls.md) for defaults and current limitations.

## Updates without checking GitHub yourself

The new launcher checker runs in the background at startup. A home-screen notice
appears when a newer stable release is available, and **Help > Check for updates…**
shows installed/latest versions on demand. Its download button opens the release
page. Nothing is installed automatically, and offline failures do not block play.
Included in v1.2.0; the download button opens GitHub in your browser.

## What remains

- Extend [performance qualification](development/issues/goldwood-combat-slowdown.md) beyond the improved owner-tested route and investigate the exit-cutscene report.
- Refine modern camera behavior and finish mouse targets on remaining menu pages.
- Continue physical-device and hardware compatibility checks.
- Extend high-refresh and ultrawide qualification, community mod
  interfaces, and optional controller features such as gyro and native SDL rumble.

These are ongoing fixes and enhancements to a playable game. See
[known issues](known-issues.md) and the [issue register](development/issues/README.md)
for the scope and evidence of each open item.

## Verification and project links

The retained campaign-completion run used infinite-health/enemy auto-kill testing
mods and a save with all Tribals unlocked. The maintainer also confirms original
multiplayer and optional content work; this dashboard records that report, not a
new exhaustive hardware test. Formal deterministic/reference-parity milestones
are separate engineering goals, described in [project status](status.md).

- [Changelog](../CHANGELOG.md), [README](../README.md), and [documentation index](README.md).
- [Report a problem](playtesting.md), including your version and support ZIP.
- [Build and development](development/README.md), [roadmap](planning/JFG_RECOMP_MASTER_PLAN.md), and
  [CI results](https://github.com/TK22-26/JetForceGemini-Recomp/actions/workflows/ci.yml).
- [Credits](../CREDITS.md): written with the help of **AI: Codex GPT 6 Astra**,
  with human direction, review and playtesting.
