import unittest
from scripts.analyze_cpu_stage_trace import analyze, ROOT


def row(target, parent, cycles, thread=7, vi=100, valid=1):
    values = [vi, 20, target, thread, cycles * 10, cycles, parent, vi, 20, valid]
    return '1000\tstage-thread-cost\t' + '\t'.join(f'{v:x}' for v in values) + '\n'


class StageTraceTests(unittest.TestCase):
    def test_nested_scopes_subtract_children_once(self):
        data = [row(2, 1, 30), row(1, ROOT, 60), row(ROOT, 0, 100)]
        report = analyze(data)
        values = {x['target']: x for x in report['stages']}
        self.assertEqual(report['completed_updates'], 1)
        self.assertEqual(values['0x00000001']['exclusive_cpu_percent'], 30)
        self.assertEqual(values['0x00000001']['inclusive_cpu_percent'], 60)
        self.assertEqual(sum(x['exclusive_cpu_percent'] for x in values.values()), 100)

    def test_other_threads_and_unfinished_scopes_excluded(self):
        report = analyze([row(1, ROOT, 60), row(3, 0, 1000, thread=8),
                          row(ROOT, 0, 100), row(2, ROOT, 10)])
        self.assertEqual(report['measured_thread_cycles'], 100)
        self.assertEqual(report['outside_game_update_calls'], {'0x3': 1})
        self.assertEqual(report['unfinished_stage_scopes'], 1)

    def test_recursive_scopes_and_siblings(self):
        report = analyze([row(1, 1, 10), row(1, ROOT, 30), row(1, ROOT, 20), row(ROOT, 0, 100)])
        nested = next(v for v in report['stages'] if v['target'] == '0x00000001')
        self.assertEqual(nested['exclusive_cycles'], 50)
        self.assertEqual(nested['inclusive_cycles'], 60)

    def test_invalid_child_excludes_its_update(self):
        report = analyze([row(1, ROOT, 0, valid=0), row(ROOT, 0, 100)])
        self.assertEqual(report['completed_updates'], 0)
        self.assertEqual(report['invalid_update_scopes'], 1)

    def test_counter_inconsistency_is_not_clamped(self):
        with self.assertRaisesRegex(ValueError, 'exceed'):
            analyze([row(1, ROOT, 101), row(ROOT, 0, 100)])

    def test_window_uses_complete_root_scope(self):
        report = analyze([row(1, ROOT, 50, vi=99), row(ROOT, 0, 100, vi=100),
                          row(ROOT, 0, 200, vi=101)], first_vi=100, last_vi=101)
        self.assertEqual(report['completed_updates'], 1)
        self.assertEqual(report['stages'][0]['exclusive_cpu_percent'], 50)

    def test_truncation_and_bad_records(self):
        self.assertEqual(analyze([row(ROOT, 0, 10).rstrip()])['partial_stage_lines'], 1)
        with self.assertRaisesRegex(ValueError, 'line 1'):
            analyze(['10\tstage-thread-cost\tx\n'])
        self.assertEqual(analyze(['10\tunknown\tx\n'])['completed_updates'], 0)


if __name__ == '__main__':
    unittest.main()
