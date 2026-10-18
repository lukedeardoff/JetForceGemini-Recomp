# Historical known-issues notes before 1.2.0

Superseded as a current-status page by [known issues](../known-issues.md). These chronological notes preserve initial failures and intermediate qualification limits.

# Known issues

See the [issue investigation register](issues/README.md) for evidence,
attempted remedies and verified resolutions.

Check this page before opening a report. It lists confirmed open limitations;
[GitHub issues](https://github.com/TK22-26/JetForceGemini-Recomp/issues)
hold the ongoing discussion, and fixed problems are recorded in the
[runtime fix log](boot-gameplay-fix.md).

## Goldwood combat cadence

The owner's tutorial capture contains real 20-25 scene-update/s intervals and a
damage-correlated hitch, even when rendered output remains near 120 FPS. The
continuation reduces CPU bookkeeping/history overhead with exact guest-state
checks, but native busy scenes still drop below 30. The full-route issue remains
open. Current long native input replays remain in the starting area and reach
death/respawn; they do not establish later-cave route coverage. See
[PERF-001](issues/goldwood-combat-slowdown.md).

Delayed-scene interpolation holds and stale CPU-written presentation endpoints
are corrected in tests. Procedural LightBeam vertices gain explicit ownership,
but the exact late crates/exit marker/bridge visual report remains unqualified.
Ground-shadow interpolation has no qualified fix. See
[RENDER-006](issues/tutorial-late-object-jitter.md) and
[RENDER-007](issues/shadow-cadence.md).

Unlimited's additional queue-window delay remains open. The scene-age candidate
reduced waiting but worsened frame spacing and was rejected. Bounded native rates/audio, exact replay scope and limits
are in [the current qualification](../../evidence/high-refresh-route-fix.json).
Do not treat output FPS as the game-update rate or hidden-window tests as
physical-monitor verification. This is a local preview, not a public release.

## Extended native stability qualification

A stale normalized overlay cache pointer was captured writing into a reused
guest allocation. The fresh-load correction passes the targeted ownership watch,
room returns, actual pause/resume and extended native 120/360 tests. Broader live
verification remains pending; these tests do not assign every historical native
exception to the same cause. The local high-refresh desktop preview now includes the qualified
correction; broader live stability and visual acceptance remain open. See
[NATIVE-001](issues/native-play-access-violation.md) and
[the continuation evidence](../../evidence/high-refresh-combat-focus.json).

## Character seams and lighting

The owner reports the thigh seams and delayed lighting are absent at 120 FPS
in the semantic high-refresh preview. Terrain interpolation cracks were also
corrected. Other poses/modes remain unverified, and the intermittent lighting
delay's exact cause has not been isolated. See
[RENDER-002](issues/character-seams-lighting.md).

## High-refresh rendering

A newer capped-presentation correction prevents repeated completed interpolation
windows and shadow-only truncation. It passes bounded camera-pan/full-circle
checks, with owner/physical-display verification pending. Genuine workload spikes remain; the latest native preview removes the historical
31-fast-frame recovery hold after a slowdown. See
[RENDER-003](issues/steady-fps-jitter.md).
Further private tests reproduce a finite-clock/presentation-queue mismatch that
can skip a rendered scene while the FPS counter remains high. A broad queue
candidate caused audio/source-cadence regressions at 360 FPS and was rejected.
The installed preview retains the previously qualified presentation policy.


The [high-refresh candidate](../features/high-refresh-rendering.md) has RmlUi
Video Settings in a separate Windows window with VSync, a Custom FPS target and Unlimited. A short replay
exceeds 360 presentations/s while preserving guest state. Unlimited pacing remains
uneven, and its full replay took about 2% longer than Original. Sustained performance,
complete semantic identity coverage, animation/effects, CPU overlays, transitions, audio-device
behavior and physical 360+ Hz display qualification are incomplete. Original remains
the default. Video Settings positioning, dropdown text alignment, simplified chrome and continuous FPS preset borders are tested; owner confirmation remains pending. See [RENDER-001](issues/high-refresh-rendering.md).

### Separate Modern Controls preview

The owner's recorded drone fight drops to approximately 15 game updates per
second in its busiest section. This is an actual update-cadence deficit, separate
from the fixed modern camera-distance stepping. Detailed logging alone does not
explain it. A native elapsed-time candidate is now available in the local Modern Controls
Preview after hidden replay qualification. Remaining slow updates and owner live
verification keep this issue critical and open; this is not a released fix. See the [timing investigation](../features/pc-controls.md#2026-10-11-critical-goldwood-timing-investigation)
for measurements, experiments and the remaining acceptance work.

## Windows 10 setup

**v1.1.0 uses a prebuilt game and one-time ROM import.** The WSL/developer-tool
setup problems below concern the old v1.0-and-earlier local-build installer or
source builds; ordinary v1.1.0 installation no longer requires those steps.

[Issue 4](https://github.com/TK22-26/JetForceGemini-Recomp/issues/4) reports
setup failures on Windows 10 with Windows PowerShell 5.1:

- Disabled firmware virtualization can cause repeated restart instructions.
- Older WSL installations may reject the requested Ubuntu distribution.
- Missing Linux packages can stop setup at the package check.
- Long RT64 dependency paths could fail checkout, and a retry could skip
  incomplete submodules.

Since 0.4.0-preview.2, setup uses short build and dependency cache paths,
enables Git long paths for dependency downloads, retries submodule
initialization and preserves native stderr and exit codes under PowerShell 5.1.
These changes have automated coverage, but the reporter's full Windows 10
sequence still needs a retest. Firmware-virtualization detection, older WSL
distributions and prerequisite probes such as `Test-LinuxPackages` remain open,
so repeated restarts alone will not fix every failure. If you are comfortable
installing prerequisites yourself, use [manual build setup](rom-bootstrap.md).

## Release and source versions

v1.1.0 ships a prebuilt game from its recorded source revision. Current source
and development builds can contain newer features, including the controls
overhaul and launcher update checker, which are not yet in that published ZIP.
Include your launcher version and game build when reporting. The new checker's
availability and validation are recorded in [LAUNCHER-001](issues/launcher-update-discovery.md).
[Getting started](../getting-started.md) explains release and source-build setup.

## Controllers

The v1.1.0 release supports keyboard and Xbox/XInput controllers. The current
controls overhaul adds compatible SDL gamepad discovery and Xbox/PlayStation
layouts. XInput rumble is available; native SDL/DualSense rumble, gyro and
adaptive triggers are not implemented. Original local multiplayer is working
per maintainer confirmation; coverage of every device/adapter combination remains open.

[Experimental PC controls](../features/pc-controls.md) are opt-in and
single-player only. Steep camera angles near scenery can hide part of the
player, pointer targets on pause subpages remain incomplete, and coverage across
weapons, characters and modes is incomplete. The start/save-selection screens and
main pause page have mouse targets in the current modern-controls preview.

## Gameplay and rendering coverage

The maintainer completed the campaign with optional infinite-health and enemy
auto-kill mods and a save with all Tribals unlocked; saving and loading are
verified. The maintainer confirms the full game, original multiplayer and
optional content are playable. The retained assisted run is not an exhaustive
unassisted or hardware-compatibility test; original-console accuracy remains under validation. Player-shadow processing
is verified for Juno only, and Juno's water splash is untested. Report a
specific failure with its location and steps using the [playtesting guide](../playtesting.md).

## Support reports

Native crash stacks and freeze capture require a game built by 0.4.0-preview.2
or later. Capture is best effort: forced termination, early startup failures,
corrupted stacks or access restrictions can leave partial logs. Reports are
text stack snapshots, not memory dumps, and nothing uploads automatically.

## Navigation mod

The opt-in [navigation mod](../features/navigation-mod.md) can walk, jump, open
chests, collect NPC rewards and cross exits, but autonomous campaign
completion is not established. Lupus hover, moving platforms, crawling,
shooting and explosive gates and unsupported scripted traversal still need
manual play. Normal gameplay leaves the mod off.


## High-refresh diagnostics and remaining visual coverage

F8 reports OUTPUT and SCENES separately and asynchronous CSV logging records
successful presents. A high output counter does not establish unique game poses
or monitor scanout. Semantic character/camera, gem and drone corrections passed
bounded replays; the owner confirmed gems and drones look good. Pickup-particle
lifetime corrections are locally delivered, with live acceptance pending.
The owner subsequently reported later-cave crates, exit light and bridge jitter,
and the full tutorial log confirms intermittent source cadence drops. Those
remain open. Historical attempts, failures, audio/Unlimited limits and delivery
receipts are preserved in [PERF-002](issues/frame-diagnostics.md),
[RENDER-003](issues/steady-fps-jitter.md),
[RENDER-004](issues/actor-jitter-edge-culling.md),
[RENDER-005](issues/pickup-particle-desync.md) and
[RENDER-006](issues/tutorial-late-object-jitter.md).

## Background-music static

Goldwood music static in the experimental native-timing preview was traced to stale compressed sample data. The local preview now loads requested samples synchronously before consumption. Source-integrity checks and normal-Play integration pass, and the owner confirmed on 2026-10-13 that the audio is fixed in the delivered local preview. See [AUDIO-001: cause, remedy and verification limits](issues/intermittent-music-crackle.md). No complete audio fix is publicly released.

## Modern controls: transitions, jumping and focus

Candidate corrections for modern area-entry facing, jump momentum/air steering and focus-loss camera reset are verified in focused tests and delivered in the local v1.1 preview. Continuously active modern input reproduced a 180-degree Goldwood entry error; the corrected heading matches the original control path. The owner confirmed all three fixes in live play on 2026-10-13, and they are committed as 7b1ea4b; broader device and unusual-state coverage remains open. See [causes, regression evidence and live checks](issues/modern-controller-state-transitions.md).

## Modern camera: terrain bounce and pond-lip contact

The revised local Modern Controls preview corrects two reproduced causes: coarse
distance halving and a native height correction above clear finite pond geometry.
The reconstructed Goldwood area-47 pond route now holds full camera distance
through the prior 200-to-100 pull-ins; mouse, low-angle terrain, real obstruction
and original Controller Mapping regressions pass. Owner live verification and
broader scenery/moving-model coverage remain pending. Unknown geometry retains
the native response. This is not a complete swept-volume camera overhaul.

Use the Desktop JFG Camera Capture shortcut to record the next live reproduction.
See [CAMERA-005](issues/camera-collision-bounce.md) for causes, failed
and successful attempts, evidence, current package and limits. CAMERA-002 remains
open for broader scenery-driven pull-ins.

## High-refresh develop integration (2026-10-13)

The combined candidate passes focused build/UI/state, muted 120/360 combat and
camera replays. Its live viewport notification conflict is corrected. See
[qualification](../../evidence/high-refresh-develop-merge.json). This does not
resolve the owner full-tutorial cadence drops or late-cave object jitter;
[RENDER-006](issues/tutorial-late-object-jitter.md) remains open.

The qualified merged preview is now installed through the existing **JFG - High
Refresh Preview** desktop shortcut. Save/settings are preserved. Its merge commit
is `f70b513c896df107163f2d72abed344a87dc2517`; the full-tutorial and late-cave
acceptance limits above remain open. No public release.

The explicit route-capture launcher now records interpolation matching/phase
alongside camera/actor/input events and frame timing. Setup smoke passes; the
owner's repeated route and visual acceptance are pending. Initial idle waiting
is excluded from active-route analysis, preserving raw replay timestamps.
This diagnostic addition does not resolve RENDER-006.

## Expanded route review status (2026-10-13)

The owner's repeated capture is complete. Initial idle time is excluded using
first active gameplay input while raw recording times remain preserved.
Damage correlates with real 20-22 SCENES intervals. A completed framebuffer-write
pacing correction and stationary-object camera-history extension pass bounded
checks, but the exact full-route slowdown and visible light/bridge jitter remain
open. The bounded 18-hit fixture stays near 120/30 with normal and expanded
logging. Ground shadows lack caster-owned high-refresh mask/UV handling; the
reported visible lag and its remedy are unverified. See
[RENDER-006](issues/tutorial-late-object-jitter.md),
[RENDER-007](issues/shadow-cadence.md), and
[qualification](../../evidence/high-refresh-route-review.json).

The local `high-refresh-route-review-20261013` candidate is now installed through
both High Refresh Preview and Jitter Capture shortcuts. Source `34c9986224c0508a45d1f7d1caa10f2b6b377526`;
existing profiles and diagnostics arguments are preserved. This delivery does
not close the three remaining owner reports above. See
[delivery evidence](../../evidence/high-refresh-route-review.json).

The current local candidate is **high-refresh-route-fix-20261013**, implementation
aaae085c7be7d6ce140cde06c683ef19082470ee, selected by both preview/capture desktop shortcuts.
It supersedes the older route-review package. Saves and settings are preserved;
combat drops, exact late-room jitter, shadow cadence and Unlimited starvation
remain open. See [current delivery evidence](../../evidence/high-refresh-route-fix.json).


The 2026-10-14 camera supplement now binds missing scenery-history anchors to
verified camera lifetimes and admits small common lens-scale changes. Two long
recorded routes gain matches with no lost/changed existing anchors; the full
Original reference retains identical state and framebuffer results. This is a
source candidate only. [RENDER-006](issues/tutorial-late-object-jitter.md)
records the limits; native stability, combat drops, shadow cadence, Unlimited
pacing and owner-visible full-route acceptance remain open. Desktop delivery is
withheld under NATIVE-001.


The local compact-timing preview reduces recorded-combat update work by about 13%, with exact tested-state correspondence, but native 20-FPS updates remain. This does not resolve the Goldwood combat cadence issue. Current qualification and delivery are recorded in [PERF-001](issues/goldwood-combat-slowdown.md) and [the component receipt](../../evidence/high-refresh-compact-timing.json).


The latest local profiled-math preview adds a qualified resident-math CPU saving and detailed opt-in profiling. This does not close the Goldwood combat cadence issue: native tests still contain occasional 20-FPS updates. Full profiling has substantial observer overhead and is unsuitable for ordinary FPS acceptance. See [PERF-001](issues/goldwood-combat-slowdown.md), [PERF-002](issues/frame-diagnostics.md), and [delivery evidence](../../evidence/high-refresh-resident-math.json).


The latest local preview adds bounded native collision traversal and expanded profiling. Repeated combat tests show reduced CPU work, but bounded runs cannot establish all-level no-dip or visual acceptance. Every missed update remains visible in the new cadence report, including level/death transitions; present-return stalls are reported separately. Both desktop shortcuts select the qualified package and retain their profiles. Detailed function profiling remains off for normal play. See the current native-collision qualification and PERF-001/PERF-002 records.
[Native collision and current evidence](native-collision.md).

- 2026-10-16 preview review: frequent [audible interruptions](issues/walking-audio-interruptions.md) and [late bridge strobing](issues/tutorial-late-object-jitter.md) remain open despite improved perceived frame stability. Owner independently confirmed bridge jump/settling behavior matches the original game.


The 2026-10-16 native audio/bridge candidate passes bounded muted playback and rendering checks. Native playback follows sample demand and immediately delivers completed PCM; the bridge now interpolates its bending vertices. Owner listening/visual acceptance remains pending, as do broader object/shadow coverage and the no-20-FPS goal. [Implementation, compatibility and qualification](native-audio-playback.md).


Both High Refresh Preview and Jitter Capture now select the local audio/bridge package with existing profiles and arguments. Owner listening and visual review remain pending. [Current implementation and evidence](native-audio-playback.md).


The native ROM loading improvement is qualified for local packaging: the tested 120/360 combat windows hold two-VI game updates with zero audio errors, and Original/recorded-clock comparisons pass. Later renderer/cold-load gaps and owner audio/bridge acceptance remain open. The isolated control underrun coincided with a roughly 160 ms game/presentation stall. [Measurements and scope](native-rom-loading.md).


Both desktop shortcuts now select the qualified native-ROM loading package, including audio/bridge remedies. Profiles and launcher arguments are preserved. Later hitches and owner listening/visual acceptance remain open. [Current local preview](native-rom-loading.md).


A renderer draw-call cache defect is corrected and passes bounded qualification: the repeated late renderer hitch is absent in the unprofiled candidate replay. Restart/load gaps, exact owner-route coverage and listening/visual acceptance remain open. [Cause and qualification](issues/renderer-cache-key-instability.md).


Both desktop links now select the qualified draw-call key correction, including audio, bridge and native-ROM improvements. Profiles remain unchanged. [Current local preview](renderer-draw-call-keys.md).


## Shuriken reticle visibility

The shuriken reticle loss is reproduced: high-refresh presentation selects a newer scene buffer while CPU-drawn strokes remain on the game-selected buffer. A retained changed-pixel mask restores the reticle in bounded 120/360 FPS tests, including VSync, movement and Aim release. The qualified remedy is delivered through both local preview shortcuts; the subsequent owner test reports the reticle issue resolved. Unlimited stress still exposes pacing/audio limits. See [RENDER-008](issues/shuriken-reticle-flicker.md).


The owner-requested Modern Controls revision puts Confirm, Back, Save and new keyboard bindings in Keys, retains the left Mouse/WASD lists, removes arrows and supports shared E Interact/Confirm. Core and native UI checks pass; both local preview shortcuts now include the revision and requested shared E. Confirm also advances dialogue. See [INPUT-008](issues/modern-capture-profile-defaults.md). The longer owner test also reports Goldwood exit cutscene choppiness/audio interruption; quantitative full-run audio acceptance remains open because the detailed trace capped before exit.


The local reticle preview also includes compact game-update/final-audio logging and removes periodic route screenshots that caused measured stalls. The reported Goldwood exit-cutscene choppiness/audio interruptions still need a fresh capture. See [current qualification](../../evidence/high-refresh-reticle.json).


The latest followup corrects [reticle pixels lost with skipped VI events](issues/shuriken-reticle-flicker.md), [missing custom shuriken matrix ownership](issues/ground-shuriken-jitter.md), and [state-dependent crouch pitch inversion](issues/crouch-mouse-inversion.md). Both mouse directions, resting-object trace coverage, real-weapon 120/360 reticle checks and bounded normal-Play audio checks pass. The owner subsequently reports these issues resolved in live play; broader coverage remains limited. [Qualification](../../evidence/high-refresh-shuriken-followup.json).


The shuriken/crouch followup is now delivered through both local preview shortcuts. Saved controls, inversion preference, video settings and saves are unchanged. The owner reports the shuriken/crouch fixes resolved in live play; the finite tests do not close Unlimited, all-route performance or the Goldwood exit-cutscene audio report. [Current delivery](../../evidence/high-refresh-shuriken-followup.json).


The owner approved the amber update card below Play. Automatic GitHub stable-release checking, the isolated forced preview, and repository/release button actions pass tests and a live API check. Both normal desktop shortcuts now include the approved card and real update discovery; the forced design preview remains separate. [Details](issues/launcher-update-discovery.md).


The [2026-10-16 open GitHub issue audit](issues/github-release-readiness-20261016.md) does **not** certify a full update as release-ready: the mouse-look startup access violation lacks a verified current-build resolution, the Linux native build still contains Windows-only code, and legacy Windows source setup retains firmware/old-WSL gaps. Controller vertical inversion is present and tested in the current modern-controls UI. No issue was closed or release published by this audit.
