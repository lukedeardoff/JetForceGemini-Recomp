#pragma once
#include "jfg/runtime/controller_mapping.hpp"
#include <array>
#include <cstdint>
namespace jfg {
// -1 automatic (sticky device), -2 disconnected, -3 keyboard; 0..3 physical
// slots. Resolved SDL identities use slots 4 and above in a host catalog.
// Availability for an explicit UI selection. Resolved automatic assignments
// reserve their device too; refreshing discovery never changes saved mappings.
template<std::size_t N=4>
inline bool controller_device_available(
    std::size_t player, int device,
    const std::array<ControllerMapping, 4> &mappings,
    const std::array<int, 4> &assigned,
    const std::array<bool, N> &connected) {
  if (player >= mappings.size() || device < -3 || device >= static_cast<int>(N))
    return false;
  if (device == -1 || device == -2)
    return true;
  if (device >= 0 && !connected[static_cast<std::size_t>(device)])
    return false;
  for (std::size_t p = 0; p < mappings.size(); ++p)
    if (p != player && (mappings[p].device == device || assigned[p] == device))
      return false;
  return true;
}
struct ControllerPortSample {
  std::uint16_t buttons = 0;
  std::int8_t x = 0, y = 0;
  bool connected = false;
};
// Physical input remains neutral until all players release controls after a
// menu or focus change. Connectivity is independent of input capture.
class ControllerInputGate {
public:
  bool blocked(bool captured, const std::array<ControllerPortSample, 4> &raw) {
    if (captured)
      waiting_ = true;
    else if (waiting_) {
      bool neutral = true;
      for (const auto &pad : raw)
        if (pad.buttons || pad.x || pad.y)
          neutral = false;
      if (neutral)
        waiting_ = false;
    }
    return captured || waiting_;
  }

private:
  bool waiting_ = false;
};
class ControllerPorts {
public:
  std::array<ControllerMapping, 4> mappings{};
  std::array<ControllerPortSample, 4> samples{};
  void configure(std::size_t p, const ControllerMapping &m) {
    if (mappings[p].device != m.device || mappings[p].device_key != m.device_key)
      assigned_[p] = -1;
    mappings[p] = m;
  }
  template<std::size_t N=4>
  std::array<int, 4> devices(const std::array<bool, N> &connected) {
    std::array<int, 4> result{-2, -2, -2, -2};
    std::array<bool, N> used{};
    bool keyboard = false;
    for (std::size_t p = 0; p < 4; ++p) {
      int d = mappings[p].device;
      if (d >= 0 && d < static_cast<int>(N) && !used[static_cast<std::size_t>(d)]) {
        result[p] = d;
        used[static_cast<std::size_t>(d)] = true;
      } else if (d == -3 && !keyboard) {
        result[p] = -3;
        keyboard = true;
      }
    }
    for (std::size_t p = 0; p < 4; ++p)
      if (mappings[p].device == -1) {
        int &d = assigned_[p];
        if (d >= static_cast<int>(N) || (d >= 0 && used[static_cast<std::size_t>(d)]))
          d = -1;
        if (d < 0)
          for (int c = 0; c < static_cast<int>(N); ++c)
            if (connected[static_cast<std::size_t>(c)] &&
                !used[static_cast<std::size_t>(c)]) {
              d = c;
              break;
            }
        if (d >= 0) {
          result[p] = d;
          used[static_cast<std::size_t>(d)] = true;
        } else if (p == 0 && !keyboard) {
          result[p] = -3;
          keyboard = true;
        }
      }
    return result;
  }
  std::uint8_t mask() const {
    std::uint8_t m = 0;
    for (std::size_t p = 0; p < 4; ++p)
      if (samples[p].connected)
        m |= static_cast<std::uint8_t>(1U << p);
    return m;
  }

private:
  std::array<int, 4> assigned_{-1, -1, -1, -1};
};
} // namespace jfg
