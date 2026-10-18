# Runtime fix log

Confirmed defects in the native runtime, renderer and audio path, with their
cause, correction and validation scope. Each validation covers the recorded
scene and build; none establishes campaign-wide or pixel-exact parity.
ROM-derived captures, saves, traces and executables remain private.

- Boot and runtime: [CIC-6105 boot state](#cic-6105-boot-state-health-and-ammunition),
  [room 54 sound-queue crash](#room-54-sound-queue-crash-and-hang)
- Rendering: [invasion presentation](#invasion-presentation-gaps),
  [options menu and widescreen](#options-menu-brightness-and-stock-widescreen),
  [planet icons](#planet-icons-in-stage-select-and-tribal-statistics),
  [player shadow](#player-shadow-darkness),
  [particle textures](#particle-rectangles-after-framebuffer-reuse),
  [opening asteroids](#opening-asteroid-textures),
  [opening lens flare](#opening-lens-flare-occlusion)
- Audio and timing: [invasion audio](#invasion-audio-pauses),
  [Vela unlock](#vela-unlock-graphics-yield-and-audio-continuity),
  [opening cutscene audio](#opening-cutscene-audio-interruptions),
  [Quarry timing and original guest timing](#quarry-audio-delay-and-original-guest-timing)

## Boot and runtime

### CIC-6105 boot state (health and ammunition)

**Symptom.** Health drained while idle and the equipped weapon's ammunition
could be cleared.

**Cause.** The native cartridge-entry runner omitted retained CIC-6105 boot
state. The player update at `0x80032B8C` calls the original RAM check at
`0x09C00000`, which reads uncached `0xA00002E8`; with the boot residue missing
it periodically subtracts `0x40` from health. A check at `0x80033440` expects
`osCicId` at `0x80000310` to identify CIC 6105; a mismatch permits clearing
the ammo counter.

**Fix.** `initialize_6105_handoff` sets the CIC ID and copies three four-byte
IPL3 residues from the validated local ROM once, before generated code runs.
The original checks still execute. The adapter contains offsets only; the
IPL3 bytes stay in the user's ROM. This is a minimal cartridge-handoff model,
not execution of the complete hardware IPL.

**Validation.** The same 3,674-poll recording, save and Pak ended with 8,192
raw health and 0/100 pistol ammo on the old build, and full 8,704 health and
100/100 on the corrected build; both high-RDRAM checks reported success.
Synthetic tests cover byte order, bounded initialization and preservation of
unrelated memory. Previously saved health or ammo values are not rewritten.

### Room 54 sound-queue crash and hang

**Symptom (2026-10-03).** A long navigation run exited with native code 4 at
guest `0x8009A710` in room 54. The `unresolved-dma` suffix named the last PI
transfer, not a DMA failure.

**Cause.** A missing indirect `osSyncPrintf` output callback. Restoring it
exposed a hang at VI 9561: the 200-entry sound pool had filled, dropping the
periodic event at VI 9555, and the sound player then looped on NONE events
with zero delay after its queue drained.

**Fix.** The host supplies the output callback after checking its loaded
instruction identity and argument-home range. A narrowly qualified recovery
(US sound player, empty queue, NONE result, prior overflow, valid interval)
delivers one periodic event; queue links, order, counts, overflow count and
high-water mark are unchanged. Other queues and malformed states are untouched.

**Validation.** The exact failing input, replayed headless and rendered, exited
with code 0 at VI 13,980 after exactly one recovery. Release regression tests
cover the callback ABI, recovery and every rejection path in both guest-memory
layouts. This does not prevent sound-pool overload or recover events the guest
already dropped.

## Rendering

### Invasion presentation gaps

**Symptom.** Multi-second visual stalls during the Goldwood invasion while
guest updates and audio continued.

**Cause.** The runner treated an eight-entry retained-snapshot cache as proof
that RT64 still owned a display target. Effects targets (ten distinct color
targets in the scene) evicted both display buffers, suppressing presentation.

**Fix.** Presentation queries RT64's frontend framebuffer registry for a
resident color target; the retained-snapshot cache was removed. Opt-in
diagnostics record `present-skip` events.

**Validation.** On the same 3,538-poll replay, presentations rose from 287 to
422 of 424 invasion updates and the maximum in-scene gap fell from 2,194.5 ms
to 57.5 ms, with identical final 4 MiB RAM. Loading gaps between scenes are
not covered.

### Options menu, brightness and stock widescreen

- **SFX panel bars and background missing.** The snapshot walker kept a
  segmented pointer into a synthetic overlay instead of substituting its
  shadow, so RT64 retained the previous texture combiner. Translated
  display-list edges now use their absolute shadow for submission and traversal.
- **Menu too bright.** `osViSetSpecialFeatures` recorded the gamma-off request
  without updating the VI control register. Feature requests now update gamma,
  gamma dither, divot, dither filtering and antialias bits with libultra semantics.
- **Stock widescreen aspect.** The game keeps a 320x240 buffer scanned into a
  320x180 region; presentation fitted the source dimensions. A hash-checked
  adapter for the pinned RT64 VI renderer fits the active scanout aspect; the
  upstream checkout is unchanged.

**Validation.** Replays show all thirteen SFX volume bars, the meters and the
background, and lowering the volume reduces the meter. Toggling widescreen
switches output between 4:3 and 16:9. The invasion replay's PCM and final RAM
were unchanged.

### Planet icons in stage select and Tribal statistics

**Cause.** RT64 restored each overlay's initial image, including zero-filled
BSS, for every graphics task, so the world-icon renderer's runtime-built
meshes were empty.

**Fix.** Submission copies current DATA/BSS from each published synthetic
overlay allocation (text stays cached) and translates relocated pointers into
the RT64 shadow, with bounds checks. Simulation memory is never modified.

**Validation.** Before/after replays of both screens show Goldwood; automated
pixel checks distinguish the restored planet from the empty panel.

### Player shadow darkness

**Cause.** The game renders an intensity silhouette, then blurs it and reduces
its opacity on the CPU. Normal play kept the rendered image in RT64's private
RDRAM, so the CPU processed zeros and the raw dark silhouette was displayed.

**Fix (2026-10-04).** Completed 8-bit intensity color buffers are committed to
guest RAM before task completion (original-timing builds later extended this
to all color and depth output; see [architecture](architecture.md#graphics)).
Ownership checks reject conflicting CPU writes, and committed pixels update
the snapshot baseline so later CPU edits, including writes back to zero, apply.

**Validation.** Juno's tutorial room produced processed mask values 0-8,
matching an Angrylion/LLE boot (previously 0 and 142). Other characters remain
unverified.

### Particle rectangles after framebuffer reuse

**Cause.** RT64's private RDRAM can retain GPU-written bytes after a
framebuffer is discarded and its storage reused for textures. The importer
copied only bytes that differed from the previous CPU snapshot, so equal-valued
texture bytes left stale pixels and transparent particle texels became rectangles.

**Fix (2026-10-05).** Snapshot replacement refreshes bytes outside RT64's
tracked, written framebuffer ranges from the submitted CPU snapshot. Live
framebuffers and intensity-mask writeback stay protected. There are no
texture- or character-specific exceptions.

**Validation.** The recorded Vela water splash (30,642-VI replay) renders
transparently with identical final CPU memory. The maintainer confirmed dust,
pickup sparkles, ship exhaust and Tawfret rain. Juno's water splash is untested.

### Opening asteroid textures

Reported and investigated by Luke Deardoff (@lukedeardoff) in
[PR #9](https://github.com/TK22-26/JetForceGemini-Recomp/pull/9); his trace of
the incorrect texture addresses guided the diagnosis.

**Cause.** The original RSP handler for the F3DDKR `BF` DMA-base command also
clears the texture-offset enable. Our handler omitted that reset, so a later
asteroid could reuse a previous model's texture-offset table and sample
unrelated memory (colored noise).

**Fix (2026-10-08).** Commit `500b2a2` clears `texture_offset`,
`texture_shift` and `texture_count` at that command boundary. The proposed
40-entry cap was not used; the command's reset semantics are the rule.

**Validation.** Replays through VI 3150 found 115 stale address shifts before
and none after; 28 of 129 consecutive frames changed. EmuHawk/Ares64 shows the
same rock textures (appearance, not pixel parity). The owner accepted the
sequence. Evidence: [opening-asteroid-texture-reset.json](../../evidence/opening-asteroid-texture-reset.json).

### Opening lens-flare occlusion

**Cause.** F3DDKR counted display lists can submit raw RDP `EF` mode commands,
which update the effective RDP mode but not RT64's cached RSP mode stack.
`RSP::drawIndexedTri` used the stale stack to extend depth-write bounds, so
depth written on the GPU was omitted from the CPU readback and the game's
visibility test treated an obstructed light as visible.

**Fix (2026-10-09).** Use the effective RDP depth-write bit, through a
hash-pinned CMake patch that generates the corrected translation unit without
modifying the pinned checkout. No flare suppression or scene timing change.
A depth trace adapted from Luke Deardoff's community diagnostic located the problem.

**Validation.** A regression covers all four RSP/RDP depth-state combinations
and fails on unmodified RT64. Across 181 consecutive presents, 50 frames changed
(1217-1266), matching EmuHawk/Ares64 reveal behavior; the owner confirmed the
fix. Evidence: [opening-lens-flare-depth-bounds.json](../../evidence/opening-lens-flare-depth-bounds.json).

## Audio and timing

### Invasion audio pauses

**Cause.** The host device paused below its 75 ms low-water mark, which the
underrun counter did not record. Graphics preparation rebuilt immutable
ROM-derived overlay shadows and relocations for every effects task.

**Fix.** The initial upper-memory shadow template is cached once per runtime
(bounded to 4 MiB) and each shadow extent is restored before traversal.
Thresholds, sample rate and samples are unchanged. The gameplay trace now
records audio queue, start, pause-low, resume and underrun events.

**Validation.** Interior low-water pauses fell from 12 (971.8 ms) to zero and
median graphics preparation from 1.570 to 0.371 ms per task, with byte-identical
PCM and final RAM.

### Vela unlock: graphics yield and audio continuity

Released in 0.4.0-preview.4: graphics completion follows the game's yield
request during the Vela unlock sequence, and nonempty audio queues keep
playing while retaining startup buffering and underrun recovery.

### Opening cutscene audio interruptions

**Cause.** Sustained host CPU work exhausted the pacing margin. When the VI
loop ran more than about 66.7 ms late it reset its deadline while the device
kept playing, draining the queue (270 ms discarded across four resets in a
profile). Live map exports exposed the overhead.

**Fix (2026-10-09).** Changed snapshot bytes merge in SIMD batches, CPU refresh
skips blocks already identical to renderer memory, and writeback ranges are
validated and copied in bulk. Conflict checks still complete before any write;
the scalar path remains. Exports stay enabled.

**Validation.** Mean snapshot import fell from 1,088 to 908 µs (p99 2,749 to
1,513) and mean writeback from 52.9 to 6.3 µs. Three full-intro replays through
VI 12000 had zero underruns or overruns, no output gaps of at least 6 ms, and
byte-identical PCM. The owner confirmed the result. Evidence:
[opening-cutscene-audio-continuity.json](../../evidence/opening-cutscene-audio-continuity.json).

### Quarry audio delay and original guest timing

**Symptom (2026-10-08).** Quarry entry cutscene audio ran up to about one
second behind the picture, with small underruns as gameplay resumed.

**Causes.** Several independent contributors:

- HLE shortcuts skipped original CPU, cache and OS work while music continued.
- Audio queued during renderer startup persisted as an output offset.
- After a host stall (1.227 s in the owner's trace) the fixed VI wall-clock
  origin stayed in the past; the game caught up and kept about 1.23 s queued
  for the rest of the run, below the 2 s overrun threshold.
- Synchronous recording, status, map and volume-file work, and Windows message
  handling (stalls of 119.8 and 80.5 ms), blocked the emulation thread.
- In the combined build, the function dispatcher read a test-only environment
  variable on every call, saturating the CPU during guest polling loops.

**Fixes.**

- Fresh generation instruments original OS and game instructions with the
  qualified cache/device clock model (see the
  [timing-model notice](../legal/dependency-license-inventory.md#original-runtime-timing-model));
  the original OS owns interrupts, queues and threads, with AI/PI models and
  paired-FPR handling.
- Initial playback drains to the startup cushion before setting the wall-clock
  origin, and `ViPlaybackClock` moves the origin forward after a new underrun.
- Window messages run on the window-owning thread with an input mailbox;
  volume reads, diagnostics and map snapshots no longer block guest execution;
  cooperative wakeups target the next participant.
- Predecoded instruction metadata and a small common instruction path reduce
  host overhead with unchanged guest costs; the test-only option is read once.
- Recorded-poll replay preserves two controller polls in one retrace.

No scene-specific waits, audio padding, audio drops or guest-clock changes
were added.

**Validation (2026-10-09).** A fresh ROM-to-build passed two full opening runs
and one Quarry cutscene/gameplay run at 1920x1080 with zero underruns, no
output gaps of at least 6 ms, and a maximum host presentation-interval excess
of 9.7-15.5 ms (never over one VI). Generated PCM matched the pre-optimization
replays; the 81 ROM-free C++ tests and renderer regressions passed. These are
host timestamps and WASAPI captures, not scanout or whole-campaign results.
Evidence: [unified-original-timing-validation.json](../../evidence/unified-original-timing-validation.json).
