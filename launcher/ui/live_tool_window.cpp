#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "native_ui.hpp"
#include <algorithm>
#include <filesystem>
#include <shellapi.h>
#include <string>
namespace fs = std::filesystem;

namespace {
struct LiveWindow {
  HWND window = nullptr, owner = nullptr;
  HANDLE worker = nullptr, job = nullptr;
  std::filesystem::path profile, helper;
  std::string kind;
  ULONGLONG token = 0;
  bool ready = false;
} live;
std::wstring quoteLive(const std::wstring &value) {
  std::wstring out = L"\"";
  size_t slashes = 0;
  for (wchar_t c : value) {
    if (c == L'\\') {
      slashes++;
      continue;
    }
    out.append(slashes * (c == L'\"' ? 2 : 1), L'\\');
    slashes = 0;
    if (c == L'\"')
      out += L'\\';
    out += c;
  }
  out.append(slashes * 2, L'\\');
  return out + L"\"";
}
void notifyModal() {
  if (IsWindow(live.owner))
    PostMessageW(live.owner, WM_APP + 22, reinterpret_cast<WPARAM>(live.window),
                 0);
}
LRESULT CALLBACK liveProcedure(HWND w, UINT m, WPARAM p, LPARAM l) {
  if (live.ready && ((m >= WM_MOUSEFIRST && m <= WM_MOUSELAST) ||
                     m == WM_KEYDOWN || m == WM_KEYUP || m == WM_SYSKEYDOWN ||
                     m == WM_SYSKEYUP || m == WM_CHAR || m == WM_ACTIVATEAPP || m == WM_KILLFOCUS))
    FrontendUiMessage(m, p, l);
  // F10 and our Alt mnemonics belong to RmlUi, not DefWindowProc's menu loop.
  if (live.ready &&
      (((m == WM_KEYDOWN || m == WM_KEYUP) && p == VK_F10) ||
       ((m == WM_SYSKEYDOWN || m == WM_SYSKEYUP) && (p == 'V' || p == 'S' || p == 'T'))))
    return 0;
  switch (m) {
  case WM_ERASEBKGND:
    return 1;
  case WM_GETMINMAXINFO: {
    auto *info = reinterpret_cast<MINMAXINFO *>(l);
    int dpi = static_cast<int>(GetDpiForWindow(w));
    info->ptMinTrackSize = {MulDiv(live.kind == "video" ? 680 : live.kind == "map" || live.kind == "controllers" ? 720 : 440, dpi, 96),
                            MulDiv(live.kind == "map" ? 500 : 540, dpi, 96)};
    return 0;
  }
  case WM_SIZE:
    if (live.ready)
      FrontendUiResize();
    return 0;
  case WM_DPICHANGED: {
    auto *r = reinterpret_cast<RECT *>(l);
    SetWindowPos(w, nullptr, r->left, r->top, r->right - r->left,
                 r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
    if (live.ready)
      FrontendUiResize();
    return 0;
  }
  case WM_TIMER:
    if (live.ready)
      FrontendUiFrame({});
    return 0;
  case WM_PAINT: {
    PAINTSTRUCT paint;
    BeginPaint(w, &paint);
    EndPaint(w, &paint);
    if (live.ready)
      FrontendUiFrame({});
    return 0;
  }
  case WM_CLOSE:
    DestroyWindow(w);
    return 0;
  case WM_DESTROY:
    if(live.kind=="video" && IsWindow(live.owner))EnableWindow(live.owner,TRUE);
    RemovePropW(w, L"JfgLiveModal");
    notifyModal();
    KillTimer(w, 1);
    if (live.ready) {
      live.ready = false;
      FrontendUiShutdown();
    }
    PostQuitMessage(0);
    return 0;
  default:
    break;
  }
  return DefWindowProcW(w, m, p, l);
}
} // namespace
int FrontendLiveToolEntry(HINSTANCE instance, int show) {
  int argc = 0;
  auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  bool found = false;
  for (int i = 1; i < argc; i++)
    if (std::wstring(argv[i]) == L"--live-tool" && i + 1 < argc) {
      auto name = std::wstring(argv[++i]);
      live.kind = name == L"map"         ? "map"
                  : name == L"inventory" ? "inventory"
                  : name == L"controllers" ? "controllers"
                  : name == L"video" ? "video"
                                         : "";
      found = true;
    } else if (std::wstring(argv[i]) == L"--profile" && i + 1 < argc)
      live.profile = argv[++i];
    else if (std::wstring(argv[i]) == L"--helper" && i + 1 < argc)
      live.helper = argv[++i];
    else if (std::wstring(argv[i]) == L"--owner" && i + 1 < argc)
      live.owner = reinterpret_cast<HWND>(_wcstoui64(argv[++i], nullptr, 10));
  LocalFree(argv);
  if (!found)
    return -1;
  if (live.kind.empty() || live.profile.empty() || live.helper.empty())
    return 2;
  auto dpi = reinterpret_cast<BOOL(WINAPI *)(HANDLE)>(GetProcAddress(
      GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext"));
  if (dpi)
    dpi(reinterpret_cast<HANDLE>(-4));
  std::error_code error;
  std::filesystem::create_directories(live.profile, error);
  if (error)
    return 2;
  WNDCLASSW cls{};
  cls.lpfnWndProc = liveProcedure;
  cls.hInstance = instance;
  cls.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
  cls.lpszClassName = L"JfgRmlLiveTool";
  cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  cls.hbrBackground = CreateSolidBrush(RGB(11, 15, 23));
  RegisterClassW(&cls);
  int density = static_cast<int>(IsWindow(live.owner) ? GetDpiForWindow(live.owner) : GetDpiForSystem());
  int initialX = CW_USEDEFAULT, initialY = CW_USEDEFAULT;
  RECT size{0, 0, MulDiv(live.kind == "video" ? 720 : live.kind == "map" ? 1200 : live.kind == "controllers" ? 1280 : 486, density, 96),
            MulDiv(live.kind == "video" ? 600 : live.kind == "map" ? 740 : live.kind == "controllers" ? 920 : 716, density, 96)};
  AdjustWindowRectExForDpi(&size, WS_OVERLAPPEDWINDOW, FALSE, 0, density);
  if(live.kind=="controllers" || live.kind=="video") {
    MONITORINFO monitor{sizeof(MONITORINFO)};
    if(GetMonitorInfoW(MonitorFromWindow(live.owner,MONITOR_DEFAULTTONEAREST),&monitor)) {
      const auto& work=monitor.rcWork;
      size.right=size.left+std::min<LONG>(size.right-size.left,work.right-work.left-32);
      size.bottom=size.top+std::min<LONG>(size.bottom-size.top,work.bottom-work.top-48);
      RECT anchor=work;
      if(IsWindow(live.owner) && !IsIconic(live.owner))GetWindowRect(live.owner,&anchor);
      const LONG width=size.right-size.left,height=size.bottom-size.top;
      initialX=std::clamp<LONG>(anchor.left+(anchor.right-anchor.left-width)/2,work.left,work.right-width);
      initialY=std::clamp<LONG>(anchor.top+(anchor.bottom-anchor.top-height)/2,work.top,work.bottom-height);
    }
  }
  live.window = CreateWindowExW(
      0, cls.lpszClassName,
      live.kind == "video" ? L"Video Settings" : live.kind == "map" ? L"JFG Live Map" : live.kind == "controllers" ? L"JFG Controller Mapping" : L"JFG Live Inventory",
      WS_OVERLAPPEDWINDOW, initialX, initialY, size.right - size.left,
      size.bottom - size.top, (live.kind=="controllers" || live.kind=="video")?live.owner:nullptr, nullptr, instance, nullptr);
  if (!live.window)
    return 3;
  SetPropW(live.window, L"JfgLiveOwner", live.owner);
  live.token = GetTickCount64();
  SetPropW(live.window, L"JfgLiveToken", reinterpret_cast<HANDLE>(live.token));
  live.ready = FrontendUiInit(
      live.window, live.profile,
      [](const std::string &action) {
        if (action == "modal-open" || action == "modal-close")
          notifyModal();
        else if (action == "inventory") {
          if (IsWindow(live.owner))
            PostMessageW(live.owner, WM_APP + 23, 0, 0);
          else {
            wchar_t executable[32768]{};
            GetModuleFileNameW(nullptr, executable, 32768);
            auto command = quoteLive(executable) +
                           L" --live-tool inventory --profile " +
                           quoteLive(live.profile.wstring()) + L" --helper " +
                           quoteLive(live.helper.wstring());
            STARTUPINFOW startup{};
            startup.cb = sizeof(startup);
            PROCESS_INFORMATION child{};
            if (CreateProcessW(executable, command.data(), nullptr, nullptr,
                               FALSE, 0, nullptr, nullptr, &startup, &child)) {
              CloseHandle(child.hThread);
              CloseHandle(child.hProcess);
            }
          }
        }
      },
      live.kind);
  if (!live.ready) {
    DestroyWindow(live.window);
    return 3;
  }
  if(live.kind!="controllers" && live.kind!="video") {
  std::wstring command =
      quoteLive(live.helper.wstring()) + L" --frontend-worker " +
      (live.kind == "map" ? std::wstring(L"map-data")
                          : std::wstring(L"inventory-data")) +
      L" " + quoteLive(live.profile.wstring()) + L" " +
      std::to_wstring(reinterpret_cast<std::uintptr_t>(live.window));
  STARTUPINFOW start{};
  start.cb = sizeof(start);
  PROCESS_INFORMATION child{};
  live.job = CreateJobObjectW(nullptr, nullptr);
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
  limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (!live.job ||
      !SetInformationJobObject(live.job, JobObjectExtendedLimitInformation,
                               &limits, sizeof(limits)) ||
      !CreateProcessW(live.helper.c_str(), command.data(), nullptr, nullptr,
                      FALSE, CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr,
                      live.helper.parent_path().c_str(), &start, &child)) {
    MessageBoxW(live.window,
                L"Cannot start the live data service. Rebuild the launcher or "
                L"extract its complete package.",
                L"JFG Live Tools", MB_OK | MB_ICONERROR);
    DestroyWindow(live.window);
    if (live.job)
      CloseHandle(live.job);
    return 4;
  }
  if (!AssignProcessToJobObject(live.job, child.hProcess)) {
    TerminateProcess(child.hProcess, 1);
    CloseHandle(child.hThread);
    CloseHandle(child.hProcess);
    CloseHandle(live.job);
    DestroyWindow(live.window);
    return 4;
  }
  live.worker = child.hProcess;
  ResumeThread(child.hThread);
  CloseHandle(child.hThread);
  }
  if(live.kind=="video")EnableWindow(live.owner,FALSE);
  if(live.kind=="controllers" || live.kind=="video"){SetPropW(live.window,L"JfgLiveModal",reinterpret_cast<HANDLE>(1));notifyModal();}
  FrontendUiFrame({});
  ShowWindow(live.window, show);
  UpdateWindow(live.window);
  SetTimer(live.window, 1, 16, nullptr);
  MSG message{};
  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  if(live.worker) {
  // The service sees its HWND disappear and stops any route before shutdown.
  if (WaitForSingleObject(live.worker, 1500) == WAIT_TIMEOUT)
    TerminateProcess(live.worker, 0);
  CloseHandle(live.worker);
  CloseHandle(live.job);
  auto transfer =
      live.profile /
      ("live-tool-" +
       std::to_string(reinterpret_cast<std::uintptr_t>(live.window)) + "-" +
       std::to_string(live.token));
  std::error_code cleanupError;
  if (fs::weakly_canonical(transfer.parent_path(), cleanupError) ==
      fs::weakly_canonical(live.profile, cleanupError)) {
    for (fs::directory_iterator it(transfer, cleanupError), end;
         !cleanupError && it != end; it.increment(cleanupError)) {
      auto name = it->path().filename().string();
      if (it->is_regular_file(cleanupError) && !it->is_symlink(cleanupError) &&
          (name == "snapshot.bin" || name == "snapshot.bin.tmp" ||
           name.rfind("command-", 0) == 0 || name.rfind("asset-", 0) == 0))
        fs::remove(it->path(), cleanupError);
    }
    fs::remove(transfer, cleanupError);
  }
  }
  DeleteObject(cls.hbrBackground);
  return static_cast<int>(message.wParam);
}
