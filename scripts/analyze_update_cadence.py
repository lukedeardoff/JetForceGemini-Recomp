"""Report every observed game-update cadence gap and present-return stall.
Cold-overlay activity is evidence near a gap, not proof of its cause. Render
presentation is not game simulation or physical monitor scanout. Wall minus
known waits still includes unmeasured waits and OS preemption.
"""
from __future__ import annotations
import argparse
import bisect
from collections import Counter, defaultdict
import csv
import json
from pathlib import Path
import statistics

def stats(values):
    v=sorted(values)
    return {"n":len(v),"median":statistics.median(v),"p95":v[int(.95*(len(v)-1))],
            "p99":v[int(.99*(len(v)-1))],"maximum":v[-1]} if v else {}

def analyze(lines, *, first_vi=0, last_vi=None, expected_delta_vi=2):
    if first_vi<0 or (last_vi is not None and last_vi<=first_vi) or expected_delta_vi<1:
        raise ValueError("expected a nonempty VI window and positive update period")
    records=defaultdict(list);diagnostics=Counter()
    fields={"update-work":7,"update-thread-cycles":6,"bsp-execution":10,
            "route-transition":6,"overlay-cold-prepare":None,"overlay-cold-publish":5,"tick":14,"update":None}
    for number,line in enumerate(lines,1):
        if line.startswith("limit\t"):diagnostics["trace_limit"]+=1;continue
        f=line.rstrip("\r\n").split("\t")
        if len(f)<2 or f[1] not in fields:continue
        if not line.endswith("\n"):diagnostics["partial_record"]+=1;continue
        try:
            stamp=int(f[0]);v=[int(x,16)for x in f[2:]]
            if stamp<0 or any(x<0 for x in v):raise ValueError("negative value")
            count=fields[f[1]]
            if count is not None and len(v)!=count:raise ValueError("wrong field count")
            if f[1] in ("tick","update") and len(v)<6:raise ValueError("short state row")
        except ValueError as error:
            raise ValueError(f"invalid {f[1]} on line {number}: {error}") from error
        records[f[1]].append((stamp,v))
    work=records["update-work"]
    if any(a[0]>z[0] or a[1][0]>z[1][0] for a,z in zip(work,work[1:])):
        raise ValueError("nonmonotonic game updates")
    if len({tuple(v[:2])for t,v in work})!=len(work):
        raise ValueError("ambiguous duplicate update key")
    if any(v[6]>v[5] for t,v in work):raise ValueError("known waits exceed update wall time")
    cycles={tuple(v[:2]):v for t,v in records["update-thread-cycles"]}
    bsp={tuple(v[:2]):v for t,v in records["bsp-execution"]}
    states=sorted(records["tick"]+records["update"]);state_times=[t for t,v in states]
    cold=sorted([(t,k,v)for k in ("overlay-cold-prepare","overlay-cold-publish")for t,v in records[k]])
    cold_times=[x[0]for x in cold]
    transitions=sorted(records["route-transition"]);transition_times=[t for t,v in transitions]
    def state_at(stamp):
        i=bisect.bisect_right(state_times,stamp)-1
        if i<0:return None
        t,v=states[i]
        return {"stamp_us":t,"vi":v[0],"poll":v[1],"game_state":v[3],"level":v[4],"actors":v[5]}
    def describe(t,v):
        begin=t-v[5];cycle=cycles.get(tuple(v[:2]));helper=bsp.get(tuple(v[:2]))
        if cycle is not None and cycle[2]!=1:diagnostics["invalid_root_cycles"]+=1
        near=cold[bisect.bisect_left(cold_times,begin):bisect.bisect_right(cold_times,t)]
        result={"vi":v[0],"poll":v[1],"return_us":t,"wall_ms":v[5]/1000,
                "active_ms":(v[5]-v[6])/1000,"known_wait_ms":v[6]/1000,
                "observed_instructions":v[4],
                "thread_cycles":cycle[4] if cycle is not None and cycle[2]==1 else None,
                "state_before":state_at(begin),"state_after":state_at(t),
                "cold_overlay_events":len(near),"cold_overlay_sections":sorted({x[2][2]for x in near if len(x[2])>=3})}
        transition_rows=transitions[bisect.bisect_left(transition_times,begin):bisect.bisect_right(transition_times,t)]
        result["route_transitions"]=[{"vi":v[0],"poll":v[1],
            "kind":{0x8004665c:"level_change",0x80046378:"death_restart"}.get(v[2],"unknown"),
            "target":f"0x{v[2]:08x}","arguments":v[3:5],"call_count":v[5]}for stamp,v in transition_rows]
        recent=bisect.bisect_right(transition_times,t)-1
        result["latest_route_transition"]=None
        if recent>=0:
            stamp,v=transitions[recent]
            result["latest_route_transition"]={"stamp_us":stamp,"vi":v[0],"poll":v[1],
              "kind":{0x8004665c:"level_change",0x80046378:"death_restart"}.get(v[2],"unknown"),
              "age_ms":(t-stamp)/1000}
        if helper is not None:
            result["native_collision"]=dict(zip(
              ("calls","nodes","edges","fallbacks","expired","invalid","max_call_ns","clock_refreshes"),helper[2:]))
        return result
    selected=[(t,v)for t,v in work if v[0]>=first_vi and (last_vi is None or v[0]<last_vi)]
    described=[describe(t,v)for t,v in selected];counts=Counter();gaps=[]
    for a,z in zip(described,described[1:]):
        delta=z["vi"]-a["vi"];counts[delta]+=1
        if delta>expected_delta_vi:
            gaps.append({"delta_vi":delta,"missed_vi_slots":delta-expected_delta_vi,
              "wall_between_returns_ms":(z["return_us"]-a["return_us"])/1000,
              "interval_transition_calls":[{"stamp_us":stamp,"vi":v[0],"poll":v[1],
                "kind":{0x8004665c:"level_change",0x80046378:"death_restart"}.get(v[2],"unknown"),
                "target":f"0x{v[2]:08x}"}for stamp,v in transitions[
                  bisect.bisect_right(transition_times,a["return_us"]):
                  bisect.bisect_right(transition_times,z["return_us"])]],
              "cold_overlays_during_update":bool(z["cold_overlay_events"]),
              "cold_overlays_during_previous_update":bool(a["cold_overlay_events"]),
              "previous_update":a,"update":z})
    return {"schema":1,"updates":len(selected),"first_vi":first_vi,"last_vi":last_vi,
      "expected_delta_vi":expected_delta_vi,"vi_intervals":dict(counts),
      "missed_update_intervals":len(gaps),"gaps":gaps,
      "active_ms":stats([x["active_ms"]for x in described]),
      "wall_ms":stats([x["wall_ms"]for x in described]),
      "known_wait_ms":stats([x["known_wait_ms"]for x in described]),
      "thread_cycles":stats([x["thread_cycles"]for x in described if x["thread_cycles"]is not None]),
      "slowest_updates":sorted(described,key=lambda x:x["active_ms"],reverse=True)[:20],
      "diagnostics":dict(diagnostics),"limits":__doc__}

def analyze_frames(lines, *, first_vi=0, last_vi=None, target_fps=None):
    if target_fps is not None and target_fps<=0:raise ValueError("positive FPS required")
    rows=[];partial=0
    for line in lines:
        if line.startswith("#"):continue
        if not line.endswith("\n"):partial+=1;continue
        rows.append(line)
    reader=csv.DictReader(rows)
    expected=["elapsed_us","render_frame","interval_us","scene_frames","scene_age_us","vi","queue_dropped"]
    if reader.fieldnames!=expected:raise ValueError("unsupported frame CSV header")
    intervals=[];ages=[];drops=0;previous=None;begin=None;end=None;missing=0
    for row in reader:
        if None in row or any(row[k] is None for k in expected):raise ValueError("incomplete frame CSV row")
        v={k:int(row[k])for k in expected}
        if any(n<0 for n in v.values()):raise ValueError("negative frame CSV value")
        if previous is not None and (v["elapsed_us"]<previous["elapsed_us"] or v["render_frame"]<=previous["render_frame"]):
            raise ValueError("nonmonotonic frame CSV")
        selected=v["vi"]>=first_vi and (last_vi is None or v["vi"]<last_vi)
        if selected:
            drops=max(drops,v["queue_dropped"]);ages.append(v["scene_age_us"]/1000)
            if begin is None:begin=v["elapsed_us"]
            end=v["elapsed_us"]
            if previous is not None and previous["vi"]>=first_vi:
                missing+=max(0,v["render_frame"]-previous["render_frame"]-1)
                if v["interval_us"]:intervals.append(v["interval_us"]/1000)
        previous=v
    return {"present_intervals_ms":stats(intervals),"scene_age_ms":stats(ages),
      "present_return_fps_from_intervals":1000/statistics.mean(intervals)if intervals else None,
      "intervals_over_16_667_ms":sum(x>1000/60 for x in intervals),
      "intervals_over_33_333_ms":sum(x>1000/30 for x in intervals),
      "intervals_over_1_5_target":sum(x>1500/target_fps for x in intervals)if target_fps else None,
      "target_fps":target_fps,"queue_dropped_cumulative_max":drops,"missing_render_rows":missing,
      "partial_records":partial,"selected_elapsed_ms":(end-begin)/1000 if begin is not None else 0,
      "limits":"Present-return timings are not monitor scanout or game-update FPS. Queue drops/missing rows limit completeness; report them, never infer no stutter from an incomplete capture."}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("trace",type=Path)
    p.add_argument("--first-vi",type=int,default=0);p.add_argument("--last-vi",type=int)
    p.add_argument("--expected-delta-vi",type=int,default=2)
    p.add_argument("--frames",type=Path,action="append",default=[])
    p.add_argument("--target-fps",type=float);p.add_argument("--output",type=Path)
    a=p.parse_args()
    with a.trace.open(encoding="utf-8")as f:
        report=analyze(f,first_vi=a.first_vi,last_vi=a.last_vi,expected_delta_vi=a.expected_delta_vi)
    report["frame_segments"]=[]
    for path in a.frames:
        with path.open(encoding="utf-8")as f:
            result=analyze_frames(f,first_vi=a.first_vi,last_vi=a.last_vi,target_fps=a.target_fps)
        report["frame_segments"].append({"file":path.name,**result})
    text=json.dumps(report,indent=2)+"\n"
    if a.output:a.output.write_text(text,encoding="utf-8")
    else:print(text,end="")
if __name__=="__main__":main()
