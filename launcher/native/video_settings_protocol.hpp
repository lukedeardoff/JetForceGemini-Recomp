#pragma once
#include "video_settings_dialog.hpp"
#include <array>
#include <cstdint>
namespace jfg::frontend {
// WM_APP + 24 is the runtime-to-launcher display-aspect notification.
inline constexpr UINT kVideoState = WM_APP + 25;
inline constexpr ULONG_PTR kVideoApply = 0x4a564431;
struct VideoRequest {
  std::uint32_t version = 1, mode = 0, fps = 60, vsync = 1;
  std::uint32_t fullscreen = 0, window_size = 0, pause_inactive = 0;
};
inline constexpr std::array<std::array<int, 2>, 5> video_sizes{
    {{0, 0}, {1280, 720}, {1920, 1080}, {1600, 900}, {2560, 1440}}};
inline bool valid_video_request(const VideoRequest &v) {
  return v.version == 1 && v.mode <= 2 && v.fps >= 60 && v.fps <= 1000 &&
         v.vsync <= 1 && v.fullscreen <= 1 && v.pause_inactive <= 1 &&
         v.window_size < video_sizes.size();
}
} // namespace jfg::frontend
