# Community branch (unofficial)

Work by Luke Deardoff ([@lukedeardoff](https://github.com/lukedeardoff)), made with AI assistance (Claude).
This branch is upstream `main` plus Luke's local fixes, diagnostics and enhancements. It is not an upstream
contribution; focused fixes are sent upstream separately (for example PR #9).
No ROM data, game assets or generated game code are included. Build it yourself from your own ROM.

Base: TK22-26/JetForceGemini-Recomp `v1.1.0`.

## What it adds on top of main

| Area | Change |
| --- | --- |
| Texture log | `JFG_TEXLOG=1` logs F3DDKR texture images and offset tables (the asteroid fix itself is upstream's BF reset, 500b2a2) |
| Animated model textures | The F3DDKR texture-offset table now works like JFG's own RSP microcode: every texture image in a model uses its table entry, whatever the format. The old GLideN64 rule (RGBA only) froze greyscale animations on frame 1, so water splashes popped in and out instead of widening and fading. `JFG_TEXSHIFT_MODE=2` restores the old rule for comparison, `1` ignores the table |
| Floor reflections | Model triangles are culled like JFG's own RSP microcode: while the game draws mirrored objects it sets `G_CULL_FRONT`, which turns the per-triangle back-face cull into a front-face cull, and double-sided triangles are never culled. The old GLideN64 rule ignored that bit, so faces (Tribals, Vela) were missing from the floor reflections on the ship level |
| Hidden objects | `JFG_HIDE_TEXTURES` skips model draws whose texture has a listed RT64 hash (as shown in the RT64 inspector; comma-separated). The default hides the stretched blue column on the ship level (`004af4b8010a38d8`), which the N64 also draws but much less visibly. `JFG_HIDE_TEXTURES=none` turns it off |
| Cutscene strobing | Skips stale re-presents when a heavy frame has several graphics tasks (`JFG_SKIP_STALE_PRESENTS=0` turns it off) |
| Coffee-throw splat | Hides the floating "sheet" in the intro (`hide_splats`) |
| Widescreen default | Fresh saves default to the game's own widescreen option (`JFG_DEFAULT_WIDESCREEN=0` restores 4:3). Display aspect is upstream's |
| Title/menu vignette | HD replacement texture pack, auto-loaded from `%LOCALAPPDATA%\JFGRecomp\texture-packs` |
| RT64 inspector | `JFG_RT64_DEVELOPER=1` enables F1; pausing (F4) no longer trips the auxiliary-target trap and lands on the 3D scene, so right-click picking and the purple highlight work |
| Crash reports | `jfg-crash-site` line with module+offset per stack frame. A crash also writes `%LOCALAPPDATA%\JFGRecomp\diag\crash-report.txt` (last game instruction, the bad address, game registers, last game calls) and `crash-rdram.bin` (game RAM) for local debugging. The RAM dump contains game data, so do not publish it |
| Actor shadows | Shadow buffers stay on the GPU at HD and are softened by an RT64 pass (`patches/rt64/jfg-shadow-soften.patch`, `JFG_SHADOW_BLUR`, default 1.5, max 2). They use the game's own grey opacity (8/14 of the silhouette, `JFG_SHADOW_OPACITY`, 0-1). RT64's shadow bytes are kept only where the game did not change memory, so reused shadow memory no longer shows as garbage textures. `JFG_SHADOW_MODE=upstream` restores main's CPU-mask writeback for comparison |
| Menu icons in widescreen | Scaled 1-cycle icon texrects get alpha-compare threshold, so the alpha-0 ring is dropped as in 4:3 copy mode (`JFG_MENU_ICON_ALPHA=0` off) |
| Colour grade | Slightly darker and more contrast to match the console: `JFG_GAMMA` (1.08), `JFG_CONTRAST` (1.04), `patches/rt64/jfg-color-grade.patch` |
| Diagnostics | Effect-activity log, frame capture, texture-image log, lens-flare depth trace (`JFG_ZB_TRACE`), translucent-effect log (`JFG_RIPPLE_LOG`) |

Dropped compared with the old `community/cosmetic-vignette-shadows` branch: the backported widescreen
presentation and overlay fixes (now in main).

`community\rebuild-community.ps1` applies the RT64 shadow patch to the build's RT64 checkout before rebuilding.

## Build and play (Windows)

- `community\build-community.ps1` builds from your ROM with the normal pipeline.
- `community\play-community.ps1` runs the newest successful build with the community settings. Test settings
  set with `$env:JFG_...` stay on in that PowerShell window; the script lists any that are still set, so start
  normal play from a new window.
