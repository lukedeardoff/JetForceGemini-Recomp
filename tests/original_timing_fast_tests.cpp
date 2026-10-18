#include "jfg/boot/original_timing_fast.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <tuple>
namespace ordinary {
#include "jfg/boot/original_timing_cache.h"
}
extern "C" { JfgOriginalTimingFast* jfg_original_timing_state = nullptr; }
static unsigned fallbacks=0;
extern "C" void jfg_phase9_execution_probe_predecoded(unsigned,unsigned,unsigned,void*) { ++fallbacks; }
static void check(bool okay,const char* reason) { if(!okay){std::cerr<<reason<<'\n';std::exit(1);} }
int main() {
 std::vector<uint8_t> ram(4U*1024U*1024U);
 uint32_t table=0x80010000;memcpy(ram.data()+0xfeaa0,&table,4);
 for(unsigned slot=1;slot<158;++slot){uint32_t base=0x80020000+slot*0x4000;memcpy(ram.data()+0x10000+slot*32,&base,4);}
 std::array<JfgTimingCacheLine,512> ic{},dc{};std::array<uint64_t,12> stats{};
 uint64_t revision=7,guest_count=0;
 JfgOriginalTimingFast fast{};fast.rdram=ram.data();fast.instruction_cache=ic.data();fast.data_cache=dc.data();fast.cache_counters=stats.data();
 fast.io_revision=&revision;fast.io_guest_count=&guest_count;fast.execution_fast_revision=revision;
 fast.execution_fast_deadline=UINT64_MAX;fast.execution_fast_profile_ready=true;
 jfg_original_timing_state=&fast;
 recomp_context cpu{};cpu.status_reg=0x20000000;
 uint64_t expected_count=0,half=0;uint32_t rng=0x73421499,previous_pc=0;bool previous_branch=false;
 for(unsigned i=0;i<250000;++i) {
  rng=rng*1664525U+1013904223U;
  unsigned primary=0,word=0,metadata=0;
  switch(i%6) {
   case 0:word=0x20;break;
   case 1:primary=0x23;metadata=1U<<12;break;
   case 2:primary=0x2b;metadata=2U<<12;break;
   case 3:primary=4;metadata=1U<<10;break;
   case 4:word=24;metadata=4U<<16;break;
   case 5:primary=17;word=(16U<<21)|2U;metadata=(1U<<8)|(4U<<16);break;
  }
  if(primary==0x23 || primary==0x2b) {
   cpu.r1=(i%11==0?0xa0000000U:i%13==0?0x00100000U:0x80000000U)+(rng&0x3ffffcU);
   word=(1U<<21)|(rng&0xffffU);
  }
  word|=primary<<26;
  if(i&1U)metadata|=0x800000U; // Host inlining hint must not change modeled costs.
  unsigned pc=(i%17==0?0x00100000U:0x80000000U)+0x1000+(i%97==0?(rng&0xfffcU):(i%8)*4);
  const auto cost=ordinary::jfg_full_cache_cost(pc,word,&cpu,ram.data())+half;
  expected_count+=cost/2;half=cost%2;
  jfg_original_timing_step(metadata,pc,word,&cpu);
  check(fallbacks==0,"ordinary instructions unexpectedly fell back");
  check(fast.cpu_count==expected_count && guest_count==expected_count && fast.diagnostic_cpu_half_tick==half,"Count or half-cycle differs");
  check(fast.guest_previous_delay==(previous_branch && pc==previous_pc+4) && fast.guest_previous_branch==((metadata&(1U<<10))!=0),"branch bookkeeping differs");
  check(fast.guest_last_pc==pc && fast.guest_last_word==word && fast.observed_guest_instructions==i+1 && fast.execution_fast_hits==i+1,"instruction bookkeeping differs");
  check(memcmp(stats.data(),ordinary::cache_stats,sizeof(stats))==0,"cache statistics differ");
  previous_pc=pc;previous_branch=fast.guest_previous_branch;
 }
 check(memcmp(ic.data(),ordinary::ic,sizeof(ic))==0 && memcmp(dc.data(),ordinary::dc,sizeof(dc))==0,"cache lines differ");
 auto boundary=[&](unsigned metadata) {
  const auto before=fast;const auto counters=stats;const auto old=fallbacks;
  jfg_original_timing_step(metadata,0x80010000,0,&cpu);
  check(fallbacks==old+1 && fast.cpu_count==before.cpu_count && fast.observed_guest_instructions==before.observed_guest_instructions && stats==counters,"event boundary changed state before native handler");
 };
 boundary(3U<<12);boundary(1U<<11);boundary(1U<<9);
 fast.guest_eret_boundary=true;boundary(0);fast.guest_eret_boundary=false;
 fast.execution_fast_profile_ready=false;boundary(0);fast.execution_fast_profile_ready=true;
 ++revision;boundary(0);--revision;
 fast.execution_fast_deadline=fast.cpu_count;boundary(0);fast.execution_fast_deadline=UINT64_MAX;
 cpu.status_reg=0;boundary(1U<<8);
 cpu.status_reg=0x401;fast.execution_fast_pending=0x400;boundary(0);
 // Native mode must preserve architectural bookkeeping without touching
 // cache state or inventing elapsed time. The full handler owns clock sampling.
 for (const unsigned quantum : {128U,512U}) {
 fast=JfgOriginalTimingFast{};cpu=recomp_context{};
 fast.diagnostic_host_clock=true;fast.execution_fast_profile_ready=false;
 fast.host_clock_poll_budget=quantum;fast.cpu_count=123456;guest_count=fast.cpu_count;
 fast.io_revision=&revision;fast.io_guest_count=&guest_count;
 fast.execution_fast_revision=revision;fast.execution_fast_deadline=UINT64_MAX;
 cpu.status_reg=0x20000000;
 const auto native_fallbacks=fallbacks;
 // Null cache pointers deliberately prove the native path never reads caches.
 for(unsigned i=0;i<quantum-1;++i) {
  jfg_original_timing_step(i%3==0?0x400U:0U,0x80010000+i*4,0,&cpu);
  check(fast.cpu_count==123456 && guest_count==123456,"native instructions invented time");
  check(fast.observed_guest_instructions==i+1 && fast.execution_fast_hits==i+1,"native bookkeeping lost instructions");
  check(fast.host_clock_poll_budget==quantum-1-i && fallbacks==native_fallbacks,"native safepoint interval changed");
 }
 jfg_original_timing_step(0,0x800101fc,0,&cpu);
 check(fallbacks==native_fallbacks+1 && fast.observed_guest_instructions==quantum-1,"native sampling boundary was skipped");
 fast.host_clock_poll_budget=quantum;
 boundary(3U<<12);boundary(1U<<11);boundary(1U<<9);
 ++revision;boundary(0);--revision;
 fast.guest_eret_boundary=true;boundary(0);fast.guest_eret_boundary=false;
 fast.execution_fast_deadline=fast.cpu_count;boundary(0);fast.execution_fast_deadline=UINT64_MAX;
 cpu.status_reg=0;boundary(1U<<8);
 cpu.status_reg=0x401;fast.execution_fast_pending=0x400;boundary(0);
 }
 std::cout<<"250000 reference transitions; native clock budget and event boundaries matched\n";
 // Differential coverage of the small-function native path and the compact
 // all-function path, including refusal at event/exception/observer boundaries.
 const auto values=[](const JfgOriginalTimingFast& v) {
  return std::tuple(v.cpu_count,v.observed_guest_instructions,v.execution_fast_hits,
    v.diagnostic_cpu_half_tick,v.guest_last_pc,v.guest_last_word,
    v.guest_previous_branch,v.guest_previous_delay,
    v.execution_fast_pending,v.execution_fast_revision,v.execution_fast_deadline);
 };
 for(unsigned i=0;i<65536;++i) {
  fast=JfgOriginalTimingFast{};cpu=recomp_context{};
  fast.diagnostic_host_clock=true;fast.host_clock_poll_budget=i%257;
  fast.cpu_count=123456;fast.observed_guest_instructions=(i%17==0?24000000000ULL:i);
  fast.io_revision=&revision;fast.io_guest_count=&guest_count;
  fast.execution_fast_revision=revision+(i%19==0);
  fast.execution_fast_deadline=fast.cpu_count+(i%23!=0);
  fast.guest_eret_boundary=i%29==0;
  fast.play_mode=i%31==0;fast.guest_previous_branch=i%3==0;
  fast.guest_last_pc=0x80010000;fast.guest_last_word=0x24020001;
  fast.execution_fast_pending=i%7==0?0x400:0;
  cpu.status_reg=(i%5==0?0U:0x20000000U)|(i%7==0?0x401U:0U);
  const auto before=fast;
  const unsigned metadata=(i&0x3fU)<<8;
  const unsigned pc=0x80010000+(i%3)*4,word=0x24020001;
  const auto old_fallbacks=fallbacks;guest_count=654;
  jfg_original_timing_step(metadata,pc,word,&cpu);
  const auto expected=values(fast);const auto expected_native_count=guest_count;
  const auto expected_fallbacks=fallbacks-old_fallbacks;
  fast=before;guest_count=654;const auto new_fallbacks=fallbacks;
  jfg_native_timing_refresh(&fast,&cpu);
  jfg_native_timing_step(metadata,pc,word,&cpu);
  check(values(fast)==expected && guest_count==expected_native_count &&
      fallbacks-new_fallbacks==expected_fallbacks,
      "all-function native path changed clock, instruction or safepoint behavior");
 }
 // Mutate each dynamic condition after certification. A stale certificate
 // must never admit Status or I/O changes, ERET, register writes or poll expiry.
 fast=JfgOriginalTimingFast{};cpu=recomp_context{};
 fast.diagnostic_host_clock=true;fast.host_clock_poll_budget=128;
 fast.execution_fast_deadline=UINT64_MAX;fast.io_revision=&revision;
 fast.io_guest_count=&guest_count;fast.execution_fast_revision=revision;
 cpu.status_reg=0x20000000U;
 jfg_native_timing_refresh(&fast,&cpu);
 check(fast.native_fast_ready,"valid native certificate refused");
 auto certified_boundary=[&](unsigned metadata) {
  const auto before=values(fast);const auto old=fallbacks;
  jfg_native_timing_step(metadata,0x80010004,0,&cpu);
  check(fallbacks==old+1 && values(fast)==before,"stale certificate admitted a boundary");
 };
 ++revision;certified_boundary(0);--revision;
 cpu.status_reg^=1U;certified_boundary(0);cpu.status_reg^=1U;
 certified_boundary(3U<<12);certified_boundary(0x800);certified_boundary(0x200);
 fast.native_fast_ready=false;certified_boundary(0);
 jfg_native_timing_refresh(&fast,&cpu);fast.host_clock_poll_budget=1;certified_boundary(0);
 fast.host_clock_poll_budget=128;fast.observed_guest_instructions=24000000000ULL-2;
 jfg_native_timing_refresh(&fast,&cpu);
 check(fast.host_clock_poll_budget==3,"instruction limit was not bounded");
 const auto old=fallbacks;
 jfg_native_timing_step(0,0x80010008,0,&cpu);
 jfg_native_timing_step(0,0x8001000c,0,&cpu);
 check(fallbacks==old && fast.observed_guest_instructions==24000000000ULL,"instruction limit changed");
 certified_boundary(0);
 std::cout<<"250000 reference and 65536 native path transitions; certificate invalidation and instruction limits matched\n";

 // Whole-span and partial-prefix accounting must match individual admitted
 // hooks exactly, including an incoming branch's delay slot and poll expiry.
 for(unsigned i=0;i<65536;++i) {
  fast=JfgOriginalTimingFast{};cpu=recomp_context{};
  fast.diagnostic_host_clock=true;fast.native_spans_enabled=i%13!=0;
  fast.host_clock_poll_budget=i%33;fast.cpu_count=123456;
  fast.io_revision=&revision;fast.io_guest_count=&guest_count;
  fast.execution_fast_revision=revision+(i%19==0);
  fast.execution_fast_deadline=fast.cpu_count+(i%23!=0);
  fast.guest_eret_boundary=i%29==0;fast.play_mode=true;
  fast.guest_previous_branch=i%3==0;fast.guest_last_pc=0x80010000;
  cpu.status_reg=i%5==0?0U:0x20000000U;
  jfg_native_timing_refresh(&fast,&cpu);
  const unsigned count=2+i%15,flags=i%2?0x100U:0U,first=0x80010004;
  const auto before=fast;const auto member_fallbacks=fallbacks;guest_count=fast.cpu_count;
  for(unsigned n=0;n<count;++n)jfg_native_timing_step(flags,first+n*4,0x24210001,&cpu);
  const auto expected=values(fast);const auto budget=fast.host_clock_poll_budget;
  const bool all_fast=fallbacks==member_fallbacks;
  fast=before;guest_count=fast.cpu_count;
  const bool admitted=jfg_native_timing_span_begin(flags,count,&cpu);
  check(admitted==(before.native_spans_enabled && all_fast),"span admission differs from member hooks");
  if(admitted) {
   const unsigned prefix=1+i%count;
   jfg_native_timing_span_commit(first,first+(prefix-1)*4,0x24210001,prefix);
   for(unsigned n=prefix;n<count;++n)jfg_native_timing_step(flags,first+n*4,0x24210001,&cpu);
   check(values(fast)==expected && fast.host_clock_poll_budget==budget && guest_count==fast.cpu_count,
       "span or partial-prefix accounting differs from individual instructions");
   check(fast.native_span_commits==1 && fast.native_span_instructions==prefix,"span diagnostics differ");
  }
 }
 fast.native_spans_enabled=true;
 check(!jfg_native_timing_span_begin(0,1,&cpu) && !jfg_native_timing_span_begin(0,17,&cpu) &&
   !jfg_native_timing_span_begin(0x200,2,&cpu) && !jfg_native_timing_span_begin(0,2,nullptr),"invalid span shape accepted");
 for(unsigned width: {1U,2U,4U,8U}) {
  check(jfg_native_timing_span_ram(0xffffffff80000000ULL,width),"first RDRAM address refused");
  check(jfg_native_timing_span_ram(0xffffffff80400000ULL-width,width),"last aligned RDRAM access refused");
  check(!jfg_native_timing_span_ram(0xffffffff80400000ULL,width),"RDRAM overrun accepted");
  check(!jfg_native_timing_span_ram(0xffffffffa0000000ULL,width),"uncached alias admitted");
  check(!jfg_native_timing_span_ram(0xffffffffa4400000ULL,width),"MMIO admitted");
  check(!jfg_native_timing_span_ram(0x80000000ULL,width),"wrong full-width address admitted");
  if(width>1)check(!jfg_native_timing_span_ram(0xffffffff80000001ULL,width),"unaligned access admitted");
 }
 check(!jfg_native_timing_span_ram(0xffffffff80000000ULL,3),"invalid width admitted");
 std::cout<<"65536 bounded span/prefix differential checks; exact address guards matched\n";
}
