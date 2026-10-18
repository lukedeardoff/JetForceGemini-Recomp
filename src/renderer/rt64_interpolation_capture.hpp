#pragma once
// Optional, bounded screenshots of actual swapchain frames for interpolation
// qualification. GPU readback changes timing; never use captures as a benchmark.
#include "jfg/renderer/interpolation_bridge.hpp"
#include "common/rt64_plume.h"
#include <string>
namespace jfg {
inline std::string interpolation_capture_environment(const char* name) {
#if defined(_MSC_VER)
    char* text = nullptr;
    std::size_t size = 0;
    (void)_dupenv_s(&text, &size, name);
    const std::string result = text == nullptr ? "" : text;
    std::free(text);
    return result;
#else
    const char* text = std::getenv(name);
    return text == nullptr ? "" : text;
#endif
}
struct InterpolationCapture {
    std::unique_ptr<RenderBuffer> buffer;
    std::string filename;
    unsigned width = 0, height = 0, pitch = 0;
};
inline std::unique_ptr<InterpolationCapture> begin_interpolation_capture(
        RenderDevice* device, RenderCommandList* commands, RenderTexture* texture,
        unsigned width, unsigned height, std::uint64_t present, unsigned frame, std::uint64_t scene = 0) {
    static const auto directory = interpolation_capture_environment("JFG_RT64_FRAME_SEQUENCE");
    static const auto first = std::strtoull(
        interpolation_capture_environment("JFG_RT64_CAPTURE_FROM").c_str(), nullptr, 10);
    static const auto firstScene = std::strtoull(
        interpolation_capture_environment("JFG_RT64_CAPTURE_SCENE_FROM").c_str(), nullptr, 10);
    static unsigned captured = 0;
    const bool inRange = firstScene != 0 ?
        (scene >= firstScene && scene-firstScene < 6) :
        (present >= first && present-first < 4);
    if (directory.empty() || !inRange || captured >= 64 || frame >= 16 ||
        width == 0 || height == 0 || width > 8192 || height > 8192) return nullptr;
    ++captured;
    auto result = std::make_unique<InterpolationCapture>();
    result->width = width;
    result->height = height;
    result->pitch = ((width * 4U + 255U) / 256U) * 256U;
    result->filename = directory + "/" + std::to_string(present) + "-" + std::to_string(frame) + ".ppm";
    result->buffer = device->createBuffer(RenderBufferDesc::ReadbackBuffer(
        std::size_t(result->pitch) * height));
    if (!result->buffer) return nullptr;
    commands->barriers(RenderBarrierStage::COPY,
        RenderBufferBarrier(result->buffer.get(), RenderBufferAccess::WRITE));
    commands->barriers(RenderBarrierStage::COPY,
        RenderTextureBarrier(texture, RenderTextureLayout::COPY_SOURCE));
    commands->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(
        result->buffer.get(), RenderFormat::B8G8R8A8_UNORM, width, height, 1U,
        result->pitch / 4U), RenderTextureCopyLocation::Subresource(texture));
    commands->barriers(RenderBarrierStage::ALL,
        RenderTextureBarrier(texture, RenderTextureLayout::COLOR_WRITE));
    return result;
}
inline void finish_interpolation_capture(const std::unique_ptr<InterpolationCapture>& capture) {
    if (!capture) return;
    const RenderRange range(0U, std::size_t(capture->pitch) * capture->height);
    const auto* bytes = static_cast<const unsigned char*>(capture->buffer->map(0U, &range));
    if (!bytes) return;
    std::ofstream output(capture->filename, std::ios::binary);
    output << "P6\n" << capture->width << ' ' << capture->height << "\n255\n";
    for (unsigned y = 0; y < capture->height; ++y)
        for (unsigned x = 0; x < capture->width; ++x) {
            const auto* pixel = bytes + std::size_t(y)*capture->pitch + x*4U;
            const char rgb[] = {static_cast<char>(pixel[2]),
                static_cast<char>(pixel[1]), static_cast<char>(pixel[0])};
            output.write(rgb, 3);
        }
    capture->buffer->unmap();
}
} // namespace jfg
