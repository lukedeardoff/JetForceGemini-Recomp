# Community branch (unofficial)

Work by Luke Deardoff ([@lukedeardoff](https://github.com/lukedeardoff)), made with AI assistance (Claude).
This branch is upstream `main` plus Luke's local fixes, diagnostics and enhancements. It is not an upstream
contribution; focused fixes are sent upstream separately (for example PR #9).
No ROM data, game assets or generated game code are included. Build it yourself from your own ROM.

Base: TK22-26/JetForceGemini-Recomp `v1.2.1` (merged into this branch; earlier work was on `v1.1.0`).

The game runs in the original 60 Hz presentation unless `JFG_VIDEO_CONFIG` points at a `video.ini`.
Upstream's high-refresh modes (Custom FPS, Unlimited) have not been checked with the community changes yet.

## What it adds on top of main

| Area | Change |
| --- | --- |
| Texture log | `JFG_TEXLOG=1` logs F3DDKR texture images and offset tables (the asteroid fix itself is upstream's BF reset, 500b2a2) |
| Animated model textures | The F3DDKR texture-offset table now works like JFG's own RSP microcode: every texture image in a model uses its table entry, whatever the format. The old GLideN64 rule (RGBA only) froze greyscale animations on frame 1, so water splashes popped in and out instead of widening and fading. `JFG_TEXSHIFT_MODE=2` restores the old rule for comparison, `1` ignores the table |
| Floor reflections | Model triangles are culled like JFG's own RSP microcode: while the game draws mirrored objects it sets `G_CULL_FRONT`, which turns the per-triangle back-face cull into a front-face cull, and double-sided triangles are never culled. The old GLideN64 rule ignored that bit, so faces (Tribals, Vela) were missing from the floor reflections on the ship level |
| Cutscene strobing | Skips stale re-presents when a heavy frame has several graphics tasks (`JFG_SKIP_STALE_PRESENTS=0` turns it off) |
| Coffee-throw splat | Hides the floating "sheet" in the intro (`hide_splats`) |
| Widescreen default | Fresh saves default to the game's own widescreen option (`JFG_DEFAULT_WIDESCREEN=0` restores 4:3). Display aspect is upstream's |
| Title/menu vignette | HD replacement texture pack, auto-loaded from `%LOCALAPPDATA%\JFGRecomp\texture-packs` |
| RT64 inspector | `JFG_RT64_DEVELOPER=1` enables F1; pausing (F4) no longer trips the auxiliary-target trap and lands on the 3D scene, so right-click picking and the purple highlight work. The pause now waits (up to 12 presents) until the screen shows the scene's own buffer, since RT64 draws the highlight into the paused scene but keeps showing the buffer of the last present. `diag\inspector.txt` logs each pause: the scene task, the buffer shown, and which calls carry the highlight |
| Crash reports | `jfg-crash-site` line with module+offset per stack frame. A crash also writes `%LOCALAPPDATA%\JFGRecomp\diag\crash-report.txt` (last game instruction, the bad address, game registers, last game calls) and `crash-rdram.bin` (game RAM) for local debugging. The RAM dump contains game data, so do not publish it |
| Actor shadows | Shadow buffers stay on the GPU at HD and are softened by an RT64 pass (`patches/rt64/jfg-shadow-soften.patch`, `JFG_SHADOW_BLUR`, default 1.5, max 2). They use the game's own grey opacity (8/14 of the silhouette, `JFG_SHADOW_OPACITY`, 0-1). RT64's shadow bytes are kept only where the game did not change memory, so reused shadow memory no longer shows as garbage textures. `JFG_SHADOW_MODE=upstream` restores main's CPU-mask writeback for comparison |
| Menu icons in widescreen | Scaled 1-cycle icon texrects get alpha-compare threshold, so the alpha-0 ring is dropped as in 4:3 copy mode (`JFG_MENU_ICON_ALPHA=0` off) |
| Colour grade | Slightly darker and more contrast to match the console: `JFG_GAMMA` (1.08), `JFG_CONTRAST` (1.04), `patches/rt64/jfg-color-grade.patch` |
| Save backups | `play-community.ps1` copies `jfg.flash` to `profiles\default\backups` before each session (newest 30 kept) |
| F9 recording | In the game window, F9 starts and F9 again stops a recording: every 2nd frame is saved to `%LOCALAPPDATA%\JFGRecomp\diag\frames\run-<time>\` (`JFG_RECORD_EVERY`, 0 = timing only) and start/stop marks go into the effect log, so the frame timing of that stretch can be checked. The old timed and shrink-beam captures are now off unless `JFG_EFFECT_CAPTURE`/`JFG_CAPTURE_EVERY` are set |
| Audio watch | `AU` lines in the effect log (`diag\effect-activity.tsv`): audio underruns, queued audio and host lateness per VI, written when audio runs low, an underrun is counted, the host is late, and once a second |
| Faster graphics tasks | Each graphics task (the scene, and one per character shadow) used to restore every overlay module into its 8 MiB snapshot and import all 8 MiB into RT64, and every present wiped RT64's copy of the modules. Now only modules a task actually changed are restored and imported, and the present imports only the game's 4 MiB. Busy scenes with many characters no longer fall behind real time (which ran the audio dry). `JFG_RANGED_IMPORT=0` restores the old full copies. `PF` lines in the effect log give the per-second cost of each step |
| No display-list culling | JFG's own RSP microcode ignores command `0xBE` (F3D's "cull display list"), so the console never skips a block of geometry there. The port culled on it, which can leave holes that show old frames (suspected cause of the straight-edged panel cutting through the title hallway). `0xBE` is now ignored like on the console. `JFG_CULLDL=1` restores the old cull |
| Faster ROM reads | The game's `romCopy` (every asset read from the cartridge) went through the original PI DMA, its interrupt and about five thread switches, which costs this port ~150-250 µs per call. Level loads make ~2000 of them, and the burst of tiny object reads at the end of a load ran slower than real time and emptied the audio queue (a short sound cut when entering some areas). The bytes are now copied directly (play mode only, after a check of the routine's code). Loads finish a little sooner; the few reads during play (a couple every few frames) also skip the DMA wait. `JFG_NATIVE_ROMCOPY=0` restores the original path. The last two `PF` fields count these calls and KiB per second |
| Title hallway slivers | A port choice, not console behaviour. Behind the title hallway's glass the game draws an outside wall, then a floor, a few thin hallway pieces and the glass tube, without depth, each over the one before. The floor's far tip, a thin dark line and the far end of the glass tube lie behind the wall but showed on top of it as a pinkish box between Juno and Vela. On the title and menus the wall now writes depth and the floor, those pieces and the glass test against it, so the wall covers them; the near floor and the glass in front of the wall stay. `JFG_TITLE_DEPTH=0` turns it off |
| Device-event tie | Two devices due on exactly the same cycle (seen on the title: the RSP task finishing and an audio DMA, VI 31293) stopped the game with `unqualified-event-tie` / exit 4. In play mode the lower device now completes first, in the native batch order (VI, PI, SI, Compare, SP, DP, AI), and the other one completes at the next boundary. Each tie writes a `TIE` line to the effect log. `JFG_STRICT_EVENT_TIES=1` restores the stop |
| Anubis exit booster glows | In the cutscene where the ship leaves the S.S. Anubis, two jet glows stay floating in front of the hangar (also in the original). The cutscene hides one ship (object flags 0x0C00, which `objPrintObject` checks) but `animshipDrawThrusters` (overlay 73, 0x049003F4) still draws its jets. Jets of hidden ships are now skipped. Each call writes an `AT` line to the effect log. `JFG_HIDE_HIDDEN_THRUSTERS=0` restores the original |
| Cutscene camera boxes | A small grey box with an orange square floated in cutscenes (Vela's cutscenes and others). It is the model of a cutscene camera object (header name `AnimCamera`); the cutscene engine clears the object's hidden flag (0x0400) when it starts using that camera. The hidden flag is set again on every `AnimCamera` object at the start of each update, so the box is never drawn. Each change writes an `AC` line to the effect log. `JFG_HIDE_ANIM_CAMERAS=0` restores the original |
| Game-flag log (diagnostic) | The pause-menu pod (menuScreen, overlay 18) shows two of its eight icons only when game flags 95 (0x5F) and 100 (0x64) are set. Every mainSetGameFlag call writes a `GS frame flag value caller` line to the effect log, and the first read of flags 95/100 from each caller writes a `GG` line. `JFG_MENU_FLAG=<n>` makes reads of flag n return true without changing the save, to see which icon it unlocks |
| Diagnostics | Effect-activity log, frame capture, texture-image log, lens-flare depth trace (`JFG_ZB_TRACE`), translucent-effect log (`JFG_RIPPLE_LOG`) |

Dropped compared with the old `community/cosmetic-vignette-shadows` branch: the backported widescreen
presentation and overlay fixes (now in main).

`community\rebuild-community.ps1` applies the RT64 shadow patch to the build's RT64 checkout before rebuilding.

## Build and play (Windows)

- `community\build-community.ps1` builds from your ROM with the normal pipeline.
- `community\play-community.ps1` runs the newest successful build with the community settings. Test settings
  set with `$env:JFG_...` stay on in that PowerShell window; the script lists any that are still set, so start
  normal play from a new window.
