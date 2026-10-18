import unittest
from scripts.analyze_update_cycle_trace import analyze


def event(t, kind, *values):
    return f"{t}\t{kind}\t" + "\t".join(f"{v:x}" for v in values) + "\n"


class UpdateCycleTraceTests(unittest.TestCase):
    def test_nested_costs_and_game_cadence_are_separate(self):
        report = analyze([
            event(3000, "update", 10, 3),
            event(9000, "submit", 12, 4, 0, 2000, 0, 0, 50, 1000, 0),
            event(10000, "update-work", 12, 4, 0, 0, 50, 6000, 1000),
            event(10001, "update-thread-cycles", 12, 4, 1, 99, 200, 50),
            event(10002, "update", 12, 4),
        ])
        row = report["slowest_updates"][0]
        self.assertEqual((row["active_wall_ms"], row["submit_ms"], row["full_sync_ms"]), (5, 2, 1))
        self.assertEqual((row["thread_cycles"], row["cycles_per_instruction"], row["vi_delta"]), (200, 4, 2))
        self.assertEqual(report["diagnostics"], {})

    def test_repeated_keys_do_not_mix_cycles(self):
        report = analyze([
            event(1000, "update-work", 12, 4, 0, 0, 50, 500, 0),
            event(1001, "update-thread-cycles", 12, 4, 1, 99, 200, 50),
            event(2000, "update-work", 12, 4, 0, 0, 30, 400, 0),
            event(2001, "update-thread-cycles", 12, 4, 1, 99, 300, 30),
        ])
        self.assertEqual([x["thread_cycles"] for x in report["slowest_updates"]], [200, 300])

    def test_missing_invalid_or_mismatched_cycles_are_not_silently_accepted(self):
        report = analyze([
            event(1000, "update-work", 10, 1, 0, 0, 50, 500, 0),
            event(1001, "update-thread-cycles", 11, 1, 1, 99, 200, 50),
            event(2000, "update-work", 12, 2, 0, 0, 50, 500, 0),
            event(2001, "update-thread-cycles", 12, 2, 0, 99, 0, 50),
            event(3000, "update-work", 14, 3, 0, 0, 50, 500, 0),
            event(3001, "update-thread-cycles", 14, 3, 1, 99, 200, 49),
        ])
        self.assertEqual(report["stats"]["thread_cycles"], {})
        self.assertEqual(report["diagnostics"]["unmatched_cycle_rows"], 1)
        self.assertEqual(report["diagnostics"]["updates_without_cycles"], 1)
        self.assertEqual(report["diagnostics"]["invalid_cycle_observations"], 2)

    def test_boundary_crossing_submit_is_not_charged_whole(self):
        report = analyze([
            event(4500, "submit", 12, 4, 0, 2000, 0, 0, 0, 1000, 0),
            event(10000, "update-work", 12, 4, 0, 0, 50, 6000, 1000),
        ])
        row = report["slowest_updates"][0]
        self.assertEqual(row["boundary_crossing_submissions"], 1)
        self.assertEqual((row["submit_ms"], row["full_sync_ms"]), (0, 0))

    def test_bad_rows_and_invalid_waits_are_reported(self):
        report = analyze(["10\tupdate-work\txyz\n", "11\tupdate-work\t0\n",
                          event(1000, "update-work", 12, 4, 0, 0, 50, 500, 501)])
        self.assertEqual(report["selected_updates"], 0)
        self.assertEqual(report["diagnostics"], {
            "malformed_rows": 1, "short_rows": 1, "invalid_wall_intervals": 1})

    def test_complete_looking_truncated_row_is_excluded(self):
        report = analyze([event(1000, "update-work", 10, 1, 0, 0, 50, 500, 0).rstrip("\n")])
        self.assertEqual(report["selected_updates"], 0)
        self.assertEqual(report["diagnostics"], {"unterminated_rows": 1})

    def test_time_outside_root_update_is_reported_separately(self):
        report = analyze([
            event(10000, "update-work", 12, 4, 0, 0, 50, 6000, 1000),
            event(10002, "update", 12, 4),
            event(60000, "update-work", 15, 5, 0, 0, 100, 33000, 5000),
            event(60002, "update", 15, 5),
        ], first_vi=15)
        row = report["slowest_updates"][0]
        self.assertEqual((row["active_wall_ms"], row["outside_update_ms"], row["update_period_ms"], row["vi_delta"]), (28, 17, 50, 3))

    def test_interval_and_limit(self):
        report = analyze([
            event(1000, "update-work", 10, 1, 0, 0, 50, 500, 0),
            event(2000, "update-work", 12, 2, 0, 0, 50, 500, 0),
            event(3000, "update-work", 14, 3, 0, 0, 50, 500, 0),
        ], first_vi=12, last_vi=14, limit=0)
        self.assertEqual(report["selected_updates"], 1)
        self.assertEqual(report["slowest_updates"], [])
        with self.assertRaises(ValueError):
            analyze([], first_vi=12, last_vi=12)


if __name__ == "__main__":
    unittest.main()
