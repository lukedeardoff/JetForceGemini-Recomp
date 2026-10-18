#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../../launcher/native/video_settings_dialog.hpp"
#include "../../launcher/native/video_settings_resource.h"
#include <algorithm>
#include <array>
#include <commctrl.h>
#pragma comment(                                                               \
    linker,                                                                    \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace jfg::frontend {
namespace {
constexpr wchar_t restart_note[] =
    L"Rendering changes apply the next time you start the game.";
void selection(HWND window, int id, int value) {
  SendDlgItemMessageW(window, id, CB_SETCURSEL, static_cast<WPARAM>(value), 0);
}
int selection(HWND window, int id) {
  return static_cast<int>(SendDlgItemMessageW(window, id, CB_GETCURSEL, 0, 0));
}
void choices(HWND window, int id,
             std::initializer_list<const wchar_t *> values) {
  for (const auto *value : values)
    SendDlgItemMessageW(window, id, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(value));
}
void enable_controls(HWND window) {
  const bool custom = selection(window, IDC_VIDEO_FRAME_MODE) == 1;
  for (int id : {IDC_VIDEO_FPS_EDIT, IDC_VIDEO_FPS_SLIDER, IDC_VIDEO_FPS_LABEL})
    EnableWindow(GetDlgItem(window, id), custom);
  EnableWindow(GetDlgItem(window, IDC_VIDEO_WINDOW_SIZE),
               selection(window, IDC_VIDEO_WINDOW_MODE) == 0);
}
void close_dialog(HWND window, VideoDialogModel &model, INT_PTR result) {
  if (model.modal)
    EndDialog(window, result);
  else
    DestroyWindow(window);
}
bool apply(HWND window, VideoDialogModel &model) {
  VideoDialogSettings candidate = model.settings;
  candidate.fullscreen = selection(window, IDC_VIDEO_WINDOW_MODE) == 1;
  candidate.window_size =
      static_cast<unsigned>(selection(window, IDC_VIDEO_WINDOW_SIZE));
  candidate.pause_inactive =
      IsDlgButtonChecked(window, IDC_VIDEO_PAUSE) == BST_CHECKED;
  candidate.rendering.vsync =
      IsDlgButtonChecked(window, IDC_VIDEO_VSYNC) == BST_CHECKED;
  const int mode = selection(window, IDC_VIDEO_FRAME_MODE);
  candidate.rendering.mode = mode == 1   ? FrameRateMode::custom
                             : mode == 2 ? FrameRateMode::unlimited
                                         : FrameRateMode::original;
  {
    std::array<char, 32> number{};
    GetDlgItemTextA(window, IDC_VIDEO_FPS_EDIT, number.data(),
                    static_cast<int>(number.size()));
    const auto value = parse_video_fps(number.data());
    if (!value && mode == 1) {
      SetDlgItemTextW(window, IDC_VIDEO_MESSAGE,
                      L"Enter a whole-number frame rate from 60 through 1000.");
      SetFocus(GetDlgItem(window, IDC_VIDEO_FPS_EDIT));
      SendDlgItemMessageW(window, IDC_VIDEO_FPS_EDIT, EM_SETSEL, 0, -1);
      return false;
    }
    if (value)
      candidate.rendering.fps = *value;
  }
  std::wstring error;
  if (!model.apply || !model.apply(candidate, error)) {
    SetDlgItemTextW(window, IDC_VIDEO_MESSAGE,
                    error.empty() ? L"Could not save video settings. Your "
                                    L"changes have not been applied."
                                  : error.c_str());
    return false;
  }
  model.settings = candidate;
  EnableWindow(GetDlgItem(window, IDC_VIDEO_APPLY), FALSE);
  SetDlgItemTextW(window, IDC_VIDEO_MESSAGE, restart_note);
  return true;
}
INT_PTR CALLBACK procedure(HWND window, UINT message, WPARAM wparam,
                           LPARAM lparam) {
  auto *model = reinterpret_cast<VideoDialogModel *>(
      GetWindowLongPtrW(window, DWLP_USER));
  if (message == WM_INITDIALOG) {
    model = reinterpret_cast<VideoDialogModel *>(lparam);
    SetWindowLongPtrW(window, DWLP_USER, lparam);
    model->updating = true;
    choices(window, IDC_VIDEO_WINDOW_MODE,
            {L"Windowed", L"Borderless fullscreen"});
    choices(window, IDC_VIDEO_WINDOW_SIZE,
            {L"Keep current size", L"1280 x 720", L"1920 x 1080"});
    choices(window, IDC_VIDEO_FRAME_MODE,
            {L"Original", L"Custom", L"Unlimited"});
    selection(window, IDC_VIDEO_WINDOW_MODE,
              model->settings.fullscreen ? 1 : 0);
    selection(window, IDC_VIDEO_WINDOW_SIZE,
              static_cast<int>(model->settings.window_size));
    selection(window, IDC_VIDEO_FRAME_MODE,
              model->settings.rendering.mode == FrameRateMode::custom      ? 1
              : model->settings.rendering.mode == FrameRateMode::unlimited ? 2
                                                                           : 0);
    CheckDlgButton(window, IDC_VIDEO_PAUSE,
                   model->settings.pause_inactive ? BST_CHECKED
                                                  : BST_UNCHECKED);
    CheckDlgButton(window, IDC_VIDEO_VSYNC,
                   model->settings.rendering.vsync ? BST_CHECKED
                                                   : BST_UNCHECKED);
    SendDlgItemMessageW(window, IDC_VIDEO_FPS_SLIDER, TBM_SETRANGE, TRUE,
                        MAKELPARAM(60, 1000));
    SendDlgItemMessageW(window, IDC_VIDEO_FPS_SLIDER, TBM_SETTICFREQ, 120, 0);
    SendDlgItemMessageW(window, IDC_VIDEO_FPS_SLIDER, TBM_SETPAGESIZE, 0, 30);
    SendDlgItemMessageW(window, IDC_VIDEO_FPS_SLIDER, TBM_SETPOS, TRUE,
                        model->settings.rendering.fps);
    SendDlgItemMessageW(window, IDC_VIDEO_FPS_EDIT, EM_SETLIMITTEXT, 4, 0);
    SetDlgItemInt(window, IDC_VIDEO_FPS_EDIT, model->settings.rendering.fps,
                  FALSE);
    enable_controls(window);
    EnableWindow(GetDlgItem(window, IDC_VIDEO_APPLY), FALSE);
    model->updating = false;
    // Center within the owner's monitor work area, including high-DPI desktops.
    RECT owner{}, dialog{};
    GetWindowRect(GetParent(window), &owner);
    GetWindowRect(window, &dialog);
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(
        MonitorFromWindow(GetParent(window), MONITOR_DEFAULTTONEAREST), &info);
    const LONG width = dialog.right - dialog.left,
               height = dialog.bottom - dialog.top;
    const LONG x = std::clamp(
        owner.left + (owner.right - owner.left - width) / 2, info.rcWork.left,
        std::max(info.rcWork.left, info.rcWork.right - width));
    const LONG y = std::clamp(
        owner.top + (owner.bottom - owner.top - height) / 2, info.rcWork.top,
        std::max(info.rcWork.top, info.rcWork.bottom - height));
    SetWindowPos(window, nullptr, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    return TRUE;
  }
  if (!model)
    return FALSE;
  if (message == WM_CLOSE) {
    close_dialog(window, *model, IDCANCEL);
    return TRUE;
  }
  if (message == WM_DPICHANGED) {
    const auto *rect = reinterpret_cast<const RECT *>(lparam);
    SetWindowPos(window, nullptr, rect->left, rect->top,
                 rect->right - rect->left, rect->bottom - rect->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    return TRUE;
  }
  if (message == WM_HSCROLL && reinterpret_cast<HWND>(lparam) ==
                                   GetDlgItem(window, IDC_VIDEO_FPS_SLIDER)) {
    model->updating = true;
    SetDlgItemInt(window, IDC_VIDEO_FPS_EDIT,
                  static_cast<UINT>(SendDlgItemMessageW(
                      window, IDC_VIDEO_FPS_SLIDER, TBM_GETPOS, 0, 0)),
                  FALSE);
    model->updating = false;
    EnableWindow(GetDlgItem(window, IDC_VIDEO_APPLY), TRUE);
    return TRUE;
  }
  if (message == WM_COMMAND) {
    const int id = LOWORD(wparam), notification = HIWORD(wparam);
    if (id == IDCANCEL) {
      close_dialog(window, *model, IDCANCEL);
      return TRUE;
    }
    if (id == IDOK || id == IDC_VIDEO_APPLY) {
      if (apply(window, *model) && id == IDOK)
        close_dialog(window, *model, IDOK);
      return TRUE;
    }
    if (!model->updating &&
        (notification == CBN_SELCHANGE || notification == BN_CLICKED ||
         notification == EN_CHANGE)) {
      enable_controls(window);
      if (id == IDC_VIDEO_FPS_EDIT && notification == EN_CHANGE) {
        BOOL valid = FALSE;
        const UINT fps = GetDlgItemInt(window, id, &valid, FALSE);
        if (valid && fps >= 60 && fps <= 1000)
          SendDlgItemMessageW(window, IDC_VIDEO_FPS_SLIDER, TBM_SETPOS, TRUE,
                              fps);
      }
      EnableWindow(GetDlgItem(window, IDC_VIDEO_APPLY), TRUE);
      SetDlgItemTextW(window, IDC_VIDEO_MESSAGE, restart_note);
      return TRUE;
    }
  }
  return FALSE;
}
void initialize() {
  INITCOMMONCONTROLSEX init{sizeof(init), ICC_BAR_CLASSES};
  InitCommonControlsEx(&init);
}
} // namespace
INT_PTR show_video_settings(HWND owner, VideoDialogModel &model) {
  initialize();
  model.modal = true;
  return DialogBoxParamW(GetModuleHandleW(nullptr),
                         MAKEINTRESOURCEW(IDD_JFG_VIDEO), owner, procedure,
                         reinterpret_cast<LPARAM>(&model));
}
HWND create_video_settings(HWND owner, VideoDialogModel &model) {
  initialize();
  model.modal = false;
  return CreateDialogParamW(GetModuleHandleW(nullptr),
                            MAKEINTRESOURCEW(IDD_JFG_VIDEO), owner, procedure,
                            reinterpret_cast<LPARAM>(&model));
}
} // namespace jfg::frontend
