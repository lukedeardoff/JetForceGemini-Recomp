#include "jfg/runtime/pc_camera_visibility.hpp"
#include "jfg/runtime/pc_input.hpp"
#include <cstdlib>
#include "jfg/runtime/window_input_mailbox.hpp"
#include <iostream>
#include <string>
namespace {
int checks=0;
void check(bool result,const char* what){++checks;if(!result){std::cerr<<what<<'\n';std::exit(1);}}
}
int main() {
  jfg::PcInputConfig config,roundtrip;
  check(!jfg::pc_experiments_enabled(""),"missing switch stays disabled");
  check(!jfg::pc_experiments_enabled("version=1\nexperimental=2\n"),"invalid opt-in stays disabled");
  check(!jfg::pc_experiments_enabled("version=1\nexperimental=1\nunknown=1\n"),"unknown opt-in rejected");
  check(jfg::pc_experiments_enabled(jfg::serialize_pc_experiments(true)),"explicit opt-in");
  check(!jfg::pc_experiments_enabled(jfg::serialize_pc_experiments(false)),"explicit opt-out");
  check(jfg::pc_experiments_enabled("version=1\r\nexperimental=1\r\n"),"CRLF opt-in");
  auto stored=jfg::pc_keyboard_preset();stored.dual_stick=1;stored.rumble_strength=75;
  auto gated=jfg::effective_pc_input(false,stored);
  check(gated.keys==stored.keys && !gated.mouse_aim && !gated.dual_stick && !gated.modern && !gated.rumble_strength,"opt-out retains bindings and disables modern behavior");
  check(jfg::effective_pc_input(true,stored).keys==stored.keys && stored.mouse_aim,"opt-in retains saved bindings");
  auto offMouse=jfg::map_pc_keyboard(gated,[](int k){return k==1 || k==2;});
  check((offMouse.buttons&0x2010)==0x2010,"mouse buttons remain bound with experiments off");
  gated.keys[14]='T';gated.keys[2]=jfg::kPcWheelUp;
  const auto offCustom=jfg::map_pc_keyboard(gated,[](int k){return k=='T' || k==jfg::kPcWheelUp;});
  check(offCustom.y==80 && (offCustom.buttons&0x2000),"custom movement and wheel work with experiments off");
  const auto text=jfg::serialize_pc_input(config);
  check(jfg::parse_pc_input(text,roundtrip)&&roundtrip.keys==config.keys,"default roundtrip");
  auto invalid=[&](std::string s){const auto before=jfg::serialize_pc_input(roundtrip);check(!jfg::parse_pc_input(s,roundtrip)&&jfg::serialize_pc_input(roundtrip)==before,"invalid config changed active settings");};
  invalid(text+"mouse_aim=1\n");invalid(text.substr(text.find('\n')+1));invalid(text+"unknown=2\n");
  auto replace=[&](std::string a,std::string b){auto t=text;t.replace(t.find(a),a.size(),b);invalid(t);};
  replace("key0=32","key0=258");replace("key0=32","key0=-1");replace("mouse_sensitivity=100","mouse_sensitivity=0");
  replace("aim_deadzone=7849","aim_deadzone=32767");replace("dual_stick=0","dual_stick=2");replace("key0=32","key0=32junk");
  replace("version=5","version=6");invalid(std::string(4097,'x'));
  std::array<bool,258> keys{};
  const auto sample=[&](){return jfg::map_pc_keyboard(config,[&](int k){return keys[static_cast<std::size_t>(k)];});};
  keys['W']=keys['D']=true;auto move=sample();check(move.x==80&&move.y==80,"movement directions");
  keys['S']=keys['A']=true;check(sample().x==0&&sample().y==0,"opposite movement cancellation");
  keys={};keys[32]=true;check(sample().buttons==0x8000,"Space compatibility");
  keys={};keys['Z']=true;check(sample().buttons==0x8000,"alternate A compatibility");
  config=jfg::pc_keyboard_preset();keys={};keys[1]=keys[2]=true;check((sample().buttons&0x2010)==0x2010,"mouse fire and aim together");
  keys={};keys[32]=true;check(sample().buttons==0x8000,"Normal jump preset");
  config=jfg::pc_keyboard_preset(true);check(sample().buttons==8,"Expert jump preset");
  for(const bool expert:{false,true}) {
    config=jfg::pc_keyboard_preset(expert);
    keys={};keys[jfg::kPcWheelUp]=true;
    check(sample().buttons==(expert?0x4000:0x8),"wheel up selects guest next weapon for selected scheme");
    keys={};keys[jfg::kPcWheelDown]=true;
    check(sample().buttons==(expert?0x8000:0x4),"wheel down selects guest previous weapon for selected scheme");
    keys={};keys[17]=true;
    check(sample().buttons==(expert?0x4:0x4000),"Ctrl uses scheme-specific crouch");
    keys={};keys['E']=true;
    check(sample().buttons==0x8000,"E retains guest interaction and menu action");
  }
  config.keys[0]=0;keys={};keys[0]=true;check(sample().buttons==0,"unbound key");
  jfg::PcWheelPulses wheel;wheel.add(60);check(wheel.sample()==0,"partial wheel notch");wheel.add(300);
  check(wheel.sample()==256&&wheel.sample()==0&&wheel.sample()==256&&wheel.sample()==0&&wheel.sample()==256&&wheel.sample()==0&&wheel.sample()==0,"wheel pulses retain releases");
  wheel.add(-240);check(wheel.sample()==257,"wheel down");wheel.clear();check(wheel.sample()==0,"wheel cleared on capture loss");
  jfg::PcMouseMotion mouse;config={};mouse.add(12,-9);auto aim=mouse.take(config);
  check(aim.x==12&&aim.y==9,"relative mouse axes");check(mouse.take(config)==jfg::N64StickSample{},"mouse delta consumed once");
  config.mouse_invert_y=1;config.mouse_aim_sensitivity=200;mouse.add(10,10);aim=mouse.take(config);check(aim.x==20&&aim.y==20,"mouse sensitivity and inversion");
  mouse.add(2147483647,-2147483647);aim=mouse.take(config);check(aim.x==127&&aim.y==-127,"bounded mouse spikes");
  mouse.add(20,20);mouse.clear();check(mouse.take(config)==jfg::N64StickSample{},"stale mouse cleared");
  config={};config.mouse_aim_sensitivity=10;int slow_total=0;
  for(int i=0;i<20;++i){mouse.add(1,0);slow_total+=mouse.take(config).x;}
  check(slow_total==2,"slow mouse displacement retained across updates");
  mouse.add(4096,0);(void)mouse.take(config);
  check(mouse.take(config)==jfg::N64StickSample{},"clipped mouse does not drift afterward");
  mouse.add(1,0);(void)mouse.take(config);mouse.clear();
  check(mouse.take(config)==jfg::N64StickSample{},"fractional motion cleared on disable");
  config.mouse_aim_sensitivity=50;mouse.add(1,-1);bool idle_stable=true;
  for(int i=0;i<10;++i)idle_stable &= mouse.take(config)==jfg::N64StickSample{};
  check(idle_stable,"half-pixel residual must not oscillate while idle");
  mouse.add(1,-1);check(mouse.take(config)==jfg::N64StickSample{1,1},"half-pixel residual accumulates on new motion");
  jfg::StandardControllerSample pad;pad.axes={32767,0,0,-32768,0,0};config={};
  aim=jfg::pc_aim_stick(config,pad,0);check(aim.x==0&&aim.y==80,"right aim independent of left movement");
  aim=jfg::pc_aim_stick(config,pad,1);check(aim.x==80&&aim.y==0,"swapped sticks remain independent");
  pad.axes={0,0,2000,-2000,0,0};check(jfg::pc_aim_stick(config,pad,0)==jfg::N64StickSample{},"aim stick drift deadzone");
  pad.axes={0,0,0,-32768,0,0};config.aim_invert_y=1;check(jfg::pc_aim_stick(config,pad,0).y==-80,"aim stick Y inversion");
  jfg::ControllerInputGate gate;std::array<jfg::ControllerPortSample,4> raw{};
  check(gate.blocked(true,raw),"settings capture");raw[0].buttons=1;check(gate.blocked(false,raw),"aim deflection prevents premature release");raw[0].buttons=0;check(!gate.blocked(false,raw),"neutral release");
  config=jfg::pc_keyboard_preset();check(config.modern==1,"modern keyboard preset");
  auto old=jfg::serialize_pc_input(config);
  for(const auto name:{"mouse_aim_sensitivity","camera_sensitivity","aim_curve","mouse_vertical_sensitivity","stick_vertical_sensitivity","mouse_invert_x","aim_invert_x","rumble_strength"}) {
    const auto at=old.find(std::string(name)+"=");old.erase(at,old.find('\n',at)-at+1);
  }
  old.replace(old.find("version=5"),9,"version=1");
  old.erase(old.find("modern=1\n"),9);
  check(jfg::parse_pc_input(old,roundtrip) && !roundtrip.modern,"v1 profile retains legacy aim");
  config={};mouse.add(10,-5);auto angular=mouse.take_angles(config);
  check(angular==jfg::PcLookDelta{180,90},"mouse displacement maps directly to angles");
  check(mouse.take_angles(config)==jfg::PcLookDelta{},"direct mouse motion consumed once");
  for(bool inverted:{false,true}) {
    jfg::PcInputConfig pitch_config;
    pitch_config.mouse_invert_y=inverted;
    jfg::PcMouseMotion pitch_mouse;pitch_mouse.add(3,-4);
    const auto look=pitch_mouse.take_angles(pitch_config);
    const auto standing=jfg::pc_direct_aim(0,0,look,-8192,8192);
    const auto guest=jfg::pc_direct_aim(0,0,jfg::pc_guest_aim_look(look,true),-8192,8192);
    check(standing.y==-guest.y && standing.x==guest.x && standing.y!=0,
      "modern crouch and aim camera pitch matches standing for both inversion settings");
    check(jfg::pc_guest_aim_look(look,false)==look,
      "legacy replay aim retains its original pitch convention");
  }
  auto direct=jfg::pc_direct_aim(-32760,0,{100,200},-100,100);
  check(direct.x==32676 && direct.y==-100,"direct aim wraps yaw and clamps pitch");
  check(jfg::pc_direct_aim(20,30,{},-100,100)==jfg::PcLookDelta{20,30},"stationary direct aim remains still");
  check(jfg::pc_shoulder_camera_yaw(-32768)==0,"forward-facing body matches the shoulder camera");
  check(jfg::pc_shoulder_camera_yaw(jfg::pc_shoulder_camera_yaw(12345))==12345,"camera and facing conventions round-trip");
  auto shoulder=jfg::pc_direct_aim(-32768,2000,{150,40},-8192,10922);
  check(jfg::pc_shoulder_camera_yaw(shoulder.x)==150 && shoulder.y==1960,"one look delta turns body and camera together");
  check(jfg::pc_direct_aim(shoulder.x,shoulder.y,{},-8192,10922)==shoulder,"released mouse leaves the shared view fixed");
  check(jfg::pc_shoulder_movement(80,0).side==4.0625F && jfg::pc_shoulder_movement(80,0).forward==0,"right movement strafes independently of facing");
  check(jfg::pc_shoulder_movement(0,-80).forward==4.0625F,"backwards movement retains the view direction");
  const auto shoulder_diagonal=jfg::pc_shoulder_movement(80,80);
  check(std::abs(std::hypot(shoulder_diagonal.side,shoulder_diagonal.forward)-4.0625F)<0.001F,"ground diagonal speed is normalized");
  check(jfg::pc_shoulder_movement(0,15).forward==-0.9375F,"native partial-stick speed is preserved");
  check(jfg::pc_shoulder_movement(0,55).forward==-3.4375F,"native sixty-unit host input after deadzone");
  check(jfg::pc_shoulder_movement(0,0).forward==0,"locked native axes cannot produce ground movement");
  for(unsigned character=0;character<3;++character) {
    check(jfg::pc_ground_animation(character,2,2,{0,4})==3,"running can reverse into native backward locomotion");
    check(jfg::pc_ground_animation(character,3,3,{0,-4})==2,"backward locomotion returns to native forward run");
    check(jfg::pc_ground_animation(character,3,3,{0,-1})==0,"slow reverse returns to walking");
    check(jfg::pc_ground_animation(character,15,15,{0,4})==15,"special animation requests remain native");
    check(jfg::pc_ground_animation(character,2,2,{0,0})==2,"idle transition remains native");
    check(jfg::pc_ground_animation(character,2,2,{NAN,1})==2,"invalid movement cannot choose an animation");
  }
  check(jfg::pc_ground_animation(1,47,9,{4,0})==10,"armed left sidestep reverses to right through native mapping");
  check(jfg::pc_ground_animation(0,43,10,{-4,0})==9,"armed right sidestep reverses to left through native mapping");
  check(jfg::pc_ground_animation(1,9,9,{0,4})==3,"sidestep can reverse directly into backward locomotion");
  check(jfg::pc_ground_animation(1,3,3,{-4,2})==9,"diagonal locomotion uses its dominant side");
  check(jfg::pc_ground_animation(2,9,9,{4,0})==9,"Lupus idle clip is not treated as a humanoid sidestep");
  check(jfg::pc_ground_animation(2,1,1,{4,0})==1,"Lupus retains his native lateral locomotion family");
  check(jfg::pc_ground_animation(3,3,3,{0,-4})==3,"unsupported characters remain native");
  const auto target=jfg::pc_shoulder_movement(0,80);
  const auto first=jfg::pc_original_acceleration({},target,2);
  check(std::abs(first.forward-(-0.396093845F))<0.0000001F,"original first two-tick acceleration matches native Juno sample");
  const auto coast=jfg::pc_original_acceleration(target,{},2);
  check(std::abs(coast.forward-(-3.66640615F))<0.0000001F,"releasing keys uses original ground braking");
  check(jfg::pc_original_acceleration({},target,0).forward==0,"zero game time does not accelerate");
  check(jfg::pc_original_acceleration({},target,2.9F).forward==first.forward,"native acceleration truncates fractional guest ticks");
  auto motion=jfg::PcMoveVelocity{};
  for(int n=0;n<150;++n)motion=jfg::pc_original_acceleration(motion,shoulder_diagonal,2);
  check(std::hypot(motion.side,motion.forward)<=4.062501F && std::hypot(motion.side,motion.forward)>4.06249F,"accelerated diagonal reaches native speed without exceeding cap");
  for(int n=0;n<150;++n)motion=jfg::pc_original_acceleration(motion,{},2);
  check(std::hypot(motion.side,motion.forward)<0.000001F,"released keys settle without persistent drift");
  check(std::abs(jfg::pc_original_acceleration({0,-8},{},2).forward)<=4.0625F,"special-state momentum cannot reintroduce excessive ground speed");
  // Environmental correction must stay on the user's viewing ray. Synthetic
  // obstacles exercise the contract independently of the game's implementation.
  const jfg::PcOrbitPoint pivot{0,40,0},low_view{0,-40,200};
  const auto floor=[](jfg::PcOrbitPoint p){p.y=std::max(p.y,16.0F);return p;};
  jfg::PcCameraBoom low_boom;
  const auto low_safe=low_boom.solve(pivot,low_view,floor(low_view),1.0F/30.0F,floor);
  check(low_safe.y>=16 && low_safe.z<200 && !low_boom.fallback,
    "low view contracts before ground instead of being pushed upward off the boom");
  check(std::abs((low_safe.y-40)*200+80*low_safe.z)<0.01F,
    "floor contact preserves the input-owned camera ray");
  const auto wall=[](jfg::PcOrbitPoint p){p.z=std::min(p.z,100.0F);return p;};
  const jfg::PcOrbitPoint flat_view{0,40,200};
  jfg::PcCameraBoom blocked;
  auto wall_safe=blocked.solve(pivot,flat_view,wall(flat_view),1.0F/30.0F,wall);
  check(wall_safe.z<=88.1F && wall_safe.z>87 && !blocked.fallback,
    "wall collision retains inward near-plane clearance");
  const auto open=[](jfg::PcOrbitPoint p){return p;};
  auto retreat=blocked.solve(pivot,flat_view,flat_view,1.0F/30.0F,open);
  check(retreat.z>wall_safe.z && retreat.z<200,"obstruction release returns gradually");
  auto new_wall=[](jfg::PcOrbitPoint p){p.z=std::min(p.z,50.0F);return p;};
  retreat=blocked.solve(pivot,flat_view,new_wall(flat_view),1.0F/30.0F,new_wall);
  check(retreat.z<=38.1F,"new obstruction contracts immediately without easing through a wall");
  const float contracted=blocked.distance;
  retreat=blocked.solve(pivot,flat_view,flat_view,1.0F/30.0F,open);
  check(std::abs(retreat.z-contracted)<0.001F,"short recovery hold prevents obstacle-edge chatter");
  auto recovery=[&](int hz) {
    jfg::PcCameraBoom boom;
    boom.solve(pivot,flat_view,flat_view,0,open);
    boom.solve(pivot,flat_view,wall(flat_view),0,wall);
    for(int n=0;n<hz;++n)boom.solve(pivot,flat_view,flat_view,1.0F/static_cast<float>(hz),open);
    return boom.distance;
  };
  check(std::abs(recovery(30)-recovery(60))<0.001F &&
        std::abs(recovery(60)-recovery(120))<0.001F,
    "collision recovery uses elapsed time consistently at 30, 60 and 120 updates");
  jfg::PcCameraBoom rotated;
  rotated.solve(pivot,flat_view,wall(flat_view),0,wall);
  const jfg::PcOrbitPoint turned_view{200,40,0};
  auto turned_camera=rotated.solve(pivot,turned_view,turned_view,1.0F/30.0F,open);
  check(turned_camera.x>0 && std::abs(turned_camera.z)<0.001F,
    "mouse yaw changes immediately while boom distance recovers");
  const auto blocked_pivot=[](jfg::PcOrbitPoint p){p.y+=50;return p;};
  jfg::PcCameraBoom impossible;
  auto fallback=impossible.solve(pivot,flat_view,blocked_pivot(flat_view),0,blocked_pivot);
  check(impossible.fallback && fallback.y==90,
    "no feasible pivot retains native correction rather than forcing geometry penetration");
  // A short recovered distance can intersect terrain even when a farther
  // endpoint is clear. It must be tested on the new frame, not assumed safe.
  jfg::PcCameraBoom uneven;uneven.distance=60;
  const auto ridge=[](jfg::PcOrbitPoint p){if(p.z>45 && p.z<100)p.y=80;return p;};
  auto over_ridge=uneven.solve(pivot,flat_view,flat_view,1.0F/120.0F,ridge);
  check(ridge(over_ridge).y==over_ridge.y && over_ridge.z<=45,
    "recovery cannot interpolate through a previously unseen ridge");
  // Narrow ray tangents must remain finite, including an almost vertical
  // view, zero simulation time and a floor whose height changes along the ray.
  const jfg::PcOrbitPoint steep{0,-160,0.001F};
  jfg::PcCameraBoom steep_boom;
  const auto steep_safe=steep_boom.solve(pivot,steep,floor(steep),0,floor);
  check(steep_safe.y>=16 && std::isfinite(steep_safe.z) && !steep_boom.fallback,
    "near-vertical floor collision remains finite and safe");
  const auto slope=[](jfg::PcOrbitPoint p){p.y=std::max(p.y,16.0F+0.15F*p.z);return p;};
  jfg::PcCameraBoom slope_boom;
  const auto slope_safe=slope_boom.solve(pivot,low_view,slope(low_view),1.0F/30.0F,slope);
  check(std::abs(slope_safe.y-slope(slope_safe).y)<=0.01F && !slope_boom.fallback &&
        slope_safe.z>43.0F && slope_safe.z<43.8F && slope_boom.queries<=12,
    "sloped floor finds the clearance boundary instead of collapsing near the pivot");
  // A shallow raised lip at the camera endpoint should require a small
  // contraction. A vertical-only correction gives no distance bound when the
  // desired view is horizontal; halving the boom discards valid clear space.
  const auto shallow_lip=[](jfg::PcOrbitPoint p) {
    if(p.z>199.0F)p.y=40.0F+0.2F*(p.z-199.0F);
    return p;
  };
  jfg::PcCameraBoom lip_boom;
  const auto lip_safe=lip_boom.solve(pivot,flat_view,shallow_lip(flat_view),0,shallow_lip);
  check(lip_safe.z>198.0F && lip_safe.z<199.1F && !lip_boom.fallback,
    "small floor clearance contact cannot halve an otherwise clear camera boom");
  check(std::abs(lip_safe.y-shallow_lip(lip_safe).y)<=0.01F,
    "closest clearance candidate is still validated against the obstacle");
  float previous_lip=lip_safe.z;
  for(unsigned step=1;step<=20;++step) {
    const float edge=199.0F-float(step)*0.1F;
    const auto moving_lip=[&](jfg::PcOrbitPoint p) {
      if(p.z>edge)p.y=40.0F+0.2F*(p.z-edge);
      return p;
    };
    const auto point=lip_boom.solve(pivot,flat_view,moving_lip(flat_view),
      1.0F/30.0F,moving_lip);
    check(std::abs(point.y-moving_lip(point).y)<=0.01F &&
          point.z>edge-0.6F && previous_lip-point.z<0.7F,
      "a gradually approaching lip cannot create a large inward distance step");
    previous_lip=point.z;
  }
  const jfg::PcOrbitPoint rim_a{-114,21,521},rim_b{-183,21,370},rim_c{-139,21,535};
  check(!jfg::pc_camera_sphere_triangle_possible({-141.3532F,45.4745F,510.6572F},
      16,rim_a,rim_b,rim_c),
    "Goldwood camera above the finite pond rim has no 16-unit volume contact");
  check(jfg::pc_camera_sphere_triangle_possible({-141.3532F,35,510.6572F},
      16,rim_a,rim_b,rim_c),"real pond-rim camera volume contact is retained");
  check(!jfg::pc_camera_sphere_triangle_possible({20,0,20},1,{0,0,0},{10,0,0},{0,0,10}),
    "a coplanar point outside a finite triangle does not collide with its infinite plane");
  check(jfg::pc_camera_sphere_triangle_possible({5,0,-0.5F},1,{0,0,0},{10,0,0},{0,0,10}),
    "sphere contacts with a triangle edge are retained");
  check(jfg::pc_camera_sphere_triangle_possible({-0.5F,0,-0.5F},1,{0,0,0},{10,0,0},{0,0,10}),
    "sphere contacts with a triangle corner are retained");
  check(jfg::pc_camera_sphere_triangle_possible({0,0,0},16,{0,0,0},{0,0,0},{0,0,0}),
    "degenerate clearance geometry keeps the native result");
  check(!jfg::pc_camera_sphere_triangle_possible({1000,0,0},16,
      {0,0,0},{0,0,0},{0,0,0}),
    "a distant degenerate face cannot invalidate otherwise complete volume clearance");
  const jfg::mod::Memory unknown_camera_ram({});
  const jfg::PcCameraBroadphase unknown_camera_scene(unknown_camera_ram);
  check(unknown_camera_scene.volume_possible({0,100,0},16),
    "unknown scenery or collision registry cannot certify camera clearance");
  // A paused input stream does not end view ownership; actor/control lifetime
  // does. Entry facing comes from the player even if the old camera faces back.
  jfg::PcShoulder persistent_view;
  check(persistent_view.acquire(0x80100000,0x80101000,12000,3000),"fresh native entrance acquired");
  persistent_view.yaw=16000;persistent_view.pitch=5000;persistent_view.retrace=10;
  check(!persistent_view.acquire(0x80100000,0x80101000,-16000,-4000) &&
    persistent_view.yaw==16000 && persistent_view.pitch==5000,"focus gap retains yaw and pitch instead of reseeding from camera");
  persistent_view={};
  check(persistent_view.acquire(0x80100000,0x80101000,-12000,1000) && persistent_view.yaw==-12000,
    "same-address area reload acquires new native facing");
  check(persistent_view.acquire(0x80100000,0x80102000,7000,0) && persistent_view.yaw==7000,
    "new control object starts a new view lifetime");
  for(const auto velocity:{jfg::PcMoveVelocity{0,-4},jfg::PcMoveVelocity{0,4},
       jfg::PcMoveVelocity{4,0},jfg::PcMoveVelocity{-4,0},jfg::PcMoveVelocity{2.5F,-3}}) {
    for(int heading:{0,13000,-29000})for(bool backward:{false,true}) {
      const auto air=jfg::pc_native_airborne_motion(velocity,heading,backward);
      const auto restored=jfg::pc_velocity_in_view({0,air.forward},air.yaw,heading);
      check(std::abs(restored.side-velocity.side)<0.001F && std::abs(restored.forward-velocity.forward)<0.001F,
        "airborne polar conversion preserves full world momentum in every direction");
      const auto turned=jfg::pc_velocity_in_view({0,air.forward},air.yaw,jfg::pc_wrap_angle(heading+16384));
      check(std::abs(turned.side+velocity.forward)<0.001F && std::abs(turned.forward-velocity.side)<0.001F,
        "camera quarter-turn changes the view basis without rotating airborne momentum");
    }
  }
  check(!jfg::pc_shoulder_state(4) && !jfg::pc_shoulder_state(7) && jfg::pc_shoulder_state(11),"swimming and damage states retain guest control");
  const auto diagonal=jfg::pc_aim_movement(80,80);
  check(std::abs(diagonal.side/2.5F*diagonal.side/2.5F+diagonal.forward/3.0F*diagonal.forward/3.0F-1.0F)<0.001F,"aim diagonal stays normalized");
  check(jfg::pc_aim_movement(0,0).side==0 && jfg::pc_aim_movement(0,0).forward==0,"neutral aim movement");
  const auto clear=jfg::pc_camera_clearance({0,0,0},{0,0,100},{0,0,50});
  check(clear.x==0 && clear.y==0 && clear.z==38,"wall hit retreats toward focus for camera clearance");
  check(jfg::pc_camera_clearance({0,0,0},{0,0,100},{0,0,100}).z==100,"unblocked camera is unchanged");
  check(jfg::pc_camera_clearance({0,0,0},{0,0,100},{0,0,20}).z==16,"camera clearance keeps minimum focus distance");
  const auto floor_clear=jfg::pc_camera_clearance({0,0,0},{0,0,100},{0,16,100});
  check(floor_clear.y==16 && floor_clear.z==100,"floor correction is not pulled back into terrain");
  const auto orbit=jfg::pc_orbit_point(16384,0,200);
  check(std::abs(orbit.x-200)<0.001F && std::abs(orbit.y)<0.001F && std::abs(orbit.z)<0.001F,"quarter orbit preserves radius");
  check(jfg::pc_camera_delta(3,{15,20},5)==jfg::PcLookDelta{15,20},"mouse orbit is displacement independent of guest ticks");
  check(jfg::pc_camera_delta(4,{80,-80},2)==jfg::PcLookDelta{512,-512},"stick orbit is a rate");
  check(jfg::pc_camera_delta(4,{80,80},0)==jfg::PcLookDelta{},"paused stick orbit is still");
  keys={};keys[13]=true;keys[39]=true;
  auto menu=jfg::map_pc_menu(jfg::ControllerPortSample{},[&](int key){return keys[static_cast<std::size_t>(key)];});
  check(menu.buttons==0x8000 && menu.x==80,"Enter and arrows work independently of gameplay preset");
  keys={};keys[8]=true;keys[38]=true;keys[40]=true;
  menu=jfg::map_pc_menu(jfg::ControllerPortSample{},[&](int key){return keys[static_cast<std::size_t>(key)];});
  check(menu.buttons==0x4000 && menu.y==0,"Backspace and opposing menu arrows");
  config={};config.mouse_sensitivity=200;config.mouse_aim_sensitivity=50;
  mouse.clear();mouse.add(10,0);check(mouse.take_angles(config,false).x==360,"camera mouse sensitivity independent");
  mouse.add(10,0);check(mouse.take_angles(config,true).x==90,"aim mouse sensitivity independent");
  config.mouse_vertical_sensitivity=50;mouse.add(0,-10);
  check(mouse.take_angles(config,true).y==45,"mouse vertical scale");
  config={};config.aim_deadzone=0;config.aim_sensitivity=50;config.camera_sensitivity=150;
  pad.axes={0,0,32767,0,0,0};
  check(jfg::pc_aim_stick(config,pad,0,true).x==40,"stick aim sensitivity independent");
  check(jfg::pc_aim_stick(config,pad,0,false).x==120,"stick camera sensitivity independent");
  config.aim_sensitivity=100;config.aim_curve=1;pad.axes[2]=16384;
  const auto precise=jfg::pc_aim_stick(config,pad,0);config.aim_curve=0;
  const auto linear=jfg::pc_aim_stick(config,pad,0);
  check(precise.x>0&&precise.x<linear.x,"precision curve retains analog magnitude near center");
  config.aim_curve=2;check(jfg::pc_aim_stick(config,pad,0).x<precise.x,"extra precision curve");
  pad.axes[2]=32767;check(jfg::pc_aim_stick(config,pad,0).x==80,"curves retain full stick travel");
  pad.axes[2]=0;check(jfg::pc_aim_stick(config,pad,0)==jfg::N64StickSample{},"curve neutral stays neutral");
  config={};config.mouse_sensitivity=175;config.aim_sensitivity=125;
  auto v2=jfg::serialize_pc_input(config);
  for(const auto name:{"mouse_aim_sensitivity","camera_sensitivity","aim_curve","mouse_vertical_sensitivity","stick_vertical_sensitivity","mouse_invert_x","aim_invert_x","rumble_strength"}) {
    const auto at=v2.find(std::string(name)+"=");v2.erase(at,v2.find('\n',at)-at+1);
  }
  v2.replace(v2.find("version=5"),9,"version=2");
  check(jfg::parse_pc_input(v2,roundtrip)&&roundtrip.mouse_aim_sensitivity==175&&roundtrip.camera_sensitivity==125&&roundtrip.aim_curve==0,"v2 migration preserves both existing look speeds");
  invalid(v2+"camera_sensitivity=100\n");
  config.aim_curve=2;config.mouse_aim_sensitivity=50;config.camera_sensitivity=150;config.mouse_vertical_sensitivity=80;config.stick_vertical_sensitivity=120;
  check(jfg::parse_pc_input(jfg::serialize_pc_input(config),roundtrip)&&jfg::serialize_pc_input(roundtrip)==jfg::serialize_pc_input(config),"all v3 settings roundtrip");
  replace("aim_curve=0","aim_curve=3");replace("camera_sensitivity=100","camera_sensitivity=301");
  replace("mouse_vertical_sensitivity=100","mouse_vertical_sensitivity=0");
  jfg::ControllerMapping guestMapping;
  jfg::StandardControllerSample menuPad;menuPad.axes[2]=32767;
  jfg::PcInputConfig dual;dual.dual_stick=dual.modern=1;
  check(jfg::map_pc_controller(guestMapping,menuPad,dual,false).buttons==jfg::map_controller(guestMapping,menuPad).buttons && jfg::map_controller(guestMapping,menuPad).buttons!=0,"unqualified menus and modes preserve original right-stick bindings");
  check(jfg::map_pc_controller(guestMapping,menuPad,dual,true).buttons==0,"qualified modern look does not also press C buttons");
  dual.modern=0;
  check(jfg::map_pc_controller(guestMapping,menuPad,dual,true).buttons!=0,"legacy aim mode preserves right-stick buttons outside aiming");
  menuPad.buttons[10]=true;
  check(jfg::map_pc_controller(guestMapping,menuPad,dual,true).buttons==0x10,"legacy aiming consumes look stick only while aiming");
  dual.dual_stick=0;
  check(jfg::map_pc_controller(guestMapping,menuPad,dual,true).buttons==jfg::map_controller(guestMapping,menuPad).buttons,"disabled dual stick retains original mapping");
  // Modern stick range is independent of the N64 stick and preserves all
  // user sensitivities, including diagonal direction and precision curves.
  config={};config.aim_deadzone=0;pad={};pad.axes[2]=32767;
  check(jfg::pc_stick_look(config,pad,0)==jfg::PcLookDelta{2048,0},"modern full travel");
  config.aim_sensitivity=200;check(jfg::pc_stick_look(config,pad,0).x==4096,"200 percent does not saturate at legacy limit");
  config.aim_sensitivity=300;check(jfg::pc_stick_look(config,pad,0).x==6144,"300 percent remains distinct from 200 percent");
  config.camera_sensitivity=50;check(jfg::pc_stick_look(config,pad,0,false).x==1024,"camera rate remains independent");
  pad.axes[2]=0;pad.axes[3]=-32768;config.stick_vertical_sensitivity=300;
  check(jfg::pc_stick_look(config,pad,0).y==18432,"maximum vertical scaling fits precise recording range");
  config={};config.aim_deadzone=0;pad.axes[2]=32767;pad.axes[3]=-32767;
  auto radial=jfg::pc_stick_look(config,pad,0);
  check(std::abs(std::hypot(double(radial.x),double(radial.y))-2048.0)<1.0,"diagonal look has cardinal maximum rate");
  pad.axes[2]=0;pad.axes[3]=-32768;auto positive=jfg::pc_stick_look(config,pad,0);
  pad.axes[3]=32767;check(jfg::pc_stick_look(config,pad,0).y==-positive.y,"asymmetric XInput endpoint is symmetric after normalization");
  config.aim_deadzone=7849;pad.axes={0,0,7849,0,0,0};
  check(jfg::pc_stick_look(config,pad,0)==jfg::PcLookDelta{},"modern deadzone boundary is neutral");
  pad.axes[2]=9000;check(jfg::pc_stick_look(config,pad,0).x>0,"travel starts above modern deadzone");
  config.aim_deadzone=0;config.aim_curve=2;pad.axes[2]=3500;
  check(jfg::pc_stick_look(config,pad,0).x>0 && jfg::pc_aim_stick(config,pad,0).x==0,"precision look retains input lost by 8-bit shaping");
  pad.axes={32767,0,0,0,0,0};config={};config.aim_invert_x=1;
  check(jfg::pc_stick_look(config,pad,1).x==-2048,"swapped look stick and X inversion");
  config.aim_invert_y=1;pad.axes[1]=-32767;
  check(jfg::pc_stick_look(config,pad,1).y<0,"Y inversion independent of X");
  jfg::PcStickMotion integrated;int slow_turn=0;
  for(int i=0;i<16;++i)slow_turn+=integrated.take({1,-1},1.0F).x;
  check(slow_turn==2,"fractional stick rate survives repeated updates");
  jfg::PcStickMotion foreground_recording,hidden_recording;
  int foreground_turn=0,hidden_turn=0;
  for(int i=0;i<16;++i) {
    if(jfg::pc_input_motion_blocked(false,true,false))foreground_recording.clear();
    if(jfg::pc_input_motion_blocked(true,true,false))hidden_recording.clear();
    foreground_turn+=foreground_recording.take({1,-1},1.0F,100).x;
    hidden_turn+=hidden_recording.take({1,-1},1.0F,100).x;
  }
  check(hidden_turn==2 && hidden_turn==foreground_turn,"recorded slow stick motion survives inactive-window replay");
  check(jfg::pc_input_motion_blocked(true,false,false),"live focus loss still resets input fractions");
  check(jfg::pc_input_motion_blocked(false,true,true),"explicit replay suspension still resets input fractions");
  check(!jfg::pc_input_motion_blocked(false,false,false),"focused live input retains fractions");

  integrated.clear();auto one_update=integrated.take({257,-100},4.0F);
  integrated.clear();jfg::PcLookDelta many_updates;
  for(int i=0;i<4;++i){auto delta=integrated.take({257,-100},1.0F);many_updates.x+=delta.x;many_updates.y+=delta.y;}
  check(many_updates==one_update,"stick rotation agrees across guest update partitions");
  integrated.clear();(void)integrated.take({7,7},1.0F,100);
  check(integrated.take({},1.0F,100)==jfg::PcLookDelta{},"neutral stick clears fractions without drifting");
  (void)integrated.take({7,7},1.0F,100);
  check(integrated.take({1,1},1.0F,101)==jfg::PcLookDelta{},"camera-to-aim transition discards stale fractional motion");
  check(integrated.take({2048,2048},0.0F)==jfg::PcLookDelta{},"paused precise stick has no motion");
  config={};config.mouse_invert_x=1;mouse.clear();mouse.add(10,-10);
  check(mouse.take_angles(config)==jfg::PcLookDelta{-180,180},"mouse horizontal inversion does not flip vertical");
  config.mouse_aim_sensitivity=10;mouse.clear();int inverted_total=0;
  for(int i=0;i<20;++i){mouse.add(1,0);inverted_total+=mouse.take(config).x;}
  check(inverted_total==-2,"inverted legacy mouse retains fractional displacement");
  auto v3=jfg::serialize_pc_input(config);
  for(const auto name:{"mouse_invert_x","aim_invert_x","rumble_strength"}){const auto at=v3.find(std::string(name)+"=");v3.erase(at,v3.find('\n',at)-at+1);}
  v3.replace(v3.find("version=5"),9,"version=3");
  check(jfg::parse_pc_input(v3,roundtrip)&&!roundtrip.mouse_invert_x&&!roundtrip.aim_invert_x&&roundtrip.mouse_aim_sensitivity==10,"v3 migrates horizontal inversion defaults without changing tuning");
  invalid(v3+"mouse_invert_x=1\n");replace("mouse_invert_x=0","mouse_invert_x=2");
  config.aim_invert_x=1;check(jfg::parse_pc_input(jfg::serialize_pc_input(config),roundtrip)&&roundtrip.aim_invert_x&&roundtrip.mouse_invert_x,"v4 inversion persists");
  guestMapping.bindings[9]=19;menuPad={};menuPad.axes[2]=32767;dual.dual_stick=dual.modern=1;
  check(jfg::map_pc_controller(guestMapping,menuPad,dual,true).buttons==0x10,"custom aim axis survives acquiring look ownership");
  auto v4=jfg::serialize_pc_input(config);
  const auto rumble_at=v4.find("rumble_strength=");v4.erase(rumble_at,v4.find('\n',rumble_at)-rumble_at+1);
  v4.replace(v4.find("version=5"),9,"version=4");
  check(jfg::parse_pc_input(v4,roundtrip)&&roundtrip.rumble_strength==0&&roundtrip.aim_invert_x==config.aim_invert_x,"v4 migration preserves controls and leaves rumble off");
  invalid(v4+"rumble_strength=100\n");replace("rumble_strength=0","rumble_strength=101");replace("rumble_strength=0","rumble_strength=-1");
  config.rumble_strength=75;
  check(jfg::parse_pc_input(jfg::serialize_pc_input(config),roundtrip)&&roundtrip.rumble_strength==75,"v5 rumble preference persists");
  jfg::PcDeviceReleaseGate reconnect;
  jfg::ControllerPortSample hardwareSample{0x2000,0,0,true};
  check(reconnect.blocked(0,hardwareSample,{}),"initial held fire waits for release");
  hardwareSample.buttons=0;check(!reconnect.blocked(0,hardwareSample,{}),"neutral device becomes available");
  hardwareSample.x=60;check(!reconnect.blocked(0,hardwareSample,{}),"normal movement remains active after release");
  hardwareSample.connected=false;check(reconnect.blocked(0,hardwareSample,{}),"disconnect is blocked");
  hardwareSample.connected=true;hardwareSample.x=0;
  check(reconnect.blocked(0,hardwareSample,{40,0}),"reconnected look deflection waits for release");
  check(!reconnect.blocked(0,hardwareSample,{}),"reconnected neutral resumes");
  hardwareSample.buttons=0x10;check(reconnect.blocked(1,hardwareSample,{}),"switching devices requires neutral");
  hardwareSample.buttons=0;check(!reconnect.blocked(-3,hardwareSample,{}),"neutral keyboard assignment works");
  // Exercise real mailbox/gate objects together across focus loss between
  // polls, including a newly held focus click and a queued wheel/mouse delta.
  jfg::WindowInputMailbox mailbox;std::array<bool,256> held{},pressed{};bool closed=false;
  mailbox.key(1,true);mailbox.motion(300,100);mailbox.wheel(120);mailbox.lose_focus();
  mailbox.key(1,true);const auto focusMotion=mailbox.consume_frame(held,pressed,closed);
  check(focusMotion.focus_lost&&!focusMotion.x&&!focusMotion.y&&!focusMotion.wheel,"focus transition discards queued mouse and wheel");
  check(held[1] && pressed[1],"focus snapshot includes the newly held activation click");
  mailbox.motion(4,5);mailbox.wheel(120);
  auto nextMotion=mailbox.consume_frame(held,pressed,closed);
  check(!nextMotion.focus_lost && nextMotion.x==4 && nextMotion.y==5 && nextMotion.wheel==120,
    "next input snapshot preserves new events without repeating focus loss");
  jfg::ControllerInputGate focusGate;std::array<jfg::ControllerPortSample,4> resumed{};
  if(focusMotion.focus_lost)(void)focusGate.blocked(true,resumed);
  resumed[0]=jfg::map_pc_keyboard(jfg::pc_keyboard_preset(),[&](int key){return key>=0&&key<256&&(held[static_cast<std::size_t>(key)]||pressed[static_cast<std::size_t>(key)]);});
  check(focusGate.blocked(false,resumed),"click which restores focus cannot fire");
  mailbox.key(1,false);pressed.fill(false);mailbox.consume(held,pressed,closed);
  resumed[0]=jfg::map_pc_keyboard(jfg::pc_keyboard_preset(),[&](int key){return key>=0&&key<256&&(held[static_cast<std::size_t>(key)]||pressed[static_cast<std::size_t>(key)]);});
  check(!focusGate.blocked(false,resumed),"focus click release re-arms keyboard and mouse");
  mailbox.key(1,true);mailbox.consume(held,pressed,closed);
  resumed[0]=jfg::map_pc_keyboard(jfg::pc_keyboard_preset(),[&](int key){return key>=0&&key<256&&(held[static_cast<std::size_t>(key)]||pressed[static_cast<std::size_t>(key)]);});
  check(!focusGate.blocked(false,resumed)&&(resumed[0].buttons&0x2000),"fresh click after focus release fires normally");
  jfg::WindowInputMailbox repeatBox;pressed.fill(false);
  repeatBox.key('W',true);repeatBox.key('W',true,true);repeatBox.consume_frame(held,pressed,closed);
  check(held['W']&&pressed['W'],"normal repeat retains a genuinely held key");
  repeatBox.lose_focus();repeatBox.consume_frame(held,pressed,closed);
  repeatBox.key('W',true,true);repeatBox.consume_frame(held,pressed,closed);
  check(!held['W']&&!pressed['W'],"held keyboard repeat after focus loss cannot resume movement");
  repeatBox.key('C',true,true);repeatBox.consume_frame(held,pressed,closed);
  check(!held['C']&&!pressed['C'],"key first held in another window cannot acquire input by repeat");
  repeatBox.key('D',true);repeatBox.consume_frame(held,pressed,closed);
  check(held['D']&&pressed['D'],"fresh unrelated key after focus return works");
  repeatBox.key('W',false);repeatBox.key('W',true);repeatBox.consume_frame(held,pressed,closed);
  check(held['W']&&pressed['W'],"released keyboard key works on a fresh press");
  const auto bodyProbes=jfg::pc_camera_body_probes({0,8,0},{0,40,100});
  check(bodyProbes[0].x==0 && bodyProbes[1].x==20 && bodyProbes[2].x==-20 && bodyProbes[1].y==8,"body probes cover both sides of the visible player");
  const auto sideProbes=jfg::pc_camera_body_probes({0,8,0},{100,40,0});
  check(sideProbes[1].x==0 && sideProbes[1].z==-20 && sideProbes[2].z==20,"body probe width rotates with camera direction");
  const auto verticalProbes=jfg::pc_camera_body_probes({0,8,0},{0,40,0});
  check(std::isfinite(verticalProbes[1].x) && verticalProbes[1].x==20,"vertical camera retains finite body probe width");
  const auto bodyClear=jfg::pc_camera_body_clearance({0,40,0},{100,40,0},{0,20,0},{50,30,0});
  check(bodyClear.x>16 && bodyClear.x<50 && bodyClear.y==40,"raw body contact shortens orbit with near-plane clearance");
  check(jfg::pc_camera_body_clearance({0,40,0},{100,40,0},{0,20,0},{100,50,0}).x==100,"off-ray floor correction cannot collapse camera");
  check(jfg::pc_camera_body_clearance({0,40,0},{100,40,0},{0,20,0},{100,40,0}).x==100,"body contact beyond ray preserves camera");
  check(jfg::pc_camera_body_clearance({0,40,0},{100,40,0},{0,20,0},{-10,18,0}).x==100,"body contact behind player preserves camera");
  check(jfg::pc_camera_body_clearance({0,40,0},{100,40,0},{0,20,0},{0,20,0}).x==16,"body contact retains minimum focus distance");
  check(jfg::pc_camera_body_clearance({0,40,0},{100,40,0},{0,20,0},{50,30,std::numeric_limits<float>::quiet_NaN()}).x==100,"nonfinite body contact preserves camera");
  check(jfg::pc_camera_body_clearance({0,40,0},{100,40,0},{100,40,0},{100,40,0}).x==100,"zero-length body ray preserves camera");
  const jfg::PcOrbitPoint ta{-10,-10,10},tb{10,-10,10},tc{0,10,10};
  check(jfg::pc_camera_triangle_possible({0,0,0},{0,0,20},ta,tb,tc),"camera prefilter retains triangle contact");
  check(jfg::pc_camera_triangle_possible({0,0,20},{0,0,0},ta,tb,tc),"camera prefilter retains reverse-face contact");
  check(!jfg::pc_camera_triangle_possible({30,0,0},{30,0,20},ta,tb,tc),"camera prefilter rejects an unobstructed ray");
  check(!jfg::pc_camera_triangle_possible({0,0,0},{0,0,5},ta,tb,tc),"camera prefilter rejects scenery beyond camera");
  check(!jfg::pc_camera_triangle_possible({0,0,20},{0,0,30},ta,tb,tc),"camera prefilter rejects scenery behind body");
  check(jfg::pc_camera_triangle_possible({0,0,10},{2,0,10},ta,tb,tc),"coplanar camera ray falls back to guest query");
  check(jfg::pc_camera_triangle_possible({0,0,0},{0,0,20},{0,0,10},{0,0,10},{0,0,10}),"degenerate camera geometry falls back to guest query");
  check(jfg::pc_camera_box_possible({0,0,0},{0,0,20},{-1,-1,9},{1,1,11}),"moving model bounds retain possible body contact");
  check(!jfg::pc_camera_box_possible({0,0,0},{0,0,20},{30,-1,9},{40,1,11}),"distant moving model does not require a query");
  check(jfg::pc_camera_box_possible({0,0,0},{0,0,20},{2,2,2},{1,1,1}),"invalid model bounds fall back to guest query");
  check(jfg::pc_air_aim(true,3),"modern Aim can coexist with airborne locomotion");
  check(!jfg::pc_air_aim(false,3,true),"releasing Aim in flight clears aim ownership");
  check(jfg::pc_air_aim(true,0,true),"landing keeps Aim through native ground transition");
  check(!jfg::pc_air_aim(true,0),"normal ground entry remains native");
  for(unsigned m:{1U,2U,4U,5U,6U,8U,10U,11U,32U})
    check(!jfg::pc_air_aim(true,m,true),"air aim never overrides unrelated native states");
  check(jfg::pc_normal_camera_owns_look(false,0),"normal camera owns unheld ground look");
  check(jfg::pc_normal_camera_owns_look(true,3),"airborne normal camera keeps look with aim held");
  check(jfg::pc_normal_camera_owns_look(true,4),"swimming normal camera keeps look with aim held");
  check(!jfg::pc_normal_camera_owns_look(true,10),"ground aim transition retains camera ownership");
  check(!jfg::pc_normal_camera_owns_look(true,11),"ground aiming retains camera ownership");
  check(!jfg::pc_normal_camera_owns_look(true,5),"other special states are not implicitly enabled");
  check(jfg::pc_camera_player_alpha({0,0,0},{0,35,0},255)==24,"camera inside player retains a faint silhouette");
  check(jfg::pc_camera_player_alpha({0,0,0},{80,35,0},255)==255,"camera outside fade range stays opaque");
  check(jfg::pc_camera_player_alpha({0,0,0},{0,140,0},255)==255,"overhead camera outside body stays opaque");
  check(jfg::pc_camera_player_alpha({0,0,0},{0,65,0},255)==24,"camera close to head fades");
  check(jfg::pc_camera_player_alpha({0,0,0},{40,35,0},255)<jfg::pc_camera_player_alpha({0,0,0},{60,35,0},255),"opacity recovers smoothly with distance");
  check(jfg::pc_camera_player_alpha({0,0,0},{40,35,0},10)==10,"camera fade preserves stronger guest fade");
  check(jfg::pc_camera_player_alpha({0,0,0},{200,35,0},95)==95,"distant camera preserves guest opacity");
  check(jfg::pc_camera_player_alpha({100,200,300},{140,235,300},255)==jfg::pc_camera_player_alpha({0,0,0},{40,35,0},255),"proximity fade is independent of room origin");
  check(jfg::pc_camera_player_alpha({0,0,0},{NAN,35,0},255)==255,"invalid camera leaves opacity unchanged");
  check(jfg::pc_camera_player_alpha({0,INFINITY,0},{0,35,0},255)==255,"invalid actor leaves opacity unchanged");
  check(jfg::pc_camera_player_alpha({0,0,0},{0,35,0},-1)==-1,"unexpected guest return is preserved");
  jfg::PcScopeMotion scope;
  auto scopeLook=scope.take({10,-20},1,1);
  check(scopeLook.x==10&&scopeLook.y==-20,"unzoomed direct look remains exact");
  int scopeX=0,scopeY=0;
  for(int i=0;i<100;++i){scopeLook=scope.take({1,-1},5,1);scopeX+=scopeLook.x;scopeY+=scopeLook.y;}
  check(scopeX==20&&scopeY==-20,"small scoped input survives fractional scaling");
  scope.clear();scope.take({1,0},2,1);scopeLook=scope.take({-1,0},2,1);
  check(scopeLook.x==0,"opposite scoped fractions cancel");
  scope.clear();scope.take({1,0},2,1);scopeLook=scope.take({1,0},2,2);
  check(scopeLook.x==0,"new actor or input mode clears scoped fractions");
  scope.clear();scope.take({1,0},2,1);scope.clear();scopeLook=scope.take({1,0},2,1);
  check(scopeLook.x==0,"focus or device reset clears scoped fractions");
  scopeLook=scope.take({0,0},2,1);check(scopeLook.x==0&&scopeLook.y==0,"neutral scope does not drift");
  for(float invalidZoom:{NAN,INFINITY,0.0F,6.0F}) {
    scopeLook=scope.take({-4,6},invalidZoom,1);check(scopeLook.x==-4&&scopeLook.y==6,"invalid magnification cannot amplify input");
  }
  scope.clear();scope.take({1,0},2,1);scope.take({0,0},1,1);scopeLook=scope.take({1,0},2,1);
  check(scopeLook.x==0,"returning to unzoomed view clears fractions");
  std::cout<<checks<<" PC input checks passed\n";
}
