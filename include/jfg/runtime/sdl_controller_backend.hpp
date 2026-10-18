#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "jfg/runtime/controller_catalog.hpp"
#include <windows.h>
#include <bcrypt.h>
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include <SDL_version.h>
#include <filesystem>
#include <unordered_set>
#include <cstring>
#pragma comment(lib,"bcrypt.lib")

namespace jfg {
inline ControllerFamily controller_family_from_sdl(SDL_GameControllerType type) {
  switch(type) {
    case SDL_CONTROLLER_TYPE_XBOX360:
    case SDL_CONTROLLER_TYPE_XBOXONE:return ControllerFamily::xbox;
    case SDL_CONTROLLER_TYPE_PS3:
    case SDL_CONTROLLER_TYPE_PS4:return ControllerFamily::playstation;
    case SDL_CONTROLLER_TYPE_PS5:return ControllerFamily::dualsense;
    default:return ControllerFamily::generic;
  }
}
// SDL is already shipped beside the runtime. Resolve its controller API from
// that explicit directory; never search the current working directory or PATH.
class SdlControllerBackend {
public:
  SdlControllerBackend() {
    FILETIME stamp{};GetSystemTimeAsFileTime(&stamp);
    session_=std::to_string(GetCurrentProcessId())+":"+std::to_string(stamp.dwHighDateTime)+":"+std::to_string(stamp.dwLowDateTime);
  }
  SdlControllerBackend(const SdlControllerBackend&)=delete;
  SdlControllerBackend& operator=(const SdlControllerBackend&)=delete;
  ~SdlControllerBackend(){close();}
  bool open(const std::filesystem::path& directory) {
    if(module_)return initialized_;
    if(directory.empty() || !directory.is_absolute())return false;
    const auto now=GetTickCount64();
    if(directory==attempted_ && now<retry_)return false;
    attempted_=directory;retry_=now+2000;
    module_=LoadLibraryExW((directory/L"SDL2.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module_)return false;
#define JFG_SDL_LOAD(name) name=reinterpret_cast<decltype(name)>(GetProcAddress(module_,#name));if(!name){close();return false;}
    JFG_SDL_LOAD(SDL_GetVersion)
    SDL_version version{};SDL_GetVersion(&version);
    if(version.major!=2 || version.minor<24){close();return false;}
    JFG_SDL_LOAD(SDL_InitSubSystem)
    JFG_SDL_LOAD(SDL_QuitSubSystem)
    JFG_SDL_LOAD(SDL_GameControllerUpdate)
    JFG_SDL_LOAD(SDL_NumJoysticks)
    JFG_SDL_LOAD(SDL_IsGameController)
    JFG_SDL_LOAD(SDL_JoystickGetDeviceGUID)
    JFG_SDL_LOAD(SDL_GameControllerPathForIndex)
    JFG_SDL_LOAD(SDL_JoystickGetDeviceInstanceID)
    JFG_SDL_LOAD(SDL_GameControllerOpen)
    JFG_SDL_LOAD(SDL_GameControllerClose)
    JFG_SDL_LOAD(SDL_GameControllerGetJoystick)
    JFG_SDL_LOAD(SDL_JoystickGetGUID)
    JFG_SDL_LOAD(SDL_JoystickGetGUIDString)
    JFG_SDL_LOAD(SDL_GameControllerGetAttached)
    JFG_SDL_LOAD(SDL_GameControllerName)
    JFG_SDL_LOAD(SDL_GameControllerPath)
    JFG_SDL_LOAD(SDL_GameControllerGetSerial)
    JFG_SDL_LOAD(SDL_GameControllerTypeForIndex)
    JFG_SDL_LOAD(SDL_GameControllerGetButton)
    JFG_SDL_LOAD(SDL_GameControllerGetAxis)
    JFG_SDL_LOAD(SDL_GameControllerEventState)
    JFG_SDL_LOAD(SDL_JoystickEventState)
#undef JFG_SDL_LOAD
    if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)!=0){close();return false;}
    initialized_=true;
    // This backend polls state; prevent an unconsumed controller event queue.
    controller_events_=SDL_GameControllerEventState(SDL_QUERY);
    joystick_events_=SDL_JoystickEventState(SDL_QUERY);
    SDL_GameControllerEventState(SDL_DISABLE);
    SDL_JoystickEventState(SDL_DISABLE);
    next_discovery_=0;
    return true;
  }
  static std::filesystem::path executable_directory() {
    std::wstring path(32768,L'\0');const auto size=GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size()));
    if(!size || size>=path.size())return {};
    path.resize(size);return std::filesystem::path(path).parent_path();
  }
  std::vector<ControllerDeviceInfo> poll() {
    std::vector<ControllerDeviceInfo> result;
    if(!initialized_)return result;
    SDL_GameControllerUpdate();
    for(auto it=handles_.begin();it!=handles_.end();) {
      if(!SDL_GameControllerGetAttached(it->pad)){SDL_GameControllerClose(it->pad);it=handles_.erase(it);}
      else ++it;
    }
    const auto now=GetTickCount64();
    if(now>=next_discovery_) {
      next_discovery_=now+500;
      std::unordered_set<SDL_JoystickID> present;
      const int count=std::clamp(SDL_NumJoysticks(),0,128);
      for(int index=0;index<count;++index) {
        const auto id=SDL_JoystickGetDeviceInstanceID(index);if(id<0)continue;present.insert(id);
        if(skipped_.contains(id) || std::any_of(handles_.begin(),handles_.end(),[&](const auto& h){return h.id==id;}))continue;
        if(!SDL_IsGameController(index))continue;
        // Xbox devices use the existing XInput backend exclusively. Decide
        // before Open: SDL's XInput Open can stop an already-running vibration.
        const auto type=SDL_GameControllerTypeForIndex(index);
        const auto device_guid=SDL_JoystickGetDeviceGUID(index);
        const auto device_path=bounded(SDL_GameControllerPathForIndex(index));
        // SDL 2.26's Windows XInput driver tags GUID byte 14 with 'x' and
        // uses XInput#N paths, even when a remapper reports another vendor.
        if(device_guid.data[14]=='x' || device_path.starts_with("XInput#") ||
           type==SDL_CONTROLLER_TYPE_XBOX360 || type==SDL_CONTROLLER_TYPE_XBOXONE) {
          skipped_.insert(id);continue;
        }
        auto* pad=SDL_GameControllerOpen(index);if(!pad)continue;
        auto path=bounded(SDL_GameControllerPath(pad));
        char guid[33]{};SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(SDL_GameControllerGetJoystick(pad)),guid,33);
        const auto serial=bounded(SDL_GameControllerGetSerial(pad));
        const bool transient=path.empty() && serial.empty();
        const auto identity=std::string(guid)+(path.empty()?serial.empty()?"|session|"+session_+":"+std::to_string(id):"|serial|"+serial:"|path|"+path);
        auto key=hash_identity(identity);
        if(key.empty() || handles_.size()>=kControllerDeviceCount-4) {SDL_GameControllerClose(pad);continue;}
        handles_.push_back({id,pad,std::move(key),bounded(SDL_GameControllerName(pad)),transient,controller_family_from_sdl(type)});
      }
      for(auto it=skipped_.begin();it!=skipped_.end();)if(!present.contains(*it))it=skipped_.erase(it);else ++it;
    }
    for(const auto& h:handles_) {
      if(!SDL_GameControllerGetAttached(h.pad))continue;
      ControllerDeviceInfo device;device.key=h.key;device.name=h.name;device.connected=true;device.session_only=h.transient;device.family=h.family;
      for(int i=0;i<15;++i)device.sample.buttons[static_cast<std::size_t>(i)]=SDL_GameControllerGetButton(h.pad,static_cast<SDL_GameControllerButton>(i))!=0;
      for(int i=0;i<6;++i)device.sample.axes[static_cast<std::size_t>(i)]=SDL_GameControllerGetAxis(h.pad,static_cast<SDL_GameControllerAxis>(i));
      result.push_back(std::move(device));
    }
    return result;
  }
  bool available()const{return initialized_;}
private:
  struct Handle {SDL_JoystickID id;SDL_GameController* pad;std::string key,name;bool transient;ControllerFamily family;};
  static std::string bounded(const char* text) {return text?std::string(text,strnlen_s(text,4096)):std::string{};}
  static std::string hash_identity(const std::string& identity) {
    BCRYPT_ALG_HANDLE algorithm=nullptr;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
    std::array<unsigned char,32> bytes{};
    const auto status=BCryptHash(algorithm,nullptr,0,reinterpret_cast<PUCHAR>(const_cast<char*>(identity.data())),static_cast<ULONG>(identity.size()),bytes.data(),static_cast<ULONG>(bytes.size()));
    BCryptCloseAlgorithmProvider(algorithm,0);if(status<0)return {};
    constexpr char hex[]="0123456789abcdef";std::string key="sdl:";key.reserve(68);
    for(unsigned char value:bytes){key+=hex[value>>4];key+=hex[value&15];}return key;
  }
  void close() {
    for(const auto& h:handles_)if(SDL_GameControllerClose)SDL_GameControllerClose(h.pad);
    handles_.clear();skipped_.clear();
    if(initialized_) {
      SDL_GameControllerEventState(controller_events_);SDL_JoystickEventState(joystick_events_);
      SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);initialized_=false;
    }
    if(module_){FreeLibrary(module_);module_=nullptr;}
  }
#define JFG_SDL_POINTER(name) decltype(&::name) name=nullptr;
  JFG_SDL_POINTER(SDL_GetVersion)
  JFG_SDL_POINTER(SDL_InitSubSystem)
  JFG_SDL_POINTER(SDL_QuitSubSystem)
  JFG_SDL_POINTER(SDL_GameControllerUpdate)
  JFG_SDL_POINTER(SDL_NumJoysticks)
  JFG_SDL_POINTER(SDL_IsGameController)
  JFG_SDL_POINTER(SDL_JoystickGetDeviceGUID)
  JFG_SDL_POINTER(SDL_GameControllerPathForIndex)
  JFG_SDL_POINTER(SDL_JoystickGetDeviceInstanceID)
  JFG_SDL_POINTER(SDL_GameControllerOpen)
  JFG_SDL_POINTER(SDL_GameControllerClose)
  JFG_SDL_POINTER(SDL_GameControllerGetJoystick)
  JFG_SDL_POINTER(SDL_JoystickGetGUID)
  JFG_SDL_POINTER(SDL_JoystickGetGUIDString)
  JFG_SDL_POINTER(SDL_GameControllerGetAttached)
  JFG_SDL_POINTER(SDL_GameControllerName)
  JFG_SDL_POINTER(SDL_GameControllerPath)
  JFG_SDL_POINTER(SDL_GameControllerGetSerial)
  JFG_SDL_POINTER(SDL_GameControllerTypeForIndex)
  JFG_SDL_POINTER(SDL_GameControllerGetButton)
  JFG_SDL_POINTER(SDL_GameControllerGetAxis)
  JFG_SDL_POINTER(SDL_GameControllerEventState)
  JFG_SDL_POINTER(SDL_JoystickEventState)
#undef JFG_SDL_POINTER
  HMODULE module_=nullptr;
  bool initialized_=false;
  int controller_events_=SDL_ENABLE,joystick_events_=SDL_ENABLE;
  std::filesystem::path attempted_;
  std::string session_;
  ULONGLONG retry_=0,next_discovery_=0;
  std::vector<Handle> handles_;
  std::unordered_set<SDL_JoystickID> skipped_;
};
} // namespace jfg
