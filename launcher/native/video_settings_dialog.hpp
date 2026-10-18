#pragma once
#include "../../include/jfg/renderer/video_settings.hpp"
#include <functional>
#include <string>
#include <windows.h>
namespace jfg::frontend {
struct VideoDialogSettings {
  VideoSettings rendering;
  bool fullscreen = false;
  bool pause_inactive = false;
  // 0 retains the current size; 1/2 select 1280x720 / 1920x1080.
  unsigned window_size = 0;
};
struct VideoDialogModel {
  VideoDialogSettings settings;
  std::function<bool(const VideoDialogSettings &, std::wstring &)> apply;
  bool modal = true;
  bool updating = false;
};
// Model lifetime must cover the modal call or the returned modeless window.
INT_PTR show_video_settings(HWND owner, VideoDialogModel &model);
HWND create_video_settings(HWND owner, VideoDialogModel &model);
} // namespace jfg::frontend
