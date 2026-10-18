#pragma once
#include <atomic>
namespace jfg {
// Optional instrumentation at the pinned RT64 synchronous framebuffer queue.
// Registration/removal require a quiescent submit owner. Rendering behavior and
// synchronization remain RT64's responsibility; observers must never wait.
enum class Rt64GpuTimingStage { begin, end, complete, workload_complete };
struct Rt64GpuTimingHook {
    void* owner = nullptr;
    void (*observe)(void*, Rt64GpuTimingStage, const void*, void*) noexcept = nullptr;
};
inline std::atomic<Rt64GpuTimingHook*> rt64_gpu_timing_hook{nullptr};
inline void observe_rt64_gpu_timing(Rt64GpuTimingStage stage,
                                   const void* state, void* commands) noexcept {
    if (auto* hook = rt64_gpu_timing_hook.load(std::memory_order_acquire))
        hook->observe(hook->owner, stage, state, commands);
}
} // namespace jfg
