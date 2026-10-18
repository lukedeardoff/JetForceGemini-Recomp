#pragma once
#include "jfg/renderer/interpolation_history.hpp"
#include <set>
#include <map>
#include <functional>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace jfg {
using PresentationVertexKey = std::array<std::uint64_t, 6>;
struct SemanticPresentationSample {
  // Lifetime, actor/instance, model, camera identity, joint, logical vertex.
  PresentationVertexKey key{};
  std::array<float, 3> world{};
  PresentationMatrix camera{};
  bool valid = false;
};
struct SemanticPresentationPrevious {
  std::array<float, 3> world{};
  PresentationMatrix camera{};
  bool matched = false;
};
inline bool semantic_motion_continuous(const SemanticPresentationSample &a,
                                       const SemanticPresentationSample &b) {
  const auto project = [](const SemanticPresentationSample &s) {
    std::array<float, 4> p{};
    for (std::size_t c = 0; c < 4; ++c)
      p[c] = s.world[0] * s.camera[c] + s.world[1] * s.camera[4 + c] +
             s.world[2] * s.camera[8 + c] + s.camera[12 + c];
    return p;
  };
  for (float x : a.world)
    if (!std::isfinite(x))
      return false;
  for (float x : b.world)
    if (!std::isfinite(x))
      return false;
  for (float x : a.camera)
    if (!std::isfinite(x))
      return false;
  for (float x : b.camera)
    if (!std::isfinite(x))
      return false;
  const auto p = project(a), q = project(b);
  for (float x : p)
    if (!std::isfinite(x))
      return false;
  for (float x : q)
    if (!std::isfinite(x))
      return false;
  if (p[3] <= 0.001f && q[3] <= 0.001f)
    return true;
  if (p[3] <= 0.001f || q[3] <= 0.001f || q[3] / p[3] < 0.5f ||
      q[3] / p[3] > 2.0f)
    return false;
  return std::abs(p[0] / p[3] - q[0] / q[3]) <= 0.35f &&
         std::abs(p[1] / p[3] - q[1] / q[3]) <= 0.35f;
}
// One immutable previous scene, indexed by semantic vertices. UV clones may
// repeat the same sample; conflicting duplicates invalidate that actor.
// Every part of one actor either interpolates on the same clock or uses the
// endpoint. Interpolate world-space vertex endpoints before applying one
// shared camera sample, avoiding cracks caused by multiplying separately
// blended joint matrices and deforming local vertices.
struct PresentationVertexHash {
  std::size_t operator()(const PresentationVertexKey& key) const noexcept {
    std::size_t result=0;
    for (const auto value:key)
      result ^= std::hash<std::uint64_t>{}(value) + std::size_t{0x9e3779b9U} +
                (result<<6U) + (result>>2U);
    return result;
  }
};
// Optional per-lifetime reasons: invalid sample, cadence break, conflicting
// duplicate, missing old endpoint, projected-motion cut, matched vertex.
using SemanticPresentationDiagnostics=std::map<std::uint64_t,std::array<std::uint64_t,6>>;
using SemanticRigidPoses=std::map<std::uint64_t,RigidPresentationPose>;
class SemanticPresentationHistory {
  struct Endpoint {
    SemanticPresentationSample sample;
    std::size_t first = 0;
  };
  // Two retained, contiguous endpoint tables avoid one heap allocation per
  // logical vertex per scene. Hash collisions always compare the entire key;
  // indices are local to each table and are rebuilt before reusing storage.
  struct Endpoints {
    static constexpr auto empty = (std::numeric_limits<std::size_t>::max)();
    std::vector<Endpoint> entries;
    std::vector<std::size_t> slots;
    void clear(std::size_t maximum) {
      if (maximum > empty / 4U) throw std::length_error("semantic history capacity");
      std::size_t capacity=16U;
      while (capacity < maximum*2U) capacity*=2U;
      if (capacity > slots.size()) slots.resize(capacity);
      std::fill(slots.begin(),slots.end(),empty);
      entries.clear();entries.reserve(maximum);
    }
    std::pair<Endpoint*,bool> insert(const SemanticPresentationSample& sample,std::size_t first) {
      auto slot=PresentationVertexHash{}(sample.key)&(slots.size()-1U);
      while (slots[slot]!=empty) {
        auto& entry=entries[slots[slot]];
        if (entry.sample.key==sample.key) return {&entry,false};
        slot=(slot+1U)&(slots.size()-1U);
      }
      slots[slot]=entries.size();entries.push_back({sample,first});
      return {&entries.back(),true};
    }
    const Endpoint* find(const PresentationVertexKey& key) const {
      if (slots.empty()) return nullptr;
      auto slot=PresentationVertexHash{}(key)&(slots.size()-1U);
      while (slots[slot]!=empty) {
        const auto& entry=entries[slots[slot]];
        if (entry.sample.key==key) return entry.sample.valid ? &entry : nullptr;
        slot=(slot+1U)&(slots.size()-1U);
      }
      return nullptr;
    }
  };
public:
  std::vector<SemanticPresentationPrevious>
  record(const std::vector<SemanticPresentationSample> &samples,
         std::uint64_t tick, bool scene, bool cadence,
         SemanticPresentationDiagnostics* diagnostics=nullptr,
         const SemanticRigidPoses* rigid_poses=nullptr) {
    std::vector<SemanticPresentationPrevious> result(samples.size());
    if (!scene)
      return result;
    next_.clear(samples.size());
    std::set<std::uint64_t> blocked, corrupt;
    const bool continuous = cadence && previous_tick_ != 0U &&
                            tick > previous_tick_ &&
                            tick - previous_tick_ <= 6U;
    std::set<std::uint64_t> continuous_rigids;
    if(rigid_poses)for(const auto& [lifetime,pose]:*rigid_poses) {
      const auto old=previous_rigid_poses_.find(lifetime);
      if(old!=previous_rigid_poses_.end() && continuous_owned_camera_motion(old->second,pose))
        continuous_rigids.insert(lifetime);
    }
    const auto bridge_continuous=[&](const auto& old,const auto& current) {
      if(current.key[4]!=0x42524944U || !continuous_rigids.contains(current.key[0]))
        return false;
      for(float value:old.camera)if(!std::isfinite(value))return false;
      for(float value:current.camera)if(!std::isfinite(value))return false;
      double distance2=0;
      for(unsigned c=0;c<3;++c) {
        if(!std::isfinite(old.world[c]) || !std::isfinite(current.world[c]))return false;
        const auto delta=double(current.world[c])-old.world[c];distance2+=delta*delta;
      }
      // Retain a world-space bound on actual deformation. Camera/zoom/model
      // cuts have already been checked using the immutable owned operands.
      return distance2<=128.0*128.0;
    };
    for (std::size_t i = 0; i < samples.size(); ++i) {
      const auto &s = samples[i];
      const auto actor = s.key[0];
      if (actor == 0U)
        continue;
      if(diagnostics) {
        (*diagnostics)[actor][0]+=!s.valid;
        (*diagnostics)[actor][1]+=!continuous;
      }
      if (!s.valid || !continuous)
        blocked.insert(actor);
      if (!s.valid) {
        corrupt.insert(actor);
        continue;
      }
      // Identical UV clones share one continuity decision. Keep the full key
      // and endpoint comparison: conflicting clones still poison the actor.
      const auto [it, inserted] = next_.insert(s,i);
      if (inserted) {
        (*it) = {s, i};
      } else if ((*it).sample.world == s.world &&
                 (*it).sample.camera == s.camera) {
        result[i] = result[(*it).first];
        continue;
      } else {
        blocked.insert(actor);
        corrupt.insert(actor);
        if(diagnostics)++(*diagnostics)[actor][2];
      }
      // A unique NaN endpoint was rejected by the original duplicate audit.
      // Preserve its poison even when that vertex is culled next scene.
      for (float value:s.world) if (std::isnan(value)) corrupt.insert(actor);
      for (float value:s.camera) if (std::isnan(value)) corrupt.insert(actor);
      const auto old = previous_.find(s.key);
      if (old == nullptr) {
        blocked.insert(actor);
        if(diagnostics)++(*diagnostics)[actor][3];
      } else if(!semantic_motion_continuous(old->sample,s) && !bridge_continuous(old->sample,s)) {
        blocked.insert(actor);
        if(diagnostics)++(*diagnostics)[actor][4];
      } else {
        result[i] = {old->sample.world, old->sample.camera, true};
        if(diagnostics)++(*diagnostics)[actor][5];
      }
    }
    for (std::size_t i = 0; i < samples.size(); ++i)
      if (blocked.contains(samples[i].key[0]))
        result[i].matched = false;
    // A conflicting endpoint is not a valid anchor for the next scene.
    for (auto& entry : next_.entries)
      if (corrupt.contains(entry.sample.key[0])) entry.sample.valid=false;
    std::swap(previous_,next_);
    previous_tick_ = tick;
    previous_rigid_poses_=rigid_poses?*rigid_poses:SemanticRigidPoses{};
    return result;
  }

private:
  Endpoints previous_, next_;
  SemanticRigidPoses previous_rigid_poses_;
  std::uint64_t previous_tick_ = 0;
};
} // namespace jfg
