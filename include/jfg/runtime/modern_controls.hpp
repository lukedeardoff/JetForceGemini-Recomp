#pragma once

#include "jfg/runtime/pc_input.hpp"
#include "jfg/runtime/controller_catalog.hpp"
#include <optional>

namespace jfg {
// Stable action IDs are shared by the RmlUi editor, profiles and runtime.
// Original controller.ini and pc-input.ini bindings are never modified here.
enum class ModernAction : std::size_t {
    aim, fire, jump, crouch, interact, weapon_previous, weapon_next,
    scope_cycle, scope_increase, scope_decrease, pause, menu_confirm, menu_back,
    move_forward, move_back, move_left, move_right, move_fast,
    menu_up, menu_down, menu_left, menu_right, menu_save, count
};
inline constexpr std::size_t kModernActionCount = static_cast<std::size_t>(ModernAction::count);
inline constexpr unsigned kModernGameplay = 1, kModernScoped = 2, kModernMenus = 4;
struct ModernActionInfo { std::string_view id, label; unsigned contexts; bool gamepad=true; };
inline constexpr std::array<ModernActionInfo,kModernActionCount> kModernActions{{
    {"aim","Aim",3}, {"fire","Fire",3}, {"jump","Jump",3}, {"crouch","Crouch",3},
    {"interact","Interact",3}, {"weapon_previous","Previous weapon",3},
    {"weapon_next","Next weapon",3}, {"scope_cycle","Cycle magnification",2},
    {"scope_increase","Increase magnification",2}, {"scope_decrease","Decrease magnification",2},
    {"pause","Pause",7}, {"menu_confirm","Confirm",4}, {"menu_back","Back",4},
    {"move_forward","Move forward",7,false}, {"move_back","Move back",7,false},
    {"move_left","Move left",7,false}, {"move_right","Move right",7,false},
    {"move_fast","Full movement",3,false}, {"menu_up","Menu up",4},
    {"menu_down","Menu down",4}, {"menu_left","Menu left",4}, {"menu_right","Menu right",4}, {"menu_save","Save game",4}
}};
inline constexpr std::uint32_t modern_bit(ModernAction action) {
    return std::uint32_t{1} << static_cast<unsigned>(action);
}
inline std::optional<ModernAction> modern_action(std::string_view id) {
    for(std::size_t i=0;i<kModernActions.size();++i)
        if(kModernActions[i].id==id)return static_cast<ModernAction>(i);
    return {};
}
enum class ModernGlyphStyle { automatic=0, xbox=1, playstation=2 };
struct ModernControlsConfig {
    bool enabled = false;
    bool aim_toggle = false;
    bool mouse_original_acceleration = false;
    ModernGlyphStyle glyph_style = ModernGlyphStyle::automatic;
    int movement_stick = 0, movement_deadzone = 7849;
    int mouse_scope_sensitivity = 100, pad_scope_sensitivity = 100;
    int axis_threshold = 8689, trigger_threshold = 3855;
    // Standardized SDL/XInput positions; -1 is unbound. Trigger +/- codes are
    // identical to ControllerMapping, but these bindings name gameplay actions.
    std::array<int,kModernActionCount> pad{
        23,25,0,1,2,9,10,8,-1,-1,6,0,1,-1,-1,-1,-1,-1,11,12,13,14,2};
    // Windows virtual keys plus portable wheel IDs. Zero is unbound.
    std::array<int,kModernActionCount> keys{
        2,1,32,17,'E',kPcWheelDown,kPcWheelUp,4,0,0,9,'E','Q',
        'W','S','A','D',16,0,0,0,0,0x74};
    PcInputConfig look = [] {
        PcInputConfig c;c.modern=c.mouse_aim=c.dual_stick=1;c.mouse_invert_y=1;return c;
    }();
};
inline constexpr std::array<std::string_view,11> kModernSettings{
    "version","enabled","aim_toggle","glyph_style","movement_stick",
    "movement_deadzone","axis_threshold","trigger_threshold","mouse_scope_sensitivity","pad_scope_sensitivity","mouse_original_acceleration"};
// Modern keyboard navigation follows the movement keys; arrow keys are not
// bindings. Keep the persisted action IDs for controller and profile compatibility.
inline constexpr bool modern_arrow_key(int key) { return key>=37 && key<=40; }
inline std::string serialize_modern_controls(const ModernControlsConfig &c) {
    std::string out;
    const std::array<int,kModernSettings.size()> values{5,int(c.enabled),int(c.aim_toggle),int(c.glyph_style),
        c.movement_stick,c.movement_deadzone,c.axis_threshold,c.trigger_threshold,
        c.mouse_scope_sensitivity,c.pad_scope_sensitivity,int(c.mouse_original_acceleration)};
    for(std::size_t i=0;i<values.size();++i)
        out+=std::string(kModernSettings[i])+"="+std::to_string(values[i])+"\n";
    for(std::size_t i=0;i<kModernActionCount;++i) {
        out+="pad."+std::string(kModernActions[i].id)+"="+std::to_string(c.pad[i])+"\n";
        out+="key."+std::string(kModernActions[i].id)+"="+std::to_string(modern_arrow_key(c.keys[i])?0:c.keys[i])+"\n";
    }
    const auto look=serialize_pc_input(c.look);
    std::size_t start=0;
    while(start<look.size()) {
        const auto end=look.find('\n',start);
        out+="look."+look.substr(start,end-start)+"\n";start=end+1;
    }
    return out;
}
inline bool parse_modern_controls(std::string_view text,ModernControlsConfig &result) {
    if(text.empty() || text.size()>8192)return false;
    ModernControlsConfig c;std::string look;
    std::array<bool,kModernSettings.size()+2*kModernActionCount> seen{};
    std::array<int,kModernSettings.size()> values{};
    while(!text.empty()) {
        const auto end=text.find('\n');auto line=text.substr(0,end);
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        if(!line.empty() && line.back()=='\r')line.remove_suffix(1);
        if(line.starts_with("look.")){look+=line.substr(5);look+='\n';continue;}
        const auto eq=line.find('=');if(eq==line.npos)return false;
        const auto key=line.substr(0,eq),value=line.substr(eq+1);
        std::size_t slot=seen.size();
        for(std::size_t i=0;i<kModernSettings.size();++i)if(key==kModernSettings[i])slot=i;
        for(std::size_t i=0;i<kModernActionCount;++i) {
            if(key=="pad."+std::string(kModernActions[i].id))slot=kModernSettings.size()+i;
            if(key=="key."+std::string(kModernActions[i].id))slot=kModernSettings.size()+kModernActionCount+i;
        }
        if(slot==seen.size() || seen[slot])return false;
        int number=0;const auto parsed=std::from_chars(value.data(),value.data()+value.size(),number);
        if(parsed.ec!=std::errc{} || parsed.ptr!=value.data()+value.size() || value!=std::to_string(number))return false;
        seen[slot]=true;
        if(slot<kModernSettings.size())values[slot]=number;
        else if(slot<kModernSettings.size()+kModernActionCount) {
            if(number < -1 || number>26 || (!kModernActions[slot-kModernSettings.size()].gamepad && number!=-1))return false;
            c.pad[slot-kModernSettings.size()]=number;
        } else {
            if(number<0 || number>kPcWheelDown) return false;
            c.keys[slot-kModernSettings.size()-kModernActionCount]=number;
        }
    }
    // Version 1 predates independent scope tuning. Its exact fields remain accepted.
    if(values[0]==1) {
        if(seen[8] || seen[9])return false;
        values[8]=values[9]=100;seen[8]=seen[9]=true;
    }
    // Older profiles default to immediate keyboard movement. Reject mixed schemas.
    if(values[0]==1 || values[0]==2) {
        if(seen[10])return false;
        values[10]=0;seen[10]=true;
    }
    // Version 4 adds a separate pause-menu Save action. Migrate older bindings
    // without assigning Save to a key already used for another menu action.
    if(values[0]>=1 && values[0]<=3) {
        constexpr auto save=static_cast<std::size_t>(ModernAction::menu_save);
        const auto padSlot=kModernSettings.size()+save,keySlot=padSlot+kModernActionCount;
        if(seen[padSlot] || seen[keySlot])return false;
        for(std::size_t i=0;i<save;++i) if(kModernActions[i].contexts&kModernMenus) {
            if(c.pad[i]==c.pad[save])c.pad[save]=-1;
            if(c.keys[i]==c.keys[save])c.keys[save]=0;
        }
        seen[padSlot]=seen[keySlot]=true;
    }
    for(bool present:seen)if(!present)return false;
    if((values[0]!=1 && values[0]!=2 && values[0]!=3 && values[0]!=4 && values[0]!=5) || values[1]<0 || values[1]>1 || values[2]<0 || values[2]>1 ||
       values[3]<0 || values[3]>2 || values[4]<0 || values[4]>1 ||
       values[5]<0 || values[5]>30000 || values[6]<1000 || values[6]>32000 ||
       values[7]<1000 || values[7]>32000 || values[8]<10 || values[8]>300 ||
       values[9]<10 || values[9]>300 || values[10]<0 || values[10]>1 || !parse_pc_input(look,c.look) ||
       c.look.modern!=1 || c.look.mouse_aim!=1 || c.look.dual_stick!=1)return false;
    // Only migrate the old default Back binding; preserve custom controls and
    // avoid taking Q from a menu or movement action. New profiles expose Q in UI.
    if(values[0]<5 && c.keys[static_cast<std::size_t>(ModernAction::menu_back)]==8) {
        bool available=true;
        for(std::size_t i=0;i<kModernActionCount;++i)
            if((kModernActions[i].contexts&kModernMenus) && c.keys[i]=='Q')available=false;
        if(available)c.keys[static_cast<std::size_t>(ModernAction::menu_back)]='Q';
    }
    c.enabled=values[1]!=0;c.aim_toggle=values[2]!=0;
    c.glyph_style=static_cast<ModernGlyphStyle>(values[3]);c.movement_stick=values[4];
    c.movement_deadzone=values[5];c.axis_threshold=values[6];c.trigger_threshold=values[7];
    c.mouse_scope_sensitivity=values[8];c.pad_scope_sensitivity=values[9];
    c.mouse_original_acceleration=values[10]!=0;
    // Retire arrow bindings from saved modern profiles without changing other
    // choices, including shared E actions, inversion and controller mappings.
    for(auto& key:c.keys)if(modern_arrow_key(key))key=0;
    result=c;return true;
}
inline bool modern_binding_conflicts(ModernAction first,ModernAction second) {
    const auto a=static_cast<std::size_t>(first),b=static_cast<std::size_t>(second);
    return a<kModernActionCount && b<kModernActionCount && a!=b &&
        (kModernActions[a].contexts&kModernActions[b].contexts)!=0;
}

enum class ModernBindPolicy { reject_conflict, replace_conflicts, keep_both };
enum class ModernBindResult { invalid, conflict, unchanged, changed };
inline std::array<bool,kModernActionCount> modern_binding_conflicts(
    const ModernControlsConfig &c,bool keyboard,ModernAction action,int binding) {
    std::array<bool,kModernActionCount> conflicts{};
    if(binding==(keyboard?0:-1))return conflicts;
    const auto &bindings=keyboard?c.keys:c.pad;
    for(std::size_t i=0;i<bindings.size();++i)
        conflicts[i]=bindings[i]==binding && modern_binding_conflicts(action,static_cast<ModernAction>(i));
    return conflicts;
}
inline ModernBindResult rebind_modern_action(ModernControlsConfig &c,bool keyboard,
    ModernAction action,int binding,ModernBindPolicy policy=ModernBindPolicy::reject_conflict) {
    const auto index=static_cast<std::size_t>(action);
    if(index>=kModernActionCount || (!keyboard && !kModernActions[index].gamepad) ||
       binding<(keyboard?0:-1) || binding>(keyboard?kPcWheelDown:26) ||
       (keyboard && modern_arrow_key(binding)))
        return ModernBindResult::invalid;
    auto &bindings=keyboard?c.keys:c.pad;
    if(bindings[index]==binding)return ModernBindResult::unchanged;
    const auto conflicts=modern_binding_conflicts(c,keyboard,action,binding);
    if(policy==ModernBindPolicy::reject_conflict &&
       std::any_of(conflicts.begin(),conflicts.end(),[](bool value){return value;}))
        return ModernBindResult::conflict;
    if(policy==ModernBindPolicy::replace_conflicts)
        for(std::size_t i=0;i<bindings.size();++i)if(conflicts[i])bindings[i]=keyboard?0:-1;
    bindings[index]=binding;return ModernBindResult::changed;
}

inline ModernGlyphStyle resolve_modern_glyph_style(ModernGlyphStyle preference,ControllerFamily family) {
    if(preference!=ModernGlyphStyle::automatic)return preference;
    if(family==ControllerFamily::xbox)return ModernGlyphStyle::xbox;
    if(family==ControllerFamily::playstation || family==ControllerFamily::dualsense)return ModernGlyphStyle::playstation;
    return ModernGlyphStyle::automatic;
}
inline std::string modern_pad_name(int binding,ModernGlyphStyle style) {
    constexpr std::array<std::string_view,15> xbox{"A","B","X","Y","View","Guide","Menu","LS click","RS click","LB","RB","D-pad up","D-pad down","D-pad left","D-pad right"};
    constexpr std::array<std::string_view,15> ps{"Cross","Circle","Square","Triangle","Create","PS","Options","L3","R3","L1","R1","D-pad up","D-pad down","D-pad left","D-pad right"};
    constexpr std::array<std::string_view,8> axes{"Left stick right","Left stick left","Left stick down","Left stick up","Right stick right","Right stick left","Right stick down","Right stick up"};
    constexpr std::array<std::string_view,15> generic{"South button","East button","West button","North button","Back","Guide","Start","Left stick click","Right stick click","Left shoulder","Right shoulder","D-pad up","D-pad down","D-pad left","D-pad right"};
    if(binding==-1)return "Unbound";
    if(style==ModernGlyphStyle::automatic && binding>=0 && binding<15)return std::string(generic[static_cast<std::size_t>(binding)]);
    if(style==ModernGlyphStyle::automatic && binding==23)return "Left trigger";
    if(style==ModernGlyphStyle::automatic && binding==25)return "Right trigger";
    if(binding>=0 && binding<15)return std::string((style==ModernGlyphStyle::playstation?ps:xbox)[static_cast<std::size_t>(binding)]);
    if(binding>=15 && binding<=22)return std::string(axes[static_cast<std::size_t>(binding-15)]);
    if(binding==23)return style==ModernGlyphStyle::playstation?"L2":"LT";
    if(binding==25)return style==ModernGlyphStyle::playstation?"R2":"RT";
    if(binding==24 || binding==26)return "Negative trigger axis";
    return "Unknown input";
}
// Aim held while swimming or airborne still owns the ordinary camera.
// Scope precision must not alter that camera or retain fractional motion.
inline bool modern_scope_sensitivity_active(bool scoped,std::uint8_t movement_state) {
    return scoped && !pc_normal_camera_owns_look(true,movement_state);
}
// Apply only to live scoped input before recording. Replays already contain the
// scaled deltas, so playback cannot change when the owner's sensitivity changes.
// Integer hundredths preserve tiny movement without drift or floating error.
class ModernScopeSensitivity {
    std::int64_t x_=0,y_=0;
    std::uint64_t context_=0;
    int percent_=100;
public:
    void clear(){x_=y_=0;context_=0;percent_=100;}
    PcLookDelta take(PcLookDelta look,bool scoped,int percent,std::uint64_t context) {
        if(!scoped){clear();return look;}
        percent=std::clamp(percent,10,300);
        if(context_!=context || percent_!=percent){clear();context_=context;percent_=percent;}
        auto scale=[&](int value,std::int64_t& remainder) {
            const auto total=std::int64_t(value)*percent+remainder;
            const auto result=std::clamp<std::int64_t>(total/100,-32767,32767);
            remainder=(result==total/100)?total-result*100:0;
            return static_cast<int>(result);
        };
        return {scale(look.x,x_),scale(look.y,y_)};
    }
};
struct ModernActionSample {
    std::uint32_t held=0;
    N64StickSample move{};
    bool gamepad=false;
    bool physically_neutral=true;
};
inline bool modern_pad_down(const ModernControlsConfig &c,const StandardControllerSample &pad,int binding) {
    if(binding>=0 && binding<15)return pad.buttons[static_cast<std::size_t>(binding)];
    if(binding<15 || binding>26)return false;
    const auto axis=static_cast<std::size_t>((binding-15)/2);
    const int threshold=axis>=4?c.trigger_threshold:c.axis_threshold;
    return (binding-15)%2==0?pad.axes[axis]>threshold:pad.axes[axis]<-threshold;
}
inline ModernActionSample sample_modern_pad(const ModernControlsConfig &c,const StandardControllerSample &pad) {
    ModernActionSample out;out.gamepad=true;
    for(std::size_t i=0;i<kModernActionCount;++i)
        if(kModernActions[i].gamepad && modern_pad_down(c,pad,c.pad[i]))out.held|=std::uint32_t{1}<<i;
    const auto axis=static_cast<std::size_t>(c.movement_stick*2);
    out.move=scale_xinput_left_stick(pad.axes[axis],-pad.axes[axis+1],c.movement_deadzone);
    for(bool button:pad.buttons)out.physically_neutral&=!button;
    for(std::size_t i=0;i<pad.axes.size();++i)
        out.physically_neutral&=std::abs(std::clamp(pad.axes[i],-32767,32767)) <=
            (i>=4?c.trigger_threshold:c.movement_deadzone);
    return out;
}
template<class Down> ModernActionSample sample_modern_keyboard(const ModernControlsConfig &c,Down down) {
    ModernActionSample out;
    for(std::size_t i=0;i<kModernActionCount;++i)
        if(c.keys[i]!=0 && !modern_arrow_key(c.keys[i]) && down(c.keys[i]))out.held|=std::uint32_t{1}<<i;
    const auto held=[&](ModernAction a){return (out.held&modern_bit(a))!=0;};
    const int speed=held(ModernAction::move_fast)?127:80;
    out.move={static_cast<std::int8_t>((int(held(ModernAction::move_right))-int(held(ModernAction::move_left)))*speed),
              static_cast<std::int8_t>((int(held(ModernAction::move_forward))-int(held(ModernAction::move_back)))*speed)};
    out.physically_neutral=out.held==0;return out;
}
// Canonical internal action masks. Interaction keeps guest A exclusively;
// previous-weapon uses L, separating it from the Expert scheme's A binding.
// A scoped temporary action-table override applies these inside player update.
inline constexpr std::array<std::uint32_t,9> kModernGuestTable{
    0x2000,0x4000,0x20,0x8,0x4,0x2,0x1,0x8,0x4};
inline constexpr std::array<std::uint32_t,9> kNormalGuestTable{
    0x2000,0x8,0x4,0x8000,0x4000,0x2,0x1,0x8000,0x4000};
inline constexpr std::array<std::uint32_t,9> kExpertGuestTable{
    0x2000,0x4000,0x8000,0x8,0x4,0x2,0x1,0x8,0x4};
// Replay flag bit 7 selects action semantics. Bits 0..2 hold scope actions.
inline constexpr std::uint8_t kModernActionsFlag=0x80;
inline std::uint8_t modern_scope_flags(std::uint32_t held) {
    return static_cast<std::uint8_t>(kModernActionsFlag |
        ((held&modern_bit(ModernAction::scope_cycle))?1:0) |
        ((held&modern_bit(ModernAction::scope_increase))?2:0) |
        ((held&modern_bit(ModernAction::scope_decrease))?4:0));
}
inline ControllerPortSample map_modern_actions(const ModernActionSample &input,bool menu,bool can_save=false) {
    ControllerPortSample out;out.connected=true;out.x=input.move.x;out.y=input.move.y;
    const auto held=[&](ModernAction a){return (input.held&modern_bit(a))!=0;};
    if(held(ModernAction::pause))out.buttons|=0x1000;
    if(menu) {
        // Movement bindings also navigate menus. Optional non-arrow keyboard
        // bindings and controller D-pad actions take precedence on their axis.
        // This respects remapped movement keys without hard-coding WASD.
        if(held(ModernAction::menu_confirm))out.buttons|=0x8000;
        if(can_save && held(ModernAction::menu_save))out.buttons|=0x2000;
        if(held(ModernAction::menu_back))out.buttons|=0x4000;
        const int x=int(held(ModernAction::menu_right))-int(held(ModernAction::menu_left));
        const int y=int(held(ModernAction::menu_up))-int(held(ModernAction::menu_down));
        if(x)out.x=static_cast<std::int8_t>(80*x);
        if(y)out.y=static_cast<std::int8_t>(80*y);
    } else {
        constexpr std::array<ModernAction,7> actions{ModernAction::aim,ModernAction::fire,
            ModernAction::jump,ModernAction::crouch,ModernAction::interact,
            ModernAction::weapon_previous,ModernAction::weapon_next};
        constexpr std::array<std::uint16_t,7> masks{0x10,0x2000,8,4,0x8000,0x20,0x4000};
        for(std::size_t i=0;i<actions.size();++i)if(held(actions[i]))out.buttons|=masks[i];
    }
    return out;
}
// Boss arenas move laterally with native C-left/C-right while their analog
// stick controls the reticle. Modern look already has a separate recorded
// channel, so consume movement here without also steering the aiming stick.
inline ControllerPortSample map_modern_boss_actions(const ModernActionSample &input) {
    auto out=map_modern_actions(input,false);
    if(input.move.x<0)out.buttons|=0x2;
    if(input.move.x>0)out.buttons|=0x1;
    out.x=out.y=0;
    return out;
}
// Toggle aim resets on context/focus/device changes. Re-entry while held needs
// a release before another press; menus always use their own bindings.
class ModernAimLatch {
    bool held_=false,latched_=false;
public:
    void reset(){held_=latched_=false;}
    void cancel(bool held){held_=held;latched_=false;}
    void apply(ModernActionSample &input,bool toggle,bool gameplay) {
        const auto bit=modern_bit(ModernAction::aim);const bool down=(input.held&bit)!=0;
        if(!gameplay){cancel(down);input.held&=~bit;return;}
        if(toggle && down && !held_)latched_=!latched_;
        held_=down;
        if(!toggle){latched_=false;return;}
        if(latched_)input.held|=bit;else input.held&=~bit;
    }
};
// Only rising scope-action edges queue a step. Opposite directions cancel.
// Context loss discards pending changes; commands never become jump/crouch.
class ModernScopeZoom {
    unsigned held_=0;
    std::array<unsigned,8> pending_{};
    std::size_t count_=0;
    std::uint64_t context_=0;
public:
    void reset(){held_=0;count_=0;context_=0;}
    void observe(std::uint8_t flags,bool scoped,std::uint64_t context) {
        const unsigned held=flags&7U;
        if(!scoped || !(flags&kModernActionsFlag)){held_=held;count_=0;context_=0;return;}
        if(context_!=context){count_=0;context_=context;}
        const unsigned pressed=held&~held_;held_=held;
        if((pressed&6U)==6U)return;
        const unsigned command=(pressed&1U)?1U:(pressed&2U)?2U:(pressed&4U)?4U:0U;
        if(command && count_<pending_.size())pending_[count_++]=command;
    }
    float take(float current) {
        if(!std::isfinite(current) || current<1 || current>5){count_=0;return current;}
        for(std::size_t i=0;i<count_;++i) {
            if(pending_[i]==1)current=current<3?3.0F:current<5?5.0F:1.0F;
            else if(pending_[i]==2)current=current<3?3.0F:5.0F;
            else current=current>3?3.0F:1.0F;
        }
        count_=0;
        return current;
    }
};
} // namespace jfg
