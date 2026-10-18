#include "jfg/mod/navigation_mod.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <sstream>

static void
check(bool ok,
      const std::source_location where = std::source_location::current()) {
  if (!ok) {
    std::cerr << "Navigation check failed at " << where.file_name() << ':'
              << where.line() << '\n';
    std::abort();
  }
}
#include "box_jump_tests.hpp"
#include "npc_rewards_tests.hpp"
#include "navigation_pilot_tests.hpp"
#include "navigation_collision_tests.hpp"
int main(int argc, char **argv) {
  box_jump_tests();
  navigation_collision_tests();
  npc_reward_tests();
  navigation_pilot_tests();
  const auto npc_catalog = NpcFixture().catalog();
  using namespace jfg::mod;
  // Private captured-state acceptance uses the production predicates. The
  // memory image stays outside the public repository and is never modified.
  if(argc==5 && std::string_view(argv[1])=="--boss-state") {
    std::ifstream in(argv[2],std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(in),
                                  std::istreambuf_iterator<char>()};
    check(bytes.size()==4194304U);
    Memory m(bytes,false);
    const auto player=static_cast<std::uint32_t>(std::strtoul(argv[3],nullptr,0));
    check(gameplay_active(m,player) && !pc_gameplay_active(m,player));
    check(pc_boss_gameplay_active(m,player) && pc_actions_active(m,player));
    std::ofstream out(argv[4]);
    out << "{\"boss_actions\":true,\"ordinary_camera\":false,\"player\":" << player << "}\n";
    check(bool(out));return 0;
  }
  if (argc >= 3 && argc <= 5) {
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(in),
                                    std::istreambuf_iterator<char>()};
    check(bytes.size() == 4194304U);
    Memory m(bytes, false);
    const auto mesh = decode_mesh(m, m.u32(0x800A0D60U));
    const auto list = actors(m);
    const auto doors = exits(m, list);
    if(argc>=4) {
      std::ofstream collision(argv[3]);write_collision(collision,collision_models(m,list));
      check(bool(collision));
    }
    if(argc==5) {
      std::ofstream progression(argv[4]);
      const auto found=std::find_if(list.begin(),list.end(),[](const Actor &a){return a.behavior==1U;});
      write_progression(progression,m,list,found==list.end()?0U:found->address,m.u32(0x800FB114U));
      check(bool(progression));
    }
    for (const auto &item : item_markers(m, list))
      std::cout << item.label << "\n";
    std::ofstream out(argv[2]);
    write_mesh(out, mesh, m.u32(0x800FB114U), 1);
    std::cout << "blocks=" << mesh.blocks
              << " vertices=" << mesh.vertices.size()
              << " triangles=" << mesh.triangles.size()
              << " exits=" << doors.size() << '\n';
    check(!mesh.triangles.empty() && !doors.empty() && bool(out));
    return 0;
  }
  bool odd_rejected = false;
  try {
    std::array<std::uint8_t, 3> odd{};
    Memory invalid(odd);
  } catch (const std::runtime_error &) {
    odd_rejected = true;
  }
  check(odd_rejected);
  for (bool swapped : {false, true}) {
    std::vector<std::uint8_t> bytes(4194304U);
    Memory m(bytes, swapped);
    const auto put32 = [&](std::uint32_t a, std::uint32_t v) {
      m.put16(a, static_cast<std::uint16_t>(v >> 16U));
      m.put16(a + 2, static_cast<std::uint16_t>(v));
    };
    const auto put8 = [&](std::uint32_t a, std::uint8_t v) {
      bytes[(a & 0x1FFFFFFFU) ^ (swapped ? 3U : 0U)] = v;
    };
    const auto putfloat = [&](std::uint32_t a, float v) {
      put32(a, std::bit_cast<std::uint32_t>(v));
    };
    const std::uint32_t hint=0x80380000U,choice=0x80384000U;
    check(!read_dialogue(m,hint).known);
    put32(hint+0xD84U,0x8C422A24U);put32(hint+0xD88U,0x03E00008U);
    check(read_dialogue(m,hint).known&&!read_dialogue(m,hint).active);
    put32(hint+0x2A24U,1);put32(hint+0x2A20U,256);
    put32(hint+0x2A58U,0x80385000U);
    auto dialogue=read_dialogue(m,hint);
    check(dialogue.active&&dialogue.ready&&!dialogue.choices);
    put32(hint+0x2A38U,16);check(!read_dialogue(m,hint).ready);put32(hint+0x2A38U,0);
    put32(hint+0x2A2CU,1);check(!read_dialogue(m,hint).ready);put32(hint+0x2A2CU,0);
    put32(hint+0x2A44U,1);put32(hint+0x2A5CU,choice);
    m.put16(choice,2);m.put16(choice+2,1);m.put16(choice+4+8,0);m.put16(choice+4+16+8,0x5000);
    dialogue=read_dialogue(m,hint);check(dialogue.choices&&dialogue.rows.size()==2&&dialogue.selected==1);
    std::vector<std::uint8_t> hint_section(0x3000U);
    for(unsigned i=0;i<hint_section.size();++i)
      hint_section[i^3U]=m.u8(hint+i);
    DialogueMemory synthetic(m,0x00700000U,std::span<const std::uint8_t>(hint_section));
    const auto synthetic_state=read_dialogue(synthetic,0x00700000U);
    check(!m.valid(0x00700000U,4));
    check(synthetic_state.known&&synthetic_state.active&&synthetic_state.ready);
    check(synthetic_state.rows.size()==2&&synthetic_state.selected==1);
    check(!synthetic.valid(0x006FFFFCU,4)&&!synthetic.valid(0x00703000U,1));
    check(!read_dialogue(synthetic,0x00800000U).known);
    DialogueInput dialogue_input;
    auto dialogue_send=[&](unsigned nonce,unsigned token,int action,bool manual=false,bool replay=false,unsigned room=27,unsigned gen=13,unsigned epoch=7,int stamp=1000) {
      std::ostringstream text;text<<"JFGDIALOGUE1 "<<room<<' '<<gen<<' '<<nonce<<' '<<stamp<<' '<<epoch<<' '<<token<<' '<<action;
      std::istringstream command(text.str());return dialogue_input.command(command,dialogue,manual,replay,27,13,7,1000);
    };
    check(dialogue_send(1,dialogue.token,1)==2);
    check(dialogue_send(1,dialogue.token,1)==0);
    check(dialogue_send(2,dialogue.token+1,0)==0);
    check(dialogue_send(3,dialogue.token,0,true)==0);
    check(dialogue_send(4,dialogue.token,0,false,true)==0);
    check(dialogue_send(5,dialogue.token,0,false,false,28)==0);
    check(dialogue_send(6,dialogue.token,0,false,false,27,12)==0);
    check(dialogue_send(7,dialogue.token,0,false,false,27,13,8)==0);
    check(dialogue_send(8,dialogue.token,0,false,false,27,13,7,-1)==0);
    check(dialogue_send(9,dialogue.token,0)==1);
    check(dialogue_send(10,dialogue.token,2)==0);
    dialogue.ready=false;check(dialogue_send(11,dialogue.token,0)==0);
    dialogue.ready=true;check(dialogue_send(11,dialogue.token,0)==0);
    m.put16(choice,33);check(!read_dialogue(m,hint).known);
    put32(hint+0xD88U,0);check(!read_dialogue(m,hint).known);
    const std::uint32_t chest_actor=0x8030D000U,chest_data=0x8030E000U;
    put32(chest_actor+0x68U,chest_data);m.put16(chest_actor+0x48U,98U);
    m.put16(chest_actor,0xC000U);
    putfloat(chest_actor+0xCU,100);putfloat(chest_actor+0x10U,60);putfloat(chest_actor+0x14U,20);
    putfloat(chest_data+0x10U,40);putfloat(chest_data+0x14U,0);putfloat(chest_data+0x18U,0);
    putfloat(chest_data+0x1CU,30);putfloat(chest_data+0x20U,40);
    check(!chest_access(m,chest_actor).known);
    put32(0x8003B6C8U,0x27BDFFE8U);
    auto access=chest_access(m,chest_actor);
    check(access.known&&access.point.x==140&&access.point.y==60&&access.radius==30&&access.facing==0x4000);
    put32(chest_data+0x1CU,0x7FC00000U);check(!chest_access(m,chest_actor).known);
    putfloat(chest_data+0x1CU,30);m.put16(chest_actor+0x48U,3U);check(!chest_access(m,chest_actor).known);
    const std::uint32_t motion_actor=0x80300000U,motion_control=0x80301000U;
    put32(motion_actor+0x68U,motion_control);put8(motion_control+0x568U,6);
    put8(motion_actor+0x3BU,42);putfloat(motion_actor+0x28U,3.5f);
    check(!player_motion(m,motion_actor).known);
    put32(0x80036234U,0x24190006U);put32(0x80036248U,0xA2190568U);
    put32(0x80036944U,0x2408000CU);put32(0x8003696CU,0xA2280568U);
    put32(0x80011580U,0xA086003BU);
    const auto motion=player_motion(m,motion_actor);
    check(motion.known && motion.state==6 && motion.animation==42 && motion.animation_frame==3.5f);
    put8(motion_control+0x568U,12);check(player_motion(m,motion_actor).state==12);
    put32(motion_actor+0x68U,motion_control+1U);check(!player_motion(m,motion_actor).known);
    put32(motion_actor+0x68U,0x80400000U);check(!player_motion(m,motion_actor).known);
    put32(0x80011580U,0);check(!player_motion(m,motion_actor).known);
    check(!control_camera(m).known);
    put32(0x80041E0CU,0x3C028010U);put32(0x80041E10U,0x03E00008U);
    put32(0x80041E14U,0x2442A4D0U);put32(0x80032ACCU,0xAC226DC0U);
    put32(0x800F6DC0U,0x800FA4D0U);m.put16(0x800FA4D0U,1234);
    putfloat(0x800FA4DCU,42);
    check(control_camera(m).known && control_camera(m).yaw==1234 && control_camera(m).position.x==42);
    put32(0x800F6DC0U,0x800FA4D1U);check(!control_camera(m).known);
    put32(0x800F6DC0U,0x800FA600U);check(!control_camera(m).known);
    put32(0x800F6DC0U,0x800FA4D0U);put32(0x80041E0CU,0);check(!control_camera(m).known);
    check(!static_camera_active(m));
    put32(0x8004271CU,0x3C028010U);put32(0x80042720U,0x8C42B084U);
    put32(0x80042724U,0x03E00008U);put32(0x80042728U,0U);
    put32(0x800FB084U,0x8010B000U);
    check(static_camera_active(m) && scripted_camera_active(m));
    put32(0x800FB084U,0x8010B001U);check(!static_camera_active(m));
    put32(0x800FB084U,0x80400000U);check(!static_camera_active(m));
    put32(0x800FB084U,0);check(!scripted_camera_active(m));
    put32(0x801045B8U,1);check(scripted_camera_active(m));put32(0x801045B8U,0);
    put32(0x800FB084U,0x8010B000U);put32(0x8004271CU,0);check(!static_camera_active(m));
    put32(0x800FB084U,0);
    const std::uint32_t player = 0x80100000U, control = 0x80101000U,
                        props = 0x80102000U, game = 0x80103000U;
    put32(player + 0x68U, control);
    put32(0x8002B484U,0x8E0505C0U);put32(0x8002B48CU,0x10A0000EU);
    put32(control+0x5C0U,0x8010C000U);m.put16(0x8010C048U,100U);
    check(scripted_camera_active(m,player));
    m.put16(0x8010C048U,54U);check(!scripted_camera_active(m,player));
    m.put16(0x8010C048U,100U);put32(0x8002B484U,0);check(!scripted_camera_active(m,player));
    put32(0x8002B484U,0x8E0505C0U);put32(control+0x5C0U,0x80400000U);check(!scripted_camera_active(m,player));
    put32(control+0x5C0U,0);
    check(pc_gameplay_active(m,player));
    for(auto offset:{0x56BU,0x577U,0x1F9U,0x1FAU,0x186U,0x198U,0x5EEU,0x10DU,0x19FU}) {
      put8(control+offset,1);check(!pc_gameplay_active(m,player));
      put8(control+offset,0);check(pc_gameplay_active(m,player));
    }
    put32(control+0xFCU,0x8010C000U);check(!pc_gameplay_active(m,player));
    put32(control+0xFCU,0);check(pc_gameplay_active(m,player));
    // A real boss update selects a different controller, not a menu. Its
    // fixed camera remains outside the ordinary shoulder/locomotion adapter.
    check(pc_actions_active(m,player) && !pc_boss_gameplay_active(m,player));
    put32(0x80032FD8U,0x8E0D00FCU);put32(0x80032FE0U,0x11A0001EU);
    put32(0x8003301CU,0x0CD00000U);put32(0x80033034U,0x0CC80000U);
    put32(0x8003304CU,0x0CCC0000U);put32(control+0xFCU,0x8010C000U);
    for(auto call:{0x8003301CU,0x80033034U,0x8003304CU}) {
      put32(call-8U,0x8FA40090U);put32(call-4U,0x8FA50094U);
    }
    for(auto character:{0U,1U,2U})for(auto movement:{0x20U,0x23U}) {
      put8(control+1U,static_cast<std::uint8_t>(character));
      put8(control+0x568U,static_cast<std::uint8_t>(movement));
      check(pc_boss_gameplay_active(m,player) && pc_actions_active(m,player));
      check(!pc_gameplay_active(m,player));
    }
    // Captured Vela relocation plus unloaded Juno/Lupus loader thunks.
    put32(0x8003301CU,0x0C0D86CCU);put32(0x80033034U,0x0C01514CU);
    put32(0x8003304CU,0x0C01514CU);check(pc_actions_active(m,player));
    for(auto call:{0x8003301CU,0x80033034U,0x8003304CU}) {
      const auto saved=m.u32(call);put32(call,saved^0x04000000U);
      check(!pc_actions_active(m,player));put32(call,saved);
      put32(call-4U,0U);check(!pc_actions_active(m,player));put32(call-4U,0x8FA50094U);
    }
    const auto boss_bytes=bytes;
    for(int i=0;i<100;++i)check(pc_actions_active(m,player));
    check(bytes==boss_bytes);
    for(auto offset:{0x56BU,0x577U,0x1F9U,0x1FAU,0x186U,0x198U,0x5EEU,0x10DU,0x19FU,0x18EU}) {
      put8(control+offset,1);check(!pc_actions_active(m,player));put8(control+offset,0);
    }
    for(auto gate:{0x800FD7C4U,0x801045B8U,0x800F6DBCU,control+0x5C0U}) {
      put32(gate,1);check(!pc_actions_active(m,player));put32(gate,0);
    }
    for(auto movement:{0U,3U,10U,0x2AU,0xFFU}) {
      put8(control+0x568U,static_cast<std::uint8_t>(movement));check(!pc_actions_active(m,player));
    }
    put8(control+0x568U,0x20U);
    for(auto pointer:{0U,0x8010C001U,0x80400000U}) {
      put32(control+0xFCU,pointer);check(!pc_boss_gameplay_active(m,player));
    }
    put32(control+0xFCU,0x8010C000U);put32(0x80032FD8U,0);
    check(!pc_actions_active(m,player));put32(0x80032FD8U,0x8E0D00FCU);
    put8(control+1U,3);check(!pc_actions_active(m,player));
    put8(control+1U,0);put8(control+0x568U,0);put32(control+0xFCU,0);
    check(pc_actions_active(m,player) && pc_gameplay_active(m,player));
    put32(0x800F6DBCU,1);check(!pc_gameplay_active(m,player));
    put32(0x800F6DBCU,0);check(pc_gameplay_active(m,player));
    put32(player + 0x4CU, props);
    put32(0x800FD7D4U, game);
    m.put16(game + 0x15EU, 6U);
    m.put16(props + 6U, 42U);
    // Speed boost must preserve guest memory, exclude other characters and
    // scripted/special movement, and apply to each fresh input only once.
    {
      NavigationMod speed_mod;
      const auto run = std::bit_cast<std::uint32_t>(6.5F);
      const auto twice = std::bit_cast<std::uint32_t>(13.0F);
      auto target = [&] { return speed_mod.juno_run_target(m, player, control, run); };
      put8(control + 1U, 1U);
      check(target() == run);
      speed_mod.enabled = true; check(target() == run);
      speed_mod.juno_double_run = true;
      const auto original_memory = bytes;
      for (int frame = 0; frame < 1000; ++frame) check(target() == twice);
      check(bytes == original_memory && speed_mod.juno_run_boosts == 1000U);
      for (auto character : {0U, 2U, 3U}) {
        put8(control + 1U, static_cast<std::uint8_t>(character)); check(target() == run);
      }
      put8(control + 1U, 1U);
      for (auto state : {1U, 2U, 3U, 5U, 6U, 12U}) {
        put8(control + 0x568U, static_cast<std::uint8_t>(state)); check(target() == run);
      }
      put8(control + 0x568U, 0U);
      for (auto gate : {0x800FD7C4U, 0x801045B8U, 0x800F6DBCU, control + 0x5C0U}) {
        put32(gate, 1U); check(target() == run); put32(gate, 0U);
      }
      put8(0x800A4FC4U, 1U); check(target() == run); put8(0x800A4FC4U, 0U);
      put32(0x8004271CU, 0x3C028010U); put32(0x800FB084U, 0x8010B000U);
      check(target() == run); put32(0x800FB084U, 0U);
      for (auto invalid : {0U, 0x80000000U, 0x7FC00000U, 0x7F800000U,
                           0xBF800000U, 0x7F7FFFFFU})
        check(speed_mod.juno_run_target(m, player, control, invalid) == invalid);
      check(speed_mod.juno_run_target(m, 0U, control, run) == run);
      check(speed_mod.juno_run_target(m, player, control + 4U, run) == run);
      speed_mod.enabled = false; check(target() == run);
      put8(control + 1U, 0U);
    }
    NavigationMod mod;
    const auto unchanged = bytes;
    check(!mod.full_health(m, player) && bytes == unchanged);
    mod.telemetry_enabled = true;
    check(mod.observing() && mod.observe_player(m, player) && mod.player == player);
    check(!mod.full_health(m, player) && bytes == unchanged);
    std::ostringstream observer_state;
    mod.write_state(observer_state, m, {}, 0, false);
    check(bytes == unchanged && observer_state.str().find("\"navigation_enabled\":false") != std::string::npos &&
          observer_state.str().find("\"clearing_active\":false") != std::string::npos);
    mod.transition();check(mod.player == 0U && bytes == unchanged);
    check(!mod.observe_player(m, 0U));
    mod.telemetry_enabled = false;check(!mod.observe_player(m, player));
    mod.enabled = true;
    check(mod.full_health(m, player));
    check(m.u16(props + 6U) == 8704U && m.u16(game + 0x15CU) == 8704U &&
          m.u16(control + 0x5C8U) == 60U);
    check(health_capacity(0) == 1024U && health_capacity(-1) == 0U &&
          health_capacity(25) == 0U);
    put8(control + 1U, 2U);
    m.put16(game + 0x15CU + 2U * 0x76U + 2U, 8U);
    check(mod.full_health(m, player) && m.u16(props + 6U) == 11264U);
    const std::uint32_t enemy = 0x80104000U, ec = 0x80104100U, ep = 0x80104200U,
                        squad = 0x80104300U, sd = 0x80104400U;
    m.put16(enemy + 0x48U, 24U);
    put32(enemy + 0x68U, ec);
    put32(enemy + 0x4CU, ep);
    put32(ec + 0x24U, squad);
    m.put16(squad + 0x48U, 23U);
    put32(squad + 0x68U, sd);
    m.put16(sd + 0x2CU, 0x67U);
    m.put16(ep + 6U, 200U);
    {
      const auto snapshot = bytes;
      const auto state = enemy_state(m, Actor{enemy, 24U, {}});
      check(state.known && state.hostile && state.health == 200);
      std::ostringstream flags; write_enemy_state(flags, state);
      check(flags.str().find("\"hostile\":true") != std::string::npos);
      check(flags.str().find("\"health\":200") != std::string::npos);
      check(bytes == snapshot); // Telemetry must never modify game state.
      check(!enemy_state(m, Actor{enemy, 90U, {}}).known);
      check(!enemy_state(m, Actor{0x80400000U, 24U, {}}).known);
    }
    check(mod.clear_enemy(m, enemy) && m.u16(ep + 6U) == 0U &&
          mod.cleared == 1U);
    check(!mod.clear_enemy(m, enemy) && mod.cleared == 1U);
    check(enemy_state(m, Actor{enemy, 24U, {}}).health == 0);
    for (auto type : {0x11C, 0x11D, 0x11E, 0x11F, 0x120, 0x66, 0x70, 0x90, 0x97,
                      0x157, 0xA5}) {
      m.put16(sd + 0x2CU, static_cast<std::uint16_t>(type));
      m.put16(ep + 6U, 200U);
      check(!mod.clear_enemy(m, enemy) && m.u16(ep + 6U) == 200U);
      const auto friendly = enemy_state(m, Actor{enemy, 24U, {}});
      check(friendly.known && !friendly.hostile && friendly.health == 200);
    }
    put32(ec + 0x24U, 0xFFFFFFFFU);
    check(!enemy_state(m, Actor{enemy, 24U, {}}).known);
    check(!mod.clear_enemy(m, enemy));
    check(!mod.clear_enemy(m, player));
    put32(ec + 0x24U, squad);
    m.put16(sd + 0x2CU, 0x67U);
    m.put16(ep + 6U, 200U);
    put8(0x800A4FC4U, 1U);
    check(!mod.clear_enemy(m, enemy) && !mod.full_health(m, player) &&
          m.u16(ep + 6U) == 200U);
    put8(0x800A4FC4U, 0U);
    put8(0x800A329CU, 1U); put8(0x800FD7BDU, 1U);
    check(transition_confirmation(m));
    put8(0x800FD7BDU, 0U); check(!transition_confirmation(m));
    put8(0x800A329CU, 0U);
    mod.manual_inputs = 7;
    mod.transition();
    check(mod.manual_inputs == 7);
    check(!mod.clear_enemy(m, enemy) && m.u16(ep + 6U) == 200U);
    check(mod.full_health(m, player));
    for (const auto gate :
         {0x800FD7C4U, 0x801045B8U, 0x800F6DBCU, control + 0x5C0U}) {
      put32(gate, 1U);
      check(!mod.clear_enemy(m, enemy) && !mod.full_health(m, player) &&
            m.u16(ep + 6U) == 200U);
      put32(gate, 0U);
    }
    m.put16(enemy + 0x48U, 12U);
    check(!mod.clear_enemy(m, enemy) && m.u16(ep + 6U) == 200U);
    m.put16(enemy + 0x48U, 24U);
    check(mod.clear_enemy(m, enemy) && mod.cleared == 2U);
    mod.enabled = false;
    m.put16(ep + 6U, 200U);
    check(!mod.clear_enemy(m, enemy) && m.u16(ep + 6U) == 200U);
    mod.enabled = true;
    const std::uint32_t chest = 0x80109000U, chest_control = chest + 0x100U;
    put32(chest + 0x68U, chest_control);
    std::vector<Actor> chest_list{{chest, 98U, {10, 20, 30}}};
    auto items = item_markers(m, chest_list);
    check(items.size() == 1U && items[0].label == "Chest: Plasma shotgun" &&
          !items[0].opened);
    put8(chest_control + 4U, 5U);
    items = item_markers(m, chest_list);
    check(items[0].opened && items[0].kind == "opened");
    put8(chest_control + 5U, 255U);
    items = item_markers(m, chest_list);
    check(items[0].label.find("unidentified") != std::string::npos);
    put32(chest + 0x68U, 0xFFFFFFFFU);
    check(item_markers(m, chest_list).empty());
    const std::uint32_t key_header = chest + 0x200U;
    put32(chest + 0x40U, key_header);
    m.put16(chest + 0x4AU, 0x258U);
    put8(key_header + 4U, 'K');
    put8(key_header + 5U, 'e');
    put8(key_header + 6U, 'y');
    chest_list[0].behavior = 100U;
    check(item_markers(m, chest_list).size() == 1U &&
          item_markers(m, chest_list)[0].kind == "key");
    m.put16(chest + 0x4AU, 0x500U);
    check(item_markers(m, chest_list).empty());

    // NPC markers never mutate memory and never classify enemies as friendlies.
    const std::uint32_t guide = 0x80130000U, gc = guide + 0x100U,
                        gp = guide + 0x200U;
    put32(guide + 0x68U, gc);
    put32(guide + 0x4CU, gp);
    m.put16(guide + 0x4AU, 350U);
    m.put16(gp + 6U, 256U);
    std::vector<Actor> characters{{guide, 90U, {20, 30, 40}},
                                  {enemy, 24U, {100, 200, 300}}};
    m.put16(ep + 6U, 1024U);
    m.put16(sd + 0x2CU, 0x11CU);
    auto before_npcs = bytes;
    auto npcs = npc_markers(m, characters);
    check(npcs.size() == 2U && std::string(npcs[0].label) == "NPC: Guide" &&
          std::string(npcs[1].kind) == "tribal" &&
          npcs[1].squad_type == 0x11C && npcs[1].position.x == 100.0F &&
          bytes == before_npcs);
    for (auto type : {0x11C, 0x11D, 0x11E, 0x11F, 0x120, 0x66, 0x70, 0x90, 0x97,
                      0x157, 0xA5}) {
      m.put16(sd + 0x2CU, static_cast<std::uint16_t>(type));
      check(npc_markers(m, characters).size() == 2U);
    }
    m.put16(sd + 0x2CU, 45U);
    check(npc_markers(m, characters).size() == 1U); // Ordinary hostile.
    m.put16(sd + 0x2CU, 0x11CU);
    m.put16(ep + 6U, 0U);
    check(npc_markers(m, characters).size() == 1U); // Dead Tribal.
    m.put16(ep + 6U, 1024U);
    put32(ec + 0x24U, 0xFFFFFFFFU);
    check(!enemy_state(m, Actor{enemy, 24U, {}}).known);
    check(npc_markers(m, characters).size() == 1U);
    put32(ec + 0x24U, squad);
    characters[0].behavior = 51U;
    characters[1].behavior = 54U;
    check(npc_markers(m, characters).empty()); // Cutscene/scenery.
    characters[0].behavior = 90U;
    put32(guide + 0x68U, 0xFFFFFFFFU);
    check(npc_markers(m, characters).empty());
    put32(guide + 0x68U, gc);
    for (auto object :
         {350, 476, 486, 487, 488, 489, 490, 550, 551, 612, 772, 999}) {
      m.put16(guide + 0x4AU, static_cast<std::uint16_t>(object));
      check(npc_markers(m, characters).size() == 1U);
    }
    std::ostringstream npc_json;
    write_npcs(npc_json, npc_markers(m, characters));
    check(npc_json.str().find("\"kind\":\"npc\"") != std::string::npos);
    m.put16(gp + 6U, 0U);
    check(npc_markers(m, characters).empty());

    // The block named ForestCrate has no verified contents and is not an item.
    const char *block_name = "ForestCrate";
    for (unsigned n = 0; n < 16U; ++n)
      put8(key_header + 4U + n,
           n < 11U ? static_cast<std::uint8_t>(block_name[n]) : 0U);
    m.put16(chest + 0x4AU, 135U);
    chest_list[0].behavior = 54U;
    check(item_markers(m, chest_list).empty());

    const std::uint32_t track = 0x80110000U, block = track + 0x100U,
                        verts = track + 0x200U, faces = track + 0x300U,
                        batches = track + 0x400U, indices = track + 0x500U,
                        planes = track + 0x600U;
    put32(track + 4U, block);
    m.put16(track + 0x1AU, 1U);
    put32(block, verts);
    put32(block + 4U, faces);
    put32(block + 0xCU, batches);
    put32(block + 0x18U, indices);
    put32(block + 0x1CU, planes);
    m.put16(block + 0x24U, 3U);
    m.put16(block + 0x26U, 1U);
    m.put16(block + 0x28U, 1U);
    m.put16(verts + 10U, 100U);
    m.put16(verts + 24U, 100U);
    put8(faces + 1U, 0U);
    put8(faces + 2U, 1U);
    put8(faces + 3U, 2U);
    m.put16(batches + 24U, 1U);
    putfloat(planes + 4U, 1.0F);
    const auto mesh = decode_mesh(m, track);
    check(mesh.vertices.size() == 3U && mesh.triangles.size() == 1U);
    check(mesh.triangles[0].vertices[2] == 2U &&
          mesh.triangles[0].normal.y == 1.0F);
    put8(faces + 3U, 3U);
    bool rejected = false;
    try {
      (void)decode_mesh(m, track);
    } catch (const std::runtime_error &) {
      rejected = true;
    }
    check(rejected);
    put8(faces + 3U, 2U);
    put32(block + 0x1CU, 0xFFFFFFF0U);
    m.put16(indices, 1U);
    rejected = false;
    try {
      (void)decode_mesh(m, track);
    } catch (const std::runtime_error &) {
      rejected = true;
    }
    check(rejected);
    check(!m.valid(0U, 4U) && !m.valid(0x803FFFFFU, 2U) &&
          !m.valid(0xFFFFFFFFU, 4U));
    const std::uint32_t exit = 0x80120000U, xc = exit + 0x100U,
                        xd = exit + 0x200U;
    put32(exit + 0x68U, xc);
    put32(exit + 0x3CU, xd);
    put8(xd + 0xAU, 0x34U);
    put8(xd + 0xBU, 0x12U);
    put32(xc + 0x10U, 30U);
    m.put16(xc + 0x14U, 0xFFFFU);
    const auto doors = exits(m, {{exit, 8U, {1, 2, 3}}});
    check(doors.size() == 1U && doors[0].destination == 0x1234U &&
          doors[0].radius == 30U && doors[0].condition == -1);
    // The three map Mods work independently in ordinary telemetry sessions.
    {
      NavigationMod mods; mods.telemetry_enabled=true;
      check(mods.observe_player(m,player));
      auto configure=[&](const char *text){std::istringstream in(text);return mods.configure_mods(in);};
      m.put16(props+6U,123U);m.put16(ep+6U,200U);m.put16(sd+0x2CU,0x67U);
      check(configure("JFGMODS1 0 0 0"));
      auto unchanged_mods=bytes;
      check(!mods.full_health(m,player)&&!mods.clear_enemy(m,enemy)&&bytes==unchanged_mods);
      check(configure("JFGMODS1 0 1 0")&&mods.full_health(m,player));
      check(!mods.clear_enemy(m,enemy)&&m.u16(ep+6U)==200U);
      check(configure("JFGMODS1 0 0 1"));m.put16(props+6U,123U);
      check(!mods.full_health(m,player)&&m.u16(props+6U)==123U&&mods.clear_enemy(m,enemy));
      m.put16(ep+6U,200U);m.put16(sd+0x2CU,0x11CU);
      check(!mods.clear_enemy(m,enemy)&&m.u16(ep+6U)==200U);
      for(const auto text:{"JFGMODS1 1 1", "JFGMODS1 0 2 0", "JFGMODS1 0 0 0 extra", "BAD 0 0 0"})
        check(!configure(text)&&mods.kill_enabled()&&!mods.health_enabled());
      mods.enabled=true;check(configure("JFGMODS1 0 0 0"));
      check(!mods.full_health(m,player)&&!mods.clear_enemy(m,enemy));mods.enabled=false;
      // Match the supported guest's contact producer and transition consumer.
      put32(0x8002561CU,0x27BDFF70U);put32(0x800257F4U,0xACF000F4U);
      put32(0x800257F8U,0xA0EC0196U);put32(0x80032B94U,0x8E0200F4U);
      put32(0x80032DACU,0xAE0000F4U);m.put16(props+6U,512U);
      put8(control+0x197U,0x5AU);
      auto send=[&](unsigned nonce,unsigned gen,unsigned room,unsigned address,int stamp=1000){
        std::ostringstream text;text<<"JFGWARP1 "<<room<<' '<<gen<<' '<<nonce<<' '<<stamp<<' '<<address;
        std::istringstream in(text.str());
        return mods.warp_to_exit(in,m,{{player,1U,{}},{exit,8U,{101,202,303}},{enemy,24U,{}}},1000);
      };
      auto room=m.u32(0x800FB114U);unchanged_mods=bytes;
      check(!send(1,1,room,exit)&&bytes==unchanged_mods);
      check(configure("JFGMODS1 1 0 0"));
      check(!send(2,2,room,exit)&&!send(3,1,room+1,exit)&&!send(4,1,room,enemy)&&bytes==unchanged_mods);
      check(!send(5,1,room,exit,-1)&&!send(6,1,room,exit,1001)&&bytes==unchanged_mods);
      put8(0x800A4FC4U,1U);check(!send(7,1,room,exit));put8(0x800A4FC4U,0U);
      check(!send(7,1,room,exit));
      check(send(8,1,room,exit));
      check(m.f32(player+12U)==101&&m.f32(player+16U)==202&&m.f32(player+20U)==303);
      check(m.u32(control+0xF4U)==exit && m.u8(control+0x196U)==0x88U &&
            m.u8(control+0x197U)==0x5AU);
      check(!send(8,1,room,exit));
      check(configure("JFGMODS1 0 0 0"));unchanged_mods=bytes;
      check(!send(9,1,room,exit)&&bytes==unchanged_mods);
      check(configure("JFGMODS1 1 0 0"));
      // A new click cannot replace a pending transition.
      check(!send(10,1,room,exit)&&bytes==unchanged_mods);
      put32(control+0xF4U,0);put8(control+0x196U,0);
      put8(0x800FD7BDU,1);unchanged_mods=bytes;
      check(!send(11,1,room,exit)&&bytes==unchanged_mods);put8(0x800FD7BDU,0);
      m.put16(props+6U,255);unchanged_mods=bytes;
      check(!send(12,1,room,exit)&&bytes==unchanged_mods);m.put16(props+6U,512);
      put32(0x800257F4U,0);unchanged_mods=bytes;
      check(!send(13,1,room,exit)&&bytes==unchanged_mods);put32(0x800257F4U,0xACF000F4U);
      // Missing progression must not queue a contact or move the character.
      m.put16(xc+0x14U,7);
      put8(0x800A113CU,23);put8(0x800A113DU,1);put8(0x800A113EU,255);
      put8(game+0x32U,static_cast<std::uint8_t>(m.u8(game+0x32U)&0x7FU));
      unchanged_mods=bytes;
      check(!send(14,1,room,exit)&&bytes==unchanged_mods);
      put8(game+0x32U,static_cast<std::uint8_t>(m.u8(game+0x32U)|0x80U));
      check(send(15,1,room,exit)&&m.u32(control+0xF4U)==exit);
      put32(control+0xF4U,0);put8(control+0x196U,0);m.put16(xc+0x14U,0xFFFFU);
      mods.transition();unchanged_mods=bytes;
      check(!send(16,1,room,exit)&&bytes==unchanged_mods);
    }
    // Inventory and dialogue completion are separate, character-scoped facts.
    put8(control + 1U, 1U);
    const auto saved1 = game + 0x15CU + 0x76U;
    put8(saved1 + 0x66U, 0U);
    put8(game + 8U, 8U);
    m.put16(saved1 + 0xAU, 4U);
    m.put16(guide + 0x4AU, 350U);
    m.put16(gp + 6U, 256U);
    put8(gc + 0x31U, 4U);
    put8(gc + 0x32U, 4U);
    std::vector<Actor> progression_list{{player, 1U, {1, 2, 3}},
                                        {guide, 90U, {20, 30, 40}}};
    const auto before_progression = bytes;
    check(std::string(weapon_name(7))=="Sniper rifle"&&std::string(weapon_name(10))=="Fish Food");
    auto inv = inventory(m, player);
    check(inv.known && inv.character == 1U && !inv.red_key &&
          inv.magnus_spoken && inv.weapons == 4U);
    auto nodes =
        interactions(m, progression_list, 157U, inv, &npc_catalog, player);
    check(nodes.size() == 1U && nodes[0].reward_item == 1 &&
          nodes[0].status == "available" && nodes[0].spoken == 1);
    check(bytes == before_progression);
    put8(saved1 + 0x66U, 0x40U);
    nodes = interactions(m, progression_list, 157U, inventory(m, player),
                         &npc_catalog, player);
    check(nodes[0].status == "owned");
    put8(control + 1U, 0U);
    check(!inventory(m, player)
               .red_key); // Other character's inventory is untouched.
    put8(control + 1U, 3U);
    check(!inventory(m, player).known);
    put8(control + 1U, 1U);
    put32(0x800FD7D4U, 0xffffffffU);
    check(!inventory(m, player).known);
    nodes = interactions(m, progression_list, 157U, inventory(m, player),
                         &npc_catalog, player);
    check(nodes[0].status == "unknown" && nodes[0].spoken == -1);
    put32(0x800FD7D4U, game);
    put8(0x800A4FC4U, 1U);
    check(!inventory(m, player).known);
    put8(0x800A4FC4U, 0U);
    check(interactions(m, progression_list, 35U, inv, &npc_catalog, player)[0]
              .reward_item == 1);
    check(interactions(m, progression_list, 35U, inv)[0].status == "unknown");
    put8(gc + 0x32U, 5U);
    check(interactions(m, progression_list, 157U, inv, &npc_catalog, player)[0]
              .reward_item == 1);
    put8(gc + 0x32U, 4U);
    std::ostringstream progression_json;
    write_progression(progression_json, m, {{guide, 90U, {20, 30, 40}}}, player,
                      157U);
    check(progression_json.str().find("\"red_key\":null") !=
          std::string::npos); // Removed player cannot expose stale inventory.

    const std::uint32_t target = 0x80230000U, tc = target + 0x100U,
                        tp = target + 0x200U, door = target + 0x300U,
                        dc = target + 0x400U;
    put32(0x80025A08U,0x27BDFF48U);
    put32(target + 0x68U, tc);
    put32(target + 0x4CU, tp);
    m.put16(target + 0x48U, 111U);
    put32(door + 0x68U, dc);
    m.put16(door + 0x48U, 7U);
    put8(tc + 8U, 2U);
    put8(dc + 0x44U, 2U);
    put8(dc + 0x43U, 3U);
    m.put16(dc + 0x3EU, 8U);
    m.put16(tp + 6U, 2048U);
    m.put16(tc + 6U, 3072U);
    m.put16(tc, 124U);
    std::vector<Actor> target_list{{target, 111U, {3260, 404, -2755}},
                                   {door, 7U, {3324, 310, -2897}}};
    nodes = interactions(m, target_list, 157U, inv);
    check(nodes.size() == 2U && nodes[0].linked_actor == door &&
          nodes[0].door_id == 2);
    check(nodes[0].required_weapon == -1 && nodes[0].reset_ticks == 124 &&
          nodes[0].target_health == 2048);
    check(nodes[0].status == "needs_shooting" &&
          nodes[1].status == "key_missing" && nodes[1].required_item == 1);
    put8(tc + 0xAU, 1U);
    inv.red_key = true;
    nodes = interactions(m, target_list, 157U, inv);
    check(nodes[0].status == "activated" && nodes[1].status == "key_owned");
    m.put16(dc + 0x3EU, 0U);
    check(interactions(m, target_list, 157U, inv)[1].status ==
          "key_lock_cleared");
    put8(dc + 0x44U, 3U);
    check(interactions(m, target_list, 157U, inv)[0].linked_actor == 0U);

    // All door inventory IDs are character scoped; a latch already cleared
    // does not become locked merely because the character no longer has a key.
    put8(dc+0x43U,2);m.put16(dc+0x3EU,8);put8(saved1+0x66U,0);
    auto yellow=interactions(m,target_list,47U,inv,nullptr,player)[1];
    check(yellow.required_item==0&&yellow.label=="Yellow key door"&&yellow.status=="key_missing");
    put8(game+0x15CU+0x66U,0x80); // another character does not unlock Juno's door
    check(!interactions(m,target_list,47U,inv,nullptr,player)[1].access_allowed);
    put8(saved1+0x66U,0x80);
    check(interactions(m,target_list,47U,inv,nullptr,player)[1].status=="key_owned");
    put8(dc+0x43U,1);m.put16(dc+0x3EU,0x28);put8(dc+0x4AU,2);
    auto enemy_gate=interactions(m,target_list,47U,inv,nullptr,player)[1];
    check(enemy_gate.status=="enemies_remaining"&&enemy_gate.pending_openers==2&&!enemy_gate.access_allowed);
    m.put16(dc+0x3EU,0x20);put8(dc+0x4AU,0);
    check(interactions(m,target_list,47U,inv,nullptr,player)[1].status=="enemy_lock_cleared");
    m.put16(dc+0x3EU,0x48);
    check(interactions(m,target_list,47U,inv,nullptr,player)[1].status=="target_required");
    m.put16(dc+0x3EU,0);put8(dc+0x43U,0);put8(dc+0x42U,5);
    check(interactions(m,target_list,47U,inv,nullptr,player)[1].status=="opens_on_approach");
    const auto indicator=target+0x500U,ic=target+0x600U;
    put32(indicator+0x68U,ic);put32(ic+4U,door);
    auto linked=target_list;linked.push_back({indicator,67U,{3324,310,-2800}});
    auto observed=interactions(m,linked,47U,inv,nullptr,player);
    check(observed.back().action=="door_indicator"&&observed.back().linked_actor==door&&observed.back().status=="opens_on_approach");
    put8(dc+0x4BU,2);
    check(interactions(m,target_list,47U,inv,nullptr,player)[1].status=="character_or_progress_required");
    put8(dc+0x4BU,0);
    put32(0x8002561CU,0x27BDFF70U);
    for(unsigned condition:{6U,7U}) {
      put8(0x800A1120U+condition*4,23);
      put8(0x800A1121U+condition*4,static_cast<std::uint8_t>(condition==7));
      put8(0x800A1122U+condition*4,255);
    }
    put8(game+0x30U+(23>>3),0);
    auto variant=doors[0];variant.condition=6;
    check(exit_condition(m,variant,npc_facts(m,inv,player),player)==Fact::met);
    variant.condition=7;
    check(exit_condition(m,variant,npc_facts(m,inv,player),player)==Fact::missing);
    put8(game+0x30U+(23>>3),0x80);
    const auto decode_before=bytes;
    check(exit_condition(m,variant,npc_facts(m,inv,player),player)==Fact::met);
    check(exit_condition(m,variant,NpcFacts{},player)==Fact::unknown);
    (void)interactions(m,linked,47U,inv,nullptr,player);
    check(bytes==decode_before);
    put32(0x80025A08U,0);
    check(!interactions(m,target_list,47U,inv,nullptr,player)[1].access_known);

    put32(door + 0x68U, 0xffffffffU);
    check(interactions(m, target_list, 157U, inv).size() == 1U);
    put32(chest + 0x68U, chest_control);
    m.put16(chest + 0x48U, 98U);
    put8(chest_control + 5U, 2U);
    put8(chest_control + 4U, 0U);
    nodes = interactions(m, {{chest, 98U, {10, 20, 30}}}, 237U, inv);
    check(nodes[0].reward_weapon == 2 && nodes[0].status == "unopened");
    put8(chest_control + 4U, 5U);
    check(interactions(m, {{chest, 98U, {10, 20, 30}}}, 237U, inv)[0].status ==
          "opened");

    put32(0x800F2CA8U, 1025U);
    rejected = false;
    try {
      (void)actors(m);
    } catch (const std::runtime_error &) {
      rejected = true;
    }
    check(rejected);
  }
  {
    NavigationConfirmation confirm;
    auto send = [&](const char *text, bool prompt=true, bool manual=false, bool replay=false) {
      std::istringstream command(text);return confirm.command(command,prompt,manual,replay,27,13,7,1000);
    };
    check(send("JFGCONFIRM1 27 13 1 1000 7"));
    check(!send("JFGCONFIRM1 27 13 1 1000 7"));
    check(!send("JFGCONFIRM1 27 13 2 1000 7",false));
    check(!send("JFGCONFIRM1 27 13 2 1000 7"));
    check(!send("JFGCONFIRM1 27 13 3 1000 7",true,true));
    check(!send("JFGCONFIRM1 27 13 4 1000 7",true,false,true));
    check(!send("JFGCONFIRM1 28 13 5 1000 7"));
    check(!send("JFGCONFIRM1 27 12 5 1000 7"));
    check(!send("JFGCONFIRM1 27 13 5 -1 7"));
    check(!send("JFGCONFIRM1 27 13 5 1001 7"));
    check(!send("JFGCONFIRM1 27 13 5 1000 6"));
    check(!send("JFGCONFIRM1 27 13 5 1000 7 extra"));
    check(confirm.count==1);
  }
  std::cout << "navigation mod tests passed\n";
}
