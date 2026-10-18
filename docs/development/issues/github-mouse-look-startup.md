# Mouse-look enabled before launch: reported startup crash

- ID: NATIVE-002; severity: high, release qualification blocker.
- Source: [GitHub #14](https://github.com/TK22-26/JetForceGemini-Recomp/issues/14), filed 2026-10-16.
- Affected report: v1.1.0, Windows 11, GTX 1650, keyboard/mouse. Mouse look enabled before launch reportedly fails every attempt; enabling after launch reportedly works.
- Status: resolved for the reproduced keyboard startup failure in released v1.2.0; exact old-runtime comparison and enabled startup checks pass. Reporter hardware confirmation remains separate.

## Initial report and audit (historical)

The supplied support ZIP has SHA-256 `b86703183465c8906d41ef56e6149acde31b87f5c3f553ea6806004a1d961a93`. Its native log reaches `rom-ready`, then reports `0xc0000005`. The runtime build source is `d3b9d62e66cc4fdc2c27be5ad2e457a8ea7da394`; runtime SHA-256 `fa6e0ec8068a18ae23eaa24eb1a5cb66bbbe4d3a0b6197adda45ff6109a1cd80`. Crash, hang, breadcrumb and system logs are empty. The support data does not contain the modern mouse configuration or a stack. The reporter's OS description remains authoritative; the legacy launcher version string is not reliable OS identification.

Current source uses selected modern-control configuration and gates camera ownership during loading. Existing loading/entry tests cover a different defect ([CAMERA-003](modern-controller-state-transitions.md)); they do not identify this access violation. Neither later source changes nor successful owner play prove the reported launch sequence is fixed. The unrelated extended-play crash record [NATIVE-001](native-play-access-violation.md) cannot close this startup report.

No runtime remedy was attempted in this launcher-only review. Next qualification must reproduce enabled-before-launch versus enable-after-launch with the reported runtime and current candidate, copied profiles and matching crash symbols. Preserve the failing settings and startup stack before assigning a cause. Until then the Windows full-update gate remains open. Private evidence: `launcher-update-button-20261016/issue-14-support` and `issue-14.json`. No personal save or ROM is published.

Lesson: a bare access-violation exit code is not a diagnosis; generic camera fixes must not be used to close a startup crash without corresponding evidence.


## Reproduction and existing remedy identified (1.2.0 qualification)

The exact published runtime now reproduces the report when **keyboard** is selected and experimental controls are enabled before launch: it reaches ROM-ready and exits with `0xc0000005`. Disabling that flag with the same keyboard profile reaches the 120-retrace target. The first attempt used automatic device selection with a connected gamepad and did not reproduce; preserve that informative mismatch rather than count it as crash coverage.

The released startup calls `sample_controller_ports` before assigning `state.rdram`. The experimental keyboard path reads menu/gameplay memory immediately, dereferencing that null pointer. Existing commit `66e14d60` moves the RDRAM assignment immediately after State construction, before controller discovery. It is already included in this candidate. The current accepted runtime passes both automatic-controller and explicitly selected keyboard startup with Modern Controls enabled, reaching the same target without invalid accesses. This is not a fix obtained by turning modern input off.

Private evidence: `release-1.2.0-20261016/startup-keyboard-comparison.json` (matching released binary, on/off) and `startup-comparison.json` (current enabled profiles and the initial non-reproducing automatic baseline). The original source-level suspicion is now backed by an exact released-binary reproduction. Final 1.2.0 binary startup and longer muted qualification remain pending; no public release yet.

## Verified candidate scope

The 1.2.0 Windows candidate passes 1,200 retraces with modern keyboard controls enabled before launch and zero unsupported accesses. Existing early RDRAM assignment fixes the exact reproduced startup cause. Final release CI/publication is tracked in the release-readiness record.

## Published release verification

Available in [v1.2.0](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0), built from `f56cd94c4bd7583bceee201eb3f229f744216bc1` after
[PR #16](https://github.com/TK22-26/JetForceGemini-Recomp/pull/16) merged. The
[tagged-release workflow](https://github.com/TK22-26/JetForceGemini-Recomp/actions/runs/38981221560) passed its full checks, verified the prebuilt
Windows bundle and published it. The public download was downloaded again and
its ZIP, per-member hashes, package inventory and clean runtime identity verified.
This release includes the remedy and finite verification described above; it
does not expand the recorded hardware, Linux, campaign or fresh-install coverage.
