#pragma once
#include <chrono>
#if defined(_WIN32)
#include <windows.h>
#endif
namespace jfg::boot {
// Wait for real deadlines without adding the coarse Sleep timer quantum.
// Recheck steady time after every wake; a wake does not prove a device is due.
class HostDeadlineWait {
public:
  HostDeadlineWait() = default;
  HostDeadlineWait(const HostDeadlineWait&) = delete;
  HostDeadlineWait& operator=(const HostDeadlineWait&) = delete;
  ~HostDeadlineWait() {
#if defined(_WIN32)
    if (timer_) CloseHandle(timer_);
#endif
  }
  bool until(std::chrono::steady_clock::time_point target) noexcept {
#if defined(_WIN32)
    if (!timer_) timer_ = CreateWaitableTimerExW(nullptr, nullptr,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_MODIFY_STATE | SYNCHRONIZE);
    if (!timer_) return false;
    for (;;) {
      const auto now = std::chrono::steady_clock::now();
      if (now >= target) return true;
      const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(target-now).count();
      LARGE_INTEGER due{};
      // Negative means relative; round up to avoid requesting an early wake.
      due.QuadPart = -(ns/100 + (ns%100 != 0));
      if (!SetWaitableTimerEx(timer_, &due, 0, nullptr, nullptr, nullptr, 0) ||
          WaitForSingleObject(timer_, INFINITE) != WAIT_OBJECT_0) return false;
    }
#else
    (void)target; return false;
#endif
  }
private:
#if defined(_WIN32)
  HANDLE timer_ = nullptr;
#endif
};
} // namespace jfg::boot
