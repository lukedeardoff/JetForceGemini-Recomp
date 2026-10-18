#include "../native_ui.cpp"
#include <stdexcept>
static fs::path home;
static unsigned checks = 0;
static void demand(bool value, const char *why) {
  ++checks;
  if (!value)
    throw std::runtime_error(why);
}
static void settle() {
  for (unsigned i = 0; i < 3; ++i)
    FrontendUiFrame({});
}
static Rml::Element *element(const char *id) {
  auto *e = surface->video->el(id);
  demand(e != nullptr, id);
  return e;
}
static void click(const char *id) {
  auto *e = element(id);
  demand(!e->HasAttribute("disabled"), id);
  e->Click();
  settle();
}
static void pointer(const char *id) {
  auto *e = element(id);
  auto point = e->GetAbsoluteOffset(Rml::BoxArea::Border) +
               Rml::Vector2f(e->GetClientWidth() / 2, e->GetClientHeight() / 2);
  auto *hit = surface->context->GetElementAtPoint(point);
  while (hit && hit != e)
    hit = hit->GetParentNode();
  demand(hit == e, "visible control receives pointer hits");
  surface->context->ProcessMouseMove(int(point.x), int(point.y), 0);
  surface->context->ProcessMouseButtonDown(0, 0);
  surface->context->ProcessMouseButtonUp(0, 0);
  settle();
}
static void type(const std::string &value) {
  auto *e = static_cast<Rml::ElementFormControlInput *>(element("video-fps"));
  e->SetValue(value);
  Rml::Dictionary args;
  args["value"] = value;
  e->DispatchEvent("change", args);
  settle();
}
static void capture(const char *name) {
  auto &s = *surface;
  settle();
  ComPtr<ID3D11Resource> resource;
  s.target->GetResource(&resource);
  ComPtr<ID3D11Texture2D> target;
  check(resource.As(&target));
  D3D11_TEXTURE2D_DESC desc{};
  target->GetDesc(&desc);
  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.MiscFlags = 0;
  ComPtr<ID3D11Texture2D> staging;
  check(s.device->CreateTexture2D(&desc, nullptr, &staging));
  s.gpu->CopyResource(staging.Get(), target.Get());
  D3D11_MAPPED_SUBRESOURCE mapped{};
  check(s.gpu->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped));
  std::ofstream file(home / name, std::ios::binary);
  unsigned char header[18]{};
  header[2] = 2;
  header[12] = static_cast<unsigned char>(desc.Width);
  header[13] = static_cast<unsigned char>(desc.Width >> 8);
  header[14] = static_cast<unsigned char>(desc.Height);
  header[15] = static_cast<unsigned char>(desc.Height >> 8);
  header[16] = 32;
  header[17] = 40;
  file.write(reinterpret_cast<char *>(header), 18);
  for (unsigned y = 0; y < desc.Height; ++y)
    file.write(static_cast<char *>(mapped.pData) + y * mapped.RowPitch,
               desc.Width * 4);
  s.gpu->Unmap(staging.Get(), 0);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
  HWND window = nullptr;
  bool ready = false;
  try {
    int argc = 0;
    auto **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc != 2) {
      LocalFree(argv);
      return 2;
    }
    home = argv[1];
    LocalFree(argv);
    if (fs::exists(home))
      return 3;
    fs::create_directories(home);
    const auto desktopName =
        L"JfgRmlVideoChecks-" + std::to_wstring(GetCurrentProcessId());
    HDESK desktop = CreateDesktopW(desktopName.c_str(), nullptr, nullptr, 0,
                                   GENERIC_ALL, nullptr);
    demand(desktop && SetThreadDesktop(desktop), "isolated inactive desktop");
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSW cls{};
    cls.lpfnWndProc = DefWindowProcW;
    cls.hInstance = instance;
    cls.lpszClassName = L"JfgRmlVideoTests";
    RegisterClassW(&cls);
    window = CreateWindowExW(0, cls.lpszClassName, L"Video Settings",
                             WS_OVERLAPPEDWINDOW, 40, 40, 720, 600, nullptr,
                             nullptr, instance, nullptr);
    const auto profile = home / "profile";
    fs::create_directories(profile);
    std::ofstream(profile / "video.ini")
        << "version=1\nmode=custom\nfps=60\nvsync=1\n";
    auto init = [&] {
      demand(FrontendUiInit(
                 window, profile, [](const std::string &) {}, "video"),
             "RmlUi initializes");
      ready = true;
      surface->video->save = [&](const jfg_video::Ui::Settings &value) {
        std::ofstream(profile / "video.ini")
            << jfg::serialize_video_settings(value.rendering);
        return true;
      };
    };
    init();
    RECT frame{0, 0, 720, 600};
    AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0,
                             GetDpiForWindow(window));
    SetWindowPos(window, nullptr, 40, 40, frame.right - frame.left,
                 frame.bottom - frame.top, SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(window, SW_SHOWNOACTIVATE);
    FrontendUiResize();
    surface->context->SetDensityIndependentPixelRatio(1);
    settle();
    auto &ui = *surface->video;
    {
      std::ofstream coordinates(home / "controls.tsv");
      Rml::ElementList buttons;
      surface->ui.doc->QuerySelectorAll(buttons, "button, input, select");
      for (auto *e : buttons) {
        const auto point =
            e->GetAbsoluteOffset(Rml::BoxArea::Border) +
            Rml::Vector2f(e->GetClientWidth() / 2, e->GetClientHeight() / 2);
        coordinates << e->GetId() << '\t' << point.x << '\t' << point.y << '\n';
      }
    }

    demand(!element("video-restart")->IsVisible(true),
           "no restart chip when no game is running");
    MSG close{};
    auto accepted = [&] {
      demand(PeekMessageW(&close, window, WM_CLOSE, WM_CLOSE, PM_REMOVE) != 0,
             "Apply accepts and closes");
    };
    demand(!element("video-apply")->HasAttribute("disabled"),
           "Apply can accept unchanged values");
    pointer("video-apply");
    accepted();
    pointer("video-preset-360");
    demand(ui.draft.rendering.fps == 360, "360 preset");
    pointer("video-vsync");
    demand(!ui.draft.rendering.vsync, "VSync switch");
    pointer("video-apply");
    accepted();
    auto saved = jfg::load_video_settings(profile / "video.ini");
    demand(saved && saved->fps == 360 && !saved->vsync,
           "Apply persists rendering");
    ui.running = true;
    ui.refresh();
    settle();
    demand(element("video-restart")->IsVisible(true),
           "game-running restart notice");
    type("59");
    demand(ui.invalid() && element("video-apply")->HasAttribute("disabled"),
           "reject underflow");
    type("1001");
    demand(ui.invalid(), "reject overflow");
    type("60.5");
    demand(ui.invalid(), "reject fraction");
    type("");
    demand(ui.invalid(), "reject empty FPS");
    type("144");
    demand(ui.draft.rendering.fps == 144 && !ui.invalid(),
           "exact numeric field");
    Rml::Dictionary change;
    change["value"] = 165.0f;
    element("video-slider")->DispatchEvent("change", change);
    settle();
    demand(ui.fpsText == "165" && ui.draft.rendering.fps == 165,
           "slider syncs exact number");
    pointer("video-slider");
    demand(ui.draft.rendering.fps != 165 && ui.draft.rendering.fps >= 60 &&
               ui.draft.rendering.fps <= 1000,
           "real slider pointer changes cap");
    element("video-fps")->Focus(true);
    surface->context->ProcessKeyDown(Rml::Input::KI_HOME, 0);
    for (unsigned i = 0; i < 4; ++i)
      surface->context->ProcessKeyDown(Rml::Input::KI_DELETE, 0);
    surface->context->ProcessTextInput("240");
    settle();
    demand(ui.fpsText == "240" && ui.draft.rendering.fps == 240,
           "real text widget edits cap");
    pointer("video-preset-165");
    capture("custom.tga");
    pointer("video-unlimited");
    demand(!element("video-custom-controls")->IsVisible(true),
           "Unlimited hides cap controls");
    pointer("video-apply");
    accepted();
    demand(ui.pendingRestart, "saved rendering needs next game session");
    capture("unlimited.tga");
    pointer("video-original");
    demand(ui.draft.rendering.mode == jfg::FrameRateMode::original,
           "Original selectable");
    pointer("video-cancel");
    demand(PeekMessageW(&close, window, WM_CLOSE, WM_CLOSE, PM_REMOVE) != 0,
           "Cancel closes");
    saved = jfg::load_video_settings(profile / "video.ini");
    demand(saved && saved->mode == jfg::FrameRateMode::unlimited &&
               saved->fps == 165,
           "Cancel keeps last Apply and remembered cap");
    FrontendUiShutdown();
    ready = false;
    init();
    settle();
    demand(surface->video->draft.rendering.mode ==
               jfg::FrameRateMode::unlimited,
           "reopen restores saved mode");
    pointer("video-fullscreen");
    demand(element("video-size")->HasAttribute("disabled"),
           "window size disabled in fullscreen");
    pointer("video-windowed");
    demand(!element("video-size")->HasAttribute("disabled"),
           "window size reenabled");
    pointer("video-size");
    auto *size =
        static_cast<Rml::ElementFormControlSelect *>(element("video-size"));
    demand(size->IsSelectBoxVisible(), "window-size list opens");
    demand(surface->video->keyboard(WM_KEYDOWN, VK_ESCAPE) &&
               !size->IsSelectBoxVisible(),
           "Escape dismisses size list first");
    demand(!PeekMessageW(&close, window, WM_CLOSE, WM_CLOSE, PM_REMOVE),
           "dropdown Escape keeps settings open");
    pointer("video-size");
    surface->context->ProcessKeyDown(Rml::Input::KI_DOWN, 0);
    surface->context->ProcessKeyDown(Rml::Input::KI_RETURN, 0);
    settle();
    demand(surface->video->draft.window_size != 0,
           "window-size list supports keyboard selection");
    pointer("video-custom");
    pointer("video-pause");
    demand(surface->video->draft.pause_inactive, "pause switch");
    surface->video->save = [](const auto &) { return false; };
    pointer("video-apply");
    demand(!surface->video->error.empty() && surface->video->dirty(),
           "save failure preserves draft");
    demand(element("video-error")->IsVisible(true) &&
               !PeekMessageW(&close, window, WM_CLOSE, WM_CLOSE, PM_REMOVE),
           "failed Apply stays open with visible error");
    surface->video->save = [&](const auto &value) {
      std::ofstream(profile / "video.ini")
          << jfg::serialize_video_settings(value.rendering);
      return true;
    };
    for (unsigned i = 0; i < 20; ++i)
      demand(surface->video->keyboard(WM_KEYDOWN, VK_TAB),
             "Tab handled inside settings");
    surface->video->error.clear();
    surface->video->draft.window_size = 0; // Capture the reported default label.
    surface->video->refresh();
    for (float scale : {1.0f, 1.25f, 1.5f}) {
      RECT scaled{0, 0, int(720 * scale), int(600 * scale)};
      AdjustWindowRectExForDpi(&scaled, WS_OVERLAPPEDWINDOW, FALSE, 0,
                               GetDpiForWindow(window));
      SetWindowPos(window, nullptr, 0, 0, scaled.right - scaled.left,
                   scaled.bottom - scaled.top,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
      FrontendUiResize();
      surface->context->SetDensityIndependentPixelRatio(scale);
      settle();
      pointer("video-preset-240");
      demand(surface->video->draft.rendering.fps == 240,
             "decorated preset stays clickable at each DPI");
      // Move the hover away so the screenshot shows the saved selection alone.
      surface->context->ProcessMouseMove(0, 0, 0);
      element("video-apply")->Focus(true);
      capture(scale == 1.0f ? "dpi100.tga" : scale == 1.25f ? "dpi125.tga" : "dpi150.tga");
    }
    pointer("video-apply");
    accepted();
    std::ofstream(home / "result.json")
        << "{\"passed\":true,\"checks\":" << checks
        << ",\"physical_desktop_input\":false}";
    FrontendUiShutdown();
    ready = false;
    DestroyWindow(window);
    return 0;
  } catch (const std::exception &e) {
    if (!home.empty())
      std::ofstream(home / "result.json")
          << "{\"passed\":false,\"checks\":" << checks << ",\"error\":\""
          << escape(e.what()) << "\"}";
    if (ready)
      FrontendUiShutdown();
    if (window)
      DestroyWindow(window);
    return 1;
  }
}
