# RENDER-009: Ground shurikens sometimes jitter

Status: missing interpolation ownership reproduced; correction verified in trace tests and delivered locally; owner reports resolved in live play; broader coverage limits remain. Severity: visible gameplay/control regression. Affected: unified-keys high-refresh preview, source 19a2373, runtime SHA-256 93514316aada84480c437c3f6eb08dfe192b22a43fa29f678242d4143bf133d7.

## Symptoms and reproduction

The owner reports occasional jitter on shurikens lying on the ground in the latest capture. Other enemies/items now appear good by the owner report. Exact onset/object state is unknown.

Private evidence: shuriken-followup-20261016/owner-capture, copied and SHA-256 verified from owner session 20261016-192948-9c8c9b5ded9b427682d6affeda20b3bc. Raw timing and initial profile are retained; no owner settings are changed for investigation.

## Initial hypotheses and acceptance

Check actor/projectile identity and draw grouping, object rest/rotation state, vertex interpolation, and camera correspondence. At intake no cause or fix was established; the later runtime findings below supersede this initial hypothesis list.

Record candidate changes, informative failures, exact checks and remaining live acceptance here. Preserve game timing, saves and other preferences. See [shuriken reticle recurrence](shuriken-reticle-flicker.md).


Source lead and candidate: US module 122 ShirukenPushMtx (0x07A00F38) bypasses camPushModelMtx and calls camPushFloatModelMtx with a custom rolling/attached matrix. Our ordinary rigid observer never sees this actor. The owner capture has 7,805 shuriken actor observations and zero corresponding rigid-owned records. Candidate observes the complete custom matrix function with the same output/actor ABI, validates the 64-byte output advance, then uses the existing actor lifetime, byte validation and world/view/projection product checks. It does not change projectile physics or game memory. This was the candidate before the runtime checks below; owner movement/camera visuals remain pending.


## Runtime coverage

The replay now has 12,612 shuriken rigid-owned observations, of which 12,268 match a preceding pose. The baseline owner capture and first replay had zero. The remaining observations are not forced to match: lifetime, visibility gaps, discontinuities and operand validation retain the existing rejection rules. A separate unchanged-position analysis checks projectiles stationary for at least 30 VI. Private evidence: shuriken-followup-20261016/line-ground-analysis.json and stationary-coverage-lifetime-checked.json. These records establish that resting projectiles enter interpolation, not that every camera/ground pose is visually accepted.

The correction changes presentation identity only. The original custom rolling/attached matrix, physics and lifetimes stay native. Existing semantic ownership and renderer tests pass. Lesson: generic actor draw hooks do not cover custom projectile matrix builders; register their completed output with the same lifetime and exact-byte validation instead of guessing batch identities.


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
