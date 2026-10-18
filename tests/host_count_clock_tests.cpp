#include "jfg/boot/host_count_clock.hpp"
#include "jfg/boot/native_frame_recovery.hpp"
#include <chrono>
#include <cstdio>
#include <vector>
#include <algorithm>
using jfg::boot::HostCountClock;
using namespace std::chrono_literals;
int main() {
  unsigned checks = 0, failures = 0;
  const auto check = [&](bool condition, const char* label) {
    ++checks; if (!condition) { ++failures; std::fprintf(stderr,"FAILED: %s\n",label); }
  };
  // Replay the supported guest's recovery decision with measured work
  // intervals. A slow frame must retain its actual delta; only artificial
  // subsequent holds disappear. Reference policy keeps all 31 holds.
  const auto recover = [](bool native, const std::vector<unsigned>& work) {
    std::uint8_t interval = 2, counter = 0;
    std::vector<unsigned> elapsed;
    for (unsigned measured : work) {
      counter = jfg::boot::native_frame_recovery_counter(native,interval,counter);
      if (interval == 3) {
        if (measured < 3) { if (++counter > 30) interval = 2; }
        else counter = 0;
      } else if (measured >= 3) { interval = 3; counter = 0; }
      elapsed.push_back((std::max)(measured,unsigned(interval)));
    }
    return elapsed;
  };
  const std::vector<unsigned> burst{2,3,2,2,5,2,2};
  check(recover(true,burst)==std::vector<unsigned>({2,3,2,2,5,2,2}),"native recovery keeps real overload and recovers on the next fast frame");
  check(recover(false,burst)==std::vector<unsigned>({2,3,3,3,5,3,3}),"reference recovery remains sticky");
  check(recover(true,{3,3,3,4,4})==std::vector<unsigned>({3,3,3,4,4}),"sustained overload is never forced to 30 Hz");
  for(unsigned interval=1;interval<=6;++interval)
    for(unsigned counter=0;counter<256;++counter) {
      check(jfg::boot::native_frame_recovery_counter(false,static_cast<std::uint8_t>(interval),static_cast<std::uint8_t>(counter))==counter,"reference counter unchanged");
      if(interval!=3)check(jfg::boot::native_frame_recovery_counter(true,static_cast<std::uint8_t>(interval),static_cast<std::uint8_t>(counter))==counter,"other guest intervals unchanged");
    }
  const auto origin = HostCountClock::Time{}+7h;
  HostCountClock clock;
  check(!clock.sample(origin),"sampling before initialization fails");
  check(!clock.deadline(1,origin),"waiting before initialization fails");
  check(clock.start(0xfffffff0ULL,origin),"clock starts at guest boot count");
  check(!clock.start(0,origin),"start cannot reset a running clock");
  check(clock.sample(origin+21ns)==0xfffffff0ULL,"fractional Count tick retained");
  check(clock.sample(origin+22ns)==0xfffffff1ULL,"first Count tick rounds down");
  check(clock.sample(origin+64ns)==0xfffffff3ULL,"exact 3/64 Count ratio");
  check(clock.sample(origin+1s)==0xfffffff0ULL+46875000ULL,"one second uses hardware Count units");
  check(clock.sample(origin+1s-1ns)==0xfffffff0ULL+46875000ULL,"backward sample never rolls Count back");
  check(clock.sample(origin+24h)==0xfffffff0ULL+4050000000000ULL,"64-bit Count survives 32-bit wrap and long session");
  check(clock.pause(origin+24h),"pause samples and freezes Count");
  const auto paused = clock.sample(origin+24h+10s);
  check(paused==0xfffffff0ULL+4050000000000ULL,"paused desktop time does not reach guest");
  check(!clock.deadline(*paused+1,origin+24h+10s),"paused clock cannot schedule a wait");
  check(clock.resume(origin+24h+10s),"resume keeps frozen count");
  check(clock.sample(origin+24h+11s)==*paused+46875000ULL,"resume excludes entire pause interval");
  check(!clock.resume(origin+24h+12s),"unpaired resume is rejected");
  for (int ns = 0; ns < 64; ++ns) {
    HostCountClock c; (void)c.start(100,origin);
    const auto now = origin+std::chrono::nanoseconds(ns);
    const auto current = c.sample(now);
    const auto due = c.deadline(*current+783520U,now);
    check(due && *due>now && c.sample(*due)>=*current+783520U,"VI wait is never early across fractional phases");
    check(c.deadline(*current,now)==now,"overdue device does not wait");
  }
  HostCountClock overflow; (void)overflow.start(UINT64_MAX-2U,origin);
  check(!overflow.sample(origin+64ns),"Count overflow is rejected");
  HostCountClock far; (void)far.start(0,origin);
  check(!far.deadline(UINT64_MAX,origin),"unrepresentable deadline is rejected");
  HostCountClock cadence; (void)cadence.start(0,origin);
  for (int i=1;i<=6000;++i) (void)cadence.sample(origin+std::chrono::microseconds(i*10000LL));
  check(cadence.sample(origin+60s)==2812500000ULL,"sampling cadence cannot accumulate drift");
  check(cadence.sample(origin+60300ms)==2826562500ULL,"elapsed time is independent of instruction count");
  std::printf("%u host Count clock checks, %u failures\n",checks,failures);
  return failures==0 ? 0 : 1;
}
