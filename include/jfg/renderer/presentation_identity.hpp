#pragma once
#include "jfg/renderer/rigid_camera.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <map>
#include <span>
#include <vector>

namespace jfg {
// Host-only identity for the supported US guest. No writes to simulation RAM.
// Addresses locate a sample; lifetime + model + bone identify what it means.
struct PresentationModelIdentity {
  std::uint64_t lifetime = 0;
  std::uint32_t actor = 0, instance = 0, model = 0;
  std::array<std::uint32_t, 2> palettes{};
  std::uint32_t vertices = 0, vertex_count = 0, bone_count = 0;
  bool deforming_bridge = false; // US hitBridgeControl (behavior 55).
};
struct PresentationCameraIdentity {
  std::uint32_t matrix = 0;
  std::uint64_t identity = 0;
  std::uint64_t generation = 0;
  RigidPresentationPose pose;
  bool pose_valid = false;
};
struct PresentationSpriteIdentity {
  std::uint32_t anchor = 0, actor = 0, sprite = 0;
  std::uint64_t lifetime = 0;
};
// A rigid object's transient combined matrix is bound to its actor lifetime.
// The address locates this task's sample; it is never the history key.
struct PresentationRigidIdentity {
  std::uint32_t matrix = 0, actor = 0;
  std::uint64_t lifetime = 0, camera = 0;
  RigidPresentationPose pose;
};
// US behavior 68 / beamDraw: two eight-vertex buffers in actor control data.
// The physical buffer alternates; actor lifetime, control allocation and shape
// flags identify the same logical vertices across scenes.
struct PresentationBeamIdentity {
  std::uint64_t lifetime = 0;
  std::uint32_t actor = 0, control = 0, header = 0, vertices = 0, flags = 0;
};
struct PresentationSceneIdentity {
  std::vector<PresentationModelIdentity> models;
  std::vector<PresentationCameraIdentity> cameras;
  std::vector<PresentationSpriteIdentity> sprites;
  std::vector<PresentationRigidIdentity> rigids;
  std::vector<PresentationBeamIdentity> beams;
};
// A reduced-detail mesh can use the main model's bone palette. Resolve
// palette ownership first, then the vertex stream within that same actor.
struct PresentationVertexIdentity {
  const PresentationModelIdentity *owner = nullptr, *geometry = nullptr;
  std::uint32_t joint = 0, vertex = 0;
};
inline PresentationVertexIdentity
presentation_vertex_identity(const PresentationSceneIdentity &scene,
                             std::uint32_t matrix, std::uint32_t source) {
  PresentationVertexIdentity result;
  for (const auto &model : scene.models)
    for (const auto palette : model.palettes) {
      if (!palette || matrix < palette ||
          matrix - palette >= model.bone_count * 64U ||
          (matrix - palette) % 64U)
        continue;
      if (result.owner && result.owner != &model)
        // Preserve invalid semantic provenance so this mesh holds its
        // endpoint instead of falling back to an ambiguous shape match.
        return result;
      result.owner = &model;
      result.joint = (matrix - palette) / 64U;
    }
  if (!result.owner)
    return result;
  for (const auto &model : scene.models) {
    if (model.actor != result.owner->actor ||
        model.lifetime != result.owner->lifetime || source < model.vertices ||
        source - model.vertices >= model.vertex_count * 10U ||
        (source - model.vertices) % 10U)
      continue;
    if (result.geometry) {
      result.geometry = nullptr;
      return result;
    }
    result.geometry = &model;
    result.vertex = (source - model.vertices) / 10U;
  }
  return result;
}
inline const PresentationRigidIdentity* presentation_rigid_owner(
    const PresentationSceneIdentity& scene, std::uint32_t matrix) {
  const PresentationRigidIdentity* owner=nullptr;
  for (const auto& rigid:scene.rigids) if(rigid.matrix==matrix) {
    if(owner) return nullptr;
    owner=&rigid;
  }
  return owner;
}
inline const PresentationBeamIdentity* presentation_beam_vertex(
    const PresentationSceneIdentity& scene, const PresentationRigidIdentity& owner,
    std::uint32_t source) {
  const PresentationBeamIdentity* found = nullptr;
  for (const auto& beam:scene.beams) {
    if (beam.actor!=owner.actor || beam.lifetime!=owner.lifetime ||
        source<beam.vertices || source-beam.vertices>=80U ||
        (source-beam.vertices)%10U) continue;
    if (found) return nullptr;
    found=&beam;
  }
  return found;
}
// A bridge bends by changing local vertices, independently of its rigid pose.
// Resolve a whole DMA range against one current actor/model allocation. The
// buffer address locates samples; model instance + logical index own history.
inline const PresentationModelIdentity* presentation_bridge_vertices(
    const PresentationSceneIdentity& scene, const PresentationRigidIdentity& owner,
    std::uint32_t source, std::uint32_t count) {
  if (!owner.lifetime || !owner.camera || !count) return nullptr;
  const PresentationModelIdentity* found = nullptr;
  for (const auto& model:scene.models) {
    if (model.actor!=owner.actor || model.lifetime!=owner.lifetime ||
        source<model.vertices || (source-model.vertices)%10U) continue;
    const auto first=(source-model.vertices)/10U;
    if (first>=model.vertex_count || count>model.vertex_count-first) continue;
    if (found || !model.deforming_bridge) return nullptr;
    found=&model;
  }
  return found;
}
inline std::array<std::uint64_t, 4> presentation_rigid_key(
    const PresentationSceneIdentity &scene, std::uint32_t matrix,
    std::uint32_t source, std::uint32_t count) {
  const auto* owner = presentation_rigid_owner(scene, matrix);
  if (!owner || !owner->lifetime || !owner->camera || !count) return {};
  const PresentationModelIdentity *geometry = nullptr;
  for (const auto &model : scene.models) {
    if (model.actor != owner->actor || model.lifetime != owner->lifetime ||
        source < model.vertices || (source - model.vertices) % 10U) continue;
    const auto first = (source - model.vertices) / 10U;
    if (first >= model.vertex_count || count > model.vertex_count - first) continue;
    if (geometry) return {};
    geometry = &model;
  }
  if (!geometry) return {};
  return {owner->lifetime, (std::uint64_t(owner->actor) << 32U) | geometry->instance,
          geometry->model, owner->camera};
}
class PresentationGuestView {
public:
  explicit PresentationGuestView(std::span<const std::uint8_t> data)
      : data_(data) {}
  static std::uint32_t physical(std::uint32_t address) {
    return address & 0x00FFFFFFU;
  }
  bool valid(std::uint32_t address, std::size_t size) const {
    const auto p = physical(address);
    const auto segment = address & 0xFF000000U;
    return (segment == 0U || segment == 0x80000000U ||
            segment == 0xA0000000U) &&
           p != 0U && p <= data_.size() && size <= data_.size() - p;
  }
  std::uint8_t u8(std::uint32_t address) const {
    const auto offset = physical(address) ^ 3U;
    return valid(address, 1) && offset < data_.size() ? data_[offset] : 0;
  }
  std::uint16_t u16(std::uint32_t address) const {
    return static_cast<std::uint16_t>(valid(address, 2)
               ? (std::uint16_t(u8(address)) << 8U) | u8(address + 1U)
               : 0);
  }
  std::uint32_t u32(std::uint32_t address) const {
    if (!valid(address, 4) || (address & 3U))
      return 0;
    std::uint32_t word = 0;
    std::memcpy(&word, data_.data() + physical(address), 4);
    return word;
  }

private:
  std::span<const std::uint8_t> data_;
};
class PresentationIdentityTracker {
public:
  void retire(std::uint32_t actor) {
    actors_.erase(actor);
    for (auto it = rigids_.begin(); it != rigids_.end();)
      if (it->second.actor == actor) it = rigids_.erase(it); else ++it;
    for (auto it = sprites_.begin(); it != sprites_.end();)
      if (it->second.actor == actor)
        it = sprites_.erase(it);
      else
        ++it;
  }
  // US particle slots are 0x78 bytes. Allocation/free hooks distinguish
  // immediate slot reuse even when it occurs between two captured scenes.
  void particle_retire(std::uint32_t particle) {
    particle = PresentationGuestView::physical(particle);
    particles_.erase(particle);
    for (auto it = particle_sprites_.begin(); it != particle_sprites_.end();)
      if (it->second.particle == particle)
        it = particle_sprites_.erase(it);
      else
        ++it;
  }
  void particles_clear() {
    particles_.clear();
    particle_sprites_.clear();
  }
  void particle_allocate(std::span<const std::uint8_t> data,
                         std::uint32_t particle) {
    const PresentationGuestView ram(data);
    level(ram.u32(0x800FB114U));
    particle_retire(particle);
    if (!ram.valid(particle, 0x78U) || ram.u16(particle + 0x2CU) != 2U)
      return;
    if (particles_.size() >= 4096U)
      particles_clear();
    particles_[PresentationGuestView::physical(particle)] = ++serial_;
  }
  // Only the verified partDraw billboard caller uses this layout. Particle
  // asset ownership is independent of nearby pickup actors and their assets.
  void particle_sprite(std::span<const std::uint8_t> data,
                       std::uint32_t output_pointer, std::uint32_t particle,
                       std::uint32_t asset, std::uint64_t tick) {
    const PresentationGuestView ram(data);
    level(ram.u32(0x800FB114U));
    const auto address = ram.u32(output_pointer);
    const auto physical = PresentationGuestView::physical(particle);
    const auto live = particles_.find(physical);
    if (live == particles_.end() || !ram.valid(address, 10) ||
        !ram.valid(particle, 0x78U) || ram.u16(particle + 0x2CU) != 2U ||
        !asset || ram.u32(particle + 0x4CU) != asset)
      return;
    if (particle_sprites_.size() >= 4096U)
      particle_sprites_.clear();
    particle_sprites_[PresentationGuestView::physical(address)] =
        {physical, asset, live->second, tick};
  }
  // camDoSprite emits one world-space anchor at *a2 for the actor in a3.
  // Record addresses only; capture binds them to the actor's live lifetime.
  void sprite(std::span<const std::uint8_t> data, std::uint32_t output_pointer,
              std::uint32_t actor, std::uint32_t asset, std::uint64_t tick) {
    const PresentationGuestView ram(data);
    level(ram.u32(0x800FB114U));
    const auto address = ram.u32(output_pointer);
    if (!ram.valid(address, 10) || !ram.valid(actor, 0xA1U))
      return;
    if (sprites_.size() >= 4096U)
      sprites_.clear();
    sprites_[PresentationGuestView::physical(address)] = {
        actor, asset, ram.u32(actor + 0x40U), ram.u32(actor + 0x6CU), tick};
  }
  // US camPushModelMtx (0x80041CA0), and module 122 ShirukenPushMtx
  // (0x07A00F38), consume ObjectTransform* in a2 and
  // write a combined model/camera matrix at the entry value of *a1. Observe
  // after it returns. capture() accepts only an exact live actor and unchanged
  // emitted matrix bytes, so unrelated transforms/buffer reuse cannot bind.
  void rigid(std::span<const std::uint8_t> data, std::uint32_t matrix,
             std::uint32_t actor, std::uint64_t tick) {
    const PresentationGuestView ram(data);
    level(ram.u32(0x800FB114U));
    const auto physical = PresentationGuestView::physical(matrix);
    rigids_.erase(physical);
    const auto number = ram.u32(0x800FA634U), mode = ram.u32(0x800FA630U);
    if (!ram.valid(matrix, 64U) || (matrix & 3U) ||
        !ram.valid(actor, 0xA1U) || number > 3U) return;
    if (rigids_.size() >= 4096U) rigids_.clear();
    Rigid binding{actor, ram.u32(actor + 0x40U), ram.u32(actor + 0x6CU),
                  tick, (std::uint64_t(mode) << 32U) | (number + 1U), {}};
    for (unsigned i = 0; i < 16; ++i) binding.words[i] = ram.u32(matrix + i*4U);
    // US camPushModelMtx constructs model at 0x800FAAB0, concatenates it
    // with view/projection (0x800FAF78/0x800FAA70), then emits fixed MVP.
    // Capture before another object reuses these scratch globals. The matrix
    // byte/lifetime checks below and decoder product check bind the operands.
    for (unsigned i=0;i<16;++i) {
      binding.pose.world[i]=std::bit_cast<float>(ram.u32(0x800FAAB0U+i*4U));
      binding.pose.view[i]=std::bit_cast<float>(ram.u32(0x800FAF78U+i*4U));
      binding.pose.projection[i]=std::bit_cast<float>(ram.u32(0x800FAA70U+i*4U));
    }
    rigids_[physical] = binding;
  }
  // camSetProjMtx writes one camera MVP at *a1 and emits its DMA command.
  // Observe before the guest call; bind the record to the eventual task copy.
  void camera(std::span<const std::uint8_t> data, std::uint32_t output_pointer,
              std::uint64_t tick) {
    const PresentationGuestView ram(data);
    level(ram.u32(0x800FB114U));
    const auto number = ram.u32(0x800FA634U), mode = ram.u32(0x800FA630U);
    const auto address = ram.u32(output_pointer);
    if (number > 3U || !ram.valid(address, 64))
      return;
    if (cameras_.size() >= 256U)
      cameras_.clear();
    cameras_[PresentationGuestView::physical(address)] = {
        (std::uint64_t(mode) << 32U) | (number + 1U), tick};
  }
  // Capture camera operands after camSetProjMtx emits the fixed matrix, before
  // another object reuses the guest scratch matrices. Bind to exact bytes.
  void camera_completed(std::span<const std::uint8_t> data,
                        std::uint32_t matrix, std::uint64_t tick) {
    const PresentationGuestView ram(data);
    const auto found=cameras_.find(PresentationGuestView::physical(matrix));
    if (found==cameras_.end() || !ram.valid(matrix,64U) || (matrix&3U)) return;
    auto& camera=found->second;
    camera.tick=tick;
    camera.pose.world={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    for (unsigned i=0;i<16;++i) {
      camera.words[i]=ram.u32(matrix+i*4U);
      camera.pose.view[i]=std::bit_cast<float>(ram.u32(0x800FAF78U+i*4U));
      camera.pose.projection[i]=std::bit_cast<float>(ram.u32(0x800FAA70U+i*4U));
    }
    camera.pose_valid=true;
  }
  PresentationSceneIdentity capture(std::span<const std::uint8_t> data,
                                    std::uint64_t tick) {
    const PresentationGuestView ram(data);
    level(ram.u32(0x800FB114U));
    PresentationSceneIdentity result;
    for (auto it = cameras_.begin(); it != cameras_.end();) {
      if (tick < it->second.tick || tick - it->second.tick > 6U)
        it = cameras_.erase(it);
      else {
        bool unchanged=it->second.pose_valid;
        for(unsigned i=0;i<16 && unchanged;++i)
          unchanged=ram.u32(it->first+i*4U)==it->second.words[i];
        result.cameras.push_back({it->first,it->second.identity,camera_generation_,
                                  it->second.pose,unchanged});
        ++it;
      }
    }
    const auto list = ram.u32(0x800F2CA4U), count = ram.u32(0x800F2CA8U);
    if (count > 256U || !ram.valid(list, count * 4U)) {
      actors_.clear();
      return result;
    }
    std::map<std::uint32_t, Lifetime> live;
    for (std::uint32_t i = 0; i < count; ++i) {
      const auto actor = ram.u32(list + i * 4U);
      if (!ram.valid(actor, 0xA1U) || ram.u8(actor + 0xA0U) != 0U ||
          live.contains(actor))
        continue;
      const auto header = ram.u32(actor + 0x40U),
                 models = ram.u32(actor + 0x6CU);
      const auto model_count = ram.u8(header + 0x1FU);
      if (!ram.valid(header, 0x20U) || model_count > 16U ||
          (model_count != 0U && !ram.valid(models, model_count * 4U)))
        continue;
      auto old = actors_.find(actor);
      const auto serial = old != actors_.end() &&
                                  old->second.header == header &&
                                  old->second.models == models
                              ? old->second.serial
                              : ++serial_;
      live[actor] = {serial, header, models};
      // Verified US objPrintCustomObject case 68 calls beamDraw. Its control
      // allocation contains 2 * 80 vertex bytes followed by state at +0xCC.
      // A malformed/disabled beam gets no procedural interpolation identity.
      const auto control=ram.u32(actor+0x68U);
      if (ram.u16(actor+0x48U)==68U && ram.valid(control,0xD0U) &&
          (control&3U)==0U && ram.u8(control+0xCCU)!=0U &&
          ram.u8(control+0xCDU)<=1U) {
        const auto flags=ram.u16(control+0xCEU);
        if (flags && !(flags&~7U))
          result.beams.push_back({serial,actor,control,header,
              PresentationGuestView::physical(control)+ram.u8(control+0xCDU)*80U,flags});
      }
      for (std::uint32_t j = 0; j < model_count; ++j) {
        const auto instance = ram.u32(models + j * 4U),
                   model = ram.u32(instance);
        if (!ram.valid(instance, 8U) || !ram.valid(model, 0x80U))
          continue;
        const auto bones = ram.u8(model + 0x4FU);
        const auto vertices = ram.u16(model + 0x12U);
        const auto vertex_base = ram.u32(instance + 4U);
        const auto p0 = ram.u32(instance + 0x10U),
                   p1 = ram.u32(instance + 0x14U);
        if (bones > 127U || vertices == 0U || vertices > 8192U ||
            !ram.valid(vertex_base, vertices * 10U))
          continue;
        // Compact LOD instances own vertices but share another instance's
        // palette. Words at +0x10/+0x14 can already be vertex data there.
        const auto source = PresentationGuestView::physical(vertex_base);
        const auto fields = PresentationGuestView::physical(instance) + 0x10U;
        const bool inline_vertices =
            source < fields + 8U && source + vertices * 10U > fields;
        const bool palette = !inline_vertices && ram.u8(model + 0x4EU) != 0U &&
                             bones != 0U && ram.valid(p0, bones * 64U) &&
                             ram.valid(p1, bones * 64U);
        result.models.push_back(
            {serial,
             actor,
             instance,
             model,
             {palette ? PresentationGuestView::physical(p0) : 0U,
              palette ? PresentationGuestView::physical(p1) : 0U},
             PresentationGuestView::physical(vertex_base),
             vertices,
             bones,
             ram.u16(actor + 0x48U) == 55U});
      }
    }
    actors_ = std::move(live);
    for (auto it = rigids_.begin(); it != rigids_.end();) {
      const auto &rigid = it->second;
      const auto actor = actors_.find(rigid.actor);
      bool bytes_match = true;
      for (unsigned i = 0; i < 16; ++i)
        bytes_match = bytes_match && ram.u32(it->first + i*4U) == rigid.words[i];
      if (tick < rigid.tick || tick - rigid.tick > 6U || !bytes_match ||
          actor == actors_.end() || actor->second.header != rigid.header ||
          actor->second.models != rigid.models) it = rigids_.erase(it);
      else {
        result.rigids.push_back({it->first, rigid.actor, actor->second.serial, rigid.camera, rigid.pose});
        ++it;
      }
    }
    for (auto it = sprites_.begin(); it != sprites_.end();) {
      const auto &sprite = it->second;
      const auto actor = actors_.find(sprite.actor);
      if (tick < sprite.tick || tick - sprite.tick > 6U ||
          actor == actors_.end() || actor->second.header != sprite.header ||
          actor->second.models != sprite.models)
        it = sprites_.erase(it);
      else {
        result.sprites.push_back(
            {it->first, sprite.actor, sprite.asset, actor->second.serial});
        ++it;
      }
    }
    for (auto it = particle_sprites_.begin(); it != particle_sprites_.end();) {
      const auto &sprite = it->second;
      const auto particle_live = particles_.find(sprite.particle);
      if (tick < sprite.tick || tick - sprite.tick > 6U ||
          particle_live == particles_.end() || particle_live->second != sprite.serial ||
          ram.u16(sprite.particle + 0x2CU) != 2U ||
          ram.u32(sprite.particle + 0x4CU) != sprite.asset)
        it = particle_sprites_.erase(it);
      else {
        result.sprites.push_back({it->first, sprite.particle | 0x80000000U,
                                  sprite.asset, sprite.serial});
        ++it;
      }
    }
    return result;
  }

private:
  struct Lifetime {
    std::uint64_t serial;
    std::uint32_t header, models;
  };
  struct Camera {
    std::uint64_t identity, tick;
    std::array<std::uint32_t,16> words{};
    RigidPresentationPose pose{};
    bool pose_valid=false;
  };
  struct Sprite {
    std::uint32_t actor, asset, header, models;
    std::uint64_t tick;
  };
  struct Rigid {
    std::uint32_t actor, header, models;
    std::uint64_t tick, camera;
    std::array<std::uint32_t, 16> words;
    RigidPresentationPose pose{};
  };
  std::map<std::uint32_t, Rigid> rigids_;
  struct ParticleSprite {
    std::uint32_t particle, asset;
    std::uint64_t serial, tick;
  };
  std::map<std::uint32_t, std::uint64_t> particles_;
  std::map<std::uint32_t, ParticleSprite> particle_sprites_;
  std::map<std::uint32_t, Sprite> sprites_;
  std::map<std::uint32_t, Lifetime> actors_;
  std::map<std::uint32_t, Camera> cameras_;
  std::uint64_t serial_ = 0;
  std::uint32_t level_ = UINT32_MAX;
  std::uint64_t camera_generation_=0;
  void level(std::uint32_t current) {
    if (current != level_) {
      actors_.clear();
      rigids_.clear();
      cameras_.clear();
      sprites_.clear();
      particles_clear();
      level_ = current;
      camera_generation_=++serial_;
    }
  }
};
} // namespace jfg
