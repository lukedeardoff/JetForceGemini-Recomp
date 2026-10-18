#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
namespace jfg {
inline std::uint64_t frame_clock_us() noexcept {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
// Registered before observation starts, cleared after the present queue joins.
// draw and presented run on the same present thread; the writer only sees samples.
struct FramePresentationObserver {
    virtual void presented(std::uint64_t time_us) noexcept = 0;
    virtual void finish() noexcept {}
};
struct FrameDiagnosticState {
    std::atomic<FramePresentationObserver*> observer{nullptr};
    void presented() noexcept {
        if (auto* sink = observer.load(std::memory_order_acquire))
            sink->presented(frame_clock_us());
    }

    void finish() noexcept {
        if(auto* sink=observer.exchange(nullptr,std::memory_order_acq_rel))
            sink->finish();
    }

    std::atomic<std::uint64_t> scenes{0}, scene_time{0}, vi{0};
    void scene(std::uint64_t tick) noexcept {
        // A source scene may submit multiple full-screen tasks (for example,
        // the boss motion-blur pass). Count the VI once; presentation remains
        // independently measured by the present observer. Scene submission is
        // serialized by the renderer, and initialization precedes that thread.
        const auto previous = vi.exchange(tick, std::memory_order_relaxed);
        if (scenes.load(std::memory_order_relaxed) != 0 && previous == tick) return;
        scene_time.store(frame_clock_us(), std::memory_order_relaxed);
        scenes.fetch_add(1, std::memory_order_relaxed);
    }
};
inline FrameDiagnosticState& frame_diagnostic_state() {
    static FrameDiagnosticState state;
    return state;
}
} // namespace jfg
