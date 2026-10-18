# Modern controls: area entry, jumping and focus

- Reported: 2026-10-13.
- IDs: CAMERA-003, MOVE-003, CAMERA-004.
- Affected: local v1.1 preview with modern controls and the owner-verified native audio repair.
- Severity: high gameplay regression for entry/jumping; medium camera regression for focus.
- Status: reproduced/source-qualified, corrected and verified in focused tests; updated local preview delivered. Owner live verification remains required. No public release or commit is claimed.

## Reports and expected behavior

| ID | Reproduction / observed behavior | Required behavior |
| --- | --- | --- |
| CAMERA-003 | Select XInput dual sticks, leave Goldwood red-key area. New area starts facing the exit. Owner observes this at many transitions and confirms that the original N64 game walks in facing forward. | Native area-entry facing is retained when modern view ownership begins. |
| MOVE-003 | Jump with dual sticks while moving in a direction. Jump goes forward, loses directional momentum, and cannot steer in the air. | Preserve the full takeoff movement vector and the original airborne steering, collision and vertical physics. |
| CAMERA-004 | Click outside the game so it loses focus. Camera resets to a lower angle. | Suppress/release physical input while preserving the gameplay view through focus loss and recovery. |

The owner separately confirmed **"Audio is fixed."** on this date. The audio repair
remains installed; see [AUDIO-001](intermittent-music-crackle.md).

## Actual causes and changes

### CAMERA-003: camera pose was used as player-facing state

The previous shoulder adapter initialized its heading from the current camera.
During entrance setup that camera can still face the doorway while the original
player control already contains the forward entry heading. Taking ownership at
that moment gives the modern player a persistent 180-degree error. The old view
also relied on actor identity and a 15-retrace gap to infer a new lifetime; those
are insufficient when a level transition recycles actor/control storage.

The new adapter initializes from the native player-control facing, tracks both
actor and control identity, and explicitly invalidates the view at
`mainChangeLevel`. Existing player input locks, scripted-camera checks and native
walk-in handling remain authoritative. There is no hardcoded doorway angle,
position warp or transition delay.

The crucial regression fixture keeps modern input enabled **throughout loading**.
On a copy of the owner's current save, the old preview loads Goldwood with body
heading `0x8000`; the candidate and Controller Mapping reference use `0x0000`,
with identical final player positions. This reproduces and corrects the 180-degree
entry error. Earlier fixtures enabled modern input only after loading, after the
native camera had settled, and therefore missed the defect.

The exact red-key exit from the owner's report has not been replayed. The
continuous-input Goldwood entry establishes the faulty initialization and its
correction; broader doorway/entrance coverage still needs live verification.

### MOVE-003: one heading was serving incompatible purposes

Modern ground movement has separate forward and strafe components. The original
air handler represents horizontal momentum using a signed speed and a heading.
Previously, the modern adapter kept forcing that airborne heading back to the
view heading. This discarded sideways takeoff direction and overwrote the native
air handler's steering result.

At takeoff, `pc_native_airborne_motion` converts the complete ground vector into
the original airborne representation without changing its world-space direction
or magnitude. `pc_velocity_in_view` first accounts for a camera-basis change,
which prevents a turn at takeoff from rotating existing momentum accidentally.
While airborne, the original handler retains its heading, acceleration and
steering. The modern adapter restores visual body facing after original physics
has consumed the movement heading, and only while modern gameplay still owns the
actor. Collision, jump charging, gravity, landing and ledge handling remain in the
original routines; no jump-speed multiplier or replacement gravity was added.

### CAMERA-004: input suspension destroyed persistent view state

Focus blocking previously cleared both the shoulder and orbit, disabled the
modern camera adapter and later reacquired the native camera's lower angle.
The fix separates transient input from persistent view state: discard pending
mouse/stick deltas, scope filters, buttons and movement, but retain the view and
its modern mode while input is suspended. The normal collision-aware camera can
continue using that view. The existing physical release gate still prevents
held controls from leaking through on return.

Pausing, scripted control, actor replacement, area changes and disabling modern
controls retain their explicit ownership rules. Focus loss is not treated as a
new player or area.

## Implementation

- [Modern view, movement conversion and identity](../../../include/jfg/runtime/pc_input.hpp).
- [Player/physics scopes, area lifetime and input suspension](../../../src/boot/native_boot.cpp).
- [Coordinate and view-lifetime regression tests](../../../tests/pc_input_tests.cpp).

A bounded diagnostic suspension interval can exercise the actual input-blocking
branch in noninteractive replays. It requires a replay and rejects interactive
Play; tests did not take desktop focus or inject global input.

## Attempts and evidence

Private evidence is retained under `controls-regressions-evidence-20261013`;
ROM bytes, generated game code and personal saves are not published.

| Attempt | Result / interpretation |
| --- | --- |
| Baseline backward/side jump replay | Reproduced lost sideways motion and unchanging airborne heading despite steering input. |
| Vector conversion and native-air ownership candidate | Backward and sideways momentum retained; native steering responds; body remains aligned with the view. |
| First source-edit helper | Ambiguous text anchor matched twice; assertion stopped it before any source write. Corrected helper uses an exact struct declaration. |
| Intro-area exit route | Stopped at the guide. Repeated confirm presses while stationary could reopen dialogue. These runs do not qualify a transition. |
| Continue walking through dialogue | Exercises native/script handoff, but different camera policies diverge the paths; not used as a same-position area-fix proof. |
| Natural Goldwood-to-ship exit, both timing paths | Candidate matches original mapping's final position and facing. Old preview also passes this route: regression coverage, not reproduction. |
| Delayed modern activation on owner's copied save | Both old and new previews face correctly after the camera has settled; this concealed the startup/entry defect. |
| Continuously enabled modern input on owner's copied save | Old preview faces 180 degrees backwards; candidate matches original mapping at the same position. Qualified reproduction and fix. |
| First Vela steering fixture | Landed before the scheduled turn, so the steering assertion correctly failed. A new fixture turns earlier while Vela is airborne; the same assertion passes. No verifier threshold or runtime behavior was weakened. |
| Character-switch fixture labels | Some private case names reflect intended rather than selected characters. Coverage is established from actual guest IDs: Vela 0, Juno 1, Lupus 2. |

Key receipts: `jump-candidate-comparison.json`,
`focus-and-jump-verification.json`, `expanded-controls-verification-v2.json`,
`entry-facing-verification.json`, `controls-regressions-qualification.json` and
`controls-state-preview-activation.json`. The failed first expanded verification
is retained separately.

## Validation and limits

- Relevant PC input, modern controls, native audio source and audio DMA unit suites pass.
- All three characters retain their takeoff vectors; the largest measured horizontal-vector difference is approximately `4.9e-8` units from floating-point conversion.
- Backward/sideways takeoff and mid-air steering are exercised for all three characters. Additional Juno cases cover forward, diagonal and stationary takeoff, including camera turns, on reference and native timing. A stationary jump begins with zero horizontal momentum.
- The original Controller Mapping jump replay has byte-identical final RDRAM before and after the implementation.
- Mouse and controller suspension tests pass from ordinary view and from aiming. Each retains 77 recorded view samples across the suspended/recovery interval. Injected movement/jump/look is ignored; fresh look works after release. These are tests of the runtime suspension branch, not physical Win32 focus transfer.
- The final candidate passes a hidden, owned-window normal Play smoke. Native timing and the repaired ROM audio sample provider activate without private enable flags; shutdown is clean and the source save is unchanged.
- Copied saves preserve their source hashes. Preview activation verifies that the user's ROM, saves, mappings and unrelated profile files are unchanged.

The Controller Mapping reference uses the original guest control/physics path in
the same runtime. This investigation did not run a new independent emulator or
physical N64 comparison. Physical XInput feel, mouse recapture, the exact red-key
exit, other entrances and interaction with unusual airborne states remain live
checks. The unrelated planted-foot pivot and collision-camera research remain
separate issues.

## Delivery and live check

The existing **JFG Modern Controls Preview** desktop shortcut now uses the local
`modern-controls-state-fixes-20261013` package. Its v1.1 launcher is unchanged;
the runtime was rebuilt, its package manifest refreshed and the existing profile
preserved. Runtime SHA-256:

`b2a9a6c6d3ccd706482e688f2bb784c2a548e171daad6ad87fb7815d7e23bb0e`

Verify with XInput Modern Controls:

1. Leave the same red-key area and several other entrances; Juno should finish entering facing forward.
2. Jump forward, backward, sideways and diagonally; steer while airborne, including while turning the camera.
3. Set a raised/lowered view, click outside and return. Repeat while aiming. The view should not drop, and held inputs should require release before resuming.

## Lessons and future refresh-rate work

Preserve semantic state across input suspension, and invalidate it on explicit
world/actor lifetime boundaries. Player entry facing must not be inferred from a
transient presentation camera. Test the real device-mode lifetime: enabling an
adapter only after a scene is ready can hide a takeover bug.

Keep motion and visual orientation separate where the original game couples
them. Convert coordinate representations without changing the world vector, and
leave native elapsed-time physics in charge. These changes add no 30-fps movement
constant or presentation-frame integration; future high-refresh rendering must
continue to run this adaptation once per native simulation update and must not
apply input or physics again for interpolated frames.

The existing production guard remained enabled throughout the owner-approved
two-hour continuation. Investigation identity, prior charges and failed evidence
were retained. Completion of this local work does not authorize additional work
under an exhausted guard or a public release.

## Owner live verification, 2026-10-13

The owner confirmed that all three delivered corrections (area-entry facing,
directional jumping and air steering, and focus-loss camera orientation) are
validated in live play, and authorized committing them. This validates the
local preview; it does not claim a public release. The separately reported
terrain-driven camera bounce remains under investigation in
[camera collision bounce](camera-collision-bounce.md).

The three owner-validated fixes were committed as 7b1ea4ba6228c126757b2b2e966ed7c0e1de8ca4 using the maintainer identity. No public release or push was performed. The subsequent camera-collision candidate retains those fixes; its separate issue record tracks new validation and delivery.
