#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace jfg::boot {

// Identity lookup for the pinned game's 157 overlay records, in native-word
// RDRAM layout. The caller validates the entire source extent first. Compare
// every byte on every call, including all identities and allocation fields;
// rebuild on any change. This never caches an allocation or skips guest loads.
class NativeRunlinkIdentityIndex {
public:
  using Identity = std::array<std::uint32_t, 4>;
  static constexpr std::size_t kSlotCount = 157U;
  static constexpr std::size_t kRecordBytes = (kSlotCount - 1U) * 32U + 20U;

  // Records begin at overlay slot one. Zero means missing, duplicated, or an
  // invalid source extent. Full identity equality resolves all hash collisions.
  [[nodiscard]] std::uint32_t find(std::span<const std::uint8_t> records,
                                   const Identity &key) {
    if (records.size() != kRecordBytes)
      return 0U;
    if (!ready_ ||
        std::memcmp(records.data(), previous_.data(), kRecordBytes) != 0) {
      std::memcpy(previous_.data(), records.data(), kRecordBytes);
      // Derive the index from the same immutable bytes retained for comparison.
      entries_.fill({});
      for (std::uint32_t i = 0; i < kSlotCount; ++i) {
        Identity value;
        std::memcpy(value.data(), previous_.data() + i * 32U + 4U,
                    sizeof(value));
        auto slot = hash(value);
        while (entries_[slot].used && entries_[slot].key != value)
          slot = (slot + 1U) & 511U;
        auto &entry = entries_[slot];
        if (entry.used)
          entry.slot = 0U;
        else
          entry = {value, i + 1U, true};
      }
      ready_ = true;
    }
    auto slot = hash(key);
    while (entries_[slot].used) {
      if (entries_[slot].key == key)
        return entries_[slot].slot;
      slot = (slot + 1U) & 511U;
    }
    return 0U;
  }

private:
  struct Entry {
    Identity key{};
    std::uint32_t slot = 0U;
    bool used = false;
  };
  static std::size_t hash(const Identity &key) {
    std::uint32_t value = 2166136261U;
    for (const auto component : key) {
      value ^= component;
      value *= 16777619U;
    }
    value ^= value >> 16U;
    value *= 0x7feb352dU;
    value ^= value >> 15U;
    return value & 511U;
  }
  std::array<std::uint8_t, kRecordBytes> previous_{};
  std::array<Entry, 512> entries_{};
  bool ready_ = false;
};

} // namespace jfg::boot
