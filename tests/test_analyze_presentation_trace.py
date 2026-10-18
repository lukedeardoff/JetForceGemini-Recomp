import math
import unittest
from scripts.analyze_presentation_trace import analyze


def row(tick, rate, alpha, timestamp=0):
    return f"display\t{timestamp}\t{tick}\t{rate}\t120\t0\t4\t{alpha:.6g}\t42\t0\n"


class PresentationTraceTests(unittest.TestCase):
    def test_changing_source_cadence_keeps_uniform_output_phase(self):
        lines = [row(100, 30, 1)]
        lines += [row(103, 20, n / 6, n * 8333) for n in range(1, 7)]
        lines += [row(105, 30, .25, 7 * 8333)]
        report = analyze(lines, expected_fps=120)
        self.assertEqual(report["samples"], 8)
        self.assertEqual(report["uneven_phase_steps"], 0)
        self.assertEqual(report["repeated_phases"], 0)
        self.assertEqual(report["backward_phases"], 0)

    def test_stable_present_counter_does_not_hide_freeze_and_jump(self):
        lines = [row(100, 30, 1, n * 8333) for n in range(4)]
        lines += [row(104, 30, .25, 4 * 8333)]
        report = analyze(lines, expected_fps=120)
        self.assertEqual(report["present_interval_ms"]["maximum"], 8.333)
        self.assertEqual(report["repeated_phases"], 3)
        self.assertEqual(report["uneven_phase_steps"], 4)
        self.assertEqual(report["phase_step_counts"]["2.5000"], 1)

    def test_backwards_filter_and_uninterpolated_records(self):
        report = analyze(["render\tignored\n", row(99, 30, 1), row(102, 30, 1),
                          row(100, 30, 1), row(103, 0, 1), row(104, 30, 1)],
                         first_vi=100, last_vi=104)
        self.assertEqual(report["samples"], 2)
        self.assertEqual(report["backward_phases"], 1)

    def test_completion_distinguishes_legacy_truncated_and_drained_logs(self):
        footer="trace-end\t0\t0\t0\t0\t0\t0\t0\t0\t0\n"
        self.assertFalse(analyze([row(100,30,1)])["complete"])
        self.assertTrue(analyze([row(100,30,1),footer])["complete"])
        self.assertFalse(analyze([footer.replace("\t0\t0\n","\t3\t1\n")])["complete"])
        for lines in ([footer,row(102,30,1)],[footer,footer],["trace-end\t0\n"]):
            with self.assertRaises(ValueError):
                analyze(lines)

    def test_corrupt_render_rate_cannot_hide_behind_zero_display_samples(self):
        corrupt = "render\t1000\t4200\t864323521\t120\t0\t1\t1\t0\t0\n"
        with self.assertRaises(ValueError):
            analyze([corrupt, row(4200, 0, 1)])
        with self.assertRaises(ValueError):
            analyze([row(4200, 864323521, 1)])
        valid = corrupt.replace("864323521", "30")
        report = analyze([valid, row(4200, 0, 1), row(4202, 30, .25)])
        self.assertEqual(report["render_source_rates"], {30: 1})
        self.assertEqual(report["display_source_rates"], {0: 1, 30: 1})

    def test_native_clock_is_explicit_and_preserves_nominal_vi_spans(self):
        hz = 46875000 / 783520
        lines = [row(100, 30, 1)]
        for sample in range(1, 25):
            phase = 100 + sample * hz / 120
            tick = math.ceil(phase / 2) * 2
            alpha = (phase - tick + 2) / 2
            lines.append(row(tick, 30, alpha, round(sample * 1e6 / 120)))
        native = analyze(lines, expected_fps=120, vi_hz=hz)
        nominal = analyze(lines, expected_fps=120)
        self.assertEqual(native["uneven_phase_steps"], 0)
        self.assertEqual(nominal["uneven_phase_steps"], 24)
        self.assertLess(abs(native["elapsed_phase_error_ms"]["end"]), .001)
        self.assertEqual(native["phase_anomaly_count"], 0)

    def test_anomalies_keep_bounded_scene_context_and_selected_rates(self):
        lines = [row(50, 20, 1)] + [row(100, 30, 1, n * 8333) for n in range(40)]
        lines += [row(104, 30, .25, 40 * 8333), row(200, 20, 1)]
        result = analyze(lines, first_vi=100, last_vi=110, expected_fps=120)
        self.assertEqual(result["display_source_rates"], {20: 2, 30: 41})
        self.assertEqual(result["selected_display_source_rates"], {30: 41})
        self.assertEqual(result["phase_anomaly_count"], 40)
        self.assertEqual(len(result["phase_anomalies"]), 32)
        self.assertTrue(result["phase_anomalies_truncated"])
        self.assertEqual(result["phase_anomalies"][0]["current"]["present_id"], 42)
        self.assertEqual(result["phase_anomalies"][0]["current"]["source_vi"], 100)
        self.assertIsNone(analyze([])["elapsed_phase_error_ms"])
        for hz in (0, -1, math.inf, math.nan):
            with self.assertRaises(ValueError):
                analyze([], vi_hz=hz)

    def test_small_clock_error_cannot_hide_a_later_large_jump(self):
        lines = []
        for sample in range(80):
            phase = 100 + sample * .5
            tick = math.ceil(phase / 2) * 2
            lines.append(row(tick, 30, (phase - tick + 2) / 2, sample * 8333))
        lines.append(row(150, 30, 1, 80 * 8333))
        result = analyze(lines, expected_fps=120, vi_hz=46875000 / 783520)
        self.assertTrue(result["phase_anomalies_truncated"])
        self.assertEqual(result["phase_anomalies"][-1]["current"]["source_vi"], 150)
        self.assertGreater(result["phase_anomalies"][-1]["phase_step"], 10)

    def test_invalid_data_is_not_silently_successful(self):
        for line in ["display\t1\n", row(100, 30, math.nan), row(100, -1, .5)]:
            with self.assertRaises(ValueError):
                analyze([line])
        with self.assertRaises(ValueError):
            analyze([], expected_fps=0)
        self.assertEqual(analyze([])["samples"], 0)


if __name__ == "__main__":
    unittest.main()
