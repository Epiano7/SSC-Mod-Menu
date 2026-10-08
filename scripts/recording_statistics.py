"""Read-only, offline statistics for recorder JSONL files. No upload or identity inference."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path


def number(value):
    return type(value) in (int, float) and -2**53 < value < 2**53 and math.isfinite(value)


def observed(tracking, key):
    availability = tracking.get('availability', {})
    metrics = tracking.get('metrics', {})
    if not isinstance(availability, dict) or not isinstance(metrics, dict):
        return None
    value = metrics.get(key)
    if availability.get(key) != 'observed':
        return None
    if key == 'className':
        return value if isinstance(value, str) and 0 < len(value) <= 64 else None
    return value if number(value) and value >= 0 else None


def summarize(folder, class_name=None):
    seen, rounds, warnings = set(), [], []
    total_bytes = 0
    for path in sorted(Path(folder).glob('*.jsonl')):
        total_bytes += path.stat().st_size
        if total_bytes > 100 * 1024 * 1024:
            raise ValueError('Input exceeds 100 MiB; select a smaller batch')
        raw = path.read_bytes()
        digest = hashlib.sha256(raw).hexdigest()
        if digest in seen:
            continue
        seen.add(digest)
        groups = {}
        sampling_interval = 10000  # Legacy unchanged-state heartbeat interval.
        for line in raw.splitlines():
            if len(line) > 1024 * 1024:
                raise ValueError('Recording row exceeds 1 MiB')
            try:
                row = json.loads(line)
                if not isinstance(row, dict):
                    raise ValueError('row must be an object')
            except (ValueError, UnicodeError):
                warnings.append('malformed_row')
                continue
            if row.get('event') == 'recording_started':
                interval = row.get('sampling_interval_ms')
                if type(interval) is int and 100 <= interval <= 10000:
                    sampling_interval = interval
                continue
            if row.get('event') not in ('observed_state', 'heartbeat', 'round_ended', 'recording_stopped'):
                continue
            tracking = row.get('tracking')
            elapsed = row.get('elapsed_ms')
            if not isinstance(tracking, dict) or not number(elapsed) or elapsed < 0:
                continue
            round_id = tracking.get('round')
            if type(round_id) is not int or round_id < 0:
                continue
            # Legacy files lack reliable match identity. Never combine separate files.
            dropped = row.get('dropped_events')
            groups.setdefault(round_id, []).append((elapsed, tracking,
                dropped if type(dropped) is int and dropped >= 0 else None))
        for round_id, samples in groups.items():
            rollback = any(samples[i][0] < samples[i-1][0] for i in range(1, len(samples)))
            if rollback:
                warnings.append('elapsed_clock_rollback')
            samples.sort(key=lambda sample: sample[0])
            # A final snapshot at the same timestamp supersedes an earlier one.
            samples = list({t: (t, s, d) for t, s, d in samples}.values())
            ends = [(t, s, d) for t, s, d in samples if s.get('round_end_observed') is True]
            end_time, final, _ = ends[-1] if ends else samples[-1]
            cls = observed(final, 'className')
            if class_name is not None and cls != class_name:
                continue
            timeline = []
            last_time = None
            has_gaps = False
            for elapsed, sample, _ in samples:
                if elapsed > end_time:
                    continue
                gap = elapsed - last_time if last_time is not None else 0
                gap_before = gap > max(3000, sampling_interval * 1.5)
                has_gaps |= gap_before
                last_time = elapsed
                timeline.append({'elapsed_ms': elapsed, 'sample_gap_ms': gap,
                                 'gap_before': gap_before, **{
                    key: observed(sample, key) for key in ('level', 'xp', 'money')}})
            if has_gaps:
                warnings.append('sampling_gap')
            dropped_values = [d for t, _, d in samples if t <= end_time and d is not None]
            dropped_reported = max(dropped_values) if dropped_values else None
            if dropped_reported:
                warnings.append('dropped_events_reported')
            complete = bool(ends) and final.get('full_round_timing') is True and not rollback
            rounds.append({
                'recording_sha256': digest, 'round': round_id, 'class': cls,
                'round_end_observed': bool(ends), 'full_round_timing': complete,
                'sampling_gaps': has_gaps, 'elapsed_clock_rollback': rollback,
                'dropped_events_reported': dropped_reported,
                'end_level': observed(final, 'level') if ends else None,
                'end_cash': observed(final, 'money') if ends else None,
                'shots_fired': observed(final, 'shotsFired') if complete else None,
                'projectiles_fired': observed(final, 'projectilesFired') if complete else None,
                'timeline': timeline,
            })
    complete = [r for r in rounds if r['full_round_timing']]
    levels = [r['end_level'] for r in complete if number(r['end_level'])]
    classes = Counter(r['class'] for r in complete if isinstance(r['class'], str))
    return {'schema': 1, 'coverage': 'observed_rounds_only', 'rounds': rounds,
            'summary': {'recordings': len(seen), 'observed_rounds': len(rounds),
                        'complete_rounds': len(complete), 'end_level_samples': len(levels),
                        'average_end_level': sum(levels) / len(levels) if levels else None,
                        'class_round_counts': dict(sorted(classes.items())),
                        'damage_dealt': None, 'damage_taken': None, 'accuracy': None},
            'warnings': dict(Counter(warnings)),
            'limitations': ['Client observations are not authoritative gameplay evidence.',
                           'File hashes deduplicate identical files; they do not prove honesty.',
                           'Legacy recordings cannot establish match identity or reliable calendar filters.',
                           'Cash is a balance, not income. Missing XP is not zero.',
                           'Round start/end coverage does not establish continuous sampling; preserve gaps in charts.',
                           'Damage, healing and accuracy remain excluded pending source validation.']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--class-name')
    args = parser.parse_args()
    result = summarize(args.folder, args.class_name)
    with args.output.open('x', encoding='utf-8') as stream:
        json.dump(result, stream, allow_nan=False, indent=2)
    print(json.dumps(result['summary']))
