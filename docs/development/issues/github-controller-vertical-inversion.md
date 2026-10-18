# Controller camera vertical inversion request

- ID: INPUT-010; severity: medium controls/usability.
- Source: [GitHub #13](https://github.com/TK22-26/JetForceGemini-Recomp/issues/13), filed 2026-10-16.
- Affected report: v1.1.0, Windows 11, Xbox 360 controller. Right-stick vertical camera direction feels reversed; asks for a modern default or a GUI inversion toggle.
- Status: requested GUI inversion toggle is available in v1.2.0 and verified for persistence and camera/aim deltas. Reporter physical-controller confirmation remains pending.

## Initial report and audit (historical)

For a controller selected in Modern Controls, **Sensitivity & behavior > Invert vertical** changes `look.aim_invert_y`. `pc_stick_look` uses that value for both normal camera input and aimed input. Mouse inversion is independent. No preference migration, forced reset or runtime behavior change was made in this audit.

The focused regression clicks the actual controller toggle, reloads the saved profile, verifies opposite vertical look deltas for both camera and aim, verifies the mouse preference is preserved, and toggles back. The complete native controls UI fixture passes **790 assertions** with injected controller state and no desktop input. Private evidence: `launcher-update-button-20261016/issue-13-ui-checks/result.json`; test: `launcher/ui/tests/modern_controls_ui_tests.cpp`. Existing pure input tests cover endpoint normalization, deadzones, sensitivity and inversion.

This establishes the alternative requested by the reporter (a GUI toggle), not physical Xbox hardware acceptance or that all original/modern mappings have identical defaults. GitHub remains open; a release including these controls and a reporter retest should precede closure. Do not confuse movement-stick inversion in Controller Mapping with camera-look inversion in Modern Controls.

## Published release verification

Available in [v1.2.0](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0), built from `f56cd94c4bd7583bceee201eb3f229f744216bc1` after
[PR #16](https://github.com/TK22-26/JetForceGemini-Recomp/pull/16) merged. The
[tagged-release workflow](https://github.com/TK22-26/JetForceGemini-Recomp/actions/runs/38981221560) passed its full checks, verified the prebuilt
Windows bundle and published it. The public download was downloaded again and
its ZIP, per-member hashes, package inventory and clean runtime identity verified.
This release includes the remedy and finite verification described above; it
does not expand the recorded hardware, Linux, campaign or fresh-install coverage.
