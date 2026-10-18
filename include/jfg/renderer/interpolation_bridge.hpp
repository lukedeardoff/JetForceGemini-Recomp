#pragma once
// Build-tree RT64 adapters share immutable host metadata with the decoder.
#include "jfg/renderer/interpolation_history.hpp"
#include "jfg/renderer/presentation_policy.hpp"
#include "jfg/runtime/async_diagnostic_file.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <mutex>
#include <unordered_map>
namespace jfg {
struct PresentationSnapshots {
    std::mutex mutex;
    std::unordered_map<const void*, std::shared_ptr<const PresentationHistoryFrame>> frames;
};
inline PresentationSnapshots& presentation_snapshots() {
    static PresentationSnapshots snapshots;
    return snapshots;
}
inline void publish_presentation_snapshot(const void* owner,
        std::shared_ptr<const PresentationHistoryFrame> frame) {
    auto& storage = presentation_snapshots();
    const std::scoped_lock lock(storage.mutex);
    if (frame) storage.frames[owner] = std::move(frame);
    else storage.frames.erase(owner);
}
inline std::shared_ptr<const PresentationHistoryFrame> presentation_snapshot(const void* owner) {
    auto& storage = presentation_snapshots();
    const std::scoped_lock lock(storage.mutex);
    const auto found = storage.frames.find(owner);
    return found == storage.frames.end() ? nullptr : found->second;
}
inline bool interpolation_trace_enabled() {
    static const bool enabled = [] {
#if defined(_MSC_VER)
        char* path = nullptr; std::size_t size = 0;
        (void)_dupenv_s(&path, &size, "JFG_RT64_INTERPOLATION_TRACE");
        const bool result = path != nullptr && *path != 0;
        std::free(path); return result;
#else
        const char* path = std::getenv("JFG_RT64_INTERPOLATION_TRACE");
        return path != nullptr && *path != 0;
#endif
    }();
    return enabled;
}
struct PresentationOutputSample {
    std::uint64_t tick = 0;
    unsigned source = 0, frame = 0, count = 0;
    float alpha = 1.0f;
};
// Private capture qualification: select an actual output phase, rather than
// repeatedly capturing only the base image. Requires interpolation tracing.
inline PresentationOutputSample& current_capture_output() {
    static thread_local PresentationOutputSample sample; return sample;
}
inline int requested_capture_phase() {
    static const int phase = [] {
        int result = -1;
#if defined(_MSC_VER)
        char* value = nullptr; std::size_t size = 0;
        (void)_dupenv_s(&value, &size, "JFG_DIAGNOSTIC_CAPTURE_PHASE");
#else
        const char* value = std::getenv("JFG_DIAGNOSTIC_CAPTURE_PHASE");
#endif
        if (value && *value && interpolation_trace_enabled()) {
            const char* end = value + std::strlen(value); int parsed = -1;
            const auto conversion = std::from_chars(value, end, parsed);
            if (conversion.ec == std::errc{} && conversion.ptr == end &&
                parsed >= 0 && parsed < 64) result = parsed;
        }
#if defined(_MSC_VER)
        std::free(value);
#endif
        return result;
    }();
    return phase;
}
inline bool diagnostic_cpu_write_baseline() {
    static const bool baseline = [] {
#if defined(_MSC_VER)
        char* value = nullptr; std::size_t size = 0;
        (void)_dupenv_s(&value, &size, "JFG_DIAGNOSTIC_CPU_WRITE_BASELINE");
#else
        const char* value = std::getenv("JFG_DIAGNOSTIC_CPU_WRITE_BASELINE");
#endif
        const bool result = requested_capture_phase() >= 0 &&
            value && value[0] == '1' && value[1] == '\0';
#if defined(_MSC_VER)
        std::free(value);
#endif
        return result;
    }();
    return baseline;
}
inline bool diagnostic_present_overlay_baseline() {
    static const bool baseline = [] {
#if defined(_MSC_VER)
        char* value = nullptr; std::size_t size = 0;
        (void)_dupenv_s(&value, &size, "JFG_DIAGNOSTIC_PRESENT_OVERLAY_BASELINE");
#else
        const char* value = std::getenv("JFG_DIAGNOSTIC_PRESENT_OVERLAY_BASELINE");
#endif
        const bool result = requested_capture_phase() >= 0 &&
            value && value[0] == '1' && value[1] == '\0';
#if defined(_MSC_VER)
        std::free(value);
#endif
        return result;
    }();
    return baseline;
}
struct PresentationOutputDiagnostics {
    std::mutex mutex;
    std::map<std::pair<const void*, unsigned>, PresentationOutputSample> samples;
};
inline PresentationOutputDiagnostics& presentation_output_diagnostics() {
    static PresentationOutputDiagnostics state; return state;
}
inline void record_presentation_output(const void* counters, unsigned index,
        PresentationOutputSample sample) {
    if (!interpolation_trace_enabled()) return;
    auto& state = presentation_output_diagnostics();
    const std::scoped_lock lock(state.mutex);
    state.samples[{counters,index}] = sample;
}
inline PresentationOutputSample presentation_output_sample(const void* counters,
        unsigned index) {
    if (!interpolation_trace_enabled()) return {};
    auto& state = presentation_output_diagnostics();
    const std::scoped_lock lock(state.mutex);
    const auto it = state.samples.find({counters,index});
    return it == state.samples.end() ? PresentationOutputSample{} : it->second;
}
// Baseline geometry matching is an explicit investigation comparison, not
// required to record the final interpolation decisions during owner play.
inline bool interpolation_compare_enabled() {
    static const bool enabled=[] {
#if defined(_MSC_VER)
        char* flag=nullptr;std::size_t size=0;
        (void)_dupenv_s(&flag,&size,"JFG_RT64_COMPARE_MATCHING");
        const bool result=flag && flag[0]=='1' && flag[1]==0;
        std::free(flag);return result;
#else
        const char* flag=std::getenv("JFG_RT64_COMPARE_MATCHING");
        return flag && flag[0]=='1' && flag[1]==0;
#endif
    }();
    return enabled;
}
struct InterpolationTraceFile {
    std::mutex mutex;
    AsyncDiagnosticFile output;
    std::string path;
    bool closed=false;
    InterpolationTraceFile() {
#if defined(_MSC_VER)
        char* value=nullptr;std::size_t size=0;
        (void)_dupenv_s(&value,&size,"JFG_RT64_INTERPOLATION_TRACE");
        if(value) path=value;
        std::free(value);
#else
        const char* value=std::getenv("JFG_RT64_INTERPOLATION_TRACE");
        if(value) path=value;
#endif
        if(!path.empty()) output.open(path,std::ios::binary|std::ios::trunc);
    }
    void finish() noexcept {
        const std::scoped_lock lock(mutex);
        if(closed || path.empty()) return;
        closed=true;
        // Shutdown only: reject later producers, drain, and preserve status.
        const bool incomplete=!output;
        output.finish();
        try {
            std::ofstream footer(path,std::ios::binary|std::ios::app);
            footer<<"trace-end\t0\t0\t0\t0\t0\t0\t0\t"
                  <<output.dropped()<<'\t'<<unsigned(incomplete || output.failed())<<'\n';
        } catch(...) {}
    }
    ~InterpolationTraceFile(){finish();}
};
inline InterpolationTraceFile& interpolation_trace_file() {
    static InterpolationTraceFile file;return file;
}
inline void finish_interpolation_trace() noexcept {
    interpolation_trace_file().finish();
}
inline void trace_interpolation(const char* event, std::uint64_t id,
        unsigned source, unsigned target, unsigned frame, unsigned count,
        float alpha, std::size_t matched, std::size_t moving) {
    auto& log=interpolation_trace_file();
    if(log.path.empty()) return;
    const auto now=std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if(target==kUnlimitedPresentation) target=0;
    if(count==kUnlimitedPresentation) count=0;
    // One complete row outside the producer lock; the lock protects only
    // an in-memory buffer. Filesystem writes run on the bounded worker.
    std::array<char,384> row{};
    const auto length=(std::min)(std::strlen(event),std::size_t{32});
    std::memcpy(row.data(),event,length);
    char* cursor=row.data()+length;*cursor++='\t';
    const auto number=[&](auto value) {
        const auto result=std::to_chars(cursor,row.data()+row.size()-1U,value);
        if(result.ec!=std::errc{}) return false;
        cursor=result.ptr;*cursor++='\t';return true;
    };
    if(!number(now)||!number(id)||!number(source)||!number(target)||
       !number(frame)||!number(count)) return;
    const auto fraction=std::to_chars(cursor,row.data()+row.size()-1U,alpha,
                                      std::chars_format::general,6);
    if(fraction.ec!=std::errc{}) return;
    cursor=fraction.ptr;*cursor++='\t';
    if(!number(matched)||!number(moving)) return;
    cursor[-1]='\n';
    const std::scoped_lock lock(log.mutex);
    if(log.closed || !log.output) return;
    log.output.write(row.data(),static_cast<std::streamsize>(cursor-row.data()));
}
} // namespace jfg
