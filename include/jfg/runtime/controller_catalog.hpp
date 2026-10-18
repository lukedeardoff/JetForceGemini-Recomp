#pragma once
#include "jfg/runtime/controller_mapping.hpp"
#include "jfg/runtime/controller_device_key.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace jfg {
enum class ControllerFamily { generic, xbox, playstation, dualsense };
struct ControllerDeviceInfo {
  std::string key,name;
  StandardControllerSample sample{};
  bool connected=false,session_only=false;
  ControllerFamily family=ControllerFamily::generic;
};
// Slots 0..3 retain XInput meaning. Further slots are stable for this process;
// profiles persist an identity, never an SDL enumeration/instance index.
class ControllerCatalog {
public:
  int reserve(std::string_view key) {
    if(!valid_controller_device_key(key))return -2;
    if(const int old=find(key);old>=0)return old;
    for(std::size_t i=4;i<entries_.size();++i)if(entries_[i].key.empty()) {
      entries_[i].key=key;entries_[i].name="Saved controller";return static_cast<int>(i);
    }
    return -2;
  }
  int find(std::string_view key) const {
    if(key.empty())return -2;
    for(std::size_t i=4;i<entries_.size();++i)if(entries_[i].key==key)return static_cast<int>(i);
    return -2;
  }
  void update(const std::vector<ControllerDeviceInfo>& devices) {
    for(std::size_t i=4;i<entries_.size();++i){entries_[i].connected=false;entries_[i].sample={};}
    for(const auto& device:devices) {
      if(!device.connected || !valid_controller_device_key(device.key))continue;
      // An ambiguous identity never silently chooses a different physical pad.
      if(std::count_if(devices.begin(),devices.end(),[&](const auto& d){return d.connected&&d.key==device.key;})!=1)continue;
      const int slot=reserve(device.key);if(slot<0)continue;
      auto& entry=entries_[static_cast<std::size_t>(slot)];entry=device;
      if(entry.name.empty())entry.name="Game controller";
      if(entry.name.size()>160) {
        std::size_t end=160;while(end>0 && (static_cast<unsigned char>(entry.name[end])&0xc0)==0x80)--end;
        entry.name.resize(end);
      }
      for(auto& axis:entry.sample.axes)axis=std::clamp(axis,-32768,32767);
      for(std::size_t i=4;i<6;++i)entry.sample.axes[i]=std::max(0,entry.sample.axes[i]);
    }
  }
  const ControllerDeviceInfo* get(int slot) const {
    return slot>=4 && static_cast<std::size_t>(slot)<entries_.size()?&entries_[static_cast<std::size_t>(slot)]:nullptr;
  }
private:
  std::array<ControllerDeviceInfo,kControllerDeviceCount> entries_{};
};
} // namespace jfg
