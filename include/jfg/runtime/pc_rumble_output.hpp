#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

namespace jfg {
// Owns the motor calls on a separate thread so a stalled guest cannot leave
// vibration latched. A frame is a 250 ms lease, invalidated by loss of focus.
// Output and focus callbacks permit verification without a physical controller.
class PcRumbleOutput final {
public:
  using Levels=std::array<std::uint16_t,4>;
  using Output=std::function<bool(unsigned,std::uint16_t)>;
  using Focus=std::function<bool(std::uintptr_t)>;
  PcRumbleOutput(Output output,Focus focus):output_(std::move(output)),focus_(std::move(focus)),thread_([this]{run();}) {}
  PcRumbleOutput(const PcRumbleOutput&)=delete;
  PcRumbleOutput& operator=(const PcRumbleOutput&)=delete;
  ~PcRumbleOutput(){stop();}
  void publish(Levels levels,std::uintptr_t window) {
    std::lock_guard lock(mutex_);
    levels_=levels;window_=window;deadline_=Clock::now()+std::chrono::milliseconds(250);++generation_;
    wake_.notify_one();
  }
  void stop() noexcept {
    {std::lock_guard lock(mutex_);stop_=true;wake_.notify_one();}
    if(thread_.joinable())thread_.join();
  }
private:
  using Clock=std::chrono::steady_clock;
  Output output_;Focus focus_;
  std::mutex mutex_;std::condition_variable wake_;
  Levels levels_{};std::uintptr_t window_=0;Clock::time_point deadline_{};
  std::uint64_t generation_=0;bool stop_=false;
  std::thread thread_;
  void run() noexcept {
    Levels sent{},attempted{};std::array<bool,4> touched{},retry{};
    std::array<Clock::time_point,4> retry_after{};
    std::uint64_t generation=0;bool invalid=true;
    while(true) {
      Levels desired{};bool stopping=false;
      {
        std::unique_lock lock(mutex_);
        wake_.wait_for(lock,std::chrono::milliseconds(8),[&]{return stop_ || generation!=generation_;});
        stopping=stop_;
        if(generation!=generation_) {generation=generation_;invalid=false;}
        invalid=invalid || Clock::now()>=deadline_ || !focus_(window_);
        if(!stopping && !invalid)desired=levels_;
      }
      for(unsigned device=0;device<4;++device) {
        if(stopping && !touched[device])continue;
        if(!stopping && desired[device]==sent[device] && !retry[device])continue;
        if(!stopping && retry[device] && desired[device]==attempted[device] && Clock::now()<retry_after[device])continue;
        touched[device]=true;attempted[device]=desired[device];
        const bool ok=output_(device,desired[device]);
        retry[device]=!ok;
        if(!ok)retry_after[device]=Clock::now()+std::chrono::milliseconds(100);
        if(ok)sent[device]=desired[device];
      }
      if(stopping)break;
    }
  }
};
} // namespace jfg
