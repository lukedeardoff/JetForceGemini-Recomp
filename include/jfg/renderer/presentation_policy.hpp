#pragma once
#include <atomic>
#include <cstdint>
namespace jfg {
// Immutable for a runtime session; apply settings by starting a new session.
inline std::atomic<unsigned> &presentation_policy_bits() {
  static std::atomic<unsigned> bits{1U};
  return bits;
}
inline void configure_presentation_policy(bool vsync, bool unlimited) {
  presentation_policy_bits().store((vsync ? 1U : 0U) | (unlimited ? 2U : 0U));
}
inline bool presentation_vsync() {
  return (presentation_policy_bits().load() & 1U) != 0;
}
inline bool presentation_unlimited() {
  return presentation_policy_bits().load() == 2U;
}
inline bool presentation_unlimited_requested() {
  return (presentation_policy_bits().load() & 2U) != 0;
}
// Sentinels, not FPS limits. Uncapped output uses a two-target
// producer/consumer ring and samples interpolation from monotonic time within
// each source frame.
// The active RT64 capture owner requests an actual draw even when its VI
// refers to a scene whose interpolation sequence was already consumed.
inline std::atomic<bool>& presentation_capture_pending() {
  static std::atomic<bool> pending{false};
  return pending;
}
inline constexpr std::uint32_t kUnlimitedPresentation = 0x7fffffffU;
inline constexpr std::uint32_t kPresentationRingSize = 2U;
inline unsigned presentation_target(unsigned requested, unsigned display) {
  if (presentation_unlimited())
    return kUnlimitedPresentation;
  if (presentation_unlimited_requested())
    return display ? display : 60U;
  return presentation_vsync() && display && requested > display ? display
                                                                : requested;
}
} // namespace jfg
