#include "jfg/boot/gameplay_trace.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>
namespace {
// Independent oracle: the original printf-based format and limit condition.
jfg::boot::detail::TraceFormatResult reference_row(std::array<char, 2048>& row,
    std::int64_t timestamp, const char* kind, std::span<const std::uint64_t> values) {
  using Error = jfg::boot::detail::TraceFormatError;
  int used = std::snprintf(row.data(), row.size(), "%lld\t%.32s",
      static_cast<long long>(timestamp), kind);
  if (used < 0) return {0, Error::format_error};
  for (const auto value : values) {
    if (static_cast<std::size_t>(used) + 19U >= row.size()) return {0, Error::record_limit};
    const int count = std::snprintf(row.data() + used, row.size() - static_cast<std::size_t>(used),
        "\t%llx", static_cast<unsigned long long>(value));
    if (count < 0) return {0, Error::format_error};
    used += count;
  }
  row[static_cast<std::size_t>(used++)] = '\n';
  return {static_cast<std::size_t>(used), Error::none};
}
bool format_differential() {
  std::array<char, 2048> expected, actual;
  std::vector<std::uint64_t> values(1100U);
  const std::array<std::int64_t, 7> times{0, 1, -1, 99999999,
      (std::numeric_limits<std::int64_t>::min)(),
      (std::numeric_limits<std::int64_t>::max)(), 12345678901234LL};
  std::uint64_t random = 0x123456789abcdefULL;
  for (unsigned i = 0; i < 12000U; ++i) {
    for (auto& value : values) {
      random ^= random << 13U; random ^= random >> 7U; random ^= random << 17U;
      value = i % 4U == 0U ? 0U : i % 4U == 1U ?
          (std::numeric_limits<std::uint64_t>::max)() : random;
    }
    std::string kind(i % 65U, 'x');
    if (i % 3U == 0U) kind = "actor";
    const std::size_t count = i % 7U == 0U ? 1100U : i % 145U;
    expected.fill(static_cast<char>(0x55)); actual.fill(static_cast<char>(0xaa));
    const auto a = reference_row(expected, times[i % times.size()], kind.c_str(),
        {values.data(), count});
    const auto c = jfg::boot::detail::format_gameplay_trace_row(actual,
        times[i % times.size()], kind.c_str(), {values.data(), count});
    if (a.error != c.error || a.length != c.length ||
        std::memcmp(expected.data(), actual.data(), a.length) != 0) return false;
  }
  return true;
}
} // namespace
int main(int argc, char **argv) {
  if (argc != 2) return 1;
  if (!format_differential()) return 7;
  jfg::boot::GameplayTrace trace;
  trace.event("disabled", {1, 2});
  if (trace || trace.open(argv[1], 64U)) return 2;
  if (!trace.open(argv[1], 256U)) return 3;
  trace.event("actor", {1, 2, 0x80100000U});
  for (unsigned i = 0; i < 100; ++i) trace.event("tick", {i, 9, 8, 7, 6, 5, 4});
  trace.finish();
  std::ifstream input(argv[1], std::ios::binary);
  const std::string text{std::istreambuf_iterator<char>(input), {}};
  if (text.size() > 256U || text.find("\tactor\t1\t2\t80100000\n") == std::string::npos) return 4;
  if (trace || text.find("limit\tbyte-limit\n") == std::string::npos) return 5;
  const auto size = text.size();
  trace.event("after-limit", {1}); trace.flush();
  if (std::filesystem::file_size(argv[1]) != size) return 6;
  const auto memory_path=std::filesystem::path(argv[1]).string()+".memory";
  {
    jfg::boot::GameplayTrace memory;
    if(!memory.open(memory_path,256U,true) || memory.open(memory_path,256U))return 8;
    memory.event("memory-row",{0x123U,0x456U});memory.flush();
    if(std::filesystem::file_size(memory_path)!=0U)return 9;
    for(unsigned i=0;i<100;++i)memory.event("tick",{i,9,8,7,6,5,4});
    if(memory)return 10;
    if(std::filesystem::file_size(memory_path)!=0U)return 11;
    memory.finish();memory.finish();
    if(memory.memory_write_failed() || memory)return 12;
  }
  std::ifstream stored(memory_path,std::ios::binary);
  const std::string contents{std::istreambuf_iterator<char>(stored),{}};
  if(contents.size()>256U || contents.find("\tmemory-row\t123\t456\n")==std::string::npos ||
     contents.find("limit\tbyte-limit\n")==std::string::npos)return 13;
  const auto destructor_path=memory_path+".destructor";
  {
    jfg::boot::GameplayTrace memory;
    if(!memory.open(destructor_path,256U,true))return 14;
    memory.event("destructor",{77U});
  }
  std::ifstream destructor_file(destructor_path,std::ios::binary);
  const std::string destructor_text{std::istreambuf_iterator<char>(destructor_file),{}};
  if(destructor_text.find("\tdestructor\t4d\n")==std::string::npos)return 15;
  return 0;
}
