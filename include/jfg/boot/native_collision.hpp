#pragma once
#include <cstdint>

struct recomp_context;

namespace jfg::boot {
using NativeCollisionFunction = void(std::uint8_t*, recomp_context*);
struct NativeCollisionCounters {
  std::uint64_t native_nodes=0, native_edges=0, fallbacks=0,
      expired=0, invalid=0, max_nanoseconds=0;
};

#if defined(JFG_NATIVE_COLLISION_ENABLED)
inline constexpr bool native_collision_available() { return true; }
bool native_collision_in_fallback() noexcept;
NativeCollisionCounters native_collision_counters() noexcept;
void native_collision_reset_max() noexcept;
// Executes the complete operation, delegating unsupported nodes to reference.
// Returns whether any node used native execution. The caller must refresh
// elapsed clocks/events before its next generated instruction when true.
// Smaller limits support qualification; larger arguments are capped here.
bool native_collision_execute(NativeCollisionFunction* reference,
    std::uint8_t* ram, recomp_context* cpu,
    unsigned max_nodes=32U, unsigned max_edges=128U);
#else
inline constexpr bool native_collision_available() { return false; }
inline bool native_collision_in_fallback() noexcept { return false; }
inline NativeCollisionCounters native_collision_counters() noexcept { return {}; }
inline void native_collision_reset_max() noexcept {}
inline bool native_collision_execute(NativeCollisionFunction* reference,
    std::uint8_t* ram, recomp_context* cpu, unsigned=32U, unsigned=128U) {
  reference(ram,cpu);
  return false;
}
#endif
}
