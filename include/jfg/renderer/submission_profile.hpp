#pragma once
#include <array>
#include <cstdint>
#include <chrono>
namespace jfg {
struct SubmissionWaitCounters {std::array<std::uint64_t,6> ns{},calls{};};
inline thread_local bool submission_wait_profile_active=false;
inline thread_local SubmissionWaitCounters submission_wait_profile_counters{};
inline void record_submission_wait(unsigned stage,std::chrono::steady_clock::time_point begin) noexcept {
 const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-begin).count();
 if(submission_wait_profile_active && stage<6 && ns>=0){submission_wait_profile_counters.ns[stage]+=static_cast<std::uint64_t>(ns);++submission_wait_profile_counters.calls[stage];}
}
// One synchronous submission owner. Nested scopes preserve the outer collector;
// worker threads have independent counters. No allocation, locks or file writes.
struct SubmissionProfileScope {
 bool previous=false,enabled=false;SubmissionWaitCounters before{};
 explicit SubmissionProfileScope(bool selected):previous(submission_wait_profile_active),enabled(selected) {
  if(enabled){before=submission_wait_profile_counters;submission_wait_profile_active=true;}
 }
 SubmissionProfileScope(const SubmissionProfileScope&)=delete;
 SubmissionProfileScope& operator=(const SubmissionProfileScope&)=delete;
 ~SubmissionProfileScope(){submission_wait_profile_active=previous;}
 SubmissionWaitCounters delta() const {
  SubmissionWaitCounters result{};
  if(enabled)for(unsigned i=0;i<6;++i){result.ns[i]=submission_wait_profile_counters.ns[i]-before.ns[i];result.calls[i]=submission_wait_profile_counters.calls[i]-before.calls[i];}
  return result;
 }
};
}
