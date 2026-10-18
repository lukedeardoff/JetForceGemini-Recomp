import unittest
from scripts.analyze_function_profile import analyze,ROOT
def row(kind,values):
 return "1000\t"+kind+"\t"+"\t".join(f"{x:x}" for x in values)+"\n"
KEY=[100,20,98,19,7]
def fixture():
 return [row("update-work",[100,20,1,0,100,1000,100]),
  row("function-profile",KEY+[2,ROOT,2,400,400,40,40,0,30,300,30,1]),
  row("function-profile",KEY+[ROOT,0,1,1000,600,100,60,100,100,1000,100,1]),
  row("function-profile-end",KEY+[2,1,3,2,0])]
class FunctionProfileTests(unittest.TestCase):
 def test_exclusive_percentages_and_slow_update(self):
  result=analyze(fixture());self.assertEqual(result["updates"],1)
  self.assertEqual(sum(x["exclusive_cpu_percent"] for x in result["functions"]),100)
  self.assertEqual(result["slowest_updates"][0]["calls"],3)
  self.assertEqual(result["slowest_updates"][0]["update_active_ms"],.9)
 def test_incomplete_capture(self):
  self.assertEqual(analyze(fixture()[:-1])["diagnostics"]["unfinished_edges"],2)
 def test_missing_edge(self):
  result=analyze(fixture()[:1]+fixture()[2:])
  self.assertEqual(result["updates"],0);self.assertEqual(result["diagnostics"]["missing_rows_or_calls"],1)
 def test_bad_sums_rejected(self):
  data=fixture();data[1]=row("function-profile",KEY+[2,ROOT,2,400,400,40,41,0,30,300,30,1])
  with self.assertRaisesRegex(ValueError,"do not sum"):analyze(data)
 def test_invalid_and_capacity_are_explicit(self):
  data=fixture();data[-1]=row("function-profile-end",KEY+[2,0,3,2,1])
  self.assertEqual(analyze(data)["diagnostics"]["invalid_counters_or_overflow"],1)
  self.assertEqual(analyze(["limit\tbyte-limit\n"])["diagnostics"]["trace_limit"],1)
 def test_window_and_symbol_names(self):
  self.assertEqual(analyze(fixture(),last_vi=100)["updates"],0)
  report=analyze(fixture(),names={2:"child"})
  self.assertEqual(report["functions"][1]["name"],"child")
 def test_host_emission_and_completed_gpu_are_correlated(self):
  data=[row("gpu-workload-time",[98,19,10,1000,100,100,0]).replace("1000\t","0\t",1)]
  data+=fixture()[:-1]+[row("function-profile-host",KEY+[1,200,2,4,10000]),fixture()[-1],
   row("function-profile-emission",KEY+[1,40000,123])]
  report=analyze(data);update=report["slowest_updates"][0]
  self.assertEqual(update["host"]["process_cycles"],200)
  self.assertEqual(update["host"]["baton_wake_ms"],.01)
  self.assertEqual(update["profiler_emission"]["wall_ms"],.04)
  self.assertTrue(update["gpu_completed_observations"]["gpu-workload-time"]["valid"])
 def test_submission_detail_preserves_repeated_scene_keys(self):
  base=fixture()
  a=row("submit",[100,20,1,100,0,0,0,50,0]).replace("1000\t","200\t",1)
  b=row("submit",[100,20,1,100,0,0,0,50,0]).replace("1000\t","500\t",1)
  da=row("submit-detail",[100,20,1,30,20,10,10,5,3,2,99,44])
  db=row("submit-detail",[100,20,1,40,10,20,10,5,3,2,88,33])
  result=analyze([a,da,b,db]+base)["slowest_updates"][0]["graphics"]
  self.assertEqual(result["profiled_submissions"],2)
  self.assertEqual(result["history_prepare_ms"],.07)
  self.assertEqual(result["vertices_loaded"],187)
  self.assertEqual(result["history_stages_ms"]["geometry_keys"],.03)
 def test_truncated_or_malformed_rows(self):
  self.assertEqual(analyze([fixture()[1].rstrip()])["diagnostics"]["partial_record"],1)
  with self.assertRaises(ValueError):analyze(["0\tfunction-profile\txyz\n"])

 def test_submission_host_wait_and_thread_ownership(self):
  records=[]
  for stamp,owner,m in [(200,7,1),(500,11,2)]:
   records.append(row("submit",[100,20,1,100,0,0,0,50,0]).replace("1000\t",f"{stamp}\t",1))
   records.append(row("submit-host",[100,20,1,1000*m,2000*m,3000*m]))
   records.append(row("submit-sync",[100,20,1]+[1000*m]*6+[m]*6))
   records.append(row("submit-thread",[100,20,1,owner,1]+[10*m]*3+[20*m]*3))
  report=analyze(records+fixture());g=report["slowest_updates"][0]["graphics"]
  self.assertEqual(g["host_stages_ms"],dict(snapshot=.003,decode=.006,writeback=.009))
  self.assertEqual(g["synchronization_waits"]["fence"],dict(wall_ms=.003,calls=3))
  owners=g["submission_threads"]
  self.assertEqual(owners[0]["thread"],7);self.assertTrue(owners[0]["is_root_thread"])
  self.assertEqual(owners[0]["cycles"],30);self.assertEqual(owners[0]["os_cpu_ms"],.006)
  self.assertEqual(owners[1]["thread"],11);self.assertFalse(owners[1]["is_root_thread"])
  self.assertEqual(owners[1]["cycles"],60)
  self.assertFalse(any(report["diagnostics"].values()))
 def test_unpaired_and_missing_submission_observations(self):
  host=row("submit-host",[100,20,1,1,2,3])
  a=row("submit",[100,20,1,100,0,0,0,50,0]).replace("1000\t","200\t",1)
  b=a.replace("200\t","500\t",1)
  result=analyze([host,a,host,host,b]+fixture())
  self.assertEqual(result["diagnostics"]["unpaired_submit_host"],2)
  self.assertEqual(result["diagnostics"]["missing_submit_host"],1)
 def test_invalid_submission_thread_is_not_cpu_work(self):
  a=row("submit",[100,20,1,100,0,0,0,50,0]).replace("1000\t","200\t",1)
  cpu=row("submit-thread",[100,20,1,7,0]+[99]*6)
  report=analyze([a,cpu]+fixture())
  self.assertEqual(report["diagnostics"]["invalid_submission_thread"],1)
  self.assertEqual(report["slowest_updates"][0]["graphics"]["submission_threads"],[])
  with self.assertRaisesRegex(ValueError,"invalid submission thread validity"):
   analyze([a,row("submit-thread",[100,20,1,7,2]+[0]*6)]+fixture())
 def test_submission_extra_boundary_exclusion(self):
  a=row("submit",[100,20,1,500,0,0,0,50,0]).replace("1000\t","200\t",1)
  host=row("submit-host",[100,20,1,100000,200000,300000])
  g=analyze([a,host]+fixture())["slowest_updates"][0]["graphics"]
  self.assertEqual(g["boundary_crossing_submissions"],1)
  self.assertEqual(g["complete_submissions"],0)
  self.assertNotIn("host_stages_ms",g)
if __name__=="__main__":unittest.main()
