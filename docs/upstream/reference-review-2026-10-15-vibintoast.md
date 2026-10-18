# VibinToast reference review, 2026-10-15

Status: pinned source review, verified release download and muted gameplay cadence capture; combat performance and visual parity remain unqualified. This supplements the October 1 reference
catalog and does not replace its historical observations or the production guard.
The owner requested this comparison while PERF-001 remains open.

## Reviewed artifacts

- [Port source](https://github.com/VibinToast/JetForceGemini-Recomp/tree/356b72d4c38b07b365173c2db160e6b448f76362):
  `356b72d4c38b07b365173c2db160e6b448f76362`.
- The Windows build pins [DKR base](https://github.com/cyscott/dkr-recomp/tree/eb8151c05837359c9eca0ad006077a14f5bf72b8)
  `eb8151c05837359c9eca0ad006077a14f5bf72b8`, N64ModernRuntime
  `13120e9fbdd0ba53e563e516c0ccca1e8ecef573`, N64Recomp
  `c68b1a962367a1c419f2e870246e1bbe97fca250`, and RT64
  `e017b11119a970ee578ba6b7b8b0175fb97a925b`.
- [Release v1.1.3](https://github.com/VibinToast/JetForceGemini-Recomp/releases/tag/v1.1.3)
  was published October 15 at 00:44:24 UTC. The 22,879,364-byte Windows archive
  matches its published SHA-256:
  `a3a08766e065b823a7d01162327f30979cb904b86d06b2af8d73bdd86b6d9964`.
  This verifies download integrity, not source-to-binary reproducibility.

## Confirmed source behavior

[Frame-rate implementation](https://github.com/VibinToast/JetForceGemini-Recomp/blob/356b72d4c38b07b365173c2db160e6b448f76362/src/jfg/jfg_debug_menu.cpp)
provides enabled Original, 60 FPS and 60 FPS plus display-rate modes. The 60 modes
set `runInOneFrame`, replace VI synchronization, and cooperate with scheduler
hooks in `jfg.us.toml`. This changes actual game update scheduling. It is not
merely a higher presentation counter. Timing compatibility across weapons,
animations and the full game has not been established by this review. A stale
comment saying the option is hidden does not match the enabled constant and UI.

The original-rate path also contains explicit recovery from three-VI updates:
it retries after 45 ticks and backs off on repeated failures. Its existence does
not establish that combat always stays above 20 FPS. Our own recovery correction
and real execution-time overrun must likewise remain separate claims.

The decisive interpolation check follows both patches in the published build
order. The port's `RSP::matrixJFG` calls `selectMatrixDKR`. The inherited
[DKR RT64 patch](https://github.com/cyscott/dkr-recomp/blob/eb8151c05837359c9eca0ad006077a14f5bf72b8/submodule-patches/rt64-f3ddkr.patch)
sets matrix identity to `G_EX_ID_IGNORE` and position, rotation, scale, skew,
perspective and tile components to `G_EX_COMPONENT_SKIP`. The
[port patch](https://github.com/VibinToast/JetForceGemini-Recomp/blob/356b72d4c38b07b365173c2db160e6b448f76362/patches/rt64-jfg.patch)
does not replace those settings. Applying both patches to the exact pinned RT64
source confirms this final path. Thus the reviewed source disables transform
interpolation on this JFG matrix path. Display-rate presentation alone cannot
qualify smooth 120-360 FPS object/camera motion. Real 60 Hz updates can still
improve motion. The later `SkipBuffering` choice addresses frame ownership,
not the ignored matrix groups. No explicit character/skeleton/camera identity
pipeline was found in the reviewed port patches.

Base graphics UI exposes Original, Display and a manual 20-360 FPS cap. No
Unlimited selector was found there; the port's combined 60/interpolated mode
overrides the refresh choice to Display. Modern controls, widescreen, post
processing, texture-pack support and debugging have implementation, but their
quality and full-game coverage are not independently qualified here.

## Runtime comparison and limitations

The reference uses N64ModernRuntime to replace operating-system services. Our
current native path executes the guest OS and retains frequent instruction,
clock and device bookkeeping. Lower reference CPU overhead is a plausible
architectural inference, not a measured result yet. A port-wide runtime change
would require controller/save/audio/scheduling validation and is not a small
patch to our renderer.

The release tag is bf93087a40d648bd0393c24260b07e6d9c79ebc4. Its debug-menu implementation, JFG configuration, build batch file and both port patches match the reviewed HEAD. The verified executable SHA-256 is ec3bca1d45bb9dd07084a6b094216ae5fd399f6b0879bd06ed47484fd293f243.

The initial hidden/inactive attempts reached only the launcher. Activating the owned Start control and completing save/character selection reaches actual Goldwood gameplay (game mode 0 with HUD). A private copy of the existing save is imported only into each disposable reference profile, and all audio volume settings are zero. No owner profile, preview or desktop shortcut is replaced.

Earlier forced warps while the front end was active, and a later one-shot teleport that left the player out of bounds before respawning, are invalid combat fixtures. Their timings cannot support a combat comparison. The quiet-gameplay captures use normal level selection, movement, turning and firing near spawn, without warp or teleport. They do not reproduce the owner's drone encounter.

Private evidence IDs: vibintoast-20261015-receipt.json, release API metadata, vibintoast-renderer-review-lf/result.json and matrix-path.txt, vibintoast-runtime-reference.json, vibintoast-focused-start-runs, vibintoast-complete-selection, vibintoast-combat-teleport-runs, and vibintoast-gameplay-cadence. Earlier failed capture attempts remain preserved. The first patch fixture failed because the reviewer's clone converted its patch to CRLF; normalizing the copied patch fixed it. This is a local fixture problem, not an upstream build defect.

## Follow-up and compatibility

Startup is qualified for the captured sequence. Distinguish actual simulation
updates, presented frames and interpolation quality. Keep scripted routes and
settings explicit: our recorded owner route is not automatically portable to a
different runtime. Source inspection does not establish all user-reported
lighting, particles, damage hitches, culling or shadow issues resolved.
Original-mode restoration after switching away from 60 Hz also needs a targeted
check because the reviewed setter has no matching clear at that point.

No upstream implementation has been copied into our runtime in this review.
Respect its GPL-3.0 license and attribution if any implementation is adopted.
PERF-001 and the rendering issues retain their existing open statuses.


## Published-release quiet-gameplay cadence

The requested options file contains frame_rate 0, 1 or 2, respectively. Four independent muted runs reach game mode 0 in level 21. The table uses 300 updates before each final 300-update tail, excluding the close-request interval. It is a quiet-area cadence check, not the owner combat route. The debug output rounds each duration to 0.1 ms.

| Requested setting | Mean update rate | Median / p95 interval, ms | Updates over 50 ms | VI counts per update |
|---|---|---|---|---|
| Original (0) | 30.0 | 33.4 / 34.5 | 0 / 300 | {2: 300} |
| 60 FPS (1) | 30.0 | 32.3 / 64.5 | 100 / 300 | {4: 100, 1: 200} |
| 60 FPS (1), SkipBuffering override | 30.0 | 32.4 / 64.5 | 100 / 300 | {1: 200, 4: 100} |
| 60 plus display (2) | 30.0 | 32.3 / 64.1 | 100 / 300 | {4: 100, 1: 200} |

The non-original captures contain repeating one/one/four-VI updates: fast updates followed by a long stall can produce an apparently steady average near 30. This differs from Original mode pacing and is not evidence of stable 60 Hz simulation. The file settings, logs and captured overlays are preserved; the active UI setting and a fresh pinned-source build are separate checks. Do not label the average counter alone as smoothness or transplant this scheduler override as a validated remedy for our runtime. Evidence: vibintoast-gameplay-cadence/cadence-review.json and retained stderr/image captures.


The fresh pinned-source build did not produce a completed build receipt. Its guarded request/log is preserved as 473a-vibintoast-source-build-invocation; no source-build success is claimed.


## Active setting confirmation and owner capture

The guarded owned-window check opened the published release through normal save/character selection. The RmlUi Visuals tab visibly selected **60 + Interpolated**, while the gameplay overlay and per-update timing still showed about 30 updates/presents per second with the previously recorded uneven pacing. This confirms the requested setting reached the active UI; it does not rely solely on the options file. Evidence: vibintoast-owned-activation-check/visuals-active.png and retained command receipts. Switching to Original through that UI saved frame_rate=0 and restored even two-VI updates in this quiet area. The internal delta changed to 1; broader mode-switch compatibility remains unqualified.

The earlier F9 keyboard injection omitted the scan code. Supplying MapVirtualKeyW and bounded AttachThreadInput foreground activation opened the debug window. No input was sent without checking the window's process and foreground ownership. These were local capture problems, not upstream runtime regressions.

The fresh-source build first stopped at a locally incorrect CMake tool path. After correction, the unmodified upstream batch exited 255 with ".z64 was unexpected at this time." when JFG_ROM contained the owner's parenthesized filename. The conditional error-message echo expands that path without escaping parentheses. A private short-path ROM copy is prepared as the next workaround; no successful source build is claimed. Prior logs and scripts are retained, including the initial pre-execution quoting correction.

The owner offered a manual tutorial run. Automated comparisons and the source build were withdrawn before execution while a separate muted, copied-save release session records per-update game timing, player/level observations and visible FPS. The release binary exposes screen FPS in its overlay but not per-present timestamps in this logging path. Exclude startup and initial waiting when reviewing the route. Evidence: vibintoast-owner-tutorial-20261015. The owner's visual report and resulting logs must qualify combat and later-level conclusions; setup alone is not validation.


Owner tutorial follow-up: severe stalls are confirmed in the recorded Original-mode route, including 15.0-update/s diagnostic windows. The capture inherited debug cheats, forced pistol selection and inactive dual-stick controls; it is not a clean default-game comparison. See [DIAG-002](../development/issues/reference-owner-capture-contamination.md) for the full audit, per-level timing and prepared correction. Missed-input causation and matching original-game targeting remain open; no auto-aim cheat was identified among the enabled overrides.


## Owner-directed end of reference testing

The owner explicitly stopped further upstream debugging. The later source-build retry was canceled before completion; no build success or clean replacement capture is claimed. The earlier automated quiet-cadence runs also inherited gameplay debug overrides, so they are observations under those profiles rather than clean release benchmarks. The contaminated owner recording cannot support a fair overall quality ranking. No reviewed upstream change has demonstrated the stable combat updates required by PERF-001, and none is adopted as a fix. Any retained source idea must prove correctness and benefit in our own implementation. Evidence: owner-cancelled-build receipt and DIAG-002.
