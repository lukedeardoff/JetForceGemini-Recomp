#include "jfg/renderer/interpolation_history.hpp"
#include <cstdlib>
#include <limits>
static void require(bool value) { if (!value) std::abort(); }
int main() {
    // The worker's preflight gate must equal the gate recorded by geometry,
    // including offscreen passes, repeat ticks, cadence changes and resets.
    {
        jfg::PresentationHistory history;
        for (const auto tick : {10ULL, 12ULL, 12ULL, 15ULL, 20ULL, 26ULL, 30ULL, 29ULL, 31ULL}) {
            const bool expected = history.has_continuous_cadence(tick, true);
            require(!history.has_continuous_cadence(tick, false));
            require(history.record({}, tick, false).source_rate == 0U);
            require((history.record({}, tick, true).source_rate != 0U) == expected);
        }
    }

    // A captured stationary bridge near cancellation. Guest concatenation is
    // world * (view * projection), with single-precision paired sums.
    // Reassociation in double rejects this valid signed 16.16 endpoint.
    {
        jfg::RigidPresentationPose precise{{0.0f,0.0f,0.796875f,0.0f,-0.0f,0.796875f,0.0f,0.0f,-0.796875f,-0.0f,0.0f,0.0f,1331.0f,0.0f,882.0f,1.0f},{-0.99926477670669556f,-0.0f,0.038340121507644653f,0.0f,-0.0f,1.0f,0.0f,0.0f,-0.038340121507644653f,0.0f,-0.99926477670669556f,0.0f,1368.890380859375f,-40.010002136230469f,1518.405029296875f,1.0f},{1.5415109395980835f,0.0f,0.0f,0.0f,0.0f,2.0553479194641113f,0.0f,0.0f,0.0f,0.0f,-1.0013341903686523f,-1.0f,0.0f,0.0f,-20.013341903686523f,0.0f}};
        auto endpoint=std::array<float,16>{-0.0471038818359375f,0.0f,0.7973480224609375f,0.796295166015625f,0.0f,1.637847900390625f,0.0f,0.0f,1.2274932861328125f,0.0f,0.0305938720703125f,0.030548095703125f,7.789031982421875f,-82.234466552734375f,-709.01556396484375f,-688.084228515625f};
        require(jfg::rigid_pose_matches_matrix(precise,endpoint));
        endpoint[12]+=0.01f;
        require(!jfg::rigid_pose_matches_matrix(precise,endpoint));
        endpoint[12]=std::numeric_limits<float>::infinity();
        require(!jfg::rigid_pose_matches_matrix(precise,endpoint));
    }

    // A stationary owned bridge must not stop at source cadence merely
    // because one of its vertices crosses the camera plane during a small pan.
    const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    const std::array<float,16> perspective{1,0,0,0,0,1,0,0,0,0,1,1,0,0,-1,0};
    jfg::PresentationMesh bridge;
    bridge.key={10,11,12}; bridge.positions={{{1,0,0.005f}},{{0,0,10}}};
    bridge.matrix=perspective;
    auto pan=bridge; pan.matrix[14]=-1.01f; pan.matrix[15]=-0.01f;
    require(!jfg::continuous_presentation_motion(bridge,pan));
    jfg::RigidPresentationPose pose{identity,identity,perspective};
    auto next_pose=pose; next_pose.view[14]=-0.01f;
    require(jfg::rigid_pose_matches_matrix(pose,bridge.matrix));
    require(jfg::rigid_pose_matches_matrix(next_pose,pan.matrix));
    bridge.rigid_pose=pose; pan.rigid_pose=next_pose;
    require(jfg::continuous_presentation_motion(bridge,pan));
    jfg::PresentationHistory bridge_history;
    bridge_history.record({bridge},200,true);
    require(bridge_history.record({pan},202,true).transforms[0].matched);
    {
        auto camera_a=bridge,camera_b=pan;
        camera_a.rigid_pose.reset();camera_b.rigid_pose.reset();
        camera_a.camera_owner=camera_b.camera_owner=std::array<std::uint64_t,2>{77,1};
        camera_a.camera_pose=pose;camera_b.camera_pose=next_pose;camera_b.key={45,46};
        auto camera_part=camera_b;camera_part.key={51,52};
        camera_part.complete_local_topology=false;
        jfg::PresentationHistory direct;direct.record({camera_a},300,true);
        auto camera_result=direct.record({camera_b,camera_part},302,true);
        require(camera_result.transforms[0].matched && camera_result.transforms[1].matched);
        require(camera_result.transforms[0].previous==camera_a.matrix);
        auto wrong_camera=camera_part;wrong_camera.camera_pose->view[0]=2;
        jfg::PresentationHistory conflict;conflict.record({camera_a},300,true);
        camera_result=conflict.record({camera_b,wrong_camera},302,true);
        require(!camera_result.transforms[0].matched && !camera_result.transforms[1].matched);
        jfg::PresentationHistory reversed;reversed.record({camera_a},300,true);
        camera_result=reversed.record({wrong_camera,camera_b},302,true);
        require(!camera_result.transforms[0].matched && !camera_result.transforms[1].matched);
        auto camera_cut=camera_b;camera_cut.camera_pose->view[12]=300;camera_cut.matrix[12]=300;
        jfg::PresentationHistory cut_history;cut_history.record({camera_a},300,true);
        require(!cut_history.record({camera_cut},302,true).transforms[0].matched);
        auto new_camera=camera_b;new_camera.camera_owner->at(0)++;
        jfg::PresentationHistory new_history;new_history.record({camera_a},300,true);
        require(!new_history.record({new_camera},302,true).transforms[0].matched);
        auto bad_camera_vertex=camera_b;bad_camera_vertex.positions[0][0]=std::numeric_limits<float>::infinity();
        jfg::PresentationHistory bad_history;bad_history.record({camera_a},300,true);
        require(!bad_history.record({bad_camera_vertex},302,true).transforms[0].matched);
        auto anchored_a=camera_a,anchored_b=camera_a;
        anchored_a.positions=anchored_b.positions={{{0,0,10}}};
        anchored_b.camera_pose->projection[0]=2.0f; // Doesn't describe emitted bytes.
        jfg::PresentationHistory fallback_history;fallback_history.record({anchored_a},300,true);
        require(fallback_history.record({anchored_b},302,true).transforms[0].matched);
        jfg::PresentationHistory gap_history;gap_history.record({camera_a},300,true);
        gap_history.record({},302,true);
        require(!gap_history.record({camera_b},304,true).transforms[0].matched);
    }
    auto teleported=next_pose; teleported.view[12]=300;
    require(!jfg::stationary_rigid_camera_motion(pose,teleported));
    auto turn=pose; turn.view[0]=turn.view[10]=0; turn.view[2]=1;turn.view[8]=-1;
    require(!jfg::stationary_rigid_camera_motion(pose,turn));
    auto changed_model=next_pose; changed_model.world[12]=0.5f;
    require(!jfg::stationary_rigid_camera_motion(pose,changed_model));
    require(!jfg::rigid_pose_matches_matrix(changed_model,pan.matrix));
    auto zoom=next_pose; zoom.projection[0]=2;
    require(!jfg::stationary_rigid_camera_motion(pose,zoom));
    {
        auto small_zoom=next_pose;
        small_zoom.projection[0]*=1.01f;small_zoom.projection[5]*=1.01f;
        require(!jfg::stationary_rigid_camera_motion(pose,small_zoom));
        require(jfg::continuous_owned_camera_motion(pose,small_zoom));
        require(jfg::continuous_owned_camera_motion(small_zoom,pose));
        auto aspect=small_zoom;aspect.projection[5]=pose.projection[5];
        require(!jfg::continuous_owned_camera_motion(pose,aspect));
        auto depth=small_zoom;depth.projection[14]+=0.01f;
        require(!jfg::continuous_owned_camera_motion(pose,depth));
        auto abrupt=small_zoom;abrupt.projection[0]=abrupt.projection[5]=1.03f;
        require(!jfg::continuous_owned_camera_motion(pose,abrupt));
        auto reflected=small_zoom;reflected.projection[11]*=-1.0f;
        require(!jfg::continuous_owned_camera_motion(pose,reflected));
        auto nonfinite=small_zoom;nonfinite.projection[0]=std::numeric_limits<float>::infinity();
        require(!jfg::continuous_owned_camera_motion(pose,nonfinite));
        auto moving_cut=small_zoom;moving_cut.view[12]=300;
        require(!jfg::continuous_owned_camera_motion(pose,moving_cut));
        auto first=bridge,last=pan;
        first.rigid_pose.reset();last.rigid_pose.reset();
        first.camera_owner=last.camera_owner=std::array<std::uint64_t,2>{78,1};
        first.camera_pose=pose;last.camera_pose=small_zoom;
        last.matrix[0]*=1.01f;last.matrix[5]*=1.01f;last.key={501};
        jfg::PresentationHistory zoom_history;zoom_history.record({first},300,true);
        require(zoom_history.record({last},302,true).transforms[0].matched);
    }
    auto singular=next_pose;singular.view[0]=0;
    require(!jfg::stationary_rigid_camera_motion(pose,singular));
    auto table_view=next_pose;table_view.view[5]=table_view.view[10]=0.9993437f;
    require(jfg::stationary_rigid_camera_motion(pose,table_view));
    auto scaled_view=next_pose;scaled_view.view[5]=0.99f;
    require(!jfg::stationary_rigid_camera_motion(pose,scaled_view));

    auto nan=next_pose;nan.view[12]=std::numeric_limits<float>::quiet_NaN();
    require(!jfg::stationary_rigid_camera_motion(pose,nan));
    require(!jfg::rigid_pose_matches_matrix(nan,pan.matrix));
    auto recycled=pan;recycled.key.push_back(99);
    require(!bridge_history.record({recycled},204,true).transforms[0].matched);
    auto invalid_vertex=pan;invalid_vertex.positions[0][0]=std::numeric_limits<float>::infinity();
    require(!jfg::continuous_presentation_motion(bridge,invalid_vertex));

    require(jfg::parse_render_fps("") == 0U);
    require(jfg::parse_render_fps("60") == 60U);
    require(jfg::parse_render_fps("360") == 360U);
    require(jfg::parse_render_fps("1000") == 1000U);
    for (auto text : {"59", "1001", "-60", "60hz", " 60", "999999999999"})
        require(!jfg::parse_render_fps(text));
    jfg::PresentationMesh a;
    a.key = {1,2,3}; a.positions = {{{0,0,0}}, {{0.1f,0,0}}, {{0,0.1f,0}}};
    a.matrix = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    jfg::PresentationHistory history;
    require(!history.record(std::vector<jfg::PresentationMesh>{a},100,true).transforms[0].matched);
    auto b=a; b.matrix[12]=0.1f;
    const auto second=history.record(std::vector<jfg::PresentationMesh>{b},102,true);
    require(second.source_rate==30 && second.transforms[0].matched);
    require(second.transforms[0].previous==a.matrix);
    history.record({},103,false);
    b.matrix[12]=0.2f;
    require(history.record(std::vector<jfg::PresentationMesh>{b},104,true).transforms[0].matched);
    auto cut=b; cut.matrix[12]=1;
    require(!history.record(std::vector<jfg::PresentationMesh>{cut},106,true).transforms[0].matched);
    std::vector<jfg::PresentationMesh> duplicates{cut,cut};
    const auto duplicate=history.record(duplicates,108,true);
    require(!duplicate.transforms[0].matched && !duplicate.transforms[1].matched);
    require(!history.record(std::vector<jfg::PresentationMesh>{cut},110,true).transforms[0].matched);
    require(history.record(std::vector<jfg::PresentationMesh>{cut},112,true).transforms[0].matched);
    require(!history.record(std::vector<jfg::PresentationMesh>{cut},200,true).transforms[0].matched);
    cut.key.push_back(9);
    require(!history.record(std::vector<jfg::PresentationMesh>{cut},202,true).transforms[0].matched);
    auto invalid=cut; invalid.matrix[0]=std::numeric_limits<float>::quiet_NaN();
    require(!history.record(std::vector<jfg::PresentationMesh>{invalid},204,true).transforms[0].matched);
    // Two touching batches share one transform. A changed/cull-split batch
    // must follow the same motion as its exact-geometry anchor.
    auto left=a; auto right=a; right.key={4,5,6};
    jfg::PresentationHistory coherent;
    coherent.record({left,right},100,true);
    left.matrix[12]=right.matrix[12]=0.1f;
    right.key.push_back(7); // Topology changed; the transform did not split.
    auto together=coherent.record({left,right},102,true);
    require(together.transforms[0].matched && together.transforms[1].matched);
    require(together.transforms[0].previous==together.transforms[1].previous);
    const auto midpoint=[&](size_t i) {
        return 0.5f*together.transforms[i].previous[12]+0.05f;
    };
    require(midpoint(0)==midpoint(1));

    // Unsafe mixed-transform geometry holds the entire shared transform.
    jfg::PresentationHistory blocked;
    blocked.record({a,a},100,true);
    auto distinct=a; distinct.key={4,5,6};
    blocked.record({a,distinct},102,true);
    left=a; left.matrix[12]=0.1f;
    distinct.matrix=left.matrix; distinct.eligible=false;
    const auto held=blocked.record({left,distinct},104,true);
    require(!held.transforms[0].matched && !held.transforms[1].matched);

    // Different old transforms converging at the same matrix are ambiguous.
    jfg::PresentationHistory conflict;
    right=a; right.key={4,5,6}; right.matrix[12]=0.2f;
    conflict.record({a,right},100,true);
    right.matrix[12]=left.matrix[12]=0.1f;
    const auto ambiguous=conflict.record({left,right},102,true);
    require(!ambiguous.transforms[0].matched && !ambiguous.transforms[1].matched);
    // A changing 30/20 Hz cadence is valid motion. Shadow submissions do
    // not consume the scene's previous timestamp; cuts still reject it.
    jfg::PresentationHistory variable;
    auto moving=a;
    variable.record({moving},100,true);
    moving.matrix[12]=0.05f;
    require(variable.record({moving},102,true).source_rate==30);
    variable.record({},103,false);
    moving.matrix[12]=0.10f;
    const auto slower=variable.record({moving},105,true);
    require(slower.source_rate==20 && slower.transforms[0].matched);
    require(slower.transforms[0].previous[12]==0.05f);
    moving.matrix[12]=0.15f;
    const auto faster=variable.record({moving},107,true);
    require(faster.source_rate==30 && faster.transforms[0].matched);
    require(faster.transforms[0].previous[12]==0.10f);
    moving.matrix[12]=1.0f;
    require(!variable.record({moving},110,true).transforms[0].matched);
    require(variable.record({moving},114,true).transforms[0].matched);
    require(variable.record({moving},118,true).source_rate==15);
    // A missed update and recovery are timing changes, not camera cuts.
    // These intervals occur in the owner's later-cave capture.
    jfg::PresentationHistory missed;
    auto late=a;
    std::uint64_t tick=1000;
    missed.record({late},tick,true);
    for (const unsigned interval : {2U,4U,2U,1U,3U,6U,2U}) {
        tick+=interval;late.matrix[12]+=0.01f;
        const auto sample=missed.record({late},tick,true);
        require(sample.source_rate==60U/interval && sample.transforms[0].matched);
        require(sample.transforms[0].previous[12]<late.matrix[12]);
    }
    late.matrix[12]+=1.0f;
    require(!missed.record({late},tick+4U,true).transforms[0].matched);
    require(!missed.record({late},tick+40U,true).transforms[0].matched);
    require(!missed.record({late},tick+40U,true).transforms[0].matched);
    // A living sprite can change dimensions without becoming another object.
    // Its anchor still interpolates; free/reuse and topology changes hold.
    auto sprite = a;
    sprite.positions = {{{0,0,0}}};
    sprite.key = {0,0,0,0xFFFFFFFFU,1,0,0,0};
    sprite.matrix[12] = 0;
    auto resized = sprite;
    resized.positions[0][0] = 0.05f;
    resized.key[0] = 123;
    resized.matrix[12] = 0.1f;
    jfg::bind_presentation_billboard(sprite,{71,72,73,74});
    jfg::bind_presentation_billboard(resized,{71,72,73,74});
    jfg::PresentationHistory sprite_history;
    sprite_history.record({sprite},100,true);
    require(sprite_history.record({resized},102,true).transforms[0].matched);
    auto reused_sprite = a;
    reused_sprite.positions = {{{0,0,0}}};
    reused_sprite.key = {0,0,0,0xFFFFFFFFU,1,0,0,0};
    jfg::bind_presentation_billboard(reused_sprite,{75,72,73,74});
    require(!sprite_history.record({reused_sprite},104,true).transforms[0].matched);
    auto topology = resized;
    topology.key.push_back(1);
    jfg::PresentationHistory topology_history;
    topology_history.record({resized},100,true);
    require(!topology_history.record({topology},102,true).transforms[0].matched);
    auto unknown = a;
    const auto original_key = unknown.key;
    jfg::bind_presentation_billboard(unknown,{0,72,73,74});
    require(unknown.key == original_key);

    // Identical rigid crates need actor identity; draw order is not identity.
    auto crate_a=a, crate_b=a;
    crate_a.matrix[12]=0.0f; crate_b.matrix[12]=0.1f;
    jfg::PresentationHistory unbound_crates;
    unbound_crates.record({crate_a,crate_b},100,true);
    const auto unbound=unbound_crates.record({crate_b,crate_a},102,true);
    require(!unbound.transforms[0].matched && !unbound.transforms[1].matched);
    jfg::bind_presentation_rigid(crate_a,{101,102,103,104});
    jfg::bind_presentation_rigid(crate_b,{201,202,103,104});
    jfg::PresentationHistory rigid_history;
    rigid_history.record({crate_a,crate_b},100,true);
    crate_a.matrix[12]=0.01f; crate_b.matrix[12]=0.11f;
    const auto rigid_frame=rigid_history.record({crate_b,crate_a},102,true);
    require(rigid_frame.transforms[0].matched && rigid_frame.transforms[1].matched);
    require(rigid_frame.transforms[0].previous[12]==0.1f &&
            rigid_frame.transforms[1].previous[12]==0.0f);
    auto changed_crate=a;
    changed_crate.matrix[12]=0.02f;
    jfg::bind_presentation_rigid(changed_crate,{301,102,103,104});
    require(!rigid_history.record({changed_crate},104,true).transforms[0].matched);
    jfg::PresentationHistory rigid_geometry;
    rigid_geometry.record({crate_a},100,true);
    crate_a.key.push_back(17); // identity cannot authorize different geometry
    require(!rigid_geometry.record({crate_a},102,true).transforms[0].matched);
    // Reproduced camera stall: equal matrices in distinct slots were marked
    // ineligible, poisoning the whole scenery cohort. Incomplete cross-slot
    // geometry can follow a complete anchor, but cannot invent one itself.
    {
        auto anchor=a, cross_left=a, cross_right=a;
        anchor.key={901};cross_left.key={902};cross_right.key={903};
        anchor.matrix=cross_left.matrix=cross_right.matrix=identity;
        anchor.positions=cross_left.positions=cross_right.positions={{{0,0,0}}};
        require(jfg::allow_shared_matrix_triangle(cross_left,cross_right,cross_left));
        require(cross_left.eligible && cross_right.eligible &&
            !cross_left.complete_local_topology && !cross_right.complete_local_topology);
        jfg::PresentationHistory shared;
        shared.record({anchor,cross_left,cross_right},100,true);
        anchor.matrix[12]=cross_left.matrix[12]=cross_right.matrix[12]=0.1f;
        const auto frame=shared.record({anchor,cross_left,cross_right},102,true);
        for(const auto& transform:frame.transforms)
            require(transform.matched && transform.previous==identity);
        jfg::PresentationHistory unanchored;
        unanchored.record({cross_left,cross_right},100,true);
        const auto no_anchor=unanchored.record({cross_left,cross_right},102,true);
        require(!no_anchor.transforms[0].matched && !no_anchor.transforms[1].matched);
        auto different=cross_right;different.matrix[12]+=0.01f;
        require(!jfg::allow_shared_matrix_triangle(cross_left,different,cross_left));
        require(!cross_left.eligible && !different.eligible);
        require(!shared.record({anchor,cross_left,different},104,true).transforms[0].matched);
        auto invalid_cross=a;invalid_cross.matrix[0]=std::numeric_limits<float>::infinity();
        auto invalid2=invalid_cross;
        require(!jfg::allow_shared_matrix_triangle(invalid_cross,invalid2,invalid_cross));
        invalid_cross=a;invalid_cross.matrix[0]=std::numeric_limits<float>::quiet_NaN();
        invalid2=invalid_cross;
        require(!jfg::allow_shared_matrix_triangle(invalid_cross,invalid2,invalid_cross));
    }
    // Reusing and growing a probe table must not retain old slot indices or
    // turn duplicate shapes into anchors. These keys share low hash bits.
    {
        std::vector<jfg::PresentationMesh> shapes;
        for (unsigned i=0;i<96;++i) {
            auto shape=a;shape.key={i*65536U+1U};shape.matrix=identity;
            shape.matrix[12]=static_cast<float>(i)*0.001f;
            shape.positions={{{0,0,1}}};shapes.push_back(shape);
        }
        jfg::PresentationHistory reused;
        reused.record(shapes,100,true);
        for(const auto& transform:reused.record(shapes,102,true).transforms)
            require(transform.matched);
        const auto duplicate_frame=reused.record({shapes[0],shapes[0]},104,true);
        require(!duplicate_frame.transforms[0].matched && !duplicate_frame.transforms[1].matched);
        require(!reused.record({shapes[0]},106,true).transforms[0].matched);
        require(reused.record({shapes[0]},108,true).transforms[0].matched);
        reused.record({},110,true);
        for(const auto& transform:reused.record(shapes,112,true).transforms)
            require(!transform.matched);
        std::reverse(shapes.begin(),shapes.end());
        for(const auto& transform:reused.record(shapes,114,true).transforms)
            require(transform.matched);
    }
  return 0;
}
