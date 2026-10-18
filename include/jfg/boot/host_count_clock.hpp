#pragma once
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>

namespace jfg::boot {
// Native execution uses elapsed steady time in the guest Count unit (46.875 MHz).
// This clock does not charge emulated instruction/cache penalties. The original
// instruction clock remains a separate policy for reference comparisons.
class HostCountClock {
public:
  using Clock = std::chrono::steady_clock;
  using Time = Clock::time_point;
  bool start(std::uint64_t count, Time now) noexcept {
    if (ready_) return false;
    origin_count_ = last_ = count; origin_ = now; ready_ = true; return true;
  }
  [[nodiscard]] bool ready() const noexcept { return ready_; }
  [[nodiscard]] std::optional<std::uint64_t> sample(Time now) noexcept {
    if (!ready_) return std::nullopt;
    if (paused_ || now <= origin_) return last_;
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now-origin_).count();
    // 3 Count ticks / 64 ns. Split before multiplying to avoid overflow.
    const auto ticks = static_cast<std::uint64_t>(ns / 64) * 3U +
                       static_cast<std::uint64_t>(ns % 64) * 3U / 64U;
    if (ticks > UINT64_MAX-origin_count_) return std::nullopt;
    const auto count = origin_count_+ticks;
    if (count > last_) last_ = count;
    return last_;
  }
  [[nodiscard]] std::optional<Time> deadline(std::uint64_t count, Time now) noexcept {
    const auto current = sample(now);
    if (!current || paused_) return std::nullopt;
    if (count <= *current) return now;
    const auto ticks = count-*current;
    if (ticks > static_cast<std::uint64_t>(INT64_MAX)/64U) return std::nullopt;
    // Round up: a wait must never report a device deadline before it is due.
    const auto ns = (ticks*64U+2U)/3U;
    const auto delta = std::chrono::nanoseconds(static_cast<std::int64_t>(ns));
    if (now > Time::max()-delta) return std::nullopt;
    return now+delta;
  }
  bool pause(Time now) noexcept {
    if (!sample(now)) return false;
    paused_ = true; return true;
  }
  bool resume(Time now) noexcept {
    if (!ready_ || !paused_) return false;
    origin_count_ = last_; origin_ = now; paused_ = false; return true;
  }
private:
  Time origin_{};
  std::uint64_t origin_count_ = 0, last_ = 0;
  bool ready_ = false, paused_ = false;
};
} // namespace jfg::boot
