#pragma once

#include "jfg/runtime/audio_dma_broker.hpp"
#include "librecomp/rsp.hpp"

namespace private_audio {

using BrokerState = jfg::audio_dma::BrokerState;
inline thread_local BrokerState* active_broker = nullptr;

inline void dma_read(
    const std::uint32_t dmem_address,
    const std::uint32_t dram_address,
    const std::uint32_t length) noexcept {
    jfg::audio_dma::dma_read(active_broker,
        std::span<std::uint8_t, 0x1000U>(dmem, 0x1000U),
        dmem_address, dram_address, length);
}

inline void dma_write(
    const std::uint32_t dmem_address,
    const std::uint32_t dram_address,
    const std::uint32_t length) noexcept {
    jfg::audio_dma::dma_write(active_broker,
        std::span<std::uint8_t, 0x1000U>(dmem, 0x1000U),
        dmem_address, dram_address, length);
}

}  // namespace private_audio

#undef DO_DMA_READ
#undef DO_DMA_WRITE
#define DO_DMA_READ(rd_len) \
    private_audio::dma_read(dma_mem_address, dma_dram_address, (rd_len))
#define DO_DMA_WRITE(wr_len) \
    private_audio::dma_write(dma_mem_address, dma_dram_address, (wr_len))
