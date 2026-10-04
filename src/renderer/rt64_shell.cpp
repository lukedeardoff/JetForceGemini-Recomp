#include "jfg/renderer/rt64_shell.hpp"

#include "rt64_f3ddkr.hpp"

#include "hle/rt64_application.h"
#include "hle/rt64_vi.h"
#include "render/rt64_texture_cache.h"
#include "rhi/rt64_render_hooks.h"

#include <atomic>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>

namespace RT64 {
// Defined in RT64 hle/rt64_framebuffer_manager.cpp (JFG local change).
extern float ShadowBlurTexels;
}

namespace jfg {
namespace {

constexpr std::size_t kRspMemoryBytes = 4096U;
constexpr std::size_t kMaximumUcodeBytes = 4096U;
constexpr std::size_t kMaximumUcodeDataBytes = 2048U;
constexpr std::uint32_t kMinimumWritebackWidth = 128U;

void ignore_rt64_interrupts() {}

// JFG_SHADOW_BLUR=<native texels> sets the soft edge on actor shadows
// (unset or 0 = off).
void apply_shadow_blur_setting() noexcept {
#ifdef _WIN32
    char* value = nullptr;
    std::size_t value_length = 0U;
    if (_dupenv_s(&value, &value_length, "JFG_SHADOW_BLUR") == 0 && value != nullptr) {
        char* end = nullptr;
        const float parsed = std::strtof(value, &end);
        if (end != value && parsed >= 0.0f && parsed <= 16.0f) {
            RT64::ShadowBlurTexels = parsed;
        }
    }
    std::free(value);
#endif
}

// Auto-load RT64 texture packs: every subfolder of %LOCALAPPDATA%\\JFGRecomp\\texture-packs
// that contains an rt64.json is loaded at startup (sorted by name).
void load_user_texture_packs(RT64::Application& application) noexcept {
    try {
        if (application.textureCache == nullptr) {
            return;
        }
        std::filesystem::path local_app_data;
#ifdef _WIN32
        wchar_t* value = nullptr;
        std::size_t value_length = 0U;
        if (_wdupenv_s(&value, &value_length, L"LOCALAPPDATA") == 0 && value != nullptr) {
            local_app_data = value;
        }
        std::free(value);
#else
        if (const char* value = std::getenv("LOCALAPPDATA"); value != nullptr) {
            local_app_data = value;
        }
#endif
        if (local_app_data.empty()) {
            return;
        }
        const std::filesystem::path root =
            local_app_data / "JFGRecomp" / "texture-packs";
        std::error_code ec;
        if (!std::filesystem::is_directory(root, ec)) {
            return;
        }
        std::vector<std::filesystem::path> packs;
        for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
            if (entry.is_directory(ec) &&
                std::filesystem::is_regular_file(entry.path() / "rt64.json", ec)) {
                packs.push_back(entry.path());
            }
        }
        if (packs.empty()) {
            return;
        }
        std::sort(packs.begin(), packs.end());
        std::vector<RT64::ReplacementDirectory> directories;
        for (const auto& pack : packs) {
            directories.emplace_back(pack);
        }
        application.textureCache->loadReplacementDirectories(directories);
    }
    catch (...) {
    }
}

[[nodiscard]] bool address_range_valid(
    const std::uint32_t address,
    const std::size_t length) noexcept {
    const std::size_t offset = address & 0x00FF'FFFFU;
    return offset <= kRt64RequiredRdramBytes &&
        length <= kRt64RequiredRdramBytes - offset;
}

[[nodiscard]] Rt64ShellError setup_error(
    const RT64::Application::SetupResult result) noexcept {
    switch (result) {
    case RT64::Application::SetupResult::Success:
        return Rt64ShellError::none;
    case RT64::Application::SetupResult::DynamicLibrariesNotFound:
        return Rt64ShellError::setup_dynamic_libraries_missing;
    case RT64::Application::SetupResult::InvalidGraphicsAPI:
        return Rt64ShellError::setup_invalid_graphics_api;
    case RT64::Application::SetupResult::GraphicsAPINotFound:
        return Rt64ShellError::setup_graphics_api_not_found;
    case RT64::Application::SetupResult::GraphicsDeviceNotFound:
        return Rt64ShellError::setup_graphics_device_not_found;
    }
    return Rt64ShellError::renderer_exception;
}

}  // namespace

struct CaptureContext;
std::atomic<CaptureContext*> active_capture = nullptr;

struct CaptureContext {
    RT64::Application* application = nullptr;
    RenderDevice* device = nullptr;
    RT64::RenderHookInit* previous_init = nullptr;
    RT64::RenderHookDraw* previous_draw = nullptr;
    RT64::RenderHookDeinit* previous_deinit = nullptr;
    std::unique_ptr<RenderBuffer> readback;
    std::vector<std::byte> pixels;
    std::size_t width = 0U;
    std::size_t height = 0U;
    std::size_t row_pitch = 0U;
    bool armed = false;
    bool scheduled = false;
    bool failed = false;
    std::mutex mutex;

    [[nodiscard]] bool activate() noexcept;
    void deactivate() noexcept;
    void arm() noexcept;
    void draw(RenderCommandList* list, RenderFramebuffer* framebuffer) noexcept;
    [[nodiscard]] bool finish() noexcept;
};

void capture_init(RenderInterface* render_interface, RenderDevice* device) {
    CaptureContext* context = active_capture.load();
    if (context == nullptr) {
        return;
    }
    if (context->previous_init != nullptr) {
        context->previous_init(render_interface, device);
    }
    const std::scoped_lock lock(context->mutex);
    context->device = device;
}

void capture_draw(RenderCommandList* list, RenderFramebuffer* framebuffer) {
    CaptureContext* context = active_capture.load();
    if (context == nullptr) {
        return;
    }
    if (context->previous_draw != nullptr) {
        context->previous_draw(list, framebuffer);
    }
    context->draw(list, framebuffer);
}

void capture_deinit() {
    CaptureContext* context = active_capture.load();
    if (context == nullptr) {
        return;
    }
    if (context->previous_deinit != nullptr) {
        context->previous_deinit();
    }
    const std::scoped_lock lock(context->mutex);
    context->device = nullptr;
}

void clear_vi_guard_texel(
    RT64::Application* application,
    RenderCommandList* list) noexcept {
    if (application == nullptr || application->swapChain == nullptr ||
        list == nullptr) {
        return;
    }

    const RT64::VI vi = application->core.decodeVI();
    const hlslpp::uint2 framebuffer_size = vi.fbSize();
    const std::uint32_t window_width = application->swapChain->getWidth();
    const std::uint32_t window_height = application->swapChain->getHeight();
    if (!vi.visible() || framebuffer_size.x == 0U ||
        framebuffer_size.y == 0U || window_width == 0U ||
        window_height == 0U) {
        return;
    }

    // RT64 deliberately allocates filtering guard texels beyond the N64's
    // useful framebuffer extent. JFG leaves the lower/right guard texel
    // undefined, so linear VI sampling otherwise repeats it as a colored
    // edge. Clip exactly one source texel after VI scaling without touching
    // guest RDRAM or the rendered image interior.
    const double scale = (std::min)(
        static_cast<double>(window_width) /
            static_cast<double>(framebuffer_size.x),
        static_cast<double>(window_height) /
            static_cast<double>(framebuffer_size.y));
    if (!std::isfinite(scale) || scale <= 0.0) {
        return;
    }
    const double rendered_width =
        static_cast<double>(framebuffer_size.x) * scale;
    const double rendered_height =
        static_cast<double>(framebuffer_size.y) * scale;
    const std::int32_t left = static_cast<std::int32_t>(std::lround(
        (static_cast<double>(window_width) - rendered_width) * 0.5));
    const std::int32_t top = static_cast<std::int32_t>(std::lround(
        (static_cast<double>(window_height) - rendered_height) * 0.5));
    const std::int32_t right = static_cast<std::int32_t>(std::lround(
        static_cast<double>(left) + rendered_width));
    const std::int32_t bottom = static_cast<std::int32_t>(std::lround(
        static_cast<double>(top) + rendered_height));
    const std::int32_t guard = static_cast<std::int32_t>(std::ceil(scale));
    if (right <= left || bottom <= top || guard <= 0) {
        return;
    }

    const std::array<RenderRect, 2U> guard_rectangles{
        RenderRect((std::max)(left, right - guard), top, right, bottom),
        RenderRect(left, (std::max)(top, bottom - guard), right, bottom),
    };
    list->clearColor(
        0U,
        RenderColor(),
        guard_rectangles.data(),
        static_cast<std::uint32_t>(guard_rectangles.size()));
}

bool CaptureContext::activate() noexcept {
    CaptureContext* expected = nullptr;
    if (!active_capture.compare_exchange_strong(expected, this)) {
        return false;
    }
    previous_init = RT64::GetRenderHookInit();
    previous_draw = RT64::GetRenderHookDraw();
    previous_deinit = RT64::GetRenderHookDeinit();
    RT64::SetRenderHooks(&capture_init, &capture_draw, &capture_deinit);
    return true;
}

void CaptureContext::deactivate() noexcept {
    CaptureContext* expected = this;
    if (active_capture.compare_exchange_strong(expected, nullptr)) {
        RT64::SetRenderHooks(previous_init, previous_draw, previous_deinit);
    }
}

void CaptureContext::arm() noexcept {
    const std::scoped_lock lock(mutex);
    pixels.clear();
    armed = true;
    scheduled = false;
    failed = false;
}

void CaptureContext::draw(
    RenderCommandList* list,
    RenderFramebuffer* framebuffer) noexcept {
    const std::scoped_lock lock(mutex);
    clear_vi_guard_texel(application, list);
    if (!armed || scheduled || failed || application == nullptr ||
        application->swapChain == nullptr ||
        application->presentQueue == nullptr || device == nullptr ||
        list == nullptr || framebuffer == nullptr) {
        return;
    }
    try {
        std::size_t texture_index =
            application->presentQueue->swapChainFramebuffers.size();
        for (std::size_t index = 0U;
             index < application->presentQueue->swapChainFramebuffers.size();
             ++index) {
            if (application->presentQueue->swapChainFramebuffers[index].get() ==
                framebuffer) {
                texture_index = index;
                break;
            }
        }
        if (texture_index >= application->swapChain->getTextureCount()) {
            failed = true;
            return;
        }
        RenderTexture* texture = application->swapChain->getTexture(
            static_cast<std::uint32_t>(texture_index));
        width = application->swapChain->getWidth();
        height = application->swapChain->getHeight();
        if (texture == nullptr || width == 0U || height == 0U) {
            failed = true;
            return;
        }
        row_pitch = ((width * 4U + 255U) / 256U) * 256U;
        const std::size_t byte_count = row_pitch * height;
        readback = device->createBuffer(RenderBufferDesc::ReadbackBuffer(
            byte_count));
        if (readback == nullptr) {
            failed = true;
            return;
        }
        const std::array<RenderBufferBarrier, 1U> buffer_barriers{
            RenderBufferBarrier(readback.get(), RenderBufferAccess::WRITE),
        };
        list->barriers(RenderBarrierStage::COPY,
            buffer_barriers.data(),
            static_cast<std::uint32_t>(buffer_barriers.size()));
        list->barriers(RenderBarrierStage::COPY,
            RenderTextureBarrier(texture, RenderTextureLayout::COPY_SOURCE));
        list->copyTextureRegion(
            RenderTextureCopyLocation::PlacedFootprint(
                readback.get(),
                RenderFormat::B8G8R8A8_UNORM,
                static_cast<std::uint32_t>(width),
                static_cast<std::uint32_t>(height),
                1U,
                static_cast<std::uint32_t>(row_pitch / 4U)),
            RenderTextureCopyLocation::Subresource(texture));
        list->barriers(RenderBarrierStage::ALL,
            RenderTextureBarrier(texture, RenderTextureLayout::COLOR_WRITE));
        scheduled = true;
    }
    catch (...) {
        failed = true;
    }
}

bool CaptureContext::finish() noexcept {
    const std::scoped_lock lock(mutex);
    armed = false;
    if (!scheduled || failed || readback == nullptr || width == 0U ||
        height == 0U || row_pitch > SIZE_MAX / height) {
        return false;
    }
    try {
        const std::size_t byte_count = row_pitch * height;
        const RenderRange range(0U, byte_count);
        const auto* mapped = static_cast<const std::byte*>(
            readback->map(0U, &range));
        if (mapped == nullptr) {
            return false;
        }
        try {
            pixels.assign(mapped, mapped + byte_count);
        }
        catch (...) {
            readback->unmap();
            throw;
        }
        readback->unmap();
        return true;
    }
    catch (...) {
        pixels.clear();
        return false;
    }
}

struct Rt64Shell::Impl {
    std::array<std::uint8_t, kRt64RequiredHeaderBytes> header{};
    std::vector<std::uint8_t> rdram =
        std::vector<std::uint8_t>(kRt64RequiredRdramBytes);
    std::vector<std::uint8_t> simulation_rdram =
        std::vector<std::uint8_t>(kRt64RequiredRdramBytes);
    std::vector<std::uint8_t> task_rdram_copy =
        std::vector<std::uint8_t>(kRt64RequiredRdramBytes);
    std::span<const std::uint8_t> task_rdram{};
    std::array<std::uint8_t, kRspMemoryBytes> dmem{};
    std::array<std::uint8_t, kRspMemoryBytes> imem{};
    std::uint32_t mi_interrupt = 0U;
    std::array<std::uint32_t, 8U> dpc{};
    std::unique_ptr<RT64::Application> application;
    std::unique_ptr<Rt64F3ddkr> f3ddkr;
    std::unique_ptr<CaptureContext> capture;
    Rt64FramebufferInfo last_framebuffer;
    bool developer_mode = false;
    bool initialized = false;
    bool setup_started = false;
    bool cpu_writeback = false;
    bool writeback_pending = false;
    bool last_submit_paused = false;
    std::vector<Rt64RdramRange> writeback_ranges;
    std::uint64_t last_rdram_check_microseconds = 0U;

    ~Impl() {
        f3ddkr.reset();
        if (setup_started && application != nullptr) {
            application->end();
        }
        if (capture != nullptr) {
            capture->deactivate();
        }
    }
};

Rt64ShellError validate_rt64_shell_configuration(
    const Rt64ShellConfiguration& configuration) noexcept {
    if (configuration.native_window == nullptr) {
        return Rt64ShellError::invalid_window;
    }
    if (configuration.rom_header.size() < kRt64RequiredHeaderBytes ||
        configuration.rdram.size() != kRt64RequiredRdramBytes ||
        configuration.vi == nullptr) {
        return Rt64ShellError::invalid_memory;
    }
    return Rt64ShellError::none;
}

Rt64ShellError copy_rt64_rdram_snapshot(
    const std::span<const std::byte> source,
    const std::span<std::byte> destination,
    const Rt64MemoryLayout layout) noexcept {
    if (source.size() != kRt64RequiredRdramBytes ||
        destination.size() != kRt64RequiredRdramBytes) {
        return Rt64ShellError::invalid_memory;
    }
    if (layout == Rt64MemoryLayout::host_word_swapped) {
        for (std::size_t index = 0U; index < source.size(); ++index) {
            destination[index] = source[index];
        }
        return Rt64ShellError::none;
    }
    for (std::size_t index = 0U; index < source.size(); index += 4U) {
        destination[index] = source[index + 3U];
        destination[index + 1U] = source[index + 2U];
        destination[index + 2U] = source[index + 1U];
        destination[index + 3U] = source[index];
    }
    return Rt64ShellError::none;
}

Rt64ShellError merge_rt64_rdram_snapshot(
    const std::span<const std::byte> source,
    const std::span<std::byte> destination,
    const std::span<std::byte> previous_source,
    const Rt64MemoryLayout layout) noexcept {
    if (source.size() != kRt64RequiredRdramBytes ||
        destination.size() != kRt64RequiredRdramBytes ||
        previous_source.size() != kRt64RequiredRdramBytes) {
        return Rt64ShellError::invalid_memory;
    }
    if (layout == Rt64MemoryLayout::host_word_swapped) {
        constexpr std::size_t kComparisonBlockBytes = 4096U;
        for (std::size_t base = 0U; base < source.size();
             base += kComparisonBlockBytes) {
            if (std::memcmp(source.data() + base,
                            previous_source.data() + base,
                            kComparisonBlockBytes) == 0) {
                continue;
            }
            for (std::size_t index = base;
                 index < base + kComparisonBlockBytes; ++index) {
                const std::byte incoming = source[index];
                if (incoming != previous_source[index]) {
                    destination[index] = incoming;
                    previous_source[index] = incoming;
                }
            }
        }
        return Rt64ShellError::none;
    }
    for (std::size_t index = 0U; index < source.size(); ++index) {
        const std::byte incoming = source[index ^ 3U];
        if (incoming != previous_source[index]) {
            destination[index] = incoming;
            previous_source[index] = incoming;
        }
    }
    return Rt64ShellError::none;
}

Rt64ShellError commit_rt64_rdram_ranges(
    const std::span<const std::byte> submitted,
    const std::span<const std::byte> rendered,
    const std::span<std::byte> live,
    const std::span<const Rt64RdramRange> ranges) noexcept {
    if (submitted.size() != kRt64RequiredRdramBytes ||
        rendered.size() != submitted.size() || live.empty() ||
        live.size() > submitted.size() || live.size() % 4U != 0U)
        return Rt64ShellError::invalid_memory;
    for (const auto range : ranges) {
        if (range.begin >= range.end || range.end > live.size())
            return Rt64ShellError::invalid_memory;
        for (std::size_t address = range.begin; address < range.end; ++address) {
            if (live[address ^ 3U] != submitted[address ^ 3U])
                return Rt64ShellError::conflicting_cpu_write;
        }
    }
    for (const auto range : ranges)
        for (std::size_t address = range.begin; address < range.end; ++address)
            live[address ^ 3U] = rendered[address ^ 3U];
    return Rt64ShellError::none;
}

Rt64ShellError inspect_rt64_framebuffer(
    const Rt64ViRegisters& vi,
    const std::size_t rdram_size,
    Rt64FramebufferInfo& framebuffer) noexcept {
    framebuffer = {};
    const std::uint32_t pixel_type = vi.status & 0x3U;
    const std::size_t bytes_per_pixel = pixel_type == 2U
        ? 2U
        : pixel_type == 3U ? 4U : 0U;
    const std::size_t vi_width = vi.width & 0xFFFU;
    const std::uint32_t horizontal_start =
        (vi.horizontal_start >> 16U) & 0x3FFU;
    const std::uint32_t horizontal_end = vi.horizontal_start & 0x3FFU;
    const std::uint32_t vertical_start =
        (vi.vertical_start >> 16U) & 0x3FFU;
    const std::uint32_t vertical_end = vi.vertical_start & 0x3FFU;
    const std::uint32_t x_scale = vi.x_scale & 0xFFFU;
    const std::uint32_t y_scale = vi.y_scale & 0xFFFU;
    if (bytes_per_pixel == 0U || vi_width == 0U ||
        horizontal_start == 0U || horizontal_end <= horizontal_start ||
        vertical_end <= vertical_start || x_scale == 0U || y_scale == 0U) {
        return Rt64ShellError::invalid_framebuffer;
    }

    std::size_t framebuffer_width = vi_width;
    const bool serrate = (vi.status & (1U << 6U)) != 0U;
    const double estimated_width =
        static_cast<double>(horizontal_end - horizontal_start) *
        static_cast<double>(x_scale) / 1024.0;
    if (serrate && estimated_width <
            static_cast<double>(vi_width) / 1.875) {
        framebuffer_width = vi_width / 2U;
    }
    if (framebuffer_width == 0U) {
        return Rt64ShellError::invalid_framebuffer;
    }

    const double estimated_height =
        static_cast<double>(vertical_end - vertical_start) *
        static_cast<double>(y_scale) * static_cast<double>(vi_width) /
        (2048.0 * static_cast<double>(framebuffer_width));
    if (!std::isfinite(estimated_height) || estimated_height <= 0.0 ||
        estimated_height > static_cast<double>(SIZE_MAX - 2U)) {
        return Rt64ShellError::invalid_framebuffer;
    }
    const double expanded_height = std::round(estimated_height) + 2.0;
    const double aligned_height = std::round(expanded_height / 4.0) * 4.0;
    if (!std::isfinite(aligned_height) || aligned_height <= 0.0 ||
        aligned_height > static_cast<double>(SIZE_MAX)) {
        return Rt64ShellError::invalid_framebuffer;
    }
    const std::size_t framebuffer_height =
        static_cast<std::size_t>(aligned_height);

    if (vi_width > SIZE_MAX / bytes_per_pixel) {
        return Rt64ShellError::invalid_framebuffer;
    }
    const std::size_t row_bytes = vi_width * bytes_per_pixel;
    const std::size_t row_count = serrate && (vi.current_line & 1U) != 0U
        ? 2U
        : 1U;
    if (row_bytes > SIZE_MAX / row_count) {
        return Rt64ShellError::invalid_framebuffer;
    }
    const std::size_t row_offset = row_bytes * row_count;
    std::size_t address = vi.origin & 0x00FF'FFFFU;
    if (address >= row_offset) {
        address -= row_offset;
    }

    framebuffer = {
        static_cast<std::uint32_t>(address),
        framebuffer_width,
        framebuffer_height,
        bytes_per_pixel,
    };
    const std::size_t byte_count = framebuffer.byte_count();
    if (byte_count == 0U || address > rdram_size ||
        byte_count > rdram_size - address) {
        framebuffer = {};
        return Rt64ShellError::invalid_framebuffer;
    }
    return Rt64ShellError::none;
}

Rt64Shell::Rt64Shell(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

Rt64Shell::~Rt64Shell() = default;

Rt64ShellError Rt64Shell::replace_rdram_snapshot(
    const std::span<const std::byte> rdram,
    const Rt64MemoryLayout layout) noexcept {
    if (impl_ == nullptr || !impl_->initialized ||
        impl_->application == nullptr) {
        return Rt64ShellError::not_initialized;
    }
    if (impl_->writeback_pending)
        return Rt64ShellError::conflicting_cpu_write;
    const Rt64ShellError merge_error = merge_rt64_rdram_snapshot(
        rdram,
        std::as_writable_bytes(std::span(impl_->rdram)),
        std::as_writable_bytes(std::span(impl_->simulation_rdram)),
        layout);
    if (merge_error != Rt64ShellError::none) {
        return merge_error;
    }
    if (layout == Rt64MemoryLayout::host_word_swapped) {
        impl_->task_rdram = {
            reinterpret_cast<const std::uint8_t*>(rdram.data()),
            rdram.size(),
        };
        return Rt64ShellError::none;
    }
    const Rt64ShellError task_copy_error = copy_rt64_rdram_snapshot(
        rdram,
        std::as_writable_bytes(std::span(impl_->task_rdram_copy)),
        layout);
    if (task_copy_error == Rt64ShellError::none) {
        impl_->task_rdram = impl_->task_rdram_copy;
    }
    return task_copy_error;
}

Rt64ShellError Rt64Shell::copy_private_rdram_snapshot(
    const std::span<std::byte> rdram,
    const Rt64MemoryLayout layout) const noexcept {
    if (impl_ == nullptr || !impl_->initialized ||
        impl_->application == nullptr) {
        return Rt64ShellError::not_initialized;
    }
    return copy_rt64_rdram_snapshot(
        std::as_bytes(std::span(impl_->rdram)), rdram, layout);
}

std::unique_ptr<Rt64Shell> Rt64Shell::create(
    const Rt64ShellConfiguration& configuration,
    Rt64ShellError& error) noexcept {
    error = validate_rt64_shell_configuration(configuration);
    if (error != Rt64ShellError::none) {
        return nullptr;
    }

    try {
        auto impl = std::make_unique<Impl>();
        impl->developer_mode = configuration.developer_mode;
        impl->cpu_writeback = configuration.cpu_writeback;
        for (std::size_t index = 0U; index < impl->header.size(); ++index) {
            impl->header[index] = std::to_integer<std::uint8_t>(
                configuration.rom_header[index]);
        }
        const Rt64ShellError snapshot_error = copy_rt64_rdram_snapshot(
            configuration.rdram,
            std::as_writable_bytes(std::span(impl->rdram)),
            configuration.memory_layout);
        if (snapshot_error != Rt64ShellError::none) {
            error = snapshot_error;
            return nullptr;
        }
        impl->simulation_rdram = impl->rdram;
        impl->task_rdram_copy = impl->rdram;
        impl->task_rdram = impl->task_rdram_copy;

        RT64::Application::Core core{};
        core.window = static_cast<RenderWindow>(configuration.native_window);
        core.HEADER = impl->header.data();
        core.RDRAM = impl->rdram.data();
        core.DMEM = impl->dmem.data();
        core.IMEM = impl->imem.data();
        core.MI_INTR_REG = &impl->mi_interrupt;
        core.DPC_START_REG = &impl->dpc[0U];
        core.DPC_END_REG = &impl->dpc[1U];
        core.DPC_CURRENT_REG = &impl->dpc[2U];
        core.DPC_STATUS_REG = &impl->dpc[3U];
        core.DPC_CLOCK_REG = &impl->dpc[4U];
        core.DPC_BUFBUSY_REG = &impl->dpc[5U];
        core.DPC_PIPEBUSY_REG = &impl->dpc[6U];
        core.DPC_TMEM_REG = &impl->dpc[7U];
        core.VI_STATUS_REG = &configuration.vi->status;
        core.VI_ORIGIN_REG = &configuration.vi->origin;
        core.VI_WIDTH_REG = &configuration.vi->width;
        core.VI_INTR_REG = &configuration.vi->intr;
        core.VI_V_CURRENT_LINE_REG = &configuration.vi->current_line;
        core.VI_TIMING_REG = &configuration.vi->timing;
        core.VI_V_SYNC_REG = &configuration.vi->vertical_sync;
        core.VI_H_SYNC_REG = &configuration.vi->horizontal_sync;
        core.VI_LEAP_REG = &configuration.vi->leap;
        core.VI_H_START_REG = &configuration.vi->horizontal_start;
        core.VI_V_START_REG = &configuration.vi->vertical_start;
        core.VI_V_BURST_REG = &configuration.vi->vertical_burst;
        core.VI_X_SCALE_REG = &configuration.vi->x_scale;
        core.VI_Y_SCALE_REG = &configuration.vi->y_scale;
        // RT64 raises SP internally at the end of processDisplayLists. The
        // project bridge owns the externally visible completion commit, after
        // parsing and oracle validation, so this callback must remain isolated.
        core.checkInterrupts = &ignore_rt64_interrupts;

        RT64::ApplicationConfiguration app_configuration;
        app_configuration.appId = "jfg-recomp";
        app_configuration.dataPath = std::filesystem::path{};
        app_configuration.detectDataPath = false;
        app_configuration.useConfigurationFile = false;
        impl->application =
            std::make_unique<RT64::Application>(core, app_configuration);
        impl->capture = std::make_unique<CaptureContext>();
        impl->capture->application = impl->application.get();
        if (!impl->capture->activate()) {
            error = Rt64ShellError::render_hook_unavailable;
            return nullptr;
        }
        impl->application->userConfig.developerMode =
            configuration.developer_mode;
        impl->setup_started = true;
        const Rt64ShellError result = setup_error(
            impl->application->setup(configuration.window_thread_id));
        if (result != Rt64ShellError::none) {
            error = result;
            return nullptr;
        }
        if (impl->application->state == nullptr ||
            impl->application->interpreter == nullptr ||
            impl->application->workloadQueue == nullptr ||
            impl->application->presentQueue == nullptr) {
            error = Rt64ShellError::setup_dynamic_libraries_missing;
            return nullptr;
        }
        impl->initialized = true;
        load_user_texture_packs(*impl->application);
        apply_shadow_blur_setting();
        impl->f3ddkr = std::make_unique<Rt64F3ddkr>(*impl->application);
        error = Rt64ShellError::none;
        return std::unique_ptr<Rt64Shell>(
            new Rt64Shell(std::move(impl)));
    }
    catch (...) {
        error = Rt64ShellError::renderer_exception;
        return nullptr;
    }
}

Rt64ShellError Rt64Shell::submit(const Rt64GraphicsTask& task) noexcept {
    if (impl_ == nullptr || !impl_->initialized ||
        impl_->application == nullptr) {
        return Rt64ShellError::not_initialized;
    }
    if (!address_range_valid(task.ucode_address, kMaximumUcodeBytes) ||
        !address_range_valid(task.ucode_data_address, kMaximumUcodeDataBytes) ||
        !address_range_valid(task.command_address, sizeof(std::uint64_t))) {
        return Rt64ShellError::invalid_task_address;
    }

    try {
        RT64::Application& application = *impl_->application;
        if (impl_->writeback_pending)
            return Rt64ShellError::conflicting_cpu_write;
        if (application.state->debuggerInspector.paused) {
            // Developer inspector paused (F4): RT64 skips the display list and
            // only raises the DP interrupt. Keep the game running without
            // treating the uninterpreted task as a renderer failure.
            impl_->last_submit_paused = true;
            application.processDisplayLists(
                const_cast<std::uint8_t*>(impl_->task_rdram.data()),
                task.command_address & 0x00FF'FFFFU,
                0U,
                true);
            if (impl_->cpu_writeback) {
                impl_->writeback_ranges.clear();
                impl_->writeback_pending = true;
            }
            return Rt64ShellError::none;
        }
        impl_->last_submit_paused = false;
        const auto previous_write = application.state->framebufferManager.writeTimestamp;
        impl_->writeback_ranges.clear();
        application.state->rsp->reset();
        if (impl_->task_rdram.size() != kRt64RequiredRdramBytes) {
            return Rt64ShellError::invalid_memory;
        }
        impl_->f3ddkr->begin(impl_->task_rdram);
        impl_->f3ddkr->install();
        const auto rdram_check_start = std::chrono::steady_clock::now();
        application.state->checkRDRAM();
        impl_->last_rdram_check_microseconds =
            static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - rdram_check_start)
                    .count());
        application.processDisplayLists(
            const_cast<std::uint8_t*>(impl_->task_rdram.data()),
            task.command_address & 0x00FF'FFFFU,
            0U,
            true);
        if (!impl_->f3ddkr->complete()) {
            return Rt64ShellError::unsupported_commands;
        }
        if (impl_->cpu_writeback) {
            // RT64's render-to-RAM full sync has finished its readback here.
            // Own whole touched framebuffer extents, including unchanged
            // pixels, so same-value GPU writes cannot hide CPU conflicts.
            for (const auto& [address, framebuffer] : application.state->framebufferManager.framebuffers) {
                (void)address;
                // Skip small offscreen targets (e.g. the 64x64 actor shadow
                // buffers). Writing those back forces RT64 to re-read them
                // from RDRAM at native resolution, which makes shadows blocky.
                // The lens-flare depth check only needs full-size buffers.
                if (framebuffer.width < kMinimumWritebackWidth) {
                    continue;
                }
                if (framebuffer.lastWriteTimestamp > previous_write) {
                    impl_->writeback_ranges.push_back(
                        {framebuffer.addressStart, framebuffer.addressEnd});
                }
            }
            impl_->writeback_pending = true;
        }
        return Rt64ShellError::none;
    }
    catch (...) {
        return Rt64ShellError::renderer_exception;
    }
}

Rt64ShellError Rt64Shell::commit_cpu_writeback(
    const std::span<const std::byte> submitted,
    const std::span<std::byte> live) noexcept {
    if (impl_ == nullptr || !impl_->initialized || !impl_->cpu_writeback ||
        !impl_->writeback_pending)
        return Rt64ShellError::not_initialized;
    const auto result = commit_rt64_rdram_ranges(submitted,
        std::as_bytes(std::span(impl_->rdram)), live, impl_->writeback_ranges);
    if (result == Rt64ShellError::none) {
        impl_->writeback_pending = false;
        impl_->writeback_ranges.clear();
    }
    return result;
}

Rt64ShellError Rt64Shell::present(const bool capture_frame) noexcept {
    if (impl_ == nullptr || !impl_->initialized ||
        impl_->application == nullptr) {
        return Rt64ShellError::not_initialized;
    }
    const Rt64ShellError framebuffer_error = inspect_rt64_framebuffer(
        {
            *impl_->application->core.VI_STATUS_REG,
            *impl_->application->core.VI_ORIGIN_REG,
            *impl_->application->core.VI_WIDTH_REG,
            *impl_->application->core.VI_INTR_REG,
            *impl_->application->core.VI_V_CURRENT_LINE_REG,
            *impl_->application->core.VI_TIMING_REG,
            *impl_->application->core.VI_V_SYNC_REG,
            *impl_->application->core.VI_H_SYNC_REG,
            *impl_->application->core.VI_LEAP_REG,
            *impl_->application->core.VI_H_START_REG,
            *impl_->application->core.VI_V_START_REG,
            *impl_->application->core.VI_V_BURST_REG,
            *impl_->application->core.VI_X_SCALE_REG,
            *impl_->application->core.VI_Y_SCALE_REG,
        },
        impl_->rdram.size(),
        impl_->last_framebuffer);
    if (framebuffer_error != Rt64ShellError::none) {
        return framebuffer_error;
    }
    try {
        if (capture_frame)
            impl_->capture->arm();
        impl_->application->updateScreen();
        if (capture_frame) {
            impl_->application->presentQueue->waitForPresentId(
                impl_->application->state->presentId);
            impl_->application->presentQueue->waitForIdle();
        }
        if (capture_frame && !impl_->capture->finish()) {
            return Rt64ShellError::frame_capture_failed;
        }
        return Rt64ShellError::none;
    }
    catch (...) {
        return Rt64ShellError::renderer_exception;
    }
}

bool Rt64Shell::developer_debugger_available() const noexcept {
    return impl_ != nullptr && impl_->initialized && impl_->developer_mode;
}

std::size_t Rt64Shell::last_command_count() const noexcept {
    return impl_ != nullptr && impl_->f3ddkr != nullptr
        ? impl_->f3ddkr->command_count()
        : 0U;
}

Rt64GraphicsDiagnostics Rt64Shell::last_graphics_diagnostics()
    const noexcept {
    if (impl_ == nullptr || impl_->f3ddkr == nullptr) {
        return {};
    }
    const Rt64F3ddkrStats stats = impl_->f3ddkr->stats();
    // Assign by name: positional aggregate initialization silently
    // misattributes fields when either struct gains or reorders a member.
    Rt64GraphicsDiagnostics diagnostics{};
    diagnostics.matrix_commands = stats.matrix_commands;
    diagnostics.vertex_batches = stats.vertex_batches;
    diagnostics.vertices_loaded = stats.vertices_loaded;
    diagnostics.vertices_finite = stats.vertices_finite;
    diagnostics.vertices_in_clip = stats.vertices_in_clip;
    diagnostics.triangle_batches = stats.triangle_batches;
    diagnostics.triangles_drawn = stats.triangles_drawn;
    diagnostics.dma_display_lists = stats.dma_display_lists;
    diagnostics.color_image_address = stats.color_image_address;
    diagnostics.renderer_paused = impl_->last_submit_paused;
    diagnostics.rejected_command_word0 = stats.rejected_command_word0;
    diagnostics.rejected_command_word1 = stats.rejected_command_word1;
    diagnostics.rejected_command_address = stats.rejected_command_address;
    diagnostics.rejection_reason = stats.rejection_reason;
    diagnostics.rejection_detail = stats.rejection_detail;
    diagnostics.rejection_source_address = stats.rejection_source_address;
    diagnostics.rejection_triangle = stats.rejection_triangle;
    diagnostics.loaded_vertices_low = stats.loaded_vertices_low;
    diagnostics.loaded_vertices_high = stats.loaded_vertices_high;
    diagnostics.full_sync_microseconds = stats.full_sync_microseconds;
    diagnostics.rdram_check_microseconds = impl_->last_rdram_check_microseconds;
    diagnostics.display_list_branches = stats.display_list_branches;
    diagnostics.last_display_list_address = stats.last_display_list_address;
    diagnostics.last_display_list_target = stats.last_display_list_target;
    return diagnostics;
}

Rt64FramebufferInfo Rt64Shell::last_framebuffer() const noexcept {
    return impl_ != nullptr ? impl_->last_framebuffer : Rt64FramebufferInfo{};
}

Rt64FrameView Rt64Shell::last_presented_frame() const noexcept {
    if (impl_ == nullptr || impl_->capture == nullptr) {
        return {};
    }
    const CaptureContext& capture = *impl_->capture;
    return {
        capture.width,
        capture.height,
        capture.row_pitch,
        capture.pixels,
    };
}

const char* rt64_shell_error_message(const Rt64ShellError error) noexcept {
    switch (error) {
    case Rt64ShellError::none:
        return "no error";
    case Rt64ShellError::invalid_window:
        return "RT64 requires a native window";
    case Rt64ShellError::invalid_memory:
        return "RT64 memory or VI registers are invalid";
    case Rt64ShellError::setup_dynamic_libraries_missing:
        return "RT64 dynamic libraries are missing";
    case Rt64ShellError::setup_invalid_graphics_api:
        return "RT64 graphics API is invalid";
    case Rt64ShellError::setup_graphics_api_not_found:
        return "RT64 graphics API is unavailable";
    case Rt64ShellError::setup_graphics_device_not_found:
        return "RT64 graphics device is unavailable";
    case Rt64ShellError::render_hook_unavailable:
        return "RT64 render hook is already owned by another shell";
    case Rt64ShellError::invalid_framebuffer:
        return "RT64 VI framebuffer address or dimensions are invalid";
    case Rt64ShellError::frame_capture_failed:
        return "RT64 presented-frame readback failed";
    case Rt64ShellError::invalid_task_address:
        return "RT64 task contains an out-of-range address";
    case Rt64ShellError::unsupported_microcode:
        return "RT64 does not recognize the submitted microcode";
    case Rt64ShellError::unsupported_commands:
        return "RT64 rejected an unsupported F3DDKR command";
    case Rt64ShellError::not_initialized:
        return "RT64 shell is not initialized";
    case Rt64ShellError::renderer_exception:
        return "RT64 raised an exception";
    case Rt64ShellError::conflicting_cpu_write:
        return "RT64 completion conflicts with CPU memory ownership";
    }
    return "unknown RT64 shell error";
}

}  // namespace jfg
