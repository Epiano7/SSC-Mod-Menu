import json
from pathlib import Path
import tempfile
import unittest
from recording_statistics import summarize


def sample(t, level=7, end=False, full=True, cls='Caster'):
    return {'event': 'observed_state', 'elapsed_ms': t, 'tracking': {
        'round': 1, 'round_end_observed': end, 'full_round_timing': full,
        'metrics': {'level': level, 'className': cls, 'shotsFired': 10, 'money': 100},
        'availability': {k: 'observed' for k in ('level', 'className', 'shotsFired', 'money')}}}


class StatisticsTests(unittest.TestCase):
    def run_rows(self, rows, duplicate=False, **kwargs):
        with tempfile.TemporaryDirectory() as directory:
            raw = '\n'.join(json.dumps(r) for r in rows)
            Path(directory, 'one.jsonl').write_text(raw, encoding='utf-8')
            if duplicate:
                Path(directory, 'two.jsonl').write_text(raw, encoding='utf-8')
            return summarize(directory, **kwargs)

    def test_snapshots_not_summed_and_duplicates_not_counted(self):
        result = self.run_rows([sample(0), sample(1000, end=True), sample(2000, end=True)], True)
        self.assertEqual(result['summary']['complete_rounds'], 1)
        self.assertEqual(result['rounds'][0]['shots_fired'], 10)
        self.assertEqual(result['summary']['average_end_level'], 7)
        self.assertIsNone(result['rounds'][0]['timeline'][0]['xp'])

    def test_partial_excluded_from_average(self):
        for row in (sample(0), sample(0, end=True, full=False)):
            result = self.run_rows([row])
            self.assertIsNone(result['summary']['average_end_level'])
            self.assertIsNone(result['rounds'][0]['shots_fired'])

    def test_missing_or_nonfinite_not_zero(self):
        row = sample(0, level=float('nan'), end=True)
        result = self.run_rows([row])
        self.assertIsNone(result['summary']['average_end_level'])
        self.assertIsNone(result['rounds'][0]['end_level'])

    def test_class_filter(self):
        self.assertEqual(self.run_rows([sample(0, end=True)], class_name='Assault')['rounds'], [])

    def test_malformed_input_and_duplicate_time(self):
        result = self.run_rows([None, sample(0), sample(0), sample(1000, end=True)])
        self.assertEqual(result['warnings'], {'malformed_row': 1})
        self.assertEqual(len(result['rounds'][0]['timeline']), 2)

    def test_timeline_marks_suspend_gap_without_inventing_samples(self):
        result = self.run_rows([{'event': 'recording_started', 'sampling_interval_ms': 1000},
                                sample(0), sample(1000), sample(20000, end=True)])
        timeline = result['rounds'][0]['timeline']
        self.assertEqual(len(timeline), 3)
        self.assertTrue(timeline[-1]['gap_before'])
        self.assertEqual(timeline[-1]['sample_gap_ms'], 19000)
        self.assertEqual(result['warnings'], {'sampling_gap': 1})

    def test_numeric_strings_and_huge_numbers_remain_unavailable(self):
        for value in ('7', 10**400, True, -1):
            result = self.run_rows([sample(0, level=value, end=True)])
            self.assertIsNone(result['summary']['average_end_level'])

    def test_clock_rollback_excluded_from_complete_round_aggregate(self):
        result = self.run_rows([sample(1000), sample(500, end=True)])
        self.assertEqual(result['summary']['complete_rounds'], 0)
        self.assertTrue(result['rounds'][0]['elapsed_clock_rollback'])

    def test_final_same_time_snapshot_and_drop_marker_preserved(self):
        last = sample(0, level=9, end=True)
        last['dropped_events'] = 2
        result = self.run_rows([sample(0), last])
        self.assertEqual(result['summary']['average_end_level'], 9)
        self.assertEqual(result['rounds'][0]['timeline'][0]['level'], 9)
        self.assertEqual(result['rounds'][0]['dropped_events_reported'], 2)


if __name__ == '__main__':
    unittest.main()
