#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../launcher/native/video_settings_dialog.hpp"
#include "../launcher/native/video_settings_resource.h"
#include <array>
#include <commctrl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <windows.h>
namespace fs = std::filesystem;
bool capture(HWND window, const fs::path &path) {
  RECT rect{};
  GetWindowRect(window, &rect);
  const LONG width = rect.right - rect.left, height = rect.bottom - rect.top;
  HDC screen = GetDC(window), memory = CreateCompatibleDC(screen);
  HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
  HGDIOBJ old = SelectObject(memory, bitmap);
  const bool printed = PrintWindow(window, memory, 0) != 0;
  SelectObject(memory, old);
  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = width;
  info.bmiHeader.biHeight = -height;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;
  std::vector<char> pixels(static_cast<size_t>(width) *
                           static_cast<size_t>(height) * 4);
  const bool copied = GetDIBits(memory, bitmap, 0, static_cast<UINT>(height),
                                pixels.data(), &info, DIB_RGB_COLORS) != 0;
  BITMAPFILEHEADER header{};
  header.bfType = 0x4d42;
  header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
  header.bfSize = header.bfOffBits + static_cast<DWORD>(pixels.size());
  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<const char *>(&header), sizeof(header));
  file.write(reinterpret_cast<const char *>(&info.bmiHeader),
             sizeof(info.bmiHeader));
  file.write(pixels.data(), static_cast<std::streamsize>(pixels.size()));
  DeleteObject(bitmap);
  DeleteDC(memory);
  ReleaseDC(window, screen);
  return printed && copied && bool(file);
}
int main(int argc, char **argv) {
  // All UI is confined to an inactive desktop; never send desktop input or
  // steal focus from an owner's running game/application.
  const std::wstring desktop_name =
      L"JfgVideoTest-" + std::to_wstring(GetCurrentProcessId());
  HDESK desktop = CreateDesktopW(desktop_name.c_str(), nullptr, nullptr, 0,
                                 GENERIC_ALL, nullptr);
  if (!desktop || !SetThreadDesktop(desktop))
    return 2;
  auto setDpi = reinterpret_cast<BOOL(WINAPI *)(HANDLE)>(GetProcAddress(
      GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext"));
  if (setDpi)
    setDpi(reinterpret_cast<HANDLE>(-4));
  const fs::path output =
      argc > 1 ? fs::path(argv[1]) : fs::temp_directory_path() / desktop_name;
  fs::create_directories(output);
  unsigned failures = 0, applies = 0;
  auto check = [&](bool ok, const char *name) {
    if (!ok) {
      ++failures;
      std::cerr << "FAIL: " << name << '\n';
    }
  };
  HWND owner = CreateWindowExW(0, L"STATIC", L"Video test owner",
                               WS_OVERLAPPEDWINDOW, 0, 0, 900, 760, nullptr,
                               nullptr, GetModuleHandleW(nullptr), nullptr);
  jfg::frontend::VideoDialogModel model;
  jfg::frontend::VideoDialogSettings accepted;
  model.apply = [&](const auto &settings, std::wstring &) {
    accepted = settings;
    ++applies;
    return true;
  };
  HWND window = jfg::frontend::create_video_settings(owner, model);
  if (!window)
    return 3;
  check(GetWindow(window, GW_OWNER) == owner, "owned native window");
  std::array<wchar_t, 80> class_name{};
  GetClassNameW(window, class_name.data(), static_cast<int>(class_name.size()));
  check(std::wstring(class_name.data()) == L"#32770", "Windows dialog class");
  check(!IsWindowEnabled(GetDlgItem(window, IDC_VIDEO_FPS_EDIT)),
        "original disables custom controls");
  check(IsDlgButtonChecked(window, IDC_VIDEO_VSYNC) == BST_CHECKED,
        "default VSync on");
  auto choose = [&](int id, int value) {
    SendDlgItemMessageW(window, id, CB_SETCURSEL, static_cast<WPARAM>(value),
                        0);
    SendMessageW(window, WM_COMMAND, MAKEWPARAM(id, CBN_SELCHANGE),
                 reinterpret_cast<LPARAM>(GetDlgItem(window, id)));
  };
  auto apply = [&]() { SendMessageW(window, WM_COMMAND, IDC_VIDEO_APPLY, 0); };
  choose(IDC_VIDEO_FRAME_MODE, 1);
  check(IsWindowEnabled(GetDlgItem(window, IDC_VIDEO_FPS_EDIT)),
        "custom enables number field");
  SetDlgItemTextW(window, IDC_VIDEO_FPS_EDIT, L"360");
  check(SendDlgItemMessageW(window, IDC_VIDEO_FPS_SLIDER, TBM_GETPOS, 0, 0) ==
            360,
        "number updates slider");
  SendDlgItemMessageW(window, IDC_VIDEO_VSYNC, BM_CLICK, 0, 0);
  apply();
  check(applies == 1 && accepted.rendering.fps == 360 &&
            !accepted.rendering.vsync &&
            accepted.rendering.mode == jfg::FrameRateMode::custom,
        "Apply returns requested rendering values");
  check(IsWindow(window), "Apply keeps dialog open");
  SetDlgItemTextW(window, IDC_VIDEO_FPS_EDIT, L"59");
  apply();
  check(applies == 1 && IsWindow(window), "invalid FPS cannot apply");
  SetDlgItemTextW(window, IDC_VIDEO_FPS_EDIT, L"1001");
  apply();
  check(applies == 1, "out-of-range FPS cannot apply");
  SendDlgItemMessageW(window, IDC_VIDEO_FPS_SLIDER, TBM_SETPOS, TRUE, 144);
  SendMessageW(
      window, WM_HSCROLL, TB_THUMBPOSITION,
      reinterpret_cast<LPARAM>(GetDlgItem(window, IDC_VIDEO_FPS_SLIDER)));
  BOOL translated = FALSE;
  check(GetDlgItemInt(window, IDC_VIDEO_FPS_EDIT, &translated, FALSE) == 144 &&
            translated,
        "slider updates number");
  SetDlgItemTextW(window, IDC_VIDEO_FPS_EDIT, L"360");
  ShowWindow(window, SW_SHOWNOACTIVATE);
  UpdateWindow(window);
  check(capture(window, output / L"video-settings.bmp"), "dialog screenshot");
  choose(IDC_VIDEO_FRAME_MODE, 2);
  choose(IDC_VIDEO_WINDOW_MODE, 1);
  SendDlgItemMessageW(window, IDC_VIDEO_PAUSE, BM_CLICK, 0, 0);
  apply();
  check(applies == 2 &&
            accepted.rendering.mode == jfg::FrameRateMode::unlimited &&
            accepted.rendering.fps == 360 && accepted.fullscreen &&
            accepted.pause_inactive,
        "unlimited preserves custom FPS and display controls");
  check(!IsWindowEnabled(GetDlgItem(window, IDC_VIDEO_FPS_EDIT)) &&
            !IsWindowEnabled(GetDlgItem(window, IDC_VIDEO_WINDOW_SIZE)),
        "irrelevant controls disabled");
  choose(IDC_VIDEO_FRAME_MODE, 0);
  SendMessageW(window, WM_COMMAND, IDCANCEL, 0);
  check(applies == 2 && !IsWindow(window) &&
            model.settings.rendering.mode == jfg::FrameRateMode::unlimited,
        "Cancel discards unapplied changes");
  const auto config = output / L"video-settings-unicode-\u03a9.ini";
  {
    std::ofstream file(config, std::ios::binary);
    file << jfg::serialize_video_settings(accepted.rendering);
  }
  auto loaded = jfg::load_video_settings(config);
  check(loaded && loaded->mode == jfg::FrameRateMode::unlimited &&
            loaded->fps == 360 && !loaded->vsync,
        "Unicode-path persistence round trip");
  window = jfg::frontend::create_video_settings(owner, model);
  check(SendDlgItemMessageW(window, IDC_VIDEO_FRAME_MODE, CB_GETCURSEL, 0, 0) ==
            2,
        "reopen retains applied mode");
  model.apply = [](const auto &, std::wstring &error) {
    error = L"Simulated write failure";
    return false;
  };
  SendMessageW(window, WM_COMMAND, IDOK, 0);
  check(IsWindow(window), "save failure prevents OK closing");
  model.apply = [&](const auto &settings, std::wstring &) {
    accepted = settings;
    ++applies;
    return true;
  };
  SendMessageW(window, WM_COMMAND, IDOK, 0);
  check(applies == 3 && !IsWindow(window), "OK saves and closes");
  DestroyWindow(owner);
  std::cout << "Native video dialog checks: " << failures
            << " failures; artifact " << output.string() << '\n';
  return failures ? 1 : 0;
}
