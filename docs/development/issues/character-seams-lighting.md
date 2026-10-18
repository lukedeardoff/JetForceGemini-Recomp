# RENDER-002: Character/terrain seams and incorrect character lighting

Status: **owner confirms thigh seams and delayed lighting absent at 120 FPS in
the semantic preview; other poses/modes and exact lighting cause unverified**
(2026-10-13 UTC).
Severity: major visual fidelity problem; gameplay impact unconfirmed.

## Report and reproduction

Juno shows visible gaps at his joints at every tested FPS with VSync enabled.
The owner confirms this also occurs in Original after stopping/restarting the
game. A supplied bridge-scene screenshot also shows pale gaps along terrain and
the bridge entrance. The owner separately reports delayed, inconsistent lighting recovery when
walking back toward the starting area: Juno stays too dark after the bridge,
then eventually brightens. The detailed clarification and comparisons are below. Do not assume these share one cause
or that higher presentation FPS alone causes them.

Affected local high-refresh preview runtime SHA-256:
`b34887ed7da310b11d52d33a1382f83b4c6e521985386d3061226fe6c43d408e`.
Most recent launcher implementation at report: 0f83a2a; runtime unchanged through
the intervening launcher/UI edits. Profile: isolated high-refresh preview.
Evidence: owner's bridge screenshot retained privately as
`high-refresh-20261011/seams-lighting-owner.png`, plus their Original/VSync/Juno
confirmation. Exact baseline comparison and controlled reproduction pending.

## Initial hypotheses and plan

- Compare the same scene/state in this runtime and the retained known-working
  build, beginning with Original mode. Preserve source saves and normal launcher.
- Examine shared geometry/matrix/lighting paths as well as presentation-only
  interpolation. Combined-matrix precision, vertex ownership, transform matching
  and lighting state are hypotheses, not established causes.
- Use a copied replay/profile and captures to distinguish baseline defects from
  branch regressions; verify a candidate at Original and Custom before delivery.
- Existing high-refresh checks establish bounded guest-state and presentation
  behavior, not joint continuity or lighting parity. See [RENDER-001](high-refresh-rendering.md).

## Guard and evidence

The owner explicitly approved reopening the same high-refresh investigation for
six additional guarded attempts and renewed its time window for three hours.
Original roots, twelve attempts/activities and 11470.6854326725 seconds retained;
new caps are eighteen attempts/activities. Guard stays enabled. Admission receipt:
`owner-seams-lighting-3hours.json`; no new investigation identity or counter reset.
The pinned upstream reference review/catalog were consulted; they do not establish
a rendering remedy. All experiments, builds and coding run under the supervisor.

## Validation and resolution

The reproduced high-refresh terrain tearing is corrected and tested in the local
candidate described below. The broader Original-mode joint and lighting reports
remain open. No release or complete visual-fidelity resolution is claimed.

## Controlled baseline comparison

At VI 4004 in the retained tutorial-room replay, Original/VSync-on runtime b34887ed matches reference 26852729 byte-for-byte in the final image, guest RDRAM, renderer snapshot, recorded input and save. Source save unchanged. This excludes a branch regression at that checkpoint only; it does not establish bridge-scene parity. Evidence: `seams-original-comparison/result.json`. A copied preview-save fixture is being checked separately.

Owner requested all tests start muted. Native fixtures use independent `volume=0, muted=1`; the Ares64 reference disables sound in its isolated configuration and runs on an inactive desktop. Personal audio settings are preserved.

A private screenshot-sequence diagnostic build initially failed under `/WX` because MSVC rejects `getenv`; changed its environment read to `_dupenv_s`. No candidate graphics correction or delivery was made by that failed build.

A private geometry diagnostic hit MSVC narrowing warnings from hlslpp vector indices. Corrected bounded component indices to int. The failed build and source are retained privately; no product correction claimed. Original-mode walk captures now reach the bridge without the large terrain cracks shown in the owner screenshot. High-refresh comparison follows; original-mode joint concerns remain open.

## Reproduced high-refresh tearing and first candidate

The muted Original and Custom 360/VSync-on walks use identical copied save and
input. Large cyan terrain/bridge cracks appear only in Custom in this fixture,
matching the supplied screenshot. Original remains intact here. This does not
invalidate the owner's separate Original-mode joint report.

Cause found in the prototype: exact-geometry matching decides motion per vertex
batch, although many batches share the same current transform. A culled/changed
batch can stay at the current endpoint while adjacent matched batches move.
The first candidate makes all batches sharing an exact current matrix use one
unambiguous history transform, or all stay at the endpoint on conflict/unsafe
geometry. Regression fails on the old implementation and passes after the change.
Runtime replay/visual verification pending; no completed fix claimed.

Private evidence: `seams-walk`, `seams-geometry`, `seams-custom` and
`seams-coherence/regression.json`. Diagnostic TSV analysis initially rejected a
raw uint8 light-count field; the parser correction retains the failed receipt.

## Verified candidate and remaining reports

The shared-transform correction removes the reproduced cyan cracks at bridge
approach/crossing captures 2880 and 2940. Five focused renderer tests pass. The
clean executable, with all temporary capture/geometry instrumentation removed,
completes four muted 4,500-VI replays: Original/VSync, Custom 60/VSync,
Custom 360/no VSync and Unlimited/no VSync. All preserve the same guest RDRAM,
renderer snapshot and save. Original's final image is byte-identical to the
pre-correction baseline. Custom and Unlimited still generate moving intermediate
frames. Detailed receipts and short-walk pacing are in
[the aggregate evidence](../../../evidence/high-refresh-transform-coherence.json).

Lighting is not declared fixed. An isolated Ares64 run using a copy of the same
FlashRAM and a scripted walk also darkens Juno on this bridge, then shows him
lit during the far-bank conversation. This is an appearance reference, not
frame-aligned pixel parity. The captured native geometry uses prelit colors
(zero RSP light counts); changing RT64 normal-lighting parameters is unsupported
by this evidence. The owner subsequently clarified the return direction and eventual recovery;
see the controlled follow-up below. The remaining intermittent lighting and
Original-joint reports are not resolved by this first comparison.

Compatibility lesson: a vertex-batch identity is not an independent animation
identity. Culling and topology changes must not split a single matrix's motion.
Conflicting old transforms and unsafe shared geometry hold the whole group at
the endpoint. This may reduce smoothing in ambiguous cases; it does not alter
guest timing, authored transforms, lighting values or feedback pixels. Changed
vertex positions, other characters and broad scene transitions remain outside
this bounded visual qualification.

Private receipts/captures: `seams-clean-qualification/result.json`,
`seams-coherent-candidate`, `seams-reviewed-2880.png`,
`seams-reviewed-2940.png`, and `seams-emulator`. Original failed diagnostic build
and parser receipts remain preserved. Reference source inspected:
[GLideN64 F3DDKR](https://github.com/gonetz/GLideN64/blob/master/src/uCodes/F3DDKR.cpp)
and [vertex pipeline](https://github.com/gonetz/GLideN64/blob/master/src/gSP.cpp),
snapshot hashes d80701e1c3a046b45540fc52757b13fb84646357126a2ec8dc74f38c6bb65f74
and 30327540e7e975e4c7eb856a1b8f29c5a5c75a0ff023129d3a28f64ed75af538.
No upstream implementation was copied or dependency upgraded.

## Owner clarification and local delivery

The owner clarified that Juno darkens when he steps onto the bridge and **stays
dark after he walks off it**. The shadow on the bridge alone is not the reported
failure. Extend the controlled route past the Magnus conversation and compare
the transition in the reference and native runtime before changing lighting.

The shared-transform correction is committed as `dbc4c91f487a5a1b064ed55a49d60fa40dc536e7` and packaged at
`C:/JFG-builds/high-refresh-coherent-preview-20261011`. The existing desktop
preview shortcut now resolves to that package with the same profile arguments.
All 14 package entries pass size/hash validation; the managed frontend resolves
the new runtime through an isolated muted profile. Launcher binary unchanged.
Old package/shortcut retained for rollback. Owner visual confirmation remains
pending; lighting and Original-mode joint reports are still open.

The owner further narrows the lighting report to walking **back toward the
starting area**. Juno eventually brightens again, but the recovery appears late
and inconsistent. Treat this as a delayed transition, not a permanently stuck
light value. The first extended native route shows lighting restored after the
Magnus dialogue, but is the opposite crossing direction; a dedicated on-bridge,
turn-back, off-bridge-and-idle replay is now queued in Original, Custom/VSync,
and the muted Ares64 reference. No lighting remedy is claimed from the outward
route or from the shared-transform seam correction.

## Completed lighting follow-up and handoff

The clarified return route walks forward during VI 4000-4180, reverses during
4240-4500, then stands still through VI 6000. Original/VSync and Custom 360/VSync
both complete muted and finish with identical guest RDRAM, renderer snapshot
and save. Native captures 3090/3120 show Juno dark on the bridge/at its lip;
3150/3180 show brighter lighting on the starting-side approach. Brighter lighting
persists through the following idle captures. The separate muted Ares64 run
shows the same spatial appearance pattern at frames 4320/4350 and 4380/4410.
These sequences are not frame-aligned pixel or precise timing parity: camera
positions, animation phase, resolution/coverage and interpolated presentation
differ. Do not claim the owner's inconsistent extra delay has been disproved.

The longer outward route also completes 10,000 VI in both native modes with
identical final guest/renderer/save data. Juno brightens during and after the
Magnus conversation; the emulator shows this too. That outward route is useful
supporting evidence, but does not substitute for the clarified return direction.
Close-up captures also show some model-joint gaps in the reference; they are not
a matched-pose diagnosis of the owner's Original-mode joint report.

No lighting behavior was changed: an additional intermittent recovery delay has
not been isolated in these routes. The remaining useful reproduction is the
specific path/position where Juno stays dark beyond the starting-side orange
approach, including whether he is moving or standing. A short owner recording
of that case can define the next replay. The Original-mode joint report likewise
needs the affected pose in the delivered build. Both remain **open**; the tested
shared-transform seam correction remains **awaiting owner visual confirmation**.

All temporary capture instrumentation was removed and the clean target rebuilt.
The delivered package remains the exact four-mode-qualified executable SHA-256
`59d828e38c409ce87c2b082d29786d63a72dc901c4b2f298abae9f5a3e07619f`, rather than either diagnostic executable.
Its fourteen manifest entries were revalidated after the follow-up. Muted tests
use disposable profile/save copies; the desktop shortcut preserves the existing
preview profile arguments. No release, main-checkout edit or dependency update
was made. Machine-local delivery only.

Private evidence: `lighting-exit/native-result.json`, `lighting-exit-emulator`,
`lighting-return/comparison.json`, `lighting-return-emulator`,
`lighting-return/restore-result.json` and `seams-package-qualification.json`.
Aggregate public results remain in the linked transform-coherence evidence.
Lesson: distinguish an authored spatial lighting boundary from delayed recovery,
and compare the reported crossing direction before changing brightness or timing.

## Continued character-joint investigation

The owner renewed a three-hour investigation window and clarified that the
noticeable gaps are around Juno's **thighs**. Focus the next comparisons on the
thigh/hip joins through a running cycle in Original and high-refresh modes,
with the original-game reference. The earlier terrain correction is retained.
Distinguish authored mesh separation, raster coverage/precision and interpolation
of independently moving body pieces. No cause or new fix is established yet.

The production guard remains enabled on the same investigation and root, with
thirteen prior attempts/activities and 14832.39721751213 seconds retained.
Existing limits are unchanged (18 attempts/activities, 43200 seconds); the owner
approval bounds this continuation to three hours. All tests start muted, use
copied saves/profiles, and run through the guarded supervisor. Receipt:
`owner-character-seams-3hours.json`.


The owner further clarified the thigh symptom as "a see thru thin line between
parts". Dense muted runs now retain 116 native captures per mode, plus 176 Ares64
reference frames. Original and Custom preserve identical final guest/render/save
data at 640x480 and 1280x960. The initial topology view shows connected thigh
surfaces; exact-coincident vertex checks found no newly separating shared points
in the sampled intermediate poses. This does not cover near-coincident edges,
overlapping separate body pieces, every phase, or GPU raster coverage.

The temporary MSAA4 experiment completed but changed guest and renderer memory
hashes, failing the full-memory comparison assertion; it is not a qualified fix.
The diagnostic history-dump build initially failed from escaped newline syntax;
the corrected v2 run passed. An initial analysis of the failed run had no input
snapshots and is invalid evidence; v2 explicitly requires all 17 snapshots.
All failed receipts are retained. The delivered preview remains unchanged.


Additional bounded checks do not establish a thigh remedy. In the inspected
high-refresh samples, pelvis and leg matrices are held at the current pose;
only gun/hand groups interpolate. Disabling back-face culling did not expose a
missing thigh strip in inspected comparisons. Corrected draw-call indexing
shows opaque sampled thigh materials and alpha 255; the earlier uninitialized
call-offset dump is invalid for material conclusions. Its replacement's first
assertion wrongly required visible actor materials even in off-screen samples;
the corrected qualification records observed and empty frames separately.

The earlier routes used the legacy controller path. A further 4,500-VI pair
exercises modern keyboard/shoulder handling (replay mode 3, modern flag 0x80),
confirmed by pc-shoulder and modern-actions trace events. It includes forward
movement, stopping, reverse input and idle. This is better camera coverage,
not proof of the owner's exact route. The owner is unsure whether the thin
thigh lines persist when standing still. All runs remain muted and isolated.

Private evidence: character-body-motion, character-no-cull,
character-owner-window-v2, character-modern-camera, character-modern-geometry.
Two instrumentation scripts failed on Python quoting/slicing before replay;
corrected v2 receipts are preserved alongside failures. No new remedy shipped.


## Dense thigh follow-up, 2026-10-12

No new character-thigh fix has been established. The modern Original/VSync and
Custom 360/VSync replays each retain 200 captures and end with identical guest
RDRAM, renderer snapshot and save. A denser modern run records 430 workload
snapshots: 215 contain the connected body geometry and 215 do not. In all 215
observed body frames, the connected skin transforms are held at the original
pose. No separating shared vertices are found at the midpoint of presentation
interpolation. Near-coincident boundary pairs in eight frames are around the
head/neck, not demonstrated thigh holes, and do not open during interpolation.

The sampled hip/thigh/knee surface topology is connected. This is a CPU geometry
check, not proof of GPU raster coverage, reference pixel parity or absence of the
owner's reported line. Captured running, stopping, turning and idle views have
not isolated that specific see-through thigh line. A marked close-up or short
recording from the current preview is the next useful reproduction evidence.
Check both the hip and knee joins; the owner could not confirm whether standing
still retains the line. Do not adjust bone size, stitch nearby unrelated vertices
or change global culling/brightness without identifying the affected pixels.

The last recorded owner-profile runtime still names the older border-UI preview
at 2026-10-11 21:22:08 UTC. The desktop shortcut was updated to the coherence
package at 21:57:32 UTC. This does not prove which build the owner has observed,
but it supplies no confirmation of a fresh run of the corrected package. The
managed launcher resolves its own package on both initialization and Play,
so a fully closed/reopened **JFG - High Refresh Preview** selects the corrected
runtime. No personal profile entry was edited during this investigation.

All temporary capture/history/material/AA instrumentation is now removed;
renderer and native-boot source match the existing commit. The clean executable
was rebuilt and completed a muted 4,500-VI modern Custom 360/VSync replay with
the same final guest/renderer/save hashes as the diagnostic pair. All fourteen
delivered package entries were hash/size verified. The desktop package remains
the previously qualified executable; no experimental culling or AA change was
delivered. A leftover diagnostic AA hook caused by nested script variable reuse
was found and removed before that clean rebuild. It was never packaged.

The aggregate [character investigation evidence](../../../evidence/high-refresh-character-seams-investigation.json)
records coverage, failed/invalid diagnostics and clean-build qualification.
Detailed private receipts are in `character-dense-skin`, `character-dense-analysis`
and `character-clean-qualification`. Earlier files remain preserved, including
the invalid empty analyses; only corrected, nonempty observations support the
conclusions above. The production guard stayed enabled on the same investigation
and root with all prior charges retained.

Transferable lessons: a modern-enabled profile alone does not activate modern
handling in a legacy replay; verify replay mode, modern flags and runtime trace.
Separate guest geometry from presented pixels, require actual observed frames in
analysis, and distinguish the last launched runtime from an updated shortcut.
The conservative high-refresh matcher currently holds connected skin at the
original animation rate in this route. Higher output FPS is not a claim that
all actor animation has been interpolated. Lighting recovery remains open and
was not changed by these thigh checks.


## Owner-approved continuation (2026-10-13 UTC)

The owner approved five further hours to continue the open pacing, thigh-gap and
delayed-lighting issues after local semantic-interpolation delivery. The same
production guard remains enabled, retaining 19 attempts, 18 activities and
33222.14147090912 seconds before the new lease. One additional guarded session
is authorized until 05:29:07 UTC; no counters or roots are reset. New large
artifacts use private storage on D:. Tests remain muted with owned hidden windows
and copied profiles/saves. The installed semantic preview remains the comparison
baseline until a new candidate is independently qualified.

First step: correlate retained present intervals, guest update work and graphics
submission waits around the combined-motion three-VI recovery episode. Reuse
existing seam/lighting captures to define an exact pixel or spatial boundary;
avoid repeating broad captures without a new discriminating measurement.


### Owner verification of semantic preview

After closing/reopening the new semantic preview, the owner reports "No seems
at 120". The 120-FPS thigh symptom is therefore absent in this live check. This
supersedes the earlier report that connected skin was held at source cadence:
the semantic implementation now interpolates world-space vertex endpoints with
shared joint/camera ownership. Other frame-rate modes and poses are not newly
qualified by that statement. Lighting recovery remains under investigation.


### Owner follow-up: stationary camera spin (2026-10-13 UTC)

The owner reports only movement/camera jitter remains in the delivered semantic
preview at 120 FPS. The thigh lines and delayed lighting are absent in that
live check; this is scoped owner confirmation, not a claim of all-scene parity.
A more precise jitter reproduction is to stand still and rotate the camera in a
full circle. The view jumps and looks blurred despite a steady 120 OUTPUT /
30 SCENES counter. Present frequency alone is therefore insufficient acceptance:
measure the camera/geometry displacement of consecutive presented images.

Current hypotheses include camera/terrain history falling back as visibility
changes, skipped or repeated interpolation samples, and camera sampling phase.
Separate the broad 20-Hz recovery event from this stable-counter reproduction.
