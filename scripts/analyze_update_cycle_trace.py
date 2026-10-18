"""Summarize update wall, thread-cycle and nested graphics costs without conflating them."""
from __future__ import annotations

import argparse
import bisect
import collections
import json
import statistics
from pathlib import Path
from typing import Iterable


def _stats(values: list[float | int]) -> dict:
    values = sorted(values)
    if not values:
        return {}
    return {
        "n": len(values),
        "median": statistics.median(values),
        "p95": values[int(.95 * (len(values) - 1))],
        "p99": values[int(.99 * (len(values) - 1))],
        "max": values[-1],
    }


def analyze(lines: Iterable[str], *, first_vi: int = 0, last_vi: int | None = None,
            limit: int = 20) -> dict:
    if first_vi < 0 or (last_vi is not None and last_vi <= first_vi) or limit < 0:
        raise ValueError("Invalid VI interval or result limit")
    work = []
    submits = []
    updates = []
    pending = None
    errors: collections.Counter[str] = collections.Counter()
    previous_stamp = -1
    for line in lines:
        if not line.endswith("\n"):
            errors["unterminated_rows"] += 1
            continue
        fields = line.rstrip("\r\n").split("\t")
        if len(fields) < 2 or fields[1] not in {
            "update-work", "update-thread-cycles", "submit", "update"}:
            continue
        try:
            stamp = int(fields[0])
            values = [int(x, 16) for x in fields[2:]]
            if stamp < 0 or any(x < 0 for x in values):
                raise ValueError
        except ValueError:
            errors["malformed_rows"] += 1
            continue
        if stamp < previous_stamp:
            errors["nonmonotonic_timestamps"] += 1
        previous_stamp = stamp
        kind = fields[1]
        required = {"update-work": 7, "update-thread-cycles": 6, "submit": 9, "update": 2}[kind]
        if len(values) < required:
            errors["short_rows"] += 1
            continue
        if kind == "update-work":
            row = {"end_us": stamp, "values": values, "cycles": None}
            work.append(row)
            pending = row
        elif kind == "update-thread-cycles":
            # A cycle row follows its update-work row. Do not use a dictionary
            # keyed only by VI/poll: those values can repeat across a capture.
            if pending is None or pending["cycles"] is not None or \
                    pending["values"][:2] != values[:2] or stamp < pending["end_us"]:
                errors["unmatched_cycle_rows"] += 1
                continue
            pending["cycles"] = values
        elif kind == "submit":
            submits.append((stamp, values))
        else:
            updates.append((stamp, values))
    # Timestamps are used for containment, never to add overlapping nested scopes.
    submits.sort(key=lambda row: row[0])
    submit_stamps = [stamp for stamp, _ in submits]
    updates.sort(key=lambda row: row[0])
    update_stamps = [stamp for stamp, _ in updates]
    rows = []
    previous_work_end = None
    for item in work:
        end = item["end_us"]
        v = item["values"]
        preceding_end = previous_work_end
        previous_work_end = end
        if v[0] < first_vi or (last_vi is not None and v[0] >= last_vi):
            continue
        if v[6] > v[5] or end < v[5]:
            errors["invalid_wall_intervals"] += 1
            continue
        start = end - v[5]
        outside_us = None if preceding_end is None else start - preceding_end
        if outside_us is not None and outside_us < 0:
            errors["overlapping_update_intervals"] += 1
            outside_us = None
        candidates = submits[bisect.bisect_left(submit_stamps, start):
                             bisect.bisect_right(submit_stamps, end)]
        # Exclude submissions crossing the update boundary. Their full-sync
        # position within the partially overlapping interval is unknown.
        enclosed = [(t, x) for t, x in candidates if t - x[3] >= start]
        partial = len(candidates) - len(enclosed)
        row = {
            "vi": v[0], "poll": v[1], "end_us": end,
            "wall_ms": v[5] / 1000, "known_wait_ms": v[6] / 1000,
            "outside_update_ms": outside_us / 1000 if outside_us is not None else None,
            "update_period_ms": (end - preceding_end) / 1000 if preceding_end is not None and end >= preceding_end else None,
            "active_wall_ms": (v[5] - v[6]) / 1000,
            "guest_instructions": v[4],
            "submit_count": len(enclosed),
            "submit_ms": sum(x[3] for _, x in enclosed) / 1000,
            "full_sync_ms": sum(x[7] for _, x in enclosed) / 1000,
            "rdram_check_ms": sum(x[6] for _, x in enclosed) / 1000,
            "boundary_crossing_submissions": partial,
            "thread_cycles": None,
            "cycles_per_instruction": None,
            "vi_delta": None,
        }
        c = item["cycles"]
        if c is None:
            errors["updates_without_cycles"] += 1
        elif c[2] != 1 or c[4] == 0 or c[5] != v[4]:
            errors["invalid_cycle_observations"] += 1
        else:
            row["thread_cycles"] = c[4]
            if v[4]:
                row["cycles_per_instruction"] = c[4] / v[4]
        next_update = bisect.bisect_left(update_stamps, end)
        if next_update < len(updates) and updates[next_update][1][:2] == v[:2] and next_update:
            delta = v[0] - updates[next_update - 1][1][0]
            if delta >= 0:
                row["vi_delta"] = delta
            else:
                errors["decreasing_vi_counter"] += 1
        rows.append(row)
    keys = ("wall_ms", "known_wait_ms", "active_wall_ms", "outside_update_ms",
            "update_period_ms", "guest_instructions",
            "thread_cycles", "cycles_per_instruction", "submit_ms", "full_sync_ms")
    return {
        "schema": 1,
        "selected_updates": len(rows),
        "vi_interval": [first_vi, last_vi],
        "stats": {key: _stats([x[key] for x in rows if x[key] is not None]) for key in keys},
        "vi_deltas": dict(sorted(collections.Counter(x["vi_delta"] for x in rows
                                                    if x["vi_delta"] is not None).items())),
        "diagnostics": dict(errors),
        "slowest_updates": sorted(rows, key=lambda x: x["active_wall_ms"], reverse=True)[:limit],
        "limits": [
            "Active wall time excludes only the runtime's known wait counter; it is not CPU time.",
            "Thread cycles exclude other host threads and must not be converted to milliseconds.",
            "Submission and full-sync durations are nested; do not add them to each other or update wall time.",
            "Only submissions fully enclosed by an update contribute to its cost subtotals.",
            "Outside-update time is the gap between measured root calls; it can contain work and waits.",
            "VI deltas describe game updates, not monitor scanout or interpolated output.",
            "These observations alone do not identify a bottleneck or establish a scheduling cause.",
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--first-vi", type=int, default=0)
    parser.add_argument("--last-vi", type=int)
    parser.add_argument("--limit", type=int, default=20)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    with args.trace.open(encoding="utf-8", errors="replace") as stream:
        result = analyze(stream, first_vi=args.first_vi, last_vi=args.last_vi, limit=args.limit)
    text = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")


if __name__ == "__main__":
    main()
