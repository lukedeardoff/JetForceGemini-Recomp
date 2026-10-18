#pragma once
#include "../native/video_settings_protocol.hpp"
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <string>
namespace jfg_video {
struct Ui final : Rml::EventListener {
  using Settings = jfg::frontend::VideoDialogSettings;
  HWND window = nullptr, owner = nullptr;
  Rml::ElementDocument *doc = nullptr;
  Settings saved{}, draft{};
  bool updating = false, failedLoad = false, running = false,
       pendingRestart = false;
  std::string fpsText = "60", error;
  std::function<bool(const Settings &)> save;
  Rml::Element *el(const char *id) const { return doc->GetElementById(id); }
  void text(const char *id, const std::string &s) { el(id)->SetInnerRML(s); }
  static bool sameRender(const jfg::VideoSettings &a,
                         const jfg::VideoSettings &b) {
    return a.mode == b.mode && a.vsync == b.vsync &&
           (a.mode != jfg::FrameRateMode::custom || a.fps == b.fps);
  }
  bool dirty() const {
    return draft.fullscreen != saved.fullscreen ||
           draft.window_size != saved.window_size ||
           draft.pause_inactive != saved.pause_inactive ||
           !sameRender(draft.rendering, saved.rendering) ||
           draft.rendering.fps != saved.rendering.fps ||
           fpsText != std::to_string(saved.rendering.fps);
  }
  bool invalid() const {
    return draft.rendering.mode == jfg::FrameRateMode::custom &&
           !jfg::parse_video_fps(fpsText);
  }
  void init(HWND w, Rml::ElementDocument *d,
            const std::filesystem::path &profile) {
    window = w;
    doc = d;
    owner = reinterpret_cast<HWND>(GetPropW(w, L"JfgLiveOwner"));
    const auto loaded = jfg::load_video_settings(profile / L"video.ini");
    failedLoad = !loaded;
    if (loaded)
      saved.rendering = *loaded;
    DWORD_PTR state = 0;
    if (IsWindow(owner))
      SendMessageTimeoutW(owner, jfg::frontend::kVideoState, 0, 0,
                          SMTO_ABORTIFHUNG, 1000, &state);
    saved.fullscreen = (state & 1) != 0;
    saved.pause_inactive = (state & 2) != 0;
    running = (state & 4) != 0;
    draft = saved;
    fpsText = std::to_string(draft.rendering.fps);
    save = [this](const Settings &candidate) {
      if (!IsWindow(owner))
        return false;
      jfg::frontend::VideoRequest request;
      request.mode = static_cast<unsigned>(candidate.rendering.mode);
      request.fps = candidate.rendering.fps;
      request.vsync = candidate.rendering.vsync;
      request.fullscreen = candidate.fullscreen;
      request.window_size = candidate.window_size;
      request.pause_inactive = candidate.pause_inactive;
      COPYDATASTRUCT packet{jfg::frontend::kVideoApply, sizeof(request),
                            &request};
      DWORD_PTR result = 0;
      return SendMessageTimeoutW(owner, WM_COPYDATA,
                                 reinterpret_cast<WPARAM>(window),
                                 reinterpret_cast<LPARAM>(&packet),
                                 SMTO_ABORTIFHUNG, 5000, &result) &&
             result == 1;
    };
    doc->AddEventListener("click", this);
    doc->AddEventListener("change", this);
    if (failedLoad)
      error = "Could not read video.ini. Check your save folder before "
              "changing settings.";
    refresh(true);
  }
  void selected(const char *id, bool on) {
    el(id)->SetClass("selected", on);
    el(id)->SetAttribute("aria-checked", on ? "true" : "false");
  }
  void refresh(bool input = false) {
    updating = true;
    selected("video-windowed", !draft.fullscreen);
    selected("video-fullscreen", draft.fullscreen);
    selected("video-pause", draft.pause_inactive);
    selected("video-vsync", draft.rendering.vsync);
    selected("video-original",
             draft.rendering.mode == jfg::FrameRateMode::original);
    selected("video-custom",
             draft.rendering.mode == jfg::FrameRateMode::custom);
    selected("video-unlimited",
             draft.rendering.mode == jfg::FrameRateMode::unlimited);
    auto *size = static_cast<Rml::ElementFormControlSelect *>(el("video-size"));
    size->SetValue(std::to_string(draft.window_size));
    size->SetDisabled(draft.fullscreen || failedLoad);
    text("video-size-hint",
         draft.fullscreen ? "Fills the screen" : "Windowed mode only");
    const bool custom = draft.rendering.mode == jfg::FrameRateMode::custom;
    el("video-custom-controls")
        ->SetProperty("display", custom ? "block" : "none");
    if (input)
      static_cast<Rml::ElementFormControlInput *>(el("video-fps"))
          ->SetValue(fpsText);
    static_cast<Rml::ElementFormControlInput *>(el("video-slider"))
        ->SetValue(std::to_string(draft.rendering.fps));
    for (unsigned n : {60U, 120U, 144U, 165U, 240U, 360U}) {
      const auto id = "video-preset-" + std::to_string(n);
      el(id.c_str())->SetClass("selected", draft.rendering.fps == n);
    }
    const bool bad = invalid();
    el("video-fps")->SetClass("invalid", bad);
    el("video-range")->SetClass("invalid", bad);
    text("video-range",
         bad ? "Enter a whole number from 60 to 1000" : "Range 60 to 1000 FPS");
    std::string hint;
    if (draft.rendering.mode == jfg::FrameRateMode::original)
      hint = "Uses the game's original presentation rate.";
    else if (custom)
      hint = "Experimental smooth rendering up to your cap.";
    else
      hint = "Experimental smooth rendering with no FPS cap.";
    if (draft.rendering.mode != jfg::FrameRateMode::original &&
        draft.rendering.vsync)
      hint = "Experimental smooth rendering. VSync limits output to your "
             "display's refresh rate.";
    text("video-fps-hint", hint);
    const bool renderDirty = !sameRender(draft.rendering, saved.rendering);
    el("video-restart")->SetProperty("display", running ? "block" : "none");
    el("video-restart")->SetClass("pending", renderDirty || pendingRestart);
    text("video-restart", renderDirty || pendingRestart
                              ? "Restart game to apply"
                              : "Applies next game launch");
    text("video-error", error);
    el("video-error")->SetProperty("display", error.empty() ? "none" : "block");
    if (bad || failedLoad)
      el("video-apply")->SetAttribute("disabled", true);
    else
      el("video-apply")->RemoveAttribute("disabled");
    updating = false;
  }
  bool apply() {
    if (failedLoad || invalid()) {
      refresh();
      return false;
    }
    if (!dirty())
      return true;
    const bool renderingChanged = !sameRender(draft.rendering, saved.rendering);
    if (!save || !save(draft)) {
      error =
          "Could not save video settings. Check access to your save folder.";
      refresh();
      return false;
    }
    pendingRestart |= running && renderingChanged;
    saved = draft;
    fpsText = std::to_string(draft.rendering.fps);
    error.clear();
    refresh(true);
    return true;
  }
  void ProcessEvent(Rml::Event &event) override {
    if (updating)
      return;
    auto *target = event.GetTargetElement();
    while (target && target->GetId().empty())
      target = target->GetParentNode();
    if (!target)
      return;
    const auto id = target->GetId();
    if (event.GetType() == "change") {
      if (id == "video-fps") {
        fpsText =
            static_cast<Rml::ElementFormControlInput *>(target)->GetValue();
        if (auto n = jfg::parse_video_fps(fpsText))
          draft.rendering.fps = *n;
      } else if (id == "video-slider") {
        const auto value =
            event.GetParameter<float>("value", float(draft.rendering.fps));
        if (std::isfinite(value) && value >= 60 && value <= 1000) {
          draft.rendering.fps = static_cast<unsigned>(std::round(value));
          fpsText = std::to_string(draft.rendering.fps);
        }
      } else if (id == "video-size") {
        const auto value =
            static_cast<Rml::ElementFormControlSelect *>(target)->GetValue();
        if (value.size() == 1 && value[0] >= '0' && value[0] <= '4')
          draft.window_size = static_cast<unsigned>(value[0] - '0');
      } else
        return;
      error.clear();
      refresh(id != "video-fps");
      return;
    }
    if (event.GetType() != "click" || target->HasAttribute("disabled"))
      return;
    if (id == "video-cancel") {
      PostMessageW(window, WM_CLOSE, 0, 0);
      return;
    }
    if (id == "video-apply") {
      if (apply())
        PostMessageW(window, WM_CLOSE, 0, 0);
      return;
    }
    if (id == "video-windowed")
      draft.fullscreen = false;
    else if (id == "video-fullscreen")
      draft.fullscreen = true;
    else if (id == "video-pause")
      draft.pause_inactive = !draft.pause_inactive;
    else if (id == "video-vsync")
      draft.rendering.vsync = !draft.rendering.vsync;
    else if (id == "video-original")
      draft.rendering.mode = jfg::FrameRateMode::original;
    else if (id == "video-custom")
      draft.rendering.mode = jfg::FrameRateMode::custom;
    else if (id == "video-unlimited")
      draft.rendering.mode = jfg::FrameRateMode::unlimited;
    else if (id.rfind("video-preset-", 0) == 0) {
      if (auto n =
              jfg::parse_video_fps(id.substr(sizeof("video-preset-") - 1))) {
        draft.rendering.fps = *n;
        fpsText = std::to_string(*n);
      }
    } else
      return;
    error.clear();
    refresh(true);
  }
  bool keyboard(UINT m, WPARAM key) {
    if (m != WM_KEYDOWN)
      return false;
    auto *size = static_cast<Rml::ElementFormControlSelect *>(el("video-size"));
    if (size->IsSelectBoxVisible()) {
      if (key == VK_ESCAPE) {
        size->CancelSelectBox();
        return true;
      }
      if (key == VK_TAB)
        size->HideSelectBox();
      else
        return false;
    }
    if (key == VK_ESCAPE) {
      PostMessageW(window, WM_CLOSE, 0, 0);
      return true;
    }
    auto *context = doc->GetContext();
    auto *focused = context->GetFocusElement();
    if (key == VK_TAB) {
      Rml::ElementList controls;
      doc->QuerySelectorAll(controls, "button, input, select");
      controls.erase(std::remove_if(controls.begin(), controls.end(),
                                    [](auto *e) {
                                      return !e->IsVisible(true) ||
                                             e->HasAttribute("disabled");
                                    }),
                     controls.end());
      if (!controls.empty()) {
        const bool back = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        auto it = std::find(controls.begin(), controls.end(), focused);
        int n =
            it == controls.end() ? (back ? 0 : -1) : int(it - controls.begin());
        n = (n + (back ? -1 : 1) + int(controls.size())) % int(controls.size());
        controls[n]->Focus(true);
        controls[n]->ScrollIntoView(false);
      }
      return true;
    }
    if ((key == VK_RETURN || key == VK_SPACE) && focused &&
        focused->GetTagName() == "button") {
      focused->Click();
      return true;
    }
    if (key == VK_RETURN && focused && focused->GetId() == "video-fps") {
      if (apply())
        PostMessageW(window, WM_CLOSE, 0, 0);
      return true;
    }
    return false;
  }
};
} // namespace jfg_video
