#pragma once
#include "jfg/runtime/controller_ports.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <bit>
#include <cmath>
#include <sstream>
#include <string>
#include <string_view>

namespace jfg {
// Windows virtual keys, with two portable binding IDs for wheel pulses.
inline constexpr int kPcWheelUp = 256, kPcWheelDown = 257;
struct PcInputConfig {
  int modern = 0;
  int rumble_strength = 0; // 0 disables host rumble; otherwise 1-100 percent.
  int mouse_aim = 0, mouse_sensitivity = 100, mouse_invert_y = 0;
  int dual_stick = 0, aim_sensitivity = 100, aim_deadzone = 7849, aim_invert_y = 0;
  int mouse_aim_sensitivity = 100, camera_sensitivity = 100, aim_curve = 0;
  int mouse_vertical_sensitivity = 100, stick_vertical_sensitivity = 100;
  int mouse_invert_x = 0, aim_invert_x = 0;
  // A, B, Z, Start, D-pad, L, R, C-buttons, movement, fast movement, alternate A.
  std::array<int,20> keys{32,'X','C',13,38,40,37,39,'Q','E','I','K','J','L',
                           'W','S','A','D',16,'Z'};
};
// Historical fixture/profile helpers. The live runtime now uses only the
// selected ModernControlsConfig.enabled style; this legacy file is ignored.
inline bool pc_experiments_enabled(std::string_view text) {
  return text == "version=1\nexperimental=1\n" ||
         text == "version=1\r\nexperimental=1\r\n";
}
inline std::string serialize_pc_experiments(bool enabled) {
  return enabled ? "version=1\nexperimental=1\n" : "version=1\nexperimental=0\n";
}
inline PcInputConfig effective_pc_input(bool enabled,const PcInputConfig &saved) {
  auto active=saved;
  if(!enabled)active.modern=active.mouse_aim=active.dual_stick=active.rumble_strength=0;
  return active;
}
inline constexpr std::array<std::string_view,17> kPcSettings{
  "version","mouse_aim","mouse_sensitivity","mouse_invert_y","dual_stick",
  "aim_sensitivity","aim_deadzone","aim_invert_y","modern",
  "mouse_aim_sensitivity","camera_sensitivity","aim_curve",
  "mouse_vertical_sensitivity","stick_vertical_sensitivity","mouse_invert_x","aim_invert_x","rumble_strength"};
inline std::string serialize_pc_input(const PcInputConfig &c) {
  const std::array<int,17> values{5,c.mouse_aim,c.mouse_sensitivity,c.mouse_invert_y,
    c.dual_stick,c.aim_sensitivity,c.aim_deadzone,c.aim_invert_y,c.modern,
    c.mouse_aim_sensitivity,c.camera_sensitivity,c.aim_curve,
    c.mouse_vertical_sensitivity,c.stick_vertical_sensitivity,c.mouse_invert_x,c.aim_invert_x,c.rumble_strength};
  std::ostringstream out;
  for (std::size_t i=0;i<values.size();++i) out<<kPcSettings[i]<<'='<<values[i]<<'\n';
  for (std::size_t i=0;i<c.keys.size();++i) out<<"key"<<i<<'='<<c.keys[i]<<'\n';
  return out.str();
}
inline bool parse_pc_input(std::string_view text,PcInputConfig &result) {
  if(text.empty() || text.size()>4096) return false;
  PcInputConfig c;
  std::array<int,37> values{};
  std::array<bool,37> seen{};
  while(!text.empty()) {
    const auto n=text.find('\n');auto line=text.substr(0,n);
    text=n==text.npos?std::string_view{}:text.substr(n+1);
    if(!line.empty() && line.back()=='\r')line.remove_suffix(1);
    const auto eq=line.find('=');if(eq==line.npos)return false;
    const auto key=line.substr(0,eq),value=line.substr(eq+1);
    std::size_t slot=seen.size();
    for(std::size_t i=0;i<kPcSettings.size();++i)if(key==kPcSettings[i])slot=i;
    for(std::size_t i=0;i<c.keys.size();++i)if(key=="key"+std::to_string(i))slot=i+kPcSettings.size();
    if(slot==seen.size() || seen[slot] || value.empty())return false;
    int number=0;const auto parsed=std::from_chars(value.data(),value.data()+value.size(),number);
    if(parsed.ec!=std::errc{} || parsed.ptr!=value.data()+value.size() || value!=std::to_string(number))return false;
    seen[slot]=true;values[slot]=number;
  }
  if(values[0]<1 || values[0]>5)return false;
  if(values[0]<5) {if(seen[16])return false;seen[16]=true;}
  if(values[0]<4) {
    for(std::size_t i=14;i<16;++i){if(seen[i])return false;seen[i]=true;}
  }
  if(values[0]<3) {
    for(std::size_t i=9;i<14;++i){if(seen[i])return false;seen[i]=true;}
    values[9]=values[2];values[10]=values[5];values[11]=0;values[12]=values[13]=100;
  }
  if(values[0]==1 && seen[8])return false;
  if(values[0]==1)seen[8]=true;
  for(bool found:seen)if(!found)return false;
  for(auto i:{1,3,4,7,8,14,15})if(values[static_cast<std::size_t>(i)]<0 || values[static_cast<std::size_t>(i)]>1)return false;
  if(values[2]<10 || values[2]>500 || values[5]<10 || values[5]>300 || values[6]<0 || values[6]>30000)return false;
  if(values[9]<10 || values[9]>500 || values[10]<10 || values[10]>300 || values[11]<0 || values[11]>2 ||
     values[12]<10 || values[12]>300 || values[13]<10 || values[13]>300)return false;
  for(std::size_t i=0;i<c.keys.size();++i) {
    if(values[i+kPcSettings.size()]<0 || values[i+kPcSettings.size()]>kPcWheelDown)return false;
    c.keys[i]=values[i+kPcSettings.size()];
  }
  if(values[16]<0 || values[16]>100)return false;
  c.rumble_strength=values[16];
  c.mouse_aim=values[1];c.mouse_sensitivity=values[2];c.mouse_invert_y=values[3];
  c.dual_stick=values[4];c.aim_sensitivity=values[5];c.aim_deadzone=values[6];c.aim_invert_y=values[7];
  c.modern=values[8];c.mouse_aim_sensitivity=values[9];c.camera_sensitivity=values[10];c.aim_curve=values[11];
  c.mouse_vertical_sensitivity=values[12];c.stick_vertical_sensitivity=values[13];
  c.mouse_invert_x=values[14];c.aim_invert_x=values[15];
  result=c;return true;
}
inline PcInputConfig pc_keyboard_preset(bool expert=false) {
  PcInputConfig c;c.modern=1;c.mouse_aim=1;c.keys[2]=1;c.keys[9]=2;
  // Guest action tables: Normal next/previous = C-up/C-down, crouch = B;
  // Expert next/previous = B/A, crouch = C-down. E retains A for interaction.
  c.keys[0]=expert?kPcWheelDown:32;c.keys[1]=expert?kPcWheelUp:17;
  c.keys[3]=9;c.keys[10]=expert?32:kPcWheelUp;
  c.keys[11]=expert?17:kPcWheelDown;c.keys[19]='E';return c;
}
template<class Down> ControllerPortSample map_pc_keyboard(const PcInputConfig &c,Down down) {
  ControllerPortSample out;out.connected=true;
  constexpr std::array<std::uint16_t,14> masks{0x8000,0x4000,0x2000,0x1000,0x800,0x400,0x200,0x100,0x20,0x10,8,4,2,1};
  const auto held=[&](std::size_t i){return c.keys[i]!=0 && down(c.keys[i]);};
  for(std::size_t i=0;i<masks.size();++i)if(held(i))out.buttons|=masks[i];
  if(held(19))out.buttons|=0x8000;
  const int speed=held(18)?127:80;
  out.x=static_cast<std::int8_t>((int(held(17))-int(held(16)))*speed);
  out.y=static_cast<std::int8_t>((int(held(14))-int(held(15)))*speed);
  return out;
}
// Menu shortcuts remain usable with either guest control scheme or custom
// gameplay bindings. Pointer position is not interpreted as a guest menu item.
template<class Down> ControllerPortSample map_pc_menu(ControllerPortSample sample,Down down) {
  if(down(13))sample.buttons|=0x8000; // Enter: confirm.
  if(down(8))sample.buttons|=0x4000;  // Backspace: back.
  const int x=int(down(39))-int(down(37)),y=int(down(38))-int(down(40));
  if(x)sample.x=static_cast<std::int8_t>(x*80);
  if(y)sample.y=static_cast<std::int8_t>(y*80);
  return sample;
}
// Keep normal mapped buttons in menus, scripted scenes and unsupported modes.
// The unused stick becomes look input only while a qualified look path owns it.
inline MappedControllerSample map_pc_controller(const ControllerMapping &mapping,
    StandardControllerSample pad,const PcInputConfig &config,bool look_active) {
  auto mapped=map_controller(mapping,pad);
  if(look_active && config.dual_stick && (config.modern || (mapped.buttons&0x10)!=0)) {
    const std::size_t axis=mapping.stick==0?2U:0U;
    pad.axes[axis]=pad.axes[axis+1U]=0;
    const auto aim_button=mapped.buttons&0x10;
    mapped=map_controller(mapping,pad);
    mapped.buttons|=static_cast<std::uint16_t>(aim_button);
  }
  return mapped;
}
inline N64StickSample pc_aim_stick(const PcInputConfig &c,const StandardControllerSample &pad,int movement_stick,bool aiming=true) {
  const std::size_t axis=movement_stick==0?2U:0U;
  auto aim=scale_xinput_left_stick(pad.axes[axis],-pad.axes[axis+1],c.aim_deadzone);
  const float magnitude=std::min(1.0F,std::hypot(float(aim.x),float(aim.y))/80.0F);
  const float response=c.aim_curve==1?magnitude:c.aim_curve==2?magnitude*magnitude:1.0F;
  const int sensitivity=aiming?c.aim_sensitivity:c.camera_sensitivity;
  aim.x=static_cast<std::int8_t>(std::clamp(int(float(aim.x)*response*float(sensitivity)*(c.aim_invert_x?-1.0F:1.0F)/100.0F),-127,127));
  aim.y=static_cast<std::int8_t>(std::clamp(int(float(aim.y)*response*float(sensitivity)*float(c.stick_vertical_sensitivity)*(c.aim_invert_y?-1.0F:1.0F)/10000.0F),-127,127));
  return aim;
}
class PcWheelPulses {
  int remainder_=0, pending_=0;bool release_=false;
public:
  void add(int delta) { remainder_+=std::clamp(delta,-3840,3840);pending_=std::clamp(pending_+remainder_/120,-32,32);remainder_%=120; }
  int sample() {if(release_){release_=false;return 0;}if(!pending_)return 0;
    const int direction=pending_>0?1:-1;pending_-=direction;release_=true;return direction>0?kPcWheelUp:kPcWheelDown;}
  void clear(){remainder_=pending_=0;release_=false;}
};
struct PcLookDelta {
  int x=0,y=0;
  bool operator==(const PcLookDelta&) const = default;
};
// Modern look has its own precision/range; it is not an N64 movement stick.
// 2048 is full travel at 100%. The maximum configured vertical rate is 18432.
inline constexpr int kPcStickLookUnit = 2048;
inline PcLookDelta pc_stick_look(const PcInputConfig &c,
    const StandardControllerSample &pad,int movement_stick,bool aiming=true) {
  const std::size_t axis=movement_stick==0?2U:0U;
  const double x=std::clamp(pad.axes[axis],-32767,32767);
  const double y=-std::clamp(pad.axes[axis+1U],-32767,32767);
  const double magnitude=std::hypot(x,y),deadzone=std::clamp(c.aim_deadzone,0,30000);
  if(magnitude<=deadzone)return {};
  const double travel=std::clamp((magnitude-deadzone)/(32767.0-deadzone),0.0,1.0);
  const double response=c.aim_curve==1?travel*travel:c.aim_curve==2?travel*travel*travel:travel;
  const double sensitivity=std::clamp(aiming?c.aim_sensitivity:c.camera_sensitivity,10,300)/100.0;
  const double scale=kPcStickLookUnit*response*sensitivity/magnitude;
  return {static_cast<int>(std::lround(x*scale*(c.aim_invert_x?-1.0:1.0))),
    static_cast<int>(std::lround(y*scale*std::clamp(c.stick_vertical_sensitivity,10,300)/100.0*(c.aim_invert_y?-1.0:1.0)))};
}
// New devices and reconnects must be released before they can drive gameplay.
// Kept per port so one player's reconnect does not stall every other player.
class PcDeviceReleaseGate {
  int device_=-2;bool connected_=false,waiting_=true;
public:
  bool blocked(int device,const ControllerPortSample &sample,PcLookDelta look) {
    if(device!=device_ || sample.connected!=connected_)waiting_=true;
    device_=device;connected_=sample.connected;
    if(!connected_)return true;
    if(waiting_ && !sample.buttons && !sample.x && !sample.y && !look.x && !look.y)waiting_=false;
    return waiting_;
  }
};
// Preserve fine input at sniper magnification. The original aim routine uses
// the reciprocal of its 1..5 zoom factor; direct modern angles must do so too.
// Carry sub-angle motion rather than dropping small mouse/stick movements.
class PcScopeMotion {
  double x_=0,y_=0;
  std::uint64_t context_=0;
public:
  void clear(){x_=y_=0;context_=0;}
  PcLookDelta take(PcLookDelta look,float zoom,std::uint64_t context) {
    if(!std::isfinite(zoom) || zoom<=1.0F || zoom>5.0F){clear();return look;}
    if(context_!=context){clear();context_=context;}
    x_+=double(look.x)/zoom;y_+=double(look.y)/zoom;
    const int x=static_cast<int>(std::clamp(std::trunc(x_),double(INT32_MIN),double(INT32_MAX)));
    const int y=static_cast<int>(std::clamp(std::trunc(y_),double(INT32_MIN),double(INT32_MAX)));
    x_-=x;y_-=y;return {x,y};
  }
};

inline std::int16_t pc_wrap_angle(int angle) {
  return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(angle));
}
inline PcLookDelta pc_direct_aim(int yaw,int pitch,PcLookDelta delta,int minimum,int maximum) {
  if(minimum>maximum || minimum < -32768 || maximum>32767)return {yaw,pitch};
  return {pc_wrap_angle(yaw-delta.x),std::clamp(pitch-delta.y,minimum,maximum)};
}
// Modern look is expressed in camera pitch space, even when crouching or
// swimming uses the guest aim camera. Camera ownership must not change its sign.
// Older non-action-table replay modes keep their original aim convention.
inline PcLookDelta pc_guest_aim_look(PcLookDelta look, bool modern_actions) {
  return modern_actions ? PcLookDelta{look.x,-look.y} : look;
}
struct PcMoveVelocity { float side=0,forward=0; };
inline PcMoveVelocity pc_aim_movement(int x,int y) {
  // Aim walking stays within the guest's slow ground movement speeds.
  float sx=static_cast<float>(std::clamp(x,-80,80))/80.0F;
  float sy=static_cast<float>(std::clamp(y,-80,80))/80.0F;
  const float length=std::sqrt(sx*sx+sy*sy);
  if(length>1.0F){sx/=length;sy/=length;}
  return {sx*2.5F,-sy*3.0F};
}
// A modern view has one world heading. Camera yaw uses the opposite convention
// to actor/aim yaw; conversion must be shared by movement and both cameras.
inline int pc_shoulder_camera_yaw(int facing) { return pc_wrap_angle(32768-facing); }
inline bool pc_shoulder_state(unsigned state) {
  return state<=3U || state==10U || state==11U;
}
// Axes are the native post-deadzone values read by the character update.
// Native normal movement clamps their radial magnitude to 65 and scales by
// 1/16. Using raw host axes with a 5-unit cap made modern running too fast.
inline PcMoveVelocity pc_shoulder_movement(int x,int y) {
  float sx=static_cast<float>(std::clamp(x,-127,127));
  float sy=static_cast<float>(std::clamp(y,-127,127));
  const float length=std::hypot(sx,sy);
  if(length>65.0F){sx*=65.0F/length;sy*=65.0F/length;}
  return {sx/16.0F,-sy/16.0F};
}
// The native selector assumes the body turns into motion. Modern ground
// movement can reverse locally without that turn. Correct only locomotion
// requests; the caller resolves weapon variants through the native clip table.
inline unsigned pc_ground_animation(unsigned character,unsigned requested,unsigned family,PcMoveVelocity velocity) {
  const float forward=std::abs(velocity.forward),side=std::abs(velocity.side);
  const bool humanoid=character<2U;
  if(character>2U || !std::isfinite(forward) || !std::isfinite(side) ||
     std::max(forward,side)<0.1F ||
     !(family<=3U || (humanoid && (family==9U || family==10U))))return requested;
  if(humanoid && side>forward)return velocity.side<0.0F?9U:10U;
  if(velocity.forward>0.0F)return 3U;
  if(family<=2U)return requested; // Retain native forward-speed hysteresis.
  const float speed=std::max(forward,side);
  return speed>(character==0U?3.0F:3.5F)?2U:speed>(character==2U?2.0F:1.75F)?1U:0U;
}
// Native ground acceleration uses 0.95 raised by repeated single-precision
// multiplication to the integer guest tick count (80049BEC). Apply it to each
// modern movement component; mouse look and special movement states stay direct.
inline PcMoveVelocity pc_original_acceleration(PcMoveVelocity current,PcMoveVelocity target,float ticks) {
  const float speed=std::hypot(current.side,current.forward);
  if(!std::isfinite(speed))current={};
  else if(speed>4.0625F){current.side*=4.0625F/speed;current.forward*=4.0625F/speed;}
  if(!std::isfinite(ticks) || ticks<=0.0F)return current;
  float decay=1.0F;
  for(int i=0;i<static_cast<int>(std::min(ticks,120.0F));++i)decay*=0.95F;
  const float blend=1.0F-decay;
  return {current.side+blend*(target.side-current.side),
          current.forward+blend*(target.forward-current.forward)};
}
// The native air handler steers a signed forward speed and a world heading.
// Convert modern strafe/backward momentum into that representation once at
// takeoff. Camera turns must not rotate the already airborne velocity.
struct PcAirborneMotion { int yaw=0; float forward=0; };
inline PcAirborneMotion pc_native_airborne_motion(PcMoveVelocity velocity,int basis,bool backwards) {
  const float speed=std::hypot(velocity.side,velocity.forward);
  if(!std::isfinite(speed) || speed<0.0001F)return {pc_wrap_angle(basis),0};
  const float sign=backwards?1.0F:-1.0F;
  const int offset=static_cast<int>(std::lround(std::atan2(sign*velocity.side,sign*velocity.forward)*65536.0F/6.283185307179586F));
  return {pc_wrap_angle(basis+offset),sign*speed};
}
inline PcMoveVelocity pc_velocity_in_view(PcMoveVelocity velocity,int old_yaw,int view_yaw) {
  const float angle=static_cast<float>(pc_wrap_angle(old_yaw-view_yaw))*6.283185307179586F/65536.0F;
  const float sine=std::sin(angle),cosine=std::cos(angle);
  return {cosine*velocity.side+sine*velocity.forward,
          cosine*velocity.forward-sine*velocity.side};
}
struct PcOrbitPoint { float x=0,y=0,z=0; };
// Modern collision is a distance constraint on the input-owned viewing ray.
// The resolver is the native scenery query (including moving models and floor/
// ceiling clearance). A position is accepted only when that query leaves it
// unchanged. Never interpolate an off-ray correction through solid geometry.
struct PcCameraBoom {
  float distance=-1.0F,hold_seconds=0.0F;
  unsigned queries=0;
  bool constrained=false,fallback=false;
  template<class Resolve>
  PcOrbitPoint solve(PcOrbitPoint focus,PcOrbitPoint wanted,
                     PcOrbitPoint native_resolved,float seconds,Resolve resolve) {
    queries=0;constrained=false;fallback=false;
    const float dx=wanted.x-focus.x,dy=wanted.y-focus.y,dz=wanted.z-focus.z;
    const float length=std::hypot(dx,dy,dz);
    const auto finite=[](PcOrbitPoint p) {
      return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
    };
    const auto unchanged=[&](PcOrbitPoint a,PcOrbitPoint b) {
      return finite(a) && finite(b) && std::hypot(a.x-b.x,a.y-b.y,a.z-b.z)<=0.01F;
    };
    if(!finite(focus) || !finite(wanted) || !finite(native_resolved) ||
       !std::isfinite(length) || length<4.0F) {
      distance=-1;hold_seconds=0;fallback=true;return native_resolved;
    }
    const auto at=[&](float d) {
      const float scale=d/length;
      return PcOrbitPoint{focus.x+dx*scale,focus.y+dy*scale,focus.z+dz*scale};
    };
    const auto clear=[&](float d) {
      const auto p=at(d);++queries;return unchanged(p,resolve(p));
    };
    const auto limit=[&](float upper,PcOrbitPoint corrected) {
      // Use the native correction as a candidate bound, then validate it.
      // Floor/ceiling displacement bounds the vertical coordinate; wall
      // contacts bound the corresponding ray coordinates. This converges
      // quickly on ordinary planes without repeatedly bisecting the full boom.
      for(unsigned step=0;step<8;++step) {
        const auto requested=at(upper);
        float bound=upper;
        const std::array<float,3> origin{focus.x,focus.y,focus.z},
          direction{dx/length,dy/length,dz/length},
          request{requested.x,requested.y,requested.z},
          result{corrected.x,corrected.y,corrected.z};
        for(unsigned axis=0;axis<3;++axis)
          if(std::abs(direction[axis])>0.0001F && std::isfinite(result[axis]) &&
             (result[axis]-request[axis])*direction[axis]<-0.001F)
            bound=std::min(bound,(result[axis]-origin[axis])/direction[axis]);
        const bool height_only=std::abs(corrected.x-requested.x)<=0.01F &&
                               std::abs(corrected.z-requested.z)<=0.01F;
        // A floor/ceiling query already supplies vertical clearance. Applying
        // the wall inset and returning a halved seed can discard nearly the
        // entire clear boom at a shallow ledge or a horizontal view.
        const float next=std::max(4.0F,bound<upper-0.01F
          ?bound-(height_only?0.125F:12.0F):upper*0.5F);
        if(next>=upper)return -1.0F;
        const auto candidate=at(next);
        ++queries;corrected=resolve(candidate);
        if(unchanged(candidate,corrected)) {
          if(!height_only)return next;
          float accepted=next,rejected=upper;
          // First test immediately beyond the analytic estimate. Ordinary
          // planar floors then need no full-range search. A coarse seed is
          // refined only within a known accepted/rejected interval.
          const float adjacent=std::min(rejected,accepted+1.0F);
          if(clear(adjacent))accepted=adjacent;else rejected=adjacent;
          for(unsigned refine=0;refine<8 && rejected-accepted>0.5F;++refine) {
            const float middle=(accepted+rejected)*0.5F;
            if(clear(middle))accepted=middle;else rejected=middle;
          }
          return accepted;
        }
        upper=next;
      }
      // Bounded safety fallback, still subject to the native collision check.
      return clear(4.0F)?4.0F:-1.0F;
    };
    float safe=length;
    if(!unchanged(wanted,native_resolved)) {
      constrained=true;safe=limit(length,native_resolved);
      if(safe<0) {
        // No accepted point exists near the pivot. Retain the game's safe
        // correction for this exceptional configuration, never force clipping.
        distance=-1;hold_seconds=0;fallback=true;return native_resolved;
      }
    }
    const float dt=std::isfinite(seconds)?std::clamp(seconds,0.0F,0.25F):0.0F;
    float selected=safe;
    if(distance>=0 && safe>distance) {
      const float remaining=std::max(0.0F,dt-hold_seconds);
      hold_seconds=std::max(0.0F,hold_seconds-dt);
      selected=safe-(safe-distance)*std::exp(-remaining/0.18F);
      if(safe-selected<0.01F)selected=safe;
    } else if(distance>=0 && safe<distance-0.25F)hold_seconds=0.08F;
    // Recheck the recovered distance on this frame's ray. A turned camera or
    // moving obstacle invalidates yesterday's safety, even at a shorter radius.
    if(selected!=safe) {
      const auto candidate=at(selected);
      ++queries;const auto checked=resolve(candidate);
      if(!unchanged(candidate,checked)) {
        selected=limit(selected,checked);
        if(selected<0) {
          selected=safe;hold_seconds=0;
        } else hold_seconds=0.08F;
      }
    }
    distance=selected;
    return at(selected);
  }
};
struct PcShoulder {
  PcCameraBoom camera_boom{};
  PcMoveVelocity movement_before_update{};
  unsigned movement_before_state=0;
  int movement_before_yaw=0;
  bool active=false,aiming=false,air_aim=false;
  std::uint32_t actor=0,control=0;
  std::uint64_t retrace=0;
  int yaw=0,pitch=0;
  bool acquire(std::uint32_t next_actor,std::uint32_t next_control,int native_yaw,int native_pitch) {
    if(active && actor==next_actor && control==next_control)return false;
    *this={};active=true;actor=next_actor;control=next_control;
    yaw=pc_wrap_angle(native_yaw);pitch=std::clamp(native_pitch,-10922,10922);
    return true;
  }
};
// Modern Aim is independent of native jump locomotion. The landing update
// may briefly be state 0 before its ground handler re-enters native aim.
inline bool pc_air_aim(bool held,unsigned movement,bool continuing=false) {
  return held && (movement==3U || (movement==0U && continuing));
}
struct PcOrbit {
  bool active=false;
  std::uint32_t actor=0;
  std::uint64_t retrace=0;
  int yaw=0,pitch=0;
};
// The guest retains its ordinary camera during flight (state 3) and swimming
// (state 4), even with aim held. Keep look responsive there; ground aim owns
// its separate camera. Movement and special-state aiming stay with the guest.
inline bool pc_normal_camera_owns_look(bool aim_held,std::uint8_t movement_state) {
  return !aim_held || movement_state==3U || movement_state==4U;
}

// Render-only opacity for a camera inside the player's viewing space. Measure
// distance to the upright body, so looking down at the head also fades smoothly.
// Preserve stronger guest fades and fall back unchanged for invalid positions.
inline int pc_camera_player_alpha(PcOrbitPoint body,PcOrbitPoint camera,int guest_alpha) {
  if(guest_alpha<0 || guest_alpha>255 || !std::isfinite(body.x) ||
     !std::isfinite(body.y) || !std::isfinite(body.z) || !std::isfinite(camera.x) ||
     !std::isfinite(camera.y) || !std::isfinite(camera.z))return guest_alpha;
  const float y=std::clamp(camera.y,body.y+8.0F,body.y+60.0F);
  const float distance=std::hypot(camera.x-body.x,camera.y-y,camera.z-body.z);
  const float t=std::clamp((distance-20.0F)/60.0F,0.0F,1.0F);
  const int alpha=static_cast<int>(std::lround(24.0F+231.0F*t*t*(3.0F-2.0F*t)));
  return std::min(guest_alpha,alpha);
}
// Fixed modern ground boom: native follow distance includes actor/camera lag.
// Feeding that lag back into a direct camera produces periodic forward steps.
inline constexpr float kPcShoulderDistance=200.0F;
inline PcOrbitPoint pc_orbit_point(int yaw,int pitch,float radius) {
  constexpr float radians=6.283185307179586F/65536.0F;
  const float y=static_cast<float>(yaw)*radians,p=static_cast<float>(pitch)*radians;
  const float horizontal=radius*std::cos(p);
  return {std::sin(y)*horizontal,std::sin(p)*radius,-std::cos(y)*horizontal};
}
inline PcOrbitPoint pc_camera_clearance(PcOrbitPoint focus,PcOrbitPoint wanted,PcOrbitPoint resolved) {
  const float ex=resolved.x-wanted.x,ey=resolved.y-wanted.y,ez=resolved.z-wanted.z;
  const float changed=ex*ex+ey*ey+ez*ez;
  const float dx=resolved.x-focus.x,dy=resolved.y-focus.y,dz=resolved.z-focus.z;
  const float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
  if(!std::isfinite(changed) || !std::isfinite(distance) || changed<1.0F || distance<=16.0F)return resolved;
  // Only retreat along an actual line-of-sight hit. A vertical floor/ceiling
  // correction is not evidence that this ray has free clearance behind it.
  const float dot=ex*dx+ey*dy+ez*dz;
  const float perpendicular=changed-dot*dot/(distance*distance);
  if(dot>=0 || perpendicular>std::max(1.0F,changed*0.01F))return resolved;
  const float scale=(distance-std::min(12.0F,distance-16.0F))/distance;
  return {focus.x+dx*scale,focus.y+dy*scale,focus.z+dz*scale};
}
// Three lower-body probes protect the visible width, including a thin edge
// which misses the center ray. Near-vertical views retain a finite basis.
inline std::array<PcOrbitPoint,3> pc_camera_body_probes(PcOrbitPoint body,PcOrbitPoint camera) {
  const float dx=camera.x-body.x,dz=camera.z-body.z;
  const float length=std::hypot(dx,dz);
  const float x=length>0.01F?20.0F*dz/length:20.0F;
  const float z=length>0.01F?-20.0F*dx/length:0.0F;
  return {body,PcOrbitPoint{body.x+x,body.y,body.z+z},PcOrbitPoint{body.x-x,body.y,body.z-z}};
}
// Project a verified lower-body line-of-sight contact onto the camera's orbit.
// The caller supplies the raw intersection, before the guest camera helper's
// floor/height corrections. Reject off-ray, behind-body and beyond-camera hits.
inline PcOrbitPoint pc_camera_body_clearance(PcOrbitPoint focus,PcOrbitPoint camera,
                                            PcOrbitPoint body,PcOrbitPoint hit) {
  const float dx=camera.x-body.x,dy=camera.y-body.y,dz=camera.z-body.z;
  const float hx=hit.x-body.x,hy=hit.y-body.y,hz=hit.z-body.z;
  const float length2=dx*dx+dy*dy+dz*dz,along=hx*dx+hy*dy+hz*dz;
  if(!std::isfinite(length2) || length2<256.0F || !std::isfinite(along))return camera;
  const float error=hx*hx+hy*hy+hz*hz-along*along/length2;
  const float fx=camera.x-focus.x,fy=camera.y-focus.y,fz=camera.z-focus.z;
  const float distance=std::sqrt(fx*fx+fy*fy+fz*fz);
  if(!std::isfinite(error) || error>1.0F || along<0 || along>=length2 ||
     !std::isfinite(distance) || distance<=16.0F)return camera;
  const float fraction=std::max(16.0F/distance,(along-12.0F*std::sqrt(length2))/length2);
  return {focus.x+fx*fraction,focus.y+fy*fraction,focus.z+fz*fraction};
}
inline PcLookDelta pc_camera_delta(std::uint8_t mode,PcLookDelta look,float ticks) {
  if(mode==3)return look;
  if(mode!=4 || !std::isfinite(ticks) || ticks<=0)return {};
  const float rate=256.0F*std::min(ticks,5.0F)/80.0F;
  return {static_cast<int>(std::lround(static_cast<float>(look.x)*rate)),
          static_cast<int>(std::lround(static_cast<float>(look.y)*rate))};
}
// Integrate the modern stick rate without throwing away sub-angle motion on
// each guest update. Context changes and neutral input discard stale fractions.
// A recording supplies input independently of physical window focus. Explicit
// replay suspension still exercises the ordinary loss-of-input reset path.
inline bool pc_input_motion_blocked(bool physical_blocked, bool replay_loaded,
                                    bool replay_suspended) noexcept {
  return (physical_blocked && !replay_loaded) || replay_suspended;
}
class PcStickMotion {
  double x_=0,y_=0;std::uint32_t context_=0;
public:
  PcLookDelta take(PcLookDelta look,float ticks,std::uint32_t context=0) {
    if(context!=context_){clear();context_=context;}
    if(!std::isfinite(ticks) || ticks<=0){clear();return {};}
    const double rate=256.0*std::min(double(ticks),5.0)/kPcStickLookUnit;
    x_=look.x?x_+std::clamp(look.x,-18432,18432)*rate:0;
    y_=look.y?y_+std::clamp(look.y,-18432,18432)*rate:0;
    const PcLookDelta result{static_cast<int>(std::trunc(x_)),static_cast<int>(std::trunc(y_))};
    x_-=result.x;y_-=result.y;return result;
  }
  void clear(){x_=y_=0;context_=0;}
};
class PcMouseMotion {
  double x_=0,y_=0;
public:
  void add(int x,int y){x_=std::clamp(x_+double(x),-4096.0,4096.0);y_=std::clamp(y_+double(y),-4096.0,4096.0);}
  N64StickSample take(const PcInputConfig &c) {
    const double scale=double(c.mouse_aim_sensitivity)/100.0*(c.mouse_invert_x?-1.0:1.0);
    const int x=std::clamp(static_cast<int>(std::trunc(x_*scale)),-127,127);
    const double y_scale=std::abs(scale)*double(c.mouse_vertical_sensitivity)/100.0*(c.mouse_invert_y?1.0:-1.0);
    const int y=std::clamp(static_cast<int>(std::trunc(y_*y_scale)),-127,127);
    // Preserve subpixel displacement at low sensitivity, but discard clipped
    // excess so a fast flick cannot keep turning after the mouse stops.
    x_=std::abs(x_*scale)>127.0?0.0:x_-double(x)/scale;
    y_=std::abs(y_*y_scale)>127.0?0.0:y_-double(y)/y_scale;
    return {static_cast<std::int8_t>(x),static_cast<std::int8_t>(y)};
  }
  PcLookDelta take_angles(const PcInputConfig &c,bool aiming=false) {
    // One pixel at 100% is approximately 0.1 degrees, independent of update rate.
    const double sx=18.0*double(aiming?c.mouse_aim_sensitivity:c.mouse_sensitivity)/100.0*(c.mouse_invert_x?-1.0:1.0);
    const double sy=std::abs(sx)*double(c.mouse_vertical_sensitivity)/100.0*(c.mouse_invert_y?1.0:-1.0);
    const int x=std::clamp(static_cast<int>(std::trunc(x_*sx)),-16384,16384);
    const int y=std::clamp(static_cast<int>(std::trunc(y_*sy)),-16384,16384);
    x_=std::abs(x_*sx)>16384.0?0.0:x_-double(x)/sx;
    y_=std::abs(y_*sy)>16384.0?0.0:y_-double(y)/sy;
    return {x,y};
  }
  void clear(){x_=y_=0;}
};
} // namespace jfg
