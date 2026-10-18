# Live map and navigation mod

The live map reads the running game's geometry, actors and progression state.
The optional **Navigation mod** adds full health, automatic clearing of
ordinary squad enemies and experimental automatic navigation: walking, jump
chains, weapon chests, NPC rewards and exits. The mod is single-player, uses
the supported US ROM, and is off for ordinary launches. It does not establish
autonomous campaign completion.

Everything here reads memory and acts through ordinary controller input. The
navigation mod never grants keys or weapons, teleports, removes actors or
disables collision. (The separate **Warp to exits** live-map mod is described
in the [launcher guide](../development/launcher.md#live-map-mods).)

## Using the navigation mod

1. In the launcher, select **Navigation mod**, then launch. The first mod launch
   copies the campaign into `%LOCALAPPDATA%\JFGRecomp\profiles\navigation-mod`;
   later launches continue that copy and never write the normal profile.
   Controller settings are shared. Use the native build from the same source
   revision as the launcher.
2. Select the game's **Normal** (C-Up jump) or **Expert** (A jump) control
   scheme in Live Map before starting automation.
3. In **Live map**, either select an exit and click **Plan exit route**, inspect
   the dashed route and click **Start AI**, or click **Explore automatically**.
   **Advanced individual tests** exposes separate walking, box-jumping and
   weapon-chest trials.
4. **Stop AI**, Escape, any manual input, or closing the map returns control
   immediately. With the mod enabled, Escape cancels automation; close the game
   window to quit. After moving past an obstacle yourself, use **Retry room
   exits** to clear local failure blocks (discovered connections are kept).

Each launch writes its own export folder (**Open exports** in Live Map).
Exports come from your ROM and stay local; they are not included in the
repository or support ZIP. For direct native launches set
`JFG_NAVIGATION_MOD=1` and `JFG_MOD_OUTPUT` to a new directory and pass separate
`--save` and `--controller-pak` paths. View a saved session with
`JFG-Launcher.exe --map-view "C:\path\to\maps\run"`, or inspect it with the
standard-library reader:

```text
python scripts/navigation_map.py "C:\path\to\maps\run"
python scripts/navigation_map.py "C:\path\to\maps\run" --allow-stale --obj map.obj --svg map.svg
```

## Gameplay changes made by the mod

- Health is restored before and after player updates and the damage-immunity
  timer stays active; capacity follows the character's upgrades.
- Ordinary enemy-squad members, including later spawns, have their health set
  to zero so normal death processing runs. Tribal classifications (including
  friendly squad variants), dialogue NPCs and animated cutscene actors are
  excluded. Clearing starts only after normal gameplay begins and pauses in
  other game modes, cutscene cameras and while controls are disabled.
- Bosses and special scripted encounters need individual testing.

## Live map display

The map is a flat X/Z view; Y is vertical. Upward-facing collision surfaces are
filled with a fixed room-wide height palette; walls and ceilings are omitted.
Colors do not imply walkability.

- **Player floor** (default) follows the highest surface within eight units
  under the player and holds the last height while airborne (a geometry
  estimate, not a contact flag).
- **Height slice** shows a chosen elevation; reduce **Slice width** to separate
  stacked floors. **All heights** shows every surface; upper floors can cover
  lower ones.
- **Other levels** adds faint context, hollow markers with direction/distance
  labels, dashed out-of-band entity boxes and ascending ramps in gray.
- Markers: cyan player (arrow = movement direction), yellow exits, item squares
  (orange weapons, purple keys, green other, gray opened chests), blue dialogue
  NPCs, white living Tribals, red enemy diamonds, orange-red shooting targets,
  violet doors/switches and small gold Mizar tokens. Labels state verified
  categories, never guessed names; `ForestCrate` scenery is not an item.
- **Entity collision boxes** (on by default) draw registered collision models:
  fill is the top height, the side stripe the base height. Click a box for its
  name, position, base/top, size and polygon-collision state. **Unknown entity
  origins** optionally marks actors without registered bounds.
- The inspector lists interactions with position, action, reward,
  requirement and current state, and keeps its selection and scroll while
  snapshots refresh. Selection resets on room changes.

## Export format

Normal launches write read-only telemetry; mod launches add the gameplay
changes and AI command channel. All files are schema 1, replaced atomically.
`mesh.json` is written once per room load; `live.json` every six game updates.

- **`mesh.json`:** level, generation, blocks, vertices and triangles (vertex
  indices, original batch flags, block index, collision-plane normal). Material
  flags are retained, not treated as walkability rules.
- **`live.json`:** level, generation, `timestamp_ms` (UTC), update,
  `mesh_ready`, `clearing_active`, player (position, raw yaw, health in
  fixed point / 256, address), exits (position, plane normal/`plane_d`, raw
  radius, `destination_code`, `condition_code`), markers, `npcs`, `actors`
  (with sanitized names), `progression`, `collision`, `navigation_ai` and mod
  counters including `first_clear_level`/`first_clear_update`.
- **`npcs`:** address, position, kind, label, `object_id`, behavior and
  `squad_type` (-1 for dialogue NPCs; dialogue behavior 90 or the original
  Tribal classification). Classification does not establish rescue status,
  interaction range or remaining rewards.
- **`progression`:** `inventory` (`known`, `character`, `red_key`,
  `weapons_mask`), `npc_catalog` coverage and `nodes`. Nodes carry address,
  position, kind, label, action, status, requirement/reward fields, `door_id`,
  `linked_actor`, raw state/condition, target health and reset ticks, and
  `traversal` (always `"unknown"`). Door/exit nodes add `access_known`,
  `access_allowed`, `condition_known`, `condition_met`, `pending_openers` and
  `approach_radius`; chests may add an `activation` region; NPC nodes add
  `offers` with conditions (`dialogue_row`, `visibility`, `prerequisite`)
  whose state is `met`, `missing` or `unknown`.
- **`collision`:** `known`, `reason` and models with address, `enabled`,
  world `lower`/`upper` bounds, and a transformed `surface` for supported
  bridges (behavior 55) and chests (behavior 98).
- **AI channel:** `live.json.navigation_ai` reports state, nonce, waypoint
  index and jump attempts. `ai-command.txt` carries room, generation, nonce,
  timestamp and waypoints (`JFGNAV1`-`JFGNAV3`, adding running-waypoint
  ranges) or a bounded interaction (`JFGINTERACT1`). Commands expire after
  1.5 s without a heartbeat and cannot be rearmed under the same nonce after
  completion or manual cancellation. `exploration-history.json` stores rooms,
  directed exits, attempts and progress facts for that game session only.

Consumers must reject snapshots without a player, with `mesh_ready` false,
mismatched level/generation, or a timestamp older than five seconds, and must
wait for `clearing_active` before supplying movement. Addresses are valid only
within one loaded snapshot; key them by session, level and generation and never
persist them across room loads. Integer -1 means unknown. Destination and
condition codes are raw game fields; a destination code is not a room number,
and a visible exit is not an open gate.

## How the interaction data is derived

- **NPC offers** come from the supported ROM's dialogue control tables: all 45
  selector groups and 19 reachable choice tables parse into 29 offer paths
  (trades, red-key encounters, rocket-launcher rewards, ship parts, services,
  music choices, scene transitions). Offers are **available**, **blocked**,
  **owned** or **unknown**; these describe dialogue/inventory eligibility, not
  reachability. An isolated harness matched the host's predicates against
  unmodified generated game functions in 8,832 comparisons over 192 states and
  executed all 19 reward handlers for each character. Four audio-dependent
  music visibility cases stay unknown.
- **Chests** report verified weapon contents and opened state; unknown special
  rewards stay unknown.
- **Shooting targets** expose activation latch, strength, recovery timer and
  door group (controller `+0x08` door group, `+0x0A` latch, `+0x00` timer,
  `+0x06` strength). The timer is not a hit count.
- **Doors** decode character-specific key requirements (door controller
  `+0x43` item - 2, `+0x3E` bit 8 key lock, `+0x44` group), proximity opening,
  enemy-group and target locks, and supported character restrictions.
  `pending_openers` counts registered groups, not enemies.
- **Collision models** are read from the registry at `0x801047E0`/`0x801047E4`
  (actor collision state `+0x5C`, lower/upper bounds `+0x100`/`+0x10C`, polygon
  participation bit 0 of properties `+0x0A`), shared with
  `scripts/phase95_collision.py`, after a signature check of the resident accessor.
- **Camera heading** uses a signature-checked control-camera yaw accessor
  (JFG-UP-025 in the [upstream review](../upstream/reference-review-2026-10-01.md)).
- **Player motion** reads state byte `+0x568` of the player state (actor
  `+0x68`), animation ID `+0x3B` and frame `+0x28`. Hang and grab entry write
  states 6 and 12; these are confirmed entry values, not a decoded enum.
- **Area Cleared** may be acknowledged with one A pulse per pending exit only
  while the transition-prompt flag (`0x800A329C`) and pause-mode byte
  (`0x800FD7BD`) both equal one, matching the transition update at `0x800468EC`.

These are supported-US facts checked against the pinned local decompilation;
the host code is original and embeds no reference assembly or game assets.

## Automatic navigation design

**Planning.** Walking searches upward-facing triangles connected by shared
edges at least 40 units wide (including collinear split edges) with steps of
at most 24 units and faces no steeper than 45 degrees; stacked floors are
never joined by proximity. Every emitted segment needs continuous floor
coverage across a 40-unit strip, static terrain and headroom probes from 4 to
80 units above the floor, and clearance from active entity boxes expanded by a
20-unit body allowance plus 20 units of planning margin. Routes are capped at
256 waypoints, 600-unit segments and 80 units of height change per segment. A
bounded local visibility search repairs blocked segments; a room-wide A* over
floor samples (16,000-sample, three-second limit) is the fallback. Paths near
obstacles carry extra cost. Ramps are followed before the triangle graph, and
a wall-adjacent or elevated exit is approached inside its trigger volume. No
verified route means movement is refused. These dimensions are prototype
values, not a measured character capsule.

**Doors.** An exit behind a registered `pass_door` produces an approach on the
player's side. The explorer waits up to eight seconds for the current door
bounds to clear the standing body, then replans through the exit; a door that
stays closed is recorded with its requirement.

**Steering.** The native pilot uses the camera heading, full 80-unit input with
lateral velocity feedback and bounded lookahead, and brakes for endpoints,
hairpins, doors and scripted cameras. Running permission covers only the
freshly checked segment range and expires after 500 ms. Every update rechecks
the actual player-to-waypoint segment; a new obstruction releases movement and
names the blocker. Movement without progress for about 1.8 s stops and replans.

**Jumps.** Standing-jump calibration (measured on SS Anubis: vertical velocity
23.1, gravity 1.8 per update, apex rise 148.2) feeds a bounded planner over
supported landing patches with full body, ceiling and entity arc checks. Each
jump needs a confirmed landing on the intended patch before the next; landing
control corrects the predicted stopping point on both axes. Hang/grab entry
stops the action. Lupus hover, crawling and general moving platforms are not
modeled.

**Exploration.** The coordinator handles one action at a time: loose keys and
available item-giving NPCs first, then other supported NPC rewards and unowned
weapon chests, then exits (untried forward exits before the remembered
entrance, which is a checked return candidate). It interrupts walking for
verified dialogue, advances ready text, follows known choice actions and
checks the ownership flag before resuming; paid services, unknown choices and
unmet prerequisites get no button presses. A connection is recorded only after
an acknowledged command, an approach and a different room ID that stays stable
in gameplay for 600 ms; same-room reloads, rewinds and conflicting destinations
stop the explorer. New key, weapon, NPC or gate facts permit retrying blocked
routes. Limits: four local recoveries and 120 s per exit, one minute and 64
inputs per dialogue, three minutes per action, and 15 minutes and 128
transitions per session.

## Validation summary

Synthetic native, launcher and Python fixtures cover the readers, exports,
planner, controller and coordinator, including stale, malformed and
cancellation paths. Live trials used muted runs on copied saves; original
saves were hash-checked unchanged. Failed trials are retained in private evidence.

| Date | Live result |
|---|---|
| 2026-10-02 | Goldwood replay cleared all three hostiles while the friendly guide survived; health held at 8,704; SS Anubis export decoded 2,686 vertices and four exits. |
| 2026-10-02 | Offline room-35 capture decoded 15 interaction nodes, the red key and weapon mask 5; NPC catalog covered 45/19 tables. |
| 2026-10-03 | First observed automatic transition: room 27 to 157, with Area Cleared acknowledged. |
| 2026-10-03 | Room 27 hut loop and forward door to 236, waiting through the scripted camera (closest approach 8.4 units). |
| 2026-10-03 | Continuous-speed controller averaged 74.9 of 80 stick units in room 27; whole-room time 37.3 s versus 90.4 s. Exposed the [room 54 crash](../development/boot-gameplay-fix.md#room-54-sound-queue-crash-and-hang). |
| 2026-10-04 | SS Anubis five-jump staircase and shotgun chest with Normal controls (weapon mask 5 to 13). |
| 2026-10-04 | Tutorial room 92 ramp and bridge, Magnus's forced dialogue, entry to room 47 without jump calibration. |
| 2026-10-04 | Rooms 92, 47, 122: Fish Food chest on foot (mask 1 to 1025) and return through the same doorway. |
| 2026-10-04 | Room 47 Exit 4 back to 92; King's hut (48) curtain approach and return to 47. |

The saved level-157 stair geometry yields four jump links offline; the full
native walk, stair chain and conversation in that room are not yet verified.

## Limits

Not established: autonomous campaign completion, every NPC reward branch,
Expert-controls execution, universal chest access, crawl clearance, Lupus
hover, moving platforms, shooting/explosive gates, arbitrary downward parkour,
dive exits, bosses and unrecognized dialogue. The planner's boxes are
conservative: a rotated or hollow object may have usable space it refuses.

Planned traversal work uses layered surfaces with directed action links
(walk, jump, drop, crawl, door), following
[Detour off-mesh connections](https://recastnav.com/structdtOffMeshConnection.html),
[Unreal navigation links](https://dev.epicgames.com/documentation/en-us/unreal-engine/automatic-navigation-link-generation),
[kinodynamic planning](https://lavalle.pl/planning/node719.html) and
[motion-primitive planning](https://arxiv.org/abs/1809.02399). Before adding
traversal edges, measure per movement mode: standing height, lateral size,
origin offset, step and slope limits, acceleration and braking; jump velocity,
gravity, apex, range at several speeds and air control; and crawl dimensions,
speed and stand-up headroom. Validate in both directions: a safe drop does not
imply a feasible jump back. Recast's small-region filter can remove box tops;
preserve landing islands if it is adopted (zlib license notice required).
Travel-speed reports should give time-weighted stick magnitude together with
distance progressed and room completion time, with door and cutscene waits
reported separately.
