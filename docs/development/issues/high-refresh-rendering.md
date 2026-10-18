# RENDER-001: Experimental high-refresh presentation

- Severity: enhancement / significant rendering limitation.
- Status: separate RmlUi Video window and capped/uncapped rendering implemented;
  seven focused tests, launcher checks, live dialog pause/resume, guest-state
  equivalence and distinct swapchain images verified in bounded fixtures.
  Unlimited pacing and broader owner playtesting remain open; not released.
- Reported and last updated: 2026-10-11 UTC.
- Baseline: `d649cf586b13d2bff7ab7258db9991ad554ae017`, combining the owner's
  `66e14d6` workspace checkpoint with latest fetched main `acccc55`.
- Owner target: smooth 60 FPS and higher, ideally 360+ on capable hardware;
  preserve game simulation timing.
- Related: [PERF-001](goldwood-combat-slowdown.md) is a separate, ongoing
  simulation-cadence investigation. This work does not reopen or replace it.

## Symptoms and reproduction

The current renderer starts with RT64's original refresh mode. JFG's custom
F3DDKR bridge explicitly marks its transform groups as ignored for interpolation.
A high-refresh display alone therefore does not provide new interpolated scene
states. These describe the default mode; candidate measurements are recorded below.

## Initial source evidence and hypotheses

`src/renderer/rt64_shell.cpp` selects Console presentation to preserve CPU-written
overlays. `src/renderer/rt64_f3ddkr.cpp::select_matrix` selects `G_EX_ID_IGNORE`
and skips all transform components, with an already-combined matrix and identity
view/projection. RT64's pinned `5473732a` workload queue also clamps manual rates
to detected display refresh. Each boundary must be addressed explicitly.

The leading hypothesis is presentation-only RT64 interpolation with stable
render-object identities and qualified camera/model representation. DK64 uses
simple interpolation in its reviewed model path, so separate affine transforms
are not a prerequisite for a bounded initial experiment. Unqualified decomposition
remains inappropriate. Increasing VI, Count or simulation frequency is outside
the agreed first milestone.

## Attempts and results

| Date | Investigation | Result |
|---|---|---|
| 2026-10-11 | Checkpoint owner workspace and integrate fetched main | `66e14d6` and merge `d649cf5`; 10 release-prebuilt and 7 package-beta tests pass under production guard |
| 2026-10-11 | Inspect pinned RT64 and JFG bridge | Explicit interpolation opt-out, combined matrix representation, presentation ownership and display-rate clamp identified |
| 2026-10-11 | Retrieve primary sources from DK64, Zelda, Banjo, Starship and DKR ports | Revision-pinned comparison complete; stable identities, camera discontinuities and task ownership inform the proposed integration |
| 2026-10-11 | Implement, build and replay the opt-in prototype | Local commit `8d8f097`; five focused tests pass; 60/240 FPS camera fixture and exact guest-state comparison verified; owner visual acceptance pending |

## Actual resolution

Implemented in local commit `8d8f09701cc50d3a5b03eafd90ff5a41d3aeb3e9`
on `research/high-refresh-20261011`. No release is associated with this candidate.

An opt-in transform interpolation path is implemented behind
`JFG_EXPERIMENTAL_RENDER_FPS=60` (or another integer up to 1000). It preserves
the authored workload matrices and keeps copied scene history outside guest RAM.
This is an unreleased candidate; full visual acceptance and performance
qualification remain open.

## Verification and remaining limits

The candidate has matched guest memory/execution and distinct presented-frame
evidence for the retained replay. Still needed: longer pacing/GPU measurements,
audio-device behavior, semantic actor/camera lifecycle, CPU overlays, animation,
particles, menus, pause, resize and multiplayer checks. A rate setting or an FPS
counter alone is not proof of 360 FPS performance.

## Lessons and retained evidence

The production guard stays enabled across restart, compaction and handoff.
Use investigation `high-refresh-rendering-20261011` in the original operational
ledger; the owner granted a 43,200-second budget. Preserve counters and stop on
denial. Private evidence: `high-refresh-20261011`; no ROM, generated game code or
save data is added to source. Update this record and the issue index as work proceeds.

## Working-build memory baseline, 2026-10-11

Four isolated replays used retained reference-clock runtime `26852729d4f` and
copies of one save/profile, stopping at VI 4000, 4002, 4004 and a repeated 4004.
All runs exited normally. Repeated VI 4004 guest RDRAM, renderer snapshot and
captured image were byte-identical. The original save remained unchanged.
This retained runtime is a reference build, not a new build of the research branch.
Private hashes and endpoint trace metadata are retained in `memory-baseline.json`
and `memory-comparison.json`. The runtime probe marks `acceptance=false` and
`runtime_snapshot_complete=false`; these dumps are not full resumable checkpoints.
This qualifies a reference fixture only; it does not validate interpolation.

## Findings and next implementation boundary

See the [source comparison](../../upstream/high-refresh-reference-review-2026-10-11.md)
and [implementation plan](../../planning/high-refresh-rendering.md).

All four replays retained the reference clock and exited at their requested VI.
The repeated VI 4004 state hash, recorded input, 4 MiB guest RDRAM, 8 MiB renderer
snapshot and captured image agree. This establishes repeatability for this fixture
and runtime only. It does not establish unchanged gameplay for a future candidate.

- `vi4000` to `vi4002`: 23,419 guest-memory bytes differ; 10,423/307,200 pixels differ.
- `vi4002` to `vi4004`: 248,057 guest-memory bytes differ; 298,273/307,200 pixels differ.
- `vi4004` to `vi4004-repeat`: 0 guest-memory bytes differ; 0/307,200 pixels differ.

The traces alternate scene color buffers `0x10ff00` and `0x135700` with
a separate offscreen pass at `0x328480`. At the endpoints, the last submitted
task is the small offscreen pass (`command_size=0x1b`), whereas presentation
selects a scene buffer. Do not use the last arbitrary graphics task as the
interpolation endpoint. The existing single final `.rt64` dump needs task/VI
lineage before it can serve as a multi-frame interpolation replay.

At the research handoff, no renderer rate switch or new game build had been
installed. The implementation below follows that retained baseline; the research
measurements must not be presented as candidate verification.

## Implementation in progress, 2026-10-11

The owner requested executable implementation after the research handoff.
A guarded coding/build/replay session is adding an opt-in render-rate prototype.
It snapshots exact-geometry matches and previous matrices with each RT64 workload,
preserves scene interpolation state across offscreen tasks, and derives source
cadence from immutable guest VI timestamps. The default remains original mode.
The initial implementation began with build, frame evidence and gameplay comparison pending; the completed candidate results are recorded below. CPU overlays
under SkipBuffering, ambiguous repeated meshes, deforming meshes and camera
discontinuities remain acceptance limits. This continues the same investigation.

Build setup findings: CMake is supplied by Visual Studio rather than PATH. The shared generated cache failed the branch normalizer digest check, so a new private root was regenerated. Its first compile exposed the missing final original-timing header step; the normal generation finish helper now supplies that header from this branch. No freshness checks or compiler errors were disabled.

The first renderer compile caught a C++20/char8_t incompatibility in pinned RT64, explicit interop-matrix conversion requirements, and an over-broad counter patch site. The shared history interface now remains C++17 compatible; RT64 keeps its existing language standard, conversions are explicit and the counter replacement is scoped to the render loop. Runtime qualification follows below.

The full game compile reached native boot and caught an MSVC deprecated environment getter in the new FPS setting. It now uses the existing Windows-safe environment API; warnings remain errors. The replay launch gate correctly refused to execute before a successful build. The generated function objects are retained for the incremental rebuild.

The optional frame-sequence capture initially matched a second GPU wait in the resize path. Its completion hook is now anchored to the exact presentation command submission, preserving the resize path. Compilation caught this before any candidate replay was launched.

## Candidate replay qualification, 2026-10-11

Release runtime SHA-256: `3a739fbf3b1f4824341e75634305945ce36c1351e1b835c761847208e3a26ccb`. Three isolated 4,100-VI replays at original, 60 and 360 requested FPS all completed normally in about 70 seconds. Full 4 MiB guest RDRAM, 8 MiB renderer snapshot, recorded input, save and probe state hash are identical across modes. All runs report 1,906 controller samples, 2,896 graphics tasks, 2,047 decoded audio tasks, 3,680,667,236 instructions and zero unsupported accesses. The probe still marks acceptance=false and runtime_snapshot_complete=false; this is fixture-specific evidence, not full game acceptance.

- Requested 60: effective 60 FPS; moving-camera VI 4000-4098 measured 59.98 presentations/s with 100 scene renders. Median/p95/p99 presentation intervals: 16.672 / 16.738 / 16.755 ms.
- Requested 360: effective 240 FPS; moving-camera VI 4000-4098 measured 239.81 presentations/s with 400 scene renders. Median/p95/p99 presentation intervals: 4.170 / 4.303 / 4.411 ms.

Private evidence: prototype-replays-v1/result.json and prototype-pacing-v1.json. Hardware: RTX 5090 / Ryzen 7 5800X; display detected by RT64 at 240 Hz. The 360 request is display-capped, so 360 delivered FPS is not claimed. Frame sequence capture follows separately and is excluded from pacing measurements.

## Presented-frame verification and handoff

- Requested 60 FPS: captured 4 actual swapchain frames; present 2789: 2/2 unique images; present 2791: 2/2 unique images. Guest RAM remains byte-identical to original mode.
- Requested 360 FPS: captured 15 actual swapchain frames; present 2788: 8/8 unique images; present 2791: 7/7 unique images. Guest RAM remains byte-identical to original mode.

Images are retained privately in prototype-frame-sequence-v1; per-pair pixel
differences and distinct-image counts are summarized in prototype-frame-sequence-analysis-v1.json.
These capture runs are excluded from timing measurements. Sampled final images
show the HUD, character and reflection; owner motion/scene acceptance remains
pending. The [feature page](../../features/high-refresh-rendering.md) records the
preview invocation, exact coverage and remaining limits.

The first milestone is a usable experimental implementation, not a released or
fully qualified rendering replacement. Preserve this evidence, original timing
and default rendering. Recover the original production ledger before further
autonomous work; never reset counters or bypass a supervisor denial.

## Guarded continuation boundary

The implementation, builds, replays and image checks ran inside the existing
production-guarded implementation job. Its starting investigation counters were
six attempts including the active job and five completed supporting activities,
with a six-activity limit and a 43,200-second execution allowance. The guard was
verified enabled throughout; no counters, roots or limits were reset.

Completing this supporting job reaches the activity limit even though elapsed
execution is below twelve hours. Read the production ledger and the retained
implementation-progress.json before any continuation. A completed preview does
not authorize another job or reopen the investigation after shelving. Keep
RENDER-001 open for the remaining acceptance work.

## Native Windows video controls continuation (2026-10-11)

Owner requested replacing the embedded Video panel with a native Windows dialog
and adding VSync, Original/Custom/Unlimited modes and a custom FPS slider/number.
The existing candidate exposes only an environment-controlled fixed target,
always uses the renderer's default VSync and clamps to display refresh. This is
an open usability and presentation limitation (RENDER-001), not a released fix.

The owner approved a three-hour override after the six-activity guard stop.
The same production investigation and roots are retained, guard enabled, with
all six attempts/activities and 5204.53 elapsed seconds preserved. The activity
ceiling is twelve; the original 43,200-second cumulative budget remains and this
continuation also has a three-hour wall deadline. No other investigation reopens.

Candidate work and verification will be recorded here as it proceeds. Validation
must cover actual native controls, persistence/cancel/invalid input, capped and
uncapped presentation, and guest timing/state equivalence. No result is assumed.

The first continuation build caught signed/unsigned optional comparisons in the new parser tests under /WX. The test constants were corrected; no compiler warning policy was relaxed. No runtime replay had run at this point.

Runtime and native-dialog builds pass with seven focused tests. Managed launcher tests initially stopped because their fixture discovery requires the executable below the checkout; an external C:/JFG-builds output made FindSourceRoot return null. Re-running the existing suite in the worktree build/launcher directory addresses the harness layout without changing production behavior. The failed receipt is retained.

### Unlimited completed-stream correction

The first seven-mode replay series preserved game state, but source review of
the new ring found that a later VI could revisit a completed interpolation batch
and replay its indices even though the two storage slots had been overwritten.
The first unlimited result (646.24 presents/s in the short camera window) must
not be treated as a verified distinct-frame throughput result. This is a candidate
presentation defect, introduced and caught before commit/release.

The corrected presenter recognizes a completed unlimited counter, holds the
workload lock while copying its latest image once, and never replays that ring's
old history. The same original/capped/unlimited memory fixture and bounded image
capture will qualify the change. The ring remains bounded to two extra targets;
the original guest, audio and input clocks are untouched.

The real-launcher integration fixture initially hit the application's existing
session-wide singleton because another launcher was already open. No owner
window was closed or controlled. A non-shipped target now builds the same
frontend, helper and resources with only a test-specific singleton name, and
runs on an inactive desktop. Production singleton behavior remains unchanged.

The first isolated frontend keyboard test sent Alt+V/Enter while initialization
could still close transient menus. The harness now waits for profile initialization
and its timer acknowledgment, then selects the first menu item with Home before
Enter. This changes only the test; the native dialog unit checks already passed.

The corrected stream measured 315.28 presents/s with gaps around 16-17 ms.
Inspection of VI 4000-4010 showed interpolation stopping near alpha 0.4-0.5:
queued shadow-only passes cancelled the scene before its 30 Hz time window ended.
The next candidate cancels an unlimited scene for queued scenes/unknown work,
while bounded shadow-only work waits at most the remainder of that source-frame
window. Replay validation must check both exact guest state and elapsed runtime,
since rendering must not slow the original simulation. This is pending evidence.

The frontend fixture also observed a just-created dialog before WM_INITDIALOG
had initialized its controls, so its initial Custom selection was overwritten by
the default Original selection. It now waits for a visible dialog (shown after
initialization), matching the state in which a person can interact. Native control
tests and actual file values isolated this test-harness race; no owner input was used.

Waiting for dialog visibility did not resolve the integration fixture's FPS
assertion. Initialization ordering was a hypothesis, not an established cause.
The next fixture records the actual combo/edit values and saved bytes, and uses
explicit cross-process WM_SETTEXT/WM_GETTEXT messages to isolate automation from
product persistence. This preserves the failed receipts instead of accepting a
partial dialog test as complete.

The frontend integration failure was in cross-process text automation, not the
dialog's configuration model. Explicit WM_SETTEXT/WM_GETTEXT recorded Custom / 360
and the expected saved bytes; the full menu/modal/pause/persistence/cancel/reopen
flow then passed. Invalid-input injection now uses the same explicit messages and
asserts that 59 actually reached the control before testing rejection. Earlier
initialization hypotheses are not the accepted cause.

The shadow-aware unlimited candidate preserved every compared byte/counter and
measured 1137.31 presents/s in the approximately 1.74-second camera window. Its
complete replay took 71.515 seconds versus Original's 70.109: guest clock/state
equivalence does not establish identical wall-time throughput. High-rate frame
pacing, broader scenes and audio-device behavior remain open. The 1000 FPS target
measured 644.78 presents/s in that run; the requested value is not a throughput
guarantee. A separate image-capture run follows and is excluded from benchmarking.

The final capture wrapper initially selected a quoted source fragment instead of
the complete Python function and stopped with SyntaxError before launching a
capture. The wrapper now extracts the function through Python's AST. All seven
focused checks and the corrected invalid-input/frontend flow had already passed;
the capture failure is retained separately. Live pause/resume is also being
checked with a replay-driven game on the fixture's inactive desktop and isolated
save, including the runtime acknowledgment and a stopped/resumed VI clock.


### Native controls and final candidate verification

The native Win32 dialog, persisted Original/Custom/Unlimited modes, VSync and
60-1000 FPS slider/number are implemented. Display and pause preferences remain
available. Rendering preferences apply on next launch; window/pause preferences
apply immediately. Missing configuration preserves Original mode. The managed
launch environment forwards the selected profile, including navigation mode.

The final bounded live fixture passed: the actual frontend Video menu opens an
owned native dialog, Apply saves Custom 360 / VSync off, invalid 59 is rejected,
Cancel preserves the last Apply, reopening restores values and OK saves Unlimited.
A replay-driven game acknowledges pause, its VI clock stays stopped during the
dialog and resumes on close. Its source save remains unchanged. The launcher runs
on a separate inactive desktop with a test-only singleton; no physical input or
owner-window manipulation was used. Production retains its ordinary singleton.

Final capture groups 2789 and 2791 each contain four different swapchain images
at indices 0-3, crossing the two-target ring. Guest and renderer memory, input,
save and entire probe output match v3 Original. Visual inspection of the sampled
scene retains HUD, character and reflection; it is not general scene acceptance.
The final runtime SHA-256 is
`b34887ed7da310b11d52d33a1382f83b4c6e521985386d3061226fe6c43d408e`.

Seven focused C++ tests pass; the managed launcher reports 637 checks, setup 15,
graphics six and viewport checks pass. Native control tests include owned modal,
enablement, numeric limits, slider synchronization, Apply/Cancel/reopen, Unicode
configuration paths and save failure. Persistent issue status remains **candidate
verified in bounded fixtures; broader owner playtesting pending; not released**.

Private evidence remains under `tools/private/high-refresh-20261011` in the main
checkout: `video-replays-v1/result.json` (exclude invalid initial Unlimited rate),
`video-replays-v2/result.json`, `video-replays-v3/result.json`,
`video-frame-sequence-analysis-v3.json`, `video-live-final-results.json`,
`video-live-integration.log`, `video-dialog-final/`, and hashed `video-steps/`
payload/result/log receipts. The final source/binary map is `video-qualification.json`.
No ROM, generated guest code, game screenshots or private saves are committed.

Root causes and lessons: a fixed target was originally display-clamped because
VSync was unconditional and no launcher configuration reached RT64. Explicit
policy now controls synchronization, target clamping and pacing. A truly uncapped
stream needs bounded synchronized storage and time-based interpolation; completed
ring history cannot be replayed. Shadow tasks are not scene endpoints and must
not prematurely cut off interpolation. Distinct-frame captures plus guest-state
comparison are required alongside throughput: present-call counts alone misled
the first candidate. Correct cross-process UI messages and actual control-value
assertions distinguish automation defects from settings defects.

Remaining limits: high-rate pacing and the measured ~2% whole-replay wall slowdown;
sustained busy combat/effects and audio-device behavior; camera cuts/rotation and
semantic object identity; CPU overlays, resizing, multi-monitor DPI/refresh,
loading/cutscene transitions and multiplayer. VSync-off output can tear. These
changes do not fix the separate Goldwood simulation-cadence issue. Preserve the
production guard, original roots/counters and this evidence for any continuation.


### Committed handoff

Implementation and native controls: `b39533a5b3661544d5322bd70cd64ae9843f5254`
on `research/high-refresh-20261011`. The rebuilt production launcher SHA-256 is
`5af0a7b104a8b574a8cb4ce441bca443fac95597b6ab5ca98580ec4e7f6f4722`;
its embedded helper and build receipt identify that implementation commit.
The final package's real menu/persistence fixture passes after rebuilding. The
test-only launcher executable was removed from the playable preview.

The complete machine-local preview is
`C:\JFG-builds\high-refresh-video-preview-20261011`, opened with
`Play-Video-Settings.cmd`. It has a separate copied save/profile and starts at
Custom 60 / VSync on. Close another running launcher first. This is a local
candidate, with no push, release or replacement of the installed game.

`video-qualification.json` maps tested sources, runtime, launcher and evidence;
`video-manifest.json` preserves successful and failed guarded steps. The original
production ledger remains authoritative for guard state and charges. Finalizing
this job neither resets its counters nor authorizes further investigation.


## RmlUi design continuation (2026-10-11)

Owner supplied `Child Ãƒâ€šÃ‚Â· Video settings-html.zip` and requested the newer RmlUi
appearance in the separate settings window, plus verification of the restart
chip. The native-controls implementation remains preserved at b39533a/3faa8c9.
The original investigation resumes under another explicit three-hour window;
guard enabled, original roots and all seven attempts/activities and 9019.38 s
retained. No counters or limits reset. The SDCCC shortcut was first corrected as
separately requested, including its launch script's explicit Workspace argument.

Source review: video.ini is read in initialize_live_renderer, with presentation
policy configured only when creating RT64. Apply persists preferences but does
not replace a running renderer. FPS/VSync currently require a new game session;
display mode/size and pause preferences apply when saved. Live confirmation is
pending. The design is being translated into embedded RML/RCSS and native C++
events, with no browser, React or remote fonts. Supported window modes remain
Windowed and Borderless fullscreen; a separate exclusive-fullscreen mode is not
implemented and will not be represented as a working third choice.

The first RmlUi build wrapper stopped before execution on a Python slice delimiter typo. The immutable failed receipt is preserved; the corrected wrapper runs the same build and UI checks without changing verification requirements.

The first real RmlUi pointer test caught an incorrect preset-ID substring length introduced in the wrapper: clicking 360 left the cap unchanged. The handler now derives the prefix length from its literal. The renderer log also rejected an unavailable triangle decorator; the dropdown arrow now uses supported RCSS borders/rotation. No UI test was waived.

The 85 functional UI checks passed, but screenshot inspection found fragmented section backgrounds: standalone RML does not supply the browser block-layout defaults used by the HTML mockup. Explicit block containers and centered button contents restore the intended cards. Visual inspection is required in addition to passing control tests.

Centering buttons with flex exposed another RmlUi/browser difference: raw text directly inside flex buttons did not render. Explicit text spans preserve button labels and their intrinsic widths. The second visual inspection caught this before delivery; control checks alone remained insufficient.

The three runtime restart probes passed: with a 360 target, VSync on remained at
240 after editing the file to VSync off; the next session used 360. Editing that
session to 144 remained at 360; the next used 144. Source and runtime evidence
agree that rendering requires a new game session. The RmlUi chip is absent when
no game is running; window/display controls do not claim a restart requirement.

The first full-launcher pointer fixture missed Apply/display/Cancel despite the
85 in-process UI checks passing. Window DPI and the live-window work-area clamp
are a hypothesis: the fixture used fixed 720x700 layout coordinates without
enforcing that client area. It now declares per-monitor DPI awareness, records
actual dimensions and sets its own test window to the measured client size.
No owner window or physical desktop input is touched. This remains pending.

The corrected full-launcher fixture passed all Apply, Cancel, OK, fullscreen,
pause-preference and child-crash recovery checks. The actual child client was
900x875 at 120 DPI (125%) both before and after the explicit fixture size, so
work-area clamping was not the cause in this run. Declaring the test process
per-monitor DPI aware resolved the cross-process pointer-coordinate mismatch.
No product persistence behavior was changed for this harness correction.
Live pause/resume is now rerun with the same corrected pointer path; real RmlUi
slider and text-widget input are checked in addition to direct change events.


### RmlUi candidate qualification and restart result

Final UI checks: 101 passed, including actual slider pointer input, text-widget
typing, validation, presets, Apply/Cancel/OK, reopen, save-failure draft retention,
dropdown keyboard selection and Escape priority. Inspected 100%/150% screenshots
show the supplied card layout, centered labels and local-font rendering without
RmlUi parse errors. The launcher fixture at 125% DPI passes the actual Video menu
route, supervised owned window, rendering persistence, immediate fullscreen and
pause settings, cancellation and owner recovery after settings-process failure.
The corrected live fixture passes pause acknowledgment, stopped/resumed VI clock
and unchanged source save. Modern-controls 546 and mapping 207 checks also pass.

Three 900-VI renderer probes establish the restart boundary. At VI 420, changing
VSync from on to off leaves the current 360-requested session at 240 effective;
the next uses 360. Changing that session's saved cap to 144 leaves it at 360;
the following session uses 144. Runtime SHA-256 remains
`b34887ed7da310b11d52d33a1382f83b4c6e521985386d3061226fe6c43d408e`.
The actual cause is startup-only configuration loading and renderer-policy
initialization; the UI save path never reconfigures the live RT64 queues.
Retain the game-session restart notice for rendering changes and hide it when no
game is running. Display/window and pause settings apply on Save without restart.

Evidence under the same private investigation directory: `rml-video-design.json`
(input ZIP/HTML digests), `rml-video-ui-final/result.json` and screenshots,
`rml-video-live-results-v2.json`, `rml-video-live-v2.log`,
`rml-video-restart-v1/result.json`, `rml-video-modern-regression/result.json`,
`rml-video-mapping-regression/mapping-window-ui-result.json` and
`rml-video-mapping-regression/check-count.txt`. Hashed `rml-video-steps` receipts
preserve the syntax, preset, layout and DPI-automation failures and their remedies.
The source/binary handoff map is `rml-video-qualification.json`.

Compatibility and lessons: RmlUi needs explicit block containers and text spans
inside flex buttons for this standalone HTML-derived design. Cross-process pointer
tests must use the target window's DPI coordinate space. The preview reuses the
existing video.ini format and original renderer binary; no simulation, input,
audio, dependency pin or game save format changes. Exclusive fullscreen remains
unimplemented, so only Windowed and Borderless fullscreen are shown. Broad game
rendering acceptance and the previously recorded pacing limitations remain open.


### RmlUi desktop preview handoff

Implementation: `5527611ea10b247dbb2df46ca1870a8d93d365f4` on
`research/high-refresh-20261011`. Rebuilt launcher SHA-256:
`99a1d6360da2ace30d3d1eff57c526d2fc65bd8aaa5d6ffc3b545546d7b0ecc2`.
The complete package at
`C:\JFG-builds\high-refresh-rml-video-preview-20261011` passes the final real
menu/persistence/display/crash-recovery fixture after embedding that source
revision. Package runtime/DLL hashes match the retained verified manifest. The
test-only launcher is absent from the delivered directory.

Desktop **JFG - High Refresh Preview** now targets this package and reuses the
previous isolated preview profile. Packaging changed none of that profile's
files. The prior shortcut is retained privately for rollback. Close an existing
launcher before testing, then open Video > Video settings. No installed/default
save, normal launcher process or dependency checkout was replaced. No push or
release occurred. Rendering acceptance remains bounded as recorded above.

All coding, builds and replay work used the same production-guarded investigation
and retained all original charges. The renewed window does not reset counters;
read the original operational ledger and `rmlvideo-progress.json` before further
autonomous investigation. Source/binary receipts and failed attempts are preserved.


## Video Settings initial placement (2026-10-11)

Owner report: Video Settings does not open centered over the launcher. Affected
local RmlUi preview: implementation 5527611, launcher SHA-256 99a1d6360da2ace30d3d1eff57c526d2fc65bd8aaa5d6ffc3b545546d7b0ecc2.
Severity: minor UI placement defect; settings remain usable. Reproduction: move
the launcher, then open Video > Video settings. Initial source review finds
CW_USEDEFAULT for both coordinates in FrontendLiveToolEntry; the owner HWND is
supplied but no explicit centering occurs. This is a candidate cause, pending
an actual-window reproduction. The existing integration fixture moves the dialog
for pointer tests and therefore did not check its initial position.

Investigating under the same enabled production guard and unexpired owner
authorization; all previous investigation counters and evidence retained.


Reproduced on actual RmlUi windows at 125% DPI on an inactive test desktop:
all six initial-position checks failed, while the existing settings controls,
persistence and process-recovery checks still passed. Windows cascaded the
dialog at (32,32), (64,64), and later offsets regardless of its owner. Evidence:
private `video-center-before.log` and `video-center-before.json`.

Candidate remedy: initialize Video Settings using its owner's DPI, explicitly
center the outer window rectangle over the launcher, and clamp placement to the
owner monitor's work area. When the owner is unavailable or minimized, center
within that work area. Other live-tool placement stays unchanged. Checking the
position before the pointer fixture resizes it prevents this coverage gap.


Verified in tests: all six actual initial-placement checks pass at 125% DPI,
including normal opening, reopen, recovery reopening, launcher partially beyond
the upper-left and lower-right screen edges, and centered launcher. The complete
settings window stays in the work area. Existing Apply/OK/Cancel, fullscreen,
pause-preference and child-crash recovery checks still pass. Evidence:
`video-center-after.log` and `video-center-after.json`. The same checks fail on
the previous launcher with only placement errors, establishing the regression.
No physical input or owner windows were used. Multi-monitor mixed-DPI hardware
and owner visual confirmation remain pending; this is a local preview candidate.


Delivered for owner retest: implementation `68e3a63f190b9416de1bdc3841be89913633bdc0`;
launcher SHA-256 `23ac4ffbf597162230754dea96f3095449edba8f7ff82f848c28c619ddfb81d2`. The final rebuilt package repeats
all six initial-placement checks and existing dialog integration with zero
failures (`video-center-final.log`). Desktop **JFG - High Refresh Preview** now
opens the centered-window preview, retaining the previous isolated save/profile.
The previous package and shortcut backup are preserved. Source/binary/shortcut
correspondence is recorded in private `video-center-qualification.json`.
No game renderer, simulation or save data changed. Owner visual confirmation is
pending; no release or installed update was made. The production guard remains
enabled and all investigation charges are retained.


## Video Settings chrome and preset outlines (2026-10-11)

Owner annotated the centered preview: remove the duplicate in-content titlebar,
OK button, idle/unsaved status at bottom left and boxed aspect-ratio note; Apply
should be the accepting action. The FPS preset pill outlines show broken thin
pixels. Severity: minor visual defect; accepting-action change is owner requested.
Affected implementation 68e3a63. Evidence: owner screenshot and existing 1dp native
rounded borders. The modern-controls UI already solved the same fractional-DPI
artifact with supersampled nine-patch frame artwork (`modern-controls-ui.md`,
Assets and qualification). Reuse that embedded artwork rather than alter the shared
renderer or introduce a dependency. Candidate work pending visual and interaction
checks. Preserve load/save errors in a dedicated, normally hidden content notice.
This continues the same enabled guard and authorization, retaining all charges.


The first changed-action integration run passed persistence, closing, all other
controls and centering, but failed its blanket expectation that closing always
clears pause. Source review confirms `refreshPause` retains the independently
enabled Pause when inactive reason; the fixture runs on an inactive desktop.
Previously Apply kept the dialog open until the fixture disabled that preference.
The corrected assertion requires the closed dialog, enabled owner and exact
pause state implied by the saved preference. No runtime pause behavior is changed
or test waived. The original failed log is `video-polish-frontend.log`.

The existing UI checks pass (120) and captures at 100%, 125% and 150% show the
removed chrome and continuous preset frames. With that chrome removed, the
initial client height is reduced from 700 to 600 logical pixels to avoid excess
empty space; resizing/scrolling and centering behavior remain supported.


Verified candidate: 120 UI checks pass, including unchanged Apply accepting,
changed Apply persisting and closing, save failure retaining the draft and
showing an error without closing, validation, keyboard navigation and real
preset pointer hits at 100%, 125% and 150% density. All three final captures were
inspected; RmlUi logs contain no parse warnings/errors. The new compact window
keeps the native OS title bar, removes the duplicated in-content header and
aspect-ratio note, and has only Cancel and Apply in its footer, with no idle or
unsaved status. Selected/hover/focus presets reuse the controller UI's existing
embedded supersampled frame atlas; labels and hit targets remain native.

The real launcher integration passes persistence, closing/reopening between
Apply operations, immediate fullscreen/windowed and pause preference changes,
Cancel, child-crash recovery and nine initial-placement checks (including
fullscreen owner and screen edges). Evidence: `video-polish-ui-final/result.json`,
its `dpi100.png`, `dpi125.png`, `dpi150.png`, and `video-polish-frontend-final.log`.
Owner visual confirmation remains pending. No new game replay is needed for
these launcher-only changes; the renderer/restart qualifications remain those
of the unchanged runtime. Lesson: accepting a dialog must preserve independent
pause reasons; UI artwork qualification includes fractional scaling and hits.


Delivered preview: implementation `fc9099c2d754b0b953e9b602de581ddc0aa34200`; launcher SHA-256
`236f345406692882a0e57422894601df96a386d4efe32df8a8691f82df7509f0`. Final rebuilt package passes all launcher integration
checks, including accepting Apply and nine placement observations, with zero
failures (`video-polish-final.log`). Desktop **JFG - High Refresh Preview** points
to this package and retains the same isolated profile. The previous package and
shortcut are preserved for rollback. Evidence/source/binary correspondence is
recorded in `video-polish-qualification.json`. Owner confirmation remains pending.

The first packaging wrapper stopped before executing on an unterminated Python
string for README text. Its immutable failed receipt is retained as step 05;
step 06 corrected only that wrapper string and completed the same build and final
checks. The product source was unchanged by this packaging correction. No user
windows or save files were modified; no push or release. The original production
guard remains enabled with every prior charge preserved.


## Window-size dropdown text alignment (2026-10-11)

Owner screenshot: "Keep current size" sits above the vertical center of its
dropdown in the simplified preview (fc9099c). Severity: minor visual defect.
Reproduction: open Video Settings with Windowed selected and default window
size. The video select uses 7dp vertical padding but its internal selectvalue
uses the default font line box. The controller toolbar has an existing remedy:
zero vertical padding and explicit matching control/value line-height. Reuse
that sizing treatment for the 38dp video select, preserving horizontal text
alignment and aligning the arrow to the same control height. Candidate pending
scaled screenshots and existing dropdown keyboard/pointer checks. Guard remains
enabled; same investigation, authorization and all accumulated charges retained.


Candidate verified with the existing 120 UI checks (including dropdown pointer,
keyboard selection and Escape handling) and inspected screenshots of the exact
"Keep current size" label at 100%, 125% and 150%. Selected-value line-height and
height now match the dropdown's 36dp inner height; vertical padding is zero,
and arrow placement follows that inner height. Popup options retain a separate
compact line-height. Evidence: `video-align-ui/result.json`, `dpi100.png`,
`dpi125.png`, `dpi150.png`; RmlUi parse log clean. This reuses the controller
toolbar's alignment remedy without changing selection values or settings format.
Owner visual confirmation remains pending. Lesson: RmlUi selectvalue is its own
line box and must be aligned explicitly, not positioned only with parent padding.


Delivered alignment preview: implementation `be8af3c5bc70c715f374d0e2526314feebbd2199`; launcher
SHA-256 `b8050a0018b49ac6514bc7fde6e21ca7fc20ca478b0ce2991e110a82cfc4462b`. The rebuilt package passes the existing real
launcher integration, including settings persistence, Apply/Cancel, nine
placement checks and process recovery (`video-align-final.log`). The desktop
**JFG - High Refresh Preview** shortcut now opens this package with the same
isolated profile. Previous package and shortcut retained; no save/profile writes.
`video-align-qualification.json` records source, binaries and evidence. Owner
confirmation remains pending. No renderer, game timing or dependency changes.
No push/release; production guard enabled with all investigation charges retained.


## FPS preset frame side notches (2026-10-11)

Owner screenshot shows inward notches on both sides of the FPS preset buttons
in the aligned preview (be8af3c), especially highlighted 60 and 120. Severity:
minor visual regression in the antialiased frame change. The earlier full-window
captures and functional checks did not qualify these edges closely enough.
Candidate cause: seams in the nine-patch frame on these short buttons; the pinned
decorator rounds its inner patch positions. Retain the supersampled frame atlas
but render each small preset as one image quad, eliminating internal patch seams.
The controller UI's larger nine-patch controls remain unchanged. Candidate pending
visual comparison at fractional DPI and existing pointer/keyboard checks. Same
production guard/investigation, no counter or authorization reset.


Verified candidate: rendering the same supersampled atlas frame as one image
removes the side indentations in inspected 100%, 125% and 150% captures. This
isolates the visual regression to the split nine-patch rendering on these short
buttons; the atlas, renderer, text and hit targets are unchanged. The exact GPU
sampling contribution to those seams is not independently established. Existing
120 UI checks pass, including decorated preset pointer input at all three
densities. Evidence: `video-border-ui/result.json`, `dpi100.png`, `dpi125.png`,
`dpi150.png`; parse log clean. Prior captures remain as historical evidence and
must not be cited as proof that these side edges were already correct. Owner
confirmation is pending. Compatibility: only these small FPS buttons switch to
whole-image rendering; larger controller nine-patch controls keep their layout.


Delivered border preview: implementation `0f83a2afd458122bdc6fa149c29ebe6da828fe32`; launcher
SHA-256 `d5cdf5aaf28343ef2b2f34e05f102f87f0caa6a7b7c20c8dda24ee71fa9426fb`. Final rebuilt package passes the existing real
launcher integration with zero failures (`video-border-final.log`). The desktop
**JFG - High Refresh Preview** shortcut opens this package with the same isolated
profile. Previous preview and shortcut are retained. Source/binary/evidence map:
`video-border-qualification.json`. Owner visual confirmation remains pending;
no runtime, game timing, dependency or save/profile changes and no push/release.

This guarded job is the twelfth normal attempt of the same investigation, with
eleven supporting activities already charged at admission. Limits remain twelve
attempts and twelve activities; completion grading therefore reaches that limit.
Read the operational ledger and `videoborders-progress.json` before any further
autonomous work. Do not reset counters, reopen a shelved investigation, rename
the work for a fresh allowance or execute directly after denial. The production
guard stays enabled and all evidence is preserved.

## Shared-transform tearing correction

The Goldwood bridge exposed cracks when adjacent batches sharing a matrix used
different interpolation decisions. The tested candidate makes their motion
consistent; see [RENDER-002](character-seams-lighting.md) for cause, regression,
muted replays, reference captures and the remaining lighting/Original reports.


## Presentation pacing and diagnostics follow-up

[PERF-002](frame-diagnostics.md) records the owner's F8-off stutter result,
misleading GAME label, baseline 20-Hz source cadence, normal 30/20 transition
interpolation and verified correction for replaying completed image sequences.
The final candidate retains a paced repeat window and uses alternating GPU
intermediate-target banks. Its 120/360 output timing and original scene progression
pass the muted fixture; Unlimited saturation and owner live confirmation remain
limits. The former UI-only status is not broad high-refresh acceptance.

## 2026-10-14 continuation checkpoint

CPU/history work savings and numeric/equal-slot camera-history corrections are
recorded in [the feature notes](../../features/high-refresh-rendering.md#combat-and-camera-continuation-2026-10-14-source-candidate)
and [qualification](../../../evidence/high-refresh-combat-focus.json).
These remain a source candidate: extended native failures block a new desktop
delivery. Combat/damage drops, a few camera holds, exact late-room visuals,
shadow cadence and Unlimited pacing remain open. The production guard and
same investigation/counters remain intact; this checkpoint does not authorize
another attempt or extend the owner's deadline.


### Unlimited stress limit during reticle qualification (2026-10-16)
The candidate native-clock Unlimited probe reached about 1,007 output FPS but only about 23 scene updates/s in the measured late window, with 265 audio underruns overall. This failed the qualification gate; it is not counted as a clean pass. The installed pre-reticle runtime comparison (pistol Aim, same recorded inputs/profile) reports 281 underruns; its measurements are preserved in shuriken-reticle-20261016/unlimited-installed-control. This comparison uses the native-clock probe, not normal live playback-demand audio, and different reticle drawing prevents an exact cost comparison. The 120/360 reticle tests remain separate, with zero measured underruns and two-VI updates. Unlimited pacing remains open; do not present high output FPS as proof of healthy simulation/audio.
