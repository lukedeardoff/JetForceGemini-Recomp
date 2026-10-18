import unittest
from scripts.analyze_update_cadence import analyze,analyze_frames
def event(t,kind,v):return f"{t}\t{kind}\t"+"\t".join(f"{x:x}"for x in v)+"\n"
def work(t,vi,wall=1000,wait=100):return event(t,"update-work",[vi,vi//2,0,0,500,wall,wait])
class CadenceTests(unittest.TestCase):
 def test_every_gap_retained_with_loading_evidence(self):
  data=[event(0,"tick",[0,0,0,16,21,3,*([0]*8)]),work(2000,2),
    event(2200,"overlay-cold-publish",[3,1,5,0,0]),work(4000,8,2000),
    work(6000,11),work(8000,13)]
  r=analyze(data);self.assertEqual(r["missed_update_intervals"],2)
  self.assertTrue(r["gaps"][0]["cold_overlays_during_update"])
  self.assertTrue(r["gaps"][1]["cold_overlays_during_previous_update"])
  self.assertEqual(r["gaps"][1]["delta_vi"],3)
  self.assertEqual(r["gaps"][0]["update"]["state_after"]["level"],21)
 def test_explicit_transition_calls_are_preserved(self):
  data=[work(1000,2),event(2200,"route-transition",[3,1,0x80046378,0,0,1]),work(5000,8,3000)]
  report=analyze(data);gap=report["gaps"][0]
  self.assertEqual(gap["update"]["route_transitions"][0]["kind"],"death_restart")
  self.assertFalse(gap["cold_overlays_during_update"])
 def test_transition_between_updates_is_not_lost(self):
  data=[work(1000,2),event(1500,"route-transition",[3,1,0x8004665c,47,0,1]),work(5000,8,2000)]
  gap=analyze(data)["gaps"][0]
  self.assertEqual(gap["update"]["route_transitions"],[])
  self.assertEqual(gap["interval_transition_calls"][0]["kind"],"level_change")
 def test_delayed_transition_context_is_separate_from_interval(self):
  data=[event(500,"route-transition",[1,0,0x80046378,0,0,1]),work(1000,2),work(5000,8,2000)]
  gap=analyze(data)["gaps"][0]
  self.assertEqual(gap["interval_transition_calls"],[])
  self.assertEqual(gap["update"]["latest_route_transition"]["kind"],"death_restart")
  self.assertEqual(gap["update"]["latest_route_transition"]["age_ms"],4.5)
 def test_cycles_and_collision_do_not_change_cadence(self):
  data=[event(1000,"bsp-execution",[2,1,10,30,40,2,1,1,9000,10]),
    event(1000,"update-thread-cycles",[2,1,1,7,900,500]),work(1000,2),work(3000,4)]
  r=analyze(data);self.assertEqual(r["vi_intervals"],{2:1})
  z=next(x for x in r["slowest_updates"]if x["vi"]==2)
  self.assertEqual(z["thread_cycles"],900);self.assertEqual(z["native_collision"]["fallbacks"],2)
 def test_window_does_not_count_crossing_interval(self):
  r=analyze([work(1000,2),work(3000,7),work(5000,9)],first_vi=7,last_vi=10)
  self.assertEqual(r["missed_update_intervals"],0);self.assertEqual(r["updates"],2)
 def test_invalid_and_partial_evidence(self):
  self.assertEqual(analyze([work(1000,2)[:-1],"limit\tbyte-limit\n"])["diagnostics"],{"partial_record":1,"trace_limit":1})
  with self.assertRaisesRegex(ValueError,"waits exceed"):analyze([work(1000,2,100,200)])
  with self.assertRaisesRegex(ValueError,"duplicate"):analyze([work(1000,2),work(2000,2)])
  with self.assertRaisesRegex(ValueError,"nonmonotonic"):analyze([work(2000,4),work(1000,2)])
 def test_missing_optional_counters_are_unknown(self):
  r=analyze([work(1000,2)])
  self.assertIsNone(r["slowest_updates"][0]["thread_cycles"])
  self.assertNotIn("native_collision",r["slowest_updates"][0])
 def test_present_stall_distinct_from_game_fps(self):
  header="elapsed_us,render_frame,interval_us,scene_frames,scene_age_us,vi,queue_dropped\n"
  lines=[header,"0,1,0,1,0,2,0\n","8333,2,8333,1,8333,2,0\n","48333,4,40000,2,1000,4,1\n"]
  r=analyze_frames(lines,target_fps=120)
  self.assertEqual(r["intervals_over_1_5_target"],1)
  self.assertEqual(r["intervals_over_33_333_ms"],1)
  self.assertEqual(r["missing_render_rows"],1);self.assertEqual(r["queue_dropped_cumulative_max"],1)
 def test_csv_partial_header_and_window(self):
  header="elapsed_us,render_frame,interval_us,scene_frames,scene_age_us,vi,queue_dropped\n"
  r=analyze_frames([header,"0,1,0,1,0,2,0\n","50000,2,50000,2,0,6,0\n","58000,3,8000,2,8000,6,0\n","partial"],first_vi=6)
  self.assertEqual(r["present_intervals_ms"]["n"],1);self.assertEqual(r["partial_records"],1)
  with self.assertRaisesRegex(ValueError,"header"):analyze_frames(["x,y\n"])
if __name__=="__main__":unittest.main()
