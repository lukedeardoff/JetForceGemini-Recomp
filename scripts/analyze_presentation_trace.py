"""Analyze displayed interpolation progress, independently of the FPS counter.

Input: optional JFG_RT64_INTERPOLATION_TRACE TSV, after a clean process exit.
Phase is expressed in guest VI units. Source cadence may change between samples.
This measures selected interpolation samples, not GPU pixels or monitor scanout.
"""
from __future__ import annotations

import argparse
import heapq
import json
import math
import statistics
from collections import Counter
from pathlib import Path
from typing import Iterable


def analyze(lines: Iterable[str], *, first_vi: int = 0,
            last_vi: int | None = None, expected_fps: float | None = None,
            vi_hz: float = 60.0) -> dict:
    if expected_fps is not None and (not math.isfinite(expected_fps) or expected_fps <= 0):
        raise ValueError("expected FPS must be positive and finite")
    if not math.isfinite(vi_hz) or vi_hz <= 0:
        raise ValueError("VI clock rate must be positive and finite")
    samples = []
    selected_display_rates = Counter()
    selected_records = []
    completion = None
    render_rates = Counter()
    display_rates = Counter()
    for number, line in enumerate(lines, 1):
        fields = line.strip().split("\t")
        if not fields[0]:
            continue
        if completion is not None:
            raise ValueError(f"record after trace completion on line {number}")
        if fields[0] == "trace-end":
            if len(fields) != 10 or fields[1:8] != ["0"] * 7:
                raise ValueError(f"invalid completion record on line {number}")
            try:
                dropped, failed = map(int, fields[8:])
            except ValueError as error:
                raise ValueError(f"invalid completion status on line {number}") from error
            if dropped < 0 or failed not in (0, 1):
                raise ValueError(f"invalid completion status on line {number}")
            completion = {"dropped_chunks": dropped, "writer_failed": bool(failed)}
            continue
        # A stale C++ snapshot layout can corrupt only the render-worker rate
        # while display records fall back to zero. Do not accept that as a
        # phase-correct trace merely because no backwards samples remain.
        if fields[0] == "render" and len(fields) == 10:
            try:
                rate = int(fields[3])
                if not 0 <= rate <= 60:
                    raise ValueError("source rate is outside the supported VI clock")
            except ValueError as error:
                raise ValueError(f"invalid render record on line {number}: {error}") from error
            render_rates[rate] += 1
        if fields[0] != "display":
            continue
        try:
            if len(fields) != 10:
                raise ValueError("expected ten fields")
            timestamp, tick, source, target, frame, count = map(int, fields[1:7])
            alpha = float(fields[7])
            if not math.isfinite(alpha) or not 0 <= alpha <= 1 or not 0 <= source <= 60:
                raise ValueError("invalid source rate or interpolation weight")
        except ValueError as error:
            raise ValueError(f"invalid display record on line {number}: {error}") from error
        display_rates[source] += 1
        if source == 0 or tick < first_vi or (last_vi is not None and tick >= last_vi):
            continue
        # The previous endpoint is one *actual source interval* before this tick.
        # Assuming two VIs misclassifies valid 20-Hz interpolation as jumps.
        phase = tick - (60.0 / source) * (1.0 - alpha)
        samples.append((timestamp, phase))
        selected_display_rates[source] += 1
        selected_records.append({"timestamp_us": timestamp, "source_vi": tick,
                                 "source_rate": source, "alpha": alpha,
                                 "present_id": int(fields[8]), "render_frame": frame,
                                 "sequence_frames": count})
    steps = [b[1] - a[1] for a, b in zip(samples, samples[1:])]
    intervals = [(b[0] - a[0]) / 1000 for a, b in zip(samples, samples[1:])]
    epsilon = 0.0001  # TSV weights have six significant digits.
    result = {
        "samples": len(samples),
        "render_source_rates": dict(render_rates),
        "display_source_rates": dict(display_rates),
        "selected_display_source_rates": dict(selected_display_rates),
        "vi_clock_hz": vi_hz,
        "completion": completion,
        "complete": completion is not None and
                    completion["dropped_chunks"] == 0 and not completion["writer_failed"],
        "repeated_phases": sum(abs(step) <= epsilon for step in steps),
        "backward_phases": sum(step < -epsilon for step in steps),
        "phase_step_counts": dict(Counter(f"{step:.4f}" for step in steps)),
        "present_interval_ms": None,
    }
    if intervals:
        ordered = sorted(intervals)
        result["present_interval_ms"] = {
            "median": statistics.median(intervals),
            "p99": ordered[int(0.99 * (len(ordered) - 1))],
            "maximum": max(intervals),
        }
    if expected_fps is not None:
        expected = vi_hz / expected_fps
        result["expected_phase_step"] = expected
        result["uneven_phase_steps"] = sum(abs(step - expected) > epsilon for step in steps)
    # Keep enough context to locate freezes and jumps without dumping an entire
    # route. Nominal source labels still describe VI spans; only the expected
    # wall-time spacing uses the explicitly supplied VI clock frequency.
    expected = vi_hz / expected_fps if expected_fps is not None else None
    anomalies = []
    anomaly_count = 0
    for index, step in enumerate(steps):
        anomalous = abs(step - expected) > epsilon if expected is not None else step <= epsilon
        if anomalous:
            anomaly_count += 1
            severity = abs(step - expected) if expected is not None else 1.0 - step
            context = {"phase_step": step, "present_interval_ms": intervals[index],
                       "previous": selected_records[index],
                       "current": selected_records[index + 1]}
            item = (severity, -index, context)
            if len(anomalies) < 32:
                heapq.heappush(anomalies, item)
            else:
                heapq.heappushpop(anomalies, item)
    result["phase_anomaly_count"] = anomaly_count
    # Retain the largest errors, then list them in capture order. Otherwise a
    # small persistent clock mismatch can hide a later freeze behind the limit.
    result["phase_anomalies"] = [item[2] for item in sorted(anomalies, key=lambda item: -item[1])]
    result["phase_anomalies_truncated"] = anomaly_count > len(anomalies)
    if samples:
        errors = [((time - samples[0][0]) / 1e6 -
                   (phase - samples[0][1]) / vi_hz) * 1000 for time, phase in samples]
        result["elapsed_phase_error_ms"] = {
            "minimum": min(errors), "maximum": max(errors), "end": errors[-1]}
    else:
        result["elapsed_phase_error_ms"] = None
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--from-vi", type=int, default=0)
    parser.add_argument("--to-vi", type=int)
    parser.add_argument("--expected-fps", type=float,
                        help="Effective capped rate, accounting for VSync; omit for Unlimited")
    parser.add_argument("--vi-hz", type=float, default=60.0,
                        help="Elapsed VI clock Hz; nominal/reference default 60, qualified native NTSC 59.82616908311211")
    parser.add_argument("--require-complete", action="store_true",
                        help="Fail for a missing footer, queue drops or writer failure; older logs have no footer")
    args = parser.parse_args()
    try:
        with args.trace.open(encoding="utf-8") as source:
            result = analyze(source, first_vi=args.from_vi, last_vi=args.to_vi,
                             expected_fps=args.expected_fps, vi_hz=args.vi_hz)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2))
    if args.require_complete and not result["complete"]:
        return 2
    return 0 if result["samples"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
