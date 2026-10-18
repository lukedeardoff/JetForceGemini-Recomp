#pragma once

// Optional host-side tooling for the supported US ROM. No game assets are
// embedded.
#include "jfg/mod/npc_rewards.hpp"
#include "jfg/mod/navigation_pilot.hpp"
#include "jfg/mod/navigation_confirmation.hpp"
#include "jfg/mod/navigation_dialogue.hpp"
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace jfg::mod {
class Memory {
  std::span<std::uint8_t> bytes_;
  bool swapped_;

public:
  explicit Memory(std::span<std::uint8_t> bytes, bool native_words = true)
      : bytes_(bytes), swapped_(native_words) {
    if (native_words && bytes.size() % 4U != 0U)
      throw std::runtime_error("native mod memory must contain complete words");
  }
  bool valid(std::uint32_t a, std::uint64_t n) const noexcept {
    const auto segment = a & 0xE0000000U;
    const auto offset = a & 0x1FFFFFFFU;
    return (segment == 0x80000000U || segment == 0xA0000000U) &&
           offset <= bytes_.size() && n <= bytes_.size() - offset;
  }
  void require(std::uint32_t a, std::uint64_t n) const {
    if (!valid(a, n))
      throw std::runtime_error("invalid mod memory range");
  }
  std::uint8_t u8(std::uint32_t a) const {
    require(a, 1);
    return bytes_[(a & 0x1FFFFFFFU) ^ (swapped_ ? 3U : 0U)];
  }
  std::uint16_t u16(std::uint32_t a) const {
    require(a, 2);
    return static_cast<std::uint16_t>((u8(a) << 8U) | u8(a + 1U));
  }
  std::int16_t s16(std::uint32_t a) const {
    return std::bit_cast<std::int16_t>(u16(a));
  }
  std::uint32_t u32(std::uint32_t a) const {
    require(a, 4);
    return (static_cast<std::uint32_t>(u16(a)) << 16U) | u16(a + 2U);
  }
  float f32(std::uint32_t a) const {
    const auto value = std::bit_cast<float>(u32(a));
    if (!std::isfinite(value))
      throw std::runtime_error("nonfinite mod coordinate");
    return value;
  }
  void put16(std::uint32_t a, std::uint16_t v) {
    require(a, 2);
    bytes_[(a & 0x1FFFFFFFU) ^ (swapped_ ? 3U : 0U)] =
        static_cast<std::uint8_t>(v >> 8U);
    bytes_[((a + 1U) & 0x1FFFFFFFU) ^ (swapped_ ? 3U : 0U)] =
        static_cast<std::uint8_t>(v);
  }
};

struct Vec3 {
  float x{}, y{}, z{};
};
inline Vec3 position(const Memory &m, std::uint32_t actor) {
  m.require(actor, 0x18);
  return {m.f32(actor + 12), m.f32(actor + 16), m.f32(actor + 20)};
}
inline void json_vec(std::ostream &out, Vec3 p) {
  out << '[' << p.x << ',' << p.y << ',' << p.z << ']';
}

// Read-only player telemetry for the qualified US control/animation layout.
// Hang acceptance writes state 6; grab acceptance writes state 12. Other
// movement-state values stay numeric until separately identified.
struct PlayerMotion {
  bool known{}; unsigned state{}, animation{}; float animation_frame{};
};
inline PlayerMotion player_motion(const Memory &m,std::uint32_t actor) noexcept {
  PlayerMotion result;
  try {
    if(!m.valid(actor,0x6CU) || (actor&3U) ||
       m.u32(0x80036234U)!=0x24190006U || m.u32(0x80036248U)!=0xA2190568U ||
       m.u32(0x80036944U)!=0x2408000CU || m.u32(0x8003696CU)!=0xA2280568U ||
       m.u32(0x80011580U)!=0xA086003BU)return result;
    const auto control=m.u32(actor+0x68U);
    if((control&3U)||!m.valid(control,0x569U))return result;
    result.state=m.u8(control+0x568U);
    result.animation=m.u8(actor+0x3BU);
    result.animation_frame=m.f32(actor+0x28U);
    result.known=true;
  }catch(const std::runtime_error &){}
  return result;
}
inline void write_player_motion(std::ostream &out,const PlayerMotion &value) {
  out<<"{\"known\":"<<(value.known?"true":"false");
  if(value.known)out<<",\"state_id\":"<<value.state<<",\"animation_id\":"<<value.animation
      <<",\"animation_frame\":"<<value.animation_frame
      <<",\"hang_entry\":"<<(value.state==6?"true":"false")
      <<",\"grab_entry\":"<<(value.state==12?"true":"false");
  out<<'}';
}

// Identifiers observed in the game's tribal classification, not name matching.
inline bool tribal(std::uint16_t type) noexcept {
  return (type >= 0x11CU && type < 0x121U) || type == 0x66U || type == 0x70U ||
         type == 0x90U || type == 0x97U || type == 0x157U || type == 0xA5U;
}
inline std::uint16_t health_capacity(std::int16_t upgrades) noexcept {
  return upgrades >= 0 && upgrades <= 24
             ? static_cast<std::uint16_t>(
                   (5U * static_cast<unsigned>(upgrades) + 4U) * 256U)
             : 0U;
}

struct Actor {
  std::uint32_t address{};
  std::uint16_t behavior{};
  Vec3 position{};
};
// Read-only classification of normal squad members. Scenery, unsupported
// behaviors and incomplete objects remain unknown; tribals are never hostile.
struct EnemyState {
  bool known = false, hostile = false;
  std::int16_t health = 0;
};
inline EnemyState enemy_state(const Memory &m, const Actor &actor) noexcept {
  try {
    if (actor.behavior != 24U || !m.valid(actor.address, 0x6CU)) return {};
    const auto control = m.u32(actor.address + 0x68U);
    const auto properties = m.u32(actor.address + 0x4CU);
    if (!m.valid(control, 0x28U) || !m.valid(properties, 8U)) return {};
    const auto squad = m.u32(control + 0x24U);
    if (!m.valid(squad, 0x6CU) || m.u16(squad + 0x48U) != 23U) return {};
    const auto data = m.u32(squad + 0x68U);
    if (!m.valid(data, 0x2EU)) return {};
    return {true, !tribal(m.u16(data + 0x2CU)), m.s16(properties + 6U)};
  } catch (const std::runtime_error &) { return {}; }
}
inline void write_enemy_state(std::ostream &out, const EnemyState &state) {
  out << ",\"hostile_known\":" << (state.known ? "true" : "false")
      << ",\"hostile\":" << (state.hostile ? "true" : "false")
      << ",\"health\":" << state.health;
}
struct Exit {
  std::uint32_t address{};
  Vec3 position{}, normal{};
  float plane_d{};
  std::uint32_t radius{};
  std::uint16_t destination{};
  std::int16_t condition{}, directional{};
};
struct Triangle {
  std::array<std::uint32_t, 3> vertices{};
  std::uint32_t flags{}, block{};
  Vec3 normal{};
};
struct Mesh {
  std::vector<Vec3> vertices;
  std::vector<Triangle> triangles;
  std::uint32_t blocks{};
};

inline std::vector<Actor> actors(const Memory &m) {
  const auto table = m.u32(0x800F2CA4U), count = m.u32(0x800F2CA8U);
  if (count > 1024U)
    throw std::runtime_error("invalid mod actor count");
  if (count == 0U)
    return {};
  m.require(table, static_cast<std::uint64_t>(count) * 4U);
  std::vector<Actor> result;
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto a = m.u32(table + i * 4U);
    if (!m.valid(a, 0xA4U))
      continue;
    try {
      result.push_back({a, m.u16(a + 0x48U), position(m, a)});
    } catch (const std::runtime_error
                 &) { /* Incomplete objects are not actionable. */
    }
  }
  return result;
}

// Registered model bounds are broad-phase boxes, not exact collision polygons.
// Offsets and accessor pin also used by scripts/phase95_collision.py.
struct CollisionModel {
  std::uint32_t address{};
  Vec3 lower{}, upper{};
  bool enabled{};
  Mesh surface; // Present only for qualified live bridge geometry.
};
struct CollisionInventory {
  bool known{};
  const char *reason = "unavailable";
  std::vector<CollisionModel> models;
};
inline std::string actor_name(const Memory &m, std::uint32_t actor) {
  std::string name;
  try {
    const auto header = m.u32(actor + 0x40U);
    m.require(header, 0x14U);
    for (unsigned i = 0; i < 16; ++i) {
      const auto c = m.u8(header + 4U + i);
      if (c == 0) break;
      // Keep JSON and UI text safe without copying names into the executable.
      name += c >= 32 && c <= 126 && c != '"' && c != '\\' ? char(c) : '?';
    }
  } catch (const std::runtime_error &) {}
  return name.empty() ? "Unnamed entity" : name;
}
// Qualified bridges (55) and chests (98) use the registered current vertex buffer.
// Read that buffer and the collision face normals, transformed by the same
// double-buffered local-to-world matrix used by the resident collision code.
// Other model types retain conservative bounds until separately qualified.
inline Mesh bridge_surface(const Memory &m, std::uint32_t actor, std::uint32_t state) {
  Mesh result;
  const auto behavior=m.u16(actor+0x48U);
  if (behavior!=55U && behavior!=98U) return result;
  m.require(state,0x134U);
  const auto matrix_index=m.u8(state+0x12BU), model_index=m.u8(state+0x133U);
  if(matrix_index>1U||model_index>31U)throw std::runtime_error("bridge buffer index");
  const auto table=m.u32(actor+0x6CU);
  m.require(table,std::uint64_t(model_index+1U)*4U);
  const auto instance=m.u32(table+model_index*4U);m.require(instance,0x98U);
  const auto model=m.u32(instance);m.require(model,0x64U);
  const auto nv=m.u16(model+0x12U),nf=m.u16(model+0x14U),nb=m.u16(model+0x16U);
  if(nv<3U||nv>8192U||nf<1U||nf>8192U||nb<1U||nb>1024U)throw std::runtime_error("bridge mesh counts");
  const auto vertices=m.u32(instance+4U),faces=m.u32(model+0x20U),batches=m.u32(model+0x24U);
  const auto collision=m.u32(instance+12U)?m.u32(instance+12U):m.u32(model+0x60U);
  m.require(vertices,std::uint64_t(nv)*10U);m.require(faces,std::uint64_t(nf)*16U);
  m.require(batches,std::uint64_t(nb+1U)*16U);m.require(collision,16U);
  const auto facets=m.u32(collision),planes=m.u32(collision+12U);
  m.require(facets,std::uint64_t(nf)*14U);
  std::array<float,16> matrix{};
  for(unsigned i=0;i<16U;++i)matrix[i]=m.f32(state+0x80U+matrix_index*64U+i*4U);
  const auto transform=[&](Vec3 p,bool point) {
    return Vec3{p.x*matrix[0]+p.y*matrix[4]+p.z*matrix[8]+(point?matrix[12]:0),
                p.x*matrix[1]+p.y*matrix[5]+p.z*matrix[9]+(point?matrix[13]:0),
                p.x*matrix[2]+p.y*matrix[6]+p.z*matrix[10]+(point?matrix[14]:0)};
  };
  for(unsigned i=0;i<nv;++i) {
    const auto at=vertices+i*10U;
    const auto v=transform({float(m.s16(at)),float(m.s16(at+2U)),float(m.s16(at+4U))},true);
    if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||
       std::abs(v.x)>1000000||std::abs(v.y)>1000000||std::abs(v.z)>1000000)
      throw std::runtime_error("bridge world coordinate");
    result.vertices.push_back(v);
  }
  unsigned expected=0;
  for(unsigned b=0;b<nb;++b) {
    const auto at=batches+b*16U;
    const auto base=m.u16(at+6U),first=m.u16(at+8U),end=m.u16(at+24U);
    if(first!=expected||first>end||end>nf)throw std::runtime_error("bridge face ranges");
    expected=end;
    for(unsigned f=first;f<end;++f) {
      Triangle tri;tri.flags=m.u32(at+12U);
      for(unsigned j=0;j<3;++j) {
        tri.vertices[j]=base+m.u8(faces+f*16U+1U+j);
        if(tri.vertices[j]>=nv)throw std::runtime_error("bridge face index");
      }
      const auto index=m.u16(facets+f*14U);
      if(index>8191U)throw std::runtime_error("bridge plane index");
      m.require(planes,std::uint64_t(index)*16U+16U);
      const auto plane=planes+unsigned(index)*16U;
      tri.normal=transform({m.f32(plane),m.f32(plane+4U),m.f32(plane+8U)},false);
      result.triangles.push_back(tri);
    }
  }
  if(expected!=nf)throw std::runtime_error("incomplete bridge faces");
  return result;
}
inline void write_mesh(std::ostream &, const Mesh &, std::uint32_t, std::uint64_t);

inline CollisionInventory collision_models(const Memory &m,
                                           const std::vector<Actor> &list) {
  CollisionInventory result;
  try {
    constexpr std::array<std::uint32_t, 7> accessor = {
        0x3C0E8010, 0x8DCE47E0, 0x3C028010, 0xAC8E0000,
        0x8C4247E4, 0x03E00008, 0};
    for (unsigned i = 0; i < accessor.size(); ++i)
      if (m.u32(0x8007E484U + i * 4U) != accessor[i]) {
        result.reason = "unsupported collision accessor"; return result;
      }
    const auto count = m.u32(0x801047E0U), table = m.u32(0x801047E4U);
    if (count > 1024U) throw std::runtime_error("collision count");
    if (count) {
      if (table & 3U) throw std::runtime_error("collision table alignment");
      m.require(table, std::uint64_t(count) * 4U);
    }
    for (unsigned i = 0; i < count; ++i) {
      const auto actor = m.u32(table + 4U * i);
      bool present = false;
      for (const auto &a : list)
        if ((a.address & 0x1FFFFFFFU) == (actor & 0x1FFFFFFFU)) present = true;
      if (!present || (actor & 3U)) throw std::runtime_error("stale collision actor");
      m.require(actor, 0x60U);
      for (const auto &prior : result.models)
        if ((prior.address & 0x1FFFFFFFU) == (actor & 0x1FFFFFFFU))
          throw std::runtime_error("duplicate collision actor");
      const auto state = m.u32(actor + 0x5CU), properties = m.u32(actor + 0x4CU);
      if ((state | properties) & 3U) throw std::runtime_error("collision alignment");
      m.require(state, 0x118U); m.require(properties, 12U);
      CollisionModel model{actor, {}, {}, (m.u16(properties + 10U) & 1U) != 0, {}};
      model.lower = {m.f32(state + 0x100U), m.f32(state + 0x104U), m.f32(state + 0x108U)};
      model.upper = {m.f32(state + 0x10CU), m.f32(state + 0x110U), m.f32(state + 0x114U)};
      const auto valid = [](float low, float high) {
        return low <= high && std::abs(low) <= 1000000 && std::abs(high) <= 1000000;
      };
      if (!valid(model.lower.x, model.upper.x) || !valid(model.lower.y, model.upper.y) ||
          !valid(model.lower.z, model.upper.z)) throw std::runtime_error("invalid collision bounds");
      if(model.enabled)try {model.surface=bridge_surface(m,actor,state);}
        catch(const std::runtime_error &) {model.surface={};}
      result.models.push_back(std::move(model));
    }
    result.known = true; result.reason = "registered model bounds";
  } catch (const std::runtime_error &) {
    // An incomplete registry must never masquerade as a clear route.
    result.models.clear(); result.reason = "invalid or incomplete collision registry";
  }
  return result;
}
inline void write_collision(std::ostream &out, const CollisionInventory &inventory) {
  out << "{\"schema\":1,\"known\":" << (inventory.known ? "true" : "false")
      << ",\"reason\":\"" << inventory.reason << "\",\"models\":[";
  bool first = true;
  for (const auto &model : inventory.models) {
    if (!first) out << ',';
    first = false;
    out << "{\"address\":" << model.address << ",\"enabled\":"
        << (model.enabled ? "true" : "false") << ",\"lower\":";
    json_vec(out, model.lower); out << ",\"upper\":"; json_vec(out, model.upper);
    if(!model.surface.triangles.empty()) {out << ",\"surface\":";write_mesh(out,model.surface,0,0);}
    out << '}';
  }
  out << "]}";
}

inline std::vector<Exit> exits(const Memory &m,
                               const std::vector<Actor> &list) {
  std::vector<Exit> result;
  for (const auto &a : list) {
    if (a.behavior != 8U)
      continue;
    const auto control = m.u32(a.address + 0x68U),
               definition = m.u32(a.address + 0x3CU);
    if (!m.valid(control, 0x18U) || !m.valid(definition, 0x1CU))
      continue;
    try {
      result.push_back(
          {a.address,
           a.position,
           {m.f32(control), m.f32(control + 4), m.f32(control + 8)},
           m.f32(control + 12),
           m.u32(control + 0x10U),
           static_cast<std::uint16_t>(m.u8(definition + 0xAU) |
                                      (m.u8(definition + 0xBU) << 8U)),
           m.s16(control + 0x14U),
           m.s16(control + 0x16U)});
    } catch (const std::runtime_error &) {
    }
  }
  return result;
}

struct NpcMarker {
  std::uint32_t address{};
  Vec3 position{};
  std::uint16_t object_id{}, behavior{};
  int squad_type = -1;
  const char *kind = "npc";
  const char *label = "NPC";
};
// Descriptive labels verified against the supported ROM's object headers.
// These are character categories, not inferred quest/reward completion.
inline const char *npc_label(std::uint16_t object) noexcept {
  switch (object) {
  case 350:
  case 550:
  case 551:
    return "NPC: Guide";
  case 476:
    return "NPC: King";
  case 486:
    return "NPC: Wise character";
  case 487:
    return "NPC: Limpet";
  case 488:
  case 489:
  case 490:
    return "NPC: Old mole";
  case 612:
    return "NPC: Sidekick";
  case 772:
    return "NPC: DJ";
  default:
    return "NPC";
  }
}
inline const char *tribal_label(std::uint16_t type) noexcept {
  switch (type) {
  case 0x66:
    return "Tribal: Chief";
  case 0x70:
    return "Tribal: Lantern";
  case 0x90:
    return "Tribal: Pickaxe";
  case 0x97:
    return "Tribal: Barrow";
  case 0xA5:
    return "Tribal: Spade";
  case 0x157:
    return "Tribal: Hammer";
  case 0x11E:
  case 0x11F:
    return "Tribal: Child";
  case 0x120:
    return "Tribal: Baby";
  default:
    return "Tribal";
  }
}
inline std::vector<NpcMarker> npc_markers(const Memory &m,
                                          const std::vector<Actor> &list) {
  std::vector<NpcMarker> result;
  for (const auto &a : list) {
    if (a.behavior != 90U && a.behavior != 24U)
      continue;
    try {
      m.require(a.address, 0xA4U);
      const auto control = m.u32(a.address + 0x68U);
      const auto properties = m.u32(a.address + 0x4CU);
      if (!m.valid(properties, 8U))
        continue;
      NpcMarker npc{a.address,  a.position, m.u16(a.address + 0x4AU),
                    a.behavior, -1,         "npc",
                    "NPC"};
      if (a.behavior == 90U) {
        // Shared dialogue controller (mrhints), not scenery name matching.
        if (!m.valid(control, 0x38U) || m.s16(properties + 6U) == 0)
          continue;
        npc.label = npc_label(npc.object_id);
      } else {
        if (!m.valid(control, 0x28U) || m.s16(properties + 6U) <= 0)
          continue;
        const auto squad = m.u32(control + 0x24U);
        if (!m.valid(squad, 0xA4U) || m.u16(squad + 0x48U) != 23U)
          continue;
        const auto data = m.u32(squad + 0x68U);
        if (!m.valid(data, 0x2EU))
          continue;
        const auto type = m.u16(data + 0x2CU);
        if (!tribal(type))
          continue;
        npc.squad_type = type;
        npc.kind = "tribal";
        npc.label = tribal_label(type);
      }
      result.push_back(npc);
    } catch (const std::runtime_error &) {
      // Loading or removed actors are not valid interaction targets.
    }
  }
  return result;
}
inline void write_npcs(std::ostream &out, const std::vector<NpcMarker> &list) {
  out << '[';
  bool first = true;
  for (const auto &npc : list) {
    if (!first)
      out << ',';
    first = false;
    out << "{\"address\":" << npc.address << ",\"position\":";
    json_vec(out, npc.position);
    // Labels are host-owned literals; no unescaped ROM text enters JSON.
    out << ",\"kind\":\"" << npc.kind << "\",\"label\":\"" << npc.label
        << "\",\"object_id\":" << npc.object_id
        << ",\"behavior\":" << npc.behavior
        << ",\"squad_type\":" << npc.squad_type << '}';
  }
  out << ']';
}

struct ItemMarker {
  std::uint32_t address{};
  Vec3 position{};
  std::string kind, label;
  int content_code = -1;
  bool opened = false;
};
// Weapon indices used by the inventory and weapon-pickup controller.
inline const char *weapon_name(unsigned index) {
  constexpr std::array<const char *, 15> names = {"Pistol", "Homing missiles", "Machine gun", "Plasma shotgun", "Shocker", "Tri-rocket launcher", "Flamethrower", "Sniper rifle", "Grenades", "Shurikens", "Fish Food", "Timed mines", "Remote mines", "Flares", "Cluster bombs"};
  return index < names.size() ? names[index] : "Unknown weapon";
}
// Chest reward codes differ from the saved-inventory item bit indices.
// Confirmed by the original pickup messages during the owner's live playtest.
inline int chest_inventory_item(int content_code) noexcept {
  switch (content_code) {
  case 7: return 16; // Specialist magazine.
  case 18: return 9; // Blue key.
  default: return -1;
  }
}
inline std::vector<ItemMarker> item_markers(const Memory &m,
                                            const std::vector<Actor> &list) {
  std::vector<ItemMarker> result;
  for (const auto &a : list) {
    try {
      ItemMarker item{a.address, a.position, "", "", -1, false};
      const auto object = m.u16(a.address + 0x4AU);
      if (a.behavior == 98U) {
        const auto control = m.u32(a.address + 0x68U);
        if (!m.valid(control, 6U))
          continue;
        item.content_code = m.u8(control + 5U);
        item.opened = m.u8(control + 4U) == 5U;
        // Chest reward switch, including duplicate entries, verified against
        // the supported ROM's jump table. -1 denotes a special collectable
        // decoded separately from weapon reward indices.
        constexpr std::array<int, 21> weapons = {3,  1,  2,  2,  9,  8,  -1,
                                                 -1, 14, 13, 14, 12, 11, 10,
                                                 7,  6,  4,  -1, -1, -1, 5};
        const int weapon =
            item.content_code < static_cast<int>(weapons.size())
                ? weapons[static_cast<unsigned>(item.content_code)]
                : -1;
        item.kind = item.opened ? "opened" : weapon >= 0 ? "weapon" : "chest";
        item.label = "Chest: ";
        const int inventory_item = chest_inventory_item(item.content_code);
        if (weapon >= 0)
          item.label += weapon_name(static_cast<unsigned>(weapon));
        else if (inventory_item >= 0)
          item.label += npc_item_name(inventory_item);
        else
          item.label += "item " + std::to_string(item.content_code) +
                        " (unidentified)";
        if (item.opened)
          item.label += " (opened)";
      } else if (a.behavior == 109U) {
        // robotTokenControl pickup actor, confirmed in the tutorial Fish Food room.
        item.kind = "token";
        item.label = "Mizar token";
      } else if (a.behavior == 61U) {
        item.kind = "health";
        item.label = "Health";
      } else if (a.behavior == 66U) {
        item.kind = "ammo";
        item.label = "Ammo / capacity";
      } else {
        // The original pickup initializer matches these object IDs against
        // weaponPowerUpObjects before storing its inventory index at +0x88.
        bool weapon_object = object == 0x88U;
        for (unsigned n = 0; n < 15U; ++n)
          weapon_object =
              weapon_object ||
              (object != 0U && object == m.u16(0x800A0CB0U + n * 2U));
        if (weapon_object) {
          const auto index = m.u16(a.address + 0x88U);
          if (index >= 15U)
            continue;
          item.kind = "weapon";
          item.label = weapon_name(index);
          item.content_code = index;
        } else {
          const auto header = m.u32(a.address + 0x40U);
          if (!m.valid(header, 20U))
            continue;
          std::string name;
          for (unsigned n = 4; n < 20U; ++n) {
            const auto c = m.u8(header + n);
            if (c == 0U)
              break;
            if (c < 32U || c > 126U) {
              name.clear();
              break;
            }
            name += static_cast<char>(c);
          }
          std::string lower = name;
          for (char &c : lower)
            if (c >= 'A' && c <= 'Z')
              c = static_cast<char>(c + ('a' - 'A'));
          // A key-like name alone could be a switch. Require the object-ID
          // family used by keyInit as well; retain the raw variant.
          if (object >= 0x258U && object <= 0x25FU &&
              lower.find("key") != std::string::npos) {
            item.kind = "key";
            item.label = "Key";
            item.content_code = static_cast<int>(object - 0x258U);
          } else
            continue;
        }
      }
      result.push_back(std::move(item));
    } catch (const std::runtime_error &) {
      // Incomplete objects may disappear between updates; never guess.
    }
  }
  return result;
}

// Read-only progression facts for the supported US single-player layout.
// Unknown requirements/completion remain explicit; these are not route edges.
struct Inventory {
  bool known = false, red_key = false, magnus_spoken = false;
  unsigned character = 0, weapons = 0;
};
inline Inventory inventory(const Memory &m, std::uint32_t player) noexcept {
  try {
    if (!m.valid(player, 0xA4U) || m.u8(0x800A4FC4U) != 0U)
      return {};
    const auto control = m.u32(player + 0x68U), game = m.u32(0x800FD7D4U);
    if (!m.valid(control, 2U) || !m.valid(game, 0x334U))
      return {};
    const unsigned character = m.u8(control + 1U) & 3U;
    if (character == 3U)
      return {};
    const auto saved = game + 0x15CU + character * 0x76U;
    return {true, (m.u8(saved + 0x66U) & 0x40U) != 0U,
            (m.u8(game + 8U) & 8U) != 0U, character, m.u16(saved + 0xAU)};
  } catch (const std::runtime_error &) {
    return {};
  }
}
inline NpcFacts npc_facts(const Memory &m, const Inventory &inv,
                          std::uint32_t player = 0U) noexcept {
  NpcFacts f;
  try {
    if (!inv.known)
      return f;
    const auto game = m.u32(0x800FD7D4U);
    m.require(game, 0x334U);
    f.character = inv.character;
    f.weapons = inv.weapons;
    for (unsigned c = 0; c < 3; ++c)
      for (unsigned i = 0; i < 4; ++i)
        f.items[c][i] = m.u8(game + 0x15CU + c * 0x76U + 0x66U + i);
    for (unsigned i = 0; i < f.flags.size(); ++i)
      f.flags[i] = m.u8(game + 0x30U + i);
    f.currency = m.s16(game + 0x15CU + inv.character * 0x76U + 0x12U);
    if (m.valid(player, 0xA4U)) {
      const auto control = m.u32(player + 0x68U);
      if (m.valid(control, 0x18AU))
        f.race = m.u8(control + 0x189U);
    }
    f.known = true;
  } catch (const std::runtime_error &) {
    return {};
  }
  return f;
}
inline void write_inventory_tracker(std::ostream &out,const Memory &m,std::uint32_t player) {
  const auto inv=inventory(m,player);
  out<<"{\"known\":"<<(inv.known?"true":"false")<<",\"current\":"<<(inv.known?int(inv.character):-1)<<",\"characters\":[";
  if(inv.known) {
    const auto game=m.u32(0x800FD7D4U);const auto facts=npc_facts(m,inv,player);
    for(unsigned c=0;c<3;++c) {
      if(c)out<<',';
      const auto saved=game+0x15CU+c*0x76U;
      out<<"{\"id\":"<<c<<",\"weapons\":"<<m.u16(saved+0xAU)<<",\"items\":[";
      for(unsigned i=0;i<28;++i){if(i)out<<',';out<<((facts.items[c][i>>3]&(0x80U>>(i&7)))?"true":"false");}
      out<<"]}";
    }
    out<<"],\"shared\":[";
    for(unsigned i=32;i<=43;++i){if(i!=32)out<<',';out<<(facts.flag(i)==Fact::met?"true":"false");}
  }else out<<"],\"shared\":[";
  out<<"]}";
}
struct ChestAccess {
  bool known{};
  Vec3 point{};
  float radius{}, max_height{};
  std::int16_t facing{};
};
inline ChestAccess chest_access(const Memory &m,std::uint32_t actor) {
  // Observe the game's cached activation region. ObjectChest refreshes these
  // fields for the current character when A is pressed; never infer access
  // from the decorative model centre or skip its facing/height requirements.
  if(!m.valid(actor,0x6CU)||m.u16(actor+0x48U)!=98U||
      m.u32(0x8003B6C8U)!=0x27BDFFE8U)return {};
  const auto data=m.u32(actor+0x68U);
  if(!m.valid(data,0x24U))return {};
  try {
    const auto p=position(m,actor);
    const float dx=m.f32(data+0x10U),dy=m.f32(data+0x14U),dz=m.f32(data+0x18U);
    const float radius=m.f32(data+0x1CU),height=m.f32(data+0x20U);
    if(!std::isfinite(dx)||!std::isfinite(dy)||!std::isfinite(dz)||!std::isfinite(radius)||!std::isfinite(height)||
        std::abs(dx)>1000||std::abs(dy)>1000||std::abs(dz)>1000||
        radius<1||radius>256||height<0||height>256)return {};
    return {true,{p.x+dx,p.y+dy,p.z+dz},radius,height,
      static_cast<std::int16_t>(static_cast<std::uint16_t>(m.s16(actor))+0x8000U)};
  }catch(const std::runtime_error &){return {};}
}
struct Interaction {
  std::uint32_t address{}, linked_actor{};
  Vec3 position{};
  ChestAccess activation{};
  std::string kind, label, action, status = "unknown";
  std::string requirement = "Unknown", reward = "Unknown";
  std::vector<NpcOffer> offers;
  bool npc_catalog_known = false;
  float talk_radius{},talk_lower{},talk_upper{};
  bool requirement_known = false;
  bool access_known=false, access_allowed=false, condition_known=false, condition_met=false;
  unsigned pending_openers=0, approach_radius=0;
  int reward_item = -1, reward_weapon = -1, required_weapon = -1;
  int spoken = -1, encounter = -1, dialogue = -1;
  int raw_state = -1, raw_condition = -1;
  int door_id = -1, required_item = -1, target_health = -1,
      target_max_health = -1, reset_ticks = -1;
};

// US doorControl/exitControl observations; read the user's live state, never
// embed the ROM's condition table or alter its locks.
inline void door_access(Interaction &node,const Memory &m,std::uint32_t control,
                        const Inventory &inv,const NpcFacts &facts,std::uint32_t player) {
  if(!m.valid(control,0x4CU)||m.u32(0x80025A08U)!=0x27BDFF48U)return;
  node.raw_state=m.u16(control+0x3EU);node.raw_condition=m.u8(control+0x43U);
  node.door_id=m.u8(control+0x44U);node.approach_radius=m.u16(control+0x40U);
  node.pending_openers=m.u8(control+0x4AU);
  const bool locked=(node.raw_state&8)!=0;
  node.requirement_known=true;node.access_known=true;node.access_allowed=!locked;
  if(node.raw_condition>=2) {
    node.required_item=node.raw_condition-2;
    node.requirement=npc_item_name(node.required_item);
    node.label=node.requirement+" door";
    const auto owned=node.required_item==1&&inv.known?fact(inv.red_key):facts.item(static_cast<unsigned>(node.required_item));
    node.access_known=!locked||owned!=Fact::unknown;node.access_allowed=!locked||owned==Fact::met;
    node.status=!locked?"key_lock_cleared":owned==Fact::met?"key_owned":owned==Fact::missing?"key_missing":"key_inventory_unavailable";
  } else if(node.raw_state&0x20) {
    node.label="Enemy-clear door";node.requirement="Clear registered enemy groups";
    node.status=locked?"enemies_remaining":"enemy_lock_cleared";
  } else if(node.raw_state&0x40) {
    node.label="Target-controlled door";node.requirement="Activate linked shooting targets";
    node.status=locked?"target_required":"target_lock_cleared";
  } else {
    node.label=m.u8(control+0x42U)==5?"Proximity curtain":"Proximity door";
    node.requirement=locked?"External opener; source not identified":"Approach the door";
    node.status=locked?"opener_required":(node.raw_state&2)?((node.raw_state&4)?"opening":"closing"):
      (node.raw_state&1)?"open":"opens_on_approach";
    if(locked)node.requirement_known=false;
  }
  // Character exclusions and additional progression predicates remain separate
  // from the door's animation and latch.
  const unsigned restrictions=m.u8(control+0x4BU);
  Fact allowed=Fact::met;
  if(restrictions&7) {
    node.requirement+="; character restriction";
    allowed=inv.known?fact((restrictions&(1U<<inv.character))==0):Fact::unknown;
  }
  if(restrictions&0x40) {
    node.requirement+="; progress flag 0";
    allowed=both(allowed,facts.flag(0));
  }
  if(restrictions&0x80) {
    node.requirement+="; player capability";
    auto pc=m.valid(player,0x6CU)?m.u32(player+0x68U):0U;
    allowed=both(allowed,m.valid(pc,0x510U)?fact(m.u32(pc+0x50CU)!=0):Fact::unknown);
  }
  if(restrictions&0x38) {allowed=both(allowed,Fact::unknown);node.requirement_known=false;}
  if(allowed==Fact::missing){node.status="character_or_progress_required";node.access_known=true;node.access_allowed=false;}
  else if(allowed==Fact::unknown){node.status="restriction_unverified";node.access_known=false;node.access_allowed=false;}
}
inline Fact exit_condition(const Memory &m,const Exit &exit,const NpcFacts &facts,std::uint32_t player) {
  if(m.u32(0x8002561CU)!=0x27BDFF70U)return Fact::unknown;
  if(exit.condition<0||exit.condition>=25)return Fact::met;
  Fact met=Fact::met;
  for(unsigned i=0;i<2;++i) {
    const auto at=0x800A1120U+unsigned(exit.condition)*4U+i*2U;
    const auto id=m.u8(at),expected=m.u8(at+1);
    if(id==255)continue;
    if(expected>1)return Fact::unknown;
    auto value=facts.flag(id);met=both(met,expected?value:negate(value));
  }
  if(exit.condition==3||exit.condition==4) {
    auto pc=m.valid(player,0x6CU)?m.u32(player+0x68U):0U;
    met=both(met,m.valid(pc,2U)?fact(((m.u8(pc+1)&4)!=0)==(exit.condition==4)):Fact::unknown);
  }
  if(exit.condition==5)met=both(met,facts.race<0?Fact::unknown:fact(facts.race==1));
  return met;
}

inline std::vector<Interaction>
interactions(const Memory &m, const std::vector<Actor> &list,
             std::uint32_t /* level */, const Inventory &inv,
             const NpcRewardCatalog *catalog = nullptr,
             std::uint32_t player = 0U) {
  std::vector<Interaction> result;
  for (const auto &npc : npc_markers(m, list)) {
    Interaction node;
    node.address = npc.address;
    node.position = npc.position;
    node.kind = npc.kind;
    node.label = npc.label;
    node.action = npc.behavior == 90U ? "talk" : "rescue";
    if (npc.behavior == 90U) {
      const auto control = m.u32(npc.address + 0x68U);
      node.raw_state = m.u8(control);
      node.talk_radius=m.f32(control+0x18U);
      node.talk_lower=m.f32(control+0x20U);node.talk_upper=m.f32(control+0x1CU);
      if(node.talk_radius<8||node.talk_radius>500||node.talk_lower>node.talk_upper||
         node.talk_lower< -1000||node.talk_upper>1000)node.talk_radius=0;
      node.dialogue = m.u8(control + 0x31U);
      node.encounter = m.u8(control + 0x32U);
      auto facts = npc_facts(m, inv, player);
      facts.first_conversation = m.u8(control + 1U) != 0U ? 1 : 0;
      if (facts.known) {
        const auto game = m.u32(0x800FD7D4U);
        node.spoken = (m.u8(game + 8U + (unsigned(node.encounter) >> 3U)) &
                       (0x80U >> (unsigned(node.encounter) & 7U))) != 0U
                          ? 1
                          : 0;
      }
      node.npc_catalog_known =
          catalog != nullptr && catalog->ready() && node.dialogue < 45;
      if (node.npc_catalog_known) {
        node.offers = catalog->offers(unsigned(node.dialogue), facts);
        node.requirement =
            "Select an offer below for its dialogue and trade requirements";
        node.requirement_known = true;
        node.reward = node.offers.empty()
                          ? "No direct item or service reward in this dialogue"
                          : "See NPC offers";
        node.status = node.offers.empty() ? "no_direct_reward" : "blocked";
        bool owned = false, unknown = false, available = false;
        std::set<std::string> names;
        for (const auto &offer : node.offers) {
          available |= offer.status == "available";
          owned |= offer.status == "owned";
          unknown |= offer.status == "unknown";
          if (offer.kind != "transition" && offer.kind != "music")
            names.insert(offer.reward);
        }
        if (!node.offers.empty())
          node.status = available ? "available"
                        : unknown ? "unknown"
                        : owned   ? "owned"
                                  : "blocked";
        if (names.size() == 1) {
          node.reward = *names.begin();
          node.label += ": " + node.reward;
          for (const auto &offer : node.offers)
            if (offer.reward == node.reward) {
              node.reward_item = offer.item;
              node.reward_weapon = offer.weapon;
              break;
            }
        } else if (names.size() > 1)
          node.label += ": " + std::to_string(names.size()) + " offers";
      }
    }
    result.push_back(std::move(node));
  }
  for (const auto &item : item_markers(m, list)) {
    if (item.kind == "token") continue; // Compact map dots, not interaction rows.
    Interaction node;
    node.address = item.address;
    node.position = item.position;
    node.kind = item.kind;
    node.label = item.label;
    const auto behavior = m.u16(item.address + 0x48U);
    node.action = behavior == 98U ? "open_chest" : "collect";
    node.reward = item.label;
    if (behavior == 98U) {
      node.activation=chest_access(m,item.address);
      constexpr std::array<int, 21> weapons = {3,  1,  2,  2,  9,  8,  -1,
                                               -1, 14, 13, 14, 12, 11, 10,
                                               7,  6,  4,  -1, -1, -1, 5};
      if (item.content_code >= 0 &&
          item.content_code < static_cast<int>(weapons.size()))
        node.reward_weapon = weapons[static_cast<unsigned>(item.content_code)];
      node.raw_state = m.u8(m.u32(item.address + 0x68U) + 4U);
      node.status = item.opened ? "opened" : "unopened";
      if (node.reward_weapon >= 0)
        node.reward = weapon_name(static_cast<unsigned>(node.reward_weapon));
      else {
        node.reward_item = chest_inventory_item(item.content_code);
        node.reward = node.reward_item >= 0
                          ? npc_item_name(node.reward_item)
                          : "Unknown special collectable";
      }
    } else if (item.kind == "weapon") {
      node.reward_weapon = item.content_code;
      node.status = "present";
    } else
      node.status = "present";
    // Presence/ownership does not establish access, reachability, or
    // collection.
    result.push_back(std::move(node));
  }
  for (const auto &actor : list) {
    if (actor.behavior != 111U && actor.behavior != 67U && actor.behavior != 7U)
      continue;
    try {
      const auto control = m.u32(actor.address + 0x68U);
      if (!m.valid(control, 0x10U))
        continue;
      Interaction node;
      node.address = actor.address;
      node.position = actor.position;
      if (actor.behavior == 111U) {
        node.kind = "target";
        node.label = "Shooting target";
        node.action = "shoot_target";
        // targetswitchControl: +8 is door ID, +A the activation latch.
        // +0 is a recovery timer, not a hit counter. No weapon restriction
        // inferred.
        node.raw_state = m.u8(control + 0xAU);
        node.raw_condition = m.s16(control + 4U);
        node.door_id = m.u8(control + 8U);
        node.label += " (door " + std::to_string(node.door_id) + ")";
        node.status = node.raw_state != 0 ? "activated" : "needs_shooting";
        node.requirement =
            "Repeated shots; weapon and timing constraints unverified";
        node.reward =
            "Activates linked door group " + std::to_string(node.door_id);
        node.reset_ticks = m.s16(control);
        node.target_max_health = m.u16(control + 6U);
        const auto props = m.u32(actor.address + 0x4CU);
        if (m.valid(props, 8U))
          node.target_health = m.s16(props + 6U);
        for (const auto &other : list) {
          if (other.behavior != 7U)
            continue;
          const auto dc = m.u32(other.address + 0x68U);
          if (m.valid(dc, 0x4BU) && m.u8(dc + 0x44U) == node.door_id) {
            node.linked_actor = other.address;
            break;
          }
        }
      } else if (actor.behavior == 7U) {
        if (!m.valid(control, 0x4BU))
          continue;
        node.kind = "gate";
        node.label = "Door";
        node.action = "pass_door";
        node.door_id = m.u8(control + 0x44U);
        node.label += " " + std::to_string(node.door_id);
        node.raw_state = m.u16(control + 0x3EU);
        node.raw_condition = m.u8(control + 0x43U);
        door_access(node,m,control,inv,npc_facts(m,inv,player),player);
      } else {
        node.kind = "gate";
        node.label = "Door indicator";
        node.action = "door_indicator";
        node.raw_state = m.u8(control + 8U);
        node.raw_condition = m.u8(control + 9U);
        const auto linked = m.u32(control + 4U);
        for (const auto &other : list)
          if (other.address == linked && other.behavior == 7U)
            { node.linked_actor = linked;
              door_access(node,m,m.u32(linked+0x68U),inv,npc_facts(m,inv,player),player);
              node.label="Indicator: "+node.label;
            }
      }
      result.push_back(std::move(node));
    } catch (const std::runtime_error &) {
    }
  }
  unsigned exit_index = 0;
  for (const auto &exit : exits(m, list)) {
    Interaction node;
    node.address = exit.address;
    node.position = exit.position;
    node.kind = "exit";
    node.label = "Exit " + std::to_string(++exit_index);
    node.action = "enter_exit";
    node.raw_condition = exit.condition;
    node.reward = "Destination code " + std::to_string(exit.destination);
    const auto condition=exit_condition(m,exit,npc_facts(m,inv,player),player);
    node.condition_known=condition!=Fact::unknown;node.condition_met=condition==Fact::met;
    node.access_known=node.condition_known;node.access_allowed=node.condition_met;
    node.requirement_known=node.condition_known;
    node.requirement=exit.condition<0||exit.condition>=25?"Enter the trigger":"Game progression condition "+std::to_string(exit.condition);
    node.status=condition==Fact::met?"open_passage":condition==Fact::missing?"inactive_alternative":"condition_unverified";
    // Proximity association is explicitly a nearby door, not a game pointer.
    // Refuse ambiguous/vertically distant matches.
    const Interaction *near=nullptr;float best=240,second=240;
    for(const auto &gate:result)if(gate.action=="pass_door") {
      const float dx=gate.position.x-exit.position.x,dy=gate.position.y-exit.position.y,dz=gate.position.z-exit.position.z;
      if(std::abs(dy)>180)continue;
      const float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
      if(distance<best){second=best;best=distance;near=&gate;}else if(distance<second)second=distance;
    }
    if(near&&second-best>32&&condition==Fact::met) {
      node.linked_actor=near->address;node.label+=": "+near->label;
      node.status=near->status;node.requirement=near->requirement;node.requirement_known=near->requirement_known;
      node.access_known=near->access_known;node.access_allowed=near->access_allowed;
      node.required_item=near->required_item;node.door_id=near->door_id;
      node.pending_openers=near->pending_openers;node.approach_radius=near->approach_radius;
    }

    result.push_back(std::move(node));
  }
  return result;
}
inline void write_progression(std::ostream &out, const Memory &m,
                              const std::vector<Actor> &list,
                              std::uint32_t player, std::uint32_t level,
                              const NpcRewardCatalog *catalog = nullptr) {
  bool present = false;
  for (const auto &a : list)
    if (a.address == player)
      present = true;
  const auto inv = inventory(m, present ? player : 0U);
  out << "{\"schema\":1,\"inventory\":{\"known\":"
      << (inv.known ? "true" : "false") << ",\"character\":"
      << (inv.known ? std::to_string(inv.character) : "null") << ",\"red_key\":"
      << (inv.known ? (inv.red_key ? "true" : "false") : "null")
      << ",\"weapons_mask\":"
      << (inv.known ? std::to_string(inv.weapons) : "null")
      << "},\"npc_catalog\":{\"known\":"
      << (catalog != nullptr && catalog->ready() ? "true" : "false")
      << ",\"dialogue_groups\":" << (catalog ? catalog->group_count() : 0)
      << ",\"choice_tables\":" << (catalog ? catalog->choice_count() : 0)
      << "},\"nodes\":[";
  bool first = true;
  for (const auto &node : interactions(m, list, level, inv, catalog, player)) {
    if (!first)
      out << ',';
    first = false;
    out << "{\"address\":" << node.address << ",\"position\":";
    json_vec(out, node.position);
    out << ",\"kind\":\"" << node.kind << "\",\"label\":\"" << node.label
        << "\",\"action\":\"" << node.action << "\",\"status\":\""
        << node.status << "\",\"requirement\":\"" << node.requirement
        << "\",\"reward\":\"" << node.reward << "\",\"requirement_known\":"
        << (node.requirement_known ? "true" : "false")
        << ",\"access_known\":" << (node.access_known?"true":"false")
        << ",\"access_allowed\":" << (node.access_allowed?"true":"false")
        << ",\"condition_known\":" << (node.condition_known?"true":"false")
        << ",\"condition_met\":" << (node.condition_met?"true":"false")
        << ",\"pending_openers\":" << node.pending_openers
        << ",\"approach_radius\":" << node.approach_radius
        << ",\"reward_item\":" << node.reward_item
        << ",\"reward_weapon\":" << node.reward_weapon
        << ",\"activation\":{\"known\":" << (node.activation.known?"true":"false")
        << ",\"point\":[" << node.activation.point.x << ',' << node.activation.point.y << ',' << node.activation.point.z
        << "],\"radius\":" << node.activation.radius << ",\"max_height\":" << node.activation.max_height
        << ",\"facing\":" << node.activation.facing << '}'
        << ",\"required_weapon\":" << node.required_weapon
        << ",\"spoken\":" << node.spoken << ",\"encounter\":" << node.encounter
        << ",\"dialogue\":" << node.dialogue
        << ",\"talk_radius\":" << node.talk_radius << ",\"talk_lower\":" << node.talk_lower << ",\"talk_upper\":" << node.talk_upper
        << ",\"raw_state\":" << node.raw_state
        << ",\"raw_condition\":" << node.raw_condition
        << ",\"door_id\":" << node.door_id
        << ",\"required_item\":" << node.required_item
        << ",\"target_health\":" << node.target_health
        << ",\"target_max_health\":" << node.target_max_health
        << ",\"reset_ticks\":" << node.reset_ticks
        << ",\"linked_actor\":" << node.linked_actor
        << ",\"traversal\":\"unknown\",\"npc_catalog_known\":"
        << (node.npc_catalog_known ? "true" : "false") << ",\"offers\":[";
    bool first_offer = true;
    for (const auto &offer : node.offers) {
      if (!first_offer)
        out << ',';
      first_offer = false;
      out << "{\"id\":\"" << offer.id << "\",\"kind\":\"" << offer.kind
          << "\",\"reward\":\"" << offer.reward << "\",\"status\":\""
          << offer.status << "\",\"scope\":\"" << offer.scope
          << "\",\"action\":" << offer.action << ",\"item\":" << offer.item
          << ",\"weapon\":" << offer.weapon << ",\"flag\":" << offer.flag
          << ",\"destination\":" << offer.destination
          << ",\"cost\":" << offer.cost << ",\"consumed_items\":[";
      for (std::size_t i = 0; i < offer.consumed_items.size(); ++i) {
        if (i)
          out << ',';
        out << offer.consumed_items[i];
      }
      out << "],\"conditions\":[";
      for (std::size_t i = 0; i < offer.conditions.size(); ++i) {
        if (i)
          out << ',';
        const auto &c = offer.conditions[i];
        out << "{\"domain\":\"" << c.domain << "\",\"id\":" << c.id
            << ",\"description\":\"" << c.description << "\",\"state\":\""
            << fact_name(c.state) << "\"}";
      }
      out << "]}";
    }
    out << "]}";
  }
  out << "]}";
}

inline Mesh decode_mesh(const Memory &m, std::uint32_t track) {
  m.require(track, 0x20U);
  const auto blocks = m.u32(track + 4U);
  const auto count = m.u16(track + 0x1AU);
  if (count == 0U || count > 512U)
    throw std::runtime_error("invalid mod mesh block count");
  m.require(blocks, static_cast<std::uint64_t>(count) * 0x48U);
  Mesh result;
  result.blocks = count;
  for (std::uint32_t b = 0; b < count; ++b) {
    const auto block = blocks + b * 0x48U;
    const auto vertices = m.u32(block), faces = m.u32(block + 4U),
               batches = m.u32(block + 0xCU);
    const auto planes = m.u32(block + 0x1CU),
               plane_indices = m.u32(block + 0x18U);
    const auto nv = m.u16(block + 0x24U), nf = m.u16(block + 0x26U),
               nb = m.u16(block + 0x28U);
    if (nv > 16384U || nf > 16384U || nb > 4096U ||
        result.vertices.size() + nv > 131072U ||
        result.triangles.size() + nf > 262144U)
      throw std::runtime_error("mod mesh too large");
    if (nv == 0U || nf == 0U)
      continue;
    m.require(vertices, static_cast<std::uint64_t>(nv) * 10U);
    m.require(faces, static_cast<std::uint64_t>(nf) * 16U);
    m.require(batches, static_cast<std::uint64_t>(nb + 1U) * 16U);
    m.require(plane_indices, static_cast<std::uint64_t>(nf) * 8U);
    const auto base = static_cast<std::uint32_t>(result.vertices.size());
    for (std::uint32_t i = 0; i < nv; ++i) {
      const auto v = vertices + i * 10U;
      result.vertices.push_back({static_cast<float>(m.s16(v)),
                                 static_cast<float>(m.s16(v + 2)),
                                 static_cast<float>(m.s16(v + 4))});
    }
    for (std::uint32_t batch = 0; batch < nb; ++batch) {
      const auto at = batches + batch * 16U;
      const auto vertex_base = m.u16(at + 6U), first = m.u16(at + 8U),
                 end = m.u16(at + 24U);
      if (first > end || end > nf)
        throw std::runtime_error("invalid mod mesh batch");
      for (std::uint32_t i = first; i < end; ++i) {
        Triangle tri;
        tri.flags = m.u32(at + 12U);
        tri.block = b;
        for (std::uint32_t j = 0; j < 3U; ++j) {
          const auto index = static_cast<std::uint32_t>(vertex_base) +
                             m.u8(faces + i * 16U + 1U + j);
          if (index >= nv)
            throw std::runtime_error("invalid mod triangle index");
          tri.vertices[j] = base + index;
        }
        const auto plane_offset =
            static_cast<std::uint32_t>(m.u16(plane_indices + i * 8U)) * 16U;
        m.require(planes, static_cast<std::uint64_t>(plane_offset) + 16U);
        const auto plane = planes + plane_offset;
        m.require(plane, 16U);
        tri.normal = {m.f32(plane), m.f32(plane + 4U), m.f32(plane + 8U)};
        result.triangles.push_back(tri);
      }
    }
  }
  return result;
}

inline void write_mesh(std::ostream &out, const Mesh &mesh, std::uint32_t level,
                       std::uint64_t generation) {
  out << "{\"schema\":1,\"level\":" << level << ",\"generation\":" << generation
      << ",\"blocks\":" << mesh.blocks << ",\"vertices\":[";
  bool first = true;
  for (const auto &v : mesh.vertices) {
    if (!first)
      out << ',';
    first = false;
    json_vec(out, v);
  }
  out << "],\"triangles\":[";
  first = true;
  for (const auto &t : mesh.triangles) {
    if (!first)
      out << ',';
    first = false;
    out << "{\"v\":[" << t.vertices[0] << ',' << t.vertices[1] << ','
        << t.vertices[2] << "],\"flags\":" << t.flags
        << ",\"block\":" << t.block << ",\"normal\":";
    json_vec(out, t.normal);
    out << '}';
  }
  out << "]}\n";
}

// Read-only steering observation for the supported US camera layout. The
// control camera pointer, rather than the renderer's temporary selection, is
// pinned by camGetListPtr and controlPlayer's store at 0x80032ACC.
struct ControlCamera { bool known{}; std::int16_t yaw{}; Vec3 position{}; };
inline ControlCamera control_camera(const Memory &m) {
  if (!m.valid(0x800F6DC0U,4) || m.u32(0x80041E0CU)!=0x3C028010U ||
      m.u32(0x80041E10U)!=0x03E00008U || m.u32(0x80041E14U)!=0x2442A4D0U ||
      m.u32(0x80032ACCU)!=0xAC226DC0U) return {};
  auto address=m.u32(0x800F6DC0U);
  if(address<0x800FA4D0U || address>=0x800FA600U ||
     (address-0x800FA4D0U)%0x4CU!=0 || !m.valid(address,0x4CU)) return {};
  try {return {true,m.s16(address),position(m,address)};}
  catch(const std::runtime_error &) {return {};}
}

// Static-camera overrides are a separate path from animcamera and cutcamera.
inline bool static_camera_active(const Memory &m) {
  if (!m.valid(0x800FB084U,4) || m.u32(0x8004271CU)!=0x3C028010U ||
      m.u32(0x80042720U)!=0x8C42B084U || m.u32(0x80042724U)!=0x03E00008U ||
      m.u32(0x80042728U)!=0U) return false;
  auto pointer=m.u32(0x800FB084U);
  return (pointer&3U)==0U && m.valid(pointer,0x18U);
}
inline bool scripted_camera_active(const Memory &m, std::uint32_t player=0) {
  if(m.u32(0x801045B8U)!=0U || static_camera_active(m)) return true;
  // controlPlayer's camera dispatcher reads +0x5C0 before the static-camera
  // fallback. The observed door shot binds a live cutcamera actor (behavior 100).
  if(!m.valid(player,0x6CU) || m.u32(0x8002B484U)!=0x8E0505C0U ||
     m.u32(0x8002B48CU)!=0x10A0000EU) return false;
  auto control=m.u32(player+0x68U);
  if(!m.valid(control,0x5C4U)) return false;
  auto camera=m.u32(control+0x5C0U);
  return (camera&3U)==0U && m.valid(camera,0x4AU) && m.u16(camera+0x48U)==100U;
}

// Normal single-player gameplay, without scripted camera or input control.
inline bool gameplay_active(const Memory &m, std::uint32_t player) {
  if (!m.valid(player, 0xA4U))
    return false;
  const auto control = m.u32(player + 0x68U);
  if (!m.valid(control, 0x5C4U) || m.u32(control + 0x5C0U) != 0U)
    return false;
  return m.u8(0x800A4FC4U) == 0U && m.u32(0x800FD7C4U) == 0U &&
         m.u32(0x801045B8U) == 0U && m.u32(0x800F6DBCU) == 0U;
}

// PC view ownership must honor the character's own input locks before its
// update publishes 0x800F6DBC. Pickups/dialogue and scripted actor control can
// leave the general gameplay/camera flags clear at the outer player boundary.
inline bool pc_gameplay_active(const Memory &m, std::uint32_t player) {
  if(!gameplay_active(m,player) || scripted_camera_active(m,player))return false;
  const auto control=m.u32(player+0x68U);
  if(!m.valid(control,0x5F0U) || m.u32(control+0xFCU)!=0U)return false;
  // Shared by Vela, Juno and Lupus before their native read-input call.
  for(auto offset:{0x56BU,0x577U,0x1F9U,0x1FAU,0x186U,0x198U,0x5EEU,0x10DU,0x19FU})
    if(m.u8(control+offset)!=0U)return false;
  return true;
}

// Boss combat keeps a fixed camera and uses its own player update. +0xFC is
// the boss-control actor selected by controlPlayer, not a general input lock.
// Keep ordinary camera/locomotion ownership separate from playable actions.
inline bool pc_boss_gameplay_active(const Memory &m, std::uint32_t player) {
  if(!gameplay_active(m,player) || scripted_camera_active(m,player))return false;
  const auto control=m.u32(player+0x68U);
  if(!m.valid(control,0x5F0U) || (m.u8(control+1U)&3U)>=3U)return false;
  const auto boss=m.u32(control+0xFCU);
  if((boss&3U)!=0U || !m.valid(boss,0x40U))return false;
  // Pin the resident dispatcher, including its argument loads and delay
  // slots. Overlay load/unload relocates the JAL destinations, so their raw
  // ROM targets are not stable live-memory signatures.
  if(m.u32(0x80032FD8U)!=0x8E0D00FCU || m.u32(0x80032FE0U)!=0x11A0001EU)
    return false;
  for(auto call:{0x8003301CU,0x80033034U,0x8003304CU}) {
    if(m.u32(call-8U)!=0x8FA40090U || m.u32(call-4U)!=0x8FA50094U ||
       (m.u32(call)&0xFC000000U)!=0x0C000000U || m.u32(call+4U)!=0U)
      return false;
  }
  const auto movement=m.u8(control+0x568U);
  // 0x20 is active combat, 0x23 is its jump; 0x2A is the boss death handler.
  if(movement!=0x20U && movement!=0x23U)return false;
  for(auto offset:{0x56BU,0x577U,0x1F9U,0x1FAU,0x186U,0x198U,0x5EEU,0x10DU,0x19FU,0x18EU})
    if(m.u8(control+offset)!=0U)return false;
  return true;
}
inline bool pc_actions_active(const Memory &m, std::uint32_t player) {
  return pc_gameplay_active(m,player) || pc_boss_gameplay_active(m,player);
}

inline bool transition_confirmation(const Memory &m) {
  // Supported US ROM: mainChangeLevel sets this prompt flag and pause mode;
  // its update routine consumes A/Start before beginning the level load.
  return m.u8(0x800A329CU) == 1U && m.u8(0x800FD7BDU) == 1U;
}

class NavigationMod {
public:
  NpcRewardCatalog npc_rewards;
  NavigationPilot pilot;
  NavigationConfirmation confirmation;
  DialogueInput dialogue_input;
  DialogueState dialogue;
  bool enabled = false;
  bool telemetry_enabled = false;
  bool observing() const noexcept { return enabled || telemetry_enabled; }
  // Called only at the known player-update boundary. Never writes guest memory.
  bool observe_player(const Memory &m, std::uint32_t actor) noexcept {
    if (!observing()) return false;
    try {
      if (!gameplay_active(m, actor) || !m.valid(actor, 0xA4U)) return false;
      const auto control = m.u32(actor + 0x68U), properties = m.u32(actor + 0x4CU);
      if (!m.valid(control, 0x5CAU) || !m.valid(properties, 8U) ||
          (m.u8(control + 1U) & 3U) > 2U) return false;
      player = actor;
      return true;
    } catch (const std::runtime_error &) { return false; }
  }
  // Explicit map settings override the legacy all-in-one debug defaults.
  bool mods_configured = false, warp_exits = false, infinite_health = false,
       instant_kill = false;
  std::uint64_t warp_nonce = 0;
  bool health_enabled() const noexcept { return mods_configured ? infinite_health : enabled; }
  bool kill_enabled() const noexcept { return mods_configured ? instant_kill : enabled; }
  bool configure_mods(std::istream &in) {
    std::string magic, extra; int warp{}, health{}, kill{};
    if (!(in >> magic >> warp >> health >> kill) || magic != "JFGMODS1" ||
        (warp != 0 && warp != 1) || (health != 0 && health != 1) ||
        (kill != 0 && kill != 1) || (in >> extra)) return false;
    warp_exits = warp != 0; infinite_health = health != 0; instant_kill = kill != 0;
    mods_configured = true;
    return true;
  }
  bool warp_to_exit(std::istream &in, Memory &m, const std::vector<Actor> &list,
                    std::int64_t now) {
    std::string magic, extra;
    std::uint32_t level{}, address{}; std::uint64_t gen{}, nonce{};
    std::int64_t stamp{};
    if (!(in >> magic >> level >> gen >> nonce >> stamp >> address) ||
        magic != "JFGWARP1" || (in >> extra) || nonce <= warp_nonce) return false;
    // Consume even rejected requests: an old double-click must never fire later.
    warp_nonce = nonce;
    if (!observing() || !warp_exits || stamp < 0 || stamp > now || now-stamp > 5000 ||
        gen != generation || player == 0U) return false;
    try {
      if (level != m.u32(0x800FB114U) || !gameplay_active(m, player) ||
          scripted_camera_active(m, player)) return false;
      // Queue the same contact as exitControl, then let controlPlayer perform
      // the normal transition (spawn selection, region prompts and game flags).
      // An exit actor's origin lies ON its directional plane; position alone
      // does not satisfy the guest's strict negative-side contact test.
      const auto control=m.u32(player+0x68U), properties=m.u32(player+0x4CU);
      if(m.u8(0x800FD7BDU)!=0U || !m.valid(control,0x198U) ||
         !m.valid(properties,8U) || m.s16(properties+6U)<0x100 ||
         m.u32(control+0xF4U)!=0U ||
         m.u32(0x800257F4U)!=0xACF000F4U ||
         m.u32(0x800257F8U)!=0xA0EC0196U ||
         m.u32(0x80032B94U)!=0x8E0200F4U ||
         m.u32(0x80032DACU)!=0xAE0000F4U)return false;
      bool present = false;
      for (const auto &actor : list) if (actor.address == player) present = true;
      if (!present) return false;
      for (const auto &exit : exits(m, list)) {
        if (exit.address != address) continue;
        if(exit_condition(m,exit,npc_facts(m,inventory(m,player),player),player)!=Fact::met)
          return false;
        const auto p = exit.position;
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
            std::abs(p.x)>1000000 || std::abs(p.y)>1000000 || std::abs(p.z)>1000000)
          return false;
        m.require(player, 0x18U);
        pilot.stop("exit_warp");
        const std::array<float,3> xyz{p.x,p.y,p.z};
        for (unsigned i=0; i<3; ++i) {
          const auto bits=std::bit_cast<std::uint32_t>(xyz[i]);
          m.put16(player+12U+i*4U, static_cast<std::uint16_t>(bits>>16U));
          m.put16(player+14U+i*4U, static_cast<std::uint16_t>(bits));
        }
        m.put16(control+0xF4U,static_cast<std::uint16_t>(address>>16U));
        m.put16(control+0xF6U,static_cast<std::uint16_t>(address));
        // The original writes a single signed byte (-120); preserve +0x197.
        m.put16(control+0x196U,static_cast<std::uint16_t>(0x8800U|m.u8(control+0x197U)));
        return true;
      }
    } catch (const std::runtime_error &) { ++invalid; }
    return false;
  }
  bool juno_double_run = false;
  std::uint64_t juno_run_boosts = 0;
  std::uint32_t player = 0, last_track = 0, last_level = UINT32_MAX;
  std::uint64_t updates = 0, generation = 1, cleared = 0, health_restores = 0,
                invalid = 0, manual_inputs = 0;
  std::uint32_t first_clear_level = UINT32_MAX;
  std::uint64_t first_clear_update = 0;
  void transition() noexcept {
    pilot.reset();
    player = 0;
    last_track = 0;
    last_level = UINT32_MAX;
    ++generation;
  }

  // US Juno ground control (0x01002708), a2: stick-derived target speed.
  // Scale the freshly computed target, never accumulated velocity/position.
  // Original acceleration, wall/floor collision and vertical physics still run.
  // This is opt-in separately from navigation, and excludes other movement states.
  std::uint32_t juno_run_target(const Memory &m, std::uint32_t actor,
                                std::uint32_t control, std::uint32_t bits) noexcept {
    if (!enabled || !juno_double_run) return bits;
    try {
      if ((actor & 3U) || (control & 3U) || !gameplay_active(m, actor) ||
          static_camera_active(m) || m.u32(actor + 0x68U) != control ||
          (m.u8(control + 1U) & 3U) != 1U || m.u8(control + 0x568U) != 0U)
        return bits;
      const float speed = std::bit_cast<float>(bits);
      // The original input is a nonnegative stick magnitude times run scale.
      // Reject invalid data and leave zero input completely untouched.
      if (!std::isfinite(speed) || speed <= 0.0F || speed > 25.0F) return bits;
      ++juno_run_boosts;
      return std::bit_cast<std::uint32_t>(speed * 2.0F);
    } catch (const std::runtime_error &) {
      return bits;
    }
  }
  bool full_health(Memory &m, std::uint32_t actor) noexcept {
    if (!observing() || !health_enabled())
      return false;
    try {
      if (!gameplay_active(m, actor) || !m.valid(actor, 0xA4U))
        return false;
      const auto control = m.u32(actor + 0x68U),
                 properties = m.u32(actor + 0x4CU);
      m.require(control, 0x5CAU);
      m.require(properties, 8U);
      const auto character =
          static_cast<std::uint32_t>(m.u8(control + 1U) & 3U);
      const auto game = m.u32(0x800FD7D4U);
      m.require(game, 0x334U);
      const auto saved = game + 0x15CU + character * 0x76U;
      const auto maximum = health_capacity(m.s16(saved + 2U));
      if (maximum == 0U)
        return false;
      if (m.u16(properties + 6U) != maximum)
        ++health_restores;
      m.put16(properties + 6U, maximum);
      m.put16(saved, maximum);
      // Keep the game's invulnerability timer alive without creating a shield
      // object.
      m.put16(control + 0x5C8U, 60U);
      player = actor;
      return true;
    } catch (const std::runtime_error &) {
      ++invalid;
      return false;
    }
  }
  bool clear_enemy(Memory &m, std::uint32_t actor) noexcept {
    if (!observing() || !kill_enabled() || player == 0U || actor == player)
      return false;
    try {
      if (!gameplay_active(m, player) || !m.valid(actor, 0xA4U))
        return false;
      // Only the normal squad-member behavior; animated cutscene actors differ.
      if (m.u16(actor + 0x48U) != 24U)
        return false;
      const auto control = m.u32(actor + 0x68U),
                 properties = m.u32(actor + 0x4CU);
      if (!m.valid(control, 0xA8U) || !m.valid(properties, 12U))
        return false;
      const auto squad = m.u32(control + 0x24U);
      if (!m.valid(squad, 0xA4U) || m.u16(squad + 0x48U) != 23U)
        return false;
      const auto data = m.u32(squad + 0x68U);
      if (!m.valid(data, 0x34U))
        return false;
      const auto type = m.u16(data + 0x2CU);
      if (tribal(type) || m.s16(properties + 6U) <= 0)
        return false;
      // Leave the object in the game so its original death logic updates squad
      // gates.
      m.put16(properties + 6U, 0U);
      if (cleared == 0U) {
        first_clear_level = m.u32(0x800FB114U);
        first_clear_update = updates;
      }
      ++cleared;
      return true;
    } catch (const std::runtime_error &) {
      ++invalid;
      return false;
    }
  }
  void write_state(std::ostream &out, const Memory &m,
                   const std::vector<Actor> &list, std::uint32_t level,
                   bool mesh_ready) const {
    out << "{\"inventory_tracker\":";write_inventory_tracker(out,m,player);
    out << ",\"dialogue\":";write_dialogue(out,dialogue);
    out << ",\"schema\":1,\"level\":" << level
        << ",\"generation\":" << generation << ",\"timestamp_ms\":"
        << std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
               .count()
        << ",\"update\":" << updates
        << ",\"transition_confirm\":" << (transition_confirmation(m) ? "true" : "false")
        << ",\"mesh_ready\":" << (mesh_ready ? "true" : "false")
        << ",\"scripted_camera\":" << (scripted_camera_active(m,player) ? "true" : "false")
        << ",\"mods_available\":true"
        << ",\"mod_warp_exits\":" << (warp_exits ? "true" : "false")
        << ",\"mod_infinite_health\":" << (health_enabled() ? "true" : "false")
        << ",\"mod_instant_kill\":" << (kill_enabled() ? "true" : "false")
        << ",\"navigation_enabled\":" << (enabled ? "true" : "false")
        << ",\"gameplay_active\":" << (player != 0U && gameplay_active(m, player) ? "true" : "false")
        << ",\"clearing_active\":"
        << (enabled && player != 0U && gameplay_active(m, player) ? "true" : "false")
        << ",\"first_clear_level\":" << first_clear_level
        << ",\"first_clear_update\":" << first_clear_update
        << ",\"enemies_cleared\":" << cleared
        << ",\"health_restores\":" << health_restores
        << ",\"invalid_reads\":" << invalid << ",\"player\":";
    bool found = false;
    for (const auto &a : list)
      if (a.address == player) {
        out << "{\"address\":" << player << ",\"position\":";
        json_vec(out, a.position);
        out << ",\"yaw\":" << m.s16(player)
            << ",\"health\":" << m.s16(m.u32(player + 0x4CU) + 6U) << ",\"motion\":";
        write_player_motion(out,player_motion(m,player));out<<'}';
        found = true;
        break;
      }
    if (!found)
      out << "null";
    const auto camera=control_camera(m);
    const auto steering=pilot.observed_input();
    out << ",\"steering\":{\"x\":" << steering.x << ",\"y\":" << steering.y
        << ",\"camera_known\":" << (camera.known?"true":"false")
        << ",\"camera_yaw\":" << camera.yaw << ",\"camera_position\":";
    json_vec(out,camera.position);
    out << ",\"static_camera\":" << (static_camera_active(m)?"true":"false")
        << ",\"game_mode\":" << m.u32(0x800FD7C4U)
        << ",\"animation_camera\":" << m.u32(0x801045B8U)
        << ",\"joy_disabled\":" << m.u32(0x800F6DBCU);
    const auto control=m.valid(player,0x6CU)?m.u32(player+0x68U):0U;
    out << ",\"player_script\":" << (m.valid(control,0x5C4U)?m.u32(control+0x5C0U):0U) << '}';
    out << ",\"exits\":[";
    bool first = true;
    for (const auto &e : exits(m, list)) {
      if (!first)
        out << ',';
      first = false;
      out << "{\"address\":" << e.address << ",\"position\":";
      json_vec(out, e.position);
      out << ",\"normal\":";
      json_vec(out, e.normal);
      out << ",\"plane_d\":" << e.plane_d << ",\"radius\":" << e.radius
          << ",\"destination_code\":" << e.destination
          << ",\"condition_code\":" << e.condition
          << ",\"directional\":" << e.directional << '}';
    }

    out << "],\"markers\":[";
    first = true;
    for (const auto &item : item_markers(m, list)) {
      if (!first)
        out << ',';
      first = false;
      out << "{\"address\":" << item.address << ",\"position\":";
      json_vec(out, item.position);
      // Labels contain only host-owned ASCII literals and numeric IDs.
      out << ",\"kind\":\"" << item.kind << "\",\"label\":\"" << item.label
          << "\",\"content_code\":" << item.content_code
          << ",\"opened\":" << (item.opened ? "true" : "false") << '}';
    }
    out << "],\"npcs\":";
    write_npcs(out, npc_markers(m, list));
    out << ",\"progression\":";
    write_progression(out, m, list, player, level, &npc_rewards);
    out << ",\"navigation_ai\":{\"state\":\"" << pilot.state << "\",\"active\":" << (pilot.active() ? "true" : "false")
        << ",\"confirmations\":" << confirmation.count << ",\"manual_inputs\":" << manual_inputs << ",\"nonce\":" << pilot.nonce() << ",\"waypoint\":" << pilot.waypoint << ",\"count\":" << pilot.count() << ",\"jump_attempts\":" << pilot.jump_attempts << "}";
    out << ",\"box_jump\":";pilot.write_box_jump(out);
    out << ",\"collision\":";
    write_collision(out, collision_models(m, list));
    out << ",\"actors\":[";
    first = true;
    for (const auto &a : list) {
      if (!first)
        out << ',';
      first = false;
      out << "{\"address\":" << a.address << ",\"name\":\"" << actor_name(m, a.address)
          << "\",\"behavior\":" << a.behavior;
      write_enemy_state(out, enemy_state(m, a));
      out << ",\"position\":";
      json_vec(out, a.position);
      out << '}';
    }
    out << "]}\n";
  }
};
} // namespace jfg::mod
