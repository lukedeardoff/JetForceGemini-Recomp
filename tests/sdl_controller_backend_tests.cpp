#include "jfg/runtime/sdl_controller_backend.hpp"
#include <iostream>
#include <cstdlib>
namespace {int checks=0;void check(bool value,const char* reason){++checks;if(!value){std::cerr<<reason<<": "<<SDL_GetError()<<'\n';std::exit(1);}}}
int main(){
  check(jfg::controller_family_from_sdl(SDL_CONTROLLER_TYPE_PS5)==jfg::ControllerFamily::dualsense,"SDL DualSense identification");
  check(jfg::controller_family_from_sdl(SDL_CONTROLLER_TYPE_PS4)==jfg::ControllerFamily::playstation,"SDL PlayStation identification");
  check(jfg::controller_family_from_sdl(SDL_CONTROLLER_TYPE_XBOXONE)==jfg::ControllerFamily::xbox,"SDL Xbox identification");
  check(jfg::controller_family_from_sdl(SDL_CONTROLLER_TYPE_UNKNOWN)==jfg::ControllerFamily::generic,"unknown family fallback");
  // Isolate this test from real hardware. It never opens a window or sends input.
  for(const char* hint:{"SDL_XINPUT_ENABLED","SDL_JOYSTICK_HIDAPI","SDL_JOYSTICK_RAWINPUT","SDL_JOYSTICK_WGI",SDL_HINT_DIRECTINPUT_ENABLED})SDL_SetHint(hint,"0");
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
  check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,"SDL initialization");
  SDL_GameControllerEventState(SDL_ENABLE);SDL_JoystickEventState(SDL_ENABLE);
  SDL_VirtualJoystickDesc desc{};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
  desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;desc.naxes=6;desc.nbuttons=15;
  desc.vendor_id=0x1234;desc.product_id=0x5678;desc.name="JFG isolated virtual pad";
  desc.button_mask=(1U<<15)-1;desc.axis_mask=(1U<<6)-1;
  int index=SDL_JoystickAttachVirtualEx(&desc);check(index>=0,"virtual attachment");
  SDL_Joystick* joystick=SDL_JoystickOpen(index);check(joystick!=nullptr,"virtual handle");
  SDL_JoystickSetVirtualAxis(joystick,4,-32768);SDL_JoystickSetVirtualAxis(joystick,5,-32768);
  {
    jfg::SdlControllerBackend backend;
    check(!backend.open("relative-path"),"relative DLL lookup accepted");
    check(backend.open(jfg::SdlControllerBackend::executable_directory()),"backend initialization");
    auto found=backend.poll();check(found.size()==1,"virtual controller enumeration");
    const auto key=found[0].key;check(jfg::valid_controller_device_key(key),"device identity invalid");
    check(found[0].session_only,"virtual identity pretends to survive process restart");
    check(found[0].name==desc.name,"device name lost");
    check(found[0].family==jfg::ControllerFamily::generic,"virtual device must not pretend to be DualSense");
    SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1);
    SDL_JoystickSetVirtualAxis(joystick,0,23000);SDL_JoystickSetVirtualAxis(joystick,3,-21000);
    SDL_JoystickSetVirtualAxis(joystick,5,32767);
    found=backend.poll();check(found.size()==1&&found[0].sample.buttons[0],"button input lost");
    check(found[0].sample.axes[0]==23000&&found[0].sample.axes[3]==-21000,"dual-stick axes lost");
    check(found[0].sample.axes[4]==0&&found[0].sample.axes[5]==32767,"triggers not normalized");
    SDL_JoystickClose(joystick);check(SDL_JoystickDetachVirtual(index)==0,"virtual detach");
    check(backend.poll().empty(),"detached controller retained");
    index=SDL_JoystickAttachVirtualEx(&desc);check(index>=0,"virtual reattach");
    const auto replacementId=SDL_JoystickGetDeviceInstanceID(index);
    Sleep(550);found=backend.poll();
    check(found.size()==1&&found[0].key!=key,"unidentifiable replacement stole saved identity");
    check(!found[0].sample.buttons[0]&&found[0].sample.axes[0]==0,"replacement retained held input");
    int currentIndex=-1;
    for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_JoystickGetDeviceInstanceID(i)==replacementId)currentIndex=i;
    check(currentIndex>=0&&SDL_JoystickIsVirtual(currentIndex),"replacement identity lost");
    check(SDL_JoystickDetachVirtual(currentIndex)==0,"final virtual detach");
    check(backend.poll().empty(),"final detach retained input");
  }
  check(SDL_GameControllerEventState(SDL_QUERY)==SDL_ENABLE&&SDL_JoystickEventState(SDL_QUERY)==SDL_ENABLE,"event state not restored");

  SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
  std::cout<<checks<<" SDL backend checks passed\n";
}
