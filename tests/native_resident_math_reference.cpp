// Private generated C references are supplied by the validation script; this fixture contains no game code.

#include "timing.h"
#include <array>
#include <bit>
#include <random>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cfenv>
#include <cstring>
#include <vector>
extern "C" {
 JfgOriginalTimingFast* jfg_original_timing_state=nullptr;
 void math_reference(uint8_t*,recomp_context*);
 void math_candidate(uint8_t*,recomp_context*);
}
static uint64_t revision=1,guest_count=100;
static unsigned hooks=0;
extern "C" void jfg_phase9_execution_probe_predecoded(unsigned m,unsigned pc,unsigned w,void* ctx) {
 auto& s=*jfg_original_timing_state;++hooks;++s.cpu_count;++s.observed_guest_instructions;
 s.host_clock_poll_budget=128;s.execution_fast_deadline=UINT64_MAX;s.execution_fast_revision=revision;
 s.guest_previous_delay=s.guest_previous_branch&&pc==s.guest_last_pc+4;s.guest_previous_branch=(m&0x400U)!=0;
 s.guest_last_pc=pc;s.guest_last_word=w;guest_count=s.cpu_count;
 jfg_native_timing_refresh(&s,static_cast<recomp_context*>(ctx));
}
struct Snapshot {recomp_context cpu{};JfgOriginalTimingFast fast{};uint64_t count=0;unsigned calls=0;int exceptions=0;};
static void require(bool v,unsigned n,const char* why){if(!v){printf("FAIL %u %s\n",n,why);std::exit(1);}}
int main(){
 std::mt19937 rng(919816);std::vector<uint8_t> ram(0xb0000);for(auto& b:ram)b=uint8_t(rng());auto before_ram=ram;
 const uint32_t special[]={0,0x80000000U,1,0x80000001U,0x007fffffU,0x807fffffU,0x00800000U,
  0x80800000U,0x3f800000U,0xbf800000U,0x7f7fffffU,0xff7fffffU,0x7f800000U,0xff800000U,0x7fc00000U,0x7f800001U};
 unsigned admitted=0,rejected=0;
 for(unsigned n=0;n<100000;++n){
  Snapshot init;auto& c=init.cpu;auto& s=init.fast;
  const std::array<gpr*,32> registers={&c.r0,&c.r1,&c.r2,&c.r3,&c.r4,&c.r5,&c.r6,&c.r7,&c.r8,&c.r9,&c.r10,&c.r11,&c.r12,&c.r13,&c.r14,&c.r15,&c.r16,&c.r17,&c.r18,&c.r19,&c.r20,&c.r21,&c.r22,&c.r23,&c.r24,&c.r25,&c.r26,&c.r27,&c.r28,&c.r29,&c.r30,&c.r31};
  const std::array<fpr*,32> floats={&c.f0,&c.f1,&c.f2,&c.f3,&c.f4,&c.f5,&c.f6,&c.f7,&c.f8,&c.f9,&c.f10,&c.f11,&c.f12,&c.f13,&c.f14,&c.f15,&c.f16,&c.f17,&c.f18,&c.f19,&c.f20,&c.f21,&c.f22,&c.f23,&c.f24,&c.f25,&c.f26,&c.f27,&c.f28,&c.f29,&c.f30,&c.f31};
  for(unsigned k=0;k<32;++k){*registers[k]=(uint64_t(rng())<<32U)|rng();floats[k]->u64=(uint64_t(rng())<<32U)|rng();}
  c.r0=0;c.status_reg=0x24000001U;c.mips3_float_mode=1;c.fcr31=rng()&3U;
  c.f12.u32l=n%3==0?special[n%16]:rng();c.f14.u32l=n%3==0?special[(n/16)%16]:rng();
  if constexpr(JFG_MATH_FIXTURE_KIND==1) {
   const uint32_t edges[]={0U,1U,32767U,32768U,65535U,65536U,0x7fffffffU,0x80000000U,0xffffffffU};
   if(n%3==0){c.r4=U64(S32(edges[n%9]));c.r5=U64(S32(edges[(n/9)%9]));}
  }
  if constexpr(JFG_MATH_FIXTURE_KIND>=2) {if(n<65536U)c.r4=n;}
  s.rdram=ram.data();s.diagnostic_host_clock=true;s.native_spans_enabled=true;s.play_mode=true;
  s.host_clock_poll_budget=n%145;s.cpu_count=100;s.io_guest_count=&guest_count;s.io_revision=&revision;
  s.execution_fast_revision=1;s.execution_fast_deadline=UINT64_MAX;s.guest_last_pc=0x80049c80U;s.guest_previous_branch=n%2;
  if(n%29==0)s.native_spans_enabled=false;
  if(n%31==0)c.r0=1;
  if(n%37==0)c.status_reg&=~0x20000000U;
  auto run=[&](bool candidate){
   Snapshot out=init;out.cpu.f_odd=&out.cpu.f1.u32l;guest_count=100;hooks=0;revision=1;
   const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
   require(std::fesetround(modes[n%4])==0,n,"rounding");std::feclearexcept(FE_ALL_EXCEPT);
   jfg_original_timing_state=&out.fast;jfg_native_timing_refresh(&out.fast,&out.cpu);
   if(candidate)math_candidate(ram.data(),&out.cpu);else math_reference(ram.data(),&out.cpu);
   out.count=guest_count;out.calls=hooks;out.exceptions=std::fetestexcept(FE_ALL_EXCEPT);out.cpu.f_odd=nullptr;return out;
  };
  auto before=run(false),after=run(true);
  if(std::memcmp(&before.cpu,&after.cpu,sizeof(before.cpu))){
   for(size_t k=0;k<sizeof(before.cpu);k+=4){uint32_t a,b;memcpy(&a,(char*)&before.cpu+k,4);memcpy(&b,(char*)&after.cpu+k,4);if(a!=b)printf("CPU_DIFF n=%u offset=%zx before=%08x after=%08x\n",n,k,a,b);}
   return 2;
  }
  if(after.fast.native_span_instructions>before.fast.native_span_instructions)++admitted;else ++rejected;
  before.fast.native_span_instructions=after.fast.native_span_instructions=0;
  before.fast.native_span_commits=after.fast.native_span_commits=0;
  require(std::memcmp(&before.fast,&after.fast,sizeof(before.fast))==0,n,"timing state");
  require(before.count==after.count&&before.calls==after.calls&&before.exceptions==after.exceptions,n,"count hooks fenv");
 }
 require(ram==before_ram,0,"RAM unchanged");
 printf("PASS cases=100000 admitted=%u fallback=%u full_context_timing_fenv_exact=1\n",admitted,rejected);
 return 0;
}
