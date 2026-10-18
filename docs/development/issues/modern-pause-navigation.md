# INPUT-004 / INPUT-006 / LAUNCHER-002: Modern pause navigation and aiming jump

Status: verified in focused tests and installed in the local preview, 2026-10-13;
owner live verification pending. No public release is claimed.

## Reports and reproduction

- Open Character from the pause screen. The visible left/right arrows do not
  accept mouse clicks; WASD does not navigate; E does not confirm and return.
- Open Health, Weapons, Map, Inventory, Tribals or Ship Parts. Back is
  undiscoverable, and informational screens do not dismiss with E.
- Open Options and its settings. Pointer targets are missing; A/D should
  adjust settings such as volume. Pointer selection should play the native
  selection sound once when the item changes.
- Open Controller Mapping from the launcher: the separate RmlUi window
  does not initially center over its owner.
- Modern Controls intentionally disallows jumping from the aiming state.
  The owner now requests jumping while holding aim, retaining the separate
  scope-magnification bindings.

Severity: usability blocker for keyboard/mouse pause navigation; movement
feature request for aiming jumps. Original Controller Mapping must retain
native semantics. Preserve remapping, saves, and the separate high-refresh work.

## Evidence and approach

The source discards keyboard movement axes in menu context. Modern defaults
map Back to Backspace, and native informational pages check B rather than A.
The existing pointer decoder recognizes only the main pause page; it excludes
every other pause-manager screen. The controller tool uses CW_USEDEFAULT
for initial placement.

Inspect native menu page/state transitions and selected-item side effects
before extending targets. Route actual native actions and settings changes,
rather than walking through intermediate selections or modifying saves.
Check gated transitions, disabled targets, held-input edges, replay compatibility
and the unmodified Controller Mapping path. Inspect the native aiming-to-jump
state machine before changing movement behavior.

Private evidence: pause-modern-evidence-20261013, guarded under the existing
pc-input-aiming-20261008 investigation. All previous charges retained.

## Attempts, validation and resolution

Pending investigation and implementation. Build success alone will not close
these reports. Record automated checks separately from owner live verification.

## Related records

- [Previous pause targets and Save](../../features/pc-controls.md#2026-10-11-goldwood-capture-camera-stepping-and-native-pause-save)
- [Directional airborne movement](modern-controller-state-transitions.md)

## Candidate progress, 2026-10-13

- Modern keyboard movement bindings now also navigate guest menus; explicit
  menu arrow bindings remain available. Opposing movement inputs cancel.
- Keyboard Back defaults to Q. Older default Backspace profiles migrate to Q
  only when it is free in the menu context; custom Back bindings are retained.
- Contextual Confirm dismisses informational pages and finishes character
  selection. A held opening press cannot immediately close the new page,
  and a held dismissal cannot reopen the parent.
- Pause decoding now distinguishes character selection, informational pages,
  Map, Options and nested Options. Selection targets preserve native disabled
  items and transition gates. Map retains its native travel confirmation.
- Pointer changes request the native selection sound on the guest menu thread.
  Native settings persistence and save operations remain owned by the game.
- The Controller Mapping tool window initially centers over its launcher
  owner, clamped to the selected monitor work area and using that owner's DPI.
- Portable checks pass for mapping migration/navigation and semantic button
  transitions. Native and launcher results are recorded below.

### Rejected candidate: Options selection field

The first native test opened Speaker Settings after clicking the Music icon.
The Options overlay has two different selection fields: its root icon index
and a nested confirmation choice. The initial decoder used the latter.
The candidate was not installed. Native instruction review identified the
root icon field; both pointer selection and replay validation now use it.
The regression test must assert the destination page, not merely a successful
click event or clean process exit.

## Implemented resolution

The runtime previously decoded only the pause root. Submenus therefore had no
pointer Back path, and character arrows and Options icons had no targets.
Modern keyboard movement was also explicitly cleared from menu axes. Removing
that restriction makes remapped movement directions work throughout the native
menu handlers; menu-direction bindings still take precedence per axis.

The qualified pause manager now identifies information pages, character
selection, Map, Options and nested settings. Character arrows emit one native
directional pulse per click. Root Options selection writes its actual selected
icon, then uses the game's activation path. It never walks through intermediate
icons. Right-click supplies native Back on these pages without needing a hit
rectangle. Native save/setting persistence is unchanged.

Contextual Confirm translates a fresh press to Back on informational pages,
character selection and ordinary Options adjustment. The held-press state
survives page transitions to prevent the opening press from dismissing its page
or a closing press from reopening the parent. Map and confirmation dialogs keep
their native Confirm actions. This applies to rebound Confirm, including E by
default, and to modern controller Confirm. Q defaults to Back. Version 5 migrates
an older default Backspace only if Q is not already a menu/movement binding.
Original Controller Mapping and its independent bindings remain intact.

Pointer selection queues sound 0x16B only when the selection changes. The game
thread invokes the game's own sound routine on its menu stack; input polling
does not call audio code or replay a sound for a stationary cursor.

For jumping from aim, the modern adapter invokes the character's complete
native ground-jump handler, masking only its Aim-first branch for that call.
Native eligibility, animation, vertical motion, collision and landing remain
owned by the game. The existing modern airborne adapter retains direction and
steering. Held/toggled Aim remains an input intent and resumes on landing.
Continuous scoped ADS while airborne is not implemented.

The RmlUi tool window uses its owner's DPI and rectangle for initial centering,
clamped to that monitor's usable work area. It does not move an already-open
window or change focus policy.

## Verification

Private evidence ID: pause-modern-evidence-20261013 in the existing guarded
pc-input-aiming-20261008 investigation. Saves used in experiments are copies;
source hashes remain unchanged. No physical desktop input was injected.

- 140 modern-controls checks, 107 menu-pointer checks and input-replay tests pass.
  Coverage includes Q migration/conflicts, remapped WASD, explicit direction
  precedence, disabled targets, page transitions, contextual E, held press
  suppression and strict old/new replay schemas.
- Offscreen RmlUi modern-controls tests pass 546 checks. The original mapping
  suite also passes device filtering/reconnect, capture, player isolation and
  minimum-size layout.
- Native pointer sweep opens and returns from Health, Weapons, Inventory,
  Tribals, Map and pause Options. Character arrows record -80/+80 native axes.
  Root and Options hover changes execute the native selection sound.
- Clicking Music opens native page 2. Leftward menu input lowers the actual
  native music-volume field to 0; left followed by right restores 256. The
  keyboard mapper separately proves A/D supplies these same axes.
- Native aim-jump replays identify Juno, Vela and Lupus from actual guest
  character IDs. Each rises off the ground, enters native airborne movement
  and resumes aiming on landing. Juno mouse and dual-stick cases pass; the
  latter retains sideways travel and produces one takeoff while Jump is held.
- A before/after original-mapping jump replay has byte-identical final RDRAM.
- Actual hidden Controller Mapping windows center over a hidden owner and
  clamp at a monitor edge; foreground focus remains unchanged.

### Test-harness corrections

The initial old-binary comparison received a newer profile version. It is not
valid evidence of modern aiming behavior and was excluded from the aim-jump
claim. Missing artifact arguments caused an initial UI-test invocation to exit
before testing; corrected invocations pass. The volume assertion initially
decoded a logical big-endian dump as live word-swapped memory; correcting the
reader proves the native value changes. A launcher smoke reached rendered
play, then its harness hit a transient Windows sharing lock reading shutdown
status. Retrying that read in the bounded polling loop produced a complete pass;
no product change was needed. These failed receipts remain retained.

A stale generated-source fingerprint required regeneration. All generated
executable sources were identical after normalizing line endings; only
regenerated metadata was refreshed. An interrupted build left eight empty
object files. Removing only those disposable objects and rebuilding restored a
clean build. No normalizer checks or production guard protections were disabled.

## Remaining live verification

1. In Modern KBM, open every available pause item. E should close Health,
   Weapons, Inventory, Tribals and Ship Parts; Q/right-click should go Back.
   Map keeps E for travel. The fixture did not have Ship Parts unlocked, so
   it shares the tested informational-page path but still needs an unlocked
   native/live check.
2. In Character selection, click both arrows or use A/D. E or the central
   label should accept and return. Confirm the desired character visually.
3. In pause Options, hover and click each enabled icon. Listen for one selection
   sound per change. Adjust volume with A/D, then use E or Q to return.
4. With mouse and XInput, hold/toggle Aim and press Jump. Check takeoff direction,
   animation, landing, and return to aim. Repeat with sniper magnification.
   This is jump-from-aim, not continuous scoped airborne aiming.
5. Open Controllers from the launcher on the usual monitor/DPI and verify its
   initial position.

Nested Options still uses directional keyboard/controller input for adjustment;
this change does not claim a mouse slider/drag interface or complete pointer
coverage in every game menu.

## Local delivery

The existing **JFG Modern Controls Preview** desktop shortcut now uses the
rebuilt v1.1 launcher and runtime in the local modern-controls-pause-navigation-20261013
package. Its original profile, saves, ROM selection and bindings are preserved.
Only the runtime path files and desktop shortcut target changed; rollback copies
are retained privately.

The final pair passes a normal Play launch on a separate, never-displayed
Windows desktop: the launcher starts the intended embedded runtime, reaches
rendered gameplay, and both game and launcher exit cleanly. The source preview
profile remains unchanged by that test.

Runtime SHA-256: e20a0258c493eed0867e19733ef046704f5acd42dbc352ea490a567f79fcf24b

Launcher SHA-256: a73f93ad865e354cf876adad1b34797aa247cb8ab3e06b4ccb4bbe0df2f4d52c

Follow-up: [continuous airborne aim and widescreen presentation](widescreen-pointer-and-airborne-aim.md) supersedes the initial airborne ADS limitation. Its distinct test and live-verification status is recorded there.
