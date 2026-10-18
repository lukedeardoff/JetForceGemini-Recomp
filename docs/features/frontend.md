# Unified frontend and live inventory

The native launcher owns one window for setup, home and gameplay, plus
separate live-tool windows. This page records its architecture, design rules,
the live inventory and its locally derived artwork, and remaining
qualification. User instructions are in the [launcher guide](../development/launcher.md);
the live map and navigation are in [their own page](navigation-mod.md).

## Architecture

- **Presentation.** C++ with [RmlUi](../legal/dependency-license-inventory.md#launcher-release-dependencies),
  embedded RML/RCSS and fonts, rendered with Direct3D. DirectComposition places
  the interface above the hosted game window. The standalone design harness
  remains available for review.
- **Game session.** The game runs in an isolated, supervised process that
  creates its render child inside the frontend viewport. Stop saves and
  returns home; a crashed session returns home with support reports available.
  The game build must speak the frontend protocol; older builds get a rebuild
  message.
- **Services.** An embedded managed helper, extracted locally, provides ROM
  verification, setup/download/build, support sessions, ZIP export, freeze
  capture and the map/inventory/navigation models as background services.
  Setup shows actual status only: no invented percentages, ETAs, device
  identities or "up to date" claims.
- **Pause and input capture.** Modal dialogs request a cooperative VI-boundary
  pause that stops gameplay and queued audio; the PAUSED badge requires an
  acknowledgment from the game process. Dropdowns capture input but leave the
  game running. **Pause when inactive** uses the same mechanism. Held input is
  cleared when settings close or focus is lost.
- **Controller ports.** Four independently latched ports with SI status/read
  delivery, atomic per-port profile files, live reload, explicit disconnection
  and sticky automatic XInput assignment that survives disconnects within a
  session.
- **Display.** Borderless fullscreen (F11) restores prior placement. Window mode
  is independent of the game's own widescreen option; aspect follows the game.
  Video Settings is an owned native Windows window rendered with RmlUi, with window controls,
  VSync and experimental Original/Custom/Unlimited rendering. Rendering changes
  apply on the next game launch. Expanded aspect and AA overrides remain absent.
- **Live tools.** Map and inventory open separate native RmlUi windows with the
  launcher's renderer, fonts and per-monitor DPI handling. `NativeLiveTools.cs`
  adapts the existing validated map/inventory models and navigation planner;
  a per-window transfer directory carries bounded, atomically replaced snapshots
  and commands. The session PID and creation time gate all live state, so a
  stopped process cannot resurrect old data. The WinForms classes remain only as
  compatibility models and regression fixtures.
- **Telemetry.** Normal launches export read-only telemetry through
  `JFG_LIVE_OUTPUT` into a fresh directory inside the save profile; observer
  tests compare guest memory before and after export. The navigation mod keeps
  its separate profile and explicit gate for health writes, enemy removal and
  synthetic input.

## Design rules

The design follows eight owner-provided references (launcher, in-game menu,
child window, controller mapping, video, support report, setup/build, audio)
plus live map and inventory references, all from 2026-10-08. They are visual
references only; no browser or JavaScript is embedded.

- Navy backgrounds, amber actions and focus, cyan device indicators,
  Barlow/Chakra Petch typography, keyboard and controller focus states.
- Home asks only for the required **Game ROM**; Setup owns the generated build,
  its install location and rebuild action. Play guides users through setup when
  no compatible runtime is ready.
- Six top-level menus (Game, Controllers, Video, Audio, Tools, Help) with
  compact dropdowns, F10 and Alt+G/C/V/A/T/H access, arrow/Enter navigation and
  outside-click dismissal. Tab stays inside an open panel; Escape cancels
  binding capture before closing a panel.
- Controller notifications appear at the bottom on Play, first activation,
  connection and disconnection, naming the real device and player. About 400 ms
  entry, 4 s hold and 220 ms exit; events are coalesced; reduced motion uses a
  fade; notifications never take focus or cover settings controls.
- The home scene draws vector orbits and, optionally, ship flybys built from
  ROM models 356-358 (geometry, UVs, textures and joint hierarchy) extracted
  after verification for each launcher process. Reduced motion freezes the scene.

## Live inventory

Juno, Vela and Lupus each have a view; **Follow** tracks the active character.
The grid shows 15 weapons, 15 keys and quest items, and 12 shared ship parts,
with owned tiles bright and missing tiles dim. Unknown or stale telemetry
clears ownership and shows NO GAME RUNNING or WAITING FOR LIVE DATA, never a
saved snapshot.

**Identifiers** follow the bit positions documented by
[Jimmie1717's jfg-z common data](https://github.com/Jimmie1717/jfg-z/blob/64b9f17f1f9194d868a87970897688626de304e6/JetForceGemini/data/common.lua),
with the US [offset definitions](https://github.com/Jimmie1717/jfg-z/blob/64b9f17f1f9194d868a87970897688626de304e6/JetForceGemini/data/NJFE0.lua)
corroborating the character-record offsets (MIT; no Lua, graphics or ROM data
included). Ship-part order also agrees with the pinned decomp enum.

- Weapon IDs are low-to-high bits of the 16-bit ownership mask: 4 is Shocker,
  7 Sniper rifle, 10 Fish Food, and the fifteenth regular weapon is Cluster
  bombs. The unused Splitter bit is not shown.
- Character item IDs count high-to-low within each byte from record `+0x66`:
  the key mask covers IDs 0-15 and the objects at `+0x68` cover 16-31. Gold bars
  3, 2, 1 are IDs 23-25; 26 is Ear plugs; 27 is Arcade chip. Older 27-entry
  exports remain readable without claiming ID 27. Other unlabeled bits stay
  unmapped.
- Shared ship-part flags are IDs 32-43, a separate namespace.
- Item ID 10 (`keys.trirocket`, key-mask bit `0x0020` at `+0x66`) is an
  internal flag, not a pickup: the Tri-Rocket Launcher uses weapon bit `0x0020`
  at `+0x0A`, and an [independent item guide](https://www.raregamer.co.uk/games/jet-force-gemini-quick-reference-item-guide/)
  lists no separate key. It is excluded from the grid and completion count;
  its raw export is unchanged.

**Artwork** is derived on the user's PC from their verified ROM; no artwork is
embedded in source, launcher resources, releases or support reports.

- `InventoryImages.cs` calls `OpenVerifiedRom`: exact supported US 32 MiB
  big-endian `.z64` with SHA-1 verification, read-locked during extraction, no
  implicit byte-order conversion, verified before any cache hit.
- Weapon HUD images: weapon enum + 10 selects the frontend menu entry in
  section 26, then the sprite definition (table 22 / data 21) and texture
  (table 3 / data 2, or table 1 / data 0 for the high-bit bank). Textures use
  the US header (compression flag at byte `0x19`, pixels at `0x20`, five-byte
  raw-DEFLATE wrapper, RGBA and intensity/alpha formats, pre-swapped odd rows).
- Other pickups: object definitions (table 46 / data 47) reference models
  (table 38 / data 39); `InventoryModel.cs` renders static geometry into
  128-pixel thumbnails from a fixed view. Object IDs: character items 0, 1, 2,
  3, 9 = 118, 119, 120, 121, 127; 16, 17, 20, 21, 22 = 513, 508, 539, 509, 534;
  23-27 = 438, 437, 436, 586, 585; ship parts 0-11 = 610-615 and 617-622.
- Character plaques: texture pairs 35347/35348 (Vela), 35342/35343 (Juno) and
  35344/35345 (Lupus), joined horizontally and flipped vertically.
- 45 verified images in total (15 weapons, 15 items, 12 parts, 3 plaques). PNGs
  and a provenance manifest are cached atomically under
  `%LOCALAPPDATA%/JFGRecomp/asset-cache/<ROM SHA-1>/inventory-v5`; missing or
  corrupt files regenerate, and names remain if extraction fails. Asset ranges,
  decompression, dimensions and mesh indices are bounds-checked.

## Acceptance and remaining qualification

Automated, profile-isolated checks cover repeated Home-Play-Stop cycles,
startup cancellation and close, recovery from a terminated session, fullscreen
round trips, settings capture with pause acknowledgment, volume and mute,
Player 4 assignment, binding changes, four-port SI delivery and disconnect
isolation, map zoom anchoring and honest no-game states. A complete Windows game
build from the public checkout passed with these lifecycle tests.

Still to qualify by hand: two- and four-controller physical multiplayer,
fresh-PC first-run setup with UAC and reboot, and long sessions through
cutscenes. Hardware results must be reported separately from automated checks.

## Experimental high-refresh rendering

**Video > Video settings...** opens a separate native window rendered with RmlUi,
following the owner's refreshed 2026-10-11 Video design. It contains VSync,
Original/Custom/Unlimited modes, a 60-1000 FPS slider and exact numeric field,
window mode/size and Pause when inactive. Apply saves to the profile and closes; Cancel
discards edits. Rendering changes take effect next game
launch; the restart chip is absent when no game is running. Live probes verified
that FPS/VSync need a new game session, while display changes apply immediately.

The [experimental high-refresh mode](high-refresh-rendering.md) exceeds 360
presentations/s in a short replay with matching guest RAM, input, save and probe
state. Real-menu persistence and live pause/resume have automated coverage.
Unlimited pacing, broad visual acceptance and sustained high-rate performance
remain open; Original remains the default.