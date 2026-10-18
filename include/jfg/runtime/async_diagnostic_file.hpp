#pragma once
#include "jfg/runtime/async_bounded_trace.hpp"
#include <array>
#include <limits>
#include <ostream>
#include <streambuf>
namespace jfg {
// Optional diagnostic bytes are copied to a bounded worker. flush() submits
// buffered bytes; finish() drains at shutdown. No filesystem work in overflow
// or sync, and the worker owns no game memory or producer stream state.
class AsyncDiagnosticFile final : public std::ostream {
 class Buffer final : public std::streambuf {
 public:
  Buffer() { setp(bytes_.data(),bytes_.data()+bytes_.size()); }
  bool start(const std::filesystem::path& path,std::ios::openmode mode) {
   return writer_.start(std::make_unique<Sink>(path,mode),
       (std::numeric_limits<std::size_t>::max)(),4U*1024U*1024U);
  }
  void finish() noexcept { (void)sync(); writer_.finish(); }
  bool failed() const noexcept {return writer_.failed()||writer_.dropped()!=0U;}
  std::uint64_t dropped() const noexcept {return writer_.dropped();}
 protected:
  int sync() override {
   const auto count=pptr()-pbase();
   if(count==0)return failed()?-1:0;
   const bool ok=writer_.append_bytes(std::string(pbase(),static_cast<std::size_t>(count)));
   setp(bytes_.data(),bytes_.data()+bytes_.size());return ok?0:-1;
  }
  int_type overflow(int_type value) override {
   if(sync()!=0)return traits_type::eof();
   if(!traits_type::eq_int_type(value,traits_type::eof())){*pptr()=traits_type::to_char_type(value);pbump(1);}
   return traits_type::not_eof(value);
  }
 private:
  class Sink final:public AsyncBoundedTrace::Sink {
  public:
   Sink(std::filesystem::path path,std::ios::openmode mode):path_(std::move(path)),mode_(mode){}
   bool open(std::uintmax_t& size) override {size=0;stream_.open(path_,mode_|std::ios::out);return bool(stream_);}
   bool append(std::string_view bytes) override {stream_.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));return bool(stream_);}
   bool flush() override {stream_.flush();return bool(stream_);}
  private:
   std::filesystem::path path_;std::ios::openmode mode_;std::ofstream stream_;
  };
  std::array<char,4096> bytes_{};
  AsyncBoundedTrace writer_;
 };
public:
 AsyncDiagnosticFile():std::ostream(nullptr){rdbuf(&buffer_);}
 ~AsyncDiagnosticFile(){finish();}
 void open(const std::filesystem::path& path,std::ios::openmode mode=std::ios::out) {
  if(open_||!buffer_.start(path,mode)){setstate(std::ios::failbit);return;}open_=true;clear();
 }
 bool is_open() const noexcept{return open_;}
 bool failed() const noexcept{return buffer_.failed();}
 std::uint64_t dropped() const noexcept{return buffer_.dropped();}
 explicit operator bool() const noexcept{return open_&&!buffer_.failed()&&good();}
 bool operator!() const noexcept{return !static_cast<bool>(*this);}
 void finish() noexcept {if(open_){buffer_.finish();open_=false;}}
private:
 Buffer buffer_;bool open_=false;
};
}
