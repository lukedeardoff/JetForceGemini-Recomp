#pragma once
#include <cstdint>
#include <optional>

namespace jfg::boot {
struct NativePeriodicDeadline {
  std::uint64_t next = 0;
  std::uint64_t elapsed = 0;
};
// A level-latched periodic device has one pending interrupt, not a queue of
// historical edges. Preserve its phase while advancing beyond all elapsed edges.
// This advances the device schedule only; it never changes Count or game work.
constexpr std::optional<NativePeriodicDeadline> advance_native_periodic_deadline(
    std::uint64_t now, std::uint64_t next, std::uint64_t period) noexcept {
  if (period == 0) return std::nullopt;
  if (now < next) return NativePeriodicDeadline{next, 0};
  const auto missed = (now-next)/period;
  if (missed == UINT64_MAX) return std::nullopt;
  const auto elapsed = missed+1;
  if (elapsed > (UINT64_MAX-next)/period) return std::nullopt;
  return NativePeriodicDeadline{next+elapsed*period, elapsed};
}
// Phase remains valid even before an overdue interrupt is serviced. Modular
// subtraction avoids an unsigned underflow before the first full period.
constexpr std::uint64_t native_periodic_phase(
    std::uint64_t now, std::uint64_t next, std::uint32_t period) noexcept {
  if (period == 0) return 0;
  return (now%period+period-next%period)%period;
}
} // namespace jfg::boot
