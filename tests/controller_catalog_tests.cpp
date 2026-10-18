#include "jfg/runtime/controller_catalog.hpp"
#include "jfg/runtime/controller_ports.hpp"
#include "jfg/runtime/pc_input.hpp"
#include <iostream>
#include <cstdlib>
namespace {int checks=0;void check(bool value,const char* reason){++checks;if(!value){std::cerr<<reason<<'\n';std::exit(1);}}}
int main(){
  using namespace jfg;
  const std::string a="sdl:"+std::string(64,'a'),b="sdl:"+std::string(64,'b');
  ControllerMapping original;original.device=-4;original.device_key=a;original.bindings[2]=25;
  ControllerMapping decoded;
  const auto text=serialize_controller_mapping(original);
  check(parse_controller_mapping(text,decoded)&&decoded.device==-4&&decoded.device_key==a&&decoded.bindings[2]==25,"SDL profile round trip");
  ControllerMapping legacy;legacy.device=2;
  check(serialize_controller_mapping(legacy).starts_with("version=1\n"),"legacy profiles remain v1");
  check(parse_controller_mapping(serialize_controller_mapping(legacy),decoded)&&decoded.device==2&&decoded.device_key.empty(),"legacy parse clears SDL identity");
  for(auto bad:{text+"device_key="+a+"\n",text+"unknown=0\n",std::string("version=2\n"),serialize_controller_mapping(legacy)+"device_key="+a+"\n"}){
    decoded=original;check(!parse_controller_mapping(bad,decoded),"malformed profile accepted");
    check(decoded.device_key==a&&decoded.bindings==original.bindings,"malformed profile partially applied");
  }
  for(auto replacement:{std::pair{"device=-4","device=-3"},std::pair{"version=2","version=1"},std::pair{"device_key=sdl:","device_key=SDL:"}}){
    auto bad=text;bad.replace(bad.find(replacement.first),std::string(replacement.first).size(),replacement.second);
    check(!parse_controller_mapping(bad,decoded),"mismatched version or identity accepted");
  }
  ControllerCatalog catalog;
  const int reserved=catalog.reserve(b);
  ControllerDeviceInfo pad{a,"Pad <one>"};pad.connected=true;pad.family=ControllerFamily::dualsense;pad.sample.buttons[0]=true;pad.sample.axes={32767,-32768,14000,0,0,32767};
  catalog.update({pad});const int slot=catalog.find(a);
  check(reserved==4&&slot==5,"saved identity reserved before enumeration");
  check(catalog.get(slot)->family==ControllerFamily::dualsense,"device family retained for UI glyphs");
  check(catalog.get(slot)->connected&&catalog.get(slot)->sample.axes[1]==-32768,"sample preserved");
  std::array<bool,kControllerDeviceCount> connected{};connected[static_cast<std::size_t>(slot)]=true;connected[0]=true;
  ControllerPorts ports;for(auto& m:ports.mappings)m.device=-2;
  auto resolved=original;resolved.device=slot;ports.configure(1,resolved);ControllerMapping automatic;ports.configure(0,automatic);
  auto assigned=ports.devices(connected);
  check(assigned[1]==slot&&assigned[0]==0,"mixed XInput/SDL assignments");
  check(!controller_device_available(2,slot,ports.mappings,assigned,connected),"SDL assignment shared");
  catalog.update({});connected[static_cast<std::size_t>(slot)]=false;assigned=ports.devices(connected);
  check(assigned[1]==slot&&!catalog.get(slot)->connected,"disconnect keeps identity reservation");
  check(catalog.get(slot)->family==ControllerFamily::dualsense,"disconnected summary retains known presentation");
  check(catalog.get(slot)->sample.axes==StandardControllerSample{}.axes&&!catalog.get(slot)->sample.buttons[0],"disconnect clears input");
  check(!controller_device_available(1,slot,ports.mappings,assigned,connected),"offline device offered");
  ControllerDeviceInfo other{b,"Pad two"};other.connected=true;catalog.update({other,pad});
  check(catalog.find(a)==slot&&catalog.find(b)==reserved,"enumeration reorder preserves devices");
  connected[static_cast<std::size_t>(slot)]=true;assigned=ports.devices(connected);
  check(controller_device_available(1,slot,ports.mappings,assigned,connected),"owner cannot reconnect");
  PcDeviceReleaseGate gate;ControllerPortSample raw{0x8000,0,0,true};
  check(gate.blocked(slot,raw,{}),"new SDL assignment leaks held input");
  raw.buttons=0;check(!gate.blocked(slot,raw,{}),"neutral SDL does not unlock");
  raw.connected=false;check(gate.blocked(slot,raw,{}),"disconnected SDL not blocked");
  raw.connected=true;raw.buttons=0x8000;check(gate.blocked(slot,raw,{}),"held input on reconnect leaks");
  raw.buttons=0;check(!gate.blocked(slot,raw,{}),"reconnected neutral SDL stays blocked");
  catalog.update({pad,pad});check(!catalog.get(slot)->connected,"ambiguous identity chooses a pad");
  pad.sample.axes={40000,-40000,0,0,-300,40000};catalog.update({pad});
  check(catalog.get(slot)->sample.axes==std::array<int,6>{32767,-32768,0,0,0,32767},"driver input not bounded");
  check(catalog.get(-1)==nullptr&&catalog.reserve("sdl:bad")==-2,"invalid identity or slot accepted");
  for(char c='0';c<='9';++c)catalog.reserve("sdl:"+std::string(64,c));
  for(char c='c';c<='f';++c)catalog.reserve("sdl:"+std::string(64,c));
  check(catalog.reserve("sdl:"+std::string(63,'a')+'0')==-2,"full catalog reused another identity");
  std::cout<<checks<<" controller catalog checks passed\n";
}
