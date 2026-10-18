#include "timing.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdexcept>
#include "jfg/boot/native_collision.hpp"
#include <array>
#include <vector>
#include <random>
#include <cstdio>
#include <cstdlib>
#include <cfenv>
#include <chrono>
#include <cstring>
extern "C" {
 JfgOriginalTimingFast* jfg_original_timing_state=nullptr;
 static int32_t sections[4]={0,0,0,static_cast<int32_t>(0x00300000U)};
 int32_t* section_addresses=sections;
 void bsp_reference(uint8_t*,recomp_context*);void split_reference(uint8_t*,recomp_context*);void line_reference(uint8_t*,recomp_context*);
 void bsp_fast(uint8_t*,recomp_context*);void split_fast(uint8_t*,recomp_context*);void line_fast(uint8_t*,recomp_context*);
 static bool fast_mode=false;int bsp_in_fallback();void bsp_configure(unsigned);void bsp_totals();
 void do_break(uint32_t pc){std::printf("REFERENCE_BREAK %08x\n",pc);std::abort();}
 void jfg_phase9_execution_probe_predecoded(unsigned,unsigned,unsigned,void*){}
 recomp_func_t* get_function(int32_t pc) {
  switch(static_cast<uint32_t>(pc)) {
   case 0x00317174U:return (fast_mode && !bsp_in_fallback())?bsp_fast:bsp_reference;
   case 0x003170d0U:return (fast_mode && !bsp_in_fallback())?split_fast:split_reference;
   case 0x00303e90U:return (fast_mode && !bsp_in_fallback())?line_fast:line_reference;
  }
  std::printf("UNKNOWN_CALL %08x\n",static_cast<uint32_t>(pc));std::abort();
 }
}
static void require(bool b,unsigned n,const char* what){if(!b){std::printf("FAIL %u %s\n",n,what);std::exit(1);}}
extern "C" {
static unsigned node_limit=32,edge_limit=128;
int bsp_in_fallback(){return jfg::boot::native_collision_in_fallback()?1:0;}
void bsp_fast(uint8_t* ram,recomp_context* ctx){jfg::boot::native_collision_execute(bsp_reference,ram,ctx,node_limit,edge_limit);}
void split_fast(uint8_t*,recomp_context*){std::abort();}
void line_fast(uint8_t*,recomp_context*){std::abort();}
void bsp_configure(unsigned n){node_limit=n%9U==8U?32U:n%9U;edge_limit=n%7U==6U?128U:(n%7U)*3U;}
void bsp_totals(){const auto v=jfg::boot::native_collision_counters();std::printf("BOUNDS nodes=%llu edges=%llu fallbacks=%llu expired=%llu invalid=%llu max_ns=%llu\n",v.native_nodes,v.native_edges,v.fallbacks,v.expired,v.invalid,v.max_nanoseconds);}
}

static unsigned mock_calls=0;
static void mock_reference(uint8_t*,recomp_context*) {
 ++mock_calls;
 if(!jfg::boot::native_collision_in_fallback())std::abort();
}
static void throwing_reference(uint8_t*,recomp_context*) {
 mock_reference(nullptr,nullptr);throw std::runtime_error("fixture cancellation");
}
struct MappedRam {
 uint8_t* pointer=static_cast<uint8_t*>(VirtualAlloc(nullptr,0x100000000ULL,MEM_RESERVE,PAGE_NOACCESS));
 MappedRam() {
  if(!pointer || !VirtualAlloc(pointer,size(),MEM_COMMIT,PAGE_READWRITE) ||
     !VirtualAlloc(pointer+0x8031a000ULL,4096,MEM_COMMIT,PAGE_READWRITE))std::abort();
 }
 ~MappedRam(){VirtualFree(pointer,0,MEM_RELEASE);}
 uint8_t* data(){return pointer;}
 constexpr size_t size()const{return 4U*1024U*1024U;}
 uint8_t& operator[](size_t n){return pointer[n];}
};
static void admission_tests(MappedRam& ram) {
 using namespace jfg::boot;
 constexpr uint32_t sp=0x803ff000U,node=0x80100000U,edges=0x80110000U;
 const auto put=[&](uint32_t a,uint32_t v){std::memcpy(ram.data()+uint32_t(a-0x80000000U),&v,4);};
 const auto half=[&](uint32_t a,uint16_t v){std::memcpy(ram.data()+(uint32_t(a-0x80000000U)^2U),&v,2);};
 const auto byte=[&](uint32_t a,uint8_t v){ram[uint32_t(a-0x80000000U)^3U]=v;};
 for(unsigned n=0;n<18;++n) {
  sections[3]=0x00300000;put(sp+16,node);put(0x0031aed4,edges);
  byte(node+16,1);half(node,0);half(node+2,0);
  recomp_context cpu{};cpu.r29=U64(S32(sp));cpu.status_reg=0x24000001U;
  cpu.mips3_float_mode=1;cpu.f_odd=&cpu.f1.u32l;
  switch(n) {
   case 0:cpu.r0=1;break;
   case 1:cpu.r29=sp;break;
   case 2:cpu.r29=U64(S32(sp+1));break;
   case 3:cpu.r29=U64(S32(0x80000020U));break;
   case 4:cpu.status_reg=0;break;
   case 5:cpu.mips3_float_mode=0;break;
   case 6:cpu.f_odd=nullptr;break;
   case 7:sections[3]=0x00310000;break;
   case 8:put(sp+16,node+1);break;
   case 9:put(sp+16,0x80400000);break;
   case 10:cpu.f12.u32l=0x7fc00000;break;
   case 11:cpu.r7=0x7f800000;break;
   case 12:half(node+2,17);break;
   case 13:put(0x0031aed4,edges+1);break;
   case 14:byte(node+16,0);put(node,0);put(node+4,node);break;
   case 15:half(node+2,1);put(edges,0x7fc00000);break;
   case 16:cpu.f14.u32l=0x7f800000;break;
   case 17:half(node+2,1);put(0x0031aed4,0xfffffffc);break;
  }
  const auto before=cpu;const auto counters=native_collision_counters();const auto calls=mock_calls;
  require(!native_collision_execute(mock_reference,ram.data(),&cpu),n,"reject unsupported admission");
  require(mock_calls==calls+1,n,"exactly one original fallback");
  require(std::memcmp(&cpu,&before,sizeof(cpu))==0,n,"rejection preserves CPU");
  require(native_collision_counters().native_nodes==counters.native_nodes,n,"rejection performs no native nodes");
  require(!native_collision_in_fallback(),n,"fallback flag unwound");
 }
 sections[3]=0x00300000;put(sp+16,node);put(0x0031aed4,edges);
 byte(node+16,1);half(node,0);half(node+2,0);
 recomp_context cpu{};cpu.r29=U64(S32(sp));cpu.status_reg=0x24000001;
 cpu.mips3_float_mode=1;cpu.f_odd=&cpu.f1.u32l;
 bool caught=false;
 try{native_collision_execute(throwing_reference,ram.data(),&cpu,0,0);}
 catch(const std::runtime_error&){caught=true;}
 require(caught && !native_collision_in_fallback(),18,"cancellation unwinds fallback state");
 const auto calls=mock_calls;
 // Validate default limits were restored, rather than retaining the zero budget.
 require(native_collision_execute(mock_reference,ram.data(),&cpu),19,"native call after cancellation");
 require(mock_calls==calls,19,"post-cancellation default budget restored");
 native_collision_reset_max();require(native_collision_counters().max_nanoseconds==0,20,"counter max reset");
 std::printf("PASS admission=18 cancellation_cleanup=1\n");
}

int main() {
 constexpr uint32_t ram_base=0x80000000U,sp=0x803ff000U,nodes=0x80100000U,edges=0x80110000U,globals=0x0031aed0U;
 MappedRam ram;std::vector<uint8_t> expected(ram.size());
 const auto put=[&](uint32_t a,uint32_t v){std::memcpy(ram.data()+uint32_t(a-ram_base),&v,4);};
 const auto half=[&](uint32_t a,uint16_t v){std::memcpy(ram.data()+((a-ram_base)^2U),&v,2);};
 const auto byte=[&](uint32_t a,uint8_t v){ram[(a-ram_base)^3U]=v;};
 const auto fp=[&](uint32_t a,float f){uint32_t v;std::memcpy(&v,&f,4);put(a,v);};
 std::mt19937 rng(0x317174U);unsigned returns[3]{};
 constexpr unsigned cases=50000;
 const auto begin=std::chrono::steady_clock::now();
 for(unsigned n=0;n<cases;++n) {
  bsp_configure(n);
  const unsigned depth=n%5,internal=(1U<<depth)-1U,total=2U*internal+1U;
  unsigned edge_index=0;
  for(unsigned node=0;node<total;++node) {
   const uint32_t address=nodes+20U*node;
   if(node<internal) {
    byte(address+16U,0);put(address,nodes+(2U*node+1U)*20U);put(address+4U,nodes+(2U*node+2U)*20U);
    fp(address+8U,float(int(rng()%5)-2)*0.5f);fp(address+12U,float(int(rng()%201)-100));
   } else {
    const unsigned count=(n+node)%6;
    byte(address+16U,1);half(address,static_cast<uint16_t>(edge_index));half(address+2U,static_cast<uint16_t>(count));
    for(unsigned i=0;i<count;++i,++edge_index) {
     for(unsigned c=0;c<4;++c)fp(edges+edge_index*20U+c*4U,float(int(rng()%401)-200)+float(n%5)*0.125f);
     byte(edges+edge_index*20U+16U,static_cast<uint8_t>(rng()));
     byte(edges+edge_index*20U+17U,static_cast<uint8_t>(rng()));
    }
   }
  }
  const float x1=float(int(rng()%401)-200)+float(n%7)*.125f,y1=float(int(rng()%401)-200);
  const float x2=n%3==0?x1:float(int(rng()%401)-200),y2=float(int(rng()%401)-200)+float(n%5)*.25f;
  for(unsigned j=0;j<0x50U;j+=4U)put(globals+j,rng());
  put(globals,nodes);put(globals+4U,edges);
  put(globals+8U,static_cast<uint32_t>(static_cast<int32_t>(x1)));
  put(globals+12U,static_cast<uint32_t>(static_cast<int32_t>(y1)));
  put(globals+16U,static_cast<uint32_t>(static_cast<int32_t>(x2)));
  put(globals+20U,static_cast<uint32_t>(static_cast<int32_t>(y2)));
  put(globals+0x30U,n%2==0?0U:0x803fe000U);put(globals+0x3cU,n%5==0?rng()%10000U:0xffffffffU);
  for(unsigned j=0;j<2048;++j)ram[sp-ram_base-1024+j]=static_cast<uint8_t>(rng());
  put(sp+16U,nodes);
  recomp_context initial{};initial.r29=U64(S32(sp));initial.r31=U64(S32(0x80001234U));
  initial.r4=U64(S32(rng()));initial.r5=U64(S32(rng()));
  uint32_t bits=0;std::memcpy(&bits,&x2,4);initial.r6=U64(S32(bits));std::memcpy(&bits,&y2,4);initial.r7=U64(S32(bits));
  initial.f12.fl=x1;initial.f14.fl=y1;initial.status_reg=0x24000001U;initial.mips3_float_mode=1;initial.fcr31=n%4;
  std::array<uint8_t,2048> stack_before{},stack_expected{};
  std::array<uint8_t,0x50> globals_before{},globals_expected{};
  std::memcpy(stack_before.data(),ram.data()+sp-ram_base-1024,stack_before.size());
  std::memcpy(globals_before.data(),ram.data()+uint32_t(globals-ram_base),globals_before.size());
  const int modes[]={FE_TONEAREST,FE_TOWARDZERO,FE_UPWARD,FE_DOWNWARD};
  std::fesetround(modes[n%4]);std::feclearexcept(FE_ALL_EXCEPT);
  fenv_t env_before{},env_expected{},env_actual{};std::fegetenv(&env_before);
  auto reference=initial;reference.f_odd=&reference.f1.u32l;fast_mode=false;bsp_reference(ram.data(),&reference);std::fegetenv(&env_expected);
  std::memcpy(stack_expected.data(),ram.data()+sp-ram_base-1024,stack_expected.size());
  std::memcpy(globals_expected.data(),ram.data()+uint32_t(globals-ram_base),globals_expected.size());
  const bool full=(n%256U)==0U || n+1U==cases;if(full)std::memcpy(expected.data(),ram.data(),ram.size());
  std::memcpy(ram.data()+sp-ram_base-1024,stack_before.data(),stack_before.size());
  std::memcpy(ram.data()+uint32_t(globals-ram_base),globals_before.data(),globals_before.size());
  std::fesetenv(&env_before);
  auto candidate=initial;candidate.f_odd=&candidate.f1.u32l;fast_mode=true;bsp_fast(ram.data(),&candidate);std::fegetenv(&env_actual);
  reference.f_odd=candidate.f_odd;
  require(std::memcmp(&candidate,&reference,sizeof(candidate))==0,n,"all CPU context");
  require(std::memcmp(stack_expected.data(),ram.data()+sp-ram_base-1024,stack_expected.size())==0,n,"all stack");
  require(std::memcmp(globals_expected.data(),ram.data()+uint32_t(globals-ram_base),globals_expected.size())==0,n,"all scratch globals");
  require(std::memcmp(&env_actual,&env_expected,sizeof(env_actual))==0,n,"floating environment");
  if(full)require(std::memcmp(expected.data(),ram.data(),ram.size())==0,n,"full RAM");
  require(candidate.r2<3U,n,"return domain");++returns[candidate.r2];
 }
 admission_tests(ram);bsp_totals();std::printf("PASS cases=%u false=%u true=%u collinear=%u seconds=%.3f\n",cases,returns[0],returns[1],returns[2],std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count());
}
