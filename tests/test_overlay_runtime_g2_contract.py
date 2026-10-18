from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from scripts.overlay_runtime_g2_models import (
    OverlayLifecycleError,
    OverlayLifecycleHarness,
    OverlayModuleSpec,
    TRAP_KINDS,
    TrapClassificationError,
    TrapClassificationLedger,
    TrapSiteClassification,
)
from scripts.runtime_spike_models import TrapInventoryError
from scripts.check_repository_hygiene import scan_blob
from scripts.validate_overlay_runtime_g2_contract import (
    ContractLoadError,
    canonical_hash,
    load_json,
    validate_contract,
)


ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "config" / "overlay-runtime-g2-contract.json"
SCHEMA = ROOT / "schemas" / "overlay-runtime-g2-contract.schema.json"


def digest(label: str) -> str:
    return hashlib.sha256(label.encode("ascii")).hexdigest()


def overlay_specs() -> tuple[OverlayModuleSpec, OverlayModuleSpec]:
    first = OverlayModuleSpec(
        overlay_id="ovl-alpha",
        text_size=0x100,
        data_size=0x20,
        bss_size=0x40,
        relocation_counts={
            "full-word": 1,
            "jump-target": 1,
            "hi16": 1,
            "lo16": 1,
        },
        exports={"entry": 0x20},
        external_references={},
        callback_kind="initialize",
        semantic_seed_sha256=digest("alpha-state"),
    )
    second = OverlayModuleSpec(
        overlay_id="ovl-beta",
        text_size=0x80,
        data_size=0x20,
        bss_size=0x20,
        relocation_counts={
            "full-word": 1,
            "jump-target": 0,
            "hi16": 0,
            "lo16": 0,
        },
        exports={"entry": 0x10},
        external_references={"alpha-call": ("ovl-alpha", "entry")},
        callback_kind="resume",
        semantic_seed_sha256=digest("beta-state"),
    )
    return first, second


def run_closed_lifecycle() -> tuple[OverlayLifecycleHarness, list[tuple[str, int]]]:
    first, second = overlay_specs()
    callbacks: list[tuple[str, int]] = []
    harness = OverlayLifecycleHarness()
    harness.load(first, 0x10000000, operation_token=1, callback=lambda i, g: callbacks.append((i, g)))
    stale = harness.lookup_export("ovl-alpha", "entry", operation_token=2)
    harness.load(second, 0x10001000, operation_token=3, callback=lambda i, g: callbacks.append((i, g)))
    harness.resolve_external("ovl-beta", "alpha-call", operation_token=4)
    first_digest = harness.unload("ovl-alpha", operation_token=5)
    with unittest.TestCase().assertRaisesRegex(OverlayLifecycleError, "unresolved"):
        harness.resolve_external("ovl-beta", "alpha-call", operation_token=6)
    reloaded = harness.reload(
        first,
        0x10000000,
        stale,
        operation_token=6,
        callback=lambda i, g: callbacks.append((i, g)),
    )
    if reloaded.semantic_state_sha256 != first_digest:
        raise AssertionError("semantic reload digest changed")
    harness.resolve_external("ovl-beta", "alpha-call", operation_token=7)
    harness.unload("ovl-beta", operation_token=8)
    harness.unload("ovl-alpha", operation_token=9)
    harness.require_closed_synthetic_scenario()
    return harness, callbacks


class OverlayLifecycleHarnessTests(unittest.TestCase):
    def test_closed_overlay_lifecycle_and_reload(self) -> None:
        harness, callbacks = run_closed_lifecycle()
        summary = harness.coverage_summary()
        self.assertEqual(summary["active_overlay_count"], 0)
        self.assertTrue(summary["synthetic_contract_only"])
        self.assertFalse(summary["phase5_scheduler_provided"])
        self.assertFalse(summary["g2_complete"])
        self.assertEqual([item[0] for item in callbacks], ["ovl-alpha", "ovl-beta", "ovl-alpha"])
        self.assertLess(callbacks[0][1], callbacks[-1][1])
        self.assertEqual(
            {event.sequence for event in harness.events},
            set(range(1, len(harness.events) + 1)),
        )

    def test_lifecycle_journal_replays_identically(self) -> None:
        first, _ = run_closed_lifecycle()
        second, _ = run_closed_lifecycle()
        self.assertEqual(first.events, second.events)
        self.assertEqual(first.journal_digest(), second.journal_digest())

    def test_scheduler_boundary_rejects_gap_without_mutating_registry(self) -> None:
        first, _ = overlay_specs()
        harness = OverlayLifecycleHarness()
        with self.assertRaisesRegex(OverlayLifecycleError, "out of order"):
            harness.load(first, 0x10000000, operation_token=2, callback=lambda *_: None)
        self.assertEqual(harness.events, ())
        harness.load(first, 0x10000000, operation_token=1, callback=lambda *_: None)

    def test_callback_failure_rolls_back_published_module(self) -> None:
        first, _ = overlay_specs()
        harness = OverlayLifecycleHarness()

        def fail(*_: object) -> None:
            raise RuntimeError("host detail must not escape")

        with self.assertRaisesRegex(OverlayLifecycleError, "rolled back") as failure:
            harness.load(first, 0x10000000, operation_token=1, callback=fail)
        self.assertNotIn("host detail", str(failure.exception))
        self.assertIsNone(failure.exception.__cause__)
        self.assertIsNone(failure.exception.__context__)
        self.assertEqual(harness.coverage_summary()["active_overlay_count"], 0)
        loaded = harness.load(first, 0x10000000, operation_token=2, callback=lambda *_: None)
        self.assertEqual(loaded.active.generation, 2)

    def test_reload_semantic_mismatch_fails_before_publication(self) -> None:
        first, _ = overlay_specs()
        harness = OverlayLifecycleHarness()
        harness.load(first, 0x10000000, operation_token=1, callback=lambda *_: None)
        stale = harness.lookup_export("ovl-alpha", "entry", operation_token=2)
        harness.unload("ovl-alpha", operation_token=3)
        with self.assertRaisesRegex(OverlayLifecycleError, "must use reload"):
            harness.load(first, 0x10000000, operation_token=4, callback=lambda *_: None)
        wrong_lifetime = type(stale)(
            stale.overlay_id,
            stale.generation,
            first.text_size,
        )
        with self.assertRaisesRegex(OverlayLifecycleError, "prior lifetime"):
            harness.reload(
                first,
                0x10000000,
                wrong_lifetime,
                operation_token=4,
                callback=lambda *_: None,
            )
        changed = OverlayModuleSpec(
            overlay_id=first.overlay_id,
            text_size=first.text_size,
            data_size=first.data_size,
            bss_size=first.bss_size,
            relocation_counts=first.relocation_counts,
            exports=first.exports,
            external_references=first.external_references,
            callback_kind=first.callback_kind,
            semantic_seed_sha256=digest("different-state"),
        )
        with self.assertRaisesRegex(OverlayLifecycleError, "differs"):
            harness.reload(
                changed,
                0x10000000,
                stale,
                operation_token=4,
                callback=lambda *_: None,
            )
        harness.reload(
            first,
            0x10000000,
            stale,
            operation_token=4,
            callback=lambda *_: None,
        )

    def test_reload_stale_pointer_contract_failure_rolls_back_publication(self) -> None:
        first, second = overlay_specs()
        harness = OverlayLifecycleHarness()
        harness.load(first, 0x10000000, operation_token=1, callback=lambda *_: None)
        stale = harness.lookup_export("ovl-alpha", "entry", operation_token=2)
        harness.load(second, 0x10001000, operation_token=3, callback=lambda *_: None)
        harness.unload("ovl-alpha", operation_token=4)

        with mock.patch.object(harness._registry, "resolve_pointer", return_value=0):
            with self.assertRaisesRegex(OverlayLifecycleError, "stale native pointer"):
                harness.reload(
                    first,
                    0x10000000,
                    stale,
                    operation_token=5,
                    callback=lambda *_: None,
                )
        self.assertEqual(harness.coverage_summary()["active_overlay_count"], 1)
        with self.assertRaisesRegex(OverlayLifecycleError, "unresolved"):
            harness.resolve_external("ovl-beta", "alpha-call", operation_token=6)

        harness.reload(
            first,
            0x10000000,
            stale,
            operation_token=6,
            callback=lambda *_: None,
        )

    def test_callback_reentry_fails_and_rolls_back_without_consuming_a_token(self) -> None:
        first, _ = overlay_specs()
        harness = OverlayLifecycleHarness()

        def reenter(overlay_id: str, _: int) -> None:
            harness.unload(overlay_id, operation_token=2)

        with self.assertRaisesRegex(OverlayLifecycleError, "rolled back"):
            harness.load(first, 0x10000000, operation_token=1, callback=reenter)
        self.assertEqual(harness.coverage_summary()["active_overlay_count"], 0)
        harness.load(first, 0x10000000, operation_token=2, callback=lambda *_: None)

    def test_full_mapped_extent_is_bounded_and_nonoverlapping(self) -> None:
        first, second = overlay_specs()
        harness = OverlayLifecycleHarness()
        with self.assertRaisesRegex(OverlayLifecycleError, "mapped extent"):
            harness.load(first, 0xFFFFFF00, operation_token=1, callback=lambda *_: None)
        with self.assertRaisesRegex(OverlayLifecycleError, "mapped extent"):
            harness.load(first, 0x10000001, operation_token=1, callback=lambda *_: None)
        harness.load(first, 0x10000000, operation_token=1, callback=lambda *_: None)
        with self.assertRaisesRegex(OverlayLifecycleError, "mapped extents overlap"):
            harness.load(second, 0x10000120, operation_token=2, callback=lambda *_: None)
        harness.load(second, 0x10001000, operation_token=2, callback=lambda *_: None)

    def test_specs_reject_ambiguous_or_unbounded_inputs(self) -> None:
        with self.assertRaisesRegex(ValueError, "exact supported"):
            OverlayModuleSpec(
                "ovl-a",
                0x100,
                0,
                0,
                {"full-word": 1},
                {"entry": 0},
                {},
                "none",
                digest("seed"),
            )
        with self.assertRaisesRegex(ValueError, "word-aligned"):
            OverlayModuleSpec(
                "ovl-a",
                0x102,
                0,
                0,
                {key: 0 for key in ("full-word", "jump-target", "hi16", "lo16")},
                {"entry": 0},
                {},
                "none",
                digest("seed"),
            )
        with self.assertRaisesRegex(ValueError, "self references"):
            OverlayModuleSpec(
                "ovl-a",
                0x100,
                0,
                0,
                {key: 0 for key in ("full-word", "jump-target", "hi16", "lo16")},
                {"entry": 0},
                {"self-call": ("ovl-a", "entry")},
                "none",
                digest("seed"),
            )


def classified_site(
    site_id: str,
    kind: str,
    *,
    reachability: str = "reachable",
    disposition: str = "emulate",
    sources: tuple[str, ...] = ("synthetic",),
) -> TrapSiteClassification:
    return TrapSiteClassification(
        site_id=site_id,
        kind=kind,
        reachability=reachability,
        disposition=disposition,
        owner_id="runtime-owner",
        estimate_class="medium",
        behavior_evidence_sha256=digest(site_id),
        trace_sources=sources,
    )


def trap_counts(overrides: dict[str, int]) -> dict[str, int]:
    counts = {kind: 0 for kind in TRAP_KINDS}
    counts.update(overrides)
    return counts


class TrapClassificationLedgerTests(unittest.TestCase):
    def ledger(self) -> TrapClassificationLedger:
        ledger = TrapClassificationLedger(
            trap_counts({
                "cpu-break": 1,
                "cpu-syscall": 1,
                "switch-bounds": 1,
                "boot-self-check": 1,
            })
        )
        ledger.add(classified_site("break-site", "cpu-break", disposition="abort"))
        ledger.add(
            classified_site(
                "syscall-site",
                "cpu-syscall",
                disposition="defer-to-reviewed-handler",
            )
        )
        ledger.add(
            classified_site(
                "bounds-site",
                "switch-bounds",
                reachability="unreachable",
                disposition="not-applicable",
            )
        )
        ledger.add(classified_site("self-check-site", "boot-self-check"))
        return ledger

    def test_trap_classification_denominator_and_ownership(self) -> None:
        ledger = self.ledger()
        summary = ledger.summary()
        self.assertEqual(summary["candidate_count"], 4)
        self.assertEqual(summary["classified_count"], 4)
        self.assertEqual(summary["reachability_counts"]["unclassified"], 0)
        self.assertFalse(summary["required_private_trace_classes_declared"])
        self.assertFalse(summary["synthetic_contract_is_g2_proof"])
        inventory = ledger.runtime_inventory()
        self.assertEqual(inventory.dispatch("self-check-site"), "emulate")
        with self.assertRaisesRegex(TrapInventoryError, "reviewed handler"):
            inventory.dispatch("syscall-site")

    def test_required_private_trace_classes_include_oracle_native_and_review(self) -> None:
        ledger = TrapClassificationLedger(
            trap_counts({
                "cpu-break": 1,
                "boot-self-check": 1,
            })
        )
        ledger.add(
            classified_site(
                "break-site",
                "cpu-break",
                sources=("private-native", "private-oracle"),
            )
        )
        ledger.add(
            classified_site(
                "self-check-site",
                "boot-self-check",
                reachability="unreachable",
                disposition="not-applicable",
                sources=("private-static-reviewed",),
            )
        )
        self.assertTrue(
            ledger.summary()["required_private_trace_classes_declared"]
        )
        self.assertFalse(ledger.summary()["synthetic_contract_is_g2_proof"])

    def test_unknown_duplicate_and_incomplete_classifications_fail_closed(self) -> None:
        with self.assertRaisesRegex(ValueError, "bounded"):
            TrapClassificationLedger(trap_counts({"cpu-break": 0x1_0000_0000}))

        empty = TrapClassificationLedger(trap_counts({}))
        self.assertFalse(
            empty.summary()["required_private_trace_classes_declared"]
        )

        ledger = TrapClassificationLedger(
            trap_counts({
                "cpu-break": 1,
            })
        )
        site = classified_site("break-site", "cpu-break")
        ledger.add(site)
        with self.assertRaisesRegex(TrapClassificationError, "duplicate"):
            ledger.add(site)

        incomplete = TrapClassificationLedger(
            trap_counts({
                "cpu-break": 1,
            })
        )
        with self.assertRaisesRegex(TrapClassificationError, "denominator"):
            incomplete.summary()

        unclassified = TrapClassificationLedger(
            trap_counts({
                "cpu-break": 1,
            })
        )
        unclassified.add(TrapSiteClassification("break-site", "cpu-break", "unclassified"))
        with self.assertRaisesRegex(TrapClassificationError, "unclassified"):
            unclassified.summary()

    def test_classified_site_requires_owner_estimate_and_non_stub_disposition(self) -> None:
        with self.assertRaisesRegex(ValueError, "owner"):
            TrapSiteClassification(
                "break-site",
                "cpu-break",
                "reachable",
                "abort",
                None,
                "small",
                digest("break"),
                ("synthetic",),
            )
        with self.assertRaisesRegex(ValueError, "non-stub"):
            classified_site(
                "break-site",
                "cpu-break",
                disposition="not-applicable",
            )

    def test_control_integrity_categories_require_a_separate_runtime_bridge(self) -> None:
        ledger = TrapClassificationLedger(
            trap_counts(
                {
                    "dangling-jump-workaround": 1,
                    "checksum": 1,
                    "anti-tamper": 1,
                }
            )
        )
        ledger.add(classified_site("jump-site", "dangling-jump-workaround"))
        ledger.add(classified_site("checksum-site", "checksum"))
        ledger.add(classified_site("tamper-site", "anti-tamper"))
        self.assertEqual(ledger.summary()["candidate_count"], 3)
        with self.assertRaisesRegex(TrapClassificationError, "runtime-dispatch bridge"):
            ledger.runtime_inventory()


class OverlayRuntimeContractValidatorTests(unittest.TestCase):
    @staticmethod
    def contract() -> dict[str, object]:
        return json.loads(CONTRACT.read_text(encoding="utf-8"))

    @staticmethod
    def schema() -> dict[str, object]:
        return json.loads(SCHEMA.read_text(encoding="utf-8"))

    def test_reviewed_contract_and_schema_validate(self) -> None:
        self.assertEqual(validate_contract(self.contract(), self.schema()), [])

    def test_real_progress_and_g2_overclaims_are_rejected(self) -> None:
        mutations = []
        real_overlay = self.contract()
        real_overlay["overlay_lifecycle"]["real_overlay_execution_count"] = 1  # type: ignore[index]
        mutations.append(real_overlay)
        integrated = self.contract()
        integrated["overlay_lifecycle"]["generated_lookup_integration_complete"] = True  # type: ignore[index]
        mutations.append(integrated)
        real_trace = self.contract()
        real_trace["runtime_traps"]["real_trace_count"] = 1  # type: ignore[index]
        mutations.append(real_trace)
        completed = self.contract()
        completed["claims"]["g2_complete"] = True  # type: ignore[index]
        mutations.append(completed)
        authorized = self.contract()
        authorized["claims"]["phase4_completion_authorized"] = True  # type: ignore[index]
        mutations.append(authorized)
        decision = self.contract()
        decision["g2"]["decision"] = "go"  # type: ignore[index]
        mutations.append(decision)
        for mutation in mutations:
            with self.subTest(mutation=canonical_hash(mutation)):
                self.assertTrue(validate_contract(mutation, self.schema()))

    def test_required_events_traps_and_blockers_are_exact_and_ordered(self) -> None:
        cases = []
        events = self.contract()
        events["overlay_lifecycle"]["required_events"] = list(reversed(events["overlay_lifecycle"]["required_events"]))  # type: ignore[index]
        cases.append(events)
        traps = self.contract()
        traps["runtime_traps"]["required_kinds"] = traps["runtime_traps"]["required_kinds"][:-1]  # type: ignore[index]
        cases.append(traps)
        blockers = self.contract()
        blockers["g2"]["blockers"] = blockers["g2"]["blockers"][:-1]  # type: ignore[index]
        cases.append(blockers)
        scheduler = self.contract()
        scheduler["scheduler_boundary"]["phase5_scheduler_claimed"] = True  # type: ignore[index]
        cases.append(scheduler)
        for mutation in cases:
            self.assertTrue(validate_contract(mutation, self.schema()))

    def test_schema_substitution_and_lock_drift_are_rejected(self) -> None:
        permissive = copy.deepcopy(self.schema())
        permissive["properties"]["claims"] = {}  # type: ignore[index]
        errors = validate_contract(self.contract(), permissive)
        self.assertIn("supplied schema differs from the repository contract", errors)
        drifted = self.contract()
        drifted["overlay_lifecycle"]["model"] = "alternate-model"  # type: ignore[index]
        self.assertIn(
            "overlay/runtime contract differs from the reviewed lock",
            validate_contract(drifted, self.schema()),
        )

    def test_private_fields_and_hostile_values_do_not_echo(self) -> None:
        marker = "HOSTILE_PRIVATE_MARKER"
        invalid = self.contract()
        invalid["private_body"] = marker
        invalid["overlay_lifecycle"]["model"] = marker  # type: ignore[index]
        errors = validate_contract(invalid, self.schema())
        self.assertTrue(errors)
        rendered = "\n".join(errors)
        self.assertNotIn(marker, rendered)
        self.assertTrue(any("private" in error for error in errors))

    def test_changed_public_files_pass_repository_blob_hygiene(self) -> None:
        relative_paths = (
            "config/overlay-runtime-g2-contract.json",
            "docs/feasibility/README.md",
            "docs/development/architecture.md",
            "schemas/overlay-runtime-g2-contract.schema.json",
            "scripts/overlay_runtime_g2_models.py",
            "scripts/validate_overlay_runtime_g2_contract.py",
            "tests/test_overlay_runtime_g2_contract.py",
        )
        for relative in relative_paths:
            errors = scan_blob((ROOT / relative).read_bytes(), relative)
            self.assertEqual(errors, [], f"repository hygiene failed for {relative}")

    def test_programmatic_cycles_and_non_json_values_fail_closed(self) -> None:
        cyclic: dict[str, object] = {}
        cyclic["self"] = cyclic
        errors = validate_contract(cyclic, self.schema())
        self.assertIn("contract contains a cyclic container", errors)
        invalid = self.contract()
        invalid["schema_version"] = 1.0
        self.assertIn("contract contains a non-JSON value", validate_contract(invalid, self.schema()))

    def test_file_loader_rejects_duplicate_and_noncanonical_numbers(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            duplicate = root / "duplicate.json"
            duplicate.write_text('{"kind":"a","kind":"b"}', encoding="utf-8")
            with self.assertRaisesRegex(ContractLoadError, "duplicate"):
                load_json(duplicate)
            floating = root / "floating.json"
            floating.write_text('{"schema_version":1.0}', encoding="utf-8")
            with self.assertRaisesRegex(ContractLoadError, "floating"):
                load_json(floating)
            nonstandard = root / "nan.json"
            nonstandard.write_text('{"schema_version":NaN}', encoding="utf-8")
            with self.assertRaisesRegex(ContractLoadError, "nonstandard"):
                load_json(nonstandard)


if __name__ == "__main__":
    unittest.main()
