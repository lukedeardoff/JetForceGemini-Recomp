# Changelog

## 1.2.1 - 2026-10-18

- Fixed modern boss controls losing movement, strafing and separate mouse/controller aiming. Playable boss combat and jumps now retain their configured actions; actual cutscene, pause and death ownership remain intact. Owner playtests passed Vela/Ichor, Juno and Lupus.
- Fixed map exit warps stopping at a door: eligible warps now enter the next room through the native transition, preserving progression requirements.
- Corrected the scene FPS counter counting a second full-screen blur pass at the same source tick. Output FPS is still measured separately.
- Cutscene/boss motion-blur resolution loss, presentation stalls and the earlier long-route audio underruns remain open for 1.3. The controls hotfix includes no experimental rendering changes. See [the 1.3 follow-up](https://github.com/TK22-26/JetForceGemini-Recomp/issues/21).

## 1.2.0 - 2026-10-16

- Added smooth high-refresh rendering with VSync, Custom FPS and Unlimited in
  native Video Settings. Game timing remains independent of presentation rate.
  Character, camera, scenery, pickup and bridge interpolation are improved.
- Reduced combat CPU work and renderer cache stalls; improved audio delivery and
  ROM sample loading. Owner playtesting reports stable gameplay. Unlimited pacing,
  exhaustive no-dip coverage and the reported Goldwood exit-cutscene audio hitch
  remain under investigation; a high output counter is not a game-update guarantee.
- Added the full Modern Controls editor, mouse look and dual-stick aiming, shared
  action bindings, visible menu/dialogue bindings and independent inversion settings.
  Corrected crouch mouse inversion, shuriken reticle loss and resting shuriken jitter.
- Fixed the experimental keyboard/mouse startup crash by initializing game memory
  before controller discovery. The v1.1.0 crash is reproduced with keyboard selected.
- Made native execution-probe declarations and volume-file reads portable to GCC.
- Improved legacy Windows source setup: detect disabled firmware virtualization,
  refresh old WSL distribution support, and distinguish installer failures from
  actual restart requests. Normal ZIP installation still requires no build tools.
- Added an amber update banner below Play when GitHub has a newer stable release.
  **What's new** opens the project page; **Update now** opens the release/download
  page. Checks run in the background and never install updates automatically.

Extract the complete Windows x64 ZIP into a new folder. Existing saves and settings
remain in your profile. Modern gameplay and high-refresh rendering remain opt-in.
See [known issues](docs/known-issues.md) for compatibility and qualification limits.

- Added background stable-release checks, a persistent home-screen update notice,
  and **Help > Check for updatesâ€¦** with installed/latest versions and a release link.
  Offline checks leave gameplay available; downloads are never installed automatically.
- Refreshed the README and rebuilt the project dashboard around the fully playable
  v1 game, with modern-controls highlights, AI assistance credit and a startup image.

## 1.1.0 â€” 2026-10-09

- Fixed the embedded game's aspect ratio. Gameplay stays in a centered 4:3
  display area when the launcher is wide, resized, maximized, or fullscreen.
  Cutscenes retain their own top and bottom bars instead of expanding across
  the launcher window. The original game's graphics and widescreen option
  are unchanged.
- Added one-time ROM import. Select a supported ROM once; the launcher verifies
  and copies it into the local game profile. Future launches use that copy,
  so the original file can be moved or deleted. Invalid imports preserve the
  previous copy. Existing valid ROM selections are imported on upgrade.
- Removed the central ROM card after a successful import. **Game > Select ROMâ€¦**
  remains available to select another file.
- Replaced the first-run local build with a prebuilt Windows x64 game bundle.
  Players no longer need Git, Python, Visual Studio Build Tools, WSL, or a
  first-run compilation. Required Microsoft runtime DLLs are included.
- Added a clear message when only software graphics are available, avoiding
  the stalled startup and excessive memory use observed in the test VM.
- Added package integrity checks, a ROM-import regression test, and viewport
  coverage for 4:3, 16:9, ultrawide, portrait, and odd-sized windows.

Download and extract the complete v1.1.0 ZIP into a new folder. Open
`JFG-Launcher.exe`, use **Select ROMâ€¦** if prompted, and click **Play**. Existing
saves and settings remain in the same profile. The ZIP contains the compiled
program and its runtime libraries; **no ROM or game assets are included**.

The simplified installation approach was inspired by the
[Zelda64Recomp team and its plug-and-play model](https://github.com/Zelda64Recomp/Zelda64Recomp#plug-and-play).
This is an independent implementation.

## 1.0.0 â€” 2026-10-09

Initial stable release with the unified launcher, configurable controllers,
live map and inventory, and the recorded rendering and audio fixes. The v1.0.0
installer built the game locally from the player's ROM and required developer
tools. See the [v1.0.0 release](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.0.0).
