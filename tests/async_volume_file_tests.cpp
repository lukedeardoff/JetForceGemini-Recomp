#include "jfg/runtime/async_volume_file.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
void check(bool pass, const char* why) {
  if (!pass) { std::cerr << why << '\n'; std::abort(); }
}
void write(const std::filesystem::path& path, const std::string& text) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file << text; file.close(); check(bool(file), "write fixture");
}
bool await_value(jfg::AsyncVolumeFile& reader, const std::string& expected) {
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);
  do {
    auto value=reader.latest(); if(value && *value==expected)return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  } while(std::chrono::steady_clock::now()<end);
  return false;
}
}
int main() {
  const auto nonce=std::chrono::steady_clock::now().time_since_epoch().count();
  const auto path=std::filesystem::temp_directory_path() /
    ("jfg-volume-"+std::to_string(nonce)+".ini");
  check(!std::filesystem::exists(path), "fresh volume fixture");
  jfg::AsyncVolumeFile reader;
  reader.start(path); check(!reader.latest(), "missing file retains no value");
  const std::string first="version=1\nvolume=0\nmuted=1\n";
  write(path,first);check(await_value(reader,first), "new settings become visible");
  const auto snapshot=reader.latest();
  const std::string boundary(128,'x');write(path,boundary);
  check(await_value(reader,boundary), "128-byte boundary is accepted");
  check(*snapshot==first, "published snapshot remains immutable");
  write(path,std::string(129,'y'));
  std::this_thread::sleep_for(std::chrono::milliseconds(350));
  check(*reader.latest()==boundary, "oversized file preserves last bounded value");
  std::error_code error;check(std::filesystem::remove(path,error)&&!error,"remove owned fixture");
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  check(*reader.latest()==boundary, "missing file preserves last value");
  reader.finish();reader.finish();
  std::cout << "asynchronous volume file checks passed\n";
}
