# Windows 10 legacy source setup failures

- ID: SETUP-001; severity: high for affected source-build setup, outside normal prebuilt v1.1.0 ROM-import flow.
- Source: [GitHub #4](https://github.com/TK22-26/JetForceGemini-Recomp/issues/4), opened 2026-10-03; comments reviewed through 2026-10-09.
- Affected report: v0.4.0-preview.1 (`c88c411`), Windows 10 22H2, PowerShell 5.1.
- Status: firmware/old-WSL/failure-handling corrections released in v1.2.0; 28 setup checks pass. Fresh Windows 10 installation and unrelated optional-tool reports remain unverified.

## Initial report and audit (historical)

| Report | Current evidence | Remaining acceptance |
|---|---|---|
| Firmware virtualization disabled loops on restart | `Ensure-BuildTools` still converts missing Ubuntu after installation into restart-required; no `VirtualizationFirmwareEnabled` detection | Reproduce disabled firmware and report the actual prerequisite without a restart loop |
| Old WSL rejects Ubuntu-24.04 | Installer still invokes the fixed distribution name; no old-WSL update/fallback negotiation | Qualify a supported upgrade or explicit actionable error on old WSL |
| PowerShell 5.1 treats native stderr as fatal | `Invoke-SetupProbe` temporarily relaxes the error preference, captures the native exit, and restores Stop; checked commands preserve exit codes | Current **15 setup assertions pass**, including stderr/negative-probe checks; full reporter-machine setup still needed |
| Deep dependency paths and incomplete submodules | Short build roots, `core.longpaths=true` on dependency operations, and submodule update outside the clone-only branch are present | Fresh Windows 10 long-path/incomplete-checkout retest |
| Later recover-task comment | External new output roots are accepted while existing/protected checkout paths are rejected; owner comment identifies fix `32e2f3b` in v1.1.0 | Preserve the existing path protections |

Private fresh fixture receipt: `launcher-update-button-20261016/issue-4-setup-tests.log`. No tools, distributions, BIOS options or machine-level settings were installed or changed by the fixture. The report's later optional shadow work and inspector/pipeline failure observations are distinct from the four setup items; this audit does not certify them resolved. Existing shadow/rendering records retain their own scope. The request for public oracle tooling is a maintainer/distribution question, not a setup fix.

No new setup remedy was attempted in this launcher review. [Known issues](../../known-issues.md) already distinguishes legacy source setup from prebuilt ROM import. The issue must not be marked wholly resolved based only on the stderr helper or because the normal package no longer needs WSL. Compatibility lesson: a negative prerequisite probe, a failed installer and a required reboot are separate outcomes.


## 1.2.0 candidate work

Owner authorized a two-hour release continuation. The source changes address the recorded portability/setup failures; focused regression tests and platform qualification are in progress. This candidate is not yet a verified or published resolution. Existing guard state and counters remain preserved.

## Candidate qualification

The implementation now detects disabled firmware virtualization before setup,
refreshes Microsoft WSL when Ubuntu-24.04 is absent from its distribution catalog,
and treats only explicit reboot exit codes as a restart requirement. A successful
installer without registration produces an actionable error. Existing distributions
are preserved. All 28 mocked setup assertions pass, including old-WSL, firmware,
failed-install and missing-registration cases. No BIOS, WSL or system settings were
changed by tests. Full fresh Windows 10 installation and unrelated optional tooling
comments remain outside this verified scope.

## Published release verification

Available in [v1.2.0](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0), built from `f56cd94c4bd7583bceee201eb3f229f744216bc1` after
[PR #16](https://github.com/TK22-26/JetForceGemini-Recomp/pull/16) merged. The
[tagged-release workflow](https://github.com/TK22-26/JetForceGemini-Recomp/actions/runs/38981221560) passed its full checks, verified the prebuilt
Windows bundle and published it. The public download was downloaded again and
its ZIP, per-member hashes, package inventory and clean runtime identity verified.
This release includes the remedy and finite verification described above; it
does not expand the recorded hardware, Linux, campaign or fresh-install coverage.
