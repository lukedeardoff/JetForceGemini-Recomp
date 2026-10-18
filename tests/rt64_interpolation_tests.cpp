#define SDL_MAIN_HANDLED
#include "rt64_interpolation_adapter.hpp"
#include <cmath>
#include <cstdlib>
static void require(bool value) {
  if (!value)
    std::abort();
}
int main() {
  RT64::Workload workload;
  const hlslpp::float4x4 endpoint(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2, 0, 0,
                                  1);
  workload.drawData.worldTransforms.push_back(endpoint);
  require(!jfg::apply_presentation_interpolation(workload, 0, 1));
  auto frame = std::make_shared<jfg::PresentationHistoryFrame>();
  frame->scene = true;
  frame->transforms.push_back(
      {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, true});
  jfg::publish_presentation_snapshot(&workload, frame);
  require(jfg::apply_presentation_interpolation(workload, 0, 0.25f));
  float row[4];
  for (unsigned c = 0; c < 4; ++c)
    row[c] = workload.drawData.lerpWorldTransforms[0].m[3][c];
  require(std::abs(row[0] - 0.5f) < 0.00001f && row[3] == 1.0f);
  for (unsigned c = 0; c < 4; ++c)
    row[c] = workload.drawData.worldTransforms[0].m[3][c];
  require(row[0] == 2.0f); // Authored endpoint is untouched.
  require(jfg::apply_presentation_interpolation(workload, 0.5f, 1));
  for (unsigned c = 0; c < 4; ++c)
    row[c] = workload.drawData.lerpWorldTransforms[0].m[3][c];
  require(row[0] == 2.0f);
  for (unsigned c = 0; c < 4; ++c)
    row[c] = workload.drawData.prevWorldTransforms[0].m[3][c];
  require(row[0] == 1.0f);
  const auto retained = jfg::presentation_snapshot(&workload);
  jfg::publish_presentation_snapshot(&workload, nullptr);
  require(!jfg::presentation_snapshot(&workload));
  require(
      retained->transforms[0].matched); // An in-flight reader owns its data.
  frame = std::make_shared<jfg::PresentationHistoryFrame>();
  frame->transforms.push_back({});
  jfg::publish_presentation_snapshot(&workload, frame);
  require(jfg::apply_presentation_interpolation(workload, 0, 0.5f));
  for (unsigned c = 0; c < 4; ++c)
    row[c] = workload.drawData.lerpWorldTransforms[0].m[3][c];
  require(row[0] == 2.0f); // Unmatched geometry uses the current endpoint.
  jfg::publish_presentation_snapshot(&workload, nullptr);
  // World endpoint motion and the camera use the same interpolation phase.
  frame = std::make_shared<jfg::PresentationHistoryFrame>();
  jfg::PresentationTransform semantic;
  semantic.matched = true;
  semantic.semantic_vertices = true;
  semantic.current_world = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 0, 0, 1};
  semantic.previous_camera = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  semantic.current_camera = semantic.previous_camera;
  semantic.current_camera[12] = 2;
  frame->transforms.push_back(semantic);
  jfg::publish_presentation_snapshot(&workload, frame);
  for (float alpha : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
    require(jfg::apply_presentation_interpolation(workload, alpha, alpha));
    // Current local vertex 1, previous world vertex 9 -> previous local -1.
    const auto point =
        hlslpp::mul(hlslpp::float4(1.0f - 2.0f * (1.0f - alpha), 0, 0, 1),
                    workload.drawData.lerpWorldTransforms[0]);
    require(std::abs(float(point.x) - (9.0f + 4.0f * alpha)) < 0.00001f);
  }
  jfg::publish_presentation_snapshot(&workload, nullptr);
  // Coincident vertices belonging to differently rotated joints must stay
  // coincident at every subframe while the camera also moves.
  const hlslpp::float4x4 joints[]{
      hlslpp::float4x4(0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 5, 2, 0, 1),
      hlslpp::float4x4(0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, -3, 4, 0, 1)};
  const hlslpp::float4 old_world(1, 2, 0, 1), new_world(2, 3, 0, 1);
  workload.drawData.worldTransforms.resize(2);
  frame = std::make_shared<jfg::PresentationHistoryFrame>();
  for (unsigned joint = 0; joint < 2; ++joint) {
    auto pose = semantic;
    for (unsigned r = 0; r < 4; ++r)
      for (unsigned c = 0; c < 4; ++c)
        pose.current_world[r * 4 + c] = joints[joint][r][c];
    frame->transforms.push_back(pose);
  }
  jfg::publish_presentation_snapshot(&workload, frame);
  for (float alpha : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
    require(jfg::apply_presentation_interpolation(workload, alpha, alpha));
    for (unsigned joint = 0; joint < 2; ++joint) {
      const auto inverse = hlslpp::inverse(joints[joint]);
      const auto previous_local = hlslpp::mul(old_world, inverse);
      const auto current_local = hlslpp::mul(new_world, inverse);
      const auto point =
          hlslpp::mul(previous_local * (1 - alpha) + current_local * alpha,
                      workload.drawData.lerpWorldTransforms[joint]);
      require(std::abs(float(point.x) - (1 + 3 * alpha)) < 0.00001f);
      require(std::abs(float(point.y) - (2 + alpha)) < 0.00001f);
    }
  }
  jfg::publish_presentation_snapshot(&workload, nullptr);
  return 0;
}
