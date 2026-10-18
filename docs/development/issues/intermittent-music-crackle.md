# AUDIO-001: Intermittent background-music crackle after native timing preview

- Severity: significant audible regression. Status: verified in tests and owner live verification on the local preview; not publicly released.
- Reported: 2026-10-11 UTC, runtime `251700d98118a137fdcbae8af50bdc4351c0276e9c4d0d1c525a65a18505f554`.
- Owner reports gameplay feels very smooth, but intermittent crackle occurs across
  locations, mostly noticeable in background music, on wired/USB audio output.
- Related: [PERF-001](goldwood-combat-slowdown.md). Smooth gameplay does not prove
  uninterrupted audio, nor does this report yet identify the audio fault.


Latest listening evidence: the owner hears clean sound with the port's older
timing path, but still hears static with native elapsed-time timing even when
CPU span optimization is disabled. The verified DMA correction did not resolve
the reported static. A synchronous sample-source repair is now installed in the
local preview after source-integrity and runtime tests. Owner listening and live
verification of that candidate remain pending; details are recorded below.

## Evidence and plan

Earlier hidden startup checks verified PCM duration and nonzero content without
reported queue underruns. Those counters did not verify audible quality or
sample continuity. Audit the source PCM, queue boundaries and host playback
separately; compare reference and native timing, preserve existing saves and
avoid physical input/focus tests. The pinned upstream audio review JFG-UP-024
also cautions that host bridge tests do not establish all guest scheduling rules.

A DMA correctness defect is isolated and corrected below, but the reported audible distortion remains open. Detailed private evidence
is retained in `audio-confirm-evidence-20261011` within the existing guarded
`pc-input-aiming-20261008` investigation.

## Reproduction refinement and eliminated hypotheses

The owner narrowed the report to standing still in Goldwood outside the red-key
door, with no enemies required. Static occurs reliably at the loud drum crescendo.
The pause screen has no music and therefore cannot serve as a music-only check.

AI buffer-lifetime hashes matched at submission and host consumption for all
1,797 buffers in each 60-second native/reference startup fixture. No delayed-copy
corruption was observed in this fixture. Both had continuous queued PCM with no
reported queue underrun. Raw PCM contains more sharp adjacent-sample transitions
under native timing, but different music/SFX timing prevents labeling these as
corruption. Counts alone do not establish audible quality or a root cause.

A separate clean-tone test exposed discontinuities in the bundled SDL 2.26.3
resampler at 22,018 to 48,000 Hz. Official SDL 2.32.10 reduces the measured jumps,
but still has chunk-boundary phase error. This is a playback defect candidate;
it does not yet explain or resolve the reported crescendo distortion. Do not
claim a complete fix from a library replacement alone. Source and results are
retained privately in the existing investigation. The first Goldwood capture
wrapper had a Python quoting error and never launched the game; its corrected
replacement is recorded separately rather than overwriting the failed attempt.

## Owner-identified raw PCM reproduction

The owner listened to `Goldwood-source-before-Windows.wav` and confirmed the same
corrupted sound as idle gameplay. Four distinct bursts begin about 36-37 seconds
in and last roughly 1-2 seconds. The captured route contains combat, so cadence
and musical position differ from the stationary red-key-door case. Preserve that
distinction: this confirms source-audio distortion, not a single fixed musical
position or perfect deterministic gameplay reproduction. Lowering the game's
own music slider merely makes the static quieter.

The WAV wraps the 22,018 Hz stereo signed-16 PCM captured before master gain and
SDL queuing. Consequently, an SDL upgrade cannot be claimed to repair this
source defect. No SDL dependency update was installed. Both same-start native
and reference timing captures contain full-scale samples; different overlapping
sounds make aggregate clipping counts inconclusive. A bounded opt-in fixture
captures up to eight complete DSP tasks around the identified interval for an
independent Mupen HLE comparison. Diagnostic snapshots and ROM-derived data
remain private; they are not distributed with project documentation.

## Captured-task isolation (2026-10-11 UTC)

Eight consecutive audio tasks, indices 6884-6891, were retained with complete
before/after RDRAM and task descriptors. The original generated DSP producer
reproduced every 2,944-byte output exactly, and a second execution repeated those
bytes. An isolated build using the scalar RSP vector-arithmetic path also matched
all eight outputs exactly. This establishes deterministic fixtures and excludes
a SIMD-only arithmetic difference in those fixtures. It does not prove that a
fresh live session reaches the same music/voice state on an identical sample.

The independent Mupen high-level audio producer differed substantially when fed
these continuing-voice states. A command-stage comparison found exact agreement
in the two sampled ADPCM decodes and at most two signed-16 units of difference in
the sampled resampling stages. The larger divergence first appeared at envelope
mixing. A fresh synthetic envelope-command comparison also differed, including
polarity. Neither comparison yet validates the HLE command/state interpretation
against this exact microcode variant, so this is **not** sufficient evidence to
replace the production decoder or reduce its gain. Scalar/native agreement also
does not establish equivalence with original hardware or validate guest command
generation. The continuing-envelope memory layout and original microcode contract
need independent verification.

Informative setup failures remain in the private command journal: the first
output-range selector included disjoint internal reverb saves; the corrected
selector uses final INTERLEAVE/SAVEBUFF pairs. The existing HLE producer requires
8 MiB backing memory, so the guest's 4 MiB snapshot was padded with unused zero
memory for its adapter contract. Synthetic inserted commands initially needed a
matching update to their RDRAM backing bytes. A scalar-only diagnostic build
needed the recompiler headers and a small standalone main because the runtime
adapter does not implement the G2 fallback-test export. No failed setup was
counted as a successful comparison, and the scalar probe is not a new G2
qualification.

An independent BizHawk/Mupen stock-CPU capture reached Goldwood room 21 with
neutral controller input and captured one minute of music. Its isolated fixture
redirects a room transition and uses a copied fixture save, so it is not the
owner's exact idle scene. A 58-second WAV was supplied for listening; the owner reported that it sounds clean. This strengthens the source-generation
hypothesis but does not equate the two different gameplay states. It contained 41 full-scale samples after emulator
output conversion. Clipping counts across different mixes, output rates and
emulators do not identify the original cause.

## Current result and next checks

**Open; a DMA transfer correction is verified in isolated tests, but no audio fix is installed or released.** The preview contains
opt-in PCM ownership and bounded task-capture diagnostics. Normal play does not
copy audio-task RAM or hash PCM buffers. The transfer correction described below changes source-audio DMA behavior; timing and SDL playback behavior remain unchanged. Do not use a limiter, a blanket
volume reduction, a bigger playback buffer or an unrelated SDL upgrade as a
claimed repair for the demonstrated source distortion.

The continuation below validated the captured envelope/DSP execution against
independent instruction-level RSP output and checked original sample/history
inputs. Next, compare guest command generation, voice/event scheduling and the
complete PCM delivery sequence in the idle reproduction. Use the retained task fixtures as
repeatable inputs. A fix must remove the reported distortion, retain intended
music/SFX levels and envelope behavior, and pass both idle and combat listening
checks. Preserve the stock-emulator recording for comparison, with its fixture
limitations attached.

The decoder fixtures concern sample generation, not rendering frequency. Future
60+ Hz rendering must preserve the audio sample clock and voice scheduling;
these tests should remain applicable without tying audio delivery to renderer
frame count. No high-refresh or unrelated investigation state was changed.

Primary playback-source references used during elimination:
[SDL 2.26.3 conversion](https://github.com/libsdl-org/SDL/blob/release-2.26.3/src/audio/SDL_audiocvt.c)
and [SDL 2.32.10 conversion](https://github.com/libsdl-org/SDL/blob/release-2.32.10/src/audio/SDL_audiocvt.c).
These informed a rejected playback-only remedy, not an installed dependency change.

## Guarded continuation (2026-10-11 UTC)

The owner approved two more hours after reporting the stock-emulator WAV clean.
The same production investigation resumed with its guard enabled and all prior
charges preserved. Next: validate envelope-mixing behavior using an independent
instruction-level RSP implementation, then trace earlier guest command/voice state
if the DSP output is correct. No gain limiter or buffer enlargement is a verified
remedy for the source distortion.

### Instruction-level DSP comparison

Built independent [CXD4 RSP](https://github.com/mupen64plus/mupen64plus-rsp-cxd4/tree/00906a92641c540a64e4e7505012157315314810) at revision 00906a92641c540a64e4e7505012157315314810 in private storage. All eight captured tasks (6884-6891) produced byte-identical 2,944-byte PCM output with the same microcode-entry state as our adapter. No HLE audio callback ran. This excludes a generated-instruction execution mismatch for those inputs, including the first reported burst; it does not validate earlier guest sample/voice-state generation. The earlier HLE difference is not evidence for changing DSP arithmetic.

The first reference harness correctly stopped on the emulator's initial HALT bit; the second bootstrap-level attempt timed out. The successful comparison explicitly reproduced the production adapter's entry at IMEM 0x80, loaded program/data/descriptor, and zeroed scalar/vector registers with r1=0xFC0. It does not claim full boot-microcode validation. Only diagnostic initialization/logging was added to the private interpreter; arithmetic and instruction dispatch were unchanged. Private evidence: audio-dsp-evidence-20261011/lle-entry-comparison.json.

### Stock LLE and original sample checks

The owner also reported the stock CXD4 instruction-level WAV clean. Seven complete stock audio tasks replayed through our generated DSP matched the stock RDRAM PCM byte-for-byte. The captured stock and port microcode (4,096 bytes) and constant data (2,048 bytes) also match. All eleven sample loads and predictor books in the first suspect chunk match ROM data. For the two voices with extreme negative peaks, a fresh whole-instrument decode reproduces the live ADPCM history exactly; the native and independent HLE ADPCM decoders agree on the full instrument. Thus large waveform jumps/full-scale sample counts alone cannot identify the reported corruption. The strongest isolated jump is present in an original sample, and must not be mistaken for proof of a decoder bug.

The stock LLE capture reached its success marker, but EmuHawk did not terminate after client.exit. Its completed owned process was stopped after closing the auxiliary Lua window. This nonzero capture exit is retained. The RIFF header had unfinalized length fields; a separate WAV wraps only complete retained stereo frames using the recorded PCM format, without filtering/gain changes. The owner listened to that separate WAV. Evidence and failed setup results remain private.

### Guest command-generation comparison in progress

A private CPU harness links the existing generated baseline and runtime dispatcher, loads captured stock RDRAM and full integer/floating-point register state, and invokes the original envelope-command builder. Its execution probes do not advance devices; this tests function arithmetic/memory effects, not whole-system scheduling. Capturing paired stock function entry/return states allows exact command comparisons without inventing a synthetic voice state. An unhandled guest operation fails closed. Results remain pending until the captured output is compared.

Primary upstream issue searches also found general [Mupen audio popping](https://github.com/mupen64plus/mupen64plus-core/issues/571) and [renderer-dependent crackling](https://github.com/mupen64plus/mupen64plus-core/issues/972) reports. Neither establishes this issue's cause or a transferable patch. Their symptoms alone do not justify changing our buffers, gain or decoder.


### Confirmed RSP DMA transfer defect; candidate correction

Widening the retained capture to tasks 6928-6935 exposed a real mismatch in task
6932 (VI 13871). Seven tasks matched the independent instruction-level RSP
interpreter in all 4 MiB of RDRAM; task 6932 differed in 18 signed-16 PCM samples
(maximum absolute difference 85) and 38 RDRAM bytes. Native and scalar producers
both reproduced the original runtime result. This is a transfer problem, not a
SIMD arithmetic difference.

A command-prefix comparison was exact through commands 0-105. Command 106,
LOADBUFF, requests 366 bytes of reverb history. The old broker copied exactly
366 bytes, but SP DMA transfers whole eight-byte bursts and copies 368. The
missing tail contained signed sample 636; our scratch memory retained zero.
The next resampler consumed that incorrect history. This is reproducible with
a single retained task, independently of playback, Windows, enemies or gain.

The correction in [audio_dma_broker.hpp](../../../include/jfg/runtime/audio_dma_broker.hpp)
rounds inclusive transfer lengths to complete eight-byte bursts for both reads
and writes, aligns both starting addresses to eight bytes, and wraps transfers
within DMEM. The supported single-row DMEM contract remains explicit: count/skip
fields and IMEM transfers fail closed. Existing memory brokerage, transactional
outer commit and output accounting remain in place.
[brokered-rsp.hpp](../../../src/bootstrap/brokered-rsp.hpp) delegates to the same
implementation tested without ROM data. This avoids a private generated-file-only
patch that disappears on the next build.

Validation so far:

- The ROM-free DMA regression test passes with project warnings treated as
  errors. It covers the 366/368-byte tail, zero-length-register eight-byte burst,
  address alignment, rounded writes, a full 4 KiB wrapped transfer, rejected
  modes and failed memory/output accesses.
- All sixteen retained tasks (6884-6891 and 6928-6935) now match the independent
  CXD4 interpreter byte-for-byte in both PCM output and the complete 4 MiB RDRAM
  result. This includes the previously failing task.
- Eleven active stock envelope-builder calls match the generated CPU's complete
  RDRAM result. The twelfth enters OS/device dispatch and reads the MI mask
  register, outside the standalone function harness; it remains unvalidated.
  An earlier batch contained inactive voices and is not evidence for active
  mixing. Complete audio-frame CPU captures were retained but not yet replayed.
- One runtime build invocation used the wrong target spelling; the failed
  invocation is retained and the corrected target is used for the candidate.

**Scope limit:** a maximum difference of 85 in one task does not establish that
this defect explains the owner's harsh static. Reverb state persists, so a fresh
capture from startup is required; replaying a corrected task with already
incorrect history is insufficient to establish listening quality. AUDIO-001
remains open until idle and combat source/playback checks confirm the reported
noise is gone. Do not describe this as a released or owner-verified audio fix.

Primary references: [ares RSP DMA implementation](https://github.com/ares-emulator/ares/blob/master/ares/n64/rsp/dma.cpp)
and [hardware DMA measurements](https://hcs64.com/dma.html), checked against the
independent CXD4 interpreter pinned above. Private evidence:
audio-dsp-evidence-20261011/6932-first-dsp-difference.json,
dma-candidate-reference-comparison.json, stock-active-env-cpu-comparison.json.

Transferable lesson: inclusive hardware DMA lengths are not ordinary byte counts.
Test short and non-aligned transfers as well as final PCM. Persistent effect
buffers can carry an error into later tasks even when those tasks execute their
own commands correctly. This correction is independent of render rate and does
not change audio clock, gain, envelope arithmetic or future high-refresh policy.


### Owner listening result: DMA correction is not the full audio fix

The owner listened to the fresh Goldwood-DMA-correction-combat.wav and reported
that the same noticeable static remains, in different places. This is explicit
negative audible verification. Keep the DMA correction as a verified hardware
transfer fix, but do not label AUDIO-001 resolved or imply that it removes the
reported distortion. Both fresh combat and idle captures completed, and eight
fresh production-runtime task snapshots matched the independent interpreter in
all RDRAM. All 78 portable CTest tests and 9 ROM-bootstrap tests passed.

The full-frame CPU replay reached an MI interrupt-mask register read in all
twelve cases, outside its function-only device model. None is a full-frame
parity pass. The active-envelope 11/12 result remains the limited positive CPU
evidence. Next diagnostic: compare every DSP task from startup against the
independent interpreter inside a private runtime, stopping and retaining the
first mismatch. This avoids relying on eight-task spot checks or speculative
smoothing/gain changes. The diagnostic is private and will not be installed.


### Continuous DSP, sample provenance and CPU boundary results

The private continuous comparison ran the generated DSP and independent CXD4
interpreter for every task from startup through the 15,600-VI combat fixture.
All 7,797 tasks matched in the complete 4 MiB RDRAM result, with no HLE audio
callback and successful interpreter completion. This is substantially broader
than the earlier eight-task windows, but uses the same explicit microcode-entry
setup described above; it does not validate the omitted boot microcode or the
stock CPU's upstream voice/event state.

A second continuous capture retained 254,892 ADPCM input/book combinations.
Every predictor-book load matched the ROM. Of the compressed loads, 254,209
matched their whole requested span. The remaining 683 loads (594 unique spans)
differed only outside the compressed bytes consumed by ADPCM: after accounting
for the command's input offset and rounded output sample count, all consumed
payloads matched the ROM. These padding differences are not evidence for a PI
or sample-cache corruption fix. Private evidence: compressed-source-audit.json
and compressed-source-consumed-audit.json.

The wider CPU envelope capture timed out after 35 complete paired calls rather
than the intended 96. All 35 complete pairs were preserved and compared: 30
matched all RDRAM and their 32-40 generated command bytes; five stopped at MI
hardware access. These are limited arithmetic/command-builder passes, not
full-system scheduling passes. The original failed capture remains in the
journal and incomplete work is not counted as coverage.

Capturing the stock MI interrupt mask and modeling its command semantics let
the full-audio-frame harness proceed further. It then reached an EPC register
write in original OS thread dispatch (0x800758E0, COP0 register 14), which
requires actual thread/device execution. None of the twelve full-frame cases is
a parity pass. A diagnostic build first failed because MSBuild's scratch paths
were too long; a short build directory repaired that setup issue without
changing runtime behavior. Do not suppress the context switch or fabricate
hardware responses to claim a full-frame comparison.

Controlled builds using identical CPU code but old versus corrected DMA first
differed in audio at 3.777682 seconds. Repeated corrected runs were exact until
187.534926 seconds, shortly after the fixture switches to host-clock-driven
native timing. Later voice command lists also differed between repeats.
Therefore different later waveform positions are not a deterministic A/B proof
of either improvement or regression. Retained single-task fixtures remain
deterministic. Evidence: controlled-dma-audio-comparison.json.

The remaining investigation must distinguish correct DSP execution from correct
task inputs and correct delivery. Preserve the owner's explicit negative
listening result; neither the exact comparisons nor the transfer fix resolves
the reported static on its own.

Private producer/delivery comparison retained in producer-to-ai-delivery-audit.json. It compared 31187 produced chunks against 22950688 delivered PCM bytes; the best tested starting alignment differed in 0 of 22950688 bytes. The ownership trace contained 7796 AI submissions, with 0 changed-payload records retained. Interpret final unsubmitted task output separately from dropped playback.

Broader stock-emulator evidence: 24/24 retained PCM task outputs matched the corrected native producer across three separate music intervals. The emulator reached its capture-success marker, but the shutdown wrapper reported failure after stopping its owned process. All 27 input snapshots were complete; a separate command verified their sizes and compared the 24 pairs. This salvages complete evidence without relabeling the failed process result.


### Timing-path listening comparisons and remaining boundary

The owner reported Goldwood-reference-before-Windows.wav **clean**. This is our
port using the older instruction-timed path, not an independent stock emulator.
The owner then reported Goldwood-native-timing-without-CPU-spans.wav **still has
static**. That second comparison used the same DMA-corrected executable and
native elapsed-time policy as the corrected capture, with CPU span optimization
disabled. Disabling that optimization is not a remedy for the reported sound.

These recordings are not identical gameplay/voice states, so they narrow the
investigation without proving a particular scheduling defect. Combined with
exact DSP execution, correct consumed sample data and byte-exact delivery, the
next boundary is upstream audio task/voice generation under native elapsed-time
scheduling. Returning to instruction timing would restore the performance
limitation tracked by PERF-001; it is not an acceptable final fix.

The same 35 complete stock envelope CPU pairs were also replayed through the
actual native span path. Thirty matched all RDRAM and generated commands; five
again required unsupported device execution. Passing calls exercised 56-82 span
commits and 335-485 instructions in spans. An initial comparison script failed
because of escaped Windows path characters after the probe built successfully;
a corrected script used the retained executable. This limited result is not a
whole-audio-frame or interrupt-scheduling qualification.

Retained native, reference and stock sequence snapshots show zero recorded
dropped events. Queue structure offsets were checked against generated original
instructions rather than assumed from a newer header. The initial snapshot
reader used the wrong byte order, then incorrect state/volume offsets; the
verified-layout result supersedes those interpretations. Occupancy counters can
disagree with the linked queue in stock snapshots too. These observations do
not justify changing event bookkeeping or establish event-order correctness.
Evidence: retained-sequencer-state-verified-layout.json and
stock-broad-native-spans-cpu-comparison.json.

A final private comparison records native Count observations with spans disabled
and replays those same observations with spans enabled. This separates execution
optimization from differences in host timing. It is a reproducibility check,
not a proposed sound-quality fix.

The identical-clock comparison did not produce a completed comparison artifact. Retain the command results and diagnostics; do not count it as a parity or reproducibility pass.


Remaining diagnostic work should preserve an identical input/clock fixture and
observe original audio-frame entry/return, output sample counts, sequencer
callbacks, parameter timestamps and RSP launch/completion ownership. Compare the
first differing command/voice state with an independently executed guest CPU/OS,
including actual interrupts and thread switches. The standalone function harness
cannot substitute for that boundary. Task descriptor/command-buffer lifetime and
the omitted microcode boot entry remain unqualified hypotheses; neither has been
shown to cause this report.

The normal build was restored after private instrumentation. No continuous DSP
shadow, source audit or delivery audit was installed. The desktop preview
runtime, launcher, save and controller settings remain unchanged. The verified
DMA correction is in source and a normal local build; the owner-reported static
remains unresolved and no complete audio fix is released.


### Bounded clock replay and continuation handoff

The 15,600-VI clock-recording experiment reached the existing 16-million-observation diagnostic cap (512,000,016-byte tape) before completion. The runtime stopped through its fail-closed clock path; that is not evidence of a real non-monotonic host clock. Its dependent replay was skipped. The failed recording remains retained, and the cap was not changed. A separate 14,000-VI fixture was selected to cover the reported burst while staying within the existing capture limit.

The bounded comparison produced 20,599,168 bytes when recording and 20,599,168 when replaying. Exact PCM agreement: **True**. Exact final RDRAM agreement: **True**. This supplies a repeatable complete fixture through the suspect interval with both CPU execution paths. It does not establish that this particular recording has the same audible burst; the owner identified static in the separate no-spans listening capture.

AUDIO-001 remains open. The normal adapter/build and unchanged installed preview were checked at handoff. The production guard stayed enabled under the same investigation/root throughout. This continuation stops within its two-hour authorization; consult the operational ledger before any later work, without resetting counters or bypassing a denial. Remaining work is the full CPU/OS audio scheduling and voice-state comparison described above, followed by a fresh audible verification before any complete-fix claim.


### Owner-authorized three-hour scheduling continuation

The owner authorized three more hours after the cumulative-limit stop. The
production guard remains enabled on the same investigation and root; previous
attempt, elapsed-time, activity and no-progress charges are retained. This
continuation starts with a matched idle native/reference comparison using the
same DMA-corrected executable and neutral input, then traces upstream voice and
audio-frame state. No new complete-fix claim follows from the prior DSP passes.

The matched idle comparison used one DMA-corrected executable and identical
neutralized input files. Both produced 22,954,368 PCM bytes; the first differing
stereo sample begins at 187.201063 seconds, just after the native-clock boundary.
The owner listened to the corresponding music excerpts and reported **only the
native timing capture has static; the reference capture is clean**. This is a
stronger timing-path isolation than the earlier different-gameplay captures,
but a waveform difference alone does not identify the defective operation.
Evidence: matched-corrected-idle-comparison.json and
owner-matched-idle-listening.json in the private audio scheduling evidence.
A private trace now retains original voice/sequencer state and command buffers
before each DSP task through the boundary. It is diagnostic-only and is not
installed in the preview.

### Tracing changes the native-timing reproduction

The paired detailed traces align 1,997 DSP tasks by cumulative sample position.
The first RAM difference is task 5598; the first command difference is task
5600, adding two sound-effect envelope updates. All logical music-client,
note, pitch, volume and envelope state matches across the interval after
ignoring physical voice-slot assignments. Slot assignment alone is not a music
sequencing fault. An independent instruction-level trace of 142 task fixtures
preserved every original RDRAM output byte; 2,118 paired music chunks have exact
resampled input and at most one signed-16 unit of dry-mix contribution difference.

**Important limitation:** full-RAM tracing and clock recording perturb native
elapsed time. The traced native PCM lacks the strongest high-frequency bursts
present in the owner-confirmed untraced native recording at corresponding
sample positions. The traced reference PCM exactly matches the corresponding
prefix of the owner-confirmed clean reference. Therefore the detailed native
trace has not established the state of the actual failure and cannot rule out
bad voice generation in the uninstrumented run. Treat these as a diagnostic
observer effect, not a fix. The next captures reduce snapshot frequency and
omit the Count transcript; retained snapshots from the original reported burst
are also being inspected independently.

Private evidence: idle-task-state-comparison.json,
logical-music-state-comparison.json, voice-dsp-comparison.json,
traced-audio-spectral-check.json. Spectral ratios locate differences, but are
not an automatic sound-quality verdict.

### Lighter tracing and source-delivery checks

The actual owner-confirmed native idle capture, clean reference capture and
native capture without CPU spans each retain 7,797 AI submission/consumption
hash pairs. All pairs match. No buffer reaches the original driver's special
0x2000-within-0x4000 DMA boundary. Neither deferred-buffer overwrite nor that
address-carry quirk explains these recordings. This check uses the already
reported captures, avoiding a fresh tracing observer effect.

Capturing every twentieth DSP task without Count recording preserves strong
high-frequency differences. Independent instruction-level execution of twelve
native and twelve reference snapshots produces PCM that exactly occurs at the
expected location in each corresponding live recording. The inserted per-stage
SAVE probes also preserve every original RDRAM output byte. For these snapshots,
the bursts enter through a dry sound-effect voice before the effects/reverb
mix, often using sound-bank entry 4 (wave table 0x80186b98). This identifies a
diagnostic route; it does not yet establish whether that sound is corrupted,
triggered incorrectly, or simply different because gameplay timing diverges.
Spectral energy alone is not a perceptual verdict.

The CPU interrupt audit reported no differences, but its final totals were not
flushed before the probe's deliberate _Exit. Treat it as an incomplete coverage
check, not a successful full interrupt-state qualification.

Private evidence: confirmed-recordings-ai-audit.json,
sparse-audio-spectral-check.json, sparse-voice-dsp-comparison.json,
sparse-independent-pcm-checks.json and sparse-fx-stages.json. A low-overhead
per-voice diagnostic is being used to separate the suspicious effect from the
music for listening. Muting or filtering it is not an accepted remedy.

### Effect trigger identified; listening isolation pending

The native call trace identifies sound-bank entry 4 as an enemy weapon sound:
squadsFireWeapon calls the positional sound API from overlay return address
0x003085F8 with logical sound ID 12, which maps to bank entry 4. Neutral player
input does not remove enemy activity. Therefore this automated idle replay is
not a music-only scene and must not replace the owner's separate report of
standing still outside the red-key door without enemies.

A private per-voice probe records the unmodified mix and effect contributions.
The owner was supplied isolated-effect and effect-subtracted WAVs; their
listening result is pending. Subtraction is diagnostic, not a remedy. All five
listening variants use the same 0.84592145 gain to keep subtraction results in
signed-16 range; the raw recording remains preserved. Music-pool classification
can be affected by within-frame physical voice reuse, and nonlinear saturation
cannot be undone by subtraction. Do not claim perfect source separation.

Private probe setup failures are retained: an initial WAV export rejected
signed-16 range overflow; two insertion attempts rejected ambiguous/newline
anchors before editing; a compiler warning-as-error rejected an unchecked
diagnostic memory read. The corrected tracer checks the read result, builds,
and restores the public source after pinning its private executable.

### Diagnostic separation correction (not a game playback change)

The first live per-voice probe used an incorrect DMEM base, 0x450 instead of
this microcode's 0x500. Its isolated/subtracted WAVs and residual comparison
are invalid diagnostics and have been withdrawn; they must not guide a fix.
The normal game output was never changed by that probe. Earlier independent
prefix probes used the microcode's own SAVE command and therefore used the
correct buffer base; their retained task/PCM parity results are unaffected.

The corrected private probe identifies sample books at each ADPCM command,
rather than relying on end-of-frame physical voice ownership. It records the
final interleaving inputs and requires every available traced block to match
the captured PCM exactly before exporting new listening variants. It also
requires the reference capture to match the owner-confirmed clean reference.
New artifacts use the distinct Goldwood-verified-* names. They remain diagnostic
subtractions and do not reverse nonlinear mixing saturation.

Decoding sound-bank entry 4 from the beginning of its original ROM sample also
reproduces all three retained live post-task ADPCM histories exactly (tasks
5860, 6440 and 6840). Unused scratch DMEM filled with 0xA5 instead of zero leaves
all RDRAM output unchanged in the 24 retained sparse tasks. This qualifies that
initial scratch-memory question for these tasks, not full boot or every
possible instruction state.


### Compact tracing and renewed waveform investigation (2026-10-12)

Status remains **open**. The owner confirmed that the same-executable matched
idle native recording has static and the instruction-clock reference is clean.
The earlier DMA correctness repair alone is insufficient. No source gain,
filtering, muting, or limiter workaround is accepted as a resolution.

The corrected live probe's final interleaving inputs match all 5,988 recorded
blocks in each checked capture exactly. Compact snapshots reduce recording
overhead while preserving music and physical-voice metadata. Across 1,497
aligned tasks, normalized logical music-note state matches. All 40 physical
voices remain accounted for, at most 20 are allocated in the native case
(19 in the reference), and no voice-steal offsets or music notes without
physical voices occur. Voice exhaustion does not explain this interval.

Matching source samples by logical voice and block finds 45,462 exact music
chunks and 87 differing chunks, with 55 unpaired groups. End-of-task ownership
can change within a task, so those totals alone do not prove music corruption.
The first candidate, task 6170, does have the same music note, sample book,
pitch, envelope and decoder position. Its first block agrees and subsequent
blocks diverge as the decoder switches source buffers. The next check follows
the source-buffer bytes and independent DSP execution of a targeted snapshot.

The corrected CPU interrupt audit checked 86,784 interrupt contexts with no
differences at its last flushed checkpoint (VI 13983); mismatch records flush
immediately. This covers observed timer/external interrupts and the relevant
GPR, HI/LO, FPR, FCR31 and rounding state, not lazy-FPU exceptions.

Arbitrary poisoned DSP condition flags can change output, but the flags actually
observed at task completion reproduce all retained RDRAM output in 48 of 48
checks. Arbitrary scalar/vector/accumulator poison did not change these tasks.
A full-boot-entry experiment stopped before audio processing and is unqualified;
it is not evidence for changing boot or register initialization.

A second paired capture starts from a game-produced Goldwood entry checkpoint
with neutral player input. It is not the owner's red-key-door location.
Both recordings reach Goldwood, and their 5,988 traced interleaving blocks
match PCM. Their PCM differs, but listening and source attribution are still
needed: neutral player input does not guarantee absence of ambient/enemy audio.

The prior three-hour guarded job expired after these captures. Its failure and
all cumulative charges were preserved. The owner approved a further four hours;
the same production investigation and root remain in use with the guard
enabled. The installed preview and personal saves were not changed. A private
snapshot-builder syntax error was retained and corrected before any capture.

Private evidence: compact-verified-voice-separation.json,
compact-logical-music-comparison.json, compact-voice-allocation-summary.json,
compact-current-music-chunk-comparison.json, counted-irq-context-result.json,
observed-condition-flags-check.json, fullboot-entry-state-check.json,
quiet-goldwood-comparison.json, quiet-and-6170-inspection.json,
waveform-command-window.json. ROMs, generated code and save data stay private.

The owner listened to both Goldwood-quiet-checkpoint WAVs and reported that
both sound clean. Preserve this as a negative control: the new timing path
alone is not sufficient to reproduce the audible failure in every game state.
The red-key-door report and earlier owner-confirmed bad native capture remain
unresolved. Private receipt: owner-quiet-checkpoint-listening.json.


### Intended sample addresses and storage recovery (2026-10-12)

The earlier compressed-payload search established that consumed bytes occur in the ROM; it did not establish that they came from the intended ROM address. Stale but otherwise valid sample data could pass that search. This is a qualification of the previous evidence, not proof of stale data. A new audit uses each captured DMA-cache entry's ROM start and RAM buffer address to check consumed bytes at their intended offsets.

The retained-fixture audit covered 58 snapshots and 921 mapped ADPCM loads: 921 exact and 0 different. Unmapped loads: 0. This establishes only the captured intervals; it cannot exclude a timing-dependent failure between snapshots. Metadata is sampled at DSP consumption, not recorded at each original request.

The compact capture has 76 sample-cache buffers and never exhausts them in the examined interval (at least 36 free natively and 39 in the reference). The new task-6170 capture uses a different physical voice allocation; all four examined source ranges match their expected ROM addresses. It must not be treated as a capture of the earlier differing task merely because the task number agrees.

The [Nintendo synthesis-driver contract](https://ultra64.ca/files/documentation/online-manuals/man-v5-1/pro-man/pro17/17-04.htm) requires the application to make sample data available before audio command processing. Our next hypothesis is a producer/consumer ordering problem under faster CPU execution. It requires an observed wrong source range or incomplete transfer at consumption before any synchronization change is justified. No arbitrary delay, gain reduction or filtering is accepted as a remedy.

Upstream comparison: [Perfect Dark's audio DMA cache](https://github.com/perfect-dark-pc-port/perfect_dark/blob/port/src/lib/audiodma.c) uses the same general cache-list/lifetime pattern and drains completion messages without blocking. Its comments explicitly describe the risk of data arriving too late; this is not a synchronization guarantee to copy. [Our pinned N64ModernRuntime PI implementation](https://github.com/N64Recomp/N64ModernRuntime/blob/589bbf018a3e6d3646ddf7de1e7919f1b7e99bb1/librecomp/src/pi.cpp) instead performs the ROM read before queuing the external completion message. These are source observations, not validation of replacing JFG's original PI/OS path. Any native solution must establish data readiness and ownership without changing unrelated transfers or depending on a fixed frame rate.

A disk-full failure interrupted this continuation and left an incomplete pinned executable; that file was retained and never executed. A complete separately named executable was verified before resuming. [Storage recovery](diagnostic-storage-exhaustion.md) preserves old diagnostics on another volume with hash checks and original-path junctions. The operational ledger and active investigation remain in place; all failed-attempt charges and the original approval deadline are preserved.

Private evidence: expected-rom-cache-source-audit.json, audio-dma-cache-pool.json, target-6170-source-comparison.json, target-vs-compact-6170-source.json, and target-6170-independent-dsp.json when qualified. Reported static remains open; installed preview and personal saves are unchanged.

### Confirmed stale compressed sample data (2026-10-12)

A lower-overhead live check compares consumed compressed sample ranges with the exact ROM addresses recorded by their cache nodes. In a matched pair using the same executable and input file, the native run records 16 mismatched ranges across eight retained tasks; the reference records none. Both reach 14,000 VI retraces. The last flushed aggregate checkpoints differ because these captures end before task 6999; aggregate checked/exact counters apply only through their recorded checkpoints. Mismatch events flush immediately.

At the first failing task (6961), a music voice requests ROM 0x0184E176 in cache buffer 0x000D8C60. All 640 cached bytes still match a different ROM sound, and 637 bytes differ from the intended payload. The guest counts 15 audio DMA requests while its completion queue contains 14 messages. This establishes a data-readiness/lifetime failure at consumption; the exact scheduling path that leaves this request outstanding still needs qualification.

Independent instruction-level DSP replay reproduces the live PCM exactly for all eight retained failing tasks. Substituting the intended ROM payload only in an isolated copy changes the output; the first task changes 366 channel samples, with a maximum difference of 6368. That substitution is a causal diagnostic, not an installed repair. The source buffer belongs to a music wavetable. It is no longer sufficient to describe this instance as ordinary enemy sound effects or Windows playback clipping.

The earlier compact task-6170 capture has differing music samples after its first block, but the later targeted task-6170 capture does not reproduce that divergence. Both later targeted tasks match independent DSP replay. Task numbers alone do not establish identical native execution state. Diagnostic reader failures (wrong assumption of one large output save instead of four blocks, virtual-voice base mismatch, and a trailing blank line in JSON output) were retained and corrected before accepting their results.

Next remedy work must establish readiness and buffer/request ownership before the audio task consumes a sample. A native synchronous source callback is one possible implementation of the SDK data-readiness contract; changing global PI delays, copying wrong data after decoding, adding gain/filtering, or suppressing sound effects is not a qualified remedy. No fix has been installed yet, and owner listening verification remains required.

Private evidence: live-expected-source-cache-audit.json, source-mismatch-independent-replay.json, cache-audit-6961.state and seven later snapshots, target-6170-independent-dsp.json, target-vs-compact-6170-source.json. The regular build adapter was restored after the diagnostic pair.

### Native sample provider candidate and regression results (2026-10-13)

The candidate replaces the game synthesis sample callback in native timing mode with a synchronous read from the ROM already loaded by the application. It retains the original 76 cache buffers, range lookup, alignment and frame-based reclamation. A miss copies its full payload before publishing the cache entry. It creates no deferred cartridge request or completion descriptor, so there is no descriptor to recycle while the transfer is outstanding. Cache hits refresh the original lifetime metadata. Exhaustion or malformed state rejects rather than returning an unrelated sound. The DSP, gain, mix and global PI timing are unchanged.

Portable regression tests cover immediate payload availability, odd addresses, cache hits, ordered insertion, original-owner reclamation, exhaustion, invalid ranges, malformed lists, overlapping destinations, outstanding old requests, and both guest-memory layouts. Existing SP DMA broker tests also pass.

The same-executable replay reaches 17,000 VI retraces in candidate and native control modes. The candidate records 0 stale source ranges; the control records 269. The candidate has checked 110,268 ranges through its last flushed checkpoint, task 8250. Its native callback trace confirms the provider ran. The separate 14,000-VI instruction-clock reference has exactly the same PCM and final RAM as the earlier reference recording. All input files match; original saved-game data is unchanged.

Source: [native source callback](../../../include/jfg/boot/native_audio_source.hpp), [runtime selection](../../../src/boot/native_boot.cpp), and [regression cases](../../../tests/native_audio_source_tests.cpp). The candidate currently requires the private diagnostic enable flag; the installed preview is unchanged at this stage. Owner listening and live verification remain pending, especially the stationary red-key-door case. The replay demonstrates source-data readiness, not acceptance of every audible issue.

This data-availability contract does not depend on a guessed number of CPU cycles, a graphics frame rate, or a sleep duration. Future 60+ FPS work must preserve source-buffer ownership until consumption and keep the audio sample clock independent of rendering. Do not regress to enqueue-and-assume-ready behavior when parallelizing loading or audio.

Private evidence: native-audio-source-candidate-validation.json, idle-source-provider candidate/control/reference captures, and Goldwood-native-source-callback candidate/control WAVs. No public release or commit is claimed.

### Local preview delivery and normal Play validation (2026-10-13)

The source-readiness candidate is now enabled automatically when the experimental
native-timing preview starts normal Play. It is selected at startup, before the
first sample request; it is never switched over outstanding requests. The
instruction-clock reference keeps the original callback. The private enable flag
remains available for bounded comparisons, but users do not need to set it.

The final review runtime uses the normal audio adapter, with the temporary sample
address auditor and DSP tracing removed. A 17,000-VI native replay completes with
no source-contract rejection. A 14,000-VI reference replay still produces exactly
the earlier reference PCM and final RAM. Input files match, and source saves are
unchanged. The instrumented A/B results above establish 269 stale source ranges
in the control versus none in the candidate; they are separate evidence from the
normal-adapter qualification and are not claimed as always-on production checks.

A separate hidden normal-Play check launches the staged executable without any
private clock, CPU-span, or audio-provider enable flags. It confirms the automatic
native timing and synchronous source-provider support events, observes actual
source callback use, presents more than 600 retraces, and closes its owned hidden
window cleanly. It consumes only replay input and does not change global input or
the foreground desktop. This is startup/integration coverage, not manual gameplay
or listening acceptance.

The existing Modern Controls Preview shortcut now selects the complete local
`modern-controls-audio-source-20261013` package. The v1.1 launcher executable is
unchanged. All 14 runtime package members have verified hashes and sizes; only
the runtime was replaced. The earlier package and activation backups remain for
rollback. Managed ROM selection, saved games, button mappings and other profile
settings are unchanged. Updating the profile runtime path alone would be
insufficient: v1.1 resolves and verifies the runtime bundled beside its launcher.

Runtime SHA-256: `20a51b5c1e656e5fd8179feccf4c7773c43a62d7243133730af885e1687ecf3f`. This is an uncommitted local preview, not a public release. Owner listening and live verification remain pending, including the stationary Goldwood red-key-door case and drone combat. The exact original scheduling sequence leaving the stale request outstanding has not been fully traced; the new provider removes that asynchronous request lifetime from native audio loading.

Private evidence: native-audio-source-review-validation.json, native-audio-source-staged-package.json, native-audio-source-preview-activation.json, native-source-preview-hidden-play/result.json, and Goldwood-native-source-review.wav.

### Owner live verification (2026-10-13)

The owner tested the delivered native-source preview and reported: "Audio is fixed." This records live acceptance of the reported static repair in addition to the source-integrity, reference-parity and normal-Play checks above. It does not claim every game area or a public release. Three subsequent controls reports are tracked separately in [modern controller state transitions](modern-controller-state-transitions.md).
