#include "jfg/runtime/menu_pointer.hpp"
#include "jfg/runtime/window_input_mailbox.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
namespace {
int checks=0;
void check(bool ok,const char* message){++checks;if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
}
int main(){
  using namespace jfg;
  MenuPointerFrame f{960,540,1920,1080,true,true,false,false};
  auto p=menu_pointer_point(f,4,3);
  check(p.valid&&p.x==160&&p.y==120,"pillarbox center");
  f.x=239;check(!menu_pointer_point(f,4,3).valid,"left black bar");
  f.x=1680;check(!menu_pointer_point(f,4,3).valid,"right black bar");
  f={400,10,800,800,true};check(!menu_pointer_point(f,4,3).valid,"top black bar");
  f={400,400,800,800,true};p=menu_pointer_point(f,4,3);
  check(p.x==160&&p.y==120,"letterbox center");
  f={1000,562,2000,1125,true};p=menu_pointer_point(f,4,3);
  check(p.valid&&p.x==160&&p.y>119&&p.y<121,"125 percent DPI scaling");
  check(!menu_pointer_point(f,0,3).valid,"invalid aspect");
  f.width=0;check(!menu_pointer_point(f,4,3).valid,"minimized viewport");
  GuestMenuState wideOptions{GuestMenuKind::options,0,255,true,789,8};
  // Actual window pixels -> displayed VI -> logical menu coordinates. Include
  // nested letterboxing from the regression, ultrawide bars and odd DPI sizes.
  for (const auto size : {std::array<int,2>{1920,1080}, {1440,1080},
                          {3440,1440}, {1399,787}, {2000,1125}}) {
    const double scale=std::min(double(size[0])/320,double(size[1])/180);
    const double left=(size[0]-320*scale)*.5,top=(size[1]-180*scale)*.5;
    for(int i=0;i<8;++i) {
      const double x=(i&1)?234:86,y=70+34*(i/2);
      MenuPointerFrame window{int(left+x*scale),int(top+y*.75*scale),
                              size[0],size[1],true};
      check(menu_pointer_target(wideOptions,menu_pointer_point(window,320,180))==i,
            "widescreen Options hover matches displayed icon after resize/DPI");
    }
  }
  f={960,10,1920,1200,true};
  check(!menu_pointer_point(f,320,180).valid,"widescreen top bar rejects clicks");
  GuestMenuState start{GuestMenuKind::start,0,5,true,42};
  check(menu_pointer_target(start,{88,190,true})==0,"Play icon");
  check(menu_pointer_target(start,{160,190,true})==-1,"disabled Multi has no target");
  check(menu_pointer_target(start,{232,220,true})==2,"Options label");
  check(menu_pointer_target(start,{130,190,true})==-1,"space between targets");
  check(menu_pointer_target(start,{88,160,true})==-1,"above start icons");
  start.enabled=7;check(menu_pointer_target(start,{160,190,true})==1,"available Multi");
  GuestMenuState saves{GuestMenuKind::saves,0,63,true,45};
  for(int i=0;i<6;++i)check(menu_pointer_target(saves,{87.f+29.f*static_cast<float>(i),186,true})==i,"six slot targets");
  check(menu_pointer_target(saves,{102,186,true})==-1,"slot gap");
  check(menu_pointer_target(saves,{150,120,true})==0,"current game card");
  saves.selected=5;check(menu_pointer_target(saves,{150,120,true})==5,"card follows selected slot");
  saves.ready=false;check(menu_pointer_target(saves,{87,186,true})==-1,"transition not clickable");
  saves.ready=true;saves.selected=0;
  GuestMenuState actions{GuestMenuKind::save_actions,0,1,true,45};
  check(menu_pointer_target(actions,{105,186,true})==0,"Play checkmark target");
  check(menu_pointer_target(actions,{150,120,true})==0,"Play card target");
  actions.selected=1;
  check(menu_pointer_target(actions,{150,120,true})==-1,"Copy card is not misinterpreted as Play");
  check(menu_pointer_target(actions,{178,186,true})==-1,"unimplemented Copy icon is not clickable");
  GuestMenuPointer pointer;
  auto packet=pointer.sample(saves,{232,186,true},true,true,false,true,false);
  check(!packet.buttons&&packet.target<0,"new menu consumes stale click");
  packet=pointer.sample(saves,{232,186,true},true,false,false,true,false);
  check(packet.target==5&&!packet.buttons,"hover jumps directly from slot one to six");
  for(int i=0;i<4;++i)check(pointer.sample(saves,{232,186,true},false,false,false,true,false).target<0,"stationary cursor does not repeat selection");
  saves.selected=5;
  packet=pointer.sample(saves,{232,186,true},false,false,false,true,false);
  check(!packet.buttons&&packet.target<0,"hover completes without activation");
  packet=pointer.sample(saves,{232,186,true},false,true,false,true,false);
  check(packet.buttons==0x8000,"click activates selected slot");
  for(int i=0;i<10;++i)check(!pointer.sample(saves,{232,186,true},false,false,false,true,false).buttons,"single click emits one edge");
  packet=pointer.sample(saves,{87,186,true},true,true,false,true,false);
  check(packet.target==0&&packet.buttons==0x8000,"distant click selects and activates in one event");
  pointer.sample(saves,{87,186,true},false,false,false,false,false);
  packet=pointer.sample(saves,{87,186,true},false,false,false,true,false);
  check(!packet.buttons&&packet.target<0,"focus loss cancels pending activation");
  packet=pointer.sample(saves,{87,186,true},true,true,false,true,true);
  check(!packet.buttons&&packet.target<0,"keyboard/controller takes precedence");
  pointer.sample(saves,{87,186,true},false,false,false,true,false);
  packet=pointer.sample(saves,{87,186,true},false,false,true,true,false);
  check(packet.buttons==0x4000,"right click goes back");
  pointer.sample(saves,{87,186,true},true,true,false,true,false);
  packet=pointer.sample(saves,{},false,false,false,true,false);
  check(!packet.buttons&&packet.target<0,"leaving viewport cancels pending click");
  saves.identity++;
  packet=pointer.sample(saves,{87,186,true},true,true,false,true,false);
  check(!packet.buttons&&packet.target<0,"reloaded overlay rejects queued click");
  GuestMenuPointer direct;saves.selected=0;
  direct.sample(saves,{203,186,true},false,false,false,true,false);
  packet=direct.sample(saves,{203,186,true},true,false,false,true,false);
  check(packet.target==4&&!packet.buttons,"one to five skips intermediate slots");
  saves.selected=4;
  check(direct.sample(saves,{203,186,true},false,false,false,true,false).target<0,"no queued navigation");
  packet=direct.sample(saves,{203,186,true},false,true,false,true,false);
  check(packet.target==4&&packet.buttons==0x8000,"stationary click activates hovered item");
  check(!direct.sample(saves,{203,186,true},false,false,false,true,false).buttons,"click only once");
  GuestMenuState pause{GuestMenuKind::pause_root,0,383U,true,123};
  check(menu_pointer_target(pause,{78,55,true})==0,"pause character target");
  check(menu_pointer_target(pause,{243,150,true})==-1,"locked eighth icon has no target");
  check(menu_pointer_target(pause,{77,150,true})==6,"pause weapon target");
  check(menu_pointer_target(pause,{160,198,true})==8,"visible Save prompt target");
  GuestMenuPointer pausePointer;
  pausePointer.sample(pause,{160,198,true},true,false,false,true,false);
  packet=pausePointer.sample(pause,{160,198,true},true,false,false,true,false);
  check(packet.target<0&&!packet.buttons,"Save hover never writes or activates");
  packet=pausePointer.sample(pause,{160,198,true},false,true,false,true,false);
  check(packet.target==8&&packet.buttons==0x2000,"Save click routes to native save input");
  pause.enabled&=~256U;
  check(!pausePointer.sample(pause,{160,198,true},false,true,false,true,false).buttons,"save countdown blocks repeat click");
  packet=pausePointer.sample(pause,{77,150,true},true,true,false,true,false);
  check(packet.target==6&&packet.buttons==0x8000,"pause click selects and activates distant icon directly");

  GuestMenuState info{GuestMenuKind::pause_info,0,0,true,456,5};
  GuestMenuConfirm confirm;
  check(confirm.apply(pause,0x8000)==0x8000,"E opens root choice");
  check(confirm.apply(info,0x8000)==0,"held opener does not close Health");
  confirm.apply(info,0);
  check(confirm.apply(info,0x8000)==0x4000,"fresh E dismisses Health");
  check(confirm.apply(pause,0x8000)==0,"held dismissal does not reopen parent");
  confirm.apply(pause,0);
  check(confirm.apply(pause,0x8000)==0x8000,"released E can select next choice");
  confirm.apply(info,0);
  check(confirm.apply(info,0x4000)==0x4000,"Q remains native Back");
  confirm.apply(pause,0);
  confirm.apply(pause,0x8000);
  check(confirm.apply(info,0x8000)==0,"opening E is swallowed on entering informational page");
  check(confirm.apply(pause,0x8000)==0,"returning by mouse while holding opening E never reopens page");
  confirm.apply(info,0);
  GuestMenuState character{GuestMenuKind::pause_character,0,7,true,456};
  check(confirm.apply(character,0x8000)==0x4000,"E confirms character and returns");
  confirm.apply(character,0);
  GuestMenuState map{GuestMenuKind::pause_map,0,0,true,456};
  check(confirm.apply(map,0x8000)==0x8000,"map keeps its travel confirmation");
  confirm.apply(map,0);
  GuestMenuState detail{GuestMenuKind::option_detail,0,0,true,789,1};
  check(confirm.apply(detail,0x8000)==0x4000,"E finishes volume adjustment");
  confirm.apply(detail,0);detail.page=7;
  check(confirm.apply(detail,0x8000)==0x8000,"Quit keeps confirmation semantics");
  GuestMenuPointer cp;cp.sample(character,{80,180,true},false,false,false,true,false);
  packet=cp.sample(character,{80,180,true},true,false,false,true,false);
  check(packet.x==0&&!packet.buttons,"character hover never cycles character");
  packet=cp.sample(character,{80,180,true},false,true,false,true,false);
  check(packet.x==-80&&!packet.buttons,"left character arrow cycles once");
  packet=cp.sample(character,{235,180,true},false,true,false,true,false);
  check(packet.x==80,"right character arrow");
  packet=cp.sample(character,{160,185,true},false,true,false,true,false);
  check(packet.buttons==0x4000,"character label confirms and returns");
  GuestMenuState options{GuestMenuKind::options,0,255,true,789,8};
  for(int i=0;i<8;++i)check(menu_pointer_target(options,{(i&1)?238.f:84.f,70.f+35.f*static_cast<float>(i/2),true})==i,"Options icon target");
  options.enabled&=~8U;
  check(menu_pointer_target(options,{238,105,true})==-1,"disabled Options icon excluded");
  GuestMenuPointer op;op.sample(options,{84,140,true},false,false,false,true,false);
  packet=op.sample(options,{84,140,true},true,true,false,true,false);
  check(packet.target==4&&packet.buttons==0x8000,"Options click selects and activates directly");
  packet=op.sample(detail,{84,140,true},true,true,false,true,false);
  check(!packet.buttons,"queued click discarded on Options page change");
  packet=op.sample(detail,{160,120,true},false,false,true,true,false);
  check(packet.buttons==0x4000,"right click backs out of detail without hit target");
  WindowInputMailbox box;std::array<bool,256> down{},pressed{};bool close=false;
  box.pointer(80,90,640,480,1,true);box.pointer(80,90,640,480,1,false);
  auto input=box.consume_frame(down,pressed,close);
  check(input.pointer.left_press&&pressed[1]&&!down[1],"quick click retains position and edge atomically");
  input=box.consume_frame(down,pressed,close);
  check(input.pointer.valid&&!input.pointer.left_press&&!input.pointer.moved,"position persists without repeated click");
  box.pointer(100,120,640,480,1,true);box.pointer(200,220,640,480);
  input=box.consume_frame(down,pressed,close);
  check(input.pointer.left_press&&input.pointer.press_x==100&&input.pointer.press_y==120&&input.pointer.x==200,"moving after click preserves click target");
  box.pointer(81,90,640,480,2,true);box.lose_focus();
  input=box.consume_frame(down,pressed,close);
  check(input.focus_lost&&!input.pointer.valid&&!input.pointer.right_press&&!down[2],"focus clears pending pointer and buttons");
  box.pointer(81,90,640,480);box.pointer_leave();
  check(!box.consume_frame(down,pressed,close).pointer.valid,"leave invalidates coordinate");
  box.pointer(90,100,640,480,1,true);
  box.pointer(90,100,640,480,1,false);
  check(box.consume_frame(down,pressed,close,false).pointer.left_press,"frame service observes pending click");
  check(box.consume_frame(down,pressed,close,false).pointer.left_press,"repeated window service does not consume click");
  check(box.consume_frame(down,pressed,close).pointer.left_press,"controller poll receives serviced click");
  check(!box.consume_frame(down,pressed,close).pointer.left_press,"controller acknowledges click once");
  box.pointer(91,100,640,480,1,true);
  box.consume_frame(down,pressed,close,false);
  box.lose_focus();
  check(!box.consume_frame(down,pressed,close).pointer.left_press,"focus loss cancels click retained by window service");
  MenuPointerReplay replay;
  std::istringstream script("jfg-menu-pointer-v1\n10,80,90,1\n20,160,120,0\n");
  check(replay.load(script),"diagnostic script loads");
  check(!replay.sample(9).valid,"no input before fixture");
  check(replay.sample(10).left_press,"fixture click edge");
  check(!replay.sample(11).left_press,"fixture does not hold click");
  std::istringstream invalid("jfg-menu-pointer-v1\n10,80,90,1\n9,80,90,1\n");
  check(!replay.load(invalid),"out of order fixture rejected");
  std::istringstream pixels("jfg-menu-pointer-v2\n10,516,315,1,1920,1080\n20,731,432,0,1000,800\n");
  check(replay.load(pixels)&&replay.pixel_coordinates(),"pixel fixture loads with viewport");
  auto pixel=replay.sample(10);
  check(pixel.width==1920&&pixel.height==1080&&pixel.left_press,"pixel fixture retains client extent");
  check(menu_pointer_target(wideOptions,menu_pointer_point(pixel,320,180))==0,"pixel fixture reaches displayed widescreen icon");
  pixel=replay.sample(20);
  check(pixel.width==1000&&pixel.height==800,"pixel fixture supports window resize");
  std::istringstream badSize("jfg-menu-pointer-v2\n10,516,315,1,0,1080\n");
  check(!replay.load(badSize),"pixel fixture rejects minimized extent");
  std::cout<<checks<<" menu pointer checks passed\n";
}
