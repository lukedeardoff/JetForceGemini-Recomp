#pragma once
#include <cstdint>

namespace jfg::boot {
// The console waits for 31 consecutive fast frames before leaving 20 Hz.
// Native elapsed timing should recover on the next genuinely fast frame.
// viFrameSync still measures elapsed VIs, rejects late frames, performs its
// normal waits and returns the actual elapsed interval to game simulation.
constexpr std::uint8_t native_frame_recovery_counter(bool native_elapsed,
    std::uint8_t interval, std::uint8_t counter) noexcept {
  return native_elapsed && interval == 3U && counter < 30U ? 30U : counter;
}
} // namespace jfg::boot
