#pragma once
#include "jfg/renderer/rigid_camera.hpp"
#include <array>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace jfg {
// Experimental presentation only. Zero preserves the original renderer.
inline std::optional<unsigned> parse_render_fps(std::string_view value) noexcept {
    if (value.empty() || value == "0") return 0U;
    unsigned rate = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), rate);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
        rate < 60U || rate > 1000U) return std::nullopt;
    return rate;
}
using PresentationMatrix = std::array<float, 16>;
struct PresentationMesh {
    // Exact vertex positions and local triangle indices, not a guest address.
    std::vector<std::uint32_t> key;
    std::vector<std::array<float, 3>> positions;
    PresentationMatrix matrix{};
    bool eligible = true;
    // A cross-slot triangle has no complete per-slot topology key. Such a
    // mesh may follow a proven shared transform, but cannot anchor history.
    bool complete_local_topology = true;
    std::optional<RigidPresentationPose> rigid_pose;
    std::optional<std::array<std::uint64_t,2>> camera_owner;
    std::optional<RigidPresentationPose> camera_pose;
};
// F3DDKR may reload one numeric transform into several RT64 matrix slots.
// Equal finite transforms still define one triangle space. Keep incomplete
// local topology out of shape matching; another complete mesh must anchor the
// shared cohort. Actually different/invalid transforms retain endpoint safety.
inline bool allow_shared_matrix_triangle(PresentationMesh& a,
    PresentationMesh& b, PresentationMesh& c) {
    bool equal = a.matrix == b.matrix && a.matrix == c.matrix;
    for(float value:a.matrix) equal = equal && std::isfinite(value);
    if(!equal) {
        a.eligible=b.eligible=c.eligible=false;
        return false;
    }
    a.complete_local_topology=b.complete_local_topology=c.complete_local_topology=false;
    return true;
}
// Known sprite lifetimes own the transform even when animation changes the
// local quad dimensions. Retain vertex count and triangle connectivity; unknown
// meshes still require exact geometry. Texture/local-quad animation itself
// keeps its authored cadence, while the world anchor follows the shared phase.
inline void bind_presentation_billboard(PresentationMesh &mesh,
    const std::array<std::uint64_t, 4> &owner) {
    const auto coordinate_words = mesh.positions.size() * 3U;
    if (owner[0] == 0U || owner[3] == 0U ||
        mesh.key.size() < coordinate_words + 2U ||
        mesh.key[coordinate_words] != 0xFFFFFFFFU)
        return;
    mesh.key.erase(mesh.key.begin(), mesh.key.begin() +
        static_cast<std::vector<std::uint32_t>::difference_type>(coordinate_words));
    mesh.key.insert(mesh.key.begin(), 0x42494C4CU);
    for (auto value : owner) {
        mesh.key.push_back(static_cast<std::uint32_t>(value));
        mesh.key.push_back(static_cast<std::uint32_t>(value >> 32U));
    }
}
// Keep exact rigid geometry, but disambiguate identical assets by actor,
// model instance and camera. Unknown objects retain conservative shape matching.
inline void bind_presentation_rigid(PresentationMesh &mesh,
    const std::array<std::uint64_t, 4> &owner) {
    if (!owner[0] || !owner[3] || mesh.key.empty()) return;
    mesh.key.push_back(0x52494744U);
    for (auto value : owner) {
        mesh.key.push_back(static_cast<std::uint32_t>(value));
        mesh.key.push_back(static_cast<std::uint32_t>(value >> 32U));
    }
}
struct PresentationTransform {
    PresentationMatrix previous{};
    bool matched = false;
    // Semantic skin vertices already carry world-space endpoint motion.
    // Keep the current joint transform and interpolate the shared camera.
    bool semantic_vertices = false;
    PresentationMatrix current_world{}, previous_camera{}, current_camera{};
};
struct PresentationHistoryFrame {
    std::vector<PresentationTransform> transforms;
    std::uint64_t tick = 0;
    unsigned source_rate = 0;
    bool scene = false;
    // Populated only by the explicit private matching observer.
    std::vector<unsigned> debug_reasons;
    std::vector<PresentationMatrix> debug_previous;
    std::array<std::uint64_t,6> debug_cameras{};
    float debug_projection_delta=0.0f;
    std::vector<std::array<RigidPresentationPose,2>> debug_camera_rejections;
};
template<class Previous>
inline bool continuous_presentation_motion(const Previous& old,
                                          const PresentationMesh& current) noexcept {
    for (float value : old.matrix) if (!std::isfinite(value)) return false;
    for (float value : current.matrix) if (!std::isfinite(value)) return false;
    for (const auto& point : current.positions)
        for (float value : point) if (!std::isfinite(value)) return false;
    if (old.rigid_pose && current.rigid_pose &&
        stationary_rigid_camera_motion(*old.rigid_pose, *current.rigid_pose))
        return true;
    bool visible = false;
    for (const auto& point : current.positions) {
        const auto project = [&](const PresentationMatrix& matrix) {
            std::array<float, 4> clip{};
            for (std::size_t c = 0; c < 4; ++c)
                clip[c] = point[0]*matrix[c] + point[1]*matrix[4+c] +
                    point[2]*matrix[8+c] + matrix[12+c];
            return clip;
        };
        const auto a = project(old.matrix), b = project(current.matrix);
        if (!std::isfinite(a[3]) || !std::isfinite(b[3])) return false;
        if (a[3] <= 0.001f && b[3] <= 0.001f) continue;
        if (a[3] <= 0.001f || b[3] <= 0.001f) return false;
        visible = true;
        if (b[3]/a[3] < 0.5f || b[3]/a[3] > 2.0f) return false;
        for (std::size_t c = 0; c < 2; ++c) {
            const float movement = b[c]/b[3] - a[c]/a[3];
            if (!std::isfinite(movement) || std::abs(movement) > 0.35f) return false;
        }
    }
    return visible;
}
// One previous scene; no references into guest or RT64 ring-buffer storage.
// Unique shapes anchor shared transforms. Ambiguous groups, invalid cadence
// and jumps use endpoints.
// This conservative prototype does not replace semantic actor identities.
class PresentationHistory {
    struct HistoryEndpoint {
        PresentationMatrix matrix;
        std::optional<RigidPresentationPose> rigid_pose;
    };
    struct ShapeEntry {
        std::vector<std::uint32_t> key;
        HistoryEndpoint endpoint;
        std::uint64_t hash=0;
        std::size_t count=0;
    };
    // Retain two contiguous tables instead of allocating ordered-map nodes for
    // every shape in every scene. A hash only chooses a probe sequence: complete
    // keys still establish identity, and duplicate keys cannot anchor history.
    // Rebuild all slot indices before reusing either table's storage.
    struct Shapes {
        static constexpr std::size_t empty=std::size_t(-1);
        std::vector<ShapeEntry> entries;
        std::vector<std::size_t> slots;
        static std::uint64_t hash_key(const std::vector<std::uint32_t>& key) {
            std::uint64_t value=0xcbf29ce484222325ULL;
            for(auto word:key){value^=word;value*=0x100000001b3ULL;}
            return value;
        }
        void clear(std::size_t maximum) {
            if(maximum>empty/4U)throw std::length_error("shape history capacity");
            std::size_t capacity=16U;while(capacity<maximum*2U)capacity*=2U;
            if(capacity>slots.size())slots.resize(capacity);
            std::fill(slots.begin(),slots.end(),empty);entries.clear();entries.reserve(maximum);
        }
        std::size_t insert(const PresentationMesh& mesh) {
            const auto hash=hash_key(mesh.key);auto slot=hash&(slots.size()-1U);
            while(slots[slot]!=empty){auto& entry=entries[slots[slot]];
                if(entry.hash==hash && entry.key==mesh.key){++entry.count;return slots[slot];}
                slot=(slot+1U)&(slots.size()-1U);
            }
            const auto index=entries.size();slots[slot]=index;
            entries.push_back({mesh.key,{mesh.matrix,mesh.rigid_pose},hash,1U});return index;
        }
        const HistoryEndpoint* find(const ShapeEntry& key) const {
            if(slots.empty())return nullptr;
            auto slot=key.hash&(slots.size()-1U);
            while(slots[slot]!=empty){const auto& entry=entries[slots[slot]];
                if(entry.hash==key.hash && entry.key==key.key)return entry.count==1U?&entry.endpoint:nullptr;
                slot=(slot+1U)&(slots.size()-1U);
            }
            return nullptr;
        }
    };
public:
    bool has_continuous_cadence(std::uint64_t tick, bool scene) const {
        const auto delta = tick > previous_tick_ ? tick - previous_tick_ : 0U;
        return scene && previous_tick_ != 0U && delta >= 1U && delta <= 6U && 60U % delta == 0U;
    }
    PresentationHistoryFrame record(const std::vector<PresentationMesh>& meshes,
                                    std::uint64_t tick, bool scene, bool diagnostic = false) {
        PresentationHistoryFrame result;
        result.scene = scene;
        result.tick = tick;
        result.transforms.resize(meshes.size());
        if (diagnostic) {
            result.debug_reasons.resize(meshes.size());
            result.debug_previous.resize(meshes.size());
        }
        if (!scene) return result;
        const auto delta = tick > previous_tick_ ? tick - previous_tick_ : 0U;
        // Real scene intervals vary when an update misses a retrace. Use
        // the actual bounded interval; cadence changes alone are not cuts.
        // Shape/lifetime, pose, finite-coordinate and motion checks below
        // still reject discontinuities. Long gaps and repeated ticks hold.
        if (has_continuous_cadence(tick, scene))
            result.source_rate = static_cast<unsigned>(60U / delta);
        next_.clear(meshes.size());
        std::vector<std::size_t> indices(meshes.size(),Shapes::empty);
        for(std::size_t i=0;i<meshes.size();++i) {
            const auto& mesh=meshes[i];
            if(mesh.eligible && mesh.complete_local_topology && !mesh.key.empty())
                indices[i]=next_.insert(mesh);
        }
        for(std::size_t i=0;i<meshes.size();++i) {
            const auto& mesh=meshes[i];
            const auto index=indices[i];
            const bool duplicate=index!=Shapes::empty && next_.entries[index].count!=1U;
            if(index==Shapes::empty || duplicate) {
                if(diagnostic)result.debug_reasons[i]=(!mesh.eligible?1U:0U)|(mesh.key.empty()?2U:0U)|
                    (!mesh.complete_local_topology?256U:0U)|(duplicate?4U:0U);
                continue;
            }
            const auto* old=previous_.find(next_.entries[index]);
            if(diagnostic) {
                if(!old)result.debug_reasons[i]|=8U;
                else result.debug_previous[i]=old->matrix;
                if(result.source_rate==0U)result.debug_reasons[i]|=16U;
                else if(old && !continuous_presentation_motion(*old,mesh))result.debug_reasons[i]|=32U;
            }
            if(result.source_rate && old && continuous_presentation_motion(*old,mesh))
                result.transforms[i]={old->matrix,true};
        }
        // Scenery visibility changes can replace every exact geometry key.
        // A byte-bound camera lifetime provides its own endpoint, independent
        // of those batches. Validate the shared operands once per owner; all
        // claims must agree, and actual cuts/projection changes still hold.
        using CameraKey=std::array<std::uint64_t,2>;
        std::map<CameraKey,std::optional<HistoryEndpoint>> cameras;
        const PresentationMatrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        for(const auto& mesh:meshes) if(mesh.camera_owner) {
            bool valid=mesh.eligible && mesh.camera_pose &&
                mesh.camera_pose->world==identity && (*mesh.camera_owner)[0] && (*mesh.camera_owner)[1];
            for(const auto& point:mesh.positions)for(float value:point)
                valid=valid && std::isfinite(value);
            auto [found,fresh]=cameras.emplace(*mesh.camera_owner,std::nullopt);
            if(fresh) {
                if(valid && rigid_pose_matches_matrix(*mesh.camera_pose,mesh.matrix))
                    found->second=HistoryEndpoint{mesh.matrix,mesh.camera_pose};
            } else if(!valid || !found->second || found->second->matrix!=mesh.matrix ||
                      found->second->rigid_pose!=mesh.camera_pose) found->second.reset();
        }
        std::map<CameraKey,PresentationMatrix> camera_anchors;
        for(const auto& [key,current]:cameras) {
            if(diagnostic)++result.debug_cameras[0];
            if(!current)continue;
            if(diagnostic)++result.debug_cameras[1];
            const auto prior=previous_cameras_.find(key);
            if(prior==previous_cameras_.end()) {
                if(diagnostic)++result.debug_cameras[5];
                continue;
            }
            const auto& old_pose=*prior->second.rigid_pose;
            const auto& new_pose=*current->rigid_pose;
            const bool motion=continuous_owned_camera_motion(old_pose,new_pose);
            if(diagnostic) {
                if(!motion)result.debug_camera_rejections.push_back({old_pose,new_pose});
                if(motion)++result.debug_cameras[2];
                else if(old_pose.projection!=new_pose.projection) {
                    ++result.debug_cameras[3];
                    for(unsigned component:{0U,5U})if(old_pose.projection[component]!=0.0f)
                        result.debug_projection_delta=(std::max)(result.debug_projection_delta,
                            std::abs(new_pose.projection[component]/old_pose.projection[component]-1.0f));
                } else ++result.debug_cameras[4];
            }
            if(result.source_rate && motion)camera_anchors.emplace(key,prior->second.matrix);
        }
        previous_cameras_.clear();
        for(const auto& entry:cameras)if(entry.second)
            previous_cameras_.emplace(entry.first,*entry.second);
        // A single F3DDKR matrix can feed many vertex batches. Matching or
        // rejecting those batches independently tears their shared edges.
        // One unambiguous shape anchors the whole current matrix; conflicting
        // history or an unsafe batch holds every member at the endpoint.
        struct Cohort {
            PresentationMatrix previous{};
            bool anchored = false;
            bool blocked = false;
        };
        std::map<PresentationMatrix, Cohort> cohorts;
        const auto finite_matrix = [](const PresentationMatrix& matrix) {
            for (float value : matrix) if (!std::isfinite(value)) return false;
            return true;
        };
        for (std::size_t i = 0; i < meshes.size(); ++i) {
            const auto& mesh = meshes[i];
            if (mesh.positions.empty() || !finite_matrix(mesh.matrix)) continue;
            auto& cohort = cohorts[mesh.matrix];
            cohort.blocked = cohort.blocked || !mesh.eligible;
            if (result.transforms[i].matched) {
                const auto& previous = result.transforms[i].previous;
                if (cohort.anchored && cohort.previous != previous)
                    cohort.blocked = true;
                cohort.previous = previous;
                cohort.anchored = true;
            }
        }
        // Supplement an unanchored cohort only. Unusable semantic metadata
        // cannot invalidate an independently proved geometry anchor. Gather
        // all camera claims first so disagreement never depends on mesh order.
        std::map<PresentationMatrix,std::optional<PresentationMatrix>> extra;
        for(const auto& mesh:meshes) {
            if(!mesh.camera_owner || mesh.positions.empty() || !finite_matrix(mesh.matrix))continue;
            const auto& cohort=cohorts.at(mesh.matrix);
            if(cohort.anchored || cohort.blocked)continue;
            const auto anchor=camera_anchors.find(*mesh.camera_owner);
            if(anchor==camera_anchors.end())continue;
            auto [found,fresh]=extra.emplace(mesh.matrix,anchor->second);
            if(!fresh && found->second && *found->second!=anchor->second)found->second.reset();
        }
        for(const auto& [matrix,anchor]:extra)if(anchor) {
            auto& cohort=cohorts.at(matrix);
            cohort.previous=*anchor;cohort.anchored=true;
        }
        for (std::size_t i = 0; i < meshes.size(); ++i) {
            const auto& mesh = meshes[i];
            if (mesh.positions.empty() || !finite_matrix(mesh.matrix)) continue;
            const auto& cohort = cohorts.at(mesh.matrix);
            result.transforms[i] = {cohort.previous, cohort.anchored && !cohort.blocked};
            if(diagnostic) result.debug_reasons[i]|=(cohort.blocked?64U:0U)|(!cohort.anchored?128U:0U);
        }
        std::swap(previous_,next_);
        previous_tick_ = tick;
        return result;
    }
private:
    Shapes previous_,next_;
    std::map<std::array<std::uint64_t,2>,HistoryEndpoint> previous_cameras_;
    std::uint64_t previous_tick_ = 0;
};
} // namespace jfg
