#pragma once
#include "jfg/runtime/input_replay.hpp"
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace jfg::boot {
// Qualification replay of a live poll recording at its recorded VI times.
// Held controls persist between polls; mouse deltas and menu selections are
// consumed once. The existing poll-index replay remains the reference oracle.
class RecordedInputTimeline {
public:
  std::optional<ControllerReplaySample> sample(
      std::uint64_t retrace, std::span<const InputReplayEvent> events) {
    if (started_ && retrace < previous_retrace_) return std::nullopt;
    started_ = true; previous_retrace_ = retrace;
    std::int64_t mouse_x = 0, mouse_y = 0;
    std::uint16_t pressed = 0;
    std::uint8_t menu_kind = 0, menu_target = 0;
    while (next_ < events.size() && events[next_].first_retrace <= retrace) {
      const auto& event = events[next_++];
      // Live recorder samples occupy exactly one VI interval. Reject scripted
      // held ranges, whose look fields may represent repeated rate inputs.
      if (event.end_retrace-event.first_retrace != 1U) return std::nullopt;
      pressed |= static_cast<std::uint16_t>(event.sample.buttons & ~held_.buttons);
      held_ = event.sample;
      if (held_.pc_mode == 3U) { mouse_x += held_.look_x; mouse_y += held_.look_y; }
      if (held_.menu_target != 0U) {
        if (menu_target != 0U && (menu_kind != held_.menu_kind || menu_target != held_.menu_target))
          return std::nullopt; // Cannot silently merge two different menu clicks.
        menu_kind = held_.menu_kind; menu_target = held_.menu_target;
      }
    }
    auto result = held_;
    // Preserve a short press released between consumer polls for one sample.
    // This matches the edge mailbox used for physical mouse/key input.
    result.buttons |= pressed;
    result.menu_kind = menu_kind; result.menu_target = menu_target;
    if (result.pc_mode == 3U) {
      if (mouse_x < INT16_MIN || mouse_x > INT16_MAX || mouse_y < INT16_MIN || mouse_y > INT16_MAX)
        return std::nullopt; // A stalled consumer must not silently clip motion.
      result.look_x = static_cast<std::int16_t>(mouse_x);
      result.look_y = static_cast<std::int16_t>(mouse_y);
    }
    return result;
  }
private:
  ControllerReplaySample held_{};
  std::size_t next_ = 0;
  std::uint64_t previous_retrace_ = 0;
  bool started_ = false;
};
} // namespace jfg::boot
