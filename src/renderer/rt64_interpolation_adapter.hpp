#pragma once
#include "jfg/renderer/interpolation_bridge.hpp"
#include "hle/rt64_workload.h"
namespace jfg {
// This queue slot owns copied history. Never reference an older queue slot.
inline bool apply_presentation_interpolation(RT64::Workload& workload,
        float previous_weight, float current_weight) {
    const auto snapshot = presentation_snapshot(&workload);
    if (!snapshot || snapshot->transforms.size() != workload.drawData.worldTransforms.size())
        return false;
    auto& data = workload.drawData;
    data.lerpWorldTransforms.clear();
    data.prevWorldTransforms.clear();
    data.invTWorldTransforms.clear();
    for (std::size_t i = 0; i < data.worldTransforms.size(); ++i) {
        const hlslpp::float4x4 current = data.worldTransforms[i];
        auto result = current;
        auto previous = current;
        if (snapshot->transforms[i].matched && snapshot->transforms[i].semantic_vertices) {
            const auto matrix=[](const PresentationMatrix& m) {
                return hlslpp::float4x4(m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],
                    m[8],m[9],m[10],m[11],m[12],m[13],m[14],m[15]);
            };
            const auto& pose=snapshot->transforms[i];
            const auto world=matrix(pose.current_world);
            const auto old=matrix(pose.previous_camera), camera=matrix(pose.current_camera);
            result=hlslpp::mul(world,old*(1.0f-current_weight)+camera*current_weight);
            previous=hlslpp::mul(world,old*(1.0f-previous_weight)+camera*previous_weight);
        }
        else if (snapshot->transforms[i].matched) {
            const auto& m = snapshot->transforms[i].previous;
            const hlslpp::float4x4 old(m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],
                m[8],m[9],m[10],m[11],m[12],m[13],m[14],m[15]);
            result = old*(1.0f-current_weight) + current*current_weight;
            previous = old*(1.0f-previous_weight) + current*previous_weight;
        }
        data.lerpWorldTransforms.push_back(result);
        data.prevWorldTransforms.push_back(previous);
        data.invTWorldTransforms.push_back(hlslpp::transpose(hlslpp::inverse(result)));
    }
    return true;
}
inline void trace_presentation_workload(RT64::Workload& workload,
        unsigned target, unsigned frame, unsigned count, float alpha) {
    if (!interpolation_trace_enabled()) return;
    const auto snapshot = presentation_snapshot(&workload);
    if (!snapshot) return;
    std::size_t matched = 0, moving = 0;
    for (std::size_t i = 0; i < snapshot->transforms.size(); ++i) {
        if (!snapshot->transforms[i].matched) continue;
        ++matched;
        bool changed = false;
        const auto& current = workload.drawData.worldTransforms[i];
        for (std::size_t r = 0; r < 4; ++r) {
            for (std::size_t c = 0; c < 4; ++c)
                changed = changed || current.m[r][c] != snapshot->transforms[i].previous[r*4+c];
        }
        if (changed) ++moving;
    }
    trace_interpolation("render", snapshot->tick, snapshot->source_rate,
        target, frame, count, alpha, matched, moving);
}
} // namespace jfg
