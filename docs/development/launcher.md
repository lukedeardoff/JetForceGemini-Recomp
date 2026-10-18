# JFG Launcher guide

The Windows launcher imports your supported North American ROM once and runs
the bundled game in one window with in-game menus, controller settings, live
tools and support reports. Start with
[getting started](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/getting-started.md)
for downloads and check [known issues](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/known-issues.md)
before installing.

## First setup

1. Extract the complete release ZIP and open `JFG-Launcher.exe`.
2. Select your supported US big-endian `.z64` ROM. The launcher verifies and
   imports a private copy into `roms/jfg-us.z64` under your profile.
3. Click **Play**. The ROM card disappears after import; use **Game > Select
   ROM...** to replace the imported copy later.

The original selected file is no longer required after import. The launcher
checks size, byte order and SHA-1 (`493ced9008dbe932d6e91179b68e8630cf23a023`)
locally; it never downloads a ROM or uploads it. The complete release contains
matching runtime libraries and requires compatible hardware graphics on
Windows 10 or 11 x64. Players do not need development tools. **Game > Verify
game files** validates the bundled runtime and imported ROM.

Developers can still build from source; see [ROM build setup](rom-bootstrap.md)
and [release packaging](releases.md). Local experimental builds require a
matching package manifest and runtime beside the launcher.

## Files and locations

| Path under `%LOCALAPPDATA%` | Contents |
|---|---|
| `JFGRecomp\profiles\default` | Saves, remembered selections, `controller*.ini`, `pc-input*.ini`, `experimental-controls.ini`, `audio.ini`, `video.ini` (**Open saves**) |
| `JFGRecomp\profiles\navigation-mod` | Separate save copy used by the navigation mod |
| `JFGRecomp\source` | Pinned source checkout |
| `JFGRecomp\setup.log` | Setup diagnostics |
| `JFGRecomp\support-exports` | Support ZIPs you create |
| `JFG\b\<checkout-id>` | Dependencies, generated inputs, game builds and build logs |

Builds from previews before 0.4.0-preview.2 remain in their original
`tools/private/local-builds` folders; nothing is moved. Only one launcher runs
per Windows session, and closing it stops the game and waits for saves.

## Playing

Use the menu bar during play: Game, Controllers, Video, Audio, Tools and Help.
F11 toggles fullscreen. The embedded game remains in a centered 4:3 area;
the original game controls its cinematic bars. Esc opens or closes the Game menu; in
fullscreen the menu bar stays hidden until Esc or F10. Settings dialogs pause
the game at a frame boundary and resume it when closed; menu dropdowns do not
pause. **Video > Video settings...** opens a separate RmlUi window with window
mode/size, **Pause when inactive**, VSync and Original/Custom/Unlimited rendering.
Custom offers a 60-1000 FPS slider and exact number; Unlimited removes the cap
with VSync off. Apply saves and closes, while Cancel discards edits.
Rendering changes apply on the next game launch; stop and start the game to
apply FPS/VSync changes. No launcher or Windows restart is needed. The restart
chip is hidden when no game is running. Display changes apply when saved.
High-refresh modes are
[experimental](../features/high-refresh-rendering.md); Original is the default.
**Stop** returns to the home screen after saving.

The Game menu's saved shortcut starts `JFG-Launcher.exe --play --profile
"<folder>"`, which opens straight into the game with the same menus and save
handling as **Play**. Close any running launcher before opening a different
profile, and recreate older shortcuts that target the setup helper directly.
For local cutscene tests, use a separate profile folder with
`--input-replay "<file>"` and `--progress-output "<file>"`, which require
`--play`. Ordinary play ignores inherited diagnostic environment variables.

| Keys | Action |
| --- | --- |
| W, A, S, D | Analog movement |
| Shift | Full stick magnitude |
| Space or Z | A / jump |
| X / C | B / Z |
| Enter | Start |
| Q / E | L / R |
| I, J, K, L | C buttons |
| Arrow keys | D-pad |
| Escape | Game menu (standalone diagnostic game: exit) |
| F11 | Fullscreen |

## Controllers

**Controllers > Controller mapping...** opens the separate RmlUi editor.
Choose a player, then **Change device** to select an available connected
controller or **Mouse and keyboard**. Device assignments and bindings save
with the profile. Xbox/XInput and supported SDL gamepads use their matching
button labels. A saved disconnected device retains its bindings.

**Controller Mapping** preserves the original game's button mapping.
**Modern Controls (Experimental)** adds action bindings, dual-stick movement
and aiming, and mouse look. The modes remain available alongside one another;
there is no separate experimental gameplay toggle. Buttons, Sensitivity &
behavior, and Advanced have their own tabs. Keyboard bindings remain editable
in Controller Mapping mode.

Modern keyboard defaults use WASD to move, mouse to look, left/right mouse to
fire/aim, Space to jump and Tab to pause. Modern gamepads use left stick to
move/strafe, right stick to look, RT/R2 to fire and LT/L2 to aim. Bindings remain
customizable. Scope magnification, hold/toggle aim, sensitivity, deadzones and
other supported settings are saved per player. Keyboard **Original movement
acceleration** is under Advanced and defaults off.

See [PC controls](../features/pc-controls.md) for implementation evidence,
settings and remaining experimental limitations. Original mapping and modern
bindings are retained separately when switching modes.

## Audio

**Audio > Audio settings** has a 0-100% **Volume** slider. **Mute / Unmute**
is in the Audio menu, and **Ctrl+M** works while the launcher or game is in the
foreground. Both apply live and are remembered; unmuting restores the chosen
volume. They change only the final PC output (the game's own music and SFX
levels still apply), and queued audio can take a few tenths of a second to
reflect a change. Normal and navigation-mod launches share these settings.

## Live map and inventory

**Tools** opens Live Map, Live Inventory, saved profiles, support reports and
the current export folder. Both tools follow the running game, show a waiting
state before gameplay, clear when the game stops, and label a launcher-paused
game PAUSED. They only read telemetry; they never change game memory.

- **Live Map** draws the current room's floors by height, the player, exits,
  items, NPCs, Tribals, living enemies and doors. Use the wheel to zoom at the
  cursor, drag to pan, and the corner zoom/**Fit** controls or **Fit room**.
  Select an object to see its requirements and rewards; **All object labels**
  (View menu, off by default) labels everything. **Map settings** has Display
  and AI tabs and pauses the game while open.
- **Live Inventory** shows weapons, keys, quest items and shared ship parts for
  Juno, Vela or Lupus, or follows the active character. Its artwork, including
  the character plaques, is extracted locally from your verified ROM; no game
  artwork is included in the launcher.

The [live map reference](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/features/navigation-mod.md)
explains every view, marker and export.

### Live map mods

**Live map > Settings > Map settings > Mods** has three independent toggles,
all off by default:

- **Warp to exits:** double-click a loaded exit marker, or an exit in the
  inspector, to activate that exit and enter the next room. The game's normal
  exit requirements and area-clear prompts apply. Warping stops autopilot and
  rejects stale rooms, scripted scenes and already pending transitions.
- **Infinite health:** keeps your current character at full health.
- **Instant kill enemies:** defeats loaded ordinary squad enemies; Tribals and
  friendly NPCs are spared. Switching off does not revive enemies.

Choices are saved with your profile and apply on later launches, even with the
map closed. A loading failure shows **Error loading mods** and disables mods
for that connection; reopening the map or starting a new game retries.
**Restore defaults** turns all three off.

## Navigation mod

The optional **Navigation mod** checkbox enables full health, automatic
clearing of ordinary squad enemies, map exports and experimental automatic
navigation in Live Map. It uses a separate campaign profile created on first
use; controller and audio settings are shared. A complete native build beside
the launcher is selected automatically. See
[navigation mod usage and limits](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/features/navigation-mod.md).

## Reporting a problem

Launch through `JFG-Launcher.exe` so a local support session is recorded. For a
freeze, leave the game open and click **Capture freeze** first. Then click
**Create support report**, choose the affected session, inspect the ZIP and
attach it to a
[playtest issue](https://github.com/TK22-26/JetForceGemini-Recomp/issues/new?template=playtest.yml)
with the level or menu, what you pressed, what you expected and whether it
repeats. The ZIP excludes ROM data, saves and personal paths. See
[playtesting and reporting](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/playtesting.md)
for its contents.

## Updating

Extract each complete release into a new folder and launch its executable.
Keep the existing profile to retain saves, imported ROM, controller bindings
and other preferences. The launcher verifies the bundled runtime and libraries;
copying a launcher executable alone does not update the complete application.
Use **Game > Verify game files** to check an installation. A local development
preview is not a published release.

## Release notifications (current source; unreleased)

The launcher checks public GitHub release metadata in a separate background helper
at startup. A newer stable version produces a persistent home-screen notice.
**Help > Check for updatesÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â¦** checks manually and shows installed/latest versions,
an error with retry when offline, and **View release / download**. This opens the
project release page; the launcher does not install files or alter saves. Extract
the full new ZIP into a new folder and run its launcher to retain the same profile.

Checks need no GitHub account. Only a public metadata request is sent, with no ROM,
saves or settings. Drafts and prereleases are excluded; version components are
compared numerically. Requests time out and response sizes are bounded. The existing
v1.1.0 package predates this feature. See the [issue/validation record](https://github.com/TK22-26/JetForceGemini-Recomp/blob/main/docs/development/issues/launcher-update-discovery.md).


### Release notification and design preview

At startup the launcher checks the project's latest stable GitHub release in a background helper. A newer version displays a large JFG-amber **Update available** card below Play. **What's new** opens the main GitHub repository page; **Update now** opens the GitHub release/download page. Help > Check for updates remains available. Current/older releases do not display the card; a failed check leaves play available and does not claim the installation is current.

For UI review only, `JFG-Launcher.exe --preview-update --profile <separate-folder>` displays a sample update without network discovery or a persistent fake result. The details panel labels the sample. Ordinary launch arguments use the real checker. This preview does not implement automatic installation.


The owner-approved card and browser actions are delivered locally from `628d1ff40ff14ce44a6921e89e594050b290b766`. Reopen High Refresh Preview or Jitter Capture for real discovery, or Update Button Preview for the forced sample. The game runtime and owner profiles are preserved. [Delivery and limits](../../evidence/launcher-update-button.json).
