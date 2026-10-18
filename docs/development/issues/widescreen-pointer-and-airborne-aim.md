# Widescreen presentation / pointer alignment and continuous airborne aim

## Reports and status (2026-10-13)

**DISPLAY-001:** blocking owner report against the pause-navigation preview.
Enabling Options > Widescreen left black bars on all four sides of the wide
launcher and the owner reported misaligned mouse targets in the Options menu.

**INPUT-007:** complete the modern jump-from-aim behavior. The previous delivery
entered native airborne movement, dropped the aiming camera and resumed aim on
landing. The requested behavior keeps sights active throughout.

**Status:** verified in the targeted automated checks below and installed in the
local Modern Controls Preview on 2026-10-13. Owner live verification remains
pending. No public release is claimed.
The production guard remains enabled. The independent high-refresh worktree,
owner saves/bindings and unrelated README/screenshot edits are preserved.

## Cause and implementation

The launcher fitted its render child to 4:3 even after the game selected its
16:9 mode. RT64 then fitted the wide VI inside that child, causing nested
letterboxing. The runtime now publishes the selected display aspect to the
launcher, which uses it for windowed, resized and fullscreen layouts. Temporary
cinematic VI cropping does not resize the launcher canvas.

Mouse conversion already used the displayed VI aspect; there was no evidence
justifying arbitrary changes to icon coordinates. It continues to use the same
VI presentation helper as rendering. The fixed canvas removes the conflicting
4:3 container. Earlier pointer replays forced logical 4:3 coordinates, leaving
a test gap: bounded v2 fixtures now supply client pixels and window dimensions,
then use the actual native VI for conversion. Owner confirmation of the reported
physical mouse behavior remains required.

The stock game couples aim presentation to locomotion states 10/11, while jump
physics uses state 3. Modern aim intent now remains independent of airborne
locomotion. The adapter calls the shared native aim/zoom routine with its native
guest ABI and elapsed update time. It preserves the momentum heading while
updating the independent view and weapon directions.

Scoped adapters expose aiming only to the existing camera, weapon, reticle and
fade consumers. Gravity, collision, air steering, jump animation and landing
always see the real airborne state. Hip-fire target acquisition does not replace
manual airborne aim. The native transition into grounded aim also clears its
pitch, so continuing aim restores the existing view at that boundary. Releasing
Aim in midair returns to the ordinary camera and removes the aiming reticle.
This applies to held or toggled modern aim intent; original Controller Mapping
retains the original behavior.

## Attempts and informative failures

- The first airborne candidate failed closed at its first jump. Its new call
  passed zero-extended guest pointers; generated MIPS memory accesses require
  sign-extended 32-bit pointers. Symbolication identified the shared aim call.
  Correcting the call ABI eliminated the crash before any preview installation.
- The first successful jump replay exposed a pitch reset after landing.
  Restoring pitch only at the next player-update entry was insufficient: the
  physics boundary had already cleared the continuation flag. Restoring at the
  native landing-to-aim transition retains yaw and pitch through subsequent
  grounded updates.
- The hidden launcher harness initially lacked a complete portable package,
  then encountered a transient status-file sharing violation. Both were fixture
  setup failures, retained in the private command journal.
- An inactive Windows desktop cannot become the foreground input desktop.
  Foreground-input checks therefore remained unqualified; the harness never
  switched or sent input to the owner's desktop. Recorded input verifies real
  window geometry, while unit/native pixel fixtures verify mouse conversion.
- A recorded-poll stream has identical repeated samples during boot and cannot
  be supplied unchanged to the launcher's retrace-ordered replay interface.
  Collapsing duplicate polls still left sparse one-VI pulses which could be missed
  by interactive polling. The final layout smoke uses the original deliberate
  hold windows and the recorded semantic menu targets. It verifies layout, not
  exact recorded-poll replay.
- One release assertion used a fixed VI boundary instead of the next guest
  update. The trace shows the released host sample at VI 4161 and ordinary
  camera ownership at the next update, VI 4164. The corrected assertion checks
  that actual input-consumption boundary and preserves the failed receipt.

## Verification

Private evidence ID: **wide-ads-evidence-20261013**. ROM-derived code, saves,
RAM captures and rendered game images remain private.

- PC input: 355 checks. Menu pointer: 153 checks, including wide Options targets,
  letterboxing, ultrawide, odd sizes, DPI-sized client extents and resized pixel
  fixtures. Modern controls: 140 checks. Input replay regression executable passes.
- Native launcher graphics/viewport tests and C# launcher/setup tests pass.
  RmlUi modern-controls tests pass all 546 checks. Its aim guidance now describes
  continuous airborne sights.
- Native pixel replay enables widescreen through the game's Options page,
  selects separated icons directly, opens Music Volume, returns with Back and
  selects the correct icons after changing the supplied client dimensions.
  The selected native video mode and hover-sound events are recorded.
- Real paired launcher/runtime smoke validates the imported ROM and package,
  embedded runtime process identity, 16:9 render extent and largest-aspect fit
  in windowed, resized, fullscreen and restored-window layouts. Both processes
  close cleanly and the owner profile remains byte-for-byte unchanged.
- Juno, Vela and Lupus replays enter their native airborne state with aim active.
  Camera, weapon, reticle and fade consumers continue to receive aim. The final
  Juno landing case retains view and camera pitch through touchdown and the
  return to state 11. Sideways takeoff, independent look and air steering remain.
- Pistol and sniper firing consume ammunition while airborne. Sniper
  magnification remains active and can cycle during the jump. Releasing sniper
  aim before landing removes the reticle and returns to the ordinary camera;
  the captured frame is still in native airborne state.
- Original Controller Mapping's reference jump replay has byte-identical final
  RDRAM before and after these changes: SHA-256
  1243885c7b66a45c8778fbcea6cbcce9436018342ee8bf97a9516fe5b9b69c9e.
- All native scenarios use copied saves and verify the source save is unchanged.
  No physical controller/mouse feel, every weapon, or high-refresh parity claim
  follows from these fixtures.

## Owner live check and compatibility

The existing **JFG Modern Controls Preview** desktop shortcut now launches the
paired build in **build/modern-controls-wide-ads-20261013**. Saves and bindings
are unchanged; activation changed only the preview runtime path and shortcut.
Use this updated preview. Toggle Widescreen in
pause > Options, then hover/click the icons, resize and check fullscreen. Hold
right mouse or LT, jump, look and fire; sights should remain active through
landing without resetting the view angle. Repeat with sniper magnification,
and release Aim during a jump to leave the scope.

These changes reuse guest elapsed update time and native airborne physics.
They do not introduce a fixed 30-FPS movement step or change the separate
high-refresh worktree. Future timing changes still require their own replay
and live-camera qualification. Crouching still requires leaving aim.

Transferable lessons: fit the host render area and mouse conversion to the same
display contract; logical-coordinate tests cannot qualify pixel hitboxes.
Decouple view/action intent from locomotion instead of freezing a grounded state
during jumps. Check the landing boundary and the native guest call ABI, not
only takeoff or a visible crosshair.

Source: [runtime adapters](../../../src/boot/native_boot.cpp),
[launcher viewport](../../../launcher/native/game_viewport.hpp),
[pointer fixtures](../../../include/jfg/runtime/menu_pointer.hpp).
See [prior navigation work](modern-pause-navigation.md) and
[PC controls](../../features/pc-controls.md).

Installed runtime SHA-256: c84c20c3fa83528f590633b499bccc1a0faef40d20b8188fb693978a908588b7.
Launcher SHA-256: 72734f9013795d77b6e9b65111ecba7074d07dd6243aa0df4694c51df8095c90.

## High-refresh merge regression (2026-10-13)

Review of the develop integration discovered that the high-refresh video-state
query and develop's viewport-change notification both used `WM_APP + 24`.
The earlier query handler swallowed the runtime notification, leaving the
render child at its previous aspect until another layout trigger. Initial
non-live dialog checks passed because they did not exercise a live aspect change.
The candidate reserves a distinct video-state query message and adds a muted,
isolated-desktop integration regression for 4:3/16:9 relayout while Video Settings
is open. Baseline failure and corrected results follow before preview delivery.

The merged-before-fix launcher reproduced the missed live widescreen relayout.
After assigning the video query `WM_APP + 25`, the same live test passed with
zero failures, including aspect changes, video queries, Apply and pause/resume.
The 120-check RmlUi video suite also passes. The runtime binary is unchanged.
This corrects the demonstrated integration regression; physical owner acceptance
and the separate late-cave interpolation report remain pending.

The high-refresh message-conflict correction is included in merge `f70b513c896df107163f2d72abed344a87dc2517` and the locally delivered `high-refresh-develop-merged-20261013` package.
