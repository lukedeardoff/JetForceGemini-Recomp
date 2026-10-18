#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "jfg/runtime/audio_task_bridge.hpp"

namespace jfg::audio_dma {

struct BrokerState {
    jfg::AudioMemoryAccess* memory = nullptr;
    std::uint32_t output_address = 0U;
    std::size_t output_size = 0U;
    std::size_t input_operations = 0U;
    std::size_t input_bytes = 0U;
    std::size_t region_write_operations = 0U;
    std::size_t region_write_bytes = 0U;
    std::size_t output_operations = 0U;
    std::size_t output_bytes = 0U;
    bool violation = false;
    bool write_violation = false;
    std::uint32_t first_violation_address = 0U;
};

inline void dma_read(
    BrokerState* active_broker,
    const std::span<std::uint8_t, 0x1000U> dmem,
    std::uint32_t dmem_address,
    std::uint32_t dram_address,
    std::uint32_t length) noexcept {
    // SP DMA rounds the inclusive length register up to whole eight-byte
    // bursts. A request for 366 bytes transfers 368, including the last sample.
    // This audio broker supports single-row DMEM DMA only; reject count/skip
    // fields and IMEM transfers explicitly instead of silently discarding them.
    const bool unsupported = (length & ~0xFFFU) != 0U ||
        (dmem_address & ~0xFFFU) != 0U;
    length = (length | 7U) + 1U;
    dmem_address &= 0xFF8U;
    dram_address &= 0xFFFFF8U;
    if (active_broker == nullptr || active_broker->memory == nullptr ||
        unsupported) {
        if (active_broker != nullptr) {
            active_broker->violation = true;
            active_broker->first_violation_address = dram_address;
        }
        return;
    }

    std::array<std::byte, 0x1000U> bytes{};
    const std::span<std::byte> transfer(bytes.data(), length);
    if (!active_broker->memory->read(0U, dram_address & 0x7FFFFFU, transfer)) {
        active_broker->violation = true;
        active_broker->first_violation_address = dram_address;
        return;
    }
    ++active_broker->input_operations;
    active_broker->input_bytes += length;
    for (std::uint32_t index = 0U; index < length; ++index) {
        dmem[((dmem_address + index) & 0xFFFU) ^ 3U] =
            static_cast<std::uint8_t>(bytes[index]);
    }
}

inline void dma_write(
    BrokerState* active_broker,
    const std::span<std::uint8_t, 0x1000U> dmem,
    std::uint32_t dmem_address,
    std::uint32_t dram_address,
    std::uint32_t length) noexcept {
    // SP DMA rounds the inclusive length register up to whole eight-byte
    // bursts. A request for 366 bytes transfers 368, including the last sample.
    // This audio broker supports single-row DMEM DMA only; reject count/skip
    // fields and IMEM transfers explicitly instead of silently discarding them.
    const bool unsupported = (length & ~0xFFFU) != 0U ||
        (dmem_address & ~0xFFFU) != 0U;
    length = (length | 7U) + 1U;
    dmem_address &= 0xFF8U;
    dram_address &= 0xFFFFF8U;
    if (active_broker == nullptr || active_broker->memory == nullptr ||
        unsupported) {
        if (active_broker != nullptr) {
            active_broker->violation = true;
            active_broker->write_violation = true;
            active_broker->first_violation_address = dram_address;
        }
        return;
    }

    std::array<std::byte, 0x1000U> bytes{};
    for (std::uint32_t index = 0U; index < length; ++index) {
        bytes[index] = static_cast<std::byte>(dmem[((dmem_address + index) & 0xFFFU) ^ 3U]);
    }
    const std::span<const std::byte> transfer(bytes.data(), length);
    if (!active_broker->memory->write_region(
            0U, dram_address & 0x7FFFFFU, transfer)) {
        active_broker->violation = true;
        active_broker->write_violation = true;
        active_broker->first_violation_address = dram_address;
        return;
    }
    ++active_broker->region_write_operations;
    active_broker->region_write_bytes += length;

    if (dram_address >= active_broker->output_address) {
        const std::size_t output_offset = static_cast<std::size_t>(
            dram_address - active_broker->output_address);
        if (output_offset < active_broker->output_size &&
            length <= active_broker->output_size - output_offset) {
            if (!active_broker->memory->write_output(output_offset, transfer)) {
                active_broker->violation = true;
                active_broker->write_violation = true;
                active_broker->first_violation_address = dram_address;
                return;
            }
            ++active_broker->output_operations;
            active_broker->output_bytes += length;
        }
    }
}

}  // namespace jfg::audio_dma
