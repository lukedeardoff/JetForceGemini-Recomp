# Support diagnostics

Maintainer reference for the launcher's support ZIP, native crash/freeze
capture, symbol resolution and the opt-in gameplay trace. Testers should use
[playtesting and reporting](../playtesting.md), which lists the ZIP contents.

## Support sessions and export

The launcher selects a session explicitly and exports eight bounded text files
plus instructions. It accepts only the grammar in `SupportSession.SafeLine`,
both when recording and when exporting, and never archives a whole directory.
Game data, raw stack/register contents, personal paths and symbol files are
excluded. GitHub issue attachments are the intake; nothing uploads automatically.

- **Crash stacks.** `frame=thread/index/module/pe-timestamp/image-size/rva`
  gives a module-relative instruction location. The native crash handler starts
  from the exception's CONTEXT and unwinds at most 48 frames; its SEH boundary
  tolerates unreadable stack metadata. Catastrophic stack/heap failure or forced
  termination can prevent capture.
- **Freeze capture.** The separate `jfg-support-capture.exe` checks the launched
  process's creation time and executable, prefers its direct native child, and
  revalidates identity before taking a Windows PSS snapshot. Only stack
  locations leave that process. DbgHelp runs serially with an empty symbol path
  and no symbol downloads. Limits: 64 threads, 48 frames per thread, 256 frames
  overall, 12-second launcher deadline. No game thread is explicitly suspended.
  A failed retry keeps the previous capture and logs the failure in `launcher.log`.
- **System details.** GPU IDs list installed PCI display adapters with registry
  driver versions; they do not prove which adapter RT64 selected.
- **Recent progress.** A rolling 60-sample window of elapsed milliseconds since
  the first VI sample, frame, retrace and controller-poll counts. It
  distinguishes stalled emulation from continuing progress.

## Build identity and symbols

`build_from_rom.py` enables local MSVC symbols and writes an adjacent
`jfg-native-boot.exe.support` manifest. The launcher accepts it only when its
executable SHA-256 matches the selected game, and enables freeze capture only
when the helper hash also matches. `source=` is the launcher revision;
`build_source=` identifies the game source, with a dirty flag and source-content
hash, plus the dependency lock, local PDB hash and PE CodeView GUID/age.
Keep the exact executable, dependencies and symbols locally: rebuilding the
same source does not guarantee identical instruction offsets. Neither the
executable nor the PDB belongs in a public issue.

Resolve a report locally:

```powershell
python scripts/resolve_support.py --report JFG-support.zip --executable path/to/jfg-native-boot.exe --symbols path/to/jfg-native-boot.pdb
```

The resolver checks the executable and PDB hashes and the PE CodeView identity
before resolving game frames; its output stays local. To resolve manually, match
the hashes in `build.log`, load the executable and PDB in a debugger, and
resolve each RVA relative to the module base. For system or driver frames, match
module name, PE timestamp and image size first. Report a mismatch as unresolved;
never guess from another build. CodeView GUID bytes are stored in file byte
order, followed by the age in hexadecimal.

## Gameplay trace

Set `JFG_GAMEPLAY_TRACE` to a writable file before starting the native runner
to record the opt-in `jfg-gameplay-trace-v1` timeline. Rows contain decimal
elapsed microseconds, an event name and hexadecimal values; float guest fields
keep their IEEE-754 bit patterns. The buffer flushes once per second and at
controlled exits and stops at 256 MiB with a limit marker; a hard crash can lose
the final interval.

Events cover controller input, VI ticks, completed guest updates, graphics
preparation/submission/presentation (`present-skip` when the selected target
is unavailable), audio queue/start/pause-low/resume/underrun, scene state and
changed actor state. Actor sampling covers every type when the actor table has
at most 256 entries (otherwise an `actor-table-invalid` event); rows include
name words, 32 actor words and eight words each of properties and control data,
with `ffffffff` for invalid reads. The authoritative payload order is in
`trace_gameplay_state` and the `gameplay_trace.event` calls in
`src/boot/native_boot.cpp`. Paired replays with tracing off and on produced
identical final RAM. Trace files contain raw guest memory: keep them private.

## Validation

```powershell
./scripts/build_launcher.ps1 -Test
python -m unittest discover -s tests -p test_runtime_identity.py
python scripts/build_windows.py --config Release --test
```

The Windows integration fixture triggers a real access violation, checks fault
frames, captures a waiting process, rejects a stale process identity and verifies
continued progress after capture. Launcher tests cover failed-session retention,
safe exports, legacy/stale build metadata and setup error projection; Windows
PowerShell 5.1 setup tests exercise a real subprocess writing stderr with a
nonzero exit. These checks do not cover clean-machine UAC setup or gameplay.

API references: [process snapshots](https://learn.microsoft.com/en-us/windows/win32/api/processsnapshot/nf-processsnapshot-psscapturesnapshot),
[thread records](https://learn.microsoft.com/en-us/windows/win32/api/processsnapshot/ns-processsnapshot-pss_thread_entry),
[DbgHelp stack walking](https://learn.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-stackwalk64).
