# AUDIO-002: Frequent audible interruptions in optimized high-refresh preview

Status: **native playback remedy delivered and bounded tests pass; occasional long-stall underrun observed; owner listening acceptance pending** (2026-10-16 UTC).
Severity: high; interruptions occur during ordinary walking as well as combat.

The owner reports substantially more stable gameplay and no noticeable FPS drops
in high-refresh-native-collision-20261016, but frequent audible interruptions
described as underruns. The saved session confirms runtime SHA-256
34ce90c9a11bba75a9ae47f3ae3d8a40d635b20357bf8c52391f4bd015a6bff1,
implementation 8f94f148ec367e4a221679a40255f12e6353af07.
Do not equate the audible symptom with a particular buffer counter without
checking the new capture, or treat muted replay counters as listening verification.

Private evidence: route-capture-20261013/owner-sessions/20261016-062617-3fd7bf54efdd4eab861c018a60fcc3c5;
analysis under route-audio-bridge-20261016. Preserve original timestamps and
profile; heavy evidence stays on the private evidence drive.
Related: [prior source-audio crackle](intermittent-music-crackle.md),
[combat cadence](goldwood-combat-slowdown.md), and
[bridge rendering report](tutorial-late-object-jitter.md).

Plan: inspect recorded queue/task timing and capture provenance; compare source
PCM and playback continuity using the same recorded inputs, with optional
collision acceleration disabled as a control. Audit native clock refresh and
audio scheduling before choosing a remedy. These are hypotheses, not findings.
Consult the pinned upstream audio review JFG-UP-024. Keep tests muted and record
audio to private files if required. No fix or audible validation is claimed.

Transferable lesson: smooth game/presentation counters and zero recorded queue
underflows do not establish continuous, correct audio.

Initial log review confirms 217 audio queue underrun episodes, 42 pause-empty / resume recovery pairs, and zero recorded overruns. The synchronous ROM sample provider was enabled. These facts establish host queue starvation; they do not yet identify why production missed playback demand. The owner run is about six minutes and reaches the later bridge room (26), unlike the earlier short combat fixture.

A muted normal-Play replay using the new input file reproduces 91 queue-empty episodes and two full empty-queue recovery pauses. It diverges from the owner path (rooms 21/47 rather than the late room 26), so it is audio-reproduction evidence only. Initial scheduler observations are retained privately; skipped/coalesced VI notifications may reduce synthesis cadence. No remedy is yet verified.

## Informative candidate failure (2026-10-16 UTC)

The first debt-repayment prototype performed zero repayments: the original
countdown is a signed byte, while the probe initially compared the whole
big-endian word with 2. Source assembly confirms lb/sb at 0x80050764 and
0x80050770. The first stress pair (35/67 empty-buffer events, 5/2 recovery
pauses) therefore does not test the proposed remedy and must not be cited as
improvement. The corrected opt-in prototype selects the high byte and
preserves all adjacent bytes. Observation now reports the actual byte value.
Its controlled repeat is pending. A separate first bridge build rejected
MSVC deprecated getenv under warnings-as-errors; it was corrected to the
existing _dupenv_s convention before tests.

## Controlled missed-retrace experiment (2026-10-16 UTC)

A default-off fixture inserted 23 artificial 35 ms scheduler stalls over the
same bounded input replay. The default-off native candidate retains missed
audio countdown pulses and repays them on subsequent retraces; it does not
advance gameplay or change AI/SDL sample clocks. These injected-stall runs
test causality, not ordinary-play performance or audible acceptance.

- audio-stall-v2-off: 21 queue-empty episodes, 5 actual pause/refill recoveries, 0 debt repayments.
- audio-stall-v2-on: 27 queue-empty episodes, 5 actual pause/refill recoveries, 31 debt repayments.

Private evidence: route-audio-bridge-20261016/audio-controlled-v2-analysis.json. The candidate remains experimental, unshipped and requires an unmodified heavier route test plus clock/state review. Muted tests do not establish listening quality.

## Scheduler task-gate investigation (2026-10-16 UTC)

The corrected countdown-only stress pair still had five pause/refill recoveries
in both variants (21 versus 27 empty-queue events). A longer, heavier logged run
had zero underruns, which alone does not establish improvement. Notification
traces show queued audio notifications accumulating. Original __scSchedule only
runs a ready audio task when sc->doAudio is set, normally at the next audio
notification retrace. A second default-off native candidate sets that existing
gate when a verified ready audio task and a free RSP are present, leaving the
original task scheduler, synthesis and sample clocks in control. Four same-binary
injected-stall combinations compare countdown recovery and this task gate;
results remain private in audio-factor-comparison.json. The 35 ms artificial
stall fixture must be removed before any delivery. No audible acceptance claimed.

The same-binary four-way short stress test did not identify a fix: countdown/task-gate combinations off/off, on/off, off/on and on/on had 3/11/7/4 empty-queue events and 3/2/2/3 pause/refill recoveries. The combined candidate decoded more tasks but delivered only 3000 buffers, so extra synthesis alone is insufficient. A submission-return probe initially failed to build because osAiSetNextBuffer is not in the mapped OS table; the corrected observation wraps its ordinary generated dispatch instead. No delivered build was changed by that failed build.

## Buffer rejection and native demand investigation (2026-10-16 UTC)

The corrected submission probe found 9 rejected osAiSetNextBuffer calls in the
two-slot/countdown/ready-task stress run, versus none in its control. The original
manager ignores that return and synthesizes the next buffer, losing rejected
PCM. Hashes at AI submission and later host copy match: no payload overwrite in
these runs. Host delivery occurs roughly 14-16 ms after the AI write.

A bounded four-slot native AI experiment preserves more catch-up PCM, but still
does not fix starvation: queue-only had 14 empty episodes / 2 refill pauses;
queue plus countdown/ready-task had 9 / 2 and one startup rejection. The latter
also synthesized more 552-sample short blocks. Lost retraces are not a sufficient
measure of actual sample demand. Tests retain exactly two slots by default and
verify opt-in four-slot ordering, sample durations, bounds and interrupt behavior.
The next default-off hypothesis replenishes an 85 ms host queue watermark using
the original audio synthesis and frequency, with bounded notifications and a
ready-task gate. No audio candidate is delivered or audibly verified.

Review of the expanded native queue found a boundary condition: admission must check the final queued deadline, not only the active buffer. The correction rejects aggregate Count overflow while retaining all queued buffers. The physical two-slot path remains unchanged; a near-64-bit-limit regression test covers the four-slot case. This was found before delivery.


## Qualified native playback remedy (2026-10-16 UTC)

The demonstrated chain is lost production time from retrace scheduling,
delayed ready audio tasks, catch-up PCM discarded by FIFO rejection, and late
host delivery. Lost notification counts alone and queue expansion alone failed
the controlled test. The combined remedy uses actual queued playback demand,
ready-task execution, bounded four-slot native retention and immediate PCM
delivery. It retains the original synthesizer, sample frequency, game clock and
separate reference timing path. The native queue policy and exact scope are
documented in [implementation and compatibility](../native-audio-playback.md).

The combined private candidate completes 12 forced 35 ms stalls with zero
underruns, rejected submissions or payload mismatches. Its queue range is about
74-133 ms. The clean six-minute/22,000-retrace run also has zero underruns,
overruns, refill pauses, rejected buffers and payload mismatches. The final
360 FPS capture and 17,000-retrace Original reference comparison pass. Exact
binary hashes, the final overflow-boundary correction, regression checks and
remaining game-update gaps are recorded in
[qualification](../../../evidence/high-refresh-audio-bridge.json).

Transferable lesson: count actual audio samples and host playback demand.
A completed synthesis task is not proof that its PCM was accepted or delivered.
Preserve submission outcomes and payload identity when changing scheduling.
Owner listening verification remains pending; these tests were muted.


## Local preview delivery (2026-10-16 UTC)

Both desktop shortcuts now select source ca6ef7a883b2568da9df2ff0665ec7169c9f4be1 and runtime SHA-256 8dbe521304348ecda93c9f153fbd15f95486b2050b456e37ee221562e30f6a56. Package/launcher setup checks and copied-profile migration pass; all 40 checked live profile files and shortcut arguments are preserved. The previous package and links remain available for rollback. Owner listening and bridge visual review remain pending. [Qualification and delivery](../../../evidence/high-refresh-audio-bridge.json) record the exact scope. This is a local preview, not a public release or completion of the broader no-dip goal.

A later unprofiled comparison control (native ROM fast completion disabled, PCM file capture disabled) reports one audio underrun despite the earlier zero-underrun qualification runs. It is preserved as a new observed limit, not excluded or attributed to the opt-in ROM change. The remaining comparisons must record audio counters before applying acceptance checks. Evidence: native-rom-completion-abba/reference-a and rom-reference-audio-observation under route-audio-bridge-20261016. Owner listening acceptance remains pending.

The later control underrun coincides with a 160.184 ms presentation gap, roughly 168 ms without game/audio progress, and nine coalesced retraces. About 132 ms of queued audio drains before progress resumes. This establishes a long-stall limit; it does not identify the stall initiator or show ordinary walking starvation persists unchanged. Both final native-ROM default replays have zero underruns/overruns, rejected buffers or payload mismatches. Their muted checks do not replace owner listening. Evidence: residual-audio-underrun-timeline and native-rom-audio-late-audit.


The High Refresh Preview and Jitter Capture desktop links now select the native-ROM package: source 8277a80611e73c0dc57285e32e0761e9b7519e41, runtime SHA-256 5f902049bd3e4a991a020f17e63a51d0291f28709057a9aa7efc03097692764b. Launcher/setup checks pass; all 40 checked profile files and shortcut arguments are preserved. Earlier audio and bridge remedies are included. Owner listening/visual acceptance, later renderer/load hitches and the broader no-20-FPS goal remain open. [Qualification and delivery](../../../evidence/high-refresh-native-rom.json).


Local delivery: both High Refresh Preview and Jitter Capture desktop links select source 27ddef0a5ed4afd1f1f9c26a5590b590cda5fe8e, runtime SHA-256 db65b460f399b550ca8a3ad0d98f5374d5e00a95dc4a7073edfa2649fa6772c6. Manifest and launcher/profile checks pass; all 40 checked profile files and shortcut arguments are preserved. Includes native audio, bending bridge interpolation and ROM loading improvements. Owner live acceptance and broader restart/load gaps remain open. [Current qualification](../../../evidence/high-refresh-draw-key.json).


## Longer owner capture completed (2026-10-16 UTC)

The owner reports no perceptible gameplay slowdown, with choppiness and audible interruptions during the Goldwood exit cutscene. This is not a full clean acceptance. The 21-minute recording retains all 131,288 presentation records with no queue drops, but the detailed gameplay/audio trace reached its 256 MiB cap at about 18:27. No unbounded game-update rows or final audio-health totals were written in the live path. The earlier expectation of full-run update/audio coverage was incorrect. Thus later cutscene audio cannot be cleared using the earlier zero counters. Preserve the final report as open; review timing and reproduce the exit under bounded muted tests. Private evidence: final-measurement-20261016, measurement-review.json. The native cutscene/bridge physics conclusions are unchanged.


### Unlimited stress limit during reticle qualification (2026-10-16)
The candidate native-clock Unlimited probe reached about 1,007 output FPS but only about 23 scene updates/s in the measured late window, with 265 audio underruns overall. This failed the qualification gate; it is not counted as a clean pass. The installed pre-reticle runtime comparison (pistol Aim, same recorded inputs/profile) reports 281 underruns; its measurements are preserved in shuriken-reticle-20261016/unlimited-installed-control. This comparison uses the native-clock probe, not normal live playback-demand audio, and different reticle drawing prevents an exact cost comparison. The 120/360 reticle tests remain separate, with zero measured underruns and two-VI updates. Unlimited pacing remains open; do not present high output FPS as proof of healthy simulation/audio.
