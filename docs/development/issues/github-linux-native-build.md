# Linux native build rejects compiler-specific probe declarations

- ID: BUILD-002; severity: high for Linux native/AppImage builds.
- Source: [GitHub #15](https://github.com/TK22-26/JetForceGemini-Recomp/issues/15), filed 2026-10-16.
- Affected report: Linux AppImage source `8b6837d`, GCC 16.2.1, CachyOS x86-64.
- Status: reported compiler defects corrected in released v1.2.0 source; actual GCC native translation unit and full GCC/Clang host builds pass. A playable Linux/AppImage release remains unqualified.

## Initial report and audit (historical)

Both declaration and definition of `jfg_phase9_execution_probe_slow` still use `__declspec(noinline)` in `src/boot/native_boot.cpp`. GCC rejects that MSVC spelling. The reported missing declaration and unused `emit_device_event` diagnostics are consistent with that first compilation failure; they are not evidence that the event function should be removed.

The reporter proposes a compiler-selected noinline macro and reports a successful build/play on the Linux branch. That external success has not been reproduced here. Current source also unconditionally includes `include/jfg/runtime/async_volume_file.hpp`, which includes `<windows.h>` and uses Win32 file APIs. Replacing the noinline spelling alone therefore cannot qualify this branch for Linux.

No patch, upstream merge or Linux build was performed in this launcher review. A follow-up must use portable noinline declarations, provide or gate the volume-file implementation appropriately, and build the actual native/AppImage target on GCC with warnings-as-errors. Host-shell tests alone do not qualify the generated game runtime. This is a Linux release blocker; it is not evidence of a Windows runtime regression. Private report: `launcher-update-button-20261016/issue-15.json`.

Lesson: fix the first compiler error and assess the complete platform path; do not claim whole-platform support from a two-line compiler workaround.


## 1.2.0 candidate work

Owner authorized a two-hour release continuation. The source changes address the recorded portability/setup failures; focused regression tests and platform qualification are in progress. This candidate is not yet a verified or published resolution. Existing guard state and counters remain preserved.

## Additional actual-source compiler findings

Direct GCC compilation confirmed additional defects hidden by Windows-only builds:
an unguarded block of Windows live-runner diagnostic setup, a predecoded section
variable declared inside the platform guard but used outside it, non-live shutdown
touching live-only streams, and warning failures in a FILE deleter and control parser.
The candidate gives platform-only diagnostics their existing Windows/live boundary,
keeps the section decode portable, uses a callable FILE closer and clarifies parser
indentation. The non-Windows native runner still explicitly reports unsupported
process isolation at runtime; a playable Linux/AppImage product requires its Linux
frontend branch. These source fixes do not represent a new Linux game release.

The first direct compile also omitted SYSTEM treatment for private generated headers;
the retry restores the CMake policy rather than suppress handwritten-code warnings.
Linux bounded volume-file tests already pass. Full source retry remains pending.

## Verified candidate scope

The actual native translation unit now compiles under GCC 13 with -Wall/-Wextra/-Wpedantic/-Wconversion/-Wshadow/-Werror. Windows native, Linux volume and Linux input checks pass. The separate Linux frontend/AppImage branch and reporter GCC 16.2.1 runtime remain outside this Windows release qualification.

## Published release verification

Available in [v1.2.0](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0), built from `f56cd94c4bd7583bceee201eb3f229f744216bc1` after
[PR #16](https://github.com/TK22-26/JetForceGemini-Recomp/pull/16) merged. The
[tagged-release workflow](https://github.com/TK22-26/JetForceGemini-Recomp/actions/runs/38981221560) passed its full checks, verified the prebuilt
Windows bundle and published it. The public download was downloaded again and
its ZIP, per-member hashes, package inventory and clean runtime identity verified.
This release includes the remedy and finite verification described above; it
does not expand the recorded hardware, Linux, campaign or fresh-install coverage.
