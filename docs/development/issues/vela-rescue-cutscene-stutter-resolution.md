# RENDER-011: Vela rescue cutscene stalls and changes apparent resolution

- Follow-up: https://github.com/TK22-26/JetForceGemini-Recomp/issues/21 (1.3, including general performance work).
- Severity: significant visual interruption.
- Status: stalls and interpolation loss confirmed in official v1.2.0; queue
  blocking and same-tick screen passes localized; no qualified remedy yet.
- Reported: 2026-10-18 UTC.
- Affected build: official Windows v1.2.0, Ichor owner capture on the SSD runtime.

## Symptoms and reproduction

The owner observed a large lag spike and sections that suddenly looked very
low resolution when Vela is rescued. This is the rescue cutscene, not Vela's
boss fight. Travel used map warps and health/enemy mods; this is not an
unassisted campaign correctness claim.

## Evidence

Private session: issue20-ichor-controls-20261018/owner-sessions/
20261018-030255-0015cf9d7a56445eaa72c63e1103eb71. Official runtime SHA-256:
72b2844ccaf8c5cc9a93142a7f4b42133b4a33c53ae30ecb31e527fdab40023e.
The executable is on C: SSD; capture files are on D:. Immutable guarded review
steps 220, 222 and 223 read already-recorded data while the owner continued
playing. No concurrent game, build, process-memory probe or runtime change was
used for this review. Private analysis folders are live-rescue-review-01,
live-rescue-detail-02 and live-rescue-sync-03 in that investigation.

The rescue spans approximately 521-595 seconds into the capture. The following
within-shot update gaps exclude the first row after a level/shot change:

| Shot identifier | VI range | Updates | Gaps above 42 ms | Largest gap |
| --- | --- | ---: | ---: | ---: |
| 378 | 30417-31267 | 426 | 0 | below 42 ms |
| 379 | 31281-31933 | 312 | 20 | 115.634 ms |
| 344 | 31950-32575 | 291 | 44 | 51.853 ms |
| 380, first occurrence | 32590-33398 | 324 | 118 | 134.401 ms |
| 345 | 33410-33971 | 282 | 0 | below 42 ms |
| 380, second occurrence | 33983-34749 | 384 | 0 | below 42 ms |

Audio underrun and overrun counters remain zero through this sequence. Larger
transition-spanning maxima in raw stage statistics are not counted as in-shot
stalls. A separate earlier 2.44-second gap is outside the rescue and must be
checked against pause events before being called a runtime performance fault.

## Localized stalls

The first shot 380 has renderer workload waits for presentation of up to
68.693 ms, followed by offscreen submission queue-slot waits up to 65.090 ms.
The latter site's p99 is 31.673 ms. The measured framebuffer GPU fence waits
in the same shot are at most 1.390 ms, and texture-upload waits at most
2.008 ms. This localizes a substantial delay to presentation/queue scheduling;
it does not establish exclusive GPU execution cost or explain every update.
Nested and overlapping waits must not be summed as independent CPU percentages.

The existing 1.3 opening-village investigation has a demonstrated stale
presentation-interval feedback problem and an unreleased queue-ownership
correction. Vela's long waits and expanding 4/6/8/10/12-output sequences are
consistent with that mechanism, but a controlled Vela comparison has not yet
tested that candidate. See [PERF-004](opening-village-and-cutscene-aspect.md).

## Interpolation loss and apparent resolution

Shot 344 presents almost exclusively single-output sequences (583 of 595
observed outputs), with a typical 16.808 ms output interval. Shot 345 presents
282 single-output sequences, typically 33.420 ms apart, despite stable game
updates. Ordinary neighboring shots produce four outputs per source frame.

The trace identifies a second screen-sized pass within the same VI tick:
at VI 33413 a 5,769-vertex scene renders four outputs at source rate 30, then
a scene-classified pass with zero world vertices records source rate zero;
the displayed output uses that latter single-frame sequence. A repeated tick
does not have a positive interpolation interval in PresentationHistory::record.
The current scene classifier admits screen-sized color writes without requiring
world geometry, so such a pass can replace scene history/counters. This is a
concrete lead for the interpolation loss; determine the actual copy/filter/fade
operation before changing classification or skipping any pass. HUDs, CPU
reticles, fades and legitimate 2D scenes must retain their content and ordering.

Those two shots also introduce color target 0x15af40 alongside 0x10ff00 and
0x135700. Native VI width remains 320 throughout; the cinematic mode changes
once at rescue entry and returns at exit, rather than toggling for each shot.
This does not establish the host render-target resolution. The pinned RT64
presenter has a native-resolution RAM fallback, but the current trace does not
record whether it was selected. A framebuffer-copy/effect path is another
possibility. Do not label either as the demonstrated pixel-resolution cause.

## Attempts, validation and next checks

No cutscene runtime change, new build or replay has been made for this report.
The owner recording remains active. Timing/input/interpolation logs are not
a video recording and do not prove the apparent pixel resolution. Preserve
checkpoint saves and observed warp commands for a later targeted reproduction.
After the owner closes the capture, compare the released runtime and existing
1.3 scheduling candidate on the same rescue. Record screen-pass operation,
source/destination targets and actual selected presentation resolution where
current evidence is insufficient. Boss controls remain the 1.2.1 priority.

Detailed gameplay/actor tracing reached its 256 MiB limit at 949.097411 seconds,
after the rescue. Compact game timing, frame/output logs, inputs and renderer
trace continue independently. A read-only live-context observer preserves later
room/control-lock changes for the boss investigation; it does not replace the
missing detailed actor stream. See [PERF-002](frame-diagnostics.md).

## Lessons and compatibility

One logical scene can contain multiple screen-sized graphics passes. Frame
ownership must follow the displayed scene while preserving effects, rather
than treating every full-width color write as an independent animation frame.
Keep scheduling, interpolation and apparent-resolution evidence distinct until
a shared cause is demonstrated. Do not promote partial improvement to a fix.


## Related boss-room rendering report

During the same official capture the owner reports lower apparent resolution
and poor frame rate in the Ichor boss fight (level 181), alongside the modern
controls failure tracked in [INPUT-011](github-ichor-boss-controls.md).
Two supplied screenshots show OUTPUT 47.9 / SCENES 59.9 and OUTPUT 59.8 /
SCENES 59.8. Compact game-update rows still advance by two VI. A screen-sized
zero-world-vertex pass again follows the 3D scene at the same tick and records
source rate zero, replacing the displayed sequence. This is a shared rendering
lead with rescue shots 344/345, not proof of the apparent-resolution cause.
Private evidence: owner-boss-failure-live-01. Keep input and rendering acceptance
separate; neither report is resolved by the unbuilt input candidate.


## Closed capture: motion-blur source and GPU-copy policy

The final boss memory retains the zero-geometry lists at 0x8033c7b0 and
0x8033e7b0. Their commands blend the prior screen in four-row texture strips,
with environment alpha 100, into alternating screen buffers. The pinned blur
overlay's blurUpdate (virtual 0x030000f4, section 47) builds this exact pattern;
blurTask submits its dedicated graphics task. This identifies the extra pass
as the native motion-blur effect, rather than unloaded world geometry.

The released RT64 shell disables framebuffer.copyWithGPU globally for CPU
writeback/shadow-mask compatibility. RT64's texture overlap checks therefore
skip GPU framebuffer tile copies, so this effect loads native-resolution RAM
pixels. This source path explains a concrete mechanism for losing detail;
an A/B image and native rendering validation are still required before claiming
the apparent-resolution report fixed. Re-enabling GPU copies globally would
also bypass CPU-modified masks and overlays, so is not a qualified remedy.

The second full-screen submission also replaces same-tick scene history with
a zero-rate, single-output snapshot. Preserve the original compositing effect,
CPU overlays and offscreen ordering when correcting frame ownership. No blur
removal, resolution patch or interpolation repair has been accepted yet.

Capture is now closed. Final compact play-health reports 10 audio underruns
over the full route, beyond the capped detailed trace. They remain unresolved;
the absence of underruns during the earlier rescue interval is narrower.
Private full-list/route evidence: blur-and-tail-route-review.json.


## Closed-capture rendering follow-up and rejected prototype

Full compact timing and frame logging survive closure. The final health record
contains 10 audio underruns and zero overruns; no individual underrun timestamps
exist in that compact stream. Detailed gameplay tracing capped earlier, so the
underruns cannot all be assigned to specific shots. Sparse room attribution
finds 7,151 two-VI intervals and four three-VI intervals in the late boss section.
Its output intervals have a 17.068 ms median and 35.062 ms 99th percentile.
This is a real output/presentation failure; the scene counter alone is misleading.
Private full review: long-owner-capture-review.json.

The identified 623-command native blur pass samples a 320x240 RGBA16 framebuffer.
Global GPU framebuffer reuse is disabled for CPU-softened shadow compatibility,
so this screen pass also falls back to the low-resolution RAM image. A bounded
opt-in experiment enabled GPU copies only for a fully recognized blur pass.
Both captured lists and 1,246 command mutations pass its recognition checks.
However, native route testing becomes slower and terminates at VI 13603 on the
existing fail-closed guest event-tie check (0x8003f5d8). This experiment fails
qualification and is removed from the hotfix; all source, logs and final memory
are retained privately. No guest timing check was disabled.

A planned grouped-world/blur experiment was not built because its prerequisite
failed. Queue-slot lifetime and compositing still require independent validation.
The focused 1.2.1 preview contains the controls/exit-contact/scene-counter changes,
not GPU-copy or composite-queue experiments. Rendering resolution, 120 FPS
presentation in blur scenes, and the remaining audio events are still open.


## 2026-10-18 hotfix preview follow-up

The owner reports that Vela's selection scene still appears slow and lower
resolution in 1.2.1-beta.1, while boss controls now work. The supplied screenshot
shows output 27.9 FPS, scenes 25.9 FPS, 35.9 ms and a 76.3 ms maximum. This is
reported visual/counter evidence, not a newly qualified timing experiment.
The controls hotfix does not resolve this issue; it remains open for 1.3.
