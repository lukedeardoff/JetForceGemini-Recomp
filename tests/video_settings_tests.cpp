#include "jfg/renderer/frame_diagnostics_state.hpp"
#include "jfg/renderer/presentation_policy.hpp"
#include "jfg/renderer/video_settings.hpp"
#include <iostream>
int main() {
  unsigned failures = 0;
  auto check = [&](bool ok) {
    if (!ok)
      ++failures;
  };
  for (auto mode : {jfg::FrameRateMode::original, jfg::FrameRateMode::custom,
                    jfg::FrameRateMode::unlimited}) {
    for (bool vsync : {false, true}) {
      const jfg::VideoSettings settings{mode, 360, vsync};
      auto loaded =
          jfg::parse_video_settings(jfg::serialize_video_settings(settings));
      check(loaded && loaded->mode == mode && loaded->fps == 360 &&
            loaded->vsync == vsync);
    }
  }
  check(!jfg::parse_video_settings(
      "version=2\nmode=unlimited\nfps=360\nvsync=0\n"));
  check(!jfg::parse_video_settings(
      "version=1\nmode=custom\nfps=360\nvsync=1\nvsync=0\n"));
  check(!jfg::parse_video_settings("version=1\nmode=custom\nfps=0\nvsync=1\n"));
  check(!jfg::parse_video_settings(
      "version=1\nmode=custom\nfps=1001\nvsync=1\n"));
  check(!jfg::parse_video_settings("mode=custom\nfps=360\nvsync=1\n"));
  check(!jfg::parse_video_fps({}));
  check(!jfg::parse_video_fps("360junk"));
  check(!jfg::parse_video_fps("-60"));
  check(!jfg::parse_video_fps("999999999999999999999999"));
  check(jfg::parse_video_fps("60") == 60U);
  check(jfg::parse_video_fps("1000") == 1000U);
  auto original = jfg::load_video_settings({});
  check(original && original->mode == jfg::FrameRateMode::original &&
        original->vsync);
  jfg::configure_presentation_policy(true, false);
  check(jfg::presentation_target(360, 240) == 240);
  jfg::configure_presentation_policy(false, false);
  check(jfg::presentation_target(360, 240) == 360);
  jfg::configure_presentation_policy(true, true);
  check(jfg::presentation_target(60, 240) == 240);
  jfg::configure_presentation_policy(false, true);
  check(jfg::presentation_unlimited());
  check(jfg::presentation_target(60, 240) == jfg::kUnlimitedPresentation);
  jfg::configure_presentation_policy(true, false);
  // Boss capture: one 3D submission and one blur submission per two VI.
  // Starting at tick zero is valid, and duplicate passes must not reset age.
  jfg::FrameDiagnosticState diagnostics;
  diagnostics.scene(0);
  const auto first_time = diagnostics.scene_time.load();
  diagnostics.scene(0);
  check(diagnostics.scenes.load() == 1 && diagnostics.scene_time.load() == first_time);
  for (std::uint64_t tick = 2; tick <= 60; tick += 2) {
    diagnostics.scene(tick);
    diagnostics.scene(tick);
  }
  check(diagnostics.scenes.load() == 31 && diagnostics.vi.load() == 60);
  std::cout << "Video settings checks: " << failures << " failures\n";
  return failures ? 1 : 0;
}
