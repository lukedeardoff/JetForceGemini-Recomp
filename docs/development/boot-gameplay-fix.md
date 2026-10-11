# Boot state correction for health and ammunition

The native cartridge-entry runner omitted retained CIC-6105 boot state. The
original game checks consequently removed health while idle and could clear
the equipped weapon's ammunition. The correction initializes the missing
boot state once, before generated game execution.

## Cause and implementation

The generated player update calls the original RAM check at guest address
`0x09C00000`. It reads uncached RDRAM `0xA00002E8`; when the boot residue is
missing, the player update at `0x80032B8C` periodically subtracts `0x40` from
health. A separate check at `0x80033440` expects `osCicId` at `0x80000310` to
identify CIC 6105. A mismatch permits clearing the equipped ammo counter.
Both locations were zero in the preserved live session.

`initialize_6105_handoff` initializes the CIC ID and copies three four-byte
IPL3 residues from the already validated local ROM. The two high-RDRAM words
are consumed by the original startup RAM checks. The existing generated
checks continue to execute. The public adapter contains offsets and device
metadata; the IPL3 bytes remain in the user's ROM. This is a minimal supported
cartridge handoff model, not execution of the complete hardware IPL.

## Verification

The same 3,674-poll recording, initial save and Pak were run against both
binaries, ending at the same stationary player position in the training area.
The route includes 1,500 appended neutral polls. The old build ended with
8,192 raw health and `0/100` pistol ammo; the corrected build retained full
8,704 raw health and `100/100`. Both high-RDRAM startup checks reported success
with the correction. All 66 native tests and a replay containing six additional
Start presses passed. Synthetic tests cover byte order, bounded initialization,
and preservation of unrelated memory without shipping ROM data.

The original live process was left running. Its recording, saves and four
non-atomic memory snapshots were preserved privately. The running process
does not acquire this correction until restarted. Previously saved ammo or
health values are not rewritten by the boot adapter.

## Initial visual investigation

The first Goldwood invasion stutter observation was inconclusive. A paced,
unskipped intro observation reached 9,800 VI retraces without a trap. Of 4,545
presentation gaps, 4,533 spanned two VI retraces; larger gaps also occurred.
The first observation used an actor sampler that only recognizes `animBlue` and captured no matching
actors on this route. The final scene still contained the ship's animated
characters. These observations do not isolate the reported invasion moment or
distinguish its animation cadence from rendering delays.

An earlier paced diagnostic completed in the child but was rejected by the
parent because the optional final actor dump added a console record. That
rejection is retained separately and is not a passing regression result.
The boot correction has not been established as a fix for the visual issue.

## Detailed local gameplay recording

Set `JFG_GAMEPLAY_TRACE` to a writable file path before starting the native
runner to enable the opt-in `jfg-gameplay-trace-v1` timeline. Logging is off by
default. Rows contain decimal elapsed microseconds, an event name, and hexadecimal
integer values. Float guest fields retain their raw IEEE-754 bit patterns.
The trace is buffered, flushed once per second and at controlled exits, and stops
at 256 MiB with a limit marker. A hard crash can lose the final buffered interval.

The events cover controller input, VI ticks, completed guest updates, graphics
preparation/submission/presentation timing, audio queue levels and underruns,
scene state, and changed actor state. Actor sampling covers every type when the actor table contains at most 256
entries, including the ship characters omitted by the first probe. Larger tables
produce an `actor-table-invalid` event and skip actor sampling for that update.
Actor rows include name words, the first 128 actor bytes and the first 32 bytes
of both properties and control data when readable. Unchanged actor snapshots
are omitted. These raw guest snapshots are private diagnostic data and are not
part of the launcher's filtered support ZIP.

The authoritative payload order is at each `gameplay_trace.event` call and in
`trace_gameplay_state` in `src/boot/native_boot.cpp`. Tick/update rows include
VI, input poll, diagnostic update count, frontend mode, level, actor count,
presentation/graphics counters, pending work, dispatch count, and audio status.
Actor rows begin with VI, update, slot, guest address and four name words,
followed by 32 actor words, eight properties words and eight control words.
An invalid guest read is encoded as `ffffffff`.

A paired 2,300-poll replay with tracing disabled/enabled produced identical
final 4 MiB RDRAM, full health and 100/100 ammo. The enabled trace contained
36 actor types and 15,831 changed actor snapshots in about 6.4 MB. Focused
tests passed, including trace size-limit behavior. This checks data coverage
and guest-state preservation; it does not establish zero timing overhead or
identify the reported visual stutter. The subsequent manual recording and
matched invasion replay are described below.

## Invasion presentation correction

The manual recording captured two in-scene presentation gaps of 2.61 and 2.41
seconds while guest updates and audio continued. During these gaps the game
submitted ten distinct color targets: two display buffers plus effects targets.
The native runner incorrectly used an eight-entry retained-snapshot cache as
proof that RT64 still owned a display target. Effects repeatedly evicted both
display-buffer entries, suppressing presentation even though RT64 retained the
rendered framebuffers.

Presentation now queries RT64's frontend framebuffer registry for a resident
color target. The unused retained snapshots and their eviction bookkeeping are
removed; consumed snapshots return to the existing reuse pool after submission
and any requested writeback. This does not replay historical graphics tasks or
raise the cache limit. Opt-in diagnostics record `present-skip` with VI, input
poll, update, selected display address and last submitted color address when the
selected target is unavailable.

Both binaries replayed the same 3,538-poll prefix and pre-recording
save/Pak, including the complete invasion. The old build had
287 presentations over 424
invasion updates; the corrected build had 422 over
424. Maximum in-scene presentation gaps were
2194.536 ms before and
57.500 ms after. The corrected replay had
0 gaps above 100 ms,
0 missing-target events, and no additional audio
underruns during the invasion. Final 4 MiB game memory matched byte for byte.

The renderer test, four focused native tests, and the stationary health/ammo
replay passed. These results cover this recorded scene and presentation
regression; they do not establish campaign-wide rendering correctness or
eliminate loading gaps between scenes. The corrected local runtime is prepared
for visual confirmation. Private logs, game memory, saves and binaries remain
outside the public repository.

## Audio follow-up after the visual correction

The user confirmed smooth visuals but reported brief audio gaps in the invasion.
The host audio device was proactively pausing below its 75 ms low-water mark and
resuming at 150 ms. These gaps did not increment the empty-buffer underrun count,
so the earlier underrun-only check did not establish uninterrupted playback.

The same fixed-input replay with PCM/event capture reproduced
12 low-water pauses in the interior of the invasion,
totalling 971.8 ms. Graphics preparation repeatedly
rebuilt immutable ROM-derived overlay shadows and their relocations for every
effects task. Cache the initial upper-memory shadow template once per runtime,
then restore each shadow's exact extent before that task's display-list traversal.
This preserves per-task snapshots and traversal while avoiding repeated byte
conversion and relocation. It adds a bounded 4 MiB private template; it does not
change the audio thresholds, sample rate, or generated samples.

The corrected paired replay had 0 low-water pauses and
0 underruns in the same interior window. Median graphics
preparation time changed from 1.570 ms to
0.371 ms per task. The maximum in-scene presentation
gap was 55.485 ms. Generated PCM and final 4 MiB guest
memory matched the baseline byte for byte. The user subsequently reported a clean
playthrough of Goldwood through SS Anubis before finding the menu issue below.
Raw PCM and playback event traces remain local.

The optional gameplay timeline now includes audio queue, start, pause-low,
resume and underrun events. Their payload is current queued bytes, cumulative
queued bytes, cumulative consumed bytes, sample rate and playing state. This
makes proactive pauses visible separately from underruns.

## Options rendering and stock widescreen

The SFX screen emitted all thirteen volume bars and three small meter bars,
with the correct primitive colors and volume value. Its two-command RDP setup
list pointed into a synthetic overlay whose high address byte also selected an
active scene segment. The snapshot walker kept the segmented pointer instead
of substituting the overlay shadow, so RT64 retained the preceding texture
combiner. The same addressing error suppressed the panel's noise background.
Translated display-list edges now use their absolute shadow for both command
submission and traversal; untranslated edges retain scene-segment resolution.

The overly bright menu was a separate VI omission: `osViSetSpecialFeatures`
recorded the game's gamma-off request without updating the submitted VI control
register. Feature requests now update gamma, gamma dither, divot, dither filtering
and the corresponding antialias bits with libultra's request semantics.

The supported US game's original widescreen setting keeps a 320 by 240 source
buffer and scans it into a 320 by 180 display region. Presentation previously
fitted the source dimensions, losing that display aspect. An owned, hash-checked
adapter for the pinned RT64 VI renderer fits the active scanout aspect while
preserving texture sampling and the game's own camera logic. The pinned upstream
checkout is unchanged. The bottom/right guard-texel clipping uses the same aspect.
A normal window changes width when the guest switches aspect. Maximized or
minimized windows retain their size; presentation fits their client area.

Verification on the final runtime:

- Replayed actual saved gameplay followed by Start, Down, A, Down, Down, A.
  The SFX panel shows all thirteen green-to-red volume bars, the three small
  bars and the textured background. A second replay lowers the volume and
  reduces the main meter from thirteen lit bars to five.
- Replayed stock widescreen off/on; guest mode indices switch between zero
  and one, and captured output changes between 4:3 and 16:9. A live window
  round trip measured 640x480, 853x480, 640x480 and 853x480, then closed normally.
- All 68 ROM-free tests and three focused native renderer/address/VI tests pass.
  Schema validation and repository/history hygiene checks pass.
- Replayed the same complete invasion recording and initial saves. Generated
  PCM and final 4 MiB guest memory still match the earlier corrected runtime.
  There were no interior low-buffer pauses or underruns; the maximum in-scene
  presentation gap was 59.447 ms.

These checks cover the supported US build, the reported menu and the recorded
invasion. They do not claim complete campaign or other-region parity. Screenshots,
ROM-derived data, audio, saves, traces and the locally generated executable remain
private. The player's original SS Anubis session and saves are retained.

## Planet icons in stage select and Tribal statistics

Both screens call the same world-icon renderer. It builds planet vertices and
triangle records in overlay-local writable memory. RT64 previously restored
the module's initial image for every graphics task, including zero-filled BSS,
so the renderer received empty meshes. Ordinary loaded models such as SS Anubis
still appeared, which helped distinguish this from general menu visibility.

After restoring the cached fallback, graphics submission now copies current
DATA/BSS from each published synthetic overlay allocation. Immutable text stays
cached, and inactive allocations retain their fallback image. Relocation sites
in refreshed data translate their current pointer values into the RT64 shadow;
they are not reset to the module's original pointer values. Bounds are checked
before copying or translating, and simulation memory is never modified.

Private before/after replays use identical saved progress and controller input:

- Stage select: Start, Right, A, then Up from SS Anubis to select Goldwood.
  The corrected frame contains Goldwood and the neighboring rocky planet.
- Tribal statistics: Start, Right, Down, Down, A, then Left to Goldwood.
  The corrected frame contains Goldwood above its statistics. Automated pixel
  checks distinguish the restored green planet from the empty baseline panel.
- The SFX replay still shows all thirteen main volume bars. All 68 ROM-free
  tests, three focused native renderer/address/VI tests, schema validation and
  repository/history hygiene checks pass.
- The invasion replay preserves identical generated PCM and final guest memory,
  with zero interior audio pauses/underruns and a maximum presentation gap of
  60.323 ms. Refreshing writable data does not reproduce
  the earlier multi-second stalls in this recording.

Raw screenshots, generated mesh data, saves, traces and binaries remain private.
These checks cover the reported screens and retained regression recordings;
campaign-wide rendering parity remains unverified.


## Player shadow CPU postprocessing

The game renders a small intensity silhouette, then reads that image on the
CPU to blur it and reduce its opacity before using it as an intensity/alpha
texture. Normal native play kept the rendered image in RT64's private RDRAM;
only the optional all-framebuffer diagnostic returned GPU writes to game
memory. The CPU therefore processed zeros while the renderer kept displaying
the raw, much darker silhouette.

Normal play now commits completed 8-bit intensity color buffers to guest RAM
before delivering graphics task completion. Main color and depth buffers stay
private. The existing ownership check rejects intervening CPU writes before
any range is committed. Committed pixels also update the snapshot comparison
baseline, so a subsequent CPU edit back to zero replaces the renderer's old
pixel instead of being mistaken for unchanged input. The original game
performs the blur and opacity reduction; there is no new shadow-opacity preset.

Validation on 2026-10-04:

- Built the release native runtime and passed the renderer regression tests.
  Added GPU-to-CPU-to-GPU mask coverage, including edits back to zero,
  unrelated framebuffer preservation, invalid tracking memory and atomic
  rejection of conflicting CPU writes.
- Booted a copy of the navigation save into Juno's tutorial landing room.
  Twelve live memory samples contained the processed mask with values 0
  through 8. A fresh Angrylion/LLE boot of the same room also produced that
  range; the previous native renderer retained raw silhouette values 0 and
  142. Idle animation phases differ, so this is not a pixel-identical replay.
- The muted native trial reached gameplay and closed normally. Original save
  hashes were unchanged. Private captures retain the before/after render and
  the emulator reference.

This check covers the reported Juno shadow in the supported US build.
Other characters and full-campaign rendering parity remain unverified.
ROM-derived images, memory captures and executables remain private.

## GPU shadow softening

Issue #4 compared both shadow paths against real N64 captures. The CPU path
above processes the 64x64 mask at native resolution, so the result is blocky
when RT64 renders at a higher internal resolution. GPU mode, now the default,
keeps those masks on the GPU at the internal resolution:

- Framebuffers narrower than 128 pixels are not written back, so RT64 keeps
  sampling its own high-resolution copy. Guest bytes in that range are still
  imported whenever the game changes them (the game frees mask memory and
  reuses it for textures); only unchanged bytes keep RT64's copy.
- `cmake/rt64_shadow_soften.cmake` generates hash-checked copies of
  `rt64_framebuffer_manager.cpp` and `rt64_shader_library.cpp`, and
  `cmake/rt64_shadow_soften_shader.cmake` compiles
  `src/renderer/shaders/FbShadowSoftenCS.hlsl` with RT64's own DXC rules. The
  pinned checkout stays pristine. When an 8-bit tile of at most 64 native
  texels is reinterpreted as IA8, that shader box-averages each native texel,
  reconstructs it with a Gaussian (`JFG_SHADOW_BLUR`, default 1.5 native
  texels, maximum 2) and applies the game's opacity (`JFG_SHADOW_OPACITY`,
  default 8/14).
- The opacity matches the game's own conversion. The shadow draw (2-cycle,
  normal alpha blend, no alpha-compare dither; combiner `fc121603 fffffff8`)
  outputs black with alpha = mask alpha x primitive alpha. The CPU pass turns
  raw IA `0x8E` (alpha 14/15) into `0x08` (alpha 8/15).

`JFG_SHADOW_MODE=cpu` restores the CPU-mask path. If the softening pipeline
fails to build on a driver, shadows fall back to the standard reinterpret pass.

Validation scope: Luke Deardoff's Windows build of v1.1.0 with the equivalent
RT64 changes, compared against his own N64 hardware captures (the ship and
Goldwood), including the texture-reuse case above. The CMake
integration produces translation units identical in code to that build. Other
levels and full-campaign coverage remain unverified.

## CPU texture refresh after framebuffer reuse

The renderer's private RDRAM can retain GPU-written bytes after a framebuffer
is discarded and the game reuses its storage for textures. The snapshot importer
previously copied only bytes that differed from its previous CPU snapshot.
A texture byte equal to that previous value therefore left a stale rendered
pixel in place. For intensity/alpha particles, this could turn transparent
background texels into visible rectangles.

Snapshot replacement now refreshes bytes outside RT64's tracked, written
framebuffer ranges from the submitted CPU snapshot. The existing delta merge
still imports CPU changes within those ranges. Live color/depth framebuffer
contents and the intensity-mask CPU writeback path remain protected. The
refresh supports both byte layouts and validates all ownership ranges before
copying; it has no texture-address or character-specific exceptions.

Validation on 2026-10-05:

- A recorded Vela water jump reproduced the rectangular splash. A diagnostic
  CPU texture refresh and then the general ownership fix restored transparency.
- The matching 30,642-VI replay completed normally with exactly the same final
  CPU memory as the broken baseline. Before/after frames show the rectangular
  footprint replaced by the transparent ripple.
- The current native runtime, including the separate graphics-yield and audio
  continuity fixes, was rebuilt and replayed successfully.
- Eight focused renderer, scheduler, graphics/audio bridge, replay, mod and
  playback-buffer tests passed. Ownership tests cover stale same-value CPU
  bytes, protected GPU bytes, both byte layouts, overlapping and unaligned
  ranges, and atomic rejection of malformed ownership.

One earlier validation attempt failed in the background DirectX shader
compiler during a session in which Windows also recorded a desktop compositor
crash. A monitored rerun of that same candidate completed normally; the cause
of the wider failure is not established.

The automated visual acceptance above covers Vela's recorded splash. The
maintainer subsequently confirmed that all previously reported broken particle
effects now render correctly, including dust, pickup sparkles, ship exhaust
and Tawfret rain. Juno's water splash remains untested. Private
recordings, saves, ROM-derived frames and executables are not distributed.
