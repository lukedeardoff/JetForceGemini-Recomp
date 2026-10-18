"""Attribute bounded per-update function-profile records without double counting.

CPU cycles belong to the root update's host thread; wall durations include waits
and preemption. Full instrumentation adds overhead. Native FPS acceptance must
be repeated with it disabled. No CPU-cycle-to-millisecond conversion is made.
"""
from __future__ import annotations
import argparse
import bisect
from collections import Counter, defaultdict
import json
from pathlib import Path
import statistics

ROOT=0x80044FAC
NAMES={ROOT:"game_update",0x80008FF0:"object_updates",0x00310600:"drone_behavior",
       0x09B02FD0:"pathfinding_parent",0x8003D21C:"animation_matrices",
       0x800121C0:"track_draw",0x8004DA10:"rcpFast3d",0x8004DCB0:"rcpWaitDP"}
FIELDS=("target","parent","calls","wall_ns","self_wall_ns","cycles","self_cycles",
        "known_wait_us","observed_instructions","max_wall_ns","max_cycles","valid")

def stats(values):
    values=sorted(values)
    return dict(n=len(values),median=statistics.median(values),
                p95=values[int(.95*(len(values)-1))],maximum=max(values)) if values else {}

def analyze(lines, *, first_vi=0, last_vi=None, names=None):
    if first_vi<0 or (last_vi is not None and last_vi<=first_vi):
        raise ValueError("expected a nonnegative, nonempty VI window")
    labels={**NAMES,**(names or {})}
    pending=defaultdict(list); work={}; updates=[]; errors=Counter();totals=defaultdict(Counter)
    auxiliary=defaultdict(list);timestamps_by_update={};hosts={};emissions={}
    latest_submit=None;submit_details={};submission_extra=defaultdict(dict)
    def named(row):
        return {**row,"target":f"0x{row['target']:08x}","parent":f"0x{row['parent']:08x}",
                "name":labels.get(row["target"],"unresolved"),"parent_name":labels.get(row["parent"],"root" if not row["parent"] else "unresolved")}
    for number,line in enumerate(lines,1):
        f=line.rstrip("\r\n").split("\t")
        if line.startswith("limit\t"):
            errors["trace_limit"]+=1;continue
        if len(f)<2 or f[1] not in {"function-profile","function-profile-end","update-work","function-profile-host","function-profile-emission","submit","submit-detail","submit-host","submit-sync","submit-thread","gpu-framebuffer-time","gpu-workload-time"}:continue
        if not line.endswith("\n"):
            errors["partial_record"]+=1;continue
        try:
            values=[int(x,16) for x in f[2:]]
            if any(x<0 for x in values):raise ValueError("negative value")
            expected={"function-profile":17,"function-profile-end":10,"update-work":7,"function-profile-host":10,"function-profile-emission":8,"submit":9,"submit-detail":12,"submit-host":6,"submit-sync":15,"submit-thread":11,"gpu-framebuffer-time":7,"gpu-workload-time":7}[f[1]]
            if len(values)!=expected:raise ValueError("wrong field count")
        except ValueError as error:
            raise ValueError(f"invalid record on line {number}: {error}") from error
        stamp=int(f[0])
        if stamp<0:raise ValueError("negative timestamp")
        if f[1] in {"submit-host","submit-sync","submit-thread"}:
            if f[1]=="submit-thread" and (values[4] not in (0,1) or (values[4]==1 and values[3]==0)):
                raise ValueError("invalid submission thread validity")
            if latest_submit is None or values[:3]!=latest_submit[:3] or id(latest_submit) in submission_extra[f[1]]:
                errors["unpaired_"+f[1].replace("-","_")]+=1
            else:submission_extra[f[1]][id(latest_submit)]=values
            continue
        if f[1]=="submit-detail":
            if latest_submit is None or values[:3]!=latest_submit[:3] or id(latest_submit) in submit_details:
                errors["unpaired_submit_detail"]+=1
            else:submit_details[id(latest_submit)]=values
            continue
        if f[1] in {"submit","gpu-framebuffer-time","gpu-workload-time"}:
            if f[1]=="submit":latest_submit=values
            auxiliary[f[1]].append((stamp,values));continue
        if f[1]=="function-profile-host":
            hosts[tuple(values[:5])]=values;continue
        if f[1]=="function-profile-emission":
            emissions[tuple(values[:5])]=values;continue
        if f[1]=="update-work":
            work[tuple(values[:2])]=values
            timestamps_by_update[tuple(values[:2])]=stamp
            continue
        key=tuple(values[:5])
        if f[1]=="function-profile":
            pending[key].append(dict(zip(FIELDS,values[5:])));continue
        rows=pending.pop(key,[])
        if not first_vi<=key[0] or (last_vi is not None and key[0]>=last_vi):continue
        if values[5] not in (1,2) or values[6] not in (0,1) or values[9] not in (0,1):
            raise ValueError("invalid mode/status")
        if len(rows)!=values[8] or sum(x["calls"] for x in rows)!=values[7]:
            errors["missing_rows_or_calls"]+=1;continue
        roots=[x for x in rows if x["target"]==ROOT and x["parent"]==0]
        if len(roots)!=1 or len({(x["target"],x["parent"]) for x in rows})!=len(rows):
            errors["invalid_root_or_duplicate_edge"]+=1;continue
        if not values[6] or values[9] or not all(x["valid"]==1 for x in rows):
            errors["invalid_counters_or_overflow"]+=1;continue
        root=roots[0]
        if sum(x["self_cycles"] for x in rows)!=root["cycles"] or sum(x["self_wall_ns"] for x in rows)!=root["wall_ns"]:
            raise ValueError("exclusive costs do not sum to root")
        observed=work.get(key[:2])
        if observed is None:
            errors["missing_update_work"]+=1;continue
        row={"vi":key[0],"poll":key[1],"start_vi":key[2],"start_poll":key[3],
             "thread":key[4],"mode":values[5],"calls":values[7],"edges":len(rows),
             "root_wall_ms":root["wall_ns"]/1e6,"root_cycles":root["cycles"],
             "update_wall_ms":observed[5]/1000,"known_wait_ms":observed[6]/1000,
             "update_active_ms":(observed[5]-observed[6])/1000,
             "observed_instructions":observed[4],
             "top_cpu":[named(x) for x in sorted(rows,key=lambda x:x["self_cycles"],reverse=True)[:15]],
             "top_wall":[named(x) for x in sorted(rows,key=lambda x:x["self_wall_ns"],reverse=True)[:15]]}
        row["_key"]=key;row["_timestamp"]=timestamps_by_update[key[:2]]
        updates.append(row)
        for x in rows:
            target=totals[x["target"]]
            for field in ("calls","self_wall_ns","self_cycles","cycles","wall_ns"):target[field]+=x[field]
    for kind,observations in auxiliary.items():
        if any(a[0]>z[0] for a,z in zip(observations,observations[1:])):
            raise ValueError("nonmonotonic auxiliary timestamps")
    auxiliary_times={kind:[t for t,v in observations] for kind,observations in auxiliary.items()}
    for row in updates:
        key=row.pop("_key");end=row.pop("_timestamp");begin=end-int(row["update_wall_ms"]*1000)
        host=hosts.get(key);emit=emissions.get(key)
        if host:
            row["host"]={"valid":bool(host[5]),"process_cycles":host[6],
                         "guest_handoffs":host[7],"baton_wakes":host[8],"baton_wake_ms":host[9]/1e6}
            if host[5]!=1:errors["invalid_host_counter"]+=1
        if emit:
            row["profiler_emission"]={"valid":bool(emit[5]),"wall_ms":emit[6]/1e6,"cycles":emit[7]}
            if emit[5]!=1:errors["invalid_emission_counter"]+=1
        submits=auxiliary["submit"];times=auxiliary_times.get("submit",[])
        covered=submits[bisect.bisect_left(times,begin):bisect.bisect_right(times,end)]
        complete=[v for t,v in covered if t-v[3]>=begin]
        row["graphics"]={"complete_submissions":len(complete),
                         "boundary_crossing_submissions":len(covered)-len(complete),
                         "submit_ms":sum(v[3] for v in complete)/1000,
                         "fullsync_ms":sum(v[7] for v in complete)/1000,
                         "rdram_check_ms":sum(v[6] for v in complete)/1000}
        details=[submit_details[id(v)] for v in complete if id(v) in submit_details]
        if details:
            row["graphics"]["profiled_submissions"]=len(details)
            row["graphics"]["history_prepare_ms"]=sum(v[3] for v in details)/1000
            row["graphics"]["rdp_sync_ms"]=sum(v[4] for v in details)/1000
            row["graphics"]["history_stages_ms"]={
                name:sum(v[index] for v in details)/1000 for index,name in
                [(5,"geometry_keys"),(6,"matrix_rigid_shape"),(7,"semantic_history"),
                 (8,"world_endpoint_conversion"),(9,"coverage_diagnostics")]}
            row["graphics"]["vertices_loaded"]=sum(v[10] for v in details)
            row["graphics"]["triangles_drawn"]=sum(v[11] for v in details)
        for kind in ("submit-host","submit-sync","submit-thread"):
            if submission_extra[kind]:
                errors["missing_"+kind.replace("-","_")]+=sum(id(v) not in submission_extra[kind] for v in complete)
        stages=[submission_extra["submit-host"][id(v)] for v in complete if id(v) in submission_extra["submit-host"]]
        if stages:
            row["graphics"]["host_profiled_submissions"]=len(stages)
            row["graphics"]["host_stages_ms"]={
                name:sum(v[index] for v in stages)/1e6 for index,name in [(3,"snapshot"),(4,"decode"),(5,"writeback")]}
        waits=[submission_extra["submit-sync"][id(v)] for v in complete if id(v) in submission_extra["submit-sync"]]
        if waits:
            row["graphics"]["wait_profiled_submissions"]=len(waits)
            row["graphics"]["synchronization_waits"]={
                name:{"wall_ms":sum(v[3+index] for v in waits)/1e6,"calls":sum(v[9+index] for v in waits)}
                for index,name in enumerate(("draw","transform","render","fence","texture","queue"))}
        cpu=[submission_extra["submit-thread"][id(v)] for v in complete if id(v) in submission_extra["submit-thread"]]
        if cpu:
            owners=defaultdict(lambda:{"submissions":0,"cycles":0,"os_cpu_ms":0.0,
                                      "stage_cycles":[0,0,0],"stage_os_cpu_ms":[0.0,0.0,0.0]})
            for v in cpu:
                if v[4]!=1:
                    errors["invalid_submission_thread"]+=1
                    continue
                owner=owners[v[3]];owner["submissions"]+=1
                owner["cycles"]+=sum(v[5:8]);owner["os_cpu_ms"]+=sum(v[8:11])/1e4
                for index in range(3):
                    owner["stage_cycles"][index]+=v[5+index]
                    owner["stage_os_cpu_ms"][index]+=v[8+index]/1e4
            row["graphics"]["submission_threads"]=[
                {"thread":thread,"is_root_thread":thread==row["thread"],**owner} for thread,owner in sorted(owners.items())]
        row["gpu_completed_observations"]={}
        for kind in ("gpu-framebuffer-time","gpu-workload-time"):
            observations=auxiliary[kind];times=auxiliary_times.get(kind,[])
            a=bisect.bisect_right(times,begin)-1;z=bisect.bisect_right(times,end)-1
            if a<0 or z<0:continue
            av,zv=observations[a][1],observations[z][1]
            valid=zv[2]>=av[2] and zv[3]>=av[3] and zv[6]==0
            row["gpu_completed_observations"][kind]={
                "valid":valid,"submissions":zv[2]-av[2],"total_ms":(zv[3]-av[3])/1e6,
                "limit":"Completed observations in this interval; GPU work can begin earlier. Do not add to CPU wall time."}
            if not valid:errors["invalid_gpu_counter"]+=1
    errors["unfinished_edges"]=sum(len(x) for x in pending.values())
    cycles=sum(x["root_cycles"] for x in updates)
    functions=[{"target":f"0x{target:08x}","name":labels.get(target,"unresolved"),**v,
                "exclusive_cpu_percent":100*v["self_cycles"]/cycles if cycles else None}
               for target,v in totals.items()]
    functions.sort(key=lambda x:x["self_cycles"],reverse=True)
    return {"schema":1,"updates":len(updates),"modes":dict(Counter(x["mode"] for x in updates)),
            "diagnostics":dict(errors),"root_cycles":stats([x["root_cycles"] for x in updates]),
            "active_ms":stats([x["update_active_ms"] for x in updates]),
            "functions":functions,"slowest_updates":sorted(updates,key=lambda x:x["update_active_ms"],reverse=True)[:20],
            "limits":__doc__}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("trace",type=Path)
    p.add_argument("--first-vi",type=int,default=0);p.add_argument("--last-vi",type=int)
    p.add_argument("--symbols",type=Path,help="Optional JSON mapping hex target strings to names")
    p.add_argument("--output",type=Path);a=p.parse_args()
    names={int(k,16):v for k,v in json.loads(a.symbols.read_text()).items()} if a.symbols else {}
    with a.trace.open(encoding="utf-8") as f:
        result=analyze(f,first_vi=a.first_vi,last_vi=a.last_vi,names=names)
    text=json.dumps(result,indent=2)
    if a.output:a.output.write_text(text+"\n",encoding="utf-8")
    else:print(text)

if __name__=="__main__":main()
