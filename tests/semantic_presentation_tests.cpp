#include "jfg/renderer/presentation_identity.hpp"
#include "jfg/renderer/semantic_presentation.hpp"
#include <cstdlib>
#include <limits>
static void require(bool v) {
  if (!v)
    std::abort();
}
int main() {
  // Reusing storage must preserve identity through growth, reordered subsets,
  // offscreen passes and empty scenes; stale slots must not revive old actors.
  {
    jfg::SemanticPresentationHistory history;
    std::vector<jfg::SemanticPresentationSample> points(512);
    for (std::size_t i=0;i<points.size();++i) {
      auto& point=points[i];point.key={i+1,1,2,3,4,i};
      point.camera={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
      point.world={float(i%17)*0.001f,0,0};point.valid=true;
    }
    (void)history.record(points,10,true,true);
    auto old=points;for(auto& point:points)point.world[0]+=0.01f;
    auto result=history.record(points,12,true,true);
    for(std::size_t i=0;i<points.size();++i)
      require(result[i].matched && result[i].world==old[i].world);
    for(std::size_t i=512;i<2048;++i) {
      auto point=points[i%512];point.key[0]=i+1;point.key[5]=i;points.push_back(point);
    }
    result=history.record(points,14,true,true);
    for(std::size_t i=0;i<points.size();++i)require(result[i].matched==(i<512));
    std::reverse(points.begin(),points.end());points.resize(256);
    result=history.record(points,16,true,true);
    for(const auto& point:result)require(point.matched);
    (void)history.record({},17,false,false);
    result=history.record(points,18,true,true);
    for(const auto& point:result)require(point.matched);
    (void)history.record({},20,true,true);
    result=history.record(points,22,true,true);
    for(const auto& point:result)require(!point.matched);
  }

  const jfg::PresentationMatrix identity{1, 0, 0, 0, 0, 1, 0, 0,
                                         0, 0, 1, 0, 0, 0, 0, 1};
  // A near-plane vertex can move a large screen distance during a tiny,
  // valid camera move. A bending bridge uses owned camera operands, while
  // character joints and real camera/model/deformation cuts remain strict.
  {
    jfg::RigidPresentationPose pose{identity,identity,identity};
    pose.projection[11]=1;pose.projection[15]=0;
    jfg::SemanticPresentationSample old{{90,2,3,4,0x42524944U,0},
        {1,0,0.01f},pose.projection,true};
    auto moved=old;moved.camera[12]=0.1f;moved.world[1]=0.05f;
    require(!jfg::semantic_motion_continuous(old,moved));
    jfg::SemanticRigidPoses poses{{90,pose}};
    jfg::SemanticPresentationHistory bridge;
    (void)bridge.record({old},10,true,true,nullptr,&poses);
    poses[90].view[12]=0.1f;
    require(bridge.record({moved},12,true,true,nullptr,&poses)[0].matched);
    auto jump=moved;jump.world[1]=200;
    require(!bridge.record({jump},14,true,true,nullptr,&poses)[0].matched);
    jfg::SemanticPresentationHistory camera_cut;
    (void)camera_cut.record({old},10,true,true,nullptr,&poses);
    poses[90].view[12]=1000;
    require(!camera_cut.record({moved},12,true,true,nullptr,&poses)[0].matched);
    jfg::SemanticPresentationHistory invalid_camera;
    poses[90]=pose;
    (void)invalid_camera.record({old},10,true,true,nullptr,&poses);
    auto nan_camera=moved;nan_camera.camera[0]=std::numeric_limits<float>::quiet_NaN();
    poses[90].view[12]=0.1f;
    require(!invalid_camera.record({nan_camera},12,true,true,nullptr,&poses)[0].matched);
    old.key[4]=moved.key[4]=1;
    jfg::SemanticPresentationHistory character;
    poses[90]=pose;
    (void)character.record({old},10,true,true,nullptr,&poses);
    poses[90].view[12]=0.1f;
    require(!character.record({moved},12,true,true,nullptr,&poses)[0].matched);
  }
  jfg::SemanticPresentationSample a{
      {1, 2, 3, 4, 5, 6}, {0, 0, 0}, identity, true};
  auto b = a;
  b.world[0] = 0.1f;
  b.camera[12] = 0.02f;
  jfg::SemanticPresentationHistory history;
  require(!history.record({a, a}, 100, true, false)[0].matched);
  auto current = history.record({b, b}, 102, true, true);
  require(current[0].matched && current[1].matched &&
          current[0].world == a.world);
  history.record({}, 103, false,
                 false); // Offscreen work cannot consume the scene history.
  a = b;
  a.world[0] = 0.2f;
  require(history.record({a}, 104, true, true)[0].matched);
  auto cut = a;
  cut.camera[12] = 2.0f;
  require(!history.record({cut}, 106, true, true)[0].matched);
  auto different = a;
  different.key[3]++;
  require(!history.record({different}, 108, true, true)[0].matched);
  require(history.record({different}, 110, true, true)[0].matched);
  different.key[0]++; // Reused actor address, new lifetime.
  require(!history.record({different}, 112, true, true)[0].matched);
  require(!history.record({different}, 200, true, true)[0].matched);

  jfg::SemanticPresentationHistory coherent;
  a.world = {0, 0, 0};
  a.camera = identity;
  b = a;
  b.key[4]++; // A second joint of the same actor.
  coherent.record({a, b}, 100, true, false);
  a.world[0] = 0.1f;
  b.world[0] = 0.1f;
  b.valid = false;
  auto blocked = coherent.record({a, b}, 102, true, true);
  require(!blocked[0].matched && !blocked[1].matched);
  b.valid = true;
  require(!coherent.record({a, b}, 104, true, true)[0].matched);
  current = coherent.record({a, b}, 106, true, true);
  require(current[0].matched && current[1].matched);
  auto duplicate = a;
  duplicate.world[0] = 0.2f;
  current = coherent.record({a, b, duplicate}, 108, true, true);
  require(!current[0].matched && !current[1].matched);
  require(!coherent.record({a, b}, 110, true, true)[0].matched);
  a.world[0] = std::numeric_limits<float>::quiet_NaN();
  require(!coherent.record({a, b}, 112, true, true)[0].matched);
  // A nonfinite vertex poisons the entire stored actor endpoint even if that
  // vertex is culled in the next scene. Valid visible parts must not reuse it.
  require(!coherent.record({b}, 114, true, true)[0].matched);
  require(coherent.record({b}, 116, true, true)[0].matched);

  // Synthetic guest layout: changing palette buffers keeps identity; frees,
  // absent actors, level changes and camera selection invalidate ownership.
  std::vector<std::uint8_t> ram(8U * 1024U * 1024U);
  const auto w32 = [&](std::uint32_t at, std::uint32_t value) {
    std::memcpy(ram.data() + (at & 0xffffffU), &value, 4);
  };
  const auto w8 = [&](std::uint32_t at, std::uint8_t value) {
    ram[(at & 0xffffffU) ^ 3U] = value;
  };
  w32(0x800FB114U, 1);
  w32(0x800F2CA4U, 0x80130000U);
  w32(0x800F2CA8U, 1);
  w32(0x80130000U, 0x80100000U);
  w32(0x80100040U, 0x80110000U);
  w32(0x8010006CU, 0x80120000U);
  w8(0x8011001FU, 1);
  w32(0x80120000U, 0x80140000U);
  w32(0x80140000U, 0x80150000U);
  w8(0x8015004EU, 1);
  w8(0x8015004FU, 2);
  w8(0x80150013U, 3);
  w32(0x80140004U, 0x80160000U);
  w32(0x80140010U, 0x80170000U);
  w32(0x80140014U, 0x80170100U);
  w32(0x80180000U, 0x80190000U);
  w32(0x800FA630U, 1);
  w32(0x800FA634U, 0);
  jfg::PresentationIdentityTracker tracker;
  tracker.camera(ram, 0x80180000U, 100);
  const auto first = tracker.capture(ram, 100);
  require(first.models.size() == 1 && first.models[0].bone_count == 2 &&
          first.models[0].vertex_count == 3);
  require(first.cameras.size() == 1 && first.cameras[0].matrix == 0x190000U);
  require(tracker.capture(ram, 102).models[0].lifetime ==
          first.models[0].lifetime);
  tracker.retire(0x80100000U);
  const auto reused = tracker.capture(ram, 104);
  require(reused.models[0].lifetime != first.models[0].lifetime);
  require(first.models[0].palettes[0] ==
          0x170000U); // Retained task metadata is immutable.
  w32(0x800F2CA8U, 0);
  tracker.capture(ram, 106);
  w32(0x800F2CA8U, 1);
  require(tracker.capture(ram, 108).models[0].lifetime !=
          reused.models[0].lifetime);
  require(tracker.capture(ram, 108)
              .cameras.empty()); // Stale camera buffer ownership expires.
  w32(0x800FB114U, 2);
  require(tracker.capture(ram, 110).models[0].lifetime !=
          reused.models[0].lifetime);
  w8(0x801000A0U, 1);
  require(tracker.capture(ram, 112).models.empty());
  w8(0x801000A0U, 0);
  w32(0x800F2CA8U, 257);
  require(tracker.capture(ram, 114).models.empty());
  // Camera snapshots must retain captured operands and reject buffer reuse.
  w32(0x800F2CA8U,1);w32(0x80180000U,0x80190000U);
  tracker.camera(ram,0x80180000U,114);
  for(unsigned i=0;i<16;++i) {
    w32(0x800FAF78U+i*4U,0x3f800000U);w32(0x800FAA70U+i*4U,0x40000000U);
    w32(0x80190000U+i*4U,i+17U);
  }
  tracker.camera_completed(ram,0x80190000U,114);
  const auto captured_camera=tracker.capture(ram,114);
  require(captured_camera.cameras.size()==1 && captured_camera.cameras[0].pose_valid);
  require(captured_camera.cameras[0].generation!=0);
  w32(0x800FAF78U,0x40800000U);
  require(tracker.capture(ram,115).cameras[0].pose.view[0]==1.0f);
  w32(0x80190004U,999U);
  require(!tracker.capture(ram,115).cameras[0].pose_valid);
  // Byte-swapped access must remain bounded even for a partial final word.
  const std::array<std::uint8_t, 5> truncated{0, 1, 2, 3, 4};
  const jfg::PresentationGuestView short_ram(truncated);
  require(short_ram.u8(4) == 0 && short_ram.u8(1) == 2);
  require(short_ram.u32(4) == 0 && short_ram.u32(0x81100000U) == 0);
  w32(0x800F2CA8U, 1);
  w8(0x8015004FU, 128);
  require(
      tracker.capture(ram, 116).models.empty()); // Guest bone count is signed.
  // Goldwood drones: compact LOD geometry shares the full model's palette.
  // A second identical enemy must not acquire the first one's vertices.
  jfg::PresentationSceneIdentity scene;
  scene.models.push_back(
      {1, 0x1000, 0x2000, 0x3000, {0x4000, 0x5000}, 0x6000, 20, 2});
  scene.models.push_back({1, 0x1000, 0x2100, 0x3100, {}, 0x7000, 8, 2});
  scene.models.push_back(
      {2, 0x1100, 0x2200, 0x3000, {0x8000, 0x9000}, 0xa000, 20, 2});
  scene.models.push_back({2, 0x1100, 0x2300, 0x3100, {}, 0xb000, 8, 2});
  auto vertex = jfg::presentation_vertex_identity(scene, 0x4040, 0x701e);
  require(vertex.owner && vertex.geometry && vertex.geometry->model == 0x3100 &&
          vertex.geometry->instance == 0x2100 && vertex.joint == 1 &&
          vertex.vertex == 3);
  vertex = jfg::presentation_vertex_identity(scene, 0x5040, 0x701e);
  require(vertex.geometry &&
          vertex.geometry->instance == 0x2100); // palette swap
  require(!jfg::presentation_vertex_identity(scene, 0x4040, 0xb01e).geometry);
  require(!jfg::presentation_vertex_identity(scene, 0x4040, 0x701f).geometry);
  require(!jfg::presentation_vertex_identity(scene, 0x4041, 0x701e).owner);
  scene.models.push_back(scene.models[0]);
  const auto ambiguous_palette = jfg::presentation_vertex_identity(scene, 0x4040, 0x701e);
  require(ambiguous_palette.owner && !ambiguous_palette.geometry);

  // Compact instances still appear in the task identity snapshot. Their
  // inline vertex bytes are not treated as pointers to a private palette.
  w8(0x8015004FU, 2);
  w8(0x8011001FU, 2);
  w32(0x80120004U, 0x80140100U);
  w32(0x80140100U, 0x80150100U);
  w32(0x80140104U, 0x80160100U);
  w8(0x8015014EU, 1);
  w8(0x8015014FU, 2);
  w8(0x80150113U, 3);
  w32(0x80140110U, 0xffff0001U);
  w32(0x80140114U, 0x00140020U);
  const auto lod = tracker.capture(ram, 118);
  require(lod.models.size() == 2 && lod.models[1].palettes[0] == 0 &&
          lod.models[1].vertices == 0x160100U);
  vertex = jfg::presentation_vertex_identity(lod, 0x170040U, 0x16010aU);
  require(vertex.geometry && vertex.geometry->instance == 0x80140100U &&
          vertex.vertex == 1);

  // Inline compact vertices can accidentally look like in-range pointers.
  // Their overlap with the palette fields must still prevent ownership.
  w32(0x80140104U, 0x80140110U);
  w32(0x80140110U, 0x80171000U);
  w32(0x80140114U, 0x80171100U);
  const auto inline_lod = tracker.capture(ram, 118);
  require(inline_lod.models.size() == 2 &&
          inline_lod.models[1].palettes[0] == 0);

  // Identical billboard assets retain distinct actor lifetimes. Address
  // reuse, object frees, expiry and level changes cannot reuse old ownership.
  w32(0x80180000U, 0x80190000U);
  tracker.sprite(ram, 0x80180000U, 0x80100000U, 0x801a0000U, 118);
  auto sprites = tracker.capture(ram, 118);
  require(sprites.sprites.size() == 1 &&
          sprites.sprites[0].anchor == 0x190000U &&
          sprites.sprites[0].lifetime == sprites.models[0].lifetime);
  const auto old_sprite = sprites.sprites[0].lifetime;
  tracker.retire(0x80100000U);
  require(tracker.capture(ram, 120).sprites.empty());
  tracker.sprite(ram, 0x80180000U, 0x80100000U, 0x801a0000U, 120);
  sprites = tracker.capture(ram, 120);
  require(sprites.sprites.size() == 1 &&
          sprites.sprites[0].lifetime != old_sprite);
  require(tracker.capture(ram, 127).sprites.empty());
  tracker.sprite(ram, 0x80180000U, 0x80100000U, 0x801a0000U, 128);
  w32(0x800FB114U, 3);
  require(tracker.capture(ram, 128).sprites.empty());
  // Multiple pickups with the same asset are separate owners, including
  // a sprite-only actor with no model array. Freeing one preserves the other.
  w32(0x800F2CA8U, 2);
  w32(0x80130004U, 0x80100200U);
  w32(0x80100240U, 0x80110200U);
  w32(0x8010026CU, 0);
  w8(0x8011021FU, 0);
  w32(0x80180004U, 0x80190010U);
  tracker.sprite(ram, 0x80180000U, 0x80100000U, 0x801a0000U, 130);
  tracker.sprite(ram, 0x80180004U, 0x80100200U, 0x801a0000U, 130);
  sprites = tracker.capture(ram, 130);
  require(sprites.sprites.size() == 2 &&
          sprites.sprites[0].sprite == sprites.sprites[1].sprite &&
          sprites.sprites[0].actor != sprites.sprites[1].actor &&
          sprites.sprites[0].lifetime != sprites.sprites[1].lifetime);
  tracker.retire(0x80100000U);
  sprites = tracker.capture(ram, 132);
  require(sprites.sprites.size() == 1 && sprites.sprites[0].actor == 0x80100200U);
  scene.models.resize(3);
  scene.models.push_back(scene.models[1]);
  scene.models.back().instance++;
  require(!jfg::presentation_vertex_identity(scene, 0x4040, 0x701e).geometry);
  // Pool particles share assets but never lifetimes. Free and immediate
  // reuse between captures must invalidate the old interpolation history.
  tracker.particles_clear();
  tracker.retire(0x80100200U);
  w8(0x801b002dU, 2);
  w32(0x801b004cU, 0x801c0000U);
  tracker.particle_sprite(ram,0x80180000U,0x801b0000U,0x801c0000U,134);
  require(tracker.capture(ram,134).sprites.empty()); // no observed allocation
  tracker.particle_allocate(ram,0x801b0000U);
  tracker.particle_sprite(ram,0x80180000U,0x801b0000U,0x801c0000U,134);
  sprites = tracker.capture(ram,134);
  require(sprites.sprites.size()==1 && sprites.sprites[0].actor==0x801b0000U);
  const auto particle_lifetime = sprites.sprites[0].lifetime;
  w8(0x801b00a5U,2);
  w32(0x801b00c4U,0x801c0000U);
  tracker.particle_allocate(ram,0x801b0078U);
  tracker.particle_sprite(ram,0x80180004U,0x801b0078U,0x801c0000U,134);
  sprites = tracker.capture(ram,134);
  require(sprites.sprites.size()==2 &&
          sprites.sprites[0].lifetime!=sprites.sprites[1].lifetime);
  tracker.particle_retire(0x801b0000U);
  tracker.particle_allocate(ram,0x801b0000U);
  require(tracker.capture(ram,136).sprites.size()==1);
  tracker.particle_sprite(ram,0x80180000U,0x801b0000U,0x801c0000U,136);
  sprites=tracker.capture(ram,136);
  require(sprites.sprites.size()==2 && sprites.sprites[0].lifetime!=particle_lifetime);
  w32(0x801b004cU,0x801c0100U); // asset changed without a fresh anchor
  require(tracker.capture(ram,136).sprites.size()==1);
  tracker.particle_sprite(ram,0x80180000U,0x801b0000U,0x801c0000U,136);
  require(tracker.capture(ram,136).sprites.size()==1);
  require(tracker.capture(ram,143).sprites.empty()); // stale anchors
  tracker.particle_allocate(ram,0); // failed allocation cannot mint ownership
  w8(0x801b00a5U,0x80); // a freed slot is never captured
  tracker.particle_sprite(ram,0x80180004U,0x801b0078U,0x801c0000U,144);
  require(tracker.capture(ram,144).sprites.empty());
  w8(0x801b00a5U,2);
  tracker.particle_sprite(ram,0x80180004U,0x801b0078U,0x801c0000U,144);
  require(tracker.capture(ram,144).sprites.size()==1);
  tracker.particles_clear();
  require(tracker.capture(ram,144).sprites.empty());
  tracker.particle_allocate(ram,0x801b0078U);
  tracker.particle_sprite(ram,0x80180004U,0x801b0078U,0x801c0000U,144);
  w32(0x800FB114U,4);
  require(tracker.capture(ram,144).sprites.empty());

  // Two-frame rigid matrix buffers preserve actor/model identity, while
  // overwritten memory, freed actors and changed camera/level must not.
  w32(0x800FB114U, 5); w32(0x800F2CA8U, 1);
  w8(0x801000A0U, 0); w8(0x8011001FU, 1);
  w32(0x80120000U, 0x80140000U); w32(0x80140000U,0x80150000U);
  w32(0x80140004U,0x80160000U); w8(0x80150013U,3);
  w8(0x8015004EU,0); w8(0x8015004FU,0);
  w32(0x800FA630U,1); w32(0x800FA634U,0);
  w32(0x800FAAB0U,0x3f800000U); w32(0x800FAF78U,0x40000000U);w32(0x800FAA70U,0x40400000U);
  jfg::PresentationIdentityTracker rigid_tracker;
  rigid_tracker.rigid(ram,0x80190000U,0x80100000U,200);
  auto rigid_scene=rigid_tracker.capture(ram,200);
  require(rigid_scene.rigids[0].pose.world[0]==1 && rigid_scene.rigids[0].pose.view[0]==2 && rigid_scene.rigids[0].pose.projection[0]==3);
  w32(0x800FAAB0U,0x40800000U); // Scratch is reused; captured operands remain immutable.
  require(rigid_tracker.capture(ram,200).rigids[0].pose.world[0]==1);
  require(rigid_scene.rigids.size()==1 && rigid_scene.models.size()==1);
  const auto rigid_key=jfg::presentation_rigid_key(rigid_scene,0x190000U,0x160000U,3);
  require(rigid_key[0]!=0 && rigid_key[3]!=0 && rigid_key[2]==0x80150000U);
  require(jfg::presentation_rigid_key(rigid_scene,0x190000U,0x160001U,1)[0]==0);
  require(jfg::presentation_rigid_key(rigid_scene,0x190000U,0x160000U,4)[0]==0);
  require(jfg::presentation_rigid_key(rigid_scene,0x190000U,0x160000U,0)[0]==0);
  rigid_tracker.rigid(ram,0x80190100U,0x80100000U,202);
  rigid_scene=rigid_tracker.capture(ram,202);
  require(jfg::presentation_rigid_key(rigid_scene,0x190100U,0x160000U,3)==rigid_key);
  w32(0x80190000U,123); // old transient buffer now belongs to other work
  require(rigid_tracker.capture(ram,202).rigids.size()==1);
  rigid_tracker.rigid(ram,0x80190200U,0x80100200U,202); // not in live actor list
  require(rigid_tracker.capture(ram,202).rigids.size()==1);
  require(rigid_tracker.capture(ram,209).rigids.empty());
  rigid_tracker.retire(0x80100000U);
  rigid_tracker.rigid(ram,0x80190000U,0x80100000U,210);
  rigid_scene=rigid_tracker.capture(ram,210);
  const auto reused_rigid=jfg::presentation_rigid_key(rigid_scene,0x190000U,0x160000U,3);
  require(reused_rigid[0]!=0 && reused_rigid[0]!=rigid_key[0]);
  rigid_scene.models.push_back(rigid_scene.models[0]);
  require(jfg::presentation_rigid_key(rigid_scene,0x190000U,0x160000U,3)[0]==0);
  w32(0x800FA634U,1);
  rigid_tracker.rigid(ram,0x80190000U,0x80100000U,212);
  rigid_scene=rigid_tracker.capture(ram,212);
  require(jfg::presentation_rigid_key(rigid_scene,0x190000U,0x160000U,3)[3]!=rigid_key[3]);
  w32(0x800FB114U,6);
  require(rigid_tracker.capture(ram,214).rigids.empty());
  rigid_tracker.rigid(ram,0x80190001U,0x80100000U,216);
  require(rigid_tracker.capture(ram,216).rigids.empty());
  // Bending bridge geometry has stable actor/model/vertex ownership, even
  // when its vertex buffer changes. No other behavior gets this path.
  w8(0x80100048U,0);w8(0x80100049U,55);
  rigid_tracker.rigid(ram,0x80190000U,0x80100000U,218);
  auto bridge_scene=rigid_tracker.capture(ram,218);
  require(bridge_scene.models[0].deforming_bridge && bridge_scene.rigids.size()==1);
  const auto* bridge=jfg::presentation_bridge_vertices(
      bridge_scene,bridge_scene.rigids[0],0x16000aU,2);
  require(bridge && bridge->instance==0x80140000U);
  const auto bridge_lifetime=bridge->lifetime;
  require(!jfg::presentation_bridge_vertices(bridge_scene,bridge_scene.rigids[0],0x16000aU,3));
  require(!jfg::presentation_bridge_vertices(bridge_scene,bridge_scene.rigids[0],0x160001U,1));
  require(!jfg::presentation_bridge_vertices(bridge_scene,bridge_scene.rigids[0],0x160000U,0));
  auto wrong_bridge_owner=bridge_scene.rigids[0];++wrong_bridge_owner.actor;
  require(!jfg::presentation_bridge_vertices(bridge_scene,wrong_bridge_owner,0x160000U,1));
  wrong_bridge_owner=bridge_scene.rigids[0];++wrong_bridge_owner.lifetime;
  require(!jfg::presentation_bridge_vertices(bridge_scene,wrong_bridge_owner,0x160000U,1));
  w32(0x80140004U,0x80161000U);
  bridge_scene=rigid_tracker.capture(ram,218);
  bridge=jfg::presentation_bridge_vertices(bridge_scene,bridge_scene.rigids[0],0x16100aU,2);
  require(bridge && bridge->lifetime==bridge_lifetime && bridge->instance==0x80140000U);
  require(!jfg::presentation_bridge_vertices(bridge_scene,bridge_scene.rigids[0],0x16000aU,1));
  bridge_scene.models.push_back(bridge_scene.models[0]);
  require(!jfg::presentation_bridge_vertices(bridge_scene,bridge_scene.rigids[0],0x16100aU,1));
  w8(0x80100049U,54);bridge_scene=rigid_tracker.capture(ram,218);
  require(!jfg::presentation_bridge_vertices(bridge_scene,bridge_scene.rigids[0],0x16100aU,1));
  w32(0x80140004U,0x80160000U);
  // Procedural beam vertices retain their logical index across the two DMA
  // buffers, but never bind another owner, stale matrix or malformed state.
  w8(0x80100048U,0); w8(0x80100049U,68);
  w32(0x80100068U,0x801d0000U);
  w8(0x801d00ccU,1);w8(0x801d00cdU,0);w8(0x801d00cfU,6);
  rigid_tracker.rigid(ram,0x80190000U,0x80100000U,220);
  auto beam_scene=rigid_tracker.capture(ram,220);
  require(beam_scene.beams.size()==1 && beam_scene.rigids.size()==1);
  const auto* beam=jfg::presentation_beam_vertex(beam_scene,beam_scene.rigids[0],0x1d0046U);
  require(beam && (0x1d0046U-beam->vertices)/10U==7U);
  require(!jfg::presentation_beam_vertex(beam_scene,beam_scene.rigids[0],0x1d0050U));
  require(!jfg::presentation_beam_vertex(beam_scene,beam_scene.rigids[0],0x1d0001U));
  const auto beam_life=beam->lifetime;
  w8(0x801d00cdU,1);
  beam_scene=rigid_tracker.capture(ram,222);
  beam=jfg::presentation_beam_vertex(beam_scene,beam_scene.rigids[0],0x1d0096U);
  require(beam && beam->lifetime==beam_life && (0x1d0096U-beam->vertices)/10U==7U);
  require(!jfg::presentation_beam_vertex(beam_scene,beam_scene.rigids[0],0x1d0046U));
  auto wrong_owner=beam_scene.rigids[0];++wrong_owner.actor;
  require(!jfg::presentation_beam_vertex(beam_scene,wrong_owner,0x1d0050U));
  wrong_owner=beam_scene.rigids[0];++wrong_owner.lifetime;
  require(!jfg::presentation_beam_vertex(beam_scene,wrong_owner,0x1d0050U));
  beam_scene.beams.push_back(beam_scene.beams[0]);
  require(!jfg::presentation_beam_vertex(beam_scene,beam_scene.rigids[0],0x1d0050U));
  w8(0x801d00cdU,2);require(rigid_tracker.capture(ram,224).beams.empty());
  w8(0x801d00cdU,0);w8(0x801d00cfU,8);require(rigid_tracker.capture(ram,224).beams.empty());
  w8(0x801d00cfU,6);w8(0x801d00ccU,0);require(rigid_tracker.capture(ram,224).beams.empty());
  w8(0x801d00ccU,1);w8(0x80100049U,67);require(rigid_tracker.capture(ram,224).beams.empty());
  w8(0x80100049U,68);rigid_tracker.retire(0x80100000U);
  require(rigid_tracker.capture(ram,226).beams[0].lifetime!=beam_life);
  require(rigid_tracker.capture(ram,226).rigids.empty());
  return 0;
}
