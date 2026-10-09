# Community branch (unofficial)

Work by Luke Deardoff ([@lukedeardoff](https://github.com/lukedeardoff)), made with AI assistance (Claude).
This branch is upstream `main` plus Luke's local fixes, diagnostics and enhancements. It is not an upstream
contribution; focused fixes are sent upstream separately (for example PR #9).
No ROM data, game assets or generated game code are included. Build it yourself from your own ROM.

Base: TK22-26/JetForceGemini-Recomp `main` at `f55da7d` (after launcher 0.4.0-preview.4).

## What it adds on top of main

| Area | Change |
| --- | --- |
| Intro asteroids | F3DDKR texture-offset table stops after its 40 entries (also PR #9). `JFG_TEXTABLE_ENTRIES` overrides; `JFG_TEXLOG=1` logs texture images |
| Cutscene strobing | Skips stale re-presents when a heavy frame has several graphics tasks (`JFG_SKIP_STALE_PRESENTS=0` turns it off) |
| Coffee-throw splat | Hides the floating "sheet" in the intro (`hide_splats`) |
| Widescreen default | Fresh saves default to the game's own widescreen option (`JFG_DEFAULT_WIDESCREEN=0` restores 4:3). Display aspect is upstream's |
| Lens flares | Renderer writeback is on by default so the flare depth check works from the launcher |
| Title/menu vignette | HD replacement texture pack, auto-loaded from `%LOCALAPPDATA%\JFGRecomp\texture-packs` |
| RT64 inspector | `JFG_RT64_DEVELOPER=1` enables F1; pausing (F4) no longer trips the auxiliary-target trap |
| Crash reports | `jfg-crash-site` line with module+offset per stack frame |
| Diagnostics | Effect-activity log, frame capture, texture-image log, lens-flare depth trace (`JFG_ZB_TRACE`) |

Dropped compared with the old `community/cosmetic-vignette-shadows` branch: the shadow writeback filter and
optional soften pass (upstream now writes the shadow mask back so the game's own CPU blur runs), and the
backported widescreen presentation and overlay fixes (now in main).

## Build and play (Windows)

- `community\build-community.ps1` builds from your ROM with the normal pipeline.
- `community\play-community.ps1` runs the newest successful build with the community settings.
