# Open GitHub issue review before a full update (2026-10-16)

**Initial audit (before the 1.2.0 fixes below): not all reports were resolved.** The owner accepted the current local game and amber updater design. That acceptance does not close independent GitHub reports.

Used `gh issue list --state open` and `gh issue view` with comments for all four open issues. The supplied startup support ZIP was read privately. Source and existing evidence were compared; no issue was closed, comment posted or release published.

| GitHub issue | Finding on this candidate | Release disposition |
|---|---|---|
| [#15 Linux compilation](https://github.com/TK22-26/JetForceGemini-Recomp/issues/15) | MSVC-only probe declarations and an unconditional Windows volume-file header remain | [Open Linux blocker](github-linux-native-build.md) |
| [#14 mouse-look startup crash](https://github.com/TK22-26/JetForceGemini-Recomp/issues/14) | Support log confirms access violation after ROM-ready; no stack or verified current-build remedy | [Open Windows qualification blocker](github-mouse-look-startup.md) |
| [#13 controller vertical camera](https://github.com/TK22-26/JetForceGemini-Recomp/issues/13) | Modern-controls GUI inversion persists and reverses camera/aim input; 790 UI checks pass | [Implemented/test-verified, reporter retest pending](github-controller-vertical-inversion.md) |
| [#4 Windows 10 source setup](https://github.com/TK22-26/JetForceGemini-Recomp/issues/4) | Native stderr, short paths, submodule retry and external recovery roots addressed; firmware/old-WSL gaps remain | [Partial; legacy source setup](github-windows-source-setup.md) |

The updater passes 78 UI assertions, 28 checker cases and a live packaged-helper check. It retains the accepted game runtime. The fresh setup fixture passes 15 assertions; it does not emulate an old WSL or disabled BIOS virtualization. No Linux/native gameplay experiments were launched during this launcher review. Existing higher-refresh guard counters and shelved investigations are preserved.

Before a broad release: reproduce and qualify #14, validate #15 on the actual Linux target if Linux is included, and resolve or explicitly scope the remaining legacy source-setup limitations. Obtain the reporter's controller confirmation when practical. Preserve the separate all-route/Unlimited/exit-cutscene limits in existing records; owner-reported smooth play does not certify every environment.

[Machine-readable audit](../../../evidence/github-release-readiness-20261016.json) and [updater qualification](../../../evidence/launcher-update-button.json). These results concern the local candidate, not retroactive changes to v1.1.0.

## 1.2.0 release-candidate qualification

The four-report review now has concrete remedies and bounded verification:

| Report | Remedy and validation | Remaining scope |
|---|---|---|
| #14 startup | Exact v1.1.0 runtime crashes with experimental keyboard input before game-memory assignment; current early assignment passes enabled startup, including a 1,200-retrace muted run | Reporter hardware is not reproduced; broad gameplay acceptance is separate |
| #15 GCC build | Portable noinline and volume reader; guarded Windows-only diagnostics, portable predecoded section and strict-warning corrections. Actual native translation unit compiles under GCC with all project warnings as errors; Linux volume/input tests pass | Windows package only; no new Linux AppImage or GCC 16.2.1 runtime qualification |
| #13 inversion | Existing controller vertical inversion persists and reverses camera/aim deltas; 790 native controls checks pass | Reporter physical-controller confirmation remains useful |
| #4 setup | Firmware virtualization diagnosis, old WSL catalog refresh, distinct failure/restart handling; 28 setup checks pass | Fresh Windows 10 installation and unrelated optional-tool comments remain unverified |

The launcher passes 642 managed checks, 790 modern-controls checks and 78 update
banner checks. Windows native build and input/menu/volume checks pass. Exact-body
source policy, package verification and release validation tests pass. A full local
Python run executed 1,790 tests with 48 skips and three failures in the pinned WSL
trust/sanitizer harness on this host; those failures are retained, not relabeled as
passes or fixed by changing trust pins. CI qualification is still required.

Initial direct Linux probes exposed missing build-harness include/link settings,
then genuine platform-boundary and strict-warning defects. The retries preserve
SYSTEM treatment for generated headers and warnings-as-errors for project source.
Launcher tests also assumed their output was inside the repository; source lookup
now supports the configured external output, and test screenshots stay beside it.

Private receipts: release-1.2.0-20261016. Game/input source fixes preserve the owner-
accepted Windows behavior. No clean all-route/Unlimited/audio guarantee is claimed.
This record describes a committed release candidate once the following commit is
created; publication is tracked separately. Initial audit findings above are history.

## Published release verification

Available in [v1.2.0](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0), built from `f56cd94c4bd7583bceee201eb3f229f744216bc1` after
[PR #16](https://github.com/TK22-26/JetForceGemini-Recomp/pull/16) merged. The
[tagged-release workflow](https://github.com/TK22-26/JetForceGemini-Recomp/actions/runs/38981221560) passed its full checks, verified the prebuilt
Windows bundle and published it. The public download was downloaded again and
its ZIP, per-member hashes, package inventory and clean runtime identity verified.
This release includes the remedy and finite verification described above; it
does not expand the recorded hardware, Linux, campaign or fresh-install coverage.

The four GitHub reports were reviewed and their concrete source corrections are
included. #14's exact reproduced startup defect is resolved. #13's requested GUI
toggle is available. #15's compiler corrections are shipped as source, without a
Linux AppImage claim. #4's setup remedies pass fixtures, without a fresh Windows
10 installation claim. GitHub issues have not been automatically closed or
treated as fully reporter-verified.

The live released updater returns `current` / `v1.2.0`. Owner desktop launchers
now point to the verified package while retaining their original profile and
diagnostic arguments; previous shortcuts and binaries are preserved privately.
