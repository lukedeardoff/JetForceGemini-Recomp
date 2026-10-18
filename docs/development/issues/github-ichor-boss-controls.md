# Ichor boss-room controls stop responding (GitHub #20)

- ID: INPUT-011
- Severity: critical progression blocker; modern keyboard/mouse failure reproduced by owner.
- Status: owner confirms Vela/Ichor, Juno and Lupus controls; native effective-input and regression checks pass; qualified for focused 1.2.1 release, publication pending.
- Report: https://github.com/TK22-26/JetForceGemini-Recomp/issues/20
- Affected build: reporter's v1.2.0, Windows 11 25H2, Radeon 9070 XT,
  driver 26.9.2, Xbox/XInput modern controls. Alternate input modes reportedly
  fail as well once entering Ichor / Military Base / boss room, every attempt.

## Evidence and working hypotheses

The reporter's attached support ZIP is preserved unchanged with SHA-256 and
safe extraction inventory in private `issue20-ichor-controls-20261018`.
No attachment code is executed. Review the actual runtime identity, input
settings, scene state and health records before assigning a root cause.
Initial possibilities include boss-specific input ownership or a stale gameplay
lock after the room transition; neither is established yet. Compare the
released code against boss/cutscene context rules and preserve classic input.

The owner requests a focused 1.2.1 branch from the release branch, excluding
unfinished 1.3 performance changes. Existing production guard state and charges
remain in force; this priority change does not create a fresh investigation
allowance. No GitHub comment or closure has been posted.

## Validation required

Reproduce or establish the exact boss-mode failure from concrete state;
verify entry, movement, aiming, firing, pause, exit/death and control-mode
switching without allowing input during actual cutscenes. A build or isolated
fixture pass alone is not live boss-fight acceptance. Record final cause,
remedy and commit/release only once verified.


## Initial triage and owner reproduction

The support attachment confirms the exact official v1.2.0 runtime SHA-256
`72b2844ccaf8c5cc9a93142a7f4b42133b4a33c53ae30ecb31e527fdab40023e`.
It shows normal closure and continuing progress counters, without crash/hang
stacks. It contains no save or boss-state memory. The owner emphasizes that
all input modes reportedly fail: do not narrow acceptance to modern controls.

A separate muted capture of official v1.2.0 runs on the SSD with the completed
King Jeff profile copied again. Evidence stays on the data volume. Map warps
are owner-authorized for navigation; instant kill and infinite health begin
off. No autonomous builds or game tests run during the owner session.
Private receipt: `issue20-ichor-controls-20261018/owner-launch.json`.

The isolated hotfix baseline configuration rejected the reused 1.3 generated
root because its normalizer digest differs. That rejection is preserved; the
digest check must not be bypassed. Regenerate from the hotfix source or find an
already matching retained generated root before building. No runtime fix yet.

Source lead: original controlPlayer tests player-control +0xFC at 0x80032FD8
and dispatches boss controls when nonzero. The PC input predicate currently
treats any nonzero +0xFC as an input lock. This is a concrete inconsistency to
validate, not yet proof that it explains every reported mode or movement loss.


## Unbuilt candidate

The candidate separates playable boss actions from ordinary free-camera and
locomotion ownership. It recognizes the pinned native boss dispatcher, active
combat/jump states and a valid boss actor while retaining pause, cutscene,
damage/input locks and death exclusions. Modern action routing, raw mouse
capture, the temporary action table and shared aiming accept this context.
Boss movement maps configured left/right movement to the native C-button
strafe inputs; look stays on its separate channel. Original mappings are
unchanged, pending the owner's cross-mode reproduction.

Focused fixtures cover all three character IDs, movement versus reticle
separation, remapped keyboard controls, physical deadzone, lock/death/invalid
contexts and return to ordinary gameplay. These tests have been written but
not run while the owner is navigating. No live fix is claimed.

Source review also found that the ordinary-camera exclusion cleared fractional
look accumulators every controller poll. The candidate now retains those
accumulators during playable boss aiming while clearing free-camera ownership.
Pause, focus/device loss and actual non-playable contexts still discard look.
This is a candidate correction; boss sensitivity/scope and cross-mode behavior
remain part of the live qualification.


## Owner live boss reproduction (2026-10-18 03:40 UTC)

The owner reaches level 181 / generation 86 in the official v1.2.0 capture and
reports that strafing stops and the mouse stops working entirely, as though
original controls had taken over. Modern Controls remains enabled in the
preserved profile; this is not a saved-setting switch to Original.
The live exporter shows gameplay active, scripted camera false, global
joy_disabled zero, player_script zero, and motion state 0x20 (boss combat).
Instant Kill is off; infinite health remains on. This reproduces the modern
keyboard/mouse failure, not yet the reporter's complete cross-mode claim.

The owner also reports lower apparent resolution and lower FPS in the fight.
Screenshots show OUTPUT 47.9 / SCENES 59.9, then OUTPUT 59.8 / SCENES 59.8
while the renderer target remains 120. The compact update stream continues
in two-VI steps. SCENES counts graphics submissions rather than authoritative
simulation updates; multiple screen-sized passes can inflate it here. See
[the related rendering investigation](vela-rescue-cutscene-stutter-resolution.md).

Existing live files, the current save/profile, screenshot evidence and bounded
log tails are preserved under private owner-boss-failure-live-01 with hashes.
The full input/timing recording remains active, and detailed gameplay/actor
tracing capped earlier. Final RAM must be collected through normal game closure
before the isolated hotfix build and boss-state acceptance. The live exporter
does not expose player-control +0xFC, so that exact candidate predicate still
requires memory verification. No fixed runtime has been delivered yet.


## Closed capture and exact boss state

Normal closure preserved the 4 MiB final RDRAM and complete compact timing/input
files. The final state confirms a valid nonzero player-control +0xFC boss actor,
motion 0x20, all candidate character lock bytes zero, the global joypad lock
zero and Modern Controls still enabled. Resident boss-dispatch loads/branches match; the three overlay JAL targets
are relocated and differ from their original ROM encodings.
This validates the specific context rejected by the released modern input
predicate. It does not yet validate the candidate's execution or classic modes.
Private hashed receipt: closed-boss-state.json.

The preserved 19.992-second boss window has 599 compact updates, all 598
intervals exactly two VI, while output averages 45.869 FPS. The graphics-scene
counter reports 59.824 per second because 1,198 scene passes include 599
zero-world-vertex passes. All 919 displayed outputs in the renderer window
have source rate zero and sequence count one. The source counters describe
graphics submissions, not a simulation running at 60 Hz. Existing recording
streams have slightly different flush boundaries; these are bounded observed
windows, not invented frame-for-frame joins.


### Candidate correction from real captured memory

The original unbuilt predicate incorrectly pinned raw overlay JAL addresses.
The captured active Vela call instead targets 0x80361b30; inactive character
calls target the resident overlay-loader thunk. Pinning the static resident
argument loads, branch and delay slots while accepting relocated JAL operands
corrects that candidate error. Rejection tests cover corrupt opcodes and argument
loads, alongside both byte orders and captured relocation values. A private
captured-state entry point exercises the actual production predicates against
final RDRAM before native testing. Execution is still pending at this step.

The first final-state review attempted to read a launcher session pointer that
normal shutdown removes. It failed without changing the capture or runtime.
The recovery uses the already-hashed live boss snapshot and final memory;
both immutable job receipts remain available.

Final progress reports 10 cumulative audio underruns, whereas compact
game-update rows do not carry these audio counters (their trailing zeros are
not audio acceptance). Review final play-health and their timing separately;
do not claim the full long route was audio-clean from the earlier rescue prefix.


## Focused validation and native build preparation

The five focused CTest groups pass: navigation/warp eligibility, modern actions,
PC input, controller ports and classic mapping. The production predicates also
accept the owner's real final boss RAM while rejecting ordinary-camera ownership.
This validates the relocated context without relying only on fabricated fixtures.
Native input execution and live visual acceptance remain pending.

Modern scope polling now uses the same playable-action eligibility, so boss
aiming does not retain a separate menu-only rejection of configured scope inputs.
Free camera, shoulder movement and ordinary jump hooks keep their existing
normal-gameplay boundaries.

Fresh normalization first rejected the stock dependency header's legacy clang
extern-inline/weak function linkage. Reusing the retained original build's
compatible header passes the unchanged lineage checks. The fresh generated root
uses the hotfix normalizer and timing pass, with all input hashes recorded.
The failed attempt and incompatible header remain preserved; no digest or
lineage check was bypassed and no 1.3 generated root was reused.


The fresh full native build exceeded its initial 35-minute wrapper timeout after the generated game library compiled. The still-owned MSBuild child was allowed to finish under the same guard; an incremental build then verified completion. Existing objects, failure receipt and logs were retained. This is a build-time limit, not a gameplay or compiler regression.


## Native route validation (2026-10-18)

The isolated release-based runtime builds successfully. Five input/navigation
test groups and the real captured-RAM eligibility check pass. The separate
unique-scene counter regression check also passes. The copied save boots with
Vela at level 43; source save SHA-256 remains unchanged. The first bootstrap
request ran before the timed-out build wrapper's completion receipt existed
and exited without launching a game; its recovery completed after the verified
incremental build. This bootstrap uses bounded synthetic probe timing and is
not a performance acceptance measurement.

The first automated route reaches the boss approach (level 19) through real
native room transitions, then selects an inactive exit overlapping the active
one. The native progression gate correctly rejects it, and the bounded test
stops after its request limit. This is a navigation-fixture failure, not an
observed failure of the input candidate. The next fixture filters the live
progression metadata for an active exit; it does not bypass or modify gates.
Transient loading IDs and automatically traversed cinematic rooms are preserved
in the evidence but excluded from interactive navigation targets.

Private evidence: candidate-checkpoint-bootstrap-02, candidate-boss-route-01,
candidate-boss-route-02, candidate-boss-route-summary.json. Physical mouse/pad
behavior and visual compositing still require their own validation.


## Effective boss input qualification

Both bounded native routes now reach playable level 181. The mouse-look packet
path records 971 boss aim calls with 62 distinct endpoints; the controller-look
path records 985 calls with 214 endpoints. Both move Vela more than 538 world
units horizontally, enter the native jump state 0x23, and retain boss state
0x20. Ten native exit transitions are accepted in each route. The input fixture
injects effective packets; physical mouse/Xbox binding and visual owner
acceptance remain pending. It is not a gameplay performance benchmark, and its
audio counters do not establish release audio quality.

The shorter active-exit fixture ended normally inside the lengthy scripted
level-230 introduction; extending its bounded duration reached the fight
without skipping or changing game progression. An auxiliary introduction-input
review referenced the wrong timing filename and failed before changing any
state. The successful full route makes that auxiliary check unnecessary.
All failed and successful receipts remain preserved.

Private evidence: candidate-boss-route-03, candidate-boss-pad-01,
candidate-boss-mouse-summary.json, candidate-boss-pad-summary.json.
The qualified input executable is retained separately before rendering
experiments. No visual rendering resolution is claimed from these checks.


## Focused preview qualification

Original-mode effective N64 packets also reach boss movement and jump. The
modern aim hooks remain unused and both original Normal/Expert action tables
remain byte-for-byte intact. This does not reproduce the reporter's entire
all-device claim, and does not replace physical controller/mouse acceptance.
Private evidence: candidate-boss-classic-01 and candidate-boss-classic-summary.json.

Final cause of the reproduced modern-input defect: the host interpreted the
native boss-control actor pointer as an input lock. It consequently selected
menu action mapping and released mouse capture/modern aiming during playable
combat. The candidate recognizes native boss combat/jump separately, maps
movement to boss strafe actions, retains separate aiming, and leaves native
fixed-camera and true pause/cutscene/death ownership intact.

The local 1.2.1-beta.1 preview contains this correction, native exit-contact
warping, and the distinct-source scene counter. It excludes all unfinished 1.3
performance work and the failed blur experiment. Owner verification must cover
physical strafing, aiming, firing, jumping, pause/resume and death/re-entry before
stable release or closing GitHub #20. No GitHub message has been posted.


## Local preview delivery

The 1.2.1-beta.1 package is built from clean hotfix source
`8485b04938ceb57058bc5bbb6add83eb702d7aa0`. Native and launcher/setup tests pass, package inventory
and checksums verify, and the exact packaged SSD runtime boots the copied Vela
save successfully. Runtime SHA-256: `fac87d3be47a55e2be1c64068cd4a57ef95f319d7997f379fa6383a3495c45f4`.
The original owner profile/save and official 1.2.0 package are unchanged.

Desktop shortcut: **JFG - Ichor Hotfix Preview**. It uses an isolated copied
profile, starts muted, and records on Play. Runtime/ROM are on the SSD; map,
frame and diagnostic evidence are on D:. This is a physical input/fight review
candidate. No release/tag, push, merge or GitHub issue closure is performed.
Blur rendering and the long capture's audio events remain open.

Private receipt: hotfix-preview-delivery.json; packaged-hotfix-bootstrap-01.


## 2026-10-18 owner check and direct boss fixtures

The owner reports that controls now work in the 1.2.1-beta.1 Ichor preview.
This is a scoped Vela/Ichor confirmation, not acceptance of rendering or
performance. Those issues still reproduce and remain 1.3 work.

The owner requests direct Juno and Lupus mantis-twin checkpoints, then accepts
moving forward if all three controls checks pass. Mizar 1 and Mizar 2 become
additional controls checks only if Juno or Lupus fails. Fixture preparation
failures are not evidence that a boss-controls fix failed.

Private evidence: issue20-ichor-controls-20261018/juno-boss-warp. The exact
packaged preview runtime is unchanged (SHA-256
fac87d3be47a55e2be1c64068cd4a57ef95f319d7997f379fa6383a3495c45f4).
Copies of slot 0 preserve the owner source save and use normal entrance
transitions. Juno entrance 194 transitions through introduction 193 to active
boss 195, character 1, combat state 32 and a nonzero boss actor (bootstrap-04).
This verifies fight initialization, not physical input acceptance.

Informative fixture attempts: retaining all characters selected Vela instead
of Juno; separate test saves now select only their target character. A direct
fight checkpoint is normalized by native resume logic to an entrance.
Lupus 351 is not the pre-fight entrance: its selected exit returns to 249.
Direct 352 probes remained in the introduction within the initial observation
window. These observations invalidate the corresponding shortcut candidates;
no failed fixture is labelled a gameplay regression. The corrected entrance
probe uses 249 and retains its observed outcome separately below.

Lessons: derive room names and entry transitions from concrete game state;
a boss-like room name and a patched save alone do not establish an active
fight. Keep checkpoint generation private, preserve the original save, and
verify nonzero native boss state before claiming fixture acceptance.

Latest Lupus bounded fixture result: level 352, character None, boss actor None. See private lupus-bootstrap-05-summary.json; physical controls remain pending.

Local shortcut delivery: Juno, Lupus. Separate disposable saves, muted audio, retained input settings, diagnostics on the private evidence drive; resets test checkpoint on launch. Original owner save SHA-256 remains 4bb245c34fbc5c1468f21af84c1651a5b7173ec6b426eab1cdf76a90da55a497.

Lupus native fixture verified: lupus-bootstrap-06, level 210, character 2, nonzero boss actor. Seed lupus-boss-intro-seen.flash. Physical owner verification remains pending.


The delivered Lupus fixture sets only its private intro-seen flag (22) in
addition to the selected-character/checkpoint changes. Its native entrance 249,
condition 24, reaches boss 210 with character 2 and combat state 32. The normal
introduction remained active through a 169-second probe and is not qualified
by this fixture; skipping it is test setup, not a runtime fix. Juno's shortcut
keeps the verified normal introduction. Both delivered profiles have health
and instant-kill mods off and use the unchanged hotfix executable. The failed
first launcher setup referenced a setup executable absent from the packaged
runtime; resuming with the existing built frontend setup tool succeeded and
preserved the original profile.


## Stable hotfix acceptance (2026-10-18)

The owner reports that both direct Juno and Lupus fights work perfectly,
following the earlier Vela/Ichor confirmation, and requests publication of 1.2.1.
This is the requested three-fight physical controls acceptance. No Mizar tests
were triggered. It does not imply a new all-device matrix, unassisted campaign,
Lupus-introduction check or performance acceptance.

The final release changes only launcher version and documentation relative to
the owner-tested runtime source 8485b04938ceb57058bc5bbb6add83eb702d7aa0.
Retain that runtime's original clean support identity and hash when packaging;
do not relabel its source revision. Remaining visual/audio issues are tracked in
https://github.com/TK22-26/JetForceGemini-Recomp/issues/21, together with the ongoing general 1.3 performance work.
Publication and final source/release identities will be recorded after release.
