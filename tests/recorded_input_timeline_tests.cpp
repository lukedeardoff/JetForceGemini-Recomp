#include "jfg/boot/recorded_input_timeline.hpp"
#include <array>
#include <cstdio>
int main() {
  unsigned checks=0,failures=0;
  const auto check=[&](bool ok,const char* label){++checks;if(!ok){++failures;std::fprintf(stderr,"FAILED: %s\n",label);}};
  using jfg::InputReplayEvent; using jfg::ControllerReplaySample; using jfg::boot::RecordedInputTimeline;
  ControllerReplaySample moving; moving.pc_mode=3;moving.stick_y=80;moving.look_x=10;moving.look_y=-4;moving.modern_flags=128;
  auto second=moving;second.look_x=-3;second.look_y=7;
  auto stopped=second;stopped.stick_y=0;stopped.look_x=0;stopped.look_y=0;
  const std::array<InputReplayEvent,3> events{{{10,11,moving},{12,13,second},{15,16,stopped}}};
  RecordedInputTimeline slow,fast;
  check(slow.sample(9,events)->neutral(),"before recording starts input is neutral");
  auto a=slow.sample(13,events);
  check(a && a->look_x==7 && a->look_y==3 && a->stick_y==80,"late poll conserves mouse displacement and held movement");
  a=slow.sample(14,events);check(a && a->look_x==0 && a->look_y==0 && a->stick_y==80,"mouse delta is not repeated between polls");
  a=slow.sample(14,events);check(a && a->look_x==0,"same-VI repeated poll cannot repeat motion");
  int x=0,y=0;
  for(unsigned t=9;t<=16;++t){a=fast.sample(t,events);x+=a->look_x;y+=a->look_y;}
  check(x==7 && y==3,"fast and slow polling consume identical total mouse motion");
  check(fast.sample(20,events)->stick_y==0,"released movement stays released");
  check(!fast.sample(19,events),"rewinding the clock is rejected");
  auto click=stopped;click.menu_kind=3;click.menu_target=2;
  const std::array<InputReplayEvent,2> clicked{{{10,11,click},{11,12,click}}};
  RecordedInputTimeline menus;a=menus.sample(11,clicked);
  check(a && a->menu_kind==3 && a->menu_target==2,"repeated identical menu selection merges");
  check(menus.sample(12,clicked)->menu_target==0,"menu selection is consumed once");
  auto other=click;other.menu_target=3;
  const std::array<InputReplayEvent,2> ambiguous{{{10,11,click},{11,12,other}}};
  RecordedInputTimeline conflict;check(!conflict.sample(12,ambiguous),"distinct menu selections cannot be silently merged");
  const std::array<InputReplayEvent,1> ranged{{{10,14,moving}}};RecordedInputTimeline ranges;
  check(!ranges.sample(12,ranged),"scripted held range is not a live poll recording");
  auto large=moving;large.look_x=32767;
  const std::array<InputReplayEvent,2> over{{{10,11,large},{11,12,large}}};RecordedInputTimeline overflow;
  check(!overflow.sample(12,over),"excess mouse motion is rejected rather than clipped");
  auto stick=moving;stick.pc_mode=5;stick.look_x=2048;const std::array<InputReplayEvent,1> sticks{{{10,11,stick}}};RecordedInputTimeline rate;
  check(rate.sample(10,sticks)->look_x==2048 && rate.sample(12,sticks)->look_x==2048,"stick look remains a held rate");
  auto press=stopped;press.buttons=0x1000;
  auto release=press;release.buttons=0;
  const std::array<InputReplayEvent,2> taps{{{20,21,press},{20,21,release}}};
  RecordedInputTimeline tap;a=tap.sample(20,taps);
  check(a && a->buttons==0x1000,"same-VI press and release produces one pressed sample");
  check(tap.sample(20,taps)->buttons==0,"repeat poll observes release without repeating edge");
  const std::array<InputReplayEvent,2> held{{{20,21,press},{25,26,release}}};
  RecordedInputTimeline holding;
  check(holding.sample(20,held)->buttons==0x1000 && holding.sample(24,held)->buttons==0x1000,"held button persists across polls");
  check(holding.sample(25,held)->buttons==0,"release after delivered press has no extra edge");
  RecordedInputTimeline skipped;
  check(skipped.sample(30,held)->buttons==0x1000 && skipped.sample(31,held)->buttons==0,"late consumer preserves one short button edge");
  std::printf("%u recorded timeline checks, %u failures\n",checks,failures);return failures==0?0:1;
}
