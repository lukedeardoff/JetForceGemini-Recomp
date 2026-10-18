#include <algorithm>
#include <cstdio>
#include <vector>
#include "jfg/boot/host_deadline_wait.hpp"
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;
int main() {
#if defined(_WIN32)
  unsigned failures = 0, checks = 0;
  const auto check = [&](bool condition, const char* label) {
    ++checks; if (!condition) { ++failures; std::fprintf(stderr, "FAILED: %s\n", label); }
  };
  DWORD before = 0, after = 0;
  check(GetProcessHandleCount(GetCurrentProcess(), &before) != 0, "read initial handle count");
  std::vector<double> overshoot;
  for (unsigned lifetime = 0; lifetime < 8; ++lifetime) {
    jfg::boot::HostDeadlineWait waiter;
    check(waiter.until(Clock::now()-1s), "past deadline returns successfully");
    for (auto interval : {10us, 100us, 1000us, 2000us}) {
      const auto due = Clock::now()+interval;
      check(waiter.until(due), "waitable timer completes");
      const auto now = Clock::now();
      check(now >= due, "completion never precedes deadline");
      overshoot.push_back(std::chrono::duration<double, std::micro>(now-due).count());
    }
    check(waiter.until(Clock::now()), "reused timer accepts elapsed deadline");
  }
  check(GetProcessHandleCount(GetCurrentProcess(), &after) != 0, "read final handle count");
  check(after == before, "timer handles close on destruction");
  std::sort(overshoot.begin(), overshoot.end());
  std::printf("%u deadline wait checks, %u failures; wake overshoot median %.3f us, max %.3f us\n",
      checks, failures, overshoot[overshoot.size()/2], overshoot.back());
  return failures ? 1 : 0;
#else
  return 0;
#endif
}
