# PC controls

> **Critical, open: Goldwood combat slowdown (2026-10-11).** The owner capture shows approximately 15 game updates per second in the busiest drone fight, with 85 to 98 ms update gaps. This is separate from the confirmed camera-distance stepping defect. Owner approved a further two-hour investigation and fix on 2026-10-11.

Modern Controls provides experimental dual-stick controller and mouse/keyboard
gameplay for player 1 in the supported US single-player game. It remains off by
default. Original Controller Mapping is available alongside it. Final physical
controller, mouse focus and gameplay-feel checks remain reserved for the owner.
Automated checks use copied-save replays and offscreen RmlUi windows.

## Using the controls

Open the launcher's Controllers window. Choose a player and use Add device or
Change device to assign a connected controller or Mouse and keyboard. Only
connected controllers available to that player are offered. An unplugged
assignment retains its bindings; Remove disconnects the player without erasing
them.

Choose one style:

- **Controller Mapping:** bind physical inputs to the original N64 controls.
  Normal/Expert presets must match the game's own Control Mode; Normal jumps
  with N64 A and Expert with C up. The preset does not change the saved game.
- **Modern Controls (experimental):** bind gameplay actions designed for Xbox,
  DualSense or mouse and keyboard. Selecting the style enables it immediately.
  There is no additional experimental gameplay toggle.

Switching styles preserves both layouts. Original keyboard/mouse rebinding is
always available. In modern mode, A/Cross jumps and X/Square interacts regardless
of the game's Normal/Expert option.

| Modern action | Keyboard and mouse | Xbox / DualSense |
| --- | --- | --- |
| Move / strafe | WASD; Shift for full movement | Left stick |
| Look | Mouse | Right stick |
| Aim / fire | Right mouse / left mouse | LT / RT; L2 / R2 |
| Jump | Space | A / Cross |
| Crouch | Ctrl | B / Circle |
| Interact | E | X / Square |
| Previous / next weapon | Wheel down / up | LB / RB; L1 / R1 |
| Cycle sniper magnification | Middle mouse while scoped | Right-stick click while scoped |
| Pause | Tab | Menu / Options |
| Menu select / back | E / Q; right-click also goes back | A / B; Cross / Circle |

Increase and Decrease magnification are separate optional actions. Click an input
to assign them or another action. Modern Jump/Crouch do not change sniper zoom.
Jump can start while Aim is held or toggled. Aiming, the reticle and sniper
magnification remain active through takeoff, air time and landing. Release Aim
to leave the aiming camera in midair. Native jump motion and air steering remain active.
Leave the aim stance before crouching.

Modern ground movement uses a view-locked third-person camera: turning the mouse
or look stick turns the character, forward follows that heading, and left/right
movement strafes. Entering and leaving aim retains the view direction. Mouse
vertical inversion defaults to On for new modern profiles and reset defaults;
an explicitly saved choice is retained.

Buttons (Keys & buttons), Sensitivity & behavior, and controller Advanced settings
are top tabs. The original mapping page has padded cards, physical-input glyphs
and Normal/Expert presets. Both modes retain capture and clear controls. Release
held controls before capture; Escape cancels. Focus/device loss cancels capture.
Five mouse buttons, both wheel directions, keys, gamepad buttons, triggers and
stick directions can be assigned. Escape and F11 retain launcher shortcuts.

## Mouse navigation in the native menus

With Player 1 assigned to Mouse and keyboard and Modern Controls selected:

- Click inside the title screen to reveal the start menu. Hover Play, available
  Multi, or Options to select it; left-click to open it.
- On Select Game, hover a numbered save slot to preview it. Click a slot or the
  selected game card to open that save's options. Hover selects the target
  directly, without stepping through intermediate numbered slots.
- On Select Option, click the Play checkmark or the selected Play Game card to
  start the saved game. Right-click sends the game's Back input on supported pages.

Targets follow the displayed game image when the window is resized; black bars
are excluded. Transitions, focus loss and the controls window cancel pending
pointer actions. The first click returning from another application activates the game without
also issuing an input. Clicking within the already active launcher keeps the
click when focus transfers from the launcher to its game viewport.

Pause-menu icons and the pause Options icons support direct pointer selection
and activation. A changed pause/Options selection plays the native selection
sound. Character selection has clickable left/right arrows; click its central
label or press E to accept and return.

WASD (or rebound movement keys) also navigates menus. A/D adjusts native settings
such as music volume. Modern Controls uses movement keys for navigation and removes arrow bindings. E selects at a
menu root and closes informational pages such as Health, Weapons, Inventory,
Tribals and Ship Parts. E also finishes Character selection and ordinary Options
adjustment. Q and right-click are Back, including on Map; E on Map retains the
game's travel confirmation. Held E cannot immediately close the page it opened.

This remains **partial native-menu pointer support**. Nested Options settings
use directional keys; save copy/erase, new-game naming, Options opened from the
title/start flow, and dialogue are not a complete mouse interface. Explicitly
rebound menu keys remain supported. The launcher and controller editor use RmlUi.

## Settings and compatibility

- Modern camera, aiming and scoped sensitivity are independent. Both devices
  have X/Y inversion and vertical scaling. Gamepads also have movement/look
  deadzones and Linear, Quadratic or Cubic response curves.
- Mouse input is relative displacement; precise stick rates integrate guest
  update time. Fractional motion is retained without queuing delayed turns.
- Capture releases on focus loss, settings, guest pause and scripted cameras.
  Reconnection/reassignment requires neutral controls.
- Modern settings live in modern-controls.ini; enabled selects the style.
  Player 1 single-player is the accepted gameplay scope. Missing profiles use
  original mappings. Original controller.ini and pc-input.ini bindings are
  independent and remain intact when switching styles.
- experimental-controls.ini is obsolete and ignored by the live runtime.
  Old mouse-look/dual-stick flags in pc-input.ini do not activate modern paths
  while Controller Mapping is selected. Existing files remain for rollback.
- Recordings begun in Modern Controls use v8 with independent look/actions and
  semantic native-menu selection events, including pause Options. The reader
  retains v1-v7 compatibility.
  Recordings begun in original mode retain their older format; switching mode
  mid-recording does not consume look input that cannot be recorded.
- XInput rumble is supported. Native SDL/DualSense rumble, gyro and adaptive
  triggers remain unavailable. Physical device feel still needs live testing.

See the [native RmlUi implementation](modern-controls-ui.md) for layout and
capabilities. The dated evidence below records earlier implementation stages;
their separate experimental switch was superseded by the current two-mode
selection.

## Implementation and camera integration

`include/jfg/runtime/pc_input.hpp` provides mapping, profile parsing, independent
look channels, rate integration and camera math. `window_input_mailbox.hpp`
transfers keys and focus/motion as one snapshot between window and game threads.
Sampling remains at the existing controller/SI boundary. The game retains
weapon handling, collision and special-state movement.

- Manual aim hooks the qualified US routine at `0x8003AABC`, updates yaw/pitch
  within its limits, and restores original raw movement afterward.
- Ground aim states 10/11 receive normalized forward/side movement through the
  existing physics path. Special states retain original guest behavior.
- Modern ground view/facing is coordinated around the original player update
  and the three character physics routines. Normal movement receives normalized
  forward/strafe velocity before the original collision step. Jump/flight momentum,
  swimming, ledge, damage, scripted and other special states retain their native
  movement handlers.
- The normal camera path at `0x8002CF6C` still supplies its desired position
  before native scenery constraints. Modern shoulder view uses that resolved
  position immediately, removing follow-camera drift. Scenery changes that position
  without rotating away from the shared input heading. The ground-aim camera
  retains the same yaw/pitch convention, with pitch limited to the supported
  aiming range. Legacy replay and original-mapping camera behavior is retained.
- Wall contacts retain a small inward clearance. Three lower-body visibility
  rays, spanning the player's width, additionally catch edges missed by the
  head-height camera ray. Possible contacts use the game's own
  `trackNearestIntersection` (`0x80018808`) and re-enter its camera constraint
  helper. This resolves the captured steep tree-edge obstruction.
- In tight spaces, the modern camera smoothly fades the player to a faint
  silhouette instead of leaving an opaque body across the view. The qualified
  opacity routine at `0x80015E3C` runs the original game first, then reduces its
  returned opacity according to distance from the camera to the player's body.
  Stronger original aim/death fades are preserved. The hook is restricted to
  active single-player modern camera control; scenery collision and movement
  are unchanged. Disabling experimental controls disables this fade.
- `pc_camera_visibility.hpp` conservatively filters clear rays against current
  static triangles and registered moving-model bounds. Unknown or invalid
  geometry falls back to the guest query. It avoids unnecessary guest collision
  calls in unobstructed views; it does not replace the game's collision result.

## Earlier automated qualification (2026-10-10 UTC)

The preceding delivered source passed **168 PC input checks**, controller mapping/port/SI,
recording-format and window-mailbox checks, **564 launcher checks** and **15
setup checks**. The actual D3D11/RmlUi editor passed device filtering, both
corrected presets, capture/cancel, opt-out rebinding, persistence, hotplug,
player isolation, advanced settings and minimum-window layout checks. Its
separate native window opens, resizes and closes correctly.

**43/43 native acceptance checks passed on the preceding runtime build**
(`93748c952792...`). They cover mouse and stick aim, simultaneous
forward/back/strafe movement, firing through the guest weapon path, jumping in
both Normal and Expert, all three loaded characters, camera orbit/neutral/pitch,
pause/resume, input replay, experiments-off equivalence and game-state transitions.
Record/replay matches the complete player, aim, orbit and clearance timelines.
Disabling experiments matches ordinary player/camera state exactly.

The steep tree-edge capture now keeps the player visible. The unobstructed
camera matches its baseline within 0.001 world units; guest update counts were
2831 versus 2846 in that finite run (0.995 ratio). This is a focused regression
comparison, not a general frame-rate benchmark.

Proximity fading passed **37/37 camera regression checks** on the intermediate
runtime (`1f70dcd57971...`). The checks show that the close-camera fixture fades, the settled distant view is
pixel-identical to its prior capture, and experiments-off behavior is unchanged.
Complete actor timelines match the preceding build except for the intended
render opacity byte; aim, orbit, clearance and body-ray timelines match exactly.
The close-camera recording also reproduces its fade and gameplay timelines.
The final runtime also fixes held-aim blocking look while the guest retains its
normal airborne camera. **45/45 additional checks**
passed on that final binary, including both look devices during Lupus flight,
exact record/replay, and unchanged close/clear-view regression timelines.
These targeted checks extend the earlier acceptance; the full 43-check sweep
was not rerun on the final binary.

The additional special-action qualification passed **38/38 checks**.
Lupus's powered flight lasts longer than jump-only controls with both mouse and stick
camera input and returns to the ground: 336 versus 66 VI in the stick case and 331 versus
61 VI in the mouse case. This fixture did not enter manual aim in flight. Its selected
weapon was empty, so its zero shot count does not qualify airborne firing; the later
weapon-selection report records that follow-up. The weapon-cycle comparison records new
shots for 9 weapon types: Pistol, Homing missiles, Machine gun, Plasma shotgun, Tri-rocket
launcher, Sniper rifle, Grenades, Timed mines, Remote mines. It verifies firing through
guest weapon logic, not every weapon-specific effect or target interaction.

The follow-up changed weapons before each Lupus flight and recorded 26 new guest shots,
with both fire-input windows observed in airborne state 3. This qualifies this fixture's
airborne weapon use; it does not add manual airborne aiming.

These are finite native replays through the game's original action logic, not
physical-device or whole-campaign certification.

The rebuilt launcher passed its runtime protocol/ROM checks, hosted this exact
runtime in its game viewport, reached rendered gameplay and closed cleanly.
Current source/executable hashes and results are retained in `flight-build.json`,
`flight-producers.json`, `camera-fade-acceptance-v2.json`,
`special-actions-acceptance.json`, `flight-look-acceptance.json`,
`airborne-weapon-observation.json`,
`paired-launcher-flight-smoke/result.json` and
`preview-delivery.json` under the investigation's private `completion-evidence`
directory. The earlier core evidence remains in `native-acceptance-core.json`.

- Runtime SHA-256: `c1f1df156963b8580e5dee46ba53541e13c2b564517acdb7f3387027903af598`
- Launcher SHA-256: `1a9f7b5bcf394246b8a176089f642117af3640f993d072fdd27b5731c610f8f3`

Final focused XInput and mouse/keyboard checks are pending. Earlier candidate
input-path checks passed, but another application took foreground focus during
the final rechecks, and Windows rejected raw mouse injection. Interrupted runs
are retained as failures and are not included in the 43 passing core checks.
The owner explicitly chose to perform these desktop focus and physical-device
checks during live verification. Do not run them automatically during this continuation.


The retained evidence includes failed camera trials, unfocused input harnesses,
an overlapping-event hover fixture, a final-screenshot capture failure and
incorrectly prepared character fixtures. A later successful flight comparison
uses a fresh fixture directory and a later capture endpoint. Failed and invalid
runs remain excluded from acceptance. Final character coverage uses actual in-game character
selection and verifies three distinct loaded IDs/levels. Default save files
were not used as writable test fixtures.

Device integration uses a private synthetic XInput device through the real
native backend, Windows keyboard/button/wheel messages, and `SendInput` raw
mouse movement. The private device DLL is excluded from delivery. These checks
establish the input path and its context handling; a physical controller,
actual Alt-Tab interaction and subjective aim feel require owner verification.

## Continuation builds (2026-10-10 UTC)

These changes are included in the rebuilt preview below. Earlier intermediate
builds and their results remain recorded separately. Owner saves, bindings and
experimental preferences are preserved; reapply Normal or Expert to receive
the corrected keyboard action defaults.

- **Rumble:** qualified US guest requests route to each assigned XInput device.
  The controller-only RmlUi setting is **Rumble strength (0 = off)**. Profiles
  migrate to v5 with rumble disabled. The output worker uses a 250 ms real-time
  lease, independently checks focus/settings, stops touched motors on exit and
  bounds sustained output. No accessory or save protocol is replaced. All
  replays suppress physical motor output. **36 component checks**, the offscreen
  RmlUi checks and **14 native comparison checks** passed;
  actor/camera/aim/request traces match the observation build. Evidence:
  `rumble2-build.json`, `rumble2-native-acceptance.json`, and
  `rumble-ui/mapping-window-ui-result.json`.
- **Swimming camera:** holding aim previously stopped both look paths in state 4.
  The normal camera now retains look during that observed swimming state, as it
  already does during airborne state 3. Guest diving, surfacing, movement and
  unsupported manual aiming remain unchanged. **39 checks** pass
  for both look sources, including exact recorded gameplay/camera playback.
  Evidence: `water-fixture-provenance.json`, `swim-build.json`, and
  `swimming-acceptance-v2.json`. The initial grader incorrectly required two
  different routes to reach the same deep floor; that failed result is retained.
- **Additional gamepads:** an SDL 2.26.3 backend now supplies recognized
  non-XInput controllers to the same mapping and modern-look paths. Xbox stays
  on XInput and is filtered before SDL opens it. Profiles store a hashed
  GUID/device-path identity (serial fallback), not an enumeration index. The
  picker excludes offline or assigned devices and reserves saved identities
  before automatic assignment. Identical or missing identities fail closed;
  a controller without a stable ID is not offered for assignment between the
  launcher and game processes. Try USB for such a device. Changing USB ports
  or Bluetooth transport can require selecting the device again.
  Controller profiles use v2 only for SDL; existing XInput/keyboard v1 profiles
  retain their encoding and custom mappings. There are four player ports and
  up to sixteen additional SDL identities per process. Gyro and SDL rumble
  remain unimplemented, and their controls are not shown.
  **32 catalog/profile checks, 21 isolated SDL virtual-device checks, actual
  offscreen RmlUi device/mapping checks, 572 launcher checks and 15 setup checks
  pass.** The virtual tests cover both sticks, normalized triggers, detach and
  replacement identity. RmlUi checks cover mixed devices, ownership, restart
  loading, presets/reset, Guide capture, hotplug, escaped names and the separate
  mapping window's Unicode runtime path. Physical Sony/Nintendo hardware has
  not been certified. Evidence: `sdl-ui-final/mapping-window-ui-result.json`,
  `sdl-final-build.json`, and guarded device-phase command receipts.
- **Keyboard action presets:** wheel-up/down now map to the game's actual
  next/previous weapon actions for each scheme; Ctrl crouches and E provides the
  original A action. Normal uses C-up/C-down for weapon cycling and B for crouch;
  Expert uses B/A and C-down. Reapply the desired preset to receive these changes.
  Existing custom profiles are preserved. **182 PC input checks** pass after
  these changes. `guest-button-table-audit.json` and the generated guest control
  tables provide the source evidence. **108 native checks** qualify selecting
  and consuming ammunition with all 15 weapons in Normal and Expert using
  both look sources. Charged plasma, remote-mine firing and empty-weapon
  automatic selection retain guest behavior. This does not establish every
  weapon effect against targets. Evidence: `weapons-acceptance.json`,
  `weapon-fixture-provenance.json`, and `actions-build.json`.

The continuation evidence is retained privately in `extension-evidence-20261010`
under the same guarded `pc-input-aiming-20261008` investigation. Physical Xbox
rumble, focus/reconnect behavior and subjective feel remain owner verification.
No foreground input injection was used in this continuation.

## Current preview qualification (2026-10-10 UTC)

The rebuilt pair includes the continuation changes above and magnification-aware
sniper aim. The guest retains its 1-5 zoom controls; direct mouse/right-stick
motion scales by the reciprocal zoom with fractional accumulation. At maximum
zoom the qualified mouse and stick fixtures turn at one fifth of the unscaled
rate, retaining sub-angle motion between updates. Other weapons and ordinary
aim retain their existing behavior. The RmlUi mapping
window explains the jump/crouch zoom actions; wheel input still cycles weapons.

- **193 PC input and 36 rumble checks** pass. A clean public CMake build passed
  all **7 selected CTest targets**, including replay compatibility, stick/port
  mapping, the **32 catalog** checks and **21 SDL virtual-device** checks.
  The pinned SDK download fallback matches the locally used headers and DLL.
- **572 launcher and 15 setup checks** pass. The actual offscreen RmlUi editor
  passes with a waited-for clean process exit. A prior wrapper read its result
  before the GUI process ended; that failed wrapper receipt remains retained.
- **36 sniper/water checks** pass on this runtime. Both look
  paths enter deep water, dive, surface, retain held-aim look, and return to
  ground using ordinary controls. Earlier routes that stayed on land or stopped
  against the bank remain excluded from exit acceptance. One later route
  returned to land but continued into the original Area Cleared screen; the
  final route stops on land and verifies continued neutral gameplay. A further
  **30 checks** reproduce both complete water-exit routes exactly
  from their recorded inputs. Steering clear of the bank edge fixes the route;
  no swimming or collision code was changed to make it exit.
- **125 current native regression checks** pass: unchanged
  close-camera, Lupus-flight and swimming traces; mouse/stick aim and ground
  strafing; experiments-off equivalence; and exact recorded playback for both
  scoped-look paths, both swimming/shoreline paths and the combined movement/aim case.
  The first angle-comparison grader incorrectly crossed a device-mode/pause
  transition; its failed receipt is retained. The corrected check compares
  consecutive updates within each continuous look burst.
- **21 additional weapon checks** complete the Normal/right-stick
  combination on this runtime. The preceding **108 checks** cover Expert with
  mouse and stick, and Normal with mouse. All 15 weapons are selected and use
  ammunition across that matrix; every target interaction is not certified.
- The launcher reaches rendered gameplay with the exact runtime and closes
  cleanly on a separate, never-displayed Windows desktop. This validates the
  launcher/runtime pairing without exercising the owner's desktop focus tests.

Current runtime SHA-256: `439cb0bc5d627c4a4dc2ea0896fb58c579ef2d65c1276eaebd0caf5fcc320e9f`

Current launcher SHA-256: `9bf89f1a01909b3db8cd1158dcf2db548f9756eafde1dc473a83b62af6ef3815`

Receipts in private `extension-evidence-20261010`: `scope-build.json`,
`final-producers.json`, `scope-water-exit-acceptance-v5.json`,
`current-native-acceptance-v2.json`, `water-exit-replay-acceptance-v5.json`,
`weapons-current-acceptance.json`,
`sdk-fetch-acceptance.json`, `scope-ui-verified/mapping-window-ui-result.json`,
`paired-launcher-isolated-smoke/result.json` and `preview-delivery.json`.
No physical input injection was used. Desktop focus/reconnect, rumble feel,
physical non-XInput hardware and broader campaign play remain live checks.

The isolated launch uses Windows [desktop creation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createdesktopw)
and [process desktop selection](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfow);
it never switches the user's input desktop.

## Live verification handoff

Open the desktop **JFG PC Controls Preview** shortcut. Its launcher is
`build/controls-verification/JFG-Launcher.exe`; it selects
`game-pc-controls-extended-20261010/jfg-native-boot.exe` under that build directory.

The prepared local preview uses the separate
`pc-controls-preview-20261010` profile and matched launcher/runtime build.
Its original save was copied from the default profile; this update preserves
subsequent preview progress and customized bindings. Reapply **Normal** or
**Expert** if you want the corrected action defaults, matching the game's
Control Mode. The normal installation
and save remain available separately.

1. With Xbox connected, check left-stick movement, right-stick camera, LT aim
   plus strafing, RT fire, A jump, menu confirm/back, and neutral drift. Try
   sensitivity, both inversion axes and a precise response curve. Set optional
   Rumble strength above zero and check that it stops on pause/focus loss.
   Try sniper zoom with LT plus jump/crouch and check fine scoped aiming.
2. Orbit near a wall/tree and check the new close-camera character fade, then
   aim and release aim several times. Pause/resume,
   enter settings, Alt-Tab away/back while holding a control, and unplug/replug
   with a stick or trigger held. Check for stuck input, sudden turns or firing.
3. Change player 1 to **Mouse and keyboard**. Check WASD, mouse camera, RMB aim,
   LMB fire, Space jump, Ctrl crouch, E interaction, wheel weapon cycling and
   sniper zoom with RMB plus Space/Ctrl. Check menu navigation. Rebind a key and
   mouse button with experiments off, restart, and confirm persistence.
4. Test character/room transitions and the weapons/actions you normally use.
   Try Lupus's jump/jet boots while holding aim and moving the look device.
   Check flight firing with a loaded weapon, and test Vela in deep water.
   Report the character, room, scheme, device and reproduction steps for any
   camera or movement problem.

## Remaining scope

The close-camera obstruction now has an experimental player fade. Lupus flight
and additional weapon firing have focused coverage as recorded above. The
continuation now qualifies Vela entering Water Ruin, diving and surfacing through
ordinary inputs with both look sources. Earlier wrong-character trials remain
excluded. This is a crafted start-location copy of a save, not a claim of
whole-campaign completion or an owner save modification.
Special actions continue to use original guest physics; the modern ground
strafe override does not write swimming or flight movement states. The Lupus
flight replay does not enter the guest manual-aim routine. Holding aim now
keeps modern camera look responsive in that airborne normal-camera state;
it does not add aerial manual aiming or ground-style aerial strafing.
Airborne weapon use is recorded separately from that camera fix; an empty
selected weapon is not treated as evidence of an input failure.

The remaining features were inspected in source. They can be developed
independently of the Xbox/KBM live check, but require the following work:

| Feature | Current boundary | Implementation and acceptance needed |
|---|---|---|
| Swimming and wider campaign | Vela Water Ruin entry, deep water, dive/surface, held-aim look, return to shore and exact recording playback have qualified replays. | Extend to other water scenes, remaining weapon-specific target interactions and upgraded-character actions. No live movement-state forcing or replacement swimming physics. |
| Non-XInput controllers | SDL device identities, standardized input and RmlUi assignment implemented and tested with isolated virtual devices. Xbox remains on XInput. | Physical device/USB/Bluetooth qualification remains. Unrecognized or unidentifiable devices are not claimed supported; gyro and SDL rumble remain separate. |
| Rumble | Experimental XInput output now follows qualified guest Start/Alter/Max/Stop requests; per-player strength defaults off. Model, output lifecycle, RmlUi persistence and native replay checks pass. | Owner verifies physical feel and stop behavior. Unhandled crashes/hard process termination are not certified. Replays never drive physical motors. |
| Gyro | SDL controller discovery is implemented; sensor sampling and calibration are not. | Sensor enable/calibration, orientation transforms, sensitivity and explicit gameplay gating; verify with sensor-capable hardware. |
| Modern split-screen | Per-player bindings exist, but modern camera gates accept single-player and use one guest camera context. | Identify each player's camera/viewport and update context. Test independent look, aim, capture ownership, transitions and no cross-player writes. |
| Guest-menu pointer | Title/start, save-slot selection and the saved-game Play action have experimental pointer targets; launcher menus use RmlUi. | Extend to options, save copy/erase and naming, character selection, pause/inventory and dialogues. Keep menu-specific state/transition gates; a generic click-to-confirm mapping is insufficient. |

Primary API references support the backend plan:
[Microsoft XInput vibration](https://learn.microsoft.com/en-us/windows/win32/api/xinput/nf-xinput-xinputsetstate),
[SDL controller rumble](https://wiki.libsdl.org/SDL2/SDL_GameControllerRumble),
[SDL sensor enablement](https://wiki.libsdl.org/SDL2/SDL_GameControllerSetSensorEnabled)
and [sensor data](https://wiki.libsdl.org/SDL2/SDL_GameControllerGetSensorData).
The earlier feasibility audit is `remaining-feature-feasibility.json`; the
continuation implements XInput rumble and the SDL input backend described above.
Sensor APIs remain reference material until their own implementation and acceptance.

Further autonomous work must recover the existing investigation and run through
the production guarded supervisor. The guard remains enabled; neither this
backlog nor a new session resets its counters or grants another allowance.


## References

SDL implementation references:
[device paths](https://wiki.libsdl.org/SDL2/SDL_GameControllerPath),
[serial fallback](https://wiki.libsdl.org/SDL2/SDL_GameControllerGetSerial),
[event polling contract](https://wiki.libsdl.org/SDL2/SDL_GameControllerEventState)
and the pinned [Windows XInput driver](https://github.com/libsdl-org/SDL/blob/release-2.26.3/src/joystick/windows/SDL_xinputjoystick.c).
The latter identifies XInput paths/GUIDs and stops existing vibration on Open;
our discovery excludes those devices before opening. SDK definitions are pinned
by archive SHA-256 in `cmake/ControllerSdk.cmake`; controller functions load only
from `SDL2.dll` beside the selected runtime, including in the separate RmlUi
mapping process. A missing SDK DLL leaves XInput and keyboard support available.

The input design follows the Perfect Dark PC port; no Perfect Dark game code or
host implementation was copied. Reviewed revision
[`32a1cb9f`](https://github.com/perfect-dark-pc-port/perfect_dark/tree/32a1cb9f268dd3ac73016801025c6bbbfa20130f)
of the `port` branch: [input backend](https://github.com/perfect-dark-pc-port/perfect_dark/blob/32a1cb9f268dd3ac73016801025c6bbbfa20130f/port/src/input.c),
[input interface](https://github.com/perfect-dark-pc-port/perfect_dark/blob/32a1cb9f268dd3ac73016801025c6bbbfa20130f/port/include/input.h),
[movement integration](https://github.com/perfect-dark-pc-port/perfect_dark/blob/32a1cb9f268dd3ac73016801025c6bbbfa20130f/src/game/bondmove.c#L806),
[configuration](https://github.com/perfect-dark-pc-port/perfect_dark/blob/32a1cb9f268dd3ac73016801025c6bbbfa20130f/port/src/config.c),
[config variables](https://github.com/perfect-dark-pc-port/perfect_dark/wiki/Config-variables) and
[MIT license](https://github.com/perfect-dark-pc-port/perfect_dark/blob/32a1cb9f268dd3ac73016801025c6bbbfa20130f/LICENSE).
Retained copies of `port/src/{input,config,video,fs}.c`, their headers and the
license, with a digest manifest, are in the ignored
`tools/upstream/perfect-dark-reference/32a1cb9f268dd3ac73016801025c6bbbfa20130f/`.
Any later source adaptation must keep that notice and add it to
[third-party notices](../../THIRD_PARTY_NOTICES.md).

The mapping window follows interaction patterns from Cemu's
[input settings](https://github.com/cemu-project/Cemu/blob/main/src/gui/wxgui/input/InputSettings2.cpp)
and [device discovery](https://github.com/cemu-project/Cemu/blob/main/src/gui/wxgui/input/InputAPIAddWindow.cpp)
and Dolphin's [mapping window](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DolphinQt/Config/Mapping/MappingWindow.cpp),
independently implemented in RmlUi. The JFG camera and controller entries in
the [upstream reference review](../upstream/reference-review-2026-10-01.md) also apply.

Other Perfect Dark features that could be adapted later, each needing JFG-specific
work: gyro and broader device qualification, screen-shake and crosshair options,
field of view and HUD placement, RT64-based filtering/VSync/frame cap, intro
skipping, guest-menu mouse support, and asset overrides once JFG resource
identities are established. Its high frame rates and network play are not
small portable additions.

## Research and implementation plan (2026-10-09)

The owner requested research -> current-code review -> implementation -> tests ->
live verification, and approved six hours under the existing guarded
`pc-input-aiming-20261008` investigation. Xbox/XInput is the live-verification
controller. Existing accounting and the production guard remain intact.

### Survey scope and source observations

This is a broad survey of 16 source ports/recompilations and two emulator input
editors, not a claim to have found every PC port. Projects were selected for
inspectable primary sources and relevance to analog cameras, shooting, device
mapping, focus changes or menu input. Forks are not counted as independent proof.
Each source link below is pinned to the retrieved revision. Private retained
source files and SHA-256 manifests support the review; no upstream game or host
implementation is copied into JFG.

| Project | Reviewed source | Observation and proposed lesson |
|---|---|---|
| Perfect Dark | [Source at `32a1cb9f26`](https://github.com/perfect-dark-pc-port/perfect_dark/blob/32a1cb9f268dd3ac73016801025c6bbbfa20130f/port/src/input.c) | Separate mouse/controller bindings, trigger aim/fire and wheel actions. Treat pointer movement as look input and preserve game-specific actions; its single-mouse ownership is explicit. |
| Zelda64Recomp | [Source at `b65c482ed6`](https://github.com/Zelda64Recomp/Zelda64Recomp/blob/b65c482ed672258eb684ab94a4e8c8aa662615ee/patches/input.c) | Distinct camera and aim paths; mouse displacement is accumulated separately from analog input. Right-stick ownership is contextual, retaining original button functions when required by gameplay. |
| BanjoRecomp | [Source at `dbd52154bc`](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/README.md) | Config menus support mouse, keyboard and controller; dual analog camera and axis-inversion patches are separate enhancements. Use accessible menus and retain original controls as a selectable baseline. |
| Ship of Harkinian | [Source at `2d529b3155`](https://github.com/HarbourMasters/Shipwright/blob/2d529b3155908f57a831ba4c067a0902d411005f/soh/soh/Enhancements/camera/FreeLook.cpp) | Free look is an explicit camera enhancement with input/state gating. A dedicated input editor exposes bindings. Keep game-camera ownership separate from device discovery and UI capture. |
| 2Ship2Harkinian | [Source at `5edc112595`](https://github.com/HarbourMasters/2ship2harkinian/blob/5edc11259507a03675a9b18836f07f860da0f5f7/mm/2s2h/Enhancements/Camera/FreeLook.cpp) | Camera free look passes through the game collision query and checks camera state; cutscenes reset free-look state. Preserve scene collision and special camera behavior. |
| Starship | [Source at `3322643f0b`](https://github.com/HarbourMasters/Starship/blob/3322643f0bbf80d2f8f7e1c0d2c4c578515f6c90/README.md) | Included in the port inventory. This pass retained its readme and controller interface, but did not establish a reusable modern-camera implementation; no camera behavior is inferred from the project name. |
| OpenGOAL | [Source at `efb21c3e8c`](https://github.com/open-goal/jak-project/blob/efb21c3e8c5a40a74f2e86510f915da55eaeba34/game/system/hid/devices/game_controller.cpp) | Separate HID device handling complements per-device bindings, remembered controller selection and focus policy described in official settings documentation. Do not merge keyboard, mouse and gamepad configuration into an ambiguous input list. |
| OpenLara | [Source at `3b268ca4bf`](https://github.com/XProger/OpenLara/blob/3b268ca4bf986c380f44a26ffd9218bf6d376249/src/input.h) | Input state distinguishes mouse position and buttons from game controls; camera is separate from that input layer. Use explicit state reset rather than leaving held input behind on context changes. |
| TRX | [Source at `e6a32b150d`](https://github.com/LostArtefacts/TRX/blob/e6a32b150dde68f8cf9dd291501596a93cd4e8ed/src/trx/game/input/backends/controller.c) | Keyboard/controller backends and configurable layouts separate physical input from game roles. The project documents multiple user layouts; preserve defaults and allow a recoverable reset. |
| DevilutionX | [Source at `bf574c9b74`](https://github.com/diasurgical/DevilutionX/blob/bf574c9b74f1d718b0bcee15d648b20a8bfc927c/Source/controls/controller.cpp) | Dedicated controller/motion handling and mapped actions; official changelogs record device-sensitive help and menu-navigation fixes. Test menu controls and switching devices as product behavior, not just axis arithmetic. |
| SM64 Coop Deluxe | [Source at `8cd6e5977d`](https://github.com/coop-deluxe/sm64coopdx/blob/8cd6e5977d9f920d51ca71f2c61801d019ed79c6/src/pc/controller/controller_mouse.c) | Mouse focus recovery explicitly ignores held buttons until released; relative capture depends on gameplay/UI focus. SDL controller handling rescales deadzone travel and handles detach/reopen. |
| sm64ex | [Source at `d7ca2c0436`](https://github.com/sm64pc/sm64ex/blob/d7ca2c04364a6dd0dac58b47151e04e26887e6f0/src/pc/controller/controller_sdl2.c) | Separate keyboard and SDL controller sources provide a useful compatibility baseline. Do not equate a C-button mapping with a genuinely independent analog camera. |
| ioquake3 | [Source at `83a776283b`](https://github.com/ioquake/ioq3/blob/83a776283bdb958f82db25554b5ed0966aaf6e49/code/sdl/sdl_input.c) | Mouse activation, event delivery and controller handling have explicit lifecycle paths. Relative capture and focus changes need coordinated event-state cleanup. |
| dhewm3 | [Source at `455b88e8df`](https://github.com/dhewm/dhewm3/blob/455b88e8dff2be822f08eb498f51b383e851fa38/neo/framework/KeyInput.cpp) | Central key binding/command handling is separated from platform input. This pass inspected that binding layer; it does not claim to qualify every controller backend. |
| OpenMW | [Source at `336ca84463`](https://github.com/OpenMW/openmw/blob/336ca84463ba8d1df2730cc394c9cf0dfc4dbf03/apps/openmw/mwinput/controllermanager.cpp) | A controller manager loads mapping databases and handles GUI/game contexts separately from mouse management. Device removal, menu pointer use and gameplay input need distinct handling. |
| UnleashedRecomp | [Source at `cf829a9eca`](https://github.com/hedge-dev/UnleashedRecomp/blob/cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c/UnleashedRecomp/patches/input_patches.cpp) | World-map mouse interaction and controller thresholds have explicit game-specific patches. Guest-menu pointer support requires knowledge of each menu; a launcher pointer alone does not complete it. |

Additional primary references:

- [OpenGOAL settings](https://opengoal.dev/docs/usage/settings/): controller
  selection, focus policy, per-device remapping and separate mouse axes.
- [Cemu input settings](https://github.com/cemu-project/Cemu/blob/main/src/gui/wxgui/input/InputSettings2.cpp)
  and [add-device workflow](https://github.com/cemu-project/Cemu/blob/main/src/gui/wxgui/input/InputAPIAddWindow.cpp):
  explicit add/remove and settings per selected device.
- [Dolphin mapping window](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DolphinQt/Config/Mapping/MappingWindow.cpp):
  discovery refresh and mapping configuration are separate operations.
- [Microsoft XInput guidance](https://learn.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput):
  rescale travel outside a circular deadzone; check connection results and avoid
  querying empty slots every frame. Nonlinear response is a user-tunable choice.
- [Windows Raw Input](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-raw-input)
  and [SDL relative mouse mode](https://wiki.libsdl.org/SDL3/SDL_SetWindowRelativeMouseMode):
  use relative motion for camera control and explicit capture transitions.

These are observations and design guidance, not claims that another game's
camera, aiming logic or menu code can be transplanted into JFG unchanged.

### Current implementation audit

The baseline already has a separate RmlUi mapping window, connected/unassigned
XInput filtering, device-specific bindings, experimental opt-in, raw mouse input,
independent look samples, guest camera/aim hooks and ground strafing. Rebinding
works with experiments off. Existing profile migration and older replay formats
must continue to work.

Concrete issues to address or qualify:

| Area | Current finding | Required implementation / acceptance |
|---|---|---|
| Stick precision | Look first goes through the 8-bit N64 movement scaler, then response shaping, then clamps to 127. High sensitivities saturate early and small precise input quantizes away. | A dedicated modern-look range, radial deadzone and fractional integration; full sensitivity range must remain effective. Preserve legacy recording semantics. |
| Input ownership | Right-stick button suppression depends on gameplay/aim context; custom mappings can include the aim action itself. | Never lose an aim binding while acquiring look ownership; preserve menu/script/unsupported-mode bindings. |
| Focus and reconnect | Global focus/settings release gate exists; connected devices can return while held. | Neutral reacquisition and no stale clicks, wheel pulses, look deltas or movement after focus/settings/device changes. |
| Configuration | Camera/aim sensitivities, vertical scaling and Y inversion exist. | Keep per-device choices, expose useful X/Y inversion and clear defaults, reject malformed settings atomically. |
| Game integration | Normal camera/ground aim hooks exist, but last final replay sweep and paired installation were incomplete. | Fresh current-source native replays with both look sources, ground movement/strafe, fire/jump, transitions, recording playback and experiments-off comparison. |
| Menus and special actions | Launcher has pointer controls; guest menus use keyboard/controller input. Swimming and special states retain guest logic. | Test usable navigation and preserve original actions; separately identify any unsupported pointer interactions or special-state look behavior. |
| Delivery | Old launcher/runtime processes do not acquire new source changes. | Build and identify the exact paired executables, retain saves and prior install, then hand off launch path and a finite live checklist. |

### Acceptance contract

1. **Mapping and persistence:** both game control schemes, Xbox sticks/triggers,
   keys, five mouse buttons and both wheel directions; binding capture/cancel,
   off-state remapping, presets, restart persistence, connected/unassigned device
   picker, unplug/reconnect, and device-specific RmlUi content.
2. **Look and movement:** neutral drift, full stick travel, slow precise aim,
   independent sensitivities, inversion, diagonal motion, simultaneous movement
   and look/fire, releasing aim, and mouse displacement independent of update rate.
3. **Context transitions:** menus, guest/host pause, settings, focus loss/return,
   scripted cameras, device changes and room/character changes. No stuck input or
   delayed look burst; preserve guest physics and camera collision.
4. **Compatibility:** existing profile migration and v1/v2/v3 recordings; disabled
   extensions preserve ordinary controls. Modern recordings reproduce their look
   and movement channels without changing older format interpretation.
5. **Native qualification:** isolated copied saves, pinned producer hashes,
   successful runs and event/state assertions, visual captures, and explicitly
   recorded failures. Unit checks alone do not establish gameplay feel.
6. **Handoff:** one clear launcher/runtime pair, instructions for Normal/Expert
   and enabling each device, exact automated results and limitations, then owner
   live verification with Xbox and mouse/keyboard. Full campaign feel is accepted
   by playtesting, not inferred from narrow replays.

The modern gameplay acceptance scope remains supported-US single-player.
Split-screen modern cameras and gyro remain separate features. Physical
SDL-controller and rumble-feel qualification are not prerequisites for the owner's Xbox/KBM verification. Guest pointer
menus and remaining campaign camera cases remain explicitly tracked; the
qualified close-camera case now has proximity fading. Do not label unfinished
items complete.

### Execution record

- Research: 16 pinned primary-source repositories retained and reviewed;
  existing upstream controller/camera reference review consulted.
- Current-code review: the table above records baseline findings before changes.
- Implementation: precise stick rates, X/Y inversion, reconnect/focus cleanup, corrected Normal/Expert presets, v4 profiles/recordings, camera body visibility, proximity fading and airborne camera ownership.
- Automated qualification: the current-source results and limitations are recorded above.
- Delivery: matched local preview and isolated copied profile, with a verified launcher-to-game launch. Owner live verification remains pending.

## Modern action controls backend (2026-10-10, earlier phase)

The owner requested a control scheme designed for Xbox and DualSense, alongside
all existing original-controller and keyboard mapping support. The detailed
[RmlUi handoff](modern-controls-ui.md) defines diagrams, device-specific content,
context previews, rebinding, conflicts, error states, settings ranges and the
host integration contract. This earlier phase delivered the backend before the
owner supplied the prototype; the subsequent native UI integration is recorded below.

### Implemented

- Independent modern action profiles: modern-controls.ini and per-player suffixes.
  Missing profiles default to disabled. At this earlier stage, the profile-wide opt-in and
  enabled=true were both required; current builds use enabled alone. Gameplay acceptance is supported-US
  single-player, player 1; unsupported modes retain the existing mapping path.
- Gameplay actions for aim, fire, jump, crouch, interact, previous/next weapon,
  scope magnification, pause and menu navigation. Xbox/DualSense use standardized
  physical positions with native glyph names and an explicit presentation override.
  SDL family metadata now distinguishes PlayStation/DualSense from Xbox and
  unknown devices. This does not claim physical DualSense qualification.
- Separate gamepad/keyboard bindings and independent look tuning. Context-aware
  rebind conflicts support reject, replace overlapping actions, keep both and
  explicit unbind. Original controller.ini and pc-input.ini files are untouched.
- A/Cross means Jump and X/Square means Interact regardless of the saved game's
  Normal/Expert option. A temporary, guarded action-table translation runs only
  during supported player updates and restores both original tables afterwards.
  The saved control-scheme option is not changed.
- Hold/toggle aiming, reset and neutral-reacquisition handling, and existing
  independent mouse/stick look, ground strafing and camera protections.
  Menus use explicit bindings; keyboard gameplay directions do not leak into
  menu navigation.
- Dedicated scope cycling on RS click/R3 or middle mouse. Rising edges step
  through internal factors 1, 3 and 5; holding does not repeat. Separate unbound
  increase/decrease actions clamp at the limits. These numbers are not asserted
  to be optical magnification labels.
- Replay v5 records action semantics and scope commands alongside existing
  look/movement. Older formats and original mapping semantics remain supported.

### Automated evidence

The component suite passed all eight selected CTests, including 79 modern action
checks, 193 PC-input checks, 36 rumble checks, 34 controller-catalog checks and
26 isolated SDL backend checks. Input replay, stick and controller-port suites
also passed. The rebuilt launcher passed 572 launcher checks and 15 setup checks.
The offscreen RmlUi mapping regression passed its device assignment/filtering,
reconnect, rebinding, experimental-gate, player-isolation and minimum-layout
checks. Those simulated checks do not substitute for physical device tests.

Native copied-save cases demonstrated:

| Case | Observed result |
| --- | --- |
| Action sequence in Normal and Expert | Interact leaves sniper selected; Next changes weapon 7 to 8; Previous returns to 7. Jump enters airborne state and raises player height by over 140 game units; Crouch enters the crouched state. |
| Scope with precise controller look, Normal and Expert | Exactly the requested transitions: 1 -> 3 -> 5 -> 1 -> 3 -> 1. Holding scope input does not repeat. Zoom-aware aim uses factors 3 and 5. |
| Scope with mouse-look replay | Same dedicated magnification transitions; no keyboard or mouse injection into the desktop. |
| Jump/crouch while scoped | Neither changes magnification. Original sniper-aim physics remain in the aiming state; leave aim to jump or crouch. |
| v5 record/playback | Gameplay, actor, camera, weapon, action and scope traces match exactly on the delivery runtime. |
| Original mapping regression | Selected actor, camera, aim, weapon and scope traces match the previously installed runtime exactly. No modern action translation occurs. |
| Return from modern input | Both original action tables and saved Normal/Expert choice remain intact after inputs settle. Source save fixtures remain unchanged. |

The behavior grader passed 56 explicit assertions. The full gameplay
matrix used producer ecff4ebe96f9ebd9ae05ab0f92d5918a6bb61c4a14d1c6184ae32042552ef64b.
The final UI-contract correction separates keyboard menu movement and rejects
unsupported digital gamepad movement rows; it changes no guest action hook.
Its delivery producer is d9d8bea1a2c6425bf42d4d3551991842ff7e0549615cadddca61684a87166678. That final build passed the component
suite, exact v5 roundtrip, original mapping comparison and the settled Normal
action case. Source and dependency SHA-256 manifests are retained with the runs.

Early candidate boot failures exposed an invalid pre-game actor read in the new
scope observer. Actor validation and inactive-path gating corrected it; temporary
diagnostic observers were removed. Failed runs remain retained. An initial
Normal-case table assertion sampled the process during an active translated
update; the corrected case includes 150 neutral VI after the input sequence and
verifies restoration there. An audit initially misclassified the pre-existing
crash handler as new diagnostic code; comparison with the pre-work source
corrected that assertion.

Private evidence remains under the original checkout's
tools/private/pc-input-aiming-20261008/modern-actions-evidence-20261010:
producer-final.json, producer-delivery.json,
modern-gameplay-verification-v2.json, source-delivery-review-v2.json,
ui-regression/mapping-window-ui-result.json and per-scenario traces/captures.
The public source contains no copied ROM/save payload.

### Earlier backend delivery

At the end of that earlier phase, the rebuilt launcher was at
build/modern-controls-backend-20261010/JFG-Launcher.exe.
The matching backend candidate is at build/modern-controls-backend-20261010/game-modern-actions-d9d8bea1a2c6/jfg-native-boot.exe for UI integration. The
owner's existing preview executable, shortcut, profile and saves are unchanged.
Launching the rebuilt launcher does not reveal a new Modern Controls page yet.

At that stage, the RmlUi artwork, native adapter and end-to-end UI checks were
still pending. They are implemented in the subsequent section. Live verification
still covers Xbox and keyboard/mouse: consistent actions in both guest
schemes, hold/toggle aim, scope cycle, custom bindings, device/focus changes and
settings while inputs are held. Physical desktop input/focus/reconnect/rumble
tests remain reserved for the owner. Native DualSense input and gameplay feel
need physical verification; native SDL rumble, gyro, adaptive triggers and modern
split-screen remain unsupported. Broad campaign feel is not established by the
narrow copied-save matrix.

The existing guarded investigation and production ledger remain authoritative.
This continuation preserves all prior charges and does not authorize a new
investigation, bypass a denied lease, commit, push or release.

## Native Modern Controls editor and scope tuning (2026-10-10)

This continuation installs the owner's Idea 3 prototype as a native RmlUi editor
alongside original Controller Mapping. The current contract and priority list
are in [Modern Controls](modern-controls-ui.md).

The highest-priority additions are implemented:

- Separate controller and mouse **Scope sensitivity** settings (10-300%, default
  100%), independent of camera and aim tuning. Live qualified sniper input is
  scaled before recording; playback uses the recorded deltas. Swimming/airborne
  camera ownership is excluded and fine fractional motion is retained.
- Truthful **Linear / Quadratic / Cubic** curve names and descriptions.
- Editable **Increase / Decrease magnification** alongside Cycle magnification.
  Dedicated scope commands are edge-triggered; Jump/Crouch do not change zoom.
- Saved settings and real controller discovery replace prototype examples.
  Xbox, DualSense, generic and keyboard/mouse diagrams use current bindings.
  Buttons/Settings, List/Diagram and context previews are native RmlUi content.

Modern edits remain independent of original mappings. Original keyboard/mouse
rebinding works with experiments off. At this earlier integration stage, Modern style and the profile-wide
experimental switch were separate; the current two-mode UI removes that gate. The editor handles release-before-capture,
timeout, focus/device loss, conflicts, safe swapping, save failure/retry,
selected-device reset and unreadable-profile recovery. Controller navigation
covers buttons, sliders and device selectors.

### Qualification for this integration

The modern profile/model suite passed **101 checks**. All **eight** selected
component CTests passed; the existing original-mapping UI regression also passed.
The final offscreen native RmlUi fixture passed **228 checks**, and the rebuilt
launcher passed **572 launcher checks** plus **15 setup checks**.

Thirteen copied-save native runs cover original mapping, Normal/Expert actions,
controller/mouse scope commands, jump/crouch isolation, final-runtime regression
and nonzero-look recording/playback. The behavior grader passed **95
assertions**. Selected gameplay events match the prior qualified behavior
exactly. Both controller and mouse recordings replay identically after changing
the current scope sensitivities, including nonzero aiming at factors 3 and 5.

The final runtime SHA-256 is **653cbf03b0ec9fd35b7c700c9ffeb3b6f7359c093ab16a52567b63c3fb9c7b4e**.
Portable tests verify the live multiplier, migration, fractional precision,
clamping and context resets. Replays establish playback independence and existing
gameplay semantics; they do not substitute for hands-on slider/focus testing.
No physical desktop input was injected, and source save fixtures remain unchanged.

Private evidence:
tools/private/pc-input-aiming-20261008/modern-prototype-evidence-20261010 contains
producer-final.json, modern-prototype-verification.json,
ui-final-verified/result.json and per-scenario traces/captures.
Failed intermediate harness runs remain retained: launcher tests initially used
an output directory outside the source checkout; the Cancel-click fixture
initially omitted the dialog's transform. Both fixtures were corrected and the
final checks passed.

### Owner live verification

The paired preview is build/modern-controls-preview-20261010.
Use its **Modern Controls Preview.lnk** or the desktop **JFG Modern Controls
Preview** shortcut; these select the separate modern-controls-preview-20261010
profile and matching runtime. Opening JFG-Launcher.exe without profile arguments
uses the normal default profile instead. The preview profile is copied from the
previous controls preview and has Modern Controls selected; the original profile,
saves and old preview shortcut remain intact.

Open Controllers, select Player 1 and the connected Xbox controller, and inspect
Modern controls. Verify:

1. Move/strafe with LS and look with RS. A jumps, B crouches, X interacts, LT aims,
   RT fires, and shoulders change weapons in both saved Normal/Expert schemes.
2. Hold/toggle aim; click RS to cycle sniper magnification. Bind optional
   Increase/Decrease, then compare Scope sensitivity at 50%, 100% and 150%.
   Leave aim before jumping or crouching.
3. Remap through the diagram and capture; resolve a conflict, restart the
   launcher, and verify saved bindings. Return to Controller mapping and confirm
   the old mappings are retained.
4. Select Mouse and keyboard. Verify WASD/mouse, aim/fire, custom keys, side
   buttons and wheel, and independent camera/aim/scope tuning. Esc releases mouse
   capture; click the game to recapture.
5. Alt-Tab, disconnect/reconnect and change settings while controls are held.
   Check for stuck actions, unexpected camera jumps or persistent vibration.
   Physical focus, reconnect and vibration checks are reserved for the owner.

Native SDL/DualSense rumble is the next capability to implement. Gyro and adaptive
triggers remain later work, with physical hardware qualification required.
Modern split-screen and broad campaign camera feel remain outside this narrow
acceptance. Glyph support is not a claim of those hardware features.


## Prototype fidelity revision (2026-10-10)

The Modern Controls editor was rebuilt in native RmlUi to follow the supplied
prototype's layout and artwork. It now has compact player/device controls,
radio-card style choices, three binding columns, the linked list/diagram,
original glyphs and selection outlines, grouped action pickers, compact tuning
cards and a connected/available device menu. The original mapping mode remains
available. See [the fidelity contract](modern-controls-ui.md#prototype-fidelity-contract-2026-10-10-revision).

Reset sensitivity & behavior preserves bindings; Restore modern defaults on
Buttons restores the selected input type's layout. Both explicitly restore
Hold aim, which is a shared modern-profile behavior. Clearing a gameplay row
preserves separate menu bindings.

The native offscreen fixture passes 403 assertions, including actual RmlUi
pointer hit tests. It uses injected devices and a nonactivating offscreen window;
no physical desktop input or focus/reconnect/rumble playtest was performed.
The game runtime is unchanged from the qualified 653cbf03b0ec build above.
This is a launcher/UI revision; the same Modern Controls Preview shortcut and
profile are retained, including the owner's saves and settings.

Private visual evidence and final qualification are recorded under
tools/private/pc-input-aiming-20261008/modern-fidelity-evidence-20261010.
The final preview includes a UI review gallery beside its verification records.

The fidelity launcher also passed 572 launcher checks and 15 setup checks; the
101-check modern model suite and original-mapping UI regression passed. The
candidate PE resource table was inspected to verify the actual executable
contains the final RmlUi markup, stylesheet, device artwork and glyph/highlight
atlases byte for byte.

### Owner UI review corrections, 2026-10-10

The native controls window now omits the redundant Controls banner, Gameplay
selector, persistent Saved status, footer experiment links, unbound-action card
and global rebind link. Per-input binding, capture and menu/scope actions remain.
The footer has a real reset button and centered Done text. Advanced settings is
a top tab; Camera/Sticks and Aim & Scope/Vibration stack independently.
Automatic symbols show the detected family, and XInput assignments identify
their API family. That earlier revision retained a separate switch; the current mode selector replaces it.

Repeated clicks on either former selector could reach an obsolete numeric
action handler and throw while parsing "select". Those routes are removed;
glyph changes also defer rebuilding the form until the next frame. Native
pointer tests exercise repeated selector clicks. Supersampled circular details
and nine-patch selection frames remove the broken thin edges at fractional DPI.
The actual owner screenshot size (1583 x 1095 client pixels at 125%) is included
in the isolated review gallery, alongside 100% and 150% views.

The corrective revision passed 419 native UI assertions, original-mapping
regression checks, 572 launcher checks and 15 setup checks. Both the UI fixture
and rebuilt launcher's embedded resources were checked byte for byte against
the source assets. Profile/saves and shortcuts are preserved during delivery;
the game runtime is unchanged. These tests inject only isolated fixture input,
not physical desktop/controller input.

### Two-mode controller UI and runtime qualification, 2026-10-10

The latest owner review is implemented: original Controller Mapping now uses
the prototype's padded cards and top tabs, physical-input glyphs and compact
presets. Both modes have the cleaned footer and compact device header.
The symbol label/value and square picker close button are centered.

Modern Controls is itself experimental and is the only runtime opt-in. The old
profile-wide switch is no longer read; original mapping ignores saved legacy
look flags while retaining original key/button bindings. Changing styles clears
pending look/capture and aim/zoom state and requires neutral controls.

Qualification: **421 modern UI assertions**, **207 original-mapping
assertions**, **572 launcher checks**, **15 setup checks**, and all **8 controller
component CTests** passed. Both native UI test executables and the rebuilt
launcher contain the same checked resource bytes. Review images include the
owner's 1583 x 1095 client size at 125% scaling.

Five copied-save game replays passed **25 checks**. Modern
controller and mouse input work with the legacy switch off. Original mapping
with that switch on emits no modern aim/camera/action hooks. Changing the legacy
switch produces identical relevant gameplay traces within each selected style.
The modern controller trace also matches the previously qualified runtime,
including actors, scope, aim, orbit and camera behavior. Original recordings
remain v2 and modern recordings retain their action-aware v5 format.

These checks used isolated fixture input. Physical controller, focus/rumble feel
and broader gameplay remain for the owner's live verification.


## Initial native title/save pointer implementation (2026-10-10)

The initial navigator described here was superseded by direct native selection
in the shoulder-camera audit below.

The Win32 window thread transfers client-space pointer positions and fresh
button edges through the same locked mailbox as focus/keys. Click coordinates
are retained even if the pointer moves before the next controller poll.
The game thread alone reads the supported US frontend state. Overlay reads
validate section metadata, bounds and entry signatures; no guest selection
or save data is written by the pointer adapter.

Menu targets map the displayed VI aspect into the native 320 x 240 menu space.
A bounded navigator emits the existing horizontal navigation/Confirm packets
until the observed selection reaches the clicked target. These effective
packets enter the existing recording path, so playback requires no pointer
coordinates. A parked pointer does not fight keyboard navigation, and held
mouse buttons do not repeatedly confirm across pages.

The private diagnostic variable JFG_PHASE9_MENU_POINTER_REPLAY accepts a bounded
jfg-menu-pointer-v1 script only alongside a noninteractive controller replay.
It never injects desktop input and is rejected for live play. Qualification
evidence is retained under the existing pc-input-aiming-20261008 investigation;
physical focus/cursor behavior remains part of owner live verification.

The delivered candidate passed 69 pointer/mailbox checks, 193 existing PC-input
checks and 101 modern-controls checks. Four copied-save native replays verify
all six slot previews, title/start/save/Play clicks, the start-menu Options target,
and original keyboard/controller navigation. Replaying the recorded effective
controller packets reproduces the same complete RDRAM and captured frame.
Original navigation also matches the pre-change RDRAM and frame exactly.
These are automated adapter/guest checks, not physical desktop focus testing.


## Live menu click and cursor correction (2026-10-10)

Owner verification found that pointer hover selected main-menu entries but
clicking did not activate them. The Win32 render child unconditionally returned
MA_ACTIVATEANDEAT for client activation and also discarded clicks transferring
focus to that child. Replays started after those message handlers and therefore
did not test this boundary.

The window now retains activation clicks when its launcher root is already the
foreground window. A click returning from another application is still consumed.
The client explicitly restores the arrow cursor when mouse look is not captured;
nonclient resize cursors remain managed by Windows even during gameplay capture.
The window class also has a default arrow cursor.

A regression harness compiles the actual LiveRt64Window class from the runtime
source and drives its real Win32 messages, focus, capture and cursor APIs on an
isolated desktop that is never displayed. Only the foreground-window observation
is supplied by the harness because an unshown desktop cannot become the system
foreground. It reproduces the original click/cursor failures and passes 17 checks
after the fix, including a mouse click reaching native-menu Confirm, focus
transfer within the launcher, four resize-cursor shapes, capture release and
consuming a click from another application. This supplements, rather than replaces,
owner verification of physical focus/cursor behavior.

The rebuilt runtime also passes 69 pointer/mailbox, 193 PC-input and 101 modern
controls checks and four copied-save native replays. Recorded pointer-generated
controller input produces byte-identical game memory and frames; original
keyboard/controller navigation matches the retained pre-pointer baseline.
The set of supported native pointer menus is unchanged.

API references: [mouse activation](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mouseactivate)
and [client cursor handling](https://learn.microsoft.com/en-us/windows/win32/menurc/wm-setcursor).


## Modern shoulder-camera and mouse audit (2026-10-10)

The owner reported independent camera orbit while walking, aim entry snapping
back to the character, floating mouse feel, firing dropping held aim, and
save-slot hover walking through intervening slots.

| Finding | Correction |
| --- | --- |
| The old normal camera orbited independently of player facing. | Modern ground view now owns a shared heading before movement, with forward/back and lateral strafe in the existing character physics path. |
| Native run-to-aim initialization cleared pitch and used the old body direction. | Seed aim from the shared view; preserve yaw and pitch on entering and leaving aim. Normal-camera and native-aim pitch have opposite signs, which are converted explicitly. |
| Native follow interpolation continued moving the view after raw look ended. | Use the current collision-resolved normal-camera position. Raw mouse displacement is consumed once per poll, with no added input smoothing. |
| Reacquiring Win32 capture while a second mouse button was pressed cleared the first button. | Acquire capture only when this window does not already own it. RMB remains held across LMB press and release. |
| Save-slot hover generated repeated virtual-stick pulses. | Send a semantic selection event to the validated active native menu. Select its item directly before the original activation handler; preview/rendering remain native. |
| Modern mouse reset forced noninverted vertical look. | New modern defaults and settings reset enable mouse vertical inversion; explicit saved choices and controller inversion remain independent. |

Mouse look is still sampled at the native game/controller update cadence; this
change does not add a separate render-rate camera. Removing follow easing avoids
additional drift but does not promise high-refresh PC-engine input latency.

The pointer adapter only writes the current menu-selection field after validating
the supported screen, enabled target, active overlay identity, bounds and code
signature. It does not edit save contents. Relocated overlays use their validated
mapped host span rather than physical-RAM aliases. v6 recordings include menu
kind and target independently of controller buttons, allowing exact replay of
direct selections; wrong-screen events are rejected.

This changes normal single-player modern movement, not the original mapping
mode. Swimming keeps the earlier free-look camera and native steering. Other special states retain their native movement handlers. The
camera retains native collision, obstruction clearance and close-body fading.
Weapon targeting still uses the native weapon logic. The implementation does
not add new character animations or a configurable
left/right shoulder offset.

Audit qualification passed: 201 PC-input checks, 104 modern-controls checks,
71 menu-pointer/mailbox checks, replay format regression tests, 21 Win32
mouse-chord/cursor checks, 521 modern RmlUi assertions,
207 original-mapping assertions, 572 launcher checks and 15 setup checks.
The actual RmlUi launcher and game runtime were rebuilt.

The final runtime passed five native-menu routes (including direct slot-5 preview,
Options, recorded playback and unchanged original navigation), three composite
mouse/controller gameplay runs, two additional character runs, and three sniper
regression runs. The composite checks cover ground facing/strafe, turning while
moving, vertical aim, held aim during fire, release/entry, zero neutral drift and
jumping after leaving aim. World-position deltas were also compared to the view
heading; a focused native collision-boundary capture distinguishes the original
scenery correction from the requested movement. The focused call-site audit
ruled out the native camera's minimum-distance player push in those samples.
A separate forward/back/left/right
route verifies actual movement in open space. Original-mapping gameplay is
also byte-identical to the previously installed runtime. Mouse gameplay and
direct menu recordings each reproduce byte-identical
RDRAM and captured frames. Original menu navigation matches its retained baseline.
Qualification and failed intermediate trials remain in the private investigation
evidence; no physical desktop input was injected. Physical Xbox/DualSense input, desktop focus transitions,
subjective camera feel, crosshair/weapon alignment in varied terrain, and broader
campaign/special-state coverage remain live-verification items. Native pointer
coverage remains limited to the title, main menu, save slots and Play action;
other menus listed above still use their keyboard/controller bindings.


### Upward camera, running speed and scripted-input follow-up (2026-10-10)

The owner reported idle movement at the upward look limit, unexpectedly fast
Juno movement, and mouse control during the SS Anubis shotgun-chest pickup.

- **Upward slide:** reproduced 57.891 world units of idle forward displacement.
  The native normal camera calls actor translation at 0x8002EB54 to preserve
  32 horizontal units of camera/player separation. Its floor collision at an
  upward pitch repeatedly triggered that correction, despite zero walking
  velocity. Modern shoulder view now suppresses only that qualified camera
  call. Native player physics, scenery collision and camera clearance remain
  active. The same up/down replay now has zero player displacement.
- **Running speed:** the old opt-in Juno 2x navigation mod is not enabled in the
  owner's normal preview. The launcher clears inherited JFG_* variables.
  A separate modern-control regression used raw host axes and a 5-unit speed
  cap. Native normal movement uses post-deadzone axes, clamps radial magnitude
  to 65, then scales by 1/16: 4.0625 units per guest movement tick. The modern
  cap was therefore 23.08% too fast. Modern ground movement now uses those
  native filtered axes and the same 4.0625 cap, including diagonal normalization.
  Modern start/stop is immediate by default. The optional keyboard/mouse
  original-acceleration setting below restores gradual starts and stops.
- **Pickup/cutscene ownership:** general gameplay flags alone were insufficient
  at the outer player-update boundary. The modern heading write could precede
  the character's own input lock. PC look/capture/movement now also checks the
  shared character lock fields and scripted-character controller before taking
  ownership, and rechecks ownership before physics/camera writes. This covers
  the native lock conditions used by Vela, Juno and Lupus. Synthetic memory tests
  exercise each lock and release, as well as the global lock. An additional
  native dialogue replay reproduced mouse rotation while character lock 0x1F9
  was active and the global lock was zero; the corrected runtime yields control
  completely throughout the same dialogue. The exact SS Anubis
  shotgun chest still needs owner live verification; this review did not replay
  the five-platform chest route or claim full campaign cutscene coverage.

Validation: 204 portable PC-input checks, navigation/input-lock tests, rebuilt
RmlUi launcher tests (572 launcher and 15 setup checks), eight native regression
runs (up/down neutral view,
original/modern Juno speed comparison, original-controls comparison, and mouse
plus dual-stick movement/aim/fire, and paired native dialogue-lock runs).
The original-mapping final RDRAM and rendered
frame match the prior runtime byte-for-byte. Physical desktop input was not
injected. Private qualification retains the native camera caller trace, original
and corrected motion, exact executable hashes and unchanged source-save checks.


### Optional original keyboard movement acceleration (2026-10-10)

Choose **Mouse and keyboard â†’ Modern Controls â†’ Advanced settings â†’ Original
movement acceleration** in the RmlUi controller window. The option defaults to
**Off**, including existing profiles. Off keeps immediate starts and stops.
On restores the original ground acceleration/braking curve while retaining
modern forward/back/strafe movement and the corrected 4.0625 speed limit.
Mouse look remains direct. Controller movement, original controller mapping,
and the game's special movement states are unaffected by this option.

The native Juno ground handler uses factor 0.95 raised to the integer number
of guest movement ticks (helper 0x80049BEC, return 0x01002D88), then blends
current velocity toward the requested velocity. A focused native register
capture confirms the single-precision constant and tick argument. The modern
adapter snapshots velocity before the guest action handler and applies that
curve once to each ground movement component before collision/physics. It
does not ease an already-eased value or accumulate a separate stale velocity.
Character input locks and scripted-camera ownership still gate the adapter.

The setting is stored as mouse_original_acceleration in modern profile version
3. Exact version 1 and 2 profiles migrate in memory with the option Off; saving
writes version 3. Invalid/mixed schemas remain rejected atomically. Restoring
keyboard/mouse defaults resets it to Off; controller defaults preserve it.
Recorded gameplay must be replayed with its matching modern profile, including
this setting.

Validation covers default/migration/roundtrip/error parsing, native acceleration
and braking samples, diagonal speed limits, real offscreen RmlUi hit targets,
save/reload and reset, and keyboard-only visibility. Native replay comparisons
verify unchanged Off behavior, unchanged original mapping, unchanged controller
behavior with either setting, held aim during fire, settling after releasing
keys, and deterministic playback with the matching profile. No physical desktop
input was injected. Live verification should compare starts/stops with this
option Off and On; subjective feel and the previously noted SS Anubis shotgun
chest still belong to owner playtesting.


### Native backward and directional locomotion (2026-10-10)

The original game includes backward locomotion for Juno, Vela and Lupus.
Modern controls were writing their final local movement after the native
animation selector had already run. The original handler also calculated a
bank from its attempted body turn, although modern shoulder controls replaced
that heading. This produced forward-looking steps while travelling backward
and a persistent sideways lean (about 29 degrees in the reproduced Juno frame).

Modern normal ground movement now supplies its final velocity and backward
flag before native animation selection as well as collision/physics. It clears
only the turn-generated bank in that state. The native animation update,
phase advancement, weapon remapping, terrain response and special-state
handlers still run. Original mapping, jumping, aiming, damage, pickups and
scripted input locks retain their existing ownership checks. The original
acceleration option uses the same pre-update velocity snapshot at both hooks;
it does not apply the acceleration curve twice.

The native selector can also keep the previous lateral clip when a character
reverses strafe direction without turning. The modern adapter resolves the
requested animation's family using the active character's original mapping
table, then corrects ordinary locomotion requests for their actual local
velocity. Juno and Vela use their original left/right sidesteps and backward
clip; Lupus keeps his own animation catalog. Special animation requests remain
with the game. No animation assets are included in the repository or replaced.

Validation includes 236 PC-input checks, 111 modern-controls checks, navigation
and input-lock tests, isolated native mouse/controller replays, native character
identity checks, forward/back and left/right reversals, backward fire, held aim
while firing, jumping, and original-acceleration samples. Original mapping's
captured RDRAM and rendered frame match the preceding runtime byte for byte.
Matching mouse/controller routes also produce identical final RDRAM and frame.
The existing speed limit remains 4.0625; this fix changes animation selection,
not the movement-speed policy. Exact executable hashes and per-case coverage
are retained with the preview's backward-animation-20261011 qualification.

Live verification: compare S/backward movement with forward walking, switch
A/D or left/right stick direction without stopping, then repeat while firing
and with Original movement acceleration On. Check foot motion, upright posture,
and the transition back to idle. The SS Anubis shotgun chest remains a separate
owner live-verification case from the earlier input-lock audit.

Turning in place: the normal ground handler and animation selector inspected
here use heading rotation and turn bank, not an identified dedicated stationary
pivot clip. Backward locomotion is confirmed; a reusable pivot/foot-plant
animation has not been confirmed. This is not an exhaustive assertion that no
unused asset exists. Modern mouse look therefore still turns the body directly;
new foot-plant/blending work is outside this verified locomotion correction.

Launcher provenance: this local controller preview is still based on the
1.0.0 launcher checkout d3b9d62, with the modern-controls changes. The separate
1.1.0 release's one-time ROM import/home-screen flow is not integrated by this
animation fix. Rebuilding the preview alone does not bring those release changes
across. Its saved external ROM path is updated only as explicitly approved.


## Stationary pivot feasibility review (2026-10-11)

This is an animation inventory and engineering feasibility review, not a pivot
implementation or an exhaustive visual identification of every unused asset.
The tested backward-walking fix remains unchanged.

The saved native gameplay snapshots contain 52 animation slots for Juno, 53
for Vela and 29 for Lupus. All slots have corresponding entries in each
character's five-byte animation-remapping table. The survey inspected 51 Juno,
51 Vela and 36 Lupus direct call sites to the native character or common
animation setters, alongside the ground selectors and idle/weapon remaps.
Several animations that are not directly requested are reached through those
remaps; absence of a literal call is not proof that a clip is unused.

| Character | Priority candidates for visual inspection | Qualification |
| --- | --- | --- |
| Juno | 38, 43 | No direct literal request identified in this character overlay; data-driven and external callers remain possible. |
| Vela | 48, 52 | Same limitation; no demonstrated stationary turn. |
| Lupus | 15, 17, 21 | Same limitation; four-legged locomotion needs separate evaluation. |

The final Juno slot (51) and Lupus slot (28) do have native callers; they must
not be treated as spare slots. Juno 18 is an idle candidate, not evidence of an
unused pivot. The existing backward clip is 3; human sidesteps use the 9/10
families. Clip 15 in the human ground handler is used by braking/skid logic.
No dedicated left/right stationary pivot was identified in the normal ground
selector. This does not establish that none exists anywhere in the ROM.

A custom pivot is technically feasible. The native animation setter already
retains old/current pose state and normalized phase for its animation pipeline.
Juno and Vela stream animation data while the inspected Lupus model keeps
resident animation pointers, so appending a slot requires handling both paths,
model lifetime, remapping, clip bounds and transitions. There is currently no
qualified custom-animation import/encoding pipeline for these character assets.
A model or animation header from another Rare game must not be substituted for
JFG's actual format; the original setter and loaded structures are authoritative.

The preferred sequence is:

1. Produce an isolated clip gallery for the candidates, including start/end
   poses, foot contacts and root motion. Check unarmored/armored models and
   weapon variants before declaring an asset unused or reusable.
2. If none fits, author original left/right step-turn clips for the existing
   rigs, separately for Lupus. Use planted-foot steps and zero gameplay root
   translation. Validate import/roundtrip and blending before enabling them.
3. Drive the visual foot/hip pose from signed accumulated yaw while stationary.
   Keep camera and gameplay facing responsive; do not delay mouse rotation or
   reintroduce the orbit/aim snapping problem to accommodate the animation.
4. Restrict ownership to modern, grounded, stationary, unlocked gameplay;
   release immediately for movement, jumping, aim transitions, damage, pickups,
   dialogue and cutscenes. Preserve native upper-body weapon poses.
5. Verify both directions, slow rotation, rapid reversals, 180-degree turns,
   slopes and narrow bridges for all characters. Require unchanged position,
   collision, hitboxes and gameplay timing, plus stable feet and clean blends.

A procedural leg/hip pose layer is an alternative, but it still needs validated
joint mappings and foot placement. Rotating the whole model or looping a walk
animation at zero speed would retain the visible sliding this work is meant to
solve. No speculative pose or asset change was installed in the test preview.

## Launcher v1.1 integration (2026-10-11)

Integrated the v1.1 source changes through `c994c75` into the working controls
source, retaining the RmlUi modern/original editor, mappings and native gameplay
fixes. This is an uncommitted local integration, not a published release.
The launcher now imports a verified ROM into the profile once, hides the home
ROM card after import, verifies the bundled runtime/libraries, and retains the
v1.1 graphics preflight and centered 4:3 game viewport.

The build is staged separately at `build/modern-controls-v11-20261011`.
Its bundled gameplay runtime is the previously qualified backward-animation
runtime (`de2737396c3f`); this integration changes the launcher, not that runtime.
The running preview, its profile and its existing Desktop shortcut were not
replaced during the user's live test. Source-before copies and merge resolutions
are retained in private evidence.

Validation: 597 launcher helper checks, 15 setup checks, 14 packaging tests,
6 graphics-adapter checks, game viewport cases, original controller editor
regressions, and 545 offscreen modern-editor/home checks passed. A separate
profile migration exercised init, verify and repeated init with the real
supported ROM and matching runtime package; the imported ROM hash matched and
all copied controller preference files remained byte-identical. Home-screen
checks verify the import card appears initially, disappears after an imported
state, and returns when that state is cleared. No physical desktop input was
injected. A final packaged launch and gameplay interaction remain live checks.

## Bridge frame-pacing report (2026-10-11)

The user reports substantial frame lag with modern keyboard/mouse movement over
bridges. Investigate after launcher integration: compare the same quiet bridge
route with modern keyboard/mouse, modern controller and original controls.
Separate host input latency from CPU simulation, camera collision and rendering
cost; compare without simultaneous compilation. Do not infer a speed multiplier
or change movement behavior from perceived lag alone. Preserve the delivered
movement and animation fixes until a measured cause is established.


### Follow-up findings from live testing

The bridge report is specifically **Goldwood, the bridge after the first enemy
section**. Retained level-92 diagnostic traces are not an exact reproduction.
On their matching 4000-4180 retrace interval, modern mouse and stick median
update spacing was approximately 33.45 and 33.56 ms respectively. The traced
orbit-to-clearance span was about 1.64 and 1.66 ms median. Those are wall-clock
trace intervals, not isolated CPU timings or a benchmark of the reported bridge.
The geometry broadphase currently decodes room mesh and enumerates actor/model
bounds each camera update; it is a profiling candidate, not an established cause.
No performance behavior was changed. The next measurement must isolate the
reported crossing, stop compilation, and separate input, simulation, camera
collision and presentation time. Any mesh cache needs verified invalidation for
room changes and mutable geometry; dynamic models must remain current.

The pause screen's **Z Save Game** prompt exposes a confirmed modern-input gap:
`ModernAction` and `map_modern_actions(..., menu=true)` provide confirm, back,
pause and directions, but no save action. `GuestMenuKind`/`read_guest_menu`
recognize title, start, save slots and one save-action page, not the in-game
pause screens. This affects both discoverability and mouse hit targets.

The required repair is a named, rebindable menu Save action, a discoverable PC
prompt, and native-state-aware pause/submenu hit targets, including Save Game.
Use each original menu's activation/confirmation and save pipeline; never write
a save directly from a screen coordinate. Validate legacy profile migration,
transitions, disabled entries, back behavior, letterboxing and one-click save
with an isolated save copy. Until then, Controller Mapping can expose the
original Z binding. The user's inspected original keyboard profile maps it to
left mouse, and Enter to confirm. This is a temporary workaround, not a modern
menu fix. No pause-menu changes were installed in this integration.


### 2026-10-11: Goldwood capture, camera stepping, and native pause Save

The owner captured the Goldwood bridge after the first enemy section and identified
the very end as the worst bridge segment. The same recording includes drone combat.
Private evidence is retained in the guarded `pc-input-aiming-20261008` investigation,
`pause-goldwood-evidence-20261011`; ROMs, captures, and save data are not public assets.

**Critical and still open: combat update slowdown.** Between approximately 3:10 and
3:30, update intervals typically reach 67 ms (about 15 game updates/second), with
85 to 98 ms worst gaps. The host VI tick remains near 16.7 ms. CPU graphics submission
stays around 2.6 to 3 ms median in these windows; these timings do not measure GPU
completion. More active enemies/debris coincide with the slowdown. This is evidence
of an actual cadence problem, not proof that entity simulation or the GPU is its
cause. A steady 30 Hz game also feels coarse during rapid mouse turns, but does not
explain these approximately 15 Hz periods. A subsequent full/light logging comparison reproduced the slowdown with identical
final game memory; detailed capture logging is not the sole cause (see below). Do not apply a global speed or
timer multiplier: world speed, AI, animation, audio and input timing must stay correct.

The small repeated camera steps have a separate confirmed cause. The modern ground
camera used direct yaw/pitch and position but inherited a changing distance from
the original delayed follow camera. At fixed view angles, with no collision
correction, that distance sometimes changed by roughly 16 units in one update.
The candidate uses a fixed 200-unit modern ground boom before the existing native
scenery solver. Original mapping, swimming/flying and aiming retain their own
camera ownership. This does not smooth or defer raw mouse input. The original
capture contained 186 unobstructed fixed-angle relative camera steps over five
units; replay with the candidate contained zero over 1,042 comparable pairs
(maximum relative step 0.000245 units). These are different runs, not identical
frame-by-frame performance measurements. Large collision/body-visibility pull-ins
are still observed and remain a separate unresolved camera issue.

The main native pause page now supports direct mouse selection/activation of visible
icons and clicking its Save prompt. Selection updates the native selected-icon
field and lets the original screen transition run; it does not walk through
intermediate controller directions. Save passes a qualified Z activation to the
original countdown/Flash save path. The isolated native test observed the countdown,
a changed test Flash save, direct Weapons selection, and native Back return; the
source save remained unchanged. Other pause subpages still require their existing
keyboard/controller navigation.

Modern controls add a remappable **Save game** menu action: **F5** on keyboard,
**X / Square** on a controller. It only emits Save on the validated main pause page
while the native save state permits it. Profile schema 4 migrates schemas 1 through 3 and
leaves a conflicting custom menu binding intact by leaving the new Save binding
unassigned. Input recording schema 7 adds pause selection/Save targets and retains
read compatibility with schemas 1 through 6. RmlUi includes Save in the menu action picker.

Pivot animation review remains unresolved: the previously listed candidate clips
are not confirmed stationary pivots and were not installed. The pinned upstream
model definitions describe animated vertex data and per-instance animation/frame
state, not a ready-made modern skeletal animation import pipeline. The resource
format and streaming ownership must be validated before claiming an ordinary
skeletal clip import will work. A custom pivot needs model-specific vertex/pose
export, frame interpolation, zero gameplay root displacement, planted-foot contact,
left/right selection, interruption/blending and coverage for all three characters.
The candidate list still needs visual inspection; neither an unused clip nor a
production-ready custom pivot is claimed.


### 2026-10-11: Critical Goldwood timing investigation

**Severity: critical. Status: open, clock replacement under qualification.**
The delivered v1.1 camera/Save preview is unchanged by this timing investigation.
The new clock is diagnostic-only, defaults off, and is not a launcher setting.
A successful boot or a faster run in another room is not acceptance of this fix.

#### Measurements and cause

The runtime currently charges guest Count for modeled N64 instruction/cache costs,
then paces the host to that device timeline. That preserves original-console CPU
throughput, including its slowdowns, while also paying the host cost of evaluating
the model. It is unsuitable as an unquestioned native-PC performance policy.
The initial lower-overhead/full-logging A/B ran 17,000 VIs on the same instrumented
runtime and finished with identical RDRAM SHA-256
`757b7094a9d6fabd671e4c90ba7d50e54a52741d3e7ff3ffb57df3f062b93d99`.
The authoritative complete digest is retained in `reference-final-parity.json`;
the comparison checks all 32 digest bytes, not a shortened screenshot or FPS counter.

During the original worst combat window, the light trace measured approximately
67.1 ms median updates and 83.6 ms at the 95th percentile. Modeled non-idle work
was about 62.3 ms per update. Wall time minus explicit pacing waits was about
37.9 ms per main-frame call; this includes other blocking and is **not** an
isolated CPU execution measurement. Host VI continued near 16.7 ms. Extra modern
camera visibility queries averaged approximately 0.14 ms of modeled time and
0.52 ms host wall time per traced invocation, so they do not explain this deficit.
These measurements do not establish GPU completion time.

Actor rows are deltas, not a full census on every update. Reconstruct the table
by slot, applying removals, or use the update row's table-size field. Corrected
source-capture combat counts were 44-58 table entries and 1-7 enemy entries.
Neither count implies the number visible on screen. Earlier delta-row counts
must not be used as entity-load evidence.

#### Source research and implementation

[N64ModernRuntime timer.cpp](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/ultramodern/src/timer.cpp)
derives guest Count from elapsed host time in hardware timer units; its
[VI event scheduling](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/ultramodern/src/events.cpp)
separates elapsed time from the cost of executing guest instructions.
[Perfect Dark's timing code](https://github.com/perfect-dark-pc-port/perfect_dark/blob/32a1cb9f268dd3ac73016801025c6bbbfa20130f/src/game/timing.c)
also carries elapsed-time fractions rather than treating each rendered frame as
an arbitrary amount of simulation time.
[Fix Your Timestep](https://gafferongames.com/post/fix_your_timestep/)
explains why bounded simulation work and overload behavior must be considered
when decoupling timing. These are architectural references, not drop-in JFG code.
The pinned upstream review/catalog was also consulted; no dependency pin changed.

The independently implemented `HostCountClock` uses steady elapsed time at
46.875 MHz (3 Count ticks per 64 ns), preserves fractional time, handles the
32-bit guest wrap through a 64-bit clock, and excludes explicit frontend pauses.
It rejects unrepresentable waits/overflow. The native execution candidate keeps
the existing guest OS, device deadlines, interrupts and game simulation. It skips
modeled instruction/cache charges and waits for actual device deadlines when idle.
Instruction safepoints bound timer polling; hardware/exception boundaries retain
the full handler. It does not apply a global game-speed multiplier, skip actors,
change weapons or hide frame drops with camera smoothing.

The original instruction clock remains available for deterministic reference
qualification. The default runtime and the owner's installed preview continue
using it until the replacement meets the release requirements below.

#### Qualification and remaining work

Clock tests cover conversion, fractions, monotonicity, pause/resume, overflow,
long sessions and deadline rounding (148 checks). Recorded-time replay tests
cover held controls, one-time mouse displacement, short press/release edges,
conflicting menu selections and overflow (18 checks). A movement fixture measured
identical forward, backward and both strafe speeds under both clocks (243.04 world
units/second in that fixture). Aim stayed active while firing, with five shots
consumed in each run. A native Save test completed through the guest menu and
changed only its isolated save copy; live profile/save files were not touched.

A poll-index recording cannot directly compare clocks that alter update cadence:
its real-time input durations change. Absolute-VI replay can also miss startup
menus that become ready at different times. The diagnostic replay adapter keeps
held controls and conserves mouse displacement; it preserves short button edges.
Starting at the first gameplay pose fixed startup correspondence but still did
not reproduce the entire open-loop route. Those mismatched runs are not evidence
that Goldwood combat was repaired.

Two tighter experiments are retained: a Goldwood checkpoint produced through the
native Save pipeline, and a bounded clock-policy comparison after an identical
original-clock prefix. The latter fingerprints all RDRAM and records Count/VI/input
poll before changing policy. It is disallowed during live play. Input timing is
anchored at that boundary, and both runs retain normal game AI and simulation.
Results and acceptance are recorded below without treating a completed process
as a passed performance test.

Production acceptance still requires correct startup/cutscene/audio timing,
overload/stall recovery, Save/reload, complete input and focus behavior, and live
verification of the same Goldwood fight and bridge. The large collision-driven
camera pull-ins and unqualified stationary pivot animations are separate open
items; neither is resolved by changing the execution clock.

- **Default-clock regression:** final RDRAM identical to the earlier reference: True.

- **Save reload:** saved 95-round pistol state reloaded: True.

- **Matched combat boundary:** identical initial RDRAM, Count, VI and poll index: True.
  Reference clock, seconds 1-21 after the boundary: median 67.88 ms, p95 97.78 ms; enemy table median 7, range 7-8; levels {'21': 267}.
  Native clock, seconds 1-21 after the boundary: median 49.89 ms, p95 51.82 ms; enemy table median 7.0, range 6-8; levels {'21': 430}.

Private evidence: `combat-critical-evidence-20261011` within the existing guarded
`pc-input-aiming-20261008` investigation. No ROM, generated game source, or save
data is included in public documentation.


The ongoing bug/attempt/resolution record is [PERF-001](../development/issues/goldwood-combat-slowdown.md).
It includes failed replay comparisons, partial timing improvements and future
60+ FPS compatibility requirements. Future issues follow the persistent
[issue documentation policy](../development/issues/README.md).


The contiguous native-timing candidate has completed hidden correctness and
performance qualification; see [PERF-001](../development/issues/goldwood-combat-slowdown.md#contiguous-candidate-qualification-and-handoff)
for exact measurements, installation status, retained failures and owner acceptance.
It does not resolve remaining slow updates or provide 60+ Hz rendering.


### Modern keyboard Confirm default, 2026-10-11

At the owner's request, new/reset modern profiles use **E** for menu Confirm,
sharing E with gameplay Interact in separate contexts. Existing custom bindings
remain loadable and are not globally migrated. The owner's preview profile will
receive the requested change during delivery, with all other preferences kept.
Implementation is pending build/test qualification and preview installation.


### E menu-confirm delivery (2026-10-11 UTC)

Modern keyboard/mouse menu Confirm now defaults to **E**, sharing E with Interact
in its separate gameplay context. Custom saved bindings (including an explicitly
chosen Enter) still round-trip, and original Controller Mapping keeps its own
bindings. The owner's preview profile was changed from the former Enter default
to E while closed; its save, launcher settings and other profile files were
verified unchanged. The existing desktop shortcut still targets the same v1.1
launcher/package directory.

Verification: 126 modern-controls checks and 546 native RmlUi checks passed.
The rebuilt launcher and runtime were installed and their package hashes checked.
Runtime SHA-256: `234fdcc4c5d7e207a11c566007eb3c75637587003cfd2e714e2bf53169aeb6ff`.
The accompanying [audio investigation](../development/issues/intermittent-music-crackle.md)
remains open; this delivery does not claim a crackle fix.


### 2026-10-13: native audio sample readiness preview

The native-timing preview now satisfies the synthesis sample callback's readiness
contract by loading from the resident ROM before publishing each cache entry.
This removes the stale-sample failure reproduced under faster native execution;
it does not change controls, music gain, DSP mixing, or global PI delays. The
existing v1.1 Modern Controls Preview shortcut selects the tested local runtime,
with ROM selection, saves and mappings preserved. Source checks and hidden
normal-Play integration pass; owner listening/live verification is still pending.
See [AUDIO-001](../development/issues/intermittent-music-crackle.md) for the causal
evidence, exact validation and implications for future 60+ FPS work.

### 2026-10-13: Native entry facing, jump momentum and focus continuity

The local v1.1 preview now seeds modern entry facing from the native player, preserves full takeoff momentum and original air steering, and keeps camera orientation when physical input is suspended. Tests reproduce the old 180-degree Goldwood entry error, cover all three characters and both timing paths, and retain byte-identical original-mapping replay state. The owner-accepted audio repair remains enabled.

See the [issue record](../development/issues/modern-controller-state-transitions.md) for causes, failed/inconclusive fixtures, qualification, refresh-rate implications and the remaining exact-route/physical-device live checks. These are local-preview results, not a public-release claim.

### 2026-10-13: modern camera collision response

The local v1.1 Modern Controls Preview now separates input orientation from
collision-constrained camera distance. Native-validated positions stay on the
chosen viewing ray, feet-only visibility no longer pulls the modern camera in,
and outward recovery is eased without filtering mouse/stick rotation.
Focused low-angle Goldwood tests reduce the maximum vertical camera step from
45.1 to 5.1 world units on the native-timing dual-stick route. Original mapping's
qualified replay remains byte-identical, and jump/focus regressions pass.

This is a local candidate awaiting owner live verification, including the
reported pond lip outside Goldwood spawn. A separate adjacent-obstacle replay
reached a hollow stump; it must not be presented as exact pond verification.
The native collision margin and queries are retained, large necessary pull-ins
can still occur, and this is not a new sphere-sweep collision engine.
See [CAMERA-005](../development/issues/camera-collision-bounce.md) for the cause,
rejected expensive attempt, timing caveat, actual validation and remaining work.

The preceding area-entry, jump/air-steering and focus-retention fixes were
owner-validated and committed as 7b1ea4ba6228c126757b2b2e966ed7c0e1de8ca4.


## 2026-10-13: pond false contacts and inward distance halving

The first camera candidate improved upward bounce but the owner still saw inward
jumps. Recovery of the final map identified the actual pond in Goldwood area 47;
a reconstructed route reproduced two 200-to-100 camera pull-ins above clear
geometry. Refining the distance search alone was insufficient. The current
preview additionally rejects height-only native corrections only when a finite
16-unit camera sphere and its focus ray are both proven clear. Unknown geometry
and potential contacts keep native handling.

The revised local package is build/modern-controls-camera-clearance-20261013
(runtime 15c40b95d95cf48a388f652419efe7bedd0f42f6d6b641059a1ff65498241613).
The reconstructed pond holds full distance; scripted mouse, low-angle, genuine
obstruction, portable control/audio suites and original Mapping full-memory
parity pass. Owner live verification and broad scenery coverage remain pending.

The ordinary preview had not recorded a full route. Desktop JFG Camera Capture
now explicitly enables the launcher's diagnostic recording and preserves initial
settings/save data under Desktop/JFG Camera Captures. It follows the updated
preview shortcut automatically. See the [CAMERA-005 follow-up](../development/issues/camera-collision-bounce.md)
for the full issue history, partial attempt, measured costs, install receipt,
recording limitation and future high-refresh/swept-volume requirements.


### Develop source integration, 2026-10-13

The completed controls, camera, native-timing and audio investigation changes,
regressions and issue records are integrated on local develop. This includes the
previously committed area-entry/jump/focus corrections. The owner has accepted
the audio repair and those three controls fixes; the latest pond-clearance
candidate still awaits live verification. This source integration is not a
public release, does not resolve the remaining PERF-001/CAMERA-002 scope, and
does not merge the separate high-refresh research branch.

Integration validation rebuilds eight focused portable suites from the isolated
develop worktree and runs the generated-entry, private-root generator and native
span regressions there. The previously recorded runtime replay and preview
qualification remain linked in the issue records. Original workspace files,
other pending edits and installed user profiles are preserved.


### 2026-10-13: Pause navigation, contextual action and jumping from aim

Modern keyboard movement bindings now navigate native menus, Q defaults to Back,
and E closes informational pages or finishes character selection. Direct pause
Options targets and Character arrows preserve native availability, transition
gates and selection sounds. Controller Mapping opens centered over its launcher.

Jumping from aim reuses each character's native jump handler and airborne state.
That first delivery resumed aim on landing; the follow-up below adds continuous
airborne ADS.
Native tests cover all three characters, sideways takeoff and held-button edges.
Original Controller Mapping's reference jump replay retains byte-identical final
RDRAM. See the [issue record](../development/issues/modern-pause-navigation.md)
for causes, rejected attempts, verification scope and remaining owner checks.


### 2026-10-13: Widescreen viewport and continuous airborne aim

The launcher render area now follows the original selected 4:3 or 16:9 display
mode, in windowed and fullscreen layouts. Transient cinematic VI cropping does
not resize that area. Mouse coordinates continue to follow the actual displayed
VI; pixel-coordinate replay fixtures now exercise wide and resized menu targets.

Modern aim is independent of airborne locomotion. The shared native aim/zoom
routine supplies look, while scoped adapters let camera, weapon, reticle and
fade consumers observe aiming. The original air state still owns gravity,
collision, momentum, steering, animation and landing. Continuing aim also retains
pitch through the native landing-to-aim transition. Original Controller Mapping
keeps the original restrictions. See the [issue record](../development/issues/widescreen-pointer-and-airborne-aim.md)
for test results and remaining live verification.
