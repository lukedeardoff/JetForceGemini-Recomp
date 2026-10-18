#include "jfg/renderer/submission_profile.hpp"
#include <chrono>
#include <stdexcept>
#include <thread>
using jfg::SubmissionProfileScope;
static void record(unsigned stage) {
 jfg::record_submission_wait(stage,std::chrono::steady_clock::now()-std::chrono::microseconds(1));
}
int main() {
 if(jfg::submission_wait_profile_active)return 1;
 {SubmissionProfileScope off(false);record(0);if(off.delta().calls[0]!=0)return 2;}
 {
  SubmissionProfileScope outer(true);record(0);
  {SubmissionProfileScope inner(true);record(0);record(1);
   const auto d=inner.delta();if(d.calls[0]!=1 || d.calls[1]!=1 || d.ns[0]<1000)return 3;}
  {SubmissionProfileScope off(false);record(2);if(off.delta().calls[2]!=0)return 4;}
  try {SubmissionProfileScope throwing(true);record(3);throw std::runtime_error("fixture");}
  catch(const std::runtime_error&) {}
  if(!jfg::submission_wait_profile_active)return 5;
  bool child_ok=false;
  std::thread child([&] {
    if(jfg::submission_wait_profile_active)return;
    SubmissionProfileScope owned(true);record(4);
    const auto d=owned.delta();child_ok=d.calls[4]==1 && d.calls[0]==0;
  });child.join();
  if(!child_ok)return 6;
  record(99);
  const auto d=outer.delta();
  if(d.calls[0]!=2 || d.calls[1]!=1 || d.calls[2]!=1 || d.calls[3]!=1 || d.calls[4]!=0)return 7;
 }
 if(jfg::submission_wait_profile_active)return 8;
 {SubmissionProfileScope next(true);for(auto count:next.delta().calls)if(count!=0)return 9;}
 return 0;
}
