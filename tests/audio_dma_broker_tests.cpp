#include "jfg/runtime/audio_dma_broker.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <source_location>

namespace {
void require(bool value,
    const std::source_location where = std::source_location::current()) {
    if (!value) {
        std::fprintf(stderr, "audio DMA assertion failed at line %u\n", where.line());
        std::abort();
    }
}
struct Memory final : jfg::AudioMemoryAccess {
    std::array<std::byte, 8192> ram{};
    std::array<std::byte, 8192> output{};
    bool deny_read = false, deny_write = false, deny_output = false;
    std::size_t reads = 0, writes = 0;
    bool read(std::size_t region, std::size_t offset,
              std::span<std::byte> target) noexcept override {
        ++reads;
        if (deny_read || region != 0 || offset > ram.size() ||
            target.size() > ram.size() - offset) return false;
        std::copy_n(ram.begin() + static_cast<std::ptrdiff_t>(offset),
                    target.size(), target.begin());
        return true;
    }
    bool write_region(std::size_t region, std::size_t offset,
                      std::span<const std::byte> source) noexcept override {
        ++writes;
        if (deny_write || region != 0 || offset > ram.size() ||
            source.size() > ram.size() - offset) return false;
        std::copy(source.begin(), source.end(),
                  ram.begin() + static_cast<std::ptrdiff_t>(offset));
        return true;
    }
    bool write_output(std::size_t offset,
                      std::span<const std::byte> source) noexcept override {
        if (deny_output || offset > output.size() ||
            source.size() > output.size() - offset) return false;
        std::copy(source.begin(), source.end(),
                  output.begin() + static_cast<std::ptrdiff_t>(offset));
        return true;
    }
};
struct Fixture {
    Memory memory;
    std::array<std::uint8_t, 4096> dmem{};
    jfg::audio_dma::BrokerState state{&memory};
    Fixture() { dmem.fill(0xAA); memory.ram.fill(std::byte{0x55}); }
    std::uint8_t byte(std::size_t index) const { return dmem[index ^ 3U]; }
};
}
int main() {
    // Failing reverb request: the last sample must arrive with the burst.
    {
        Fixture f;
        f.memory.ram[0x100 + 366] = std::byte{2};
        f.memory.ram[0x100 + 367] = std::byte{0x7C}; // sample 636
        jfg::audio_dma::dma_read(&f.state, f.dmem, 0x730, 0x100, 365);
        require(!f.state.violation && f.state.input_bytes == 368);
        require(f.byte(0x89E) == 2 && f.byte(0x89F) == 0x7C);
        require(f.byte(0x8A0) == 0xAA && f.byte(0x72F) == 0xAA);
    }
    {
        Fixture f;
        f.memory.ram[0x100] = std::byte{0x12};
        jfg::audio_dma::dma_read(&f.state, f.dmem, 0x37, 0x107, 0);
        require(f.byte(0x30) == 0x12 && f.byte(0x37) == 0x55);
        require(f.byte(0x38) == 0xAA && f.state.input_bytes == 8);
        f.state.output_address = 0x200; f.state.output_size = 8;
        jfg::audio_dma::dma_write(&f.state, f.dmem, 0x37, 0x207, 0);
        require(!f.state.violation && f.state.region_write_bytes == 8);
        require(f.state.output_bytes == 8);
        require(f.memory.ram[0x200] == std::byte{0x12});
        require(f.memory.output[7] == std::byte{0x55});
    }
    // A maximum-length transfer wraps within DMEM, never into IMEM.
    {
        Fixture f;
        for (std::size_t i = 0; i < 4096; ++i)
            f.memory.ram[i] = static_cast<std::byte>(i & 255);
        jfg::audio_dma::dma_read(&f.state, f.dmem, 0xFF8, 0, 4095);
        require(!f.state.violation && f.state.input_bytes == 4096);
        require(f.byte(0xFF8) == 0 && f.byte(0) == 8 && f.byte(0xFF7) == 255);
        jfg::audio_dma::dma_write(&f.state, f.dmem, 0xFF8, 4096, 4095);
        require(!f.state.violation && f.state.region_write_bytes == 4096);
        require(std::equal(f.memory.ram.begin(), f.memory.ram.begin() + 4096,
                           f.memory.ram.begin() + 4096));
    }
    {
        Fixture f;
        f.dmem.fill(0x63);
        jfg::audio_dma::dma_write(&f.state, f.dmem, 0, 0x100, 365);
        require(f.state.region_write_bytes == 368);
        require(f.memory.ram[0x100 + 367] == std::byte{0x63});
        require(f.memory.ram[0x100 + 368] == std::byte{0x55});
    }
    for (const unsigned length : {0x1000U, 0x100000U, 0xFFFFFFFFU}) {
        Fixture f;
        jfg::audio_dma::dma_read(&f.state, f.dmem, 0, 0, length);
        require(f.state.violation && f.memory.reads == 0);
        jfg::audio_dma::dma_write(&f.state, f.dmem, 0, 0, length);
        require(f.state.write_violation && f.memory.writes == 0);
    }
    {
        Fixture f;
        jfg::audio_dma::dma_read(&f.state, f.dmem, 0x1000, 0, 0);
        require(f.state.violation && f.memory.reads == 0);
        jfg::audio_dma::dma_write(&f.state, f.dmem, 0x1000, 0, 0);
        require(f.state.write_violation && f.memory.writes == 0);
    }
    {
        Fixture f;
        f.memory.deny_read = true;
        const auto before = f.dmem;
        jfg::audio_dma::dma_read(&f.state, f.dmem, 0, 0, 365);
        require(f.state.violation && f.state.input_operations == 0);
        require(f.dmem == before);
        f.memory.deny_write = true;
        jfg::audio_dma::dma_write(&f.state, f.dmem, 0, 0, 365);
        require(f.state.write_violation && f.state.region_write_operations == 0);
    }
    {
        Fixture f;
        f.state.output_size = 8; f.memory.deny_output = true;
        jfg::audio_dma::dma_write(&f.state, f.dmem, 0, 0, 0);
        require(f.state.violation && f.state.write_violation);
        require(f.state.output_operations == 0);
        require(f.state.region_write_operations == 1);
    }
    std::puts("Audio DMA broker regressions passed.");
}
