#pragma once
#include "jfg/boot/reference_event_commit.hpp"
#include <array>

namespace jfg::boot {
// Native elapsed-time policy. Equal timestamps are one device-observation
// boundary: latch all completions before allowing a guest interrupt. Source
// iteration order is reproducible but never exposed as CPU interrupt priority.
// The original emulator-reference selector remains a separate policy.
struct NativeEventBatch {
  bool valid = true;
  std::uint64_t deadline = UINT64_MAX;
  std::array<ReferenceEvent, 7> sources{};
  std::size_t size = 0;
};
inline NativeEventBatch select_native_event_batch(
    std::span<const ReferenceEventCandidate> candidates, std::uint64_t now) {
  NativeEventBatch result;
  unsigned seen = 0;
  for (const auto& event : candidates) {
    const auto source = static_cast<unsigned>(event.source);
    if (source >= 7U || (seen & (1U << source))) {
      result.valid = false; return result;
    }
    seen |= 1U << source;
    if (event.deadline <= now && event.deadline < result.deadline)
      result.deadline = event.deadline;
  }
  // Include UINT64_MAX if it is actually due; absence is distinguished by size.
  for (unsigned source = 0; source < 7U; ++source)
    for (const auto& event : candidates)
      if (static_cast<unsigned>(event.source) == source && event.deadline <= now &&
          event.deadline == result.deadline)
        result.sources[result.size++] = event.source;
  return result;
}
} // namespace jfg::boot
