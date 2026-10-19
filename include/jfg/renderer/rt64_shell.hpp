#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>
#include <span>

namespace jfg {
struct PresentationSceneIdentity;

inline constexpr char kPinnedRt64Revision[] =
    "5473732a822a4423b5696e7cb18fecc425a59875";
inline constexpr std::size_t kRt64RequiredRdramBytes = 8U * 1024U * 1024U;
inline constexpr std::size_t kRt64RequiredHeaderBytes = 0x40U;

enum class Rt64ShellError {
    none,
    invalid_window,
    invalid_refresh_rate,
    invalid_memory,
    setup_dynamic_libraries_missing,
    setup_invalid_graphics_api,
    setup_graphics_api_not_found,
    setup_graphics_device_not_found,
    render_hook_unavailable,
    invalid_framebuffer,
    frame_capture_failed,
    invalid_task_address,
    unsupported_microcode,
    unsupported_commands,
    not_initialized,
    renderer_exception,
    conflicting_cpu_write,
};

enum class Rt64MemoryLayout {
    // Canonical N64 byte order, as produced by the private emulator capture.
    big_endian,
    // Four-byte words already reversed for RT64 on a little-endian host.
    host_word_swapped,
};

struct Rt64ViRegisters {
    std::uint32_t status = 0U;
    std::uint32_t origin = 0U;
    std::uint32_t width = 0U;
    std::uint32_t intr = 0U;
    std::uint32_t current_line = 0U;
    std::uint32_t timing = 0U;
    std::uint32_t vertical_sync = 0U;
    std::uint32_t horizontal_sync = 0U;
    std::uint32_t leap = 0U;
    std::uint32_t horizontal_start = 0U;
    std::uint32_t vertical_start = 0U;
    std::uint32_t vertical_burst = 0U;
    std::uint32_t x_scale = 0U;
    std::uint32_t y_scale = 0U;
};

struct Rt64ShellConfiguration {
    void* native_window = nullptr;
    std::uint32_t window_thread_id = 0U;
    // RT64 always receives private snapshots. It cannot mutate simulation
    // memory through these views.
    std::span<const std::byte> rom_header;
    std::span<const std::byte> rdram;
    Rt64ViRegisters* vi = nullptr;
    bool developer_mode = false;
    Rt64MemoryLayout memory_layout = Rt64MemoryLayout::big_endian;
    bool cpu_writeback = false;
    // Return intensity masks to the guest for its CPU postprocessing.
    bool cpu_mask_writeback = false;
    unsigned experimental_refresh_rate = 0U;
    bool unlimited_presentation = false;
    bool vsync = true;
};

struct Rt64RdramRange {
    std::size_t begin;
    std::size_t end;
};

// Commit completed device-owned ranges, never an old whole-memory snapshot.
// All spans are host-word-swapped. Validate every range and CPU ownership
// before writing anything; intervening CPU writes reject the entire commit.
// When supplied, previous_source advances with the commit so later CPU edits
// back to pre-render values are still recognized by snapshot merging.
[[nodiscard]] Rt64ShellError commit_rt64_rdram_ranges(
    std::span<const std::byte> submitted, std::span<const std::byte> rendered,
    std::span<std::byte> live, std::span<const Rt64RdramRange> ranges,
    std::span<std::byte> previous_source = {}) noexcept;

struct Rt64GraphicsTask {
    std::uint32_t ucode_address = 0U;
    std::uint32_t ucode_data_address = 0U;
    std::uint32_t command_address = 0U;
    // Immutable guest submission time, independent of renderer output rate.
    std::uint64_t vi_tick = 0U;
    std::shared_ptr<const PresentationSceneIdentity> presentation_scene{};
};

struct Rt64FramebufferInfo {
    std::uint32_t address = 0U;
    std::size_t width = 0U;
    std::size_t height = 0U;
    std::size_t bytes_per_pixel = 0U;

    [[nodiscard]] bool valid() const noexcept {
        if (width == 0U || height == 0U || bytes_per_pixel == 0U ||
            width > SIZE_MAX / bytes_per_pixel) {
            return false;
        }
        const std::size_t row_bytes = width * bytes_per_pixel;
        return height <= SIZE_MAX / row_bytes;
    }

    [[nodiscard]] std::size_t byte_count() const noexcept {
        return valid() ? width * bytes_per_pixel * height : 0U;
    }
};

struct Rt64FrameView {
    std::size_t width = 0U;
    std::size_t height = 0U;
    std::size_t row_pitch_bytes = 0U;
    std::span<const std::byte> bgra8;

    [[nodiscard]] bool valid() const noexcept {
        return width != 0U && height != 0U && width <= SIZE_MAX / 4U &&
            row_pitch_bytes >= width * 4U &&
            row_pitch_bytes <= SIZE_MAX / height &&
            bgra8.size() == row_pitch_bytes * height;
    }
};

// Cumulative GPU durations from already fenced framebuffer submissions.
// Excludes host submission, presentation and queue delay before the first marker.
// Includes any GPU-side dependency waits between the two markers.
struct Rt64FramebufferGpuTiming {
    std::uint64_t submissions = 0;
    std::uint64_t total_nanoseconds = 0;
    std::uint64_t last_nanoseconds = 0;
    std::uint64_t maximum_nanoseconds = 0;
    bool failed = false;
};

struct Rt64GraphicsDiagnostics {
    std::size_t matrix_commands = 0U;
    std::size_t vertex_batches = 0U;
    std::size_t vertices_loaded = 0U;
    std::size_t vertices_finite = 0U;
    std::size_t vertices_in_clip = 0U;
    std::size_t triangle_batches = 0U;
    std::size_t triangles_drawn = 0U;
    std::size_t dma_display_lists = 0U;
    std::uint32_t color_image_address = 0U;
    std::uint32_t rejected_command_word0 = 0U;
    std::uint32_t rejected_command_word1 = 0U;
    std::uint32_t rejected_command_address = 0U;
    std::uint32_t rejection_reason = 0U;
    std::uint32_t rejection_detail = 0U;
    std::uint32_t rejection_source_address = 0U;
    std::uint32_t rejection_triangle = 0U;
    std::uint64_t loaded_vertices_low = 0U;
    std::uint16_t loaded_vertices_high = 0U;
    std::uint64_t full_sync_microseconds = 0U;
    std::uint64_t history_prepare_microseconds = 0U;
    std::uint64_t rdp_sync_microseconds = 0U;
    std::array<std::uint64_t,5> history_stage_microseconds{};

    std::uint64_t rdram_check_microseconds = 0U;
    std::size_t display_list_branches = 0U;
    std::uint32_t last_display_list_address = 0U;
    std::uint32_t last_display_list_target = 0U;
    // True when the RT64 developer inspector was paused and the last task was
    // not interpreted (RT64 only acknowledges it with a DP interrupt).
    bool renderer_paused = false;
};

[[nodiscard]] Rt64ShellError validate_rt64_shell_configuration(
    const Rt64ShellConfiguration& configuration) noexcept;

[[nodiscard]] Rt64ShellError copy_rt64_rdram_snapshot(
    std::span<const std::byte> source,
    std::span<std::byte> destination,
    Rt64MemoryLayout layout) noexcept;

// Imports a new simulation snapshot without erasing bytes that RT64 wrote to
// its private render-to-RAM copy. Bytes changed by the simulation always win;
// unchanged source bytes retain the renderer's current value.
[[nodiscard]] Rt64ShellError merge_rt64_rdram_snapshot(
    std::span<const std::byte> source,
    std::span<std::byte> destination,
    std::span<std::byte> previous_source,
    Rt64MemoryLayout layout) noexcept;

// Refresh CPU-owned bytes even when the CPU rewrites their previous value.
// Only the sorted (possibly overlapping) live framebuffer ranges retain GPU data.
[[nodiscard]] Rt64ShellError refresh_rt64_cpu_memory(
    std::span<const std::byte> source, std::span<std::byte> destination,
    std::span<const Rt64RdramRange> gpu_ranges, Rt64MemoryLayout layout) noexcept;

[[nodiscard]] Rt64ShellError inspect_rt64_framebuffer(
    const Rt64ViRegisters& vi,
    std::size_t rdram_size,
    Rt64FramebufferInfo& framebuffer) noexcept;

class Rt64Shell final {
public:
    ~Rt64Shell();
    Rt64Shell(const Rt64Shell&) = delete;
    Rt64Shell& operator=(const Rt64Shell&) = delete;
    Rt64Shell(Rt64Shell&&) = delete;
    Rt64Shell& operator=(Rt64Shell&&) = delete;

    [[nodiscard]] static std::unique_ptr<Rt64Shell> create(
        const Rt64ShellConfiguration& configuration,
        Rt64ShellError& error) noexcept;

    [[nodiscard]] Rt64ShellError submit(
        const Rt64GraphicsTask& task) noexcept;
    [[nodiscard]] Rt64ShellError replace_rdram_snapshot(
        std::span<const std::byte> rdram,
        Rt64MemoryLayout layout) noexcept;
    // Community (2026-10-19): import only the listed byte ranges of a
    // host_word_swapped snapshot while full CPU writeback is active. The
    // caller guarantees that every byte outside the ranges already equals
    // RT64's copy. Other modes and layouts fall back to the full import.
    // True when replace_rdram_snapshot_ranges honours its ranges.
    [[nodiscard]] bool ranged_import_available() const noexcept;
    [[nodiscard]] Rt64ShellError replace_rdram_snapshot_ranges(
        std::span<const std::byte> rdram,
        Rt64MemoryLayout layout,
        std::span<const Rt64RdramRange> ranges) noexcept;
    [[nodiscard]] Rt64ShellError copy_private_rdram_snapshot(
        std::span<std::byte> rdram,
        Rt64MemoryLayout layout) const noexcept;
    [[nodiscard]] Rt64ShellError commit_cpu_writeback(
        std::span<const std::byte> submitted, std::span<std::byte> live) noexcept;
    // Query on the same frontend thread as submit/present. This follows RT64's
    // resident framebuffer registry, independently of host snapshot lifetimes.
    [[nodiscard]] bool has_color_framebuffer(std::uint32_t address) const noexcept;
    // Community diagnostic (AI-assisted, Claude, 2026-10): one line per RT64
    // framebuffer whose RDRAM range overlaps [begin, end): address, size and
    // write state. Empty when none overlap.
    [[nodiscard]] std::string debug_framebuffers_overlapping(
        std::uint32_t begin, std::uint32_t end) const;
    [[nodiscard]] Rt64ShellError present(bool capture_frame = true) noexcept;
    [[nodiscard]] bool developer_debugger_available() const noexcept;
    [[nodiscard]] std::size_t last_command_count() const noexcept;
    [[nodiscard]] Rt64GraphicsDiagnostics last_graphics_diagnostics()
        const noexcept;
    // Internal profiling only; configure before guest submissions begin.
    [[nodiscard]] bool enable_framebuffer_gpu_timing() noexcept;
    // Read on the same submit owner; does not wait or issue GPU work.
    [[nodiscard]] Rt64FramebufferGpuTiming framebuffer_gpu_timing() const noexcept;
    // Completed regular-workload timestamps; snapshot synchronized with its worker.
    [[nodiscard]] Rt64FramebufferGpuTiming workload_gpu_timing() const noexcept;
    [[nodiscard]] Rt64FramebufferInfo last_framebuffer() const noexcept;
    [[nodiscard]] Rt64FrameView last_presented_frame() const noexcept;

private:
    struct Impl;
    explicit Rt64Shell(std::unique_ptr<Impl> impl) noexcept;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] const char* rt64_shell_error_message(
    Rt64ShellError error) noexcept;

}  // namespace jfg
