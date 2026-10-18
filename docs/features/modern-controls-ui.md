# Modern Controls: implementation and RmlUi contract

The native controller window follows the owner's Idea 3 refined prototype
in RmlUi. It offers Modern Controls alongside the original Controller Mapping
editor. Selecting Modern Controls enables its experimental gameplay; selecting
Controller Mapping uses the original game inputs. Both layouts retain full rebinding. Runtime qualification and the local preview are
recorded in [PC controls](pc-controls.md).

Modern gameplay acceptance is supported-US single-player, player 1. Other
player tabs retain original mapping. Native DualSense input and gameplay feel
still require physical verification.

## Priorities and remaining capabilities

| Priority | Item | Status and acceptance |
| --- | --- | --- |
| Highest | Real settings and devices | Implemented: saved profiles, connected available device discovery, transactional writes, error feedback, capture lifecycle and preserved original mappings replace prototype data. |
| Highest | Independent scope sensitivity | Implemented: separate mouse/controller multipliers, 10-300%, default 100%. Applies to live sniper aiming before recording; automatic magnification scaling remains. Version 1 profiles load with 100% defaults. |
| Highest | Accurate response curves | Implemented: Linear, Quadratic and Cubic match the existing backend. Quadratic and Cubic reduce response near center while preserving full-deflection speed. |
| Highest | Optional zoom in/out | Implemented: Increase/Decrease magnification are editable alongside Cycle magnification. Scope-only commands are unbound by default and clamp at the limits. |
| Next | Native SDL/DualSense rumble | Worth adding after the current input handoff. Requires capability discovery, SDL output, shared strength/focus/disconnect shutdown rules and USB/Bluetooth hardware verification. XInput rumble already exists. |
| Later | Gyro aiming | Feasible, but requires sensors, calibration, drift management, sensitivity, activation modes and deliberate combination with stick/mouse input. Needs real-device tuning and replay/focus qualification. |
| Later | Adaptive triggers | Feasible as optional controller-specific effects, with weapon behavior and reliable shutdown. Lower priority than movement and aiming; requires a physical DualSense for acceptance. |

SDL provides [rumble](https://wiki.libsdl.org/SDL2/SDL_GameControllerRumble),
[sensor enabling](https://wiki.libsdl.org/SDL2/SDL_GameControllerSetSensorEnabled)
and [controller-specific effect packets](https://wiki.libsdl.org/SDL2/SDL_GameControllerSendEffect).
These establish implementation paths, not completed JFG features. A PlayStation
glyph override does not add hardware capabilities.

## Entry point and mode ownership

The existing separate controller window retains player tabs and device
assignment. Add/Change lists connected devices available to the selected player.
A disconnected saved assignment keeps its bindings and shows its status.

The explicit style selector writes ModernControlsConfig.enabled:

| Style | Behavior |
| --- | --- |
| Controller mapping | Existing original-game mapping and tuning. |
| Modern controls | Independent gameplay actions designed for Xbox, DualSense or mouse and keyboard. |

Modern Controls is the sole opt-in. The separate Experimental gameplay switch
has been removed from both UI and runtime. The old experimental-controls.ini
file is ignored and left intact for rollback; it can neither block Modern
Controls nor enable modern behavior in Controller Mapping. Switching style
clears pending look/capture and aim/zoom state and requires neutral controls.
Original controller and keyboard mappings keep working regardless of old switch
or legacy mouse-look/dual-stick preferences.

Changing tabs, glyphs or diagram view does not change the selected style.

Modern edits, style changes and device-specific resets do not rewrite original
controller.ini or pc-input.ini files or change the saved game's Normal/Expert
option. Original mappings are not automatically converted.

## Layout and device presentation

Buttons (Keys & buttons for keyboard), Sensitivity & behavior, and the
controller-only Advanced settings tab separate bindings from tuning. The default view has three grouped columns. Show controller
diagram / Show key map replaces them with a linked input list, the original
device illustration and a selected-input detail card. Each input row opens a
grouped action picker, including scope and menu actions. The redundant gameplay
selector, unbound-action summary and global Rebind an action link are removed.
The diagram still supports moving an assigned action to another physical input.

Xbox and DualSense silhouettes have separate native glyphs; unknown controllers
use generic positional names and artwork. XInput slots are labeled Xbox / XInput;
this identifies the API family, not a specific physical Xbox model. Automatic
includes the detected symbol family (for example Automatic · Xbox). The
Automatic / Xbox / PlayStation selector is a presentation preference. A translation layer may report a DualSense as XInput,
so the explicit preference remains useful. It does not change bindings.

Keyboard/mouse shows a key layout and mouse buttons/wheel, with current action
names. Custom keys remain accessible through the input list. Controller-only
settings are hidden. Diagrams and instructions read saved bindings, including
unbound and shared inputs. The diagram and list share selection; Hide diagram
returns to the grouped overview. Side buttons appear in the mouse group when
bound and can always be selected from the diagram or captured. Extra keyboard
keys appear in the right-hand Keys list for every action, including menu and movement actions. Mouse and the W/A/S/D movement list remain at the left. Confirm, Back and Save also belong to Keys; there is no separate Menus & dialogue card. Shared inputs list every assigned action (for example E: Interact + Confirm). Selecting an assigned action in the picker removes only that assignment; clearing a physical input removes all its listed actions. Arrow keys are excluded from Modern Controls defaults, saved-profile loading and new bindings; movement keys navigate menus.

The native layout uses the launcher's dark theme and amber focus. It supports
scrolling, narrow windows and display scaling. Keyboard Tab navigation is trapped
inside pending dialogs. Controller D-pad moves focus; left/right changes a
focused slider or selector; A/Cross activates and B/Circle cancels or returns.
Capture retains exclusive ownership while listening.

## Prototype fidelity contract (2026-10-10 revision)

The supplied HTML is the visual reference, including its 1280 x 920 artboard,
68-pixel player/device row, radio-card style selector,
46-pixel tabs and 60-pixel footer. The native implementation now uses those
metrics, the three-column grouping, compact settings cards, action descriptions,
selected-input detail strip, original physical glyphs and exact selection paths.
The redundant inner Controls title bar is removed after owner live review;
the ordinary OS window title bar remains. The controller window initially requests this size and clamps it to the monitor
work area. Smaller windows and display scaling retain scrolling access.

Modern mode's Change device opens the prototype-style one-click list populated
from the existing device catalog. Unplugged devices and assignments owned by
other players are excluded; selection is revalidated before saving. Original
Controller Mapping retains its established device setup flow.

Intentional differences from the mockup are functional: real saved percentages
and capabilities, separate movement/look deadzones, truthful curve names, editable
zoom actions, explicit experimental opt-in, advanced vertical/threshold tuning
and original mapping support. Advanced settings is a top tab. Camera and Sticks
share one independent column; Aim & Scope and Vibration share the other.
The footer contains a bordered reset button and centered Done button. It has
no persistent Saved label, profile annotation or Turn off experiments link. The zoom
illustration is instructional, not a live game-state indicator. Scope controls
are binding entry points; cycle and separate increase/decrease may coexist.
The native font renderer and responsive layouts may produce small differences
from the browser reference. No simulated input buttons are shipped.

## Default action layout

| Action | Xbox | DualSense | Keyboard/mouse |
| --- | --- | --- | --- |
| Move / strafe | Left stick | Left stick | WASD |
| Look | Right stick | Right stick | Mouse |
| Aim | LT | L2 | Right mouse |
| Fire | RT | R2 | Left mouse |
| Jump | A | Cross | Space |
| Crouch | B | Circle | Ctrl |
| Interact | X | Square | E |
| Previous weapon | LB | L1 | Wheel down |
| Next weapon | RB | R1 | Wheel up |
| Cycle magnification | RS click | R3 | Middle mouse |
| Increase / Decrease magnification | Unbound | Unbound | Unbound |
| Pause | Menu | Options | Tab |
| Menu select / contextual action | A | Cross | E |
| Menu back | B | Circle | Q; right-click |
| Menu navigation | Left stick / D-pad | Left stick / D-pad | Movement keys (WASD by default) |
| Full keyboard movement | N/A | N/A | Shift |

The action layer handles Normal/Expert internally. Modern Controls does not
expose N64 button names or require the user to select the guest control scheme.
Move/Look are a stick-pair setting on controllers, not extra digital actions.
Keyboard movement bindings also navigate menus; explicit directional menu
bindings take precedence on their axis. Their shared context is included in
binding-conflict checks. E dismisses information pages and finishes character
selection; Q and right-click provide Back.

## Aim and sniper magnification

Aim and Change magnification are separate actions. Aim supports Hold or Toggle.
Cycle magnification advances once per press through the game's internal factors
1, 3 and 5, then wraps. These are not claims about optical magnification.
Increase/Decrease uses the same factors, clamps at the endpoints and does not
repeat while held.

The scope sensitivity slider multiplies the selected device's existing aiming
speed. At 100%, existing aim tuning is unchanged. Magnification still slows
aim automatically. This multiplier applies only to qualified live sniper aiming,
including aiming during jumps; it does not scale ordinary camera movement or swimming. Fractional
motion is retained for fine adjustment and cleared when the context changes.

Jump and crouch do not double as zoom actions. Jump can start from aim using
the character's native airborne movement. Held/toggled aim, sights and sniper
magnification remain active throughout the jump and landing. Crouching still
requires leaving aim. Scoped preview and Settings describe this behavior; their preview
content is instructional, not a live HUD.

## Rebinding, persistence and recovery

Capture waits for held inputs to be released, then listens for up to 12 seconds.
It consumes the initiating press. Escape or the explicit Cancel control cancels
without binding the cancellation input. F11 remains reserved. Focus loss,
device loss or timeout preserves the saved binding. Mouse side buttons and
wheel directions are supported.

Conflicts are context-aware: Jump and Menu confirm can share A/Cross. Overlapping
gameplay actions prompt Replace / Keep both / Cancel. Swap appears only for one
conflicting action when the previous binding can be exchanged safely. Replace
clears only overlapping actions for the selected device. Unbind is explicit.

Each edit copies the current config, validates it, atomically replaces the
profile file and only then publishes the new model. Save failure retains the
previous active settings and offers Retry. A failed slider edit restores its
saved displayed value. Capture/dialog completion restores usable focus.

Restore modern defaults on Buttons asks for confirmation and resets the selected
input type's modern bindings/tuning. Reset sensitivity & behavior on the tuning
tab preserves button bindings and stick roles. Both reset the shared aim behavior
to Hold; the confirmation explains this. They preserve enabled style, glyph
preference, device assignment, the other input type's bindings/sensitivity and
all original maps.
An unreadable modern profile is not silently overwritten: explicit recovery
first preserves a separate unreadable-file backup.

## Settings model

100 means default sensitivity, not degrees per second.

| Device | Label | Config field | Values / default |
| --- | --- | --- | --- |
| Both | Aim behavior | aim_toggle | Hold=0, Toggle=1; default 0 |
| Controller | Move / Look sticks | movement_stick | Left moves=0, Right moves=1; default 0 |
| Controller | Movement deadzone | movement_deadzone | 0..30000; default 7849 |
| Controller | Camera sensitivity | look.camera_sensitivity | 10..300; default 100 |
| Controller | Aim sensitivity | look.aim_sensitivity | 10..300; default 100 |
| Controller | Scope sensitivity | pad_scope_sensitivity | 10..300; default 100 |
| Controller | Look deadzone | look.aim_deadzone | 0..30000; default 7849 |
| Controller | Response curve | look.aim_curve | Linear=0, Quadratic=1, Cubic=2; default 0 |
| Controller | Vertical sensitivity | look.stick_vertical_sensitivity | 10..300; default 100 |
| Controller | Invert X / Y | look.aim_invert_x / look.aim_invert_y | 0 or 1; default 0 |
| Controller | Button axis threshold | axis_threshold | 1000..32000; default 8689 |
| Controller | Trigger threshold | trigger_threshold | 1000..32000; default 3855 |
| XInput only | Vibration strength | look.rumble_strength | 0..100; default 0 |
| Mouse/keyboard | Camera sensitivity | look.mouse_sensitivity | 10..500; default 100 |
| Mouse/keyboard | Aim sensitivity | look.mouse_aim_sensitivity | 10..500; default 100 |
| Mouse/keyboard | Scope sensitivity | mouse_scope_sensitivity | 10..300; default 100 |
| Mouse/keyboard | Vertical sensitivity | look.mouse_vertical_sensitivity | 10..300; default 100 |
| Mouse/keyboard | Invert X / Y | look.mouse_invert_x / look.mouse_invert_y | 0 or 1; current defaults X=0, Y=1; saved choices retained |

Deadzone/threshold values use a normalized 32767 axis scale; the UI displays
percentages but persists the underlying scale. Vertical tuning is shared between
camera and aim within each device type. Aim behavior and enabled style are shared
by the player's modern profile; bindings and sensitivity remain device-specific.

## Native integration and profile contract

[modern_controls.hpp](../../include/jfg/runtime/modern_controls.hpp) supplies
ModernControlsConfig, kModernActions, conflict/rebind helpers, glyph resolution,
strict parsing/serialization, action sampling and scope precision helpers.
The UI does not translate N64 buttons.

[modern_controls_ui.inl](../../launcher/ui/modern_controls_ui.inl), included
inside Ui in native_ui.cpp, handles native events and persistence.
[modern_controls_presentation.inl](../../launcher/ui/modern_controls_presentation.inl)
contains the prototype layout and device-specific presentation. These use
the existing device catalog and atomic write helper.
[modern-controls.rcss](../../launcher/ui/assets/modern-controls.rcss) supplies
styles; main.rml hosts the shared dialog and modern overlay. No browser or React
runtime is shipped.

Profiles are modern-controls.ini and modern-controls-2.ini through
modern-controls-4.ini in the existing game profile. Missing profiles default to
disabled. Version 2 requires both scope fields. Exact version 1 remains readable
and supplies 100% defaults; malformed, unknown, missing or duplicate required
fields are rejected without partially changing the active model.
The look subrecord reuses PcInputConfig serialization; legacy keys are not
displayed as modern actions.

The runtime reloads settings and requires released inputs after relevant changes.
Scope sensitivity is applied before input recording. Replay v5 stores the
effective look deltas and action commands, so playback skips the live scope
multiplier and does not depend on the current profile's sensitivity.

| Native data-action | Purpose |
| --- | --- |
| mc-style-mapping / mc-style-modern | Save the chosen style. |
| mc-experimental | Explicitly toggle profile-wide experimental gameplay. |
| mc-tab-buttons / mc-tab-settings / mc-tab-advanced | Select content tab. |
| mc-view-list / mc-view-diagram | Select binding presentation. |
| mc-select-code / mc-input-code | Select an input / open its action picker. |
| mc-capture-N / mc-palette-N / mc-add-key | Capture an action binding / a new physical input. |
| mc-scope-separate | Open scope-only capture choices. |
| mc-free-code | Clear this input in the previewed gameplay contexts, preserving menu reuse. |
| mc-device-pick-code | Revalidate and assign an available connected device. |
| mc-assign-N / mc-clear-N | Assign / unbind an action. |
| mc-conflict-replace / mc-conflict-both / mc-conflict-swap | Resolve a conflict. |
| mc-cancel | Cancel pending capture or dialog. |
| mc-reset / mc-reset-confirm | Confirm selected-device defaults. |
| mc-tune-field | Validate and save a slider setting. |
| mc-glyph-select | Automatic=0, Xbox=1, PlayStation=2. |
| mc-save-retry / mc-repair-confirm | Retry a failed write / recover an unreadable profile. |

N in action binding events is a checked index into kModernActions. Stable digital
IDs are aim, fire, jump, crouch, interact, weapon_previous, weapon_next,
scope_cycle, scope_increase, scope_decrease, pause, menu_confirm, menu_back,
move_forward, move_back, move_left, move_right, move_fast, menu_up, menu_down,
menu_left and menu_right.

Presentation state (selected input/context, scroll, capture, pending conflict and
save feedback) belongs to the UI, not gameplay configuration.

## Assets and qualification

Owner-provided illustrations are adapted in
[controls-art](../../launcher/ui/assets/controls-art/README.md), with editable
SVG sources and PNG previews. Uncompressed TGA is embedded for the current DX11
renderer. The original physical-button glyphs use a native atlas. The exact
device selection paths remain editable SVG vectors and are displayed through
a second atlas. Supersampled radio/reticle details and nine-patch selection
frames keep thin circles and rounded edges continuous at fractional DPI; the
DX11 UI backend uses four MSAA samples. Gameplay action labels, keycaps and hit targets remain native
RmlUi elements; no gameplay labels are baked into the artwork.

The optional JFG_UI_BUILD_TESTS CMake option builds jfg_modern_ui_tests. Pass a
new evidence-output directory to the executable. It runs in an offscreen,
nonactivating window with injected device/focus callbacks; it never sends desktop
input. The fixture covers persistence, independent
profiles, original mappings, conflicts, capture/cancel, save failure/retry,
invalid-profile recovery, device labels, controller navigation and scaled layouts.
The fidelity revision also verifies popup focus containment, available-device
filtering, separate tuning/layout resets, preserved menu reuse, scroll retention,
unique IDs and real RmlUi pointer hits on controller, trigger, mouse and keycap
targets. Owner-feedback checks cover repeated selector clicks without exceptions
or closing the window, independent card stacking, centered Done text, removed
chrome and the Advanced tab. Both 125% and 150% layouts have captured evidence.
Pointer tests call the isolated RmlUi context, not desktop input APIs.
The portable modern-controls suite passes 101 checks, including profile migration
and scope precision/context boundaries. Current gameplay evidence is in
[PC controls](pc-controls.md).

Physical Xbox/DualSense, mouse focus/reconnect, rumble feel, and broad campaign
camera behavior still require owner playtesting. Simulated success is not a
claim that these have been physically verified.

## Research basis

[Halo Infinite settings](https://support.halowaypoint.com/hc/en-us/articles/4407649252116-Guide-to-Halo-Infinite-Game-Settings)
separates Zoom and Zoom Level actions, supports a hold setting and changes the
controller diagram with the layout.
[Gears 5 accessibility](https://www.gearsofwar.com/en-gb/games/gears-5/accessibility/)
provides an aim-toggle alternative. This project's exact default bindings remain
its own design. Broader port and emulator references are recorded in
[PC controls](pc-controls.md) and the pinned upstream catalog.


The removed Gameplay selector and the symbol selector previously shared prefixes
with obsolete numeric button actions. Clicking either could parse "select" as an
integer and throw. Those handlers are removed. Symbol changes queue a form
refresh for the next frame, so the active select event completes before its
elements are replaced. Repeated real RmlUi pointer clicks are covered by the
isolated fixture.

## Original mapping presentation and latest owner corrections

Controller Mapping now shares the native prototype chrome: Buttons (Keys &
buttons for keyboard), Sensitivity & behavior, and a controller-only Advanced
settings tab. Grouped mapping cards have padding, physical input glyphs,
device-appropriate names, per-binding capture/clear and a compact preset toolbar.
Movement selection retains the existing Left stick / Right stick options;
the prototype's D-pad movement option is not offered as an unsupported setting.
Normal/Expert presets affect only original mappings; neither enables modern
camera behavior. Original deadzone/inversion settings and binding thresholds
are separated by tab. Resetting tuning preserves bindings.

The optional original-controller diagram highlights corresponding mapped rows.
The footer has a real restore/reset button and centered Done; persistent Saved
text and the obsolete experiment links are absent in both modes. Capture
instructions and actual save errors remain visible. The device header uses a
compact connected pill, Change device with chevron, and Remove; reported device
names/family labels are retained rather than inventing a hardware model.
Symbol labels/dropdowns and the square action-picker close button are centered.

JFG_UI_BUILD_TESTS also builds jfg_controller_mapping_ui_tests. Like the modern
fixture, it takes a new output directory and uses an offscreen, nonactivating
window with injected devices. It verifies original keyboard/mouse/gamepad
capture, saved settings, available-device filtering, reconnection, mode
preservation and layout at the owner's 125% scale. Both fixtures measure the
visible text children, preventing RmlUi flex labels from silently collapsing.


Retained capture profiles may keep older menu bindings despite a source merge. [INPUT-008](../development/issues/modern-capture-profile-defaults.md) records the current owner report and preference/default distinction.


The initial Menus & dialogue card fixed hidden saved bindings but was superseded at the owner's request. Confirm, Back, Save and new keyboard inputs now appear in Keys, while Mouse/WASD remain at the left. Shared E shows Interact + Confirm and persists both actions. Modern arrow bindings are retired. The revised native UI suite passes 772 checks, and 155 core checks cover migration and context behavior. The local preview is updated; see [INPUT-008](../development/issues/modern-capture-profile-defaults.md) and [qualification](../../evidence/unified-modern-keys.json).
