#include "jfg/boot/native_collision.hpp"
extern "C" {
#include "recomp.h"
}
// The private generated body is compiled separately from the host dispatcher.
// Its arithmetic and register effects are retained; bounded admission replaces
// instruction-by-instruction bookkeeping only in native elapsed-time mode.
#include <chrono>
#include <cstdint>
#include <cstring>
namespace jfg::boot::collision_detail {
using Totals=NativeCollisionCounters;
thread_local Totals totals{};
thread_local bool fallback=false;
thread_local unsigned limit_nodes=32U,limit_edges=128U;
struct Budget {
 recomp_func_t* reference=nullptr;unsigned nodes=0,edges=0,depth=0;
 std::chrono::steady_clock::time_point deadline{};
};
thread_local Budget* active=nullptr;
static bool range(std::uint32_t a,std::size_t n) {return a>=0x80000000U && a<=0x80400000U && n<=0x80400000ULL-a;}
static std::uint32_t word(const std::uint8_t* ram,std::uint32_t a) {std::uint32_t v;std::memcpy(&v,ram+static_cast<std::uint32_t>(a-0x80000000U),4);return v;}
static unsigned half(const std::uint8_t* ram,std::uint32_t a) {std::uint16_t v;std::memcpy(&v,ram+(static_cast<std::uint32_t>(a-0x80000000U)^2U),2);return v;}
static bool small_float(std::uint32_t bits){return (bits&0x7fffffffU)<=0x49800000U;}
void bsp_core(std::uint8_t*,recomp_context*);
void split_core(std::uint8_t*,recomp_context*);
void line_core(std::uint8_t*,recomp_context*);
static void bounded(std::uint8_t* ram,recomp_context* ctx);
#pragma warning(push, 0)
#include JFG_NATIVE_COLLISION_BODY
#pragma warning(pop)
static bool node_valid(std::uint8_t* ram,recomp_context* ctx,unsigned& edges) {
 if(!ram || !ctx || !section_addresses)return false;
 const auto sp=static_cast<std::uint32_t>(ctx->r29);
 if(ctx->r0 || ctx->r29!=U64(S32(sp)) || (sp&3U) || sp<0x80000060U || !range(sp,20U) ||
    !ctx->mips3_float_mode || ctx->f_odd!=&ctx->f1.u32l || !(ctx->status_reg&0x20000000U))return false;
 const auto section=static_cast<std::uint32_t>(section_addresses[3]);
 if(section!=0x00300000U)return false;

 const auto node=word(ram,sp+16U);
 if((node&3U) || !range(node,20U))return false;
 if(!small_float(ctx->f12.u32l) || !small_float(ctx->f14.u32l) ||
    !small_float(static_cast<std::uint32_t>(ctx->r6)) || !small_float(static_cast<std::uint32_t>(ctx->r7)))return false;
 edges=0U;
 if(ram[(node-0x80000000U+16U)^3U]==1U) {
  const auto first=half(ram,node),count=half(ram,node+2U);
  const auto base=word(ram,section+0x1aed4U);
  const auto start=std::uint64_t{base}+20ULL*first;
  if((base&3U) || count>16U || start>UINT32_MAX || !range(static_cast<std::uint32_t>(start),20U*count))return false;
  for(unsigned i=0;i<count;++i)for(unsigned j=0;j<16U;j+=4U)
   if(!small_float(word(ram,static_cast<std::uint32_t>(start)+20U*i+j)))return false;
  edges=count;
 } else {
  const auto a=word(ram,node),b=word(ram,node+4U);
  if((a&3U) || (b&3U) || !range(a,20U) || !range(b,20U) ||
     !small_float(word(ram,node+8U)) || !small_float(word(ram,node+12U)))return false;
 }
 return true;
}
static void bounded(std::uint8_t* ram,recomp_context* ctx) {
 auto& budget=*active;unsigned edges=0U;
 const bool exhausted=budget.nodes==0U || budget.depth>=24U ||
     std::chrono::steady_clock::now()>=budget.deadline;
 const bool valid=!exhausted && node_valid(ram,ctx,edges) && edges<=budget.edges;
 if(!valid) {
  ++totals.fallbacks;if(exhausted)++totals.expired;else ++totals.invalid;
  struct Fallback {bool old=fallback;Fallback(){fallback=true;}~Fallback(){fallback=old;}} guard;
  budget.reference(ram,ctx);return;
 }
 --budget.nodes;budget.edges-=edges;++budget.depth;++totals.native_nodes;totals.native_edges+=edges;
 bsp_core(ram,ctx);--budget.depth;
}
static void execute(recomp_func_t* reference,std::uint8_t* ram,recomp_context* ctx) {
 const auto started=std::chrono::steady_clock::now();
 Budget budget{reference,limit_nodes,limit_edges,0U,started+std::chrono::microseconds(50)};
 struct Scope {Budget* old=active;explicit Scope(Budget* b){active=b;}~Scope(){active=old;}} scope(&budget);
 bounded(ram,ctx);
 const auto elapsed=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-started).count());
 if(elapsed>totals.max_nanoseconds)totals.max_nanoseconds=elapsed;
}
}

namespace jfg::boot {
bool native_collision_in_fallback() noexcept { return collision_detail::fallback; }
NativeCollisionCounters native_collision_counters() noexcept { return collision_detail::totals; }
void native_collision_reset_max() noexcept { collision_detail::totals.max_nanoseconds=0U; }
bool native_collision_execute(NativeCollisionFunction* reference,
    std::uint8_t* ram,recomp_context* cpu,unsigned max_nodes,unsigned max_edges) {
  const auto before=collision_detail::totals.native_nodes;
  struct Limits {
    unsigned nodes=collision_detail::limit_nodes,edges=collision_detail::limit_edges;
    ~Limits(){collision_detail::limit_nodes=nodes;collision_detail::limit_edges=edges;}
  } limits;
  collision_detail::limit_nodes=max_nodes<32U?max_nodes:32U;
  collision_detail::limit_edges=max_edges<128U?max_edges:128U;
  collision_detail::execute(reference,ram,cpu);
  return collision_detail::totals.native_nodes!=before;
}
}
