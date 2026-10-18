#pragma once
#include "jfg/runtime/async_diagnostic_file.hpp"
#include <array>
#include <chrono>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <span>
#include <vector>
#include <string>
#include <string_view>

namespace jfg::boot {
namespace detail {
enum class TraceFormatError { none, record_limit, format_error };
struct TraceFormatResult {
  std::size_t length = 0;
  TraceFormatError error = TraceFormatError::none;
};
// Preserve the private TSV schema and its conservative per-value bound. Only
// the returned bytes are initialized; no terminator is written to the stream.
inline TraceFormatResult format_gameplay_trace_row(std::array<char, 2048>& row,
    std::int64_t microseconds, const char* kind, std::span<const std::uint64_t> values) {
  auto formatted = std::to_chars(row.data(), row.data() + row.size(), microseconds);
  if (formatted.ec != std::errc{}) return {0, TraceFormatError::format_error};
  char* cursor = formatted.ptr;
  *cursor++ = '\t';
  const char* name = kind ? kind : "(null)";
  for (std::size_t i = 0; i < 32U && name[i] != '\0'; ++i) *cursor++ = name[i];
  for (const auto value : values) {
    if (static_cast<std::size_t>(cursor - row.data()) + 19U >= row.size())
      return {0, TraceFormatError::record_limit};
    *cursor++ = '\t';
    formatted = std::to_chars(cursor, row.data() + row.size(), value, 16);
    if (formatted.ec != std::errc{}) return {0, TraceFormatError::format_error};
    cursor = formatted.ptr;
  }
  *cursor++ = '\n';
  return {static_cast<std::size_t>(cursor - row.data()), TraceFormatError::none};
}
} // namespace detail
// Explicit, private diagnostics. Disabled by default; never part of support ZIPs.
class GameplayTrace {
public:
  ~GameplayTrace() { finish(); }
  bool open(const std::filesystem::path &path,
            std::uint64_t limit = 256ULL * 1024ULL * 1024ULL,
            bool memory_only = false) {
    if (memory_open_ || stream_.is_open() || stopped_) return false;
    if (memory_only) {
      if (limit < 128U || limit > 256ULL*1024ULL*1024ULL ||
          memory_open_ || stream_.is_open()) return false;
      try { memory_.reserve(static_cast<std::size_t>(limit)); }
      catch (...) { return false; }
      std::ofstream check(path, std::ios::binary | std::ios::trunc);
      if (!check) return false;
      memory_path_ = path; memory_open_ = true; limit_ = limit;
      start_ = flushed_ = std::chrono::steady_clock::now();
      const std::string_view header="jfg-gameplay-trace-v1\ttime_us\tevent\tvalues_hex\n";
      memory_.insert(memory_.end(),header.begin(),header.end()); bytes_=50U;
      return true;
    }
    if (limit < 128U || stream_.is_open()) return false;
    stream_.open(path, std::ios::binary | std::ios::trunc);
    if (!stream_) return false;
    limit_ = limit;
    start_ = flushed_ = std::chrono::steady_clock::now();
    stream_ << "jfg-gameplay-trace-v1\ttime_us\tevent\tvalues_hex\n";
    bytes_ = 50U; // Conservative header charge.
    return true;
  }
  explicit operator bool() const { return (memory_open_ || stream_.is_open()) && !stopped_; }
  // finish() drains at shutdown; flush() only submits buffered diagnostics.
  void finish() noexcept {
    if (memory_open_) {
      try {
        std::ofstream output(memory_path_,std::ios::binary | std::ios::trunc);
        output.write(memory_.data(),static_cast<std::streamsize>(memory_.size()));
        output.flush();
        memory_write_failed_ = !output;
      } catch (...) { memory_write_failed_ = true; }
      memory_open_=false;
      if(memory_write_failed_)std::fputs("gameplay memory trace: final write failed\n",stderr);
    }
    stream_.finish();
  }
  bool memory_write_failed() const noexcept { return memory_write_failed_; }
  void flush() { if (stream_.is_open()) stream_.flush(); }
  void event(const char *kind, std::span<const std::uint64_t> values) {
    if (!*this) return;
    const auto now = std::chrono::steady_clock::now();
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - start_).count();
    std::array<char, 2048> row;
    const auto formatted = detail::format_gameplay_trace_row(row,
        static_cast<std::int64_t>(us), kind, values);
    if (formatted.error != detail::TraceFormatError::none) {
      stop(formatted.error == detail::TraceFormatError::record_limit ? "record-limit" : "format-error");
      return;
    }
    const auto used = static_cast<std::streamsize>(formatted.length);
    if (bytes_ + static_cast<std::uint64_t>(used) + 64U > limit_) { stop("byte-limit"); return; }
    if (memory_open_) {
      memory_.insert(memory_.end(),row.data(),row.data()+formatted.length);
      bytes_ += static_cast<std::uint64_t>(used);
      return;
    }
    stream_.write(row.data(), used);
    bytes_ += static_cast<std::uint64_t>(used);
    if (!stream_) { stopped_ = true; return; }
    if (now - flushed_ >= std::chrono::seconds(1)) { flush(); flushed_ = now; }
  }
  void event(const char *kind, std::initializer_list<std::uint64_t> values) {
    event(kind, std::span<const std::uint64_t>(values.begin(), values.size()));
  }
private:
  void stop(const char *reason) {
    if (memory_open_) {
      const std::string marker=std::string("limit\t")+reason+'\n';
      memory_.insert(memory_.end(),marker.begin(),marker.end()); stopped_=true;
      return;
    }
    stream_ << "limit\t" << reason << '\n';
    flush(); stopped_ = true;
  }
  std::filesystem::path memory_path_;
  std::vector<char> memory_;
  bool memory_open_=false, memory_write_failed_=false;
  jfg::AsyncDiagnosticFile stream_;
  std::chrono::steady_clock::time_point start_{}, flushed_{};
  std::uint64_t bytes_ = 0, limit_ = 0;
  bool stopped_ = false;
};
} // namespace jfg::boot
