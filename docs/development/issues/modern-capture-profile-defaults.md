# INPUT-008: Retained capture profile differs from modern defaults

Status: owner-requested unified Keys revision verified in tests and screenshot review; delivered locally; physical owner check pending. Severity: usability. Affected: draw-key preview, runtime source 27ddef0a5ed4afd1f1f9c26a5590b590cda5fe8e and retained route-capture profile.

During the longer owner measurement the owner reports unexpected mouse direction and E not acting as the expected A/Confirm. The attached Scooby-Doo image was accidental; the owner confirms JFG. Turning vertical inversion Off fixes the direction according to the owner.

The initial capture profile uses schema 2, E for Interact, Enter for Confirm, and mouse vertical inversion 1. After the owner's settings edit at approximately 13:38:51 UTC, schema 5 saves inversion 0, preserves Enter Confirm and migrates Backspace Back to Q. Keep this mid-session configuration change with the capture evidence.

Current source defaults use E for both Interact and Confirm in separate contexts. Saved Confirm bindings, including Enter, are deliberately preserved. The separate controls preview was previously aligned to E, but this older capture profile was not. Do not globally migrate explicit Enter choices based on this report. A profile-specific alignment can follow the recording without resetting other choices or saves.

Modern mouse default/reset is explicitly inversion On; existing unit/UI tests enforce it. PC-controls documentation describes On, while the Modern Controls settings table incorrectly said both mouse axes default Off. Correcting that table is documentation only. Changing the new/reset mouse default to Off is a follow-up choice; no new default or runtime fix is claimed here.

Remote develop at review is 63e49a7a71c3a914d2adc1d91fdcf00bef30d38e, already included by merge f70b513c896df107163f2d72abed344a87dc2517. The installed launcher's pinned modern_controls.hpp and modern editor/presentation source hashes match the merged source. Its older pc_input.hpp differs only by the later replay-focus helper. No evidence supports a missing merge of these controls.

Validation: read-only branch/provenance/configuration review and owner's live inversion report. No builds, tests or agent control mutations run alongside the measurement. E menu behavior follows the saved Enter binding; exact coverage of every guest dialog is not claimed. Raw inputs and the initial profile remain preserved.

Private evidence: final-measurement-20261016, session 20261016-133552-e6cd10c21fef4225a62f3f7b22eaadfa.
Compatibility lesson: merging source does not realign all retained preview profiles. Distinguish actual new defaults, saved preferences, and user-requested alignment of a known old profile. See [PC controls](../../features/pc-controls.md#modern-keyboard-confirm-default-2026-10-11).


## Confirmed hidden binding list bug

The owner subsequently added F as Confirm. The backend saved key.menu_confirm=70, but the Keys card did not display F. The owner also reports that Enter was never visible and expected E to advance Magnus dialogue. Source review confirms modernGroups filters keys to gameplay/scoped contexts, omitting menu-only actions. The earlier settings-only explanation was incomplete: retained bindings explain which key works, but the editor fails to show that binding. Preserve the owner-selected F and inversion Off. Current defaults already assign E to both Interact and Confirm in separate contexts; do not overwrite custom profiles. Candidate remedy: show explicit, editable Menus & dialogue rows in both list and diagram views, including unbound actions. Verification pending. Screenshots retained privately with this measurement. UI regression tests must cover Add F -> Confirm -> visible -> reload, clearing shared E without removing Interact, and old Enter profiles.


## Candidate UI remedy

Added a Menus & dialogue card to list and diagram views for keyboard and controller. Every menu action remains visible even when unbound; selecting its row captures a replacement and clearing affects only that action, preserving shared gameplay bindings. Help now explains that Confirm advances speech. No runtime mapping, defaults, owner F binding, inversion or save data changed. Native UI regression coverage added for old Enter, Add F to Confirm, persistence, shared E, unbound/rebind, diagram IDs and controller visibility. Candidate is not yet built or verified.


## Verification

The native offscreen RmlUi suite passes 717 checks, including the owner reproduction (old Enter profile; add F as Confirm; visible after reload), E shared with Interact, clearing and rebinding only Confirm, diagram visibility and unique IDs, controller menu editors and existing DPI/clear-button checks. The captured F layout was visually reviewed. This validates the editor; it does not prove every in-game dialogue branch. No desktop input was injected. Local launcher delivery follows; live owner confirmation is pending. Runtime controls and owner profile remain unchanged. Compatibility lesson: separate input contexts may share keys, but every stored action needs a visible editor and per-action clear semantics.


The local High Refresh Preview launcher was updated after native UI validation and launcher/setup checks. Runtime remains unchanged. Commit 95b084a contains the editor fix. The first combined packaging test could not find source fixtures from the external D: staging directory; running the same compiled C# suite from inside the checkout passed, followed by packaging. All failed output is retained. Live owner confirmation remains pending.


## Owner revision: unified Keys list and shared actions (2026-10-16)

The owner rejects a separate Menus & dialogue card and arrow-key navigation. Keep Mouse and the W/A/S/D movement list at the left; put Confirm, Back, Save and every other added keyboard key in the right-hand Keys list, including new movement/menu bindings. The owner explicitly requests E for both Interact and Confirm. Candidate removes context filtering from input rows, shows every assigned action on a shared key, and lets selecting an already assigned action remove only that assignment. Existing overlapping-context Keep both remains available. Defaults remove arrows; legacy modern profiles retire arrow bindings while preserving non-arrow choices, inversion, controller bindings and saves. Original controller mapping is outside this policy. No live profile has been changed yet. Native UI/core checks and package delivery remain pending.


The first revised checks exposed two obsolete test assumptions: the arrow-only test accidentally retained held W, and the old clear-row test expected hidden menu assignments to survive clearing that physical input. The new list displays all assignments, so clearing an input clears its visible actions; removing one assigned action in its picker preserves the others. Regression expectations now explicitly test both operations and restore the shared controller Confirm before continuing. The failed evidence is retained.


Core controls checks pass (155), and the native offscreen UI suite passes 772 checks with no physical desktop input. Coverage includes new Confirm/menu/movement keys in Keys, shared E persistence and per-action removal, Keep both for overlapping actions, no arrow rows/capture, left lists retained, diagram uniqueness and DPI geometry. The second wrapper accidentally read the first failed UI result while the new executable succeeded; verified-checks.json pins the actual successful ui-checks-02/result.json and source hashes. Both artifacts remain intact. Screenshots and package qualification follow.


Screenshot review confirms separate left Mouse/WASD groups and one right Keys group, including F/Q/F5 and shared E: Interact + Confirm without truncation at the reference size. The reticle renderer source is unchanged by this revision. Current [qualification](../../../evidence/unified-modern-keys.json) pins the controls runtime and bounded checks; delivery follows. Compatibility lesson: list physical inputs across all action contexts, keep shared actions independently editable, and do not hide newly added inputs behind context filters or a second category.


The unified layout is delivered through High Refresh Preview and Jitter Capture with source `19a2373266dc78845abddf7ea468e312e2d4b8eb`. Both shortcut profiles now use the explicitly requested shared E for Interact/Confirm and have no modern arrow bindings. Exact previous profile files and links are backed up privately. All 39 other checked files and every other field in the edited profiles are unchanged, including saves and inversion. Three copied-profile setup checks resolve the new packaged runtime. This is local delivery, not a public release or physical owner gameplay acceptance.

## Published release verification

Available in [v1.2.0](https://github.com/TK22-26/JetForceGemini-Recomp/releases/tag/v1.2.0), built from `f56cd94c4bd7583bceee201eb3f229f744216bc1` after
[PR #16](https://github.com/TK22-26/JetForceGemini-Recomp/pull/16) merged. The
[tagged-release workflow](https://github.com/TK22-26/JetForceGemini-Recomp/actions/runs/38981221560) passed its full checks, verified the prebuilt
Windows bundle and published it. The public download was downloaded again and
its ZIP, per-member hashes, package inventory and clean runtime identity verified.
This release includes the remedy and finite verification described above; it
does not expand the recorded hardware, Linux, campaign or fresh-install coverage.
