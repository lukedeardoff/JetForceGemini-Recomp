#pragma once
// Include after the selected original/native timing header. The generated and
// source timing headers share this ABI and must not both be included.
// These resident leaves never write guest RAM, call another guest function or
// yield. Admission reserves the entire original instruction path within the
// existing host-clock quantum; unsupported states retain generated execution.
// Lookup values come from guest RAM, preserving the game's quantized results.



// Only this bounded, callback-free finite-input leaf may reserve 37 operations.
// It uses the same admission certificate and preserves the original path count.
static inline bool jfg_native_angle_try(uint8_t* rdram,recomp_context* ctx) {
 JfgOriginalTimingFast* s=jfg_original_timing_state;
 if(!s || !ctx || !rdram || s->rdram!=rdram || !s->native_spans_enabled ||
    !s->diagnostic_host_clock || !s->native_fast_ready ||
    s->host_clock_poll_budget<=37U || ctx->r0!=0 ||
    !ctx->mips3_float_mode || ctx->f_odd!=&ctx->f1.u32l ||
    s->native_fast_status!=ctx->status_reg ||
    (s->native_fast_status&0x20000000U)==0U ||
    s->execution_fast_revision!=*s->io_revision)return false;
 if((ctx->f12.u32l&0x7f800000U)==0x7f800000U ||
    (ctx->f14.u32l&0x7f800000U)==0x7f800000U)return false;
 float y=ctx->f12.fl,x=ctx->f14.fl;
 unsigned count=9U,last_pc=0x80049d7cU,last_word=0U,base=0U;
 ctx->f0.u32l=0U;ctx->r2=0U;
 if(y==0.0f && x==0.0f) {
  ctx->fcr31|=0x00800000U;
 } else {
  count=(y==0.0f)?8U:5U;
  if(y<0.0f) {
   y=-y;count+=4U;
   if(x<0.0f){x=-x;base=0x8000U;count+=3U;}
   else {ctx->f13.fl=y;float swap=y;y=x;x=swap;base=0xc000U;count+=6U;}
  } else {
   count+=3U;
   if(x<0.0f){x=-x;ctx->f13.fl=y;float swap=y;y=x;x=swap;base=0x4000U;count+=8U;}
   else count+=1U;
  }
  ctx->f12.fl=y;ctx->f14.fl=x;
  const bool direct=y<x;
  ctx->fcr31=(ctx->fcr31&~0x00800000U)|(direct?0x00800000U:0U);
  const float ratio=direct?y/x:x/y;
  const float scaled=ratio*2048.0f;
  const uint32_t converted=(uint32_t)CVT_W_S(scaled);
  const unsigned index=converted&0xffeU;
  int16_t angle;memcpy(&angle,rdram+((0xa8d98U+index)^2U),sizeof(angle));
  ctx->r1=0x45000000U;ctx->f1.u32l=0x45000000U;
  ctx->f10.u32l=converted;ctx->r9=index;ctx->r8=(uint64_t)(int64_t)angle;
  ctx->r2=(direct?base+(unsigned)angle:base+0x4000U-(unsigned)angle)&0xffffU;
  count+=direct?16U:18U;
  if(direct){last_pc=0x80049d38U;last_word=0x3042ffffU;}
 }
 s->host_clock_poll_budget-=count;
 s->observed_guest_instructions+=count;s->execution_fast_hits+=count;
 s->native_span_instructions+=count;++s->native_span_commits;
 *s->io_guest_count=s->cpu_count;
 s->guest_last_pc=last_pc;s->guest_last_word=last_word;
 s->guest_previous_branch=false;s->guest_previous_delay=true;
 return true;
}

static inline bool jfg_native_angle_diff_try(uint8_t* rdram,recomp_context* ctx) {
 JfgOriginalTimingFast* s=jfg_original_timing_state;
 if(!s || !ctx || !rdram || s->rdram!=rdram || ctx->r0!=0 ||
    !jfg_native_timing_span_begin(0U,10U,ctx))return false;
 ctx->r2=U64(SUB32(ctx->r5,ctx->r4));ctx->r8=65536U;
 unsigned count=10U,pc=0x80049da4U,word=0U;
 if(SIGNED(ctx->r2)>=32768) {
  ctx->r1=0U;ctx->r2=U64(SUB32(ctx->r2,ctx->r8));
  count=7U;pc=0x80049dacU;word=0x00481022U;
 } else {
  ctx->r1=SIGNED(ctx->r2)<-32767?1U:0U;
  if(ctx->r1) {ctx->r2=U64(ADD32(ctx->r2,ctx->r8));pc=0x80049db4U;word=0x00481020U;}
 }
 jfg_native_timing_span_commit(0x80049d80U,pc,word,count);
 s->guest_previous_delay=true;
 return true;
}



static inline bool jfg_native_trig_try(uint8_t* rdram,recomp_context* ctx,bool cosine) {
 JfgOriginalTimingFast* s=jfg_original_timing_state;
 if(!s || !ctx || !rdram || s->rdram!=rdram || !s->native_spans_enabled ||
    !s->diagnostic_host_clock || !s->native_fast_ready ||
    s->host_clock_poll_budget<=20U || ctx->r0!=0 ||
    !ctx->mips3_float_mode || ctx->f_odd!=&ctx->f1.u32l ||
    s->native_fast_status!=ctx->status_reg ||
    (s->native_fast_status&0x20000000U)==0U ||
    s->execution_fast_revision!=*s->io_revision)return false;
 const auto argument=cosine?U64(ADD32(ctx->r4,0x4000)):ctx->r4;
 unsigned index=(uint32_t(argument)>>4U)&0x3ffU;
 unsigned count=13U+(cosine?1U:0U);
 if(argument&0x4000U) {
   const bool lower=(argument&0xfU)!=0U;
   index=0x400U-(index+(lower?1U:0U));
   count+=lower?5U:4U;
 }
 const bool negative=(argument&0x8000U)!=0U;
 uint32_t value=0U;memcpy(&value,rdram+0xa7d94U+index*4U,4U);
 if((value&0x7f800000U)==0x7f800000U)return false;
 if(cosine)ctx->r4=argument;
 ctx->r8=index*4U;
 ctx->r9=argument&0x8000U;
 ctx->r10=U64(S32(0x800a7d94U+index*4U));
 ctx->f0.u32l=value;
 if(negative){ctx->f0.fl=-ctx->f0.fl;++count;}
 jfg_native_timing_span_commit(cosine?0x80049c30U:0x80049c34U,0x80049c80U,0U,count);
 s->guest_previous_delay=true;
 return true;
}
