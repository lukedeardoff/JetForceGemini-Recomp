#pragma once
#include <cstddef>
#include <string_view>
namespace jfg {
inline constexpr std::size_t kControllerDeviceCount=20;
inline bool valid_controller_device_key(std::string_view key) {
  if(key.size()!=68 || !key.starts_with("sdl:"))return false;
  for(char c:key.substr(4))if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
  return true;
}
} // namespace jfg
