"""Summarize optional stage-thread-cost gameplay TSV records.

Scopes are emitted after returning. Parent targets and host thread IDs rebuild
selected scope trees, so child CPU cycles are subtracted exactly once. CPU
percentages describe only completed game-update scopes on their owning thread.
Other threads and incomplete scopes are reported separately. No cycle-to-time
conversion or physical-display FPS claim is made.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
from typing import Iterable

ROOT = 0x80044FAC
NAMES = {ROOT: 'game_update', 0x80008FF0: 'object_updates',
         0x00310600: 'drone_behavior', 0x09B02FD0: 'pathfinding_parent',
         0x8003D21C: 'animation_matrices', 0x800121C0: 'track_draw',
         0x8004DA10: 'rcpFast3d', 0x8004DCB0: 'rcpWaitDP',
         0x80001EF8: 'audio_frame'}


def analyze(lines: Iterable[str], *, first_vi: int = 0,
            last_vi: int | None = None) -> dict:
    if first_vi < 0 or (last_vi is not None and last_vi <= first_vi):
        raise ValueError('expected a nonnegative, nonempty VI window')
    pending = defaultdict(list)
    totals = defaultdict(Counter)
    roots = cycles = invalid_roots = partial_lines = 0
    outside = Counter()

    def flatten(node):
        yield node
        for child in node['children']:
            yield from flatten(child)

    def selected(vi):
        return vi >= first_vi and (last_vi is None or vi < last_vi)

    for number, line in enumerate(lines, 1):
        fields = line.rstrip('\r\n').split('\t')
        if len(fields) < 2 or fields[1] != 'stage-thread-cost':
            continue
        if not line.endswith('\n'):
            partial_lines += 1
            continue
        try:
            if len(fields) != 12:
                raise ValueError('expected timestamp, event and ten values')
            timestamp = int(fields[0])
            v = [int(x, 16) for x in fields[2:]]
            if timestamp < 0 or any(x < 0 for x in v) or v[9] not in (0, 1):
                raise ValueError('invalid timestamp/counter/status')
        except ValueError as error:
            raise ValueError(f'invalid stage record on line {number}: {error}') from error
        vi, _, target, thread, wall, cpu, parent = v[:7]
        stack = pending[thread]
        children = []
        while stack and stack[-1]['parent'] == target:
            children.append(stack.pop())
        node = dict(target=target, parent=parent, cycles=cpu, wall=wall,
                    valid=bool(v[9]), children=children, vi=vi)
        if target != ROOT:
            if parent == 0:
                if selected(vi):
                    for item in flatten(node):
                        outside[NAMES.get(item['target'], hex(item['target']))] += 1
            else:
                stack.append(node)
            continue
        if not selected(vi):
            continue
        nodes = list(flatten(node))
        # Lost/invalid child measurements must not create a precise pie chart.
        if not all(item['valid'] for item in nodes):
            invalid_roots += 1
            continue
        for item in nodes:
            child_cycles = sum(child['cycles'] for child in item['children'])
            if child_cycles > item['cycles']:
                raise ValueError('child CPU cycles exceed their parent; trace is not additive')
        roots += 1
        cycles += cpu
        for item in nodes:
            row = totals[item['target']]
            row['calls'] += 1
            row['inclusive_cycles'] += item['cycles']
            row['exclusive_cycles'] += item['cycles'] - sum(c['cycles'] for c in item['children'])
            row['inclusive_wall_ns'] += item['wall']
    rows = []
    for target, values in totals.items():
        rows.append(dict(target=f'0x{target:08x}', name=NAMES.get(target, hex(target)),
                         **values,
                         exclusive_cpu_percent=100 * values['exclusive_cycles'] / cycles if cycles else None,
                         inclusive_cpu_percent=100 * values['inclusive_cycles'] / cycles if cycles else None,
                         inclusive_wall_ms_per_update=values['inclusive_wall_ns'] / 1e6 / roots if roots else None))
    rows.sort(key=lambda x: x['exclusive_cycles'], reverse=True)
    return dict(schema=1, completed_updates=roots, measured_thread_cycles=cycles,
                invalid_update_scopes=invalid_roots, partial_stage_lines=partial_lines,
                unfinished_stage_scopes=sum(len(list(flatten(n))) for stack in pending.values() for n in stack),
                outside_game_update_calls=dict(outside), stages=rows,
                limits='Exclusive CPU shares sum within completed game updates on their owning thread. Inclusive shares overlap. Waiting and other threads are not CPU shares. Wall intervals include waits and preemption. Instrumentation changes cost; confirm FPS separately.')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('--first-vi', type=int, default=0)
    parser.add_argument('--last-vi', type=int)
    args = parser.parse_args()
    with args.trace.open(encoding='utf-8') as stream:
        result = analyze(stream, first_vi=args.first_vi, last_vi=args.last_vi)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
