/*
ares

Copyright (c) 2004-2021 ares team, Near et al

Permission to use, copy, modify, and/or distribute this software for any

purpose with or without fee is hereby granted, provided that the above

copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES

WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF

MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR

ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES

WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN

ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF

OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Cartridge DMA duration adapted from ares PI::dmaDuration, as vendored by
BizHawk bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5.
*/
#pragma once

// Retains the qualified register/data-visibility behavior while cartridge bus
// duration follows the pinned ares model below. Count is the VR4300 half-rate
// clock. This remains a timing model, not a complete hardware bus simulation.
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace jfg::boot {
class OriginalTimingPiDma final {
public:
  struct RomTransfers {
    std::uint64_t calls=0,bytes=0,bus_ticks=0,scheduled_ticks=0,native_calls=0;
  };
  using CartridgeTransfer = bool (*)(void*, bool, std::uint32_t, std::uint32_t,
                                     std::uint32_t, std::span<std::uint8_t>);
  [[nodiscard]] std::optional<std::uint32_t> read(std::uint32_t offset) const {
    if ((offset & 3U) != 0U || offset > 0x30U) return std::nullopt;
    if (offset == 0x10U) return deadline_ ? busy_status_ : 0U;
    return registers_[offset / 4U];
  }
  bool write(std::uint32_t offset, std::uint32_t value, std::uint64_t count,
             std::span<std::uint8_t> rdram, std::span<const std::uint8_t> rom,
             CartridgeTransfer flash = nullptr, void* device = nullptr,
             bool native_rom_completion = false) {
    if ((offset & 3U) != 0U || offset > 0x30U) return false;
    if (offset == 0x10U) {
      if ((value & ~3U) != 0U) return false;
      if ((value & 1U) != 0U) deadline_.reset();
      if ((value & 2U) != 0U) interrupt_ = false;
      return true;
    }
    if (offset != 8U && offset != 12U) { registers_[offset / 4U] = value; return true; }
    const auto length = std::uint64_t(value) + 1U;
    const auto dram = registers_[0];
    const auto cart = registers_[1];
    if (cart >= 0x08000000U && cart < 0x08010000U) {
      if (!flash || deadline_ || length > UINT32_MAX || count > UINT64_MAX - 4096U ||
          !flash(device, offset == 12U, cart, dram, static_cast<std::uint32_t>(length), rdram)) return false;
      registers_[offset / 4U] = value;
      deadline_ = count + 4096U;
      busy_status_ = 1U;
      ++transfers_;
      return true;
    }
    if (offset == 8U) return false; // only qualified flash writes
    // Cartridge bus timing follows the pinned Ares model. The ROM
    // contents, immediate memory visibility and register semantics are unchanged.
    const std::uint64_t bus_length = (std::uint64_t(value) | 1U) + 1U;
    const std::uint64_t page = std::uint64_t{1} << ((registers_[7] & 15U) + 2U);
    const std::uint64_t first = cart, last = first + bus_length - 2U;
    const std::uint64_t pages = last / page - first / page + 1U;
    std::uint64_t buffers = 0U, partial = 0U;
    if (first / page == last / page) {
      if (bus_length == 128U) buffers = 1U;
      else partial = bus_length;
    } else {
      if ((first & (page - 1U)) == 0U) ++buffers;
      else partial += page - (first & (page - 1U));
      if (((last + 2U) & (page - 1U)) == 0U) ++buffers;
      else partial += (last & (page - 1U)) + 2U;
      if (first / page + 1U < last / page) buffers += (pages - 2U) * page / 128U;
    }
    const auto cycles = (15U + (registers_[5] & 255U)) * pages +
        ((registers_[6] & 255U) + (registers_[8] & 3U) + 2U) * (bus_length / 2U) +
        buffers * 28U + partial;
    const auto numerator = cycles * 3U + bus_fraction_;
    // The host ROM copy below is synchronous. Native play keeps PI completion
    // asynchronous but does not additionally wait for an emulated cartridge bus.
    // The reference path and the separate FlashRAM branch retain their timing.
    const auto duration = native_rom_completion ? std::uint64_t{1} : numerator / 4U;

    if (deadline_ || (dram & 1U) != 0U || (cart & 1U) != 0U ||
        (rdram.size() & 3U) != 0U || cart < 0x10000000U ||
        dram > rdram.size() || length > rdram.size() - dram ||
        cart - 0x10000000U > rom.size() ||
        length > rom.size() - (cart - 0x10000000U) ||
        count > UINT64_MAX - duration) return false;
    for (std::uint64_t index = 0; index < length; ++index)
      rdram[(dram + index) ^ 3U] = rom[cart - 0x10000000U + index];
    registers_[3] = value;
    deadline_ = count + duration;
    bus_fraction_ = numerator % 4U;
    ++rom_transfers_.calls;
    rom_transfers_.bytes+=length;
    rom_transfers_.bus_ticks+=numerator/4U;
    rom_transfers_.scheduled_ticks+=duration;
    if(native_rom_completion)++rom_transfers_.native_calls;
    busy_status_ = 3U;
    ++transfers_;
    return true;
  }
  // Called when the qualified CPU commits device events, not once per host
  // callback or wall-clock tick. Acknowledgement remains a register command.
  void advance(std::uint64_t count) {
    if (deadline_ && count >= *deadline_) {
      deadline_.reset();
      interrupt_ = true;
    }
  }
  [[nodiscard]] bool interrupt() const { return interrupt_; }
  [[nodiscard]] std::optional<std::uint64_t> deadline() const { return deadline_; }
  [[nodiscard]] std::uint64_t transfers() const { return transfers_; }
  [[nodiscard]] RomTransfers rom_transfers() const { return rom_transfers_; }
private:
  RomTransfers rom_transfers_{};
  std::array<std::uint32_t, 13> registers_{};
  std::optional<std::uint64_t> deadline_;
  std::uint64_t transfers_ = 0;
  std::uint64_t bus_fraction_ = 0;
  std::uint32_t busy_status_ = 0;
  bool interrupt_ = false;
};
} // namespace jfg::boot
