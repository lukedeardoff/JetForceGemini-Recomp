# RENDER-008: Shuriken reticle flickers and disappears

Status: recurrence reproduced; both CPU-import paths corrected and verified in bounded tests; delivered locally and accepted by owner in live play; broader coverage limits remain. Severity: visible aiming feedback failure. Affected: local draw-key preview, 120 FPS, VSync On, modern mouse/keyboard, runtime source 27ddef0a5ed4afd1f1f9c26a5590b590cda5fe8e (SHA-256 db65b460f399b550ca8a3ad0d98f5374d5e00a95dc4a7073edfa2649fa6772c6).

## Symptoms and reproduction
The owner reports that the shuriken reticle flickers and eventually disappears altogether while continuing the tutorial capture. The owner explicitly confirms this occurs both with Aim held and without Aim. Exact first affected frame, weapon ammo/state and native no-Aim visibility rules are not yet established; do not infer the original game always draws a hip-fire reticle.

Report marker: 2026-10-16 13:43:01 UTC, about 429 seconds after the session's 13:35:52 start. This is when the report was recorded, not an exact glitch onset timestamp. Private evidence: final-measurement-20261016/session 20261016-133552-e6cd10c21fef4225a62f3f7b22eaadfa. Inputs, timing, interpolation and gameplay streams continue. Preserve startup/idle and input timestamps in raw evidence.

## Initial investigation and hypotheses
Historical commit f53dea1dcd64262c4b90e45864b0f33d87908863 preserved CPU framebuffer effects for software-drawn crosshairs and rain through color readback, CPU-edit import and game-selected buffer presentation. It is an ancestor of the active build. The remedy was not simply omitted by the develop merge.

Potential causes include loss/overwrite of CPU-drawn reticle pixels during interpolated presentation, wrong framebuffer ownership/lifetime, or guest weapon/aim-state gating. These are hypotheses; none is established by the live report. The existing airborne-aim adapters and tests cover pistol/sniper, not all shuriken states. Owner reports both aiming modes, so do not restrict reproduction to airborne aim.

No experiment, rebuild, runtime change or claimed fix accompanies this initial source review while the owner is measuring performance. After the run, correlate recorded inputs/weapon state and inspect framebuffer changes/presentation ownership, using a copied save and muted bounded replay if the existing guard scope permits. Preserve every failed or inconclusive result.

## Compatibility and acceptance
Reticle pixels may be CPU framebuffer edits rather than ordinary graphics batches. Verify real final presentation and the selected guest buffer; smooth actor interpolation does not prove software effects survive. Require stable shuriken visibility across sustained aim, throws, movement and mode transitions, with native visibility expectations checked. See [continuous airborne aim](widescreen-pointer-and-airborne-aim.md) and [high-refresh rendering](high-refresh-rendering.md).


## Candidate: CPU edits on fresh interpolated targets

VI framebuffer WriteChanges update the base target and the existing completed-scene endpoint, but not fresh interpolated targets. The candidate applies matching framebuffer color-change masks after each fresh target becomes available, outside the interpolation mutex. Untouched pixels, base/depth/unrelated buffers and the separate repeated-endpoint path are preserved. This is a plausible cause, not yet a proven explanation of the shuriken report. The first recorded-gameplay-time replay stopped at gameplay-landmark-mismatch during startup and is excluded from acceptance. A plain recorded-poll replay checks route reproduction before visual comparison.

The first plain-poll replay completed at VI 21,500 but remained on pistol, diverging from the owner route. Its screenshot and metrics are retained and excluded from shuriken acceptance. A native-clock baseline is being compared instead. The candidate builds and renderer interpolation tests pass; no visual reticle fix is claimed yet.


Added private phase-selective framebuffer capture because ordinary snapshots can repeatedly sample only the base frame. With interpolation tracing enabled, JFG_DIAGNOSTIC_CAPTURE_PHASE selects a numbered output phase and logs capture-copy. A capture-only CPU_WRITE_BASELINE switch disables only the candidate fresh-target mask path for controlled A/B evidence; normal rendering defaults to the candidate. This is diagnostic scaffolding, not visual acceptance. Planned fixture holds native Aim on a copied save after a recorded startup, avoiding unrecorded settings changes and route navigation.


The held-pistol phase fixture produced no CPU framebuffer write operations, so it does not qualify the suspected shuriken path. Pinned symbols identify module 13 frontDrawTarget, which selects a weapon-specific target table and calls the software line drawer. A private, replay-only draw fixture temporarily selects the shuriken table only inside that original drawing function and restores the weapon byte before return. It requires phase capture plus JFG_DIAGNOSTIC_SHURIKEN_DRAW; live play cannot enable it. This does not grant inventory or modify saves and must not be described as a successful replay of the owner route. It isolates software reticle pixels for phase A/B validation.


### Isolated rendering fault established
The original shuriken drawer produces visible green reticle strokes in Original mode, while the same stationary fixture loses them in 120 FPS output phases 0, 1 and 3. Both the initial matching-present-mask candidate and its control lose the reticle. Workload tracing shows 661 alternating 320x240 RGBA16 CPU framebuffer updates; the writes enter checkRDRAM/startFbOperations, so the later VI present.fbOperations list is empty. SkipBuffering displays a newer scene buffer instead of the game-selected buffer with those strokes. The first candidate was insufficient and is replaced.

New candidate retains the CPU changed-pixel textures for recognized VI buffers, with a bounded three-slot pool and per-presentation shared ownership. It composites that mask into every fresh output in the existing GPU command list, while Original mode stays on its existing path. Masks must match the selected VI address and scene age, and are released before device destruction. This is not yet qualified or delivered; exact native visibility, aim transitions, different refresh rates, performance and later owner play remain acceptance requirements. Private evidence: shuri-original, shuri-base-0/1, shuri-fixed-1/3 and shuri-workload-observe under shuriken-reticle-20261016.


The retained-mask candidate passes bounded original-drawer tests at 120 FPS (base and final output, including VSync On), 360 FPS final output, movement while aiming, and releasing Aim. Unlimited is explicitly excluded from clean qualification because the native-clock stress probe records audio underruns and slower game updates. The same binary with compositing disabled is the visual control. All captures verify their requested output phase, original weapon selection is restored, no retained mask is applied beyond its tested scene age, and audio counters stay zero. The fixture temporarily selects only the drawing table; it does not validate a full shuriken gameplay route or alter owner inventory. Visual review of the comparison panel confirms the original strokes, their absence in both controls, restored strokes at 120/360 and during movement, and clean disappearance after Aim release. Owner sustained throwing/aiming remains pending. The tested application copies pixel masks using a bounded three-slot cache, holds an immutable reference through the complete presentation sequence, and records composition into the existing GPU submission.

The retained-mask path currently qualifies recognized 320x240 RGBA16 VI buffers of the supported US game. Other buffer formats and dimensions are left on their existing path. The final test invocation initially named an obsolete presentation-schedule target retained only as an old build artifact; it failed before running that target. Rebuilding the current semantic-presentation target plus shell/interpolation/async-writer/modern-control targets passes; the old executable is not used for acceptance.


Final qualified runtime SHA-256: `bee0d0018bf7e2a14585107d51bb6d77bbebd88d8ba0a4c0970adbb1c9cf8315`. Five focused renderer/presentation/diagnostic/control executables pass. Normal Play mode with detailed gameplay tracing and semantic hashing both disabled records 2,070 consecutive game-update rows and matching final audio counters (zero underruns/overruns). See [qualification receipt](../../../evidence/high-refresh-reticle.json). The corrected mask path is a rendering remedy, not a change to weapon logic or the native no-Aim visibility rules. Full sustained throwing and aiming remains an owner live check.


### Local preview delivery

Source commit `26295c75dd43848f59cdd1268518d7131861dbee` is packaged in the local High Refresh Preview and Jitter Capture desktop links. The updated launcher includes the menu/dialogue binding editor and removes periodic capture screenshots. Three copied-profile setup/protocol checks select the new runtime and preserve saves, controls and settings; all 41 checked original profile files are unchanged. The previous package and both shortcut backups remain available. This is bounded test verification and local delivery, not a public release or full shuriken gameplay acceptance. Reopen the preview for sustained Aim, non-Aim throwing, movement and weapon-transition review.


## Owner recurrence after local qualification (2026-10-16)

On the unified-keys preview (source 19a2373, runtime SHA-256 93514316aada84480c437c3f6eb08dfe192b22a43fa29f678242d4143bf133d7), the owner reports the shuriken reticle initially worked, disappeared completely, and returned after death. The owner otherwise reports the game looks good except occasional ground-shuriken jitter and crouch mouse inversion. Death recovery provides a state-reset comparison, not a proven cause. The prior isolated original-drawer fixture did not cover this sustained gameplay transition; do not mark the issue resolved. The completed capture started 19:29:48 UTC and was preserved with hashes under private shuriken-followup-20261016/owner-capture. Compare actual weapon/aim state, CPU draw/import activity, framebuffer address, and mask age before loss and after death.


Latest owner trace analysis: 2,418 CPU write operations were all retained and presented. Shuriken aiming has no CPU framebuffer writes around 140–169 seconds (322 updates), then resumes after respawn. This narrows the sustained disappearance upstream of retained-mask composition but does not yet distinguish native drawing from write detection. Private evidence: `D:/JFG-private/high-refresh-20261011/shuriken-followup-20261016/coverage-report.json`. New trace observations record original reticle entry points and software line coordinates; no forced reticle is enabled.


The real-weapon replay confirms that frontDrawTarget and the deferred software-line writer continue producing on-screen pixels during the gap. VI updateScreen can consume these edits into present.fbOperations and update RAMHash before submit-time checkRDRAM sees them. The previous cache captured only submit-time operations. The first combined-path trial mistakenly compared visible height 240 with RT64's 32-pixel-aligned texture allocation height 256, rejecting every VI mask. This failed trial is retained as owner-present-overlay; it does not qualify a fix. The corrected candidate checks the logical VI dimensions and draws only that extent, uses each Present's pool ownership through its last GPU fence, and lets a fresh mask supersede a cached mask to avoid stale cursor strokes.


Further runtime evidence identifies skipped Present events that own CPU writes: the queue deliberately drops stale VIs while newer interpolated scenes continue. Applying writes only inside threadPresent misses those events. The new candidate retains the VI mask at import, before enqueueing. Its first trial used a thread-local submission timestamp; graphics submission and frontend VI import run on different threads, so every VI mask received tick zero and expired. The retained-vi-age trace proves this failure. The corrected timestamp is atomic across those threads; the existing framebuffer/age limits remain strict. The first short-route screenshot happened to show a reticle even in the control, so it is not proof of a visual A/B fix. Qualification stays pending.


## Recurrence cause and verified correction

The missing CPU write records in the owner trace covered only submit-time imports, not every CPU write. The original software-line writer still produced reticle pixels. Some edits entered RT64 at VI time; a stale VI event could then be skipped before threadPresent, discarding its only presentation-side mask. The first reticle cache did not retain that path.

Both import paths now retain immutable pixel/boolean textures before a Present event can be discarded. The producer and renderer share an atomic submission timestamp; framebuffer address and two-VI age checks still bound reuse. Fresh VI masks supersede older cached masks. The original line drawing, weapon visibility rules and game state are unchanged. Non-SkipBuffering modes and other screen dimensions keep their prior behavior.

The longer real-weapon replay retains and presents all 796 VI-time masks; all 423 observed shuriken-aim samples have overlay coverage within the recorded two-before/four-after VI correlation window. This checks long disappearances, not sub-frame visual perfection. The original-drawer-only fixture is no longer the sole acceptance evidence. Real shuriken tests cover 120 FPS with VSync, the final 360 FPS phase, and native Aim release. All sampled masks stay within the existing two-VI age limit; the bounded mode tests and normal Play health check report zero audio underruns/overruns. Four focused suites pass, including 363 PC input checks. Source and precise limits are pinned in [qualification](../../../evidence/high-refresh-shuriken-followup.json).

The import-time retainer's initial thread-local tick failure and the padded-texture trial are retained as informative failures. Lesson: protect CPU effects at the point of import, before a skippable queue; preserve source time across real worker threads, and distinguish texture allocation dimensions from visible framebuffer dimensions. Sustained owner play, all weapon transitions, Unlimited, full-route performance and physical display acceptance remain open.


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
