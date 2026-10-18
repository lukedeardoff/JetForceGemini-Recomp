#include "jfg/runtime/pc_rumble.hpp"
#include "jfg/runtime/pc_rumble_output.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <mutex>
#include <vector>

namespace {
int checks=0;
void check(bool ok,const char* name){++checks;if(!ok){std::cerr<<name<<'\n';std::exit(1);}}
}
int main() {
  using Kind=jfg::PcRumbleRequest;
  jfg::PcRumbleEffect effect;
  check(effect.sample(0)==0,"idle is silent");
  effect.request(Kind::start,50,0.2F,100);
  const auto medium=effect.sample(100);
  check(medium>30000 && medium<40000,"guest intensity maps to bounded analog amplitude");
  check(effect.sample(111)==medium && effect.sample(112)==0,"effect expires at guest duration");
  effect.request(Kind::start,100,1.0F,200);
  check(effect.sample(200)==65535,"high guest strength saturates");
  check(effect.sample(200,50)==32767 && effect.sample(200,0)==0,"user rumble strength scales output");
  effect.request(Kind::maximum,10,2.0F,210);
  check(effect.sample(260)==65535 && effect.sample(330)==0,"maximum preserves stronger effect and extends duration");
  effect.request(Kind::start,50,1.0F,400);
  effect.request(Kind::alter,30,0.0F,410);
  check(effect.sample(410)>0 && effect.sample(410)<medium && effect.sample(460)==0,"alter strength preserves old deadline");
  effect.request(Kind::start,50,0.2F,500);
  effect.request(Kind::alter,0,1.0F,505);
  check(effect.sample(540)==medium && effect.sample(565)==0,"alter duration preserves strength");
  effect.request(Kind::start,50,1.0F,600);effect.request(Kind::stop,0,0,601);
  check(effect.sample(602)==0,"guest stop is immediate");
  effect.request(Kind::start,0,1,700);effect.request(Kind::alter,50,0,710);
  check(effect.sample(710)==medium && effect.sample(760)==0,"alter can raise a silent but timed effect");
  effect.request(Kind::alter,0,1,770);
  check(effect.sample(770)==medium && effect.sample(830)==0,"alter duration can reuse last requested intensity");
  effect.clear();
  for(std::uint64_t tick=1000;tick<1240;tick+=10)effect.request(Kind::maximum,100,0.5F,tick);
  check(effect.sample(1239)>0 && effect.sample(1240)==0,"continuous motor use stops after four seconds");
  effect.request(Kind::start,100,1,1250);check(effect.sample(1250)==0,"cooldown rejects restart");
  effect.request(Kind::start,100,1,1300);check(effect.sample(1300)>0,"cooldown ends after one second");
  effect.clear();
  for(const float seconds:{-1.0F,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
    effect.request(Kind::start,100,seconds,10);check(effect.sample(10)==0,"invalid duration cannot start motor");
  }
  effect.request(Kind::start,-123,1,10);check(effect.sample(10)==0,"negative strength is silent");
  effect.request(Kind::start,std::numeric_limits<int>::max(),std::numeric_limits<float>::max(),100);
  check(effect.sample(100)==65535 && effect.sample(340)==0,"extreme request clamps amplitude and duration");
  effect.request(Kind::start,100,1,400);check(effect.sample(399)==0,"clock rewind clears stale effect");
  effect.request(Kind::start,100,1,500);effect.clear();check(effect.sample(500)==0,"disable or focus reset discards effect");
  check(jfg::route_pc_rumble({100,200,300,400},{2,-3,-2,0})==std::array<std::uint16_t,4>{400,0,100,0},"guest ports route to assigned devices; KBM and disconnected never vibrate");
  check(jfg::route_pc_rumble({100,200,300,400},{2,2,7,-1})==std::array<std::uint16_t,4>{0,0,200,0},"invalid devices ignored and duplicate requests do not add");

  std::mutex mutex;std::condition_variable wake;
  std::array<std::uint16_t,4> current{};std::array<unsigned,4> calls{};
  std::atomic<bool> focused=true,connected=true;
  auto wait=[&](auto predicate) {std::unique_lock lock(mutex);return wake.wait_for(lock,std::chrono::seconds(2),predicate);};
  jfg::PcRumbleOutput output([&](unsigned device,std::uint16_t value) {
    std::lock_guard lock(mutex);++calls[device];
    if(connected.load())current[device]=value;
    wake.notify_all();return connected.load();
  },[&](std::uintptr_t window){return focused.load() && window==42;});
  output.publish({0,12345,0,0},42);
  check(wait([&]{return current[1]==12345;}),"worker emits assigned motor amplitude");
  focused=false;check(wait([&]{return current[1]==0;}),"focus loss stops output without a guest update");
  focused=true;
  {std::unique_lock lock(mutex);check(!wake.wait_for(lock,std::chrono::milliseconds(35),[&]{return current[1]!=0;}),"returning focus does not revive expired lease");}
  output.publish({0,23456,0,0},42);check(wait([&]{return current[1]==23456;}),"new eligible frame renews lease");
  check(wait([&]{return current[1]==0;}),"stalled guest expires motor output after 250 ms");
  output.publish({0,2222,0,0},0);
  {std::unique_lock lock(mutex);check(!wake.wait_for(lock,std::chrono::milliseconds(35),[&]{return current[1]!=0;}),"missing window cannot start output");}
  output.publish({0,3333,0,4444},42);check(wait([&]{return current[1]==3333 && current[3]==4444;}),"independent devices receive separate output");
  connected=false;output.publish({},42);
  {std::unique_lock lock(mutex);const auto before=calls[1];check(wake.wait_for(lock,std::chrono::seconds(1),[&]{return calls[1]>before;}),"disconnect stop attempted");}
  connected=true;check(wait([&]{return current[1]==0 && current[3]==0;}),"pending stop retried after device reconnects");
  output.publish({0,5555,0,0},42);check(wait([&]{return current[1]==5555;}),"new request after reconnect");
  output.stop();
  {std::lock_guard lock(mutex);check(current[1]==0 && current[3]==0,"shutdown clears every motor owned by this worker");check(calls[0]==0 && calls[2]==0,"unassigned devices never receive motor commands");}
  output.stop();check(true,"shutdown is idempotent");
  std::cout<<checks<<" PC rumble checks passed\n";
}
