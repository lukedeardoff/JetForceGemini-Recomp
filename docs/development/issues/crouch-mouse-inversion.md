# INPUT-009: Mouse vertical direction reverses while crouching

Status: reproduced and corrected in tests; delivered locally and accepted by owner in live play; broader coverage limits remain. Severity: visible gameplay/control regression. Affected: unified-keys high-refresh preview, source 19a2373, runtime SHA-256 93514316aada84480c437c3f6eb08dfe192b22a43fa29f678242d4143bf133d7.

## Symptoms and reproduction

The owner reports mouse controls seem to become inverted again when crouching, despite their saved vertical inversion preference. Exact crouch/roll/aim movement state and whether the preference value itself changes are unverified.

Private evidence: shuriken-followup-20261016/owner-capture, copied and SHA-256 verified from owner session 20261016-192948-9c8c9b5ded9b427682d6affeda20b3bc. Raw timing and initial profile are retained; no owner settings are changed for investigation.

## Initial hypotheses and acceptance

Compare signed input and camera pitch through locomotion transitions before changing profile settings. Ordinary and aiming camera ownership may encode pitch differently; this is a hypothesis. Test inversion Off and On, crouch entry/exit, and scope/airborne behavior.

Record candidate changes, informative failures, exact checks and remaining live acceptance here. Preserve game timing, saves and other preferences. See [shuriken reticle recurrence](shuriken-reticle-flicker.md).


Candidate cause: movement state 5 uses controlGetManualAim outside shoulder ownership. The old hook negated camera-space pitch only while shoulder ownership was active, making crouch consume the opposite sign. Candidate converts modern action-table look independently of shoulder ownership; legacy modes retain their established convention. Baseline and candidate comparisons are recorded below.


## Established cause and checks

The saved inversion option is unchanged. Modern look arrives in camera-pitch space, but native controlGetManualAim consumes the opposite vertical convention. The old conversion depended on shoulder ownership; native crouch state 5 excludes shoulder ownership and therefore reversed the same input. Modern action-table input now uses one conversion for the guest aim path regardless of camera owner. Legacy replay conventions remain unchanged.

The same positive-look tape produces 25 consecutive crouch steps of -30 before the change and +30 afterward. A negative-look tape verifies the opposite direction. The 363 PC input checks include both saved inversion polarities and legacy behavior. Renderer, semantic presentation and shell suites also pass. These checks do not establish every swimming/scope/character combination or owner hardware acceptance. Private evidence: shuriken-followup-20261016/crouch-comparison.json, negative-pitch-result.json and focused-tests.json.

Lesson: translate coordinate conventions at their input boundary, independently of which camera owns the current locomotion state. Do not migrate user preferences to compensate for a state-dependent sign bug.


The shared followup runtime and bounded checks are pinned in [qualification](../../../evidence/high-refresh-shuriken-followup.json). Owner confirmation was subsequently received on 2026-10-16; see the final confirmation below.


### Followup delivery (2026-10-16)

Source `95efb8f97b47717921aa4c29ab17245771c3ff22` is delivered through both **JFG - High Refresh Preview** and **JFG - Jitter Capture** desktop shortcuts. Three setup/protocol checks selected the new runtime with fresh and copied profiles; all 41 original profile files remained unchanged. Previous packages and shortcut backups are retained. This is a local tested preview, with sustained owner visual confirmation still pending. [Qualification and limits](../../../evidence/high-refresh-shuriken-followup.json).


### Owner live confirmation (2026-10-16)

After testing the delivered followup, the owner reports no noticeable slowdowns and that all reported issues seem resolved, and authorizes committing. This closes the pending owner confirmation for this local preview within that reported play session. It is not a public release or exhaustive all-route/Unlimited acceptance. No further runtime experiments or game changes are part of the launcher update-button task.

## Published release verification

Available in [v1.2.0](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0), built from `f56cd94c4bd7583bceee201eb3f229f744216bc1` after
[PR #16](https://github.com/TK22-26/JetForceGemini-Recomp/pull/16) merged. The
[tagged-release workflow](https://github.com/TK22-26/JetForceGemini-Recomp/actions/runs/38981221560) passed its full checks, verified the prebuilt
Windows bundle and published it. The public download was downloaded again and
its ZIP, per-member hashes, package inventory and clean runtime identity verified.
This release includes the remedy and finite verification described above; it
does not expand the recorded hardware, Linux, campaign or fresh-install coverage.
