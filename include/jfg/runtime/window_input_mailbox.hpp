#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
#include <mutex>
#include "jfg/runtime/menu_pointer.hpp"
namespace jfg {
class WindowInputMailbox {
public:
  struct Motion {int x=0,y=0,wheel=0;bool focus_lost=false;MenuPointerFrame pointer{};};
  void pointer(int x,int y,int width,int height,int button=0,bool down=false) {
    const std::lock_guard lock(mutex_);
    pointer_.moved |= !pointer_.valid || pointer_.x!=x || pointer_.y!=y ||
                      pointer_.width!=width || pointer_.height!=height;
    pointer_.x=x;pointer_.y=y;pointer_.width=width;pointer_.height=height;
    pointer_.valid=width>0&&height>0&&x>=0&&y>=0&&x<width&&y<height;
    if(button==1||button==2) {
      if(down&&!down_[static_cast<std::size_t>(button)]) {
        pressed_[static_cast<std::size_t>(button)]=true;
        pointer_.press_x=x;pointer_.press_y=y;
        if(button==1)pointer_.left_press=true;else pointer_.right_press=true;
      }
      down_[static_cast<std::size_t>(button)]=down;
    }
  }
  void pointer_leave() {const std::lock_guard lock(mutex_);pointer_={};}
  void motion(int x,int y){const std::lock_guard lock(mutex_);motion_.x=std::clamp(motion_.x+std::clamp(x,-4096,4096),-4096,4096);motion_.y=std::clamp(motion_.y+std::clamp(y,-4096,4096),-4096,4096);}
  void wheel(int delta){const std::lock_guard lock(mutex_);motion_.wheel=std::clamp(motion_.wheel+std::clamp(delta,-3840,3840),-3840,3840);}
  Motion take_motion(){const std::lock_guard lock(mutex_);const auto result=motion_;motion_={};return result;}
  void key(std::size_t code, bool down, bool repeat = false) {
    if (code >= down_.size()) return;
    const std::lock_guard lock(mutex_);
    // A held key may repeat after focus returns without a new press.
    // Only a fresh key-down may acquire a key cleared by lose_focus().
    if (down && repeat && !down_[code]) return;
    if (down && !down_[code]) pressed_[code] = true;
    down_[code] = down;
  }
  void lose_focus() {
    const std::lock_guard lock(mutex_);
    motion_={};motion_.focus_lost=true;pointer_={};
    down_.fill(false);
    pressed_.fill(false);
  }
  void close() {
    const std::lock_guard lock(mutex_);
    close_ = true;
  }
  void consume(std::array<bool,256>& down, std::array<bool,256>& pressed, bool& close) {
    const std::lock_guard lock(mutex_);
    consume_locked(down, pressed, close);
  }
  // Keys and focus/motion must describe the same instant. Separate reads can
  // pair an old neutral key state with a new focus-return click notification.
  Motion consume_frame(std::array<bool,256>& down, std::array<bool,256>& pressed, bool& close, bool consume_pointer = true) {
    const std::lock_guard lock(mutex_);
    consume_locked(down, pressed, close);
    auto result = motion_;
    result.pointer=pointer_;
    // Window/audio servicing may drain motion between controller polls.
    // Only the controller consumer acknowledges pointer edges.
    if (consume_pointer) pointer_.moved=pointer_.left_press=pointer_.right_press=false;
    motion_ = {};
    return result;
  }
private:
  void consume_locked(std::array<bool,256>& down, std::array<bool,256>& pressed, bool& close) {
    down = down_;
    if (motion_.focus_lost) pressed.fill(false);
    for (std::size_t i = 0; i < down.size(); ++i) pressed[i] = pressed[i] || pressed_[i];
    pressed_.fill(false);
    close = close || close_;
  }
  std::mutex mutex_;
  std::array<bool,256> down_{}, pressed_{};
  bool close_ = false;
  Motion motion_{};
  MenuPointerFrame pointer_{};
};
}
