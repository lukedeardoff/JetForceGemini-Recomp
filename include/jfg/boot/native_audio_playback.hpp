#pragma once
#include <cstdint>
#include <optional>

namespace jfg::boot {
// Native playback follows the host device's sample demand. The original audio
// manager still builds every sample, using its existing buffers and frequency.
// One countdown adjustment can request at most one task per delivered retrace.
constexpr std::uint64_t native_audio_target_bytes(std::uint32_t frequency) {
  return frequency>=8000U && frequency<=96000U
      ? std::uint64_t(frequency)*4U*85U/1000U : 0U;
}
constexpr std::optional<std::uint32_t> native_audio_countdown(
    std::uint32_t word, std::uint32_t frames, std::uint32_t mode,
    std::uint32_t pending_notifications, std::uint32_t ai_status,
    std::uint32_t frequency, std::uint64_t buffered_bytes) {
  const auto target=native_audio_target_bytes(frequency);
  if(!target || buffered_bytes>=target || (word>>24U)!=2U || frames!=2U ||
      mode!=0U || pending_notifications!=0U || (ai_status&0x80000000U) ||
      (ai_status&~0xC0000000U)) return std::nullopt;
  // US gNextFrameCount is a signed byte; adjacent bytes are unrelated.
  return (word&0x00FFFFFFU)|0x01000000U;
}
} // namespace jfg::boot
