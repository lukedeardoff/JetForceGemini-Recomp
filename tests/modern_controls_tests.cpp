#include "jfg/runtime/modern_controls.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace jfg;
namespace {
int checks=0;
void check(bool ok,const char *message){++checks;if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
constexpr auto index(ModernAction a){return static_cast<std::size_t>(a);}
}
int main(int argc,char **argv) {
    if(argc==2 && std::string_view(argv[1])=="--emit-default-profile") {
        ModernControlsConfig c;c.enabled=true;std::cout<<serialize_modern_controls(c);return 0;
    }
    if(argc!=1)return 2;
    ModernControlsConfig config,parsed;
    check(config.look.mouse_invert_y==1 && config.look.aim_invert_y==0,"modern mouse defaults to vertical inversion without changing gamepad");
    check(PcInputConfig{}.mouse_invert_y==0,"original mouse mapping defaults remain unchanged");
    auto savedNonInverted=config;savedNonInverted.look.mouse_invert_y=0;
    check(parse_modern_controls(serialize_modern_controls(savedNonInverted),parsed) && parsed.look.mouse_invert_y==0,"explicit non-inverted modern preference survives loading");
    {
        ModernControlsConfig legacy;legacy.enabled=true;legacy.look.mouse_invert_y=0;
        legacy.keys[index(ModernAction::menu_confirm)]='F';
        auto text=serialize_modern_controls(legacy);
        for(auto [action,key]:std::array<std::pair<ModernAction,int>,4>{{
            {ModernAction::menu_up,38},{ModernAction::menu_down,40},
            {ModernAction::menu_left,37},{ModernAction::menu_right,39}}}) {
            const std::string field="key."+std::string(kModernActions[index(action)].id)+"=";
            const auto begin=text.find(field)+field.size(),end=text.find('\n',begin);
            text.replace(begin,end-begin,std::to_string(key));
        }
        ModernControlsConfig migrated;
        check(parse_modern_controls(text,migrated),"legacy arrow profile loads");
        check(serialize_modern_controls(migrated)==serialize_modern_controls(legacy),"only arrows removed from legacy profile");
        for(int key=37;key<=40;++key) {
            check(rebind_modern_action(migrated,true,ModernAction::fire,key)==ModernBindResult::invalid,"new arrow binding rejected");
            auto raw=legacy;raw.keys[index(ModernAction::fire)]=key;
            check(sample_modern_keyboard(raw,[&](int pressed){return pressed==key;}).held==0,"raw arrow binding cannot fire");
        }
        check(rebind_modern_action(migrated,true,ModernAction::menu_confirm,'E')==ModernBindResult::changed &&
            migrated.keys[index(ModernAction::interact)]=='E',"shared E confirm preserves Interact");
        const auto sample=sample_modern_keyboard(migrated,[](int key){return key=='E';});
        check(map_modern_actions(sample,true).buttons==0x8000 && map_modern_actions(sample,false).buttons==0x8000,"shared E operates by gameplay/menu context");
        check(parse_modern_controls(serialize_modern_controls(migrated),parsed) && parsed.keys[index(ModernAction::menu_confirm)]=='E' &&
            parsed.keys[index(ModernAction::interact)]=='E',"duplicate E survives save and reload");
    }
    const auto defaults=serialize_modern_controls(config);
    check(!config.enabled,"new profiles must preserve original mapping mode");
    check(parse_modern_controls(defaults,parsed) && serialize_modern_controls(parsed)==defaults,"default profile roundtrip");
    config.enabled=true;config.aim_toggle=true;config.glyph_style=ModernGlyphStyle::playstation;
    config.pad[index(ModernAction::jump)]=10;config.keys[index(ModernAction::fire)]='F';
    config.look.aim_sensitivity=173;config.look.rumble_strength=40;
    const auto custom=serialize_modern_controls(config);
    check(parse_modern_controls(custom,parsed) && serialize_modern_controls(parsed)==custom,"custom profile and independent tuning survive roundtrip");
    std::string crlf;for(char c:custom){if(c=='\n')crlf+='\r';crlf+=c;}
    check(parse_modern_controls(crlf,parsed),"CRLF profiles");
    const auto invalid=[&](std::string text){const auto before=serialize_modern_controls(parsed);
        check(!parse_modern_controls(text,parsed) && before==serialize_modern_controls(parsed),"reject malformed profile atomically");};
    invalid("");invalid(std::string(8193,'x'));invalid(custom+"enabled=0\n");invalid(custom+"unknown=0\n");
    invalid(custom.substr(custom.find('\n')+1));
    for(auto pair:std::array<std::pair<std::string,std::string>,10>{{
        {"version=5","version=6"},{"enabled=1","enabled=2"},{"aim_toggle=1","aim_toggle=-1"},
        {"pad.jump=10","pad.jump=27"},{"key.fire=70","key.fire=258"},
        {"movement_stick=0","movement_stick=2"},{"movement_deadzone=7849","movement_deadzone=32768"},
        {"axis_threshold=8689","axis_threshold=0"},{"look.modern=1","look.modern=0"},
        {"look.rumble_strength=40","look.rumble_strength=101"}}}) {
        auto text=custom;auto n=text.find(pair.first);check(n!=text.npos,"invalid fixture field exists");text.replace(n,pair.first.size(),pair.second);invalid(text);
    }
    check(!ModernControlsConfig{}.mouse_original_acceleration,"original acceleration defaults off");
    auto accelerated=config;accelerated.mouse_original_acceleration=true;
    check(parse_modern_controls(serialize_modern_controls(accelerated),parsed) && parsed.mouse_original_acceleration,"original acceleration saves and reloads enabled");
    auto version4=defaults;version4.replace(0,9,"version=4");
    version4.replace(version4.find("key.menu_back=81"),16,"key.menu_back=8");
    check(parse_modern_controls(version4,parsed) && parsed.keys[index(ModernAction::menu_back)]=='Q',"old Backspace default migrates to Q");
    auto moveQ=version4;moveQ.replace(moveQ.find("key.move_left=65"),16,"key.move_left=81");
    check(parse_modern_controls(moveQ,parsed) && parsed.keys[index(ModernAction::menu_back)]==8,
        "Q movement binding prevents Back migration conflict");
    check(modern_binding_conflicts(ModernAction::move_left,ModernAction::menu_back),
        "movement and menu actions share a context");
    auto customBack=version4;customBack.replace(customBack.find("key.menu_back=8"),15,"key.menu_back=70");
    check(parse_modern_controls(customBack,parsed) && parsed.keys[index(ModernAction::menu_back)]=='F',"custom Back binding preserved");
    auto takenQ=version4;takenQ.replace(takenQ.find("key.menu_confirm=69"),19,"key.menu_confirm=81");
    check(parse_modern_controls(takenQ,parsed) && parsed.keys[index(ModernAction::menu_back)]==8,"migration does not steal Q");
    auto version3=defaults;version3.replace(0,9,"version=3");
    for(const std::string field:{"pad.menu_save=2\n","key.menu_save=116\n"}) {
        const auto at=version3.find(field);check(at!=version3.npos,"legacy save fixture field");
        version3.erase(at,field.size());
    }
    check(parse_modern_controls(version3,parsed) && parsed.keys[index(ModernAction::menu_save)]==0x74 &&
        parsed.pad[index(ModernAction::menu_save)]==2,"legacy profile receives separate Save defaults");
    auto saveConflict=version3;
    saveConflict.replace(saveConflict.find("pad.menu_confirm=0"),18,"pad.menu_confirm=2");
    saveConflict.replace(saveConflict.find("key.menu_confirm=69"),19,"key.menu_confirm=116");
    check(parse_modern_controls(saveConflict,parsed) && parsed.pad[index(ModernAction::menu_save)]==-1 &&
        parsed.keys[index(ModernAction::menu_save)]==0,"migration preserves conflicting custom menu bindings");
    invalid(version3+"key.menu_save=116\n");
    auto missingSave=defaults;missingSave.erase(missingSave.find("pad.menu_save=2\n"),16);invalid(missingSave);
    auto version2=version3;version2.replace(0,9,"version=2");
    const std::string accelerationField="mouse_original_acceleration=0\n";
    version2.erase(version2.find(accelerationField),accelerationField.size());
    check(parse_modern_controls(version2,parsed) && !parsed.mouse_original_acceleration,"v2 profiles default original acceleration off");
    invalid(version2+accelerationField);
    auto missingAcceleration=defaults;missingAcceleration.erase(missingAcceleration.find(accelerationField),accelerationField.size());invalid(missingAcceleration);
    for(const std::string bad:{"-1","2"}){auto text=defaults;text.replace(text.find(accelerationField),accelerationField.size(),"mouse_original_acceleration="+bad+"\n");invalid(text);}
    // Strict v4 persistence and exact backwards-compatible v1/v2/v3 tuning.
    auto version1=version2;
    version1.replace(0,std::string("version=3").size(),"version=1");
    for(const std::string field:{"mouse_scope_sensitivity=100\n","pad_scope_sensitivity=100\n"})
        version1.erase(version1.find(field),field.size());
    check(parse_modern_controls(version1,parsed) && parsed.mouse_scope_sensitivity==100 && parsed.pad_scope_sensitivity==100 && !parsed.mouse_original_acceleration,"v1 profiles retain original scope feel");
    invalid(version1+"mouse_scope_sensitivity=100\n");
    config.mouse_scope_sensitivity=55;config.pad_scope_sensitivity=175;
    check(parse_modern_controls(serialize_modern_controls(config),parsed) &&
        parsed.mouse_scope_sensitivity==55 && parsed.pad_scope_sensitivity==175,"device-specific scope tuning roundtrips");
    auto missingScope=defaults;missingScope.erase(missingScope.find("pad_scope_sensitivity="),std::string("pad_scope_sensitivity=100\n").size());invalid(missingScope);
    for(int bad:{-1,0,9,301}){auto badConfig=config;badConfig.mouse_scope_sensitivity=bad;invalid(serialize_modern_controls(badConfig));}
    check(!modern_scope_sensitivity_active(false,10),"scope tuning requires scoped aim");
    check(!modern_scope_sensitivity_active(true,3),"scope tuning leaves airborne camera alone");
    check(!modern_scope_sensitivity_active(true,4),"scope tuning leaves swimming camera alone");
    check(modern_scope_sensitivity_active(true,10)&&modern_scope_sensitivity_active(true,11),"ground sniper aiming gets scope tuning");
    ModernScopeSensitivity tuning;
    check(tuning.take({200,-100},true,50,1)==PcLookDelta{100,-50},"scope tuning scales both axes");
    check(tuning.take({200,-100},false,50,1)==PcLookDelta{200,-100},"ordinary aim and camera ignore scope tuning");
    int fine=0;for(int i=0;i<100;++i)fine+=tuning.take({1,0},true,10,1).x;
    check(fine==10,"fine scope motion accumulates without loss");
    tuning.clear();tuning.take({1,0},true,50,1);
    check(tuning.take({-1,0},true,50,1).x==0,"opposite fractions cancel");
    tuning.clear();tuning.take({1,0},true,50,1);
    check(tuning.take({1,0},true,50,2).x==0,"actor or device mode change clears fractions");
    tuning.clear();tuning.take({1,0},true,50,1);
    check(tuning.take({1,0},true,10,1).x==0,"sensitivity change clears fractions");
    tuning.clear();check(tuning.take({32767,-32767},true,300,1)==PcLookDelta{32767,-32767},"scope deltas remain recordable");
    check(tuning.take({0,0},true,300,1)==PcLookDelta{},"clamping does not leave delayed motion");
    tuning.clear();tuning.take({1,0},true,50,1);tuning.take({},false,50,1);
    check(tuning.take({1,0},true,50,1).x==0,"leaving scope clears fractions");
    PcScopeMotion magnification;
    const auto tuned=tuning.take({100,-100},true,50,1);
    check(magnification.take(tuned,5,1)==PcLookDelta{10,-10},"scope multiplier composes with zoom-aware scaling");
    config={};
    check(modern_action("scope_cycle")==ModernAction::scope_cycle && !modern_action("C_up"),"semantic action lookup");
    check(!modern_binding_conflicts(ModernAction::jump,ModernAction::menu_confirm),"gameplay/menu reuse allowed");
    check(modern_binding_conflicts(ModernAction::jump,ModernAction::fire),"same-context conflict detected");
    check(modern_pad_name(0,ModernGlyphStyle::xbox)=="A" && modern_pad_name(0,ModernGlyphStyle::playstation)=="Cross","native face-button names");
    check(modern_pad_name(8,ModernGlyphStyle::playstation)=="R3" && modern_pad_name(23,ModernGlyphStyle::playstation)=="L2","native stick and trigger names");
    check(resolve_modern_glyph_style(ModernGlyphStyle::automatic,ControllerFamily::dualsense)==ModernGlyphStyle::playstation,"DualSense resolves native glyphs");
    check(resolve_modern_glyph_style(ModernGlyphStyle::xbox,ControllerFamily::dualsense)==ModernGlyphStyle::xbox,"explicit glyph override wins without changing device identity");
    check(modern_pad_name(0,ModernGlyphStyle::automatic)=="South button","unknown controller has neutral labels");
    StandardControllerSample pad;
    pad.buttons[0]=true;
    auto sample=sample_modern_pad(config,pad);
    check(map_modern_actions(sample,false).buttons==8,"A/Cross jumps");
    check(map_modern_actions(sample,true).buttons==0x8000,"A/Cross confirms without jumping in menus");
    // Boss combat keeps the configured actions and separates strafe from aim.
    pad={};pad.axes[0]=-32767;pad.axes[1]=-32767;pad.axes[5]=32767;
    auto boss_input=sample_modern_pad(config,pad);
    auto boss=map_modern_boss_actions(boss_input);
    check(boss.buttons==0x2002 && boss.x==0 && boss.y==0,"boss left stick strafes without moving the reticle; RT fires");
    check(map_modern_actions(boss_input,false).x<0 && !(map_modern_actions(boss_input,false).buttons&3),"ordinary movement remains analog");
    pad={};pad.axes[0]=32767;pad.buttons[0]=true;
    boss=map_modern_boss_actions(sample_modern_pad(config,pad));
    check(boss.buttons==9 && boss.x==0 && boss.y==0,"boss right strafe combines with configured Jump");
    pad={};pad.axes[0]=config.movement_deadzone-1;
    check(map_modern_boss_actions(sample_modern_pad(config,pad)).buttons==0,"boss movement retains the configured physical deadzone");
    auto boss_keys=config;boss_keys.keys[index(ModernAction::move_left)]='J';
    boss_keys.keys[index(ModernAction::fire)]='F';boss_keys.keys[index(ModernAction::jump)]='K';
    boss=map_modern_boss_actions(sample_modern_keyboard(boss_keys,[](int key){return key=='J'||key=='F'||key=='K';}));
    check(boss.buttons==0x200A && boss.x==0 && boss.y==0,"remapped keyboard boss movement, fire and jump compose");
    boss=map_modern_boss_actions(sample_modern_keyboard(config,[](int key){return key=='A'||key=='D';}));
    check(boss.buttons==0,"opposing keyboard movement cancels boss strafe");
    pad={};pad.buttons[1]=true;sample=sample_modern_pad(config,pad);
    check(map_modern_actions(sample,false).buttons==4 && map_modern_actions(sample,true).buttons==0x4000,"B/Circle crouch or back by context");
    pad={};pad.buttons[2]=true;
    check(map_modern_actions(sample_modern_pad(config,pad),false).buttons==0x8000,"interaction has a dedicated mask");
    check(map_modern_actions(sample_modern_pad(config,pad),true,true).buttons==0x2000,"X/Square saves only in a validated pause root");
    check(map_modern_actions(sample_modern_pad(config,pad),true,false).buttons==0,"Save cannot emit Z in other menus");
    pad={};pad.buttons[9]=true;
    check(map_modern_actions(sample_modern_pad(config,pad),false).buttons==0x20,"previous weapon does not interact");
    pad={};pad.axes[4]=pad.axes[5]=32767;sample=sample_modern_pad(config,pad);
    check(map_modern_actions(sample,false).buttons==0x2010,"both triggers aim and fire");
    pad={};pad.buttons[8]=true;sample=sample_modern_pad(config,pad);
    check(map_modern_actions(sample,false).buttons==0 && modern_scope_flags(sample.held)==0x81,"scope command never becomes jump or crouch");
    pad={};pad.axes[0]=20000;pad.axes[3]=-20000;sample=sample_modern_pad(config,pad);
    check(sample.move.x>0 && sample.move.y==0 && !sample.physically_neutral,"look does not leak into movement");
    check(pc_stick_look(config.look,pad,0).y>0,"right stick remains independent look");
    config.movement_stick=1;sample=sample_modern_pad(config,pad);
    check(sample.move.x==0 && sample.move.y>0,"southpaw movement binding");
    config={};config.pad[index(ModernAction::jump)]=10;pad={};pad.buttons[0]=true;
    check(!(map_modern_actions(sample_modern_pad(config,pad),false).buttons&8),"rebind removes old gameplay input");
    pad={};pad.buttons[10]=true;
    check(map_modern_actions(sample_modern_pad(config,pad),false).buttons&8,"rebound gameplay input works");
    config.pad[index(ModernAction::jump)]=-1;
    check(!(map_modern_actions(sample_modern_pad(config,pad),false).buttons&8),"explicit unbind works");
    config={};std::array<bool,258> keys{};
    const auto keyboard=[&](){return sample_modern_keyboard(config,[&](int k){return keys[static_cast<std::size_t>(k)];});};
    check(config.keys[index(ModernAction::menu_confirm)]=='E',"E defaults to modern confirm");
    check(config.keys[index(ModernAction::menu_back)]=='Q',"Q is the discoverable Back default");
    keys['Q']=true;check(map_modern_actions(keyboard(),true).buttons==0x4000,"Q backs out");
    keys={};keys['A']=true;check(map_modern_actions(keyboard(),true).x==-80,"A navigates menus left");
    keys={};keys['D']=true;check(map_modern_actions(keyboard(),true).x==80,"D navigates menus right");
    keys={};keys['W']=true;check(map_modern_actions(keyboard(),true).y==80,"W navigates menus up");
    keys={};keys['S']=true;check(map_modern_actions(keyboard(),true).y==-80,"S navigates menus down");
    keys['W']=true;check(map_modern_actions(keyboard(),true).y==0,"opposite movement directions cancel in menus");
    keys={};keys['A']=keys[39]=true;
    check(map_modern_actions(keyboard(),true).x==-80,"arrow keys cannot override movement navigation");
    config.keys[index(ModernAction::menu_right)]='L';keys['L']=true;
    check(map_modern_actions(keyboard(),true).x==80,"explicit non-arrow menu direction takes precedence");
    config.keys[index(ModernAction::move_left)]='J';keys={};keys['J']=true;
    check(map_modern_actions(keyboard(),true).x==-80,"rebound movement keys navigate menus");
    config={};keys={};
    keys['E']=true;
    check(map_modern_actions(keyboard(),true).buttons==0x8000,"E confirms in menus");
    check(map_modern_actions(keyboard(),false).buttons==0x8000,"E still interacts in gameplay");
    keys={};keys[13]=true;
    check(map_modern_actions(keyboard(),true).buttons==0,"Enter is no longer the modern default confirm");
    auto explicitEnter=config;explicitEnter.keys[index(ModernAction::menu_confirm)]=13;
    check(parse_modern_controls(serialize_modern_controls(explicitEnter),parsed) &&
        parsed.keys[index(ModernAction::menu_confirm)]==13,"explicit saved Enter binding remains supported");
    keys={};
    keys[0x74]=true;
    check(map_modern_actions(keyboard(),true,true).buttons==0x2000,"F5 saves in pause root");
    check(map_modern_actions(keyboard(),true,false).buttons==0 && map_modern_actions(keyboard(),false,true).buttons==0,"F5 has no gameplay or unrelated menu side effect");
    keys={};
    keys['W']=keys[32]=true;sample=keyboard();
    check(sample.move.y==80 && map_modern_actions(sample,false).buttons==8,"KBM uses same action semantics");
    keys['S']=true;check(keyboard().move.y==0,"opposite keyboard directions cancel");
    keys={};keys[4]=true;check(modern_scope_flags(keyboard().held)==0x81,"middle mouse scope");
    keys={};keys[kPcWheelUp]=true;
    check(map_modern_actions(keyboard(),false).buttons==0x4000,"wheel next weapon remains separately bound");
    ModernAimLatch aim;
    auto aimSample=[](bool down){ModernActionSample s;s.held=down?modern_bit(ModernAction::aim):0;return s;};
    auto held=aimSample(true);aim.apply(held,true,true);check(held.held&modern_bit(ModernAction::aim),"toggle enters aim");
    held=aimSample(true);aim.apply(held,true,true);check(held.held&modern_bit(ModernAction::aim),"held toggle input does not repeat");
    held=aimSample(false);aim.apply(held,true,true);check(held.held&modern_bit(ModernAction::aim),"toggle survives release");
    held=aimSample(true);aim.apply(held,true,true);check(!(held.held&modern_bit(ModernAction::aim)),"second press leaves aim");
    held=aimSample(true);aim.apply(held,true,false);held=aimSample(true);aim.apply(held,true,true);
    check(!(held.held&modern_bit(ModernAction::aim)),"menu-to-game held aim cannot relatch");
    ModernScopeZoom zoom;
    zoom.observe(0x81,true,1);check(zoom.take(1)==3,"dedicated scope cycle advances");
    zoom.observe(0x81,true,1);check(zoom.take(3)==3,"held scope input does not repeat");
    zoom.observe(0x80,true,1);zoom.observe(0x81,true,1);check(zoom.take(3)==5,"cycle reaches maximum");
    zoom.observe(0x80,true,1);zoom.observe(0x81,true,1);check(zoom.take(5)==1,"cycle wraps");
    zoom.observe(0x80,true,1);zoom.observe(0x84,true,1);check(zoom.take(1)==1,"decrease clamps at minimum");
    zoom.observe(0x80,true,1);zoom.observe(0x86,true,1);check(zoom.take(3)==3,"opposite scope commands cancel");
    zoom.observe(0x80,true,1);zoom.observe(0x81,true,1);zoom.observe(0,false,0);check(zoom.take(1)==1,"context loss drops queued zoom");
    zoom.observe(0x81,false,0);zoom.observe(0x81,true,2);check(zoom.take(1)==1,"holding zoom before aiming cannot trigger later");
    check(std::isnan(zoom.take(std::numeric_limits<float>::quiet_NaN())),"invalid guest scope values are not overwritten");

    config={};
    const auto beforeConflict=serialize_modern_controls(config);
    check(rebind_modern_action(config,false,ModernAction::jump,1)==ModernBindResult::conflict &&
        serialize_modern_controls(config)==beforeConflict,"conflict does not partially apply");
    check(rebind_modern_action(config,false,ModernAction::jump,1,ModernBindPolicy::replace_conflicts)==ModernBindResult::changed &&
        config.pad[index(ModernAction::crouch)]==-1 && config.pad[index(ModernAction::menu_back)]==1,
        "replace clears only overlapping gameplay action");
    check(rebind_modern_action(config,false,ModernAction::fire,1,ModernBindPolicy::keep_both)==ModernBindResult::changed &&
        config.pad[index(ModernAction::jump)]==1,"explicit keep-both preserves overlap");
    check(rebind_modern_action(config,true,ModernAction::jump,258)==ModernBindResult::invalid,"UI command validates binding range");
    check(rebind_modern_action(config,false,ModernAction::jump,-1)==ModernBindResult::changed,"UI command supports clearing");

    config={};keys={};keys['W']=true;
    check(map_modern_actions(keyboard(),true).y==80,"keyboard movement also navigates menus");
    keys={};keys[38]=true;check(map_modern_actions(keyboard(),true).y==0,"arrow input is not menu navigation");
    config.keys[index(ModernAction::menu_up)]='U';keys['U']=true;
    check(map_modern_actions(keyboard(),true).y==80,"dedicated non-arrow keyboard menu navigation works");
    check(rebind_modern_action(config,false,ModernAction::move_forward,9)==ModernBindResult::invalid,"pad editor rejects unsupported digital movement rows");
    auto badPad=config;badPad.pad[index(ModernAction::move_forward)]=9;
    check(!parse_modern_controls(serialize_modern_controls(badPad),parsed),"unsupported pad rows reject atomically");
    check(modern_binding_conflicts(ModernAction::interact,ModernAction::scope_cycle),"interaction and zoom overlap while scoped");
    ControllerMapping original;original.bindings[0]=3;
    const auto originalText=serialize_controller_mapping(original);
    auto oldKeyboard=pc_keyboard_preset(true);oldKeyboard.keys[2]='F';
    const auto keyboardText=serialize_pc_input(oldKeyboard);
    auto changed=config;changed.enabled=true;changed.pad[index(ModernAction::jump)]=9;
    check(serialize_controller_mapping(original)==originalText && serialize_pc_input(oldKeyboard)==keyboardText,"independent modern profile preserves mapping profiles");
    std::cout<<checks<<" modern controls checks passed\n";
}
