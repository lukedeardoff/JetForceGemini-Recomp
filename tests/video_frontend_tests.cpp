#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../launcher/native/video_settings_protocol.hpp"
#include "../launcher/native/game_viewport.hpp"
#include "jfg/renderer/video_settings.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <windows.h>
namespace fs = std::filesystem;
struct Search {
  DWORD pid;
  const wchar_t *caption;
  const wchar_t *cls;
  HWND window = nullptr;
};
BOOL CALLBACK find_window(HWND window, LPARAM value) {
  auto &search = *reinterpret_cast<Search *>(value);
  DWORD pid = 0;
  GetWindowThreadProcessId(window, &pid);
  wchar_t caption[128]{}, cls[128]{};
  GetWindowTextW(window, caption, 128);
  GetClassNameW(window, cls, 128);
  if (IsWindowVisible(window) && (search.pid == 0 || pid == search.pid) &&
      (!search.caption || std::wstring(caption) == search.caption) &&
      (!search.cls || std::wstring(cls) == search.cls))
    search.window = window;
  return TRUE;
}
HWND wait_window(HDESK desktop, DWORD pid, const wchar_t *caption,
                 const wchar_t *cls) {
  for (unsigned i = 0; i < 250; ++i) {
    Search search{pid, caption, cls};
    EnumDesktopWindows(desktop, find_window, reinterpret_cast<LPARAM>(&search));
    if (search.window)
      return search.window;
    Sleep(20);
  }
  return nullptr;
}
std::string read(const fs::path &path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), {}};
}
int wmain(int argc, wchar_t **argv) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  if (argc != 4 && argc != 5)
    return 2;
  const bool live = argc == 5;
  const fs::path launcher = argv[1], profile = argv[2];
  std::map<std::string, std::pair<double, double>> coordinates;
  {
    std::ifstream file(argv[3]);
    std::string id;
    double x = 0, y = 0;
    while (file >> id >> x >> y)
      coordinates[id] = {x, y};
  }
  if (coordinates.empty())
    return 2;
  fs::create_directories(profile);
  const std::wstring desktopName =
      L"JfgVideoFrontend-" + std::to_wstring(GetCurrentProcessId());
  HDESK desktop = CreateDesktopW(desktopName.c_str(), nullptr, nullptr, 0,
                                 GENERIC_ALL, nullptr);
  if (!desktop || !SetThreadDesktop(desktop))
    return 3;
  std::wstring command = L"\"" + launcher.wstring() + L"\" --profile \"" +
                         profile.wstring() + L"\"";
  if (live)
    command += L" --play --input-replay \"" + std::wstring(argv[4]) +
               L"\" --progress-output \"" +
               (profile / L"progress.json").wstring() + L"\"";
  auto retraces = [&]() {
    const auto progress = read(profile / L"progress.json");
    const auto start = progress.find("\"vi_retraces\":");
    std::uint64_t value = 0;
    if (start != std::string::npos)
      std::from_chars(progress.data() + start + 14,
                      progress.data() + progress.size(), value);
    return value;
  };
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  startup.lpDesktop = const_cast<wchar_t *>(desktopName.c_str());
  PROCESS_INFORMATION child{};
  if (!CreateProcessW(launcher.c_str(), command.data(), nullptr, nullptr, FALSE,
                      CREATE_UNICODE_ENVIRONMENT, nullptr,
                      launcher.parent_path().c_str(), &startup, &child))
    return 4;
  unsigned failures = 0;
  auto check = [&](bool ok, const char *name) {
    if (!ok) {
      ++failures;
      std::cerr << "FAIL: " << name << '\n';
    }
  };
  HWND frontend =
      wait_window(desktop, child.dwProcessId, nullptr, L"JfgFrontend");
  check(frontend != nullptr, "production launcher window");
  if (frontend) {
    // Exercise the real Video menu's keyboard route, with targeted messages
    // confined to the fixture desktop (no physical keyboard/mouse injection).
    if (live) {
      for (unsigned i = 0; i < 6000 && retraces() < 4000; ++i)
        Sleep(20);
      check(retraces() >= 4000, "live gameplay reaches the camera fixture");
    }
    for (unsigned i = 0; !live && i < 500; ++i) {
      if (read(profile / L"frontend-status.txt").find("Import your ROM once") !=
          std::string::npos)
        break;
      Sleep(20);
    }
    Sleep(300); // Let the launcher consume the initialization completion.
    PostMessageW(frontend, WM_SYSKEYDOWN, 'V', 1LL << 29);
    PostMessageW(frontend, WM_SYSKEYUP, 'V',
                 (1LL << 29) | (1LL << 30) | (1LL << 31));
    Sleep(250);
    PostMessageW(frontend, WM_KEYDOWN, VK_HOME, 0);
    PostMessageW(frontend, WM_KEYUP, VK_HOME, 0);
    PostMessageW(frontend, WM_KEYDOWN, VK_RETURN, 0);
    PostMessageW(frontend, WM_KEYUP, VK_RETURN, 0);
    HWND dialog = wait_window(desktop, 0, L"Video Settings", L"JfgRmlLiveTool");
    check(dialog != nullptr, "Video menu opens separate RmlUi window");
    auto checkPlacement = [&] {
      if (!dialog)
        return;
      RECT owner{}, actual{};
      MONITORINFO monitor{sizeof(MONITORINFO)};
      const bool measured = GetWindowRect(frontend, &owner) &&
                            GetWindowRect(dialog, &actual) &&
                            GetMonitorInfoW(MonitorFromWindow(
                                frontend, MONITOR_DEFAULTTONEAREST), &monitor);
      check(measured, "initial window rectangles available");
      if (!measured)
        return;
      const auto &work = monitor.rcWork;
      const LONG width = actual.right - actual.left;
      const LONG height = actual.bottom - actual.top;
      const LONG centerX = owner.left + (owner.right - owner.left - width) / 2;
      const LONG centerY = owner.top + (owner.bottom - owner.top - height) / 2;
      const LONG expectedX = std::max(work.left, std::min(centerX, work.right - width));
      const LONG expectedY = std::max(work.top, std::min(centerY, work.bottom - height));
      std::cout << "Initial placement: owner=" << owner.left << ',' << owner.top
                << ',' << owner.right << ',' << owner.bottom << " dialog="
                << actual.left << ',' << actual.top << ',' << actual.right << ','
                << actual.bottom << " expected=" << expectedX << ',' << expectedY
                << " dpi=" << GetDpiForWindow(dialog) << std::endl;
      check(std::abs(actual.left - expectedX) <= 1 &&
                std::abs(actual.top - expectedY) <= 1,
            "settings initially centered over owner and clamped to work area");
      check(actual.left >= work.left && actual.top >= work.top &&
                actual.right <= work.right && actual.bottom <= work.bottom,
            "initial settings window fits owner monitor work area");
      check(GetDpiForWindow(dialog) == GetDpiForWindow(frontend),
            "settings starts at owner monitor DPI");
    };
    auto prepareDialog = [&] {
      checkPlacement();
      if (!dialog)
        return;
      const auto dpi = GetDpiForWindow(dialog);
      RECT before{};
      GetClientRect(dialog, &before);
      std::cout << "Initial video client " << before.right << " x "
                << before.bottom << " dpi=" << dpi << std::endl;
      RECT frame{0, 0, MulDiv(720, dpi, 96), MulDiv(600, dpi, 96)};
      AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
      SetWindowPos(dialog, nullptr, 20, 20, frame.right - frame.left,
                   frame.bottom - frame.top, SWP_NOZORDER | SWP_NOACTIVATE);
      Sleep(250);
      RECT after{};
      GetClientRect(dialog, &after);
      check(after.right == MulDiv(720, dpi, 96) &&
                after.bottom == MulDiv(600, dpi, 96),
            "fixture client matches measured RmlUi coordinates");
    };
    prepareDialog();
    auto waitClosed = [&] {
      // This fixture desktop stays inactive. Accepting Pause when inactive
      // therefore preserves that pause reason after the modal window closes.
      const bool expectPause = read(profile / L"frontend-pause-inactive.txt") == "1";
      for (unsigned i = 0;
           i < 250 && (IsWindow(dialog) || !IsWindowEnabled(frontend) ||
                       (GetPropW(frontend, L"JfgFrontendPause") != nullptr) != expectPause);
           ++i)
        Sleep(20);
      check(!IsWindow(dialog) && IsWindowEnabled(frontend) &&
                (GetPropW(frontend, L"JfgFrontendPause") != nullptr) == expectPause,
            "closing restores owner and honors pause-inactive preference");
    };
    if (dialog) {
      Sleep(300);
      check(GetWindow(dialog, GW_OWNER) == frontend &&
                !IsWindowEnabled(frontend),
            "owned RmlUi window disables launcher");
      check(GetPropW(frontend, L"JfgFrontendPause") != nullptr,
            "video settings request pause");
      if (live) {
        for (unsigned i = 0;
             i < 100 && !GetPropW(frontend, L"JfgFrontendPaused"); ++i)
          Sleep(20);
        check(GetPropW(frontend, L"JfgFrontendPaused") != nullptr,
              "runtime acknowledges RmlUi settings pause");
        Sleep(200);
        const auto paused = retraces();
        Sleep(300);
        check(paused > 0 && paused == retraces(),
              "VI clock is stopped during settings");
        // The native aspect notification must reach the launcher even while
        // its separate Video Settings process queries current window state.
        Search renderSearch{0, nullptr, L"JfgPhase8LiveRt64Window"};
        EnumChildWindows(frontend, find_window,
                         reinterpret_cast<LPARAM>(&renderSearch));
        const HWND render = renderSearch.window;
        check(render != nullptr, "live render child available for aspect changes");
        if (render) {
          const auto oldWidth = reinterpret_cast<WPARAM>(
              GetPropW(render, L"JfgGameAspectWidth"));
          const auto oldHeight = reinterpret_cast<LPARAM>(
              GetPropW(render, L"JfgGameAspectHeight"));
          for (int height : {180, 240}) {
            SendMessageW(render, WM_APP + 22, 320, height);
            bool matches = false;
            for (unsigned attempt = 0; attempt < 100; ++attempt) {
              RECT parent{}, actual{};
              GetClientRect(GetParent(render), &parent);
              GetClientRect(render, &actual);
              const auto wanted = jfg::frontend::game_viewport(
                  parent.right, parent.bottom, 320, height);
              matches = actual.right == wanted.width &&
                        actual.bottom == wanted.height;
              if (matches) break;
              Sleep(10);
            }
            check(matches, height == 180
                ? "widescreen notification relayouts render child"
                : "4:3 notification restores render child");
            check((SendMessageW(frontend, jfg::frontend::kVideoState, 0, 0) & 4) != 0,
                  "video state query remains distinct from aspect notification");
          }
          SendMessageW(render, WM_APP + 22,
                       oldWidth ? oldWidth : 320,
                       oldHeight ? oldHeight : 240);
        }

      }
      auto click = [&](const char *id) {
        const auto found = coordinates.find(id);
        check(found != coordinates.end(), id);
        if (found == coordinates.end())
          return;
        const double dpi = GetDpiForWindow(dialog) / 96.0;
        const LPARAM position =
            MAKELPARAM(int(std::lround(found->second.first * dpi)),
                       int(std::lround(found->second.second * dpi)));
        SendMessageW(dialog, WM_MOUSEMOVE, 0, position);
        SendMessageW(dialog, WM_LBUTTONDOWN, MK_LBUTTON, position);
        SendMessageW(dialog, WM_LBUTTONUP, 0, position);
        Sleep(100);
      };
      auto reopen = [&] {
        PostMessageW(frontend, WM_COMMAND, 116, 0);
        dialog = wait_window(desktop, 0, L"Video Settings", L"JfgRmlLiveTool");
        check(dialog != nullptr, "settings reopens after accepting Apply");
        prepareDialog();
      };
      click("video-custom");
      click("video-preset-360");
      click("video-vsync");
      click("video-apply");
      waitClosed();
      auto saved = jfg::load_video_settings(profile / L"video.ini");
      check(saved && saved->mode == jfg::FrameRateMode::custom &&
                saved->fps == 360 && !saved->vsync,
            "actual RmlUi Apply persists custom FPS and VSync");
      if (saved)
        std::cout << "After Apply: " << jfg::serialize_video_settings(*saved);
      reopen();
      click("video-fullscreen");
      click("video-pause");
      click("video-apply");
      waitClosed();
      check((GetWindowLongPtrW(frontend, GWL_STYLE) & WS_POPUP) != 0,
            "fullscreen applies to live owner immediately");
      check(read(profile / L"frontend-pause-inactive.txt") == "1",
            "pause preference persists immediately");
      reopen();
      click("video-windowed");
      click("video-pause");
      click("video-apply");
      waitClosed();
      check((GetWindowLongPtrW(frontend, GWL_STYLE) & WS_OVERLAPPEDWINDOW) ==
                WS_OVERLAPPEDWINDOW,
            "windowed applies without game restart");
      reopen();
      const auto before = read(profile / L"video.ini");
      click("video-unlimited");
      click("video-cancel");
      waitClosed();
      check(read(profile / L"video.ini") == before,
            "Cancel discards unsaved mode");
      if (live) {
        for (unsigned i = 0;
             i < 100 && GetPropW(frontend, L"JfgFrontendPaused"); ++i)
          Sleep(20);
        check(!GetPropW(frontend, L"JfgFrontendPaused"),
              "runtime acknowledges resume");
        const auto resumed = retraces();
        for (unsigned i = 0; i < 200 && retraces() <= resumed; ++i)
          Sleep(20);
        check(retraces() > resumed, "VI clock resumes");
      }
      PostMessageW(frontend, WM_COMMAND, 116, 0);
      dialog = wait_window(desktop, 0, L"Video Settings", L"JfgRmlLiveTool");
      check(dialog != nullptr, "RmlUi settings reopen");
      if (dialog) {
        prepareDialog();
        click("video-unlimited");
        click("video-apply");
        waitClosed();
      }
      saved = jfg::load_video_settings(profile / L"video.ini");
      check(saved && saved->mode == jfg::FrameRateMode::unlimited &&
                saved->fps == 360 && !saved->vsync,
            "Apply persists Unlimited with remembered custom cap");
      // Verify recovery if only the supervised settings process fails.
      PostMessageW(frontend, WM_COMMAND, 116, 0);
      dialog = wait_window(desktop, 0, L"Video Settings", L"JfgRmlLiveTool");
      if (dialog) {
        prepareDialog();
        DWORD pid = 0;
        GetWindowThreadProcessId(dialog, &pid);
        HANDLE process =
            OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
        check(process != nullptr, "fixture owns settings process");
        if (process) {
          TerminateProcess(process, 7);
          WaitForSingleObject(process, 3000);
          CloseHandle(process);
        }
        waitClosed();
      }
    }

    if (!live) {
      MONITORINFO monitor{sizeof(MONITORINFO)};
      check(GetMonitorInfoW(MonitorFromWindow(frontend, MONITOR_DEFAULTTONEAREST),
                            &monitor) != FALSE,
            "placement fixture monitor available");
      const auto &work = monitor.rcWork;
      for (int position = 0; position < 3; ++position) {
        RECT owner{};
        GetWindowRect(frontend, &owner);
        const LONG width = owner.right - owner.left;
        const LONG height = owner.bottom - owner.top;
        const LONG x = position == 0 ? work.left - width / 3
                       : position == 1 ? work.right - 2 * width / 3
                                       : work.left + (work.right - work.left - width) / 2;
        const LONG y = position == 0 ? work.top - height / 3
                       : position == 1 ? work.bottom - 2 * height / 3
                                       : work.top + (work.bottom - work.top - height) / 2;
        SetWindowPos(frontend, nullptr, x, y, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        PostMessageW(frontend, WM_COMMAND, 116, 0);
        dialog = wait_window(desktop, 0, L"Video Settings", L"JfgRmlLiveTool");
        check(dialog != nullptr, "settings opens after moving launcher");
        if (dialog) {
          checkPlacement();
          PostMessageW(dialog, WM_CLOSE, 0, 0);
          waitClosed();
        }
      }
    }
    PostMessageW(frontend, WM_CLOSE, 0, 0);
  }
  const DWORD ended = WaitForSingleObject(child.hProcess, 10000);
  check(ended == WAIT_OBJECT_0, "launcher shuts down after dialog use");
  if (ended != WAIT_OBJECT_0)
    TerminateProcess(child.hProcess, 9);
  CloseHandle(child.hThread);
  CloseHandle(child.hProcess);
  std::cout << "RmlUi launcher video integration: " << failures
            << " failures\n";
  return failures ? 1 : 0;
}
