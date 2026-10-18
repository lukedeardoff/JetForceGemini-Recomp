import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from scripts.native_timing_spans import rewrite_source, instruction_kind, memory_expression_matches

ROOT=Path(__file__).resolve().parents[1]

def instruction(pc,word,body,meta=0):
    return f'    // 0x{pc:08X}: synthetic\n    jfg_phase9_execution_probe_predecoded(0x{meta:08x}U, 0x{pc:08x}U, 0x{word:08x}U, ctx);\n    {body}\n'

def arithmetic(pc):
    return instruction(pc,0x24210001,'ctx->r1 = U64(ADD32(ctx->r1, 1));')

class NativeSpanTests(unittest.TestCase):
    def test_exact_fallback_and_bounded_spans(self):
        source='void f(void) {\n'+''.join(arithmetic(0x80010000+4*i) for i in range(40))+'}\n'
        result,counts=rewrite_source(source)
        self.assertEqual(counts['spans'],3)
        self.assertEqual(counts['instructions'],39) # final closing scope is conservative
        self.assertEqual(re.findall(r'jfg_phase9_execution_probe_predecoded\([^\n]*',source),re.findall(r'jfg_phase9_execution_probe_predecoded\([^\n]*',result))
        with self.assertRaises(ValueError):rewrite_source(result)
    def test_labels_callbacks_control_and_cpu_ops_are_boundaries(self):
        for separator in ['L_80010008:\n','    callback(ctx);\n','    if (ctx->r1) return;\n','    uint32_t local = 1;\n']:
            source='void f(void) {\n'+arithmetic(0x80010000)+arithmetic(0x80010004)+separator+arithmetic(0x80010008)+'}\n'
            result,counts=rewrite_source(source)
            self.assertEqual(counts['spans'],0,separator)
        for word in (0x40806000,0x42000018,0x08000000,0x1000ffff,0xbc000000,0x0000000c,0x0000000d,0x44c1f800):
            self.assertFalse(instruction_kind(0,word)[0],hex(word))
    def test_ram_guard_and_partial_prefix(self):
        source='void f(void) {\n'+arithmetic(0x80010000)+instruction(0x80010004,0x8c620000,'ctx->r2 = (int32_t)MEM_W(0, ctx->r3);',0x1000)+arithmetic(0x80010008)+arithmetic(0x8001000c)+'}\n'
        result,counts=rewrite_source(source)
        self.assertEqual(counts['memory_guards'],1)
        self.assertIn('jfg_native_timing_span_ram((uint64_t)(ctx->r3)',result)
        self.assertIn('0x80010000U, 0x80010000U, 0x24210001U, 1U)',result)
    def test_memory_expression_must_match_decoded_operand(self):
        for body in ('ctx->r2 = MEM_W(ctx->r3, -0X4);',
                     'ctx->r2 = MEM_W(-4, ctx->r3);'):
            self.assertTrue(memory_expression_matches(body, 0x8c62fffc, 4))
        self.assertTrue(memory_expression_matches('SD(ctx->f2.u64, U64(S64(0X0)), ctx->r29);', 0xf7a20000, 8))
        for body in ('ctx->r2 = MEM_W(ctx->r4, -4);',
                     'ctx->r2 = MEM_W(ctx->r3, 4);',
                     'ctx->r2 = MEM_H(ctx->r3, -4);',
                     'ctx->r2 = MEM_W(ctx->r3 + 4, -8);',
                     'ctx->r2 = MEM_W(ctx->r3, RELOC_LO16(1));',
                     'ctx->r2 = MEM_W(ctx->r3, -4) + MEM_W(ctx->r3, -4);'):
            self.assertFalse(memory_expression_matches(body, 0x8c62fffc, 4), body)
        bad = instruction(0x80010004,0x8c62fffc,'ctx->r2 = MEM_W(ctx->r4, -4);',0x1000)
        _, counts = rewrite_source('void f(void) {\n'+arithmetic(0x80010000)+bad+arithmetic(0x80010008)+'}\n')
        self.assertEqual(counts['spans'],0)

    def test_pure_relocation_arithmetic_keeps_hardware_boundaries(self):
        hi=instruction(0x80010000,0x3c080010,'ctx->r8 = S32(RELOC_HI16(1, 0x7654) << 16);')
        lo=instruction(0x80010004,0x25087654,'ctx->r8 = ADD32(ctx->r8, RELOC_LO16(1, 0x7654));')
        _,counts=rewrite_source('void f(void) {\n'+hi+lo+arithmetic(0x80010008)+'}\n')
        self.assertEqual(counts['instructions'],2)
        for body in ('ctx->r2 = MEM_W(ctx->r3, RELOC_LO16(1, 0x7654));',
                     'ctx->r2 = hidden_callback(RELOC_HI16(1, 0));'):
            boundary=instruction(0x80010004,0x8c627654,body,0x1000)
            _,counts=rewrite_source('void f(void) {\n'+hi+boundary+arithmetic(0x80010008)+'}\n')
            self.assertEqual(counts['spans'],0)

    def test_relocated_memory_guards_actual_signed_operand(self):
        load=instruction(0x80010004,0x8c627654,'ctx->r2 = MEM_W((int16_t)RELOC_LO16(1, 0x7654), ctx->r3);',0x1000)
        result,counts=rewrite_source('void f(void) {\n'+arithmetic(0x80010000)+load+arithmetic(0x80010008)+'}\n')
        self.assertEqual(counts['memory_guards'],1)
        self.assertIn('(uint64_t)(int64_t)((int16_t)RELOC_LO16(1, 0x7654))',result)
        for operand in ('RELOC_LO16(1, 0x7654)', '(int16_t)RELOC_LO16(call(), 0)', '(int16_t)RELOC_LO16(1, 0) + 4'):
            body=f'ctx->r2 = MEM_W({operand}, ctx->r3);'
            self.assertFalse(memory_expression_matches(body,0x8c627654,4))
        self.assertFalse(memory_expression_matches('ctx->r2=MEM_W((int16_t)RELOC_LO16(1,0),ctx->r4);',0x8c627654,4))

    def test_pure_float_multiply_keeps_control_and_unknown_boundaries(self):
        for macro,word in [('MUL_S',0x46020802),('MUL_D',0x46220802)]:
            body=instruction(0x80010000,word,f'ctx->sf0 = {macro}(ctx->sf1, ctx->sf2);',0x100)
            _,counts=rewrite_source('void f(void) {\n'+body+arithmetic(0x80010004)+arithmetic(0x80010008)+'}\n')
            self.assertEqual(counts['instructions'],2,macro)
        for macro in ['MUL_UNKNOWN','write_fcsr','callback']:
            body=instruction(0x80010004,0x46020802,f'ctx->sf0 = {macro}(ctx->sf1, ctx->sf2);',0x100)
            _,counts=rewrite_source('void f(void) {\n'+arithmetic(0x80010000)+body+arithmetic(0x80010008)+'}\n')
            self.assertEqual(counts['spans'],0,macro)
        self.assertFalse(instruction_kind(0x100,0x44c1f800)[0]) # CTC1 is still a boundary.

    def test_malformed_pc_rejected(self):
        with self.assertRaises(ValueError):rewrite_source(arithmetic(0x80010000).replace('// 0x80010000','// 0x80010004'))
    def test_compiled_ram_mmio_and_poll_fallback_equivalence(self):
        compiler=shutil.which('clang++') or shutil.which('g++') or shutil.which('cl')
        if not compiler:self.skipTest('C++ compiler unavailable')
        body=arithmetic(0x80010000)+instruction(0x80010004,0x8c620000,'ctx->r2 = (int32_t)MEM_W(0, ctx->r3);',0x1000)+instruction(0x80010008,0x24440001,'ctx->r4 = ctx->r2 + 1;')+instruction(0x8001000c,0x00a01821,'ctx->r3 = ctx->r5;')+instruction(0x80010010,0xac640004,'MEM_W(4, ctx->r3) = (int32_t)ctx->r4;',0x2000)+arithmetic(0x80010014)+arithmetic(0x80010018)
        body += instruction(0x8001001c,0x3c080010,'ctx->r8 = S32(RELOC_HI16(1, 0x7654) << 16);')+instruction(0x80010020,0x25087654,'ctx->r8 = ADD32(ctx->r8, RELOC_LO16(1, 0x7654));')+arithmetic(0x80010024)
        body += instruction(0x80010028,0x8c697654,'ctx->r9 = MEM_W((int16_t)RELOC_LO16(1, 0x7654), ctx->r3);',0x1000)+instruction(0x8001002c,0xaca97654,'MEM_W((int16_t)RELOC_LO16(1, 0x7654), ctx->r5) = (int32_t)ctx->r9;',0x2000)+arithmetic(0x80010030)
        body += instruction(0x80010034,0x46020802,'ctx->sf0 = MUL_S(ctx->sf1, ctx->sf2);',0x100)+instruction(0x80010038,0x46220802,'ctx->df0 = MUL_D(ctx->df1, ctx->df2);',0x100)+arithmetic(0x8001003c)
        original='void original(uint8_t* rdram,recomp_context* ctx) {\n'+body+'}\n'
        optimized,counts=rewrite_source(original.replace('void original','void optimized'))
        self.assertGreater(counts['instructions'],3)
        # Verify that the differential fixture exercises both admitted and
        # rejected relocated accesses, rather than only misalignment fallbacks.
        optimized=re.sub(r'jfg_native_timing_span_ram\(([^\n]*RELOC_LO16[^\n]*)\)',
                         r'counted_relocated_ram(\1)',optimized)
        prefix=r'''
#include "jfg/boot/original_timing_fast.h"
#include <array>
#include <bit>
#include <cfenv>
#pragma STDC FENV_ACCESS ON
#include <tuple>
#include <cstdio>
#include <cstdlib>
extern "C" { JfgOriginalTimingFast* jfg_original_timing_state=nullptr; }
static uint64_t revision=1,count=0;
static unsigned relocated_admitted=0,relocated_rejected=0;
static bool counted_relocated_ram(uint64_t address,unsigned width) {
 const bool result=jfg_native_timing_span_ram(address,width);
 if(result)++relocated_admitted;else ++relocated_rejected;
 return result;
}
static unsigned slow=0,mmio=0;static int32_t peripheral=0;
extern "C" void jfg_phase9_execution_probe_predecoded(unsigned m,unsigned pc,unsigned w,void* ctx){
 auto& s=*jfg_original_timing_state;++slow;++s.cpu_count;++s.observed_guest_instructions;
 s.host_clock_poll_budget=128;s.execution_fast_deadline=UINT64_MAX;s.execution_fast_revision=revision;
 s.guest_previous_delay=s.guest_previous_branch&&pc==s.guest_last_pc+4;s.guest_previous_branch=false;
 s.guest_last_pc=pc;s.guest_last_word=w;count=s.cpu_count;
 jfg_native_timing_refresh(&s,static_cast<recomp_context*>(ctx));
}
static int32_t* access_word(uint8_t* ram,uint64_t address){
 if(jfg_native_timing_span_ram(address,4))return reinterpret_cast<int32_t*>(ram+(address&0xff));
 const auto pc=jfg_original_timing_state->guest_last_pc;
 if(pc!=0x80010004U&&pc!=0x80010010U&&pc!=0x80010028U&&pc!=0x8001002cU)std::abort();
 ++revision;++mmio;return &peripheral;
}
#define MEM_W(offset,reg) (*access_word(rdram,(uint64_t)(reg)+(offset)))
static uint32_t section_addresses[2]{};
#define S32(x) ((int32_t)(x))
#define RELOC_HI16(section,offset) (((section_addresses[section]+(offset))>>16)+(((section_addresses[section]+(offset))>>15)&1))
#define RELOC_LO16(section,offset) ((int16_t)(section_addresses[section]+(offset)))
#define MUL_S(a,b) ((a)*(b))
#define MUL_D(a,b) ((a)*(b))
#define U64(x) ((uint64_t)(x))
#define ADD32(x,y) ((int32_t)((x)+(y)))
#define jfg_phase9_execution_probe_predecoded(m,p,w,c) jfg_native_timing_step(m,p,w,c)
'''
        suffix=r'''
int main(){
 for(unsigned n=0;n<4096;++n){
  alignas(8) std::array<uint8_t,256> ram{};JfgOriginalTimingFast s{};recomp_context c{};
  s.diagnostic_host_clock=true;s.native_spans_enabled=true;s.play_mode=true;
  s.host_clock_poll_budget=n%32;s.cpu_count=100;s.io_guest_count=&count;s.io_revision=&revision;
  s.execution_fast_revision=1;s.execution_fast_deadline=UINT64_MAX;s.guest_last_pc=0x8000fffc;
  s.guest_previous_branch=n%3==0;c.status_reg=0x20000000U;c.r1=n;
  c.r3=n%2?0xffffffff80000008ULL:0xffffffffa4400000ULL;
  c.r5=n%4<2?c.r3:(n%2?0xffffffffa4400000ULL:0xffffffff80000008ULL);
  const uint32_t floats[]{0U,0x80000000U,0x3f800001U,0xbf800001U,0x7f7fffffU,0x00800000U,1U,0x7f800000U};
  const uint64_t doubles[]{0ULL,0x8000000000000000ULL,0x3ff0000000000001ULL,0xbff0000000000001ULL,0x7fefffffffffffffULL,0x0010000000000000ULL,1ULL,0x7ff0000000000000ULL};
  c.sf1=std::bit_cast<float>(floats[(n/4)%8]);c.sf2=std::bit_cast<float>(uint32_t{0x3fa66666});
  c.df1=std::bit_cast<double>(doubles[(n/4)%8]);c.df2=std::bit_cast<double>(uint64_t{0x3ff4cccccccccccdULL});
  const int rounding[]{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
  auto run=[&](bool optimized_path){
   if(std::fesetround(rounding[n%4])!=0)std::abort();std::feclearexcept(FE_ALL_EXCEPT);
   section_addresses[1]=0x80000000U+n*0x7ff0U+((n&4U)?1U:0U);ram.fill(0);revision=1;count=100;slow=mmio=0;peripheral=17;
   JfgOriginalTimingFast local=s;recomp_context cpu=c;jfg_original_timing_state=&local;
   jfg_native_timing_refresh(&local,&cpu);
   if(optimized_path)optimized(ram.data(),&cpu);else original(ram.data(),&cpu);
   return std::tuple(std::bit_cast<uint32_t>(cpu.sf0),std::bit_cast<uint64_t>(cpu.df0),std::fetestexcept(FE_ALL_EXCEPT),cpu.r1,cpu.r2,cpu.r3,cpu.r4,cpu.r5,cpu.r8,cpu.r9,ram,revision,count,slow,mmio,peripheral,
     local.observed_guest_instructions,local.execution_fast_hits,local.cpu_count,
     local.host_clock_poll_budget,local.guest_last_pc,local.guest_last_word,
     local.guest_previous_branch,local.guest_previous_delay);
  };
  if(run(false)!=run(true)){printf("mismatch %u\n",n);return 1;}
 }
 if(relocated_admitted<100U||relocated_rejected<100U){printf("relocated coverage %u %u\\n",relocated_admitted,relocated_rejected);return 2;}
 puts("4096 generated RAM/MMIO/safepoint pairs match");
}
'''
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp);cpp=path/'spans.cpp';exe=path/('spans.exe' if os.name=='nt' else 'spans')
            cpp.write_text(prefix+original+optimized+suffix)
            if Path(compiler).name.lower() in ('cl', 'cl.exe'):
                command=[compiler,'/nologo','/std:c++20','/O2','/EHsc','/fp:strict',
                    '/I'+str(ROOT/'include'),'/I'+str(ROOT/'tests/fixtures/original-timing-context'),
                    str(cpp),'/Fe'+str(exe),'/Fo'+str(path/'spans.obj')]
            else:
                command=[compiler,'-std=c++20','-O2','-frounding-math','-ffp-contract=off','-I'+str(ROOT/'include'),
                    '-I'+str(ROOT/'tests/fixtures/original-timing-context'),str(cpp),'-o',str(exe)]
            subprocess.run(command,check=True,capture_output=True,text=True)
            result=subprocess.run([str(exe)],check=True,capture_output=True,text=True)
            self.assertIn('4096 generated RAM/MMIO/safepoint pairs match',result.stdout)

if __name__=='__main__':unittest.main()
