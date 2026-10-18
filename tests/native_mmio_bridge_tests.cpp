#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "recomp.h"
#include "jfg/runtime/cpu_operation_bridge.h"
#include <array>
#include <cstdint>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#endif
extern "C" std::size_t jfg_generated_section_count(void) { return 1; }
extern "C" int jfg_generated_initialize_sections(int32_t* addresses, std::size_t count) {
    if (!count) return 0;
    addresses[0] = 0;
    return 1;
}
namespace {
struct Owner { unsigned calls = 0; bool accept = true, lifetime = true; std::uint32_t written = 0; };
int callback(void* opaque, void* context, std::uint32_t operation,
             std::uint32_t selector, std::uint64_t* value) {
    auto& owner = *static_cast<Owner*>(opaque);
    ++owner.calls;
    owner.lifetime = owner.lifetime &&
        jfg_minimal_runtime_unbind_cpu(callback, opaque) == 0;
    if (!context || selector != 0xa430000cU) return 0;
    if (operation == JFG_CPU_READ_MI_MASK) *value = 0x12345678U;
    else if (operation == JFG_CPU_WRITE_MI_MASK) {
        owner.written = static_cast<std::uint32_t>(*value);
        *value = 0xdeadbeefU;
    } else return 0;
    return owner.accept ? 1 : 0;
}
}
int main() {
    alignas(8) std::array<std::uint8_t, 64> ram{};
    recomp_context context{};
    Owner owner;
    constexpr std::uint64_t cached = UINT64_C(0xffffffff80000010);
    constexpr std::uint64_t mask = UINT64_C(0xffffffffa430000c);
    jfg_mmio_mask_write32(ram.data(), &context, cached, 0x87654321U);
    if (static_cast<std::uint32_t>(jfg_mmio_mask_read32(ram.data(), &context, cached)) != 0x87654321U) return 1;
    if (!jfg_minimal_runtime_bind_cpu(callback, &owner)) return 2;
    if (jfg_mmio_mask_read32(ram.data(), &context, mask) != 0x12345678) return 3;
    jfg_mmio_mask_write32(ram.data(), &context, mask, 0xfedcba98U);
    if (owner.calls != 2 || owner.written != 0xfedcba98U || !owner.lifetime) return 4;
    if (static_cast<std::uint32_t>(jfg_mmio_mask_read32(ram.data(), &context, cached)) != 0x87654321U || owner.calls != 2) return 5;
#if defined(_WIN32)
    void* page = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!page) return 6;
    auto* mapped_word = reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(page) + 12U);
    *mapped_word = 0x76543210U;
    auto* mapping_base = reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(page) - 0x24300000U);
    owner.accept = false;
    if (jfg_mmio_mask_read32(mapping_base, &context, mask) != 0x76543210) return 7;
    jfg_mmio_mask_write32(mapping_base, &context, mask, 0xabcdef01U);
    if (*mapped_word != 0xabcdef01U) return 8;
    if (!jfg_minimal_runtime_unbind_cpu(callback, &owner)) return 9;
    if (static_cast<std::uint32_t>(jfg_mmio_mask_read32(mapping_base, &context, mask)) != 0xabcdef01U) return 10;
    jfg_mmio_mask_write32(mapping_base, nullptr, mask, 0x11223344U);
    if (*mapped_word != 0x11223344U) return 11;
    VirtualFree(page, 0, MEM_RELEASE);
#else
    if (!jfg_minimal_runtime_unbind_cpu(callback, &owner)) return 9;
#endif
    return owner.lifetime ? 0 : 12;
}
