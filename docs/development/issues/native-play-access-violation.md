# NATIVE-001: Native Play access violation during extended replay

Status: captured stale path-cache write corrected and verified in targeted and
extended native tests; local preview installed, broader live verification pending.
Severity: critical memory corruption; bounded test verification is not a claim
that every historical exception has the same cause.

A muted hidden 120-FPS/VSync native Play replay from the preserved tutorial
profile stopped after approximately 200 seconds, before its 17,000-retrace
target. Runtime ad17e4c3 reports Windows access violation 0xc0000005 at module
RVA 0x15a9961f; the last dispatched guest target is 0x80075030 (__osException).
That last-dispatch target does not by itself identify the faulting function.
The support crash record includes a complete host stack. The input save hash is
unchanged; normal diagnostic footers and final RAM captures are absent because
the native exception exits the process.

Earlier 17,000-retrace Original-mode reference and shorter native 120/360 checks
passed. They do not cover this longer native path. The equality of those reference
states is retained as evidence, not a reason to dismiss this crash. Whether the
failure predates the CPU/matrix/equal-slot changes is still unknown.

Evidence: combat-focus-20261014/shared-extended-120 (crash/support/breadcrumb/
partial gameplay logs), shared-extended-host-load.json, extended-crash-review.json
and extended-crash-symbols.txt. Matching runtime/PDB are preserved privately.
Keep the desktop preview unchanged while locating the fault and testing the
predecessor. Do not publish generated source, ROM, or personal save data.

Transferable lesson: successful bounded state and presentation checks do not
establish long-run native stability; preserve the exact binary/PDB and separate
the exception site from the last recorded guest dispatch.

## Extended predecessor check and opt-in crash evidence

The pre-equal-slot runtime 7dd907ff also fails during the extended native route,
at approximately 208 seconds / VI 12049, with the minimal runtime's switch-error
trap (process 0xc0000409). This differs from the candidate's access violation;
it establishes an older long-route stability problem, not a shared cause.
Neither run qualifies the full native route. No foreign game/compiler was
observed by the two-second process monitors.

An explicit private RDRAM-capture session now also records the native fault's
access kind/address, host registers, last guest instruction, exception PC and
running-thread word, then writes logical RAM before the existing fail-closed
exit. Ordinary Play does not log host addresses or capture RAM. The capture is
a diagnostic, not a remedy; a render-thread fault could observe in-progress
guest state. Matching diagnostic binary/PDB and all raw runs remain private.
Evidence: predecessor-extended-120, predecessor-extended-host-load.json,
native-crash-diagnostic-baseline and extended-native-comparison.json.

The explicit-capture repeat reproduces a second access violation, at guest PC
0x80020098 reading through guest pointer 0x9f265a10 (outside RDRAM). The current
running-thread word is valid at 0x800f92a0. This does not establish the first
crash's cause. One identical-configuration run first completed 17,006 retraces
with no audio underruns; the repeat failed at 10,743. Treat the instability as
intermittent and retain both outcomes. Evidence: diagnostic-extended-120-0/1
and diagnostic-crash-symbols.txt, with the captured logical RAM kept private.

Private fault details are moved to stderr so the parent's canonical stdout trap
protocol remains intact; the first diagnostic's extra stdout text caused parent
rejection (exit 3) while correctly preserving the child's access violation/exit
4 in stderr and support logs. No crash text was lost. The diagnostic now also
records the last fully observed CPU context to locate the source of invalid
registers. Its pointer is observed only in explicit RAM-capture sessions.

The first switch-trap diagnostic build failed the generated bridge object audit:
using printf introduced CRT inline-wrapper COMDAT exports. Replacing that call
with bounded eight-digit formatting and existing stream primitives preserves the
audited bridge inventory. No audit bypass or inventory relaxation is used, and
no game was launched from the failed build. Failed logs are retained.

The captured level-model word at 0x800f5b0c is 0x1f265a10, versus 0x80265a10
in the clean candidate and delivered-preview runs. The failing host address is
the generated mapping of that invalid guest pointer; 0x9f265a10 was the host
mapping offset, not the stored guest value. The leading guest byte changed.
The pinned decomp identifies the failing function as shadow geometry work
(src/shadows.c, func_8001FD20), correcting the initial animation attribution.

A temporary opt-in model-word observer samples at existing full CPU boundaries,
retains 64 recent PCs and fails closed with RAM capture at the first valid-to-
invalid transition. It is diagnostic-only, requires explicit private RAM capture,
and never repairs the pointer or changes ordinary guest execution. Its timing
runs are causal diagnostics, not FPS qualification.

The observer's initial build stopped on shadowed-local and deprecated getenv
warnings; using the existing Windows environment-reading convention with unique
locals resolves them. No game ran from that failed build.

The first model observer run actually completes 17,012 retraces without a
pointer change or audio underrun, but its initial informational line breaks
the parent's strict canonical-child-output parser. Both child stdout and stderr
share that private protocol handle, so moving text to stderr alone was not
sufficient. This is a diagnostic protocol failure, not another game crash.
Fault details now go to an explicit .fault.txt companion beside the requested
private RAM file; ordinary successful observation emits no protocol text.
The prior run and rejection are preserved and are not counted as clean wrapper
qualification. The game itself reported a normal window-close exit.

A private hardware-watch fixture passes on twelve owned threads: three watched
writes (including a single-byte corrupting write) are observed, reads/unrelated
writes do not trigger, and restoring the debug context disarms the watch.
An optional "hardware" mode now arms one unused write-watch slot in each guest
execution thread at the existing observer boundary. A corrupting write records
the exact host instruction/stack, CPU context and RAM, then fails closed.
The software observer remains to detect writes by other threads. No external
process is attached, no occupied debugger slot is overwritten, and ordinary Play
does not install an exception observer. This diagnostic is not a gameplay fix.
Evidence: hardware-word-watch-fixture/result.json and its compiled fixture.

The first in-game hardware diagnostic stops at its own debug-register admission
check near VI 2000, before observing a corrupting write. It is not another game
crash. The asynchronous handler's armed flag is made volatile and a bounded
per-thread setup receipt records the supplied debug-register context to explain
any repeated refusal. Occupied debug registers still fail closed.

The setup receipt identifies the hardware-diagnostic rejection: RaiseException
provides ContextFlags 0x10004f, which excludes valid debug-register contents.
A second thread's apparent Dr7 is uninitialized text-shaped data, not evidence
that a real debugger occupied the slot. Volatile alone does not address that.

A second isolated fixture passes twelve arm/write/disarm sequences using the
documented suspended-thread API. An owned helper now briefly suspends only its
requesting guest thread, gets valid debug registers, configures an unused slot,
and resumes before any allocation/logging. The requester joins the helper and
continues; errors fail closed and existing debug slots remain protected.
The exception observer only handles actual owned data-watch traps. This replaces
the unqualified RaiseException setup, not the game's scheduling or memory rules.
Reference: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadcontext .

## Corrected diagnostic qualification

Private run suspended-model-120-0 ends with code 0; normal performance footer: True. No invalid watched pointer is observed in this run. Setup receipts verify the owned thread watch configuration.

Private run suspended-model-120-1 ends with code 3; normal performance footer: False. No invalid watched pointer is observed in this run. Setup receipts verify the owned thread watch configuration.

The second run stops at the original OS exception-handler jump table (guest PC 0x80075290, table 0x800aef50). It does not trigger the model-pointer watch first. Its truncated CSV also exposes a fixture parser TypeError before the normal result receipt; recovered-result.json retains the raw failure without claiming clean performance qualification. A switch-trap snapshot is still needed to distinguish malformed target/table from scheduler context corruption.

The installed 1b0bdd0f preview also completes one 17,000-retrace native comparison. This is one clean baseline run, not proof that intermittent failures are absent. No desktop shortcut/profile or released package is changed. Evidence: suspended-model-review.json and delivered-extended-120.

The watched repeat identifies a second concrete failure at __osException's
interrupt dispatch (PC 0x80075290, __osIntTable 0x800aef50), before any observed
model-pointer change. An explicit private diagnostic now handles SIGABRT to
save the existing fault context/RAM before terminating; ordinary Play keeps
its original abort disposition. Diagnostic exception code 0xe04a4647 identifies
this deliberate snapshot. No invalid target is repaired or allowed through.
The Windows CRT documents this abort-handler logging/termination path:
https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/abort .

The first abort-capture build rejects a redundant unreachable termination after the existing non-returning fault handler. Removing that unreachable statement preserves warning-as-error checks; no replay used the failed build.

The repaired diagnostic builds successfully, but its private receipt script omitted a hashlib import at final reporting. The dependent replay correctly did not run. A separate validation request rechecks the build and produces its hash; prior failed receipts remain intact.

Two abort-capable watched Play runs and the subsequent recorded-combat-prefix
diagnostic are retained separately from performance qualification. The first
two finish without memory faults but have 100/89 audio underruns amid high host
load. A foreground status sample identifies unrelated .NET CPU/disk activity;
that sample does not explain every hitch. The next controlled-prefix capture
records per-process CPU, memory and I/O totals, not merely selected executable
names. The private directory review initially included sibling host-load JSON
files in its glob; the corrected review filters directories, preserving the
failed request and preventing its dependent replay from launching.

The hardware observer is expanded only after a twelve-thread four-slot fixture
passes. It watches the level-model pointer plus the immutable redispatch, RCP
and timer interrupt-table targets. A changed table word captures the actual
writer even if its damaged value remains inside RDRAM. Occupied debug slots
still refuse setup, and ordinary Play remains unaffected. This is diagnostic
coverage, not a proven corruption remedy.

## Bounded clock recording for reproducibility

Repeated live runs can take different paths before an intermittent failure.
An explicit finite-diagnostic Count transcript allowance may now be chosen
before recording/replay starts, up to 64,000,000 samples (2,048,000,016 bytes).
The default remains 16,000,000. Active/exhausted/finished transcripts cannot
raise their allowance or reset sample counters. Exhaustion is reported as a
sample limit, and a failed exhausted tape cannot finish successfully. Private
failure exits flush the Count stream before saving RAM so buffered final
interrupt observations are retained. Unit checks exercise record/replay bounds,
invalid limits and unchanged charges. This is separate from the production
investigation ledger: its guard, ancestry and all prior charges remain intact.
Large captures stay on the owner's evidence drive; this does not select a new
clock policy or change ordinary Play.

The first configurable-transcript build catches a missed member insertion in the private edit script. The missing limit field is added, and build/tests must pass before recording; the dependent capture correctly did not launch.

The first large Count capture reaches exactly 64,000,000 samples and stops
at the explicit sample limit (guest PC 0x80098a20), near VI 16309. No memory
fault is observed. Its replay reaches the same deliberate stop. The private
postprocessor then rejects a partially written terminal gameplay-trace row;
this is a reader failure, not a game crash. Recovered metadata uses the saved
native exit code and raw RAM; it does not invent native elapsed time or a
successful original wrapper receipt. The private reader now ignores only an
unterminated final trace row, retaining all complete records and raw evidence.
The Count file is 2,048,000,016 bytes, SHA-256
2e54d0504cc82f2cea8dd30a3a6a32a7148b21dd083a9e2cd59d78897c1eb195.
Neither the transcript bound nor this replay resolves the intermittent fault.

The 64-million-observation replay matches every guest RAM byte and save at the
same explicit sample-limit PC. All 7,836 common complete update records agree;
one extra complete record remains only in the recording because the replay's
terminal trace write was interrupted. This qualifies reproducibility of this
long non-failing route, not resolution of the intermittent corruption.

GuestBacking maps cached and uncached RDRAM through different host addresses.
A hardware watch on one linear address does not cover the other alias. A
separate private page-write mode now observes writes through both views of the
two relevant pages, single-steps the original instruction, restores protection,
and compares the model word plus all nine interrupt-table targets. It captures
the pre-write host PC if a watched word becomes invalid/changes. A twelve-thread,
two-alias fixture verifies byte corruption, same-value and unrelated writes,
reads and disarming. The mode installs only with explicit private capture;
normal Play does not protect these pages. It is a diagnostic, not a memory
repair. Writes by an unexpected concurrent host thread can still observe a
non-atomic guest snapshot; writer-PC attribution remains separate from that
snapshot limit. Native recorded-route qualification is pending.

The alias-aware observer passes the complete available 64-million-sample route:
all 4 MiB of RAM, the save and all 7,837 complete update records match the
recording at the same explicit sample-limit stop. The watched replay completes
in about 177 seconds, versus 280 seconds for recording, without changing the
recorded game state. No memory fault is observed; this validates the diagnostic
on this route and does not qualify the reported corruption as fixed.
Evidence: alias-page-replay-review.json, long-clock-replay-review.json,
alias-page-write-fixture/result.json and recorded-native-clock-receipt.json.

The alias-aware 360-FPS native recording also reaches its explicit 64-million-
sample limit without a watched memory change, near VI 14231. It retains a
second complete Count tape on D: (SHA-256
3f703e333ac72ef00c978ac224f7da7b7a32f2bd5bca056fdbe6098bc9a756af).
The diagnostic records 26 audio underruns and combat source windows around
24-26 updates/s despite roughly 350 presentations/s. This is neither clean
performance qualification nor a corruption remedy; page trapping and recording
add diagnostic cost. Continue targeted matching analysis from recorded input
rather than treating repeated non-failing live runs as proof of resolution.

## External observation of the unchanged failing executable

A private Windows debugger helper can observe the exact previously failing
052e18d5 executable without recompiling it. Admission verifies the owned child
PID/image and reads its RAM pointer using the matching PDB/PE section map. It
watches both cached/uncached views of the model-pointer and interrupt-table
pages, including all eight interrupt-offset words and nine handler targets.
While single-stepping a protected write, other debuggee threads remain suspended;
a change saves the pre-write host context and private RAM before diagnostic
termination. It never repairs a value or attaches to an unrelated process.

The twelve-thread, two-alias controlled fixture passes all 72 writes and 36
invalid transitions, including byte writes and interrupt-target restoration.
This qualifies the helper in that fixture, not the game fault. Native observation
and memory dumps remain on D: under external-memory-observer and
external-watch-native-120-0. Debugger/protection overhead and concurrent host
load exclude these runs from performance acceptance. No corruption remedy is
claimed; the delivered preview is unchanged.

The unchanged 052e18d5 executable completes both bounded 17,000-VI external
observations without reproducing corruption. The write observer handles 58,519
protected writes with zero invalid transitions. The exception-only helper passes
its controlled terminal-exception/minidump-precursor fixture and captures the
4 MiB fixture RAM, then the game run exits normally. These are negative diagnostic
observations, not crash fixes or complete owner-route verification.

The exception-only game run records 661 audio underruns and reaches only
3698 input polls by VI 17005. Thus an equal VI target under a
slow observer does not imply an equal traversal of the input route. Windows
notifies an attached debugger before application vectored handlers, so even
without protected write pages it intercepts the runtime's normal MMIO exceptions.
The helper also requests thread context on those first-chance events. This is a
concrete intrusive-observer limitation; do not use these rates as game performance
or absence of faults as stability proof. Further observation should preserve
native execution timing more closely, with transient-corruption limitations made
explicit. The observer's fixed 'pages=4' banner enumerates candidate pages;
exception-only mode protects none, as its zero watched-write count confirms.
Reference: https://learn.microsoft.com/en-us/windows/win32/debug/vectored-exception-handling .


## Read-only polling and teardown qualification

The unchanged failing executable completed one 17,005-VI run under 5-ms external
read-only polling, reaching 8,159 controller polls with zero audio underruns.
No protected pages or debugger attachment were used. The helper then failed its
final memory read during orderly child teardown; the child itself exited normally.
This is not a new game crash or a complete observer receipt. Preserve both logs.
The revised helper allows up to two seconds for process exit after a failed read,
records that terminal read error, and still fails if the process remains alive.
Both a held-corruption snapshot and a controlled unmap-before-exit fixture pass.
Polling can miss transient corruption and cannot identify the writer. A snapshot
is an observation-time dump, not a faulting-instruction capture. NATIVE-001 remains
open; the helper change is diagnostic only. Evidence: external-poll-native-120-0
and external-poll-observer-v2, including its normal-exit fixture.


The revised read-only helper completes 45,625 samples over 250.687 seconds in
external-poll-native-120-1. The unchanged old runtime exits normally at
17005 VI / 8189 controller polls, with
0 audio underruns. No watched corruption is detected. The terminal
unmap/read race is recorded and process exit confirmed. This remains one further
non-reproduction, not proof of native stability or exact writer attribution.


## External debug-register experiment

A private helper sets unused write-watch registers on owned threads of the exact
previously failing executable, without debugger attachment or page protection.
It watches the model word and the first eight interrupt-table bytes through both
cached and uncached views. All other interrupt-table bytes remain uncovered.
It briefly suspends each newly observed thread to set registers; a new thread
may execute before the next scan. Occupied watch slots are rejected. The helper
restores its own registers on bounded completion. A data-watch exception goes to
the existing executable's fault capture, so the saved RIP is after the write.
Any model write can trigger, including legitimate initialization; a capture is
not automatically memory corruption. Controlled fixtures must pass before use
on the game. Evidence: external-register-observer, kept private on D:.

Controlled cached/uncached model-byte and table-word writes reach the existing
unhandled-exception path. Read/unrelated-write and disarm fixtures also pass,
with twelve worker threads armed in each case. Game cause remains unlocated.


The first unchanged-executable register watch captures an expected model-pointer
store in shadow initialization at VI 6353 / poll 2975 (guest 0x8001d804), writing
a valid 0x80265a10. This is a deliberate diagnostic stop, not another corruption.
It verifies that the old executable's fault path saves RAM and post-write host
registers without debugger attachment. The next bounded watch starts after this
observed initialization, at VI 7000; legitimate later initialization can still
trigger and must be distinguished from corruption.

The private fixture's `hidden` summary used the final owned-window list, which
is cleared when a child exits before the loop observes its process termination.
That incorrectly reports false for this deliberate stop. The fixture did assert
the window was hidden when it attached, and its saved environment requests the
hidden diagnostic window. A revised receipt retains the actual earlier check;
raw historical receipts are preserved. This is diagnostic metadata, not a game
visibility or mute-setting change. Evidence: external-register-native-120-0.


Reviewing the original bad RAM beyond the model word shows repeating
03 00 1f 1f 1f bytes at 12-byte strides across neighboring globals, with later
writes surviving between them. The model byte is part of broader structured
corruption, not evidence of a lone random bit flip. A source-pattern search
records candidate byte-store loops and the actual run extents. Pattern similarity
alone does not identify the executed writer; the filtered watch remains the
causal check. Evidence: corruption-byte-pattern.json and the preserved original
RAM capture. This observation does not yet justify changing game lighting or
shadow routines.


The after-initialization watch reaches VI 12833 / poll 6245, then stops on the
same legitimate shadowInit store (guest 0x8001d804, post-write host RVA 0x20c9d9f6)
with model value 0x80265a10 and canonical interrupt handlers. It does not capture
the corrupting write. Moving admission later cannot cover this repeated valid
initialization reliably. A private observer therefore adds a narrowly filtered
vectored handler to the unchanged failing executable: only the exact verified
post-store instruction, expected pointer value and owned model-watch slot may
continue. Other instructions, values, watched tables and exception types retain
the existing fault path. This changes the diagnostic process/module layout and
can still affect reproducibility; it is not a runtime repair or stability proof.
Controlled fixtures must qualify filtering before any game run. Evidence:
external-register-native-120-after-init and filtered-register-observer.


## Shadow polygon scratch-buffer lead

The preserved failed RAM has polygon count 472 at `0x800f45e8` and vertex
count 32. The pinned decomp allocates `0x300` bytes at `0x800f45f0`, or 64
12-byte polygon records. Both receiver collection routines (actual ROM entries
`0x8001e8b8` and `0x8001ee1c`) append records without a polygon-capacity check.
Their vertex cache clamps to index 31, matching the repeated `03 00 1f 1f 1f`
pattern. Record 450 begins at `0x800f5b08`; its third vertex index lands exactly
at model word `0x800f5b0c`, explaining the captured `0x1f265a10` value.
The per-caster caller resets both counters before collection at `0x8001e684`
and `0x8001e698`. This is strong evidence of a shadow scratch overflow, but
neither the precise executed writer nor why so many polygons were accepted has
yet been captured. Do not repair the model pointer or claim that all native
failures share this cause. A private pre-overflow capture will retain CPU/RAM
before the first out-of-bounds append. Evidence: shadow-overflow-source-evidence.

The filtered register observer passed all nine controlled fixtures, including
allowed initialization, wrong-value, wrong-instruction and uncached writes.


The first filtered-watch route completes with 72 threads armed, no watched
writes, no captured corruption and no audio underruns. It is a non-reproduction,
not a stability fix. The first private shadow-limit build fails the generated
object-shape audit because placing the capture helper in a generated translation
unit adds functions. Its original sources are restored and rebuilt. The revised
probe places capture code in the host runtime and leaves the generated function
inventory intact; the audit is retained. Evidence: filtered-register-native-120,
shadow-limit-probe and shadow-limit-probe-v2. Probe results are diagnostic only.


## Oversized secondary-shadow footprint (2026-10-14)

The failed capture retains the projector return address and descriptor on the
guest stack. They identify Juno's secondary shadow, with a footprint about
13,059 world units across and stored shadow size about 652.94. Its ten model
bone positions have X values around 7,340, while Juno's current X is about
1,938. The source computes the secondary-shadow size from transformed model
positions. This links excessive receiver collection to an oversized footprint,
but the first bad matrix/position update has not yet been observed. The trace
returns from room 16 to room 21 at VI 10720, shortly before the VI 10743 fault.
A transition-related stale position is a hypothesis, not an established cause.

The zero-size-footprint hypothesis does not explain this capture. An isolated
clipping fixture demonstrates that degenerate input can admit unrelated
triangles, but the actual captured quad is enormous, not collapsed. Preserve
this distinction when interpreting the fixture.

The third private limit probe builds with the normal object-shape audit intact,
using a host helper and calls from two generated collector bodies. It stops
before the first append beyond 64 records and does not repair guest state.
The second attempt failed the host warnings-as-errors check for getenv; all
attempts restored original files and rebuilt afterward. The third probe's
first native run did not reproduce an overflow and remained in room 21; it
did not exercise the failing return transition. Evidence: shadow-footprint-
stack-evidence, shadow-limit-probe-v3, shadow-clip-degenerate-fixture.

No production fix, crash resolution or desktop delivery follows from these
findings. The shadow animation cadence issue remains separately open.


A separate CPU-context ownership check finds that detached copies used by the
synthetic overlay loader and menu sound retain the original context's f_odd
pointer. An isolated fixture using the actual generated context type reproduces
writes into the caller's odd floating-point registers in both FP modes. Rebasing
the copied pointer isolates those writes. The guest-thread transport already
rebases its resumed contexts. This is a concrete aliasing defect; whether a
loader actually changes a live caller register on the failed route remains
unobserved, so it is not yet assigned as the shadow overflow's cause. Evidence:
context-copy-fixture. No guest timing or state repair is implied by the fixture.


The detached-context candidate rebases the overlay-loader and menu-sound copies'
odd-register pointer to their own storage, and restores the caller's host FP
rounding mode after each helper returns. This fixes the isolated ownership
fixture; the native crash linkage is still pending. Original/full-route state
comparisons and native runs must qualify the change before delivery. The first
fixture build rejected anonymous unions in the imported generated ABI under
/W4 /WX; a local warning scope around that header preserves warnings-as-errors
for the fixture itself. The first model-position observer used unsigned guest
addresses with generated memory macros and faulted in the observer. Its second
version uses bounds-checked physical RAM reads; that diagnostic fault is not
an additional production-game failure. Evidence: context-copy-fixture-v2,
model-position-probe/v2, detached-context-candidate.


## Targeted room-return fixture

The backward-crossing fixture reaches room 21, then 16, then 21, with the return
observed at update 1128. It later stops in the renderer after about 52 seconds:
triangle source `0x002de860` contains vertex index 101, while only vertices 0-23
are loaded. The rejected command and triangle bytes agree between final guest
RAM and the retained submission snapshot. This rules out a difference between
those two retained copies at those bytes; it does not identify the first writer,
exclude an earlier lifetime problem or connect this failure to the shadow
scratch overflow. No model-position diagnostic fired first. This uses an isolated
navigation-warp fixture and is not evidence of a new failure on the owner's
unmodified route. Keep renderer validation enabled. Evidence:
shadow-return-backward and shadow-return-triangle-evidence.

The model-position observer's ordinary 17,000-retrace native run completes
without capturing a large mismatch or overflow. It is a non-reproduction, not a
fix. The detached-context candidate preserves exact guest RAM, save and per-update
instruction counts in the dense recorded combat replay. Full Original and native
room-return comparisons remain separately qualified by their result artifacts.


## Detached CPU-context ownership correction

The copied-context correction passes both the dense recorded combat comparison
and the 17,000-retrace Original-mode reference: guest RAM, saved data and per-update
instruction counts match; the full reference also matches the retained renderer
RAM. This verifies preservation for those inputs, not native timing or stability.
The isolated ABI fixture covers all sixteen odd registers in both FP modes.
Only detached execution copies are rebased; same-context snapshots and the
already-rebased guest-thread transport retain their existing behavior. Host FP
rounding is restored when the detached guest helper returns.

The native room-return comparison does not establish a stability improvement:
the predecessor completes 4,801 retraces, while the candidate stops after about
52 seconds with the existing runlink-address-table validation. Host-timed warp
requests produce different transition/update timing, so this is not an exact
before/after failure comparison. A related pre-correction diagnostic already
failed with invalid limb-triangle bytes. The first corrupting write remains open;
no renderer or module-address checks have been relaxed, and delivery stays held.
Evidence: detached-context-full-original, detached-context-dense,
return-context-before/after, return-context-comparison.

The reconstructed animation fixture produces the same approximately 5,406-unit
root X displacement from the saved failed RAM without live scheduling. This is
post-fault state, not a captured pre-call replay. Its input provenance and the
first contributing value still require tracing. Evidence: animation-snapshot-fixture.


## Animation prefix corruption localized

The retained animation table identifies Juno's active asset as animation 1063,
allocated at 0x802de7b0 with size 0x2e0. Comparing against the matching ROM asset
finds 57 changed bytes, all within its first 68 bytes; the remainder is intact.
The adjacent animation 1062 matches its ROM data except for its expected runtime
reference count. This is localized live asset corruption, not merely a stale
model/world matrix or an animation-decoder rounding discrepancy.

An isolated replay clears the output matrices and still reproduces the 5,406-unit
X error from the corrupt header/frame inputs. Restoring only the affected prefix
in that private fixture and rederiving the frame pointer/stride with the original
formula produces a root close to the actor again. This counterfactual establishes
the data's contribution to the oversized shadow; it is not an approved production
repair. No runtime asset replacement, pointer repair or overflow clamp is added.
A private observer arms only ROM-equal immutable animation headers, ignores the
mutable reference-count byte, and captures the first later difference. The first
writer and its lifetime contract remain open. Evidence:
animation-snapshot-trace, animation-header-restored-fixture,
animation-prefix-corruption-evidence, animation-header-watch.


## First invalid path-cache write captured

The immutable-header observer catches a valid animation 1053 prefix changing
near VI 3484 during overlay 155 pathfinding. A second observer stops immediately
before store PC 0x09b00180: its cache base/destination 0x802de830 is inside a live
allocation owned by module 142 (base 0x802de370, size 5792), rather than an owned
0x500-byte path cache. The write is within the cache's nominal 64-record range;
this is a stale allocation pointer, not an exceeded record count. The normalized
pathfinding state still associates that pointer with Juno. This captures an
actual invalid writer before subsequent animation, triangle or shadow damage.

The guest cold loader reloads DATA and clears BSS, whereas normalized host
publication currently reuses active mutable state. The pathfinding exit callback
frees cached allocations, making stale pointers across a later fresh load unsafe.
A lifecycle correction must distinguish a fresh load from suspension/resume,
reset normalized state before its new init callback, retain live allocator bases,
and preserve the original guest loader's allocation/relocation/completion work.
Evidence: animation-header-return, path-cache-return,
path-cache-stale-owner-evidence. Native stability and delivery remain unqualified.


The cold-load candidate passes the dense combat comparison and the formerly
failing room-21/16/21 ownership-watch run. The full Original replay preserves
physical guest RAM, save, all probe counters and total instructions, but fails
the previous whole-renderer-snapshot hash: 98 bytes differ in section 74 (guest
beam module 76), comprising its texture scroll phase and 48 associated UV
halfwords. All differences share a phase offset of 224; other snapshot bytes
are exact. This is not a complete renderer equality pass. The original beam
module's DATA contains this mutable phase, so a genuine cold reload should
reset it. A private pre/post scroll and fresh-load observer is required to
verify that explanation before accepting the changed reference. The initial
failed assertion and snapshots are preserved; no check is silently waived.
Evidence: overlay-cold-full-original, path-cache-after-return,
beam-phase-lifecycle-watch-v2. The first private observer did not compile
because its macro expected a local RAM pointer; sources were restored and rebuilt.
Normal native/pause qualification is deferred while
this specific renderer difference is reviewed.


## Fresh overlay lifetime correction, 2026-10-15 UTC

The host now records an actual fresh guest module load and recreates the
normalized ROM DATA/zeroed BSS before its init callback. It preserves the guest's
chosen allocation, relocation/PI work and callback order. Already-loaded calls
retain state. Suspended modules use the original resume path and retain their
mutable state. Direct normalized calls that must allocate a module also mark
the fresh load, including modules without init callbacks. Pending state is cleared
only after successful publication. No guest pointer repair, renderer-check
relaxation or shadow-buffer clamp is used.

This corrects the demonstrated lifetime defect: the pathfinding exit callback
frees cache allocations, but stale normalized BSS previously retained those
pointers after a later cold load. A pre-store observer previously stopped with
the cache pointer inside another module's allocation. With the correction, the
same targeted room-21/16/21 fixture finishes without an invalid write; a longer
fixture completes three observed room transitions and 9,000 retraces. Its eight
warp requests include retries and are not eight successful transitions. The
17,000-retrace owner-input native run also completes with the ownership observer
and no invalid-cache capture. These fixtures do not prove all historical native
exceptions share one writer.

The full Original comparison preserves physical guest RAM, saved data, all
probe counters, and all 4,754 complete per-update instruction records. The initial
whole-renderer-snapshot assertion fails by 98 bytes. The follow-up observer
accounts for every byte: 4,176 calls to the original beamScroll routine accumulate
phase 280 without cold resets, versus 56 with the reset at VI 2118. One phase
halfword and its 48 dependent texture-coordinate halfwords explain the complete
difference; every other renderer snapshot byte is identical. Both fresh-load
observations read phase zero. This is the intended DATA reset, not a blanket
renderer equality pass or evidence of a new position-jitter fix. The private
observer and normal candidate produce the same resulting renderer snapshot.

Two actual in-game Start-button pause/resume cycles complete and return to
play. The existing module-table/suspension unit suite passes. Normal, non-observer
120-FPS/VSync and 360-FPS/no-VSync runs each complete at least 17,000 retraces,
with unchanged source saves, no invalid accesses, no audio underruns/overruns,
and complete frame logs without dropped rows. Those native routes use host time
and can differ; they do not establish stable 30-Hz combat or physical-monitor
smoothness. Existing unrelated host load was owner-authorized.

Source: src/boot/native_boot.cpp, note_guest_cold_overlay_load,
ensure_guest_overlay_allocation and publish_synthetic_overlay. Evidence:
overlay-cold-dense, path-cache-after-return, path-cache-owner-after-120,
path-cache-multiple-return, beam-phase-reference-review, overlay-cold-guest-pause,
overlay-cold-native120/360 and overlay-cold-unit-test. The first beam observer
failed compilation because a memory macro lacked its local RAM pointer; both
probe attempts restore/rebuild normal source, and only the corrected observer
qualifies the renderer difference. Preserve the initial failed hash assertion.

Compatibility lesson: stable translated code addresses do not imply persistent
module DATA/BSS. Preserve cold-load versus resume semantics, and compare copied
overlay graphics as well as physical game RAM. A reference change must be
explained by exact original operations and lifecycle evidence. Combat cadence,
shadow-mask cadence, Unlimited pacing and owner-visible full-route acceptance
remain separately open. This checkpoint does not change the desktop package.


### Local preview delivery, 2026-10-15 UTC

Both existing high-refresh preview/capture desktop links now select the qualified
overlay-lifetime package, including the separately qualified optional trace
formatter. Runtime ae65a40c6fe437d0baedb17a2a70346320976ea7e2c3f2fd532ad43ab27b9fa7
contains the source correction in 06718f7f and formatting checkpoint 38847126.
The unchanged launcher passes bundled-runtime init/setup protocol checks; all
14 manifest entries verify. Shortcut arguments and existing profile/save/config
files remain unchanged. Previous links and package are retained for rollback.
This supersedes the earlier source-only delivery status. No public release or
full-route/live acceptance is claimed; PERF-001 and the visual issues remain open.


The transition observer subsequently confirms that the long native replay stays
in level 21 and includes a death/respawn reload. Its stable completion is valid
for that path, not complete owner-route traversal. Targeted room-return fixtures
and the level-26 exit-marker fixture provide separate, explicitly bounded room
coverage. The observed path-cache ownership repair remains qualified as recorded;
all-route stability and visual acceptance remain open. See PERF-001's 2026-10-15
coverage correction and transition-call-native-review.json.
