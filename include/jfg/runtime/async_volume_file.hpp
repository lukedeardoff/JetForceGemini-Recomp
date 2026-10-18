#pragma once
#if defined(_WIN32)
#include <windows.h>
#else
#include <fstream>
#endif
#include <array>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
namespace jfg {
class AsyncVolumeFile {
public:
  ~AsyncVolumeFile() { finish(); }
  void start(std::filesystem::path path) {
    path_ = std::move(path);
    refresh(); // Called before the first audio buffer can play.
    worker_ = std::thread([this] {
      std::unique_lock lock(mutex_);
      while (!stop_) {
        if (changed_.wait_for(lock,std::chrono::milliseconds(100),[this]{return stop_;})) break;
        lock.unlock(); refresh(); lock.lock();
      }
    });
  }
  std::shared_ptr<const std::string> latest() {
    std::lock_guard lock(mutex_); return latest_;
  }
  void finish() noexcept {
    if (!worker_.joinable()) return;
    {std::lock_guard lock(mutex_);stop_=true;changed_.notify_one();}
    worker_.join();
  }
private:
  void refresh() noexcept {
    try {
      std::array<char,129> buffer{};
      std::size_t count=0;
#if defined(_WIN32)
      const HANDLE file = CreateFileW(path_.c_str(), GENERIC_READ,
          FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,
          FILE_ATTRIBUTE_NORMAL,nullptr);
      if (file == INVALID_HANDLE_VALUE) return;
      DWORD windows_count=0;
      const bool read=ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&windows_count,nullptr)!=0;
      CloseHandle(file); if (!read) return;
      count=windows_count;
#else
      std::ifstream file(path_,std::ios::binary);
      if (!file) return;
      file.read(buffer.data(),static_cast<std::streamsize>(buffer.size()));
      if (file.bad()) return;
      count=static_cast<std::size_t>(file.gcount());
#endif
      if (count>128U) return;
      auto value=std::make_shared<const std::string>(buffer.data(),count);
      std::lock_guard lock(mutex_); latest_=std::move(value);
    } catch (...) {}
  }
  std::filesystem::path path_;
  std::mutex mutex_;
  std::condition_variable changed_;
  std::shared_ptr<const std::string> latest_;
  bool stop_=false;
  std::thread worker_;
};
}
