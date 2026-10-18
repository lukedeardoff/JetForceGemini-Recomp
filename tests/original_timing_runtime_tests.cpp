#include "jfg/audio/vi_playback_clock.hpp"
#include "jfg/boot/original_timing_ai_dma.hpp"
#include "jfg/boot/original_timing_pi_dma.hpp"
#include <vector>
#include "jfg/runtime/window_input_mailbox.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
namespace {
void check(bool okay, const char* text) { if (!okay) { std::cerr << text << '\n'; std::exit(1); } }
}
int main() {
  using Clock = jfg::audio::ViPlaybackClock;
  using namespace std::chrono_literals;
  Clock clock;const Clock::Time origin{};
  clock.anchor(0,origin,0);
  check(clock.deadline(46875000,origin+1100ms,0)==origin+1s,"ordinary delay must catch up");
  check(clock.deadline(46875000,origin+1200ms,1)==origin+1200ms,"observed starvation reanchors playback");
  check(clock.deadline(93750000,origin+2100ms,1)==origin+2200ms,"new origin retains physical Count rate");
  check(clock.deadline(93750000,origin+2400ms,1)==origin+2200ms,"one underrun must not reanchor twice");
  jfg::WindowInputMailbox input;std::array<bool,256> down{},pressed{};bool close=false;
  input.key(32,true);input.key(32,false);input.consume(down,pressed,close);
  check(!down[32] && pressed[32],"short press survives between game polls");
  input.motion(9000,-9000);input.wheel(10000);input.lose_focus();input.consume(down,pressed,close);
  auto motion=input.take_motion();check(!pressed[32] && motion.focus_lost && !motion.x && !motion.y && !motion.wheel,"focus loss clears old input");
  input.motion(9000,-9000);input.wheel(10000);motion=input.take_motion();
  check(motion.x==4096 && motion.y==-4096 && motion.wheel==3840,"unbounded host input cannot overflow accumulators");
  input.close();input.consume(down,pressed,close);check(close,"close request reaches game thread");
  std::thread producer([&]{for(int i=0;i<10000;++i){input.key(8,true);input.key(8,false);}input.key(9,true);});
  for(int i=0;i<10000;++i)(void)input.consume_frame(down,pressed,close);
  producer.join();input.consume(down,pressed,close);check(down[9] && !down[8],"mailbox final state matches producer");
  jfg::boot::OriginalTimingAiDma ai;
  check(ai.write(8,1,0,4096,783520) && ai.write(16,2210,0,4096,783520) &&
      ai.write(20,15,0,4096,783520),"AI format configuration");
  check(ai.write(4,2944,100,4096,783520),"first AI slot");
  const auto end=*ai.deadline();
  check(end==1566999,"AI uses physical Count/sample ratio");
  check(ai.write(4,2944,100,4096,783520) && !ai.write(4,2944,100,4096,783520),"AI has exactly two slots");
  ai.advance(end-1);check(!ai.interrupt(),"AI cannot complete early");
  ai.advance(end);check(ai.interrupt() && *ai.deadline()==end+(end-100),"queued AI buffer starts at original deadline");
  check(ai.write(12,0,end,4096,783520) && !ai.interrupt(),"AI interrupt acknowledgment");
  ai.advance(*ai.deadline());check(ai.interrupt() && !ai.deadline(),"AI second completion");
  jfg::boot::OriginalTimingAiDma native_ai;
  check(!native_ai.native_queue_capacity(1) && !native_ai.native_queue_capacity(5),
      "native queue expansion remains bounded");
  check(native_ai.native_queue_capacity(4),"native playback opts into catch-up slots");
  check(native_ai.write(8,1,0,4096,783520) && native_ai.write(16,2210,0,4096,783520) &&
      native_ai.write(20,15,0,4096,783520),"native AI format");
  for(unsigned i=0;i<4;++i) {
    check(native_ai.read(12,100)==(i?0x40000000U:0U),"free native AI capacity is visible");
    check(native_ai.write(4,2944,100,4096,783520),"native catch-up retains each buffer");
  }
  check(native_ai.read(12,100)==0xC0000000U &&
      !native_ai.write(4,2944,100,4096,783520),"native queue still rejects overflow");
  check(!native_ai.native_queue_capacity(2),"capacity cannot discard queued audio");
  for(unsigned i=1;i<=4;++i) {
    const auto expected=100U+i*(end-100U);
    check(native_ai.deadline()==expected,"native buffers retain exact sample duration");
    native_ai.advance(expected);
    check(native_ai.interrupt(),"each native completion signals interrupt");
    check(native_ai.write(12,0,expected,4096,783520),"acknowledge native completion");
  }
  check(!native_ai.deadline() && native_ai.read(12,100+4*(end-100))==0U,
      "native catch-up queue drains completely");
  jfg::boot::OriginalTimingAiDma near_limit;
  check(near_limit.native_queue_capacity(4) &&
      near_limit.write(8,1,0,4096,783520) &&
      near_limit.write(16,2210,0,4096,783520) &&
      near_limit.write(20,15,0,4096,783520),"near-limit native AI setup");
  const auto late=UINT64_MAX-3U*(end-100U)-100U;
  for(unsigned i=0;i<3;++i)
    check(near_limit.write(4,2944,late,4096,783520),"retained native deadline fits");
  check(!near_limit.write(4,2944,late,4096,783520),"native queue tail cannot overflow Count");
  for(unsigned i=0;i<3;++i)near_limit.advance(*near_limit.deadline());
  check(!near_limit.deadline(),"rejected overflow preserves retained native queue");
  {
    for(unsigned length: {1U,2U,128U,257U,4096U}) {
      std::vector<std::uint8_t> rom(8192),a(8192,0xCC),b(8192,0xCC);
      for(unsigned i=0;i<rom.size();++i)rom[i]=static_cast<std::uint8_t>(i*13U);
      jfg::boot::OriginalTimingPiDma slow,fast;
      for(unsigned i=0;i<4;++i) {
        check(slow.write(0x14U+4U*i,0xFFU,0,a,rom),"reference bus setup");
        check(fast.write(0x14U+4U*i,0xFFU,0,b,rom),"native bus setup");
      }
      for(auto* dma: {&slow,&fast}) {
        check(dma->write(0,128,0,a,rom),"ROM destination setup");
        check(dma->write(4,0x10000100,0,a,rom),"ROM source setup");
      }
      check(slow.write(12,length-1,100,a,rom),"reference ROM read");
      check(fast.write(12,length-1,100,b,rom,nullptr,nullptr,true),"native ROM read");
      check(fast.rom_transfers().calls==1U && fast.rom_transfers().bytes==length &&
        fast.rom_transfers().native_calls==1U && fast.rom_transfers().scheduled_ticks==1U &&
        fast.rom_transfers().bus_ticks==slow.rom_transfers().bus_ticks &&
        slow.rom_transfers().scheduled_ticks==slow.rom_transfers().bus_ticks &&
        slow.rom_transfers().native_calls==0U,"ROM transfer counters separate bus and scheduled duration");
      check(a==b && slow.read(12)==fast.read(12) && fast.read(16)==3U,
        "native ROM data, registers and busy visibility");
      check(fast.deadline()==101U && slow.deadline()>101U,"native completion distinct from bus duration");
      fast.advance(100);check(!fast.interrupt(),"native completion is not immediate/reentrant");
      fast.advance(101);check(fast.interrupt() && fast.read(16)==0U,"native completion through event dispatcher");
      check(fast.write(16,2,101,b,rom) && !fast.interrupt(),"native PI interrupt acknowledged normally");
      const auto before=b;
      check(!fast.write(12,UINT32_MAX,102,b,rom,nullptr,nullptr,true) && b==before,"native invalid length preserves memory");
      check(!fast.write(12,0,UINT64_MAX,b,rom,nullptr,nullptr,true) && b==before,"native deadline overflow rejected");
      check(fast.rom_transfers().calls==1U,"rejected reads do not change ROM counters");
    }
    jfg::boot::OriginalTimingPiDma flash;
    std::vector<std::uint8_t> ram(64),rom(64);
    auto transfer=+[](void*,bool,std::uint32_t,std::uint32_t,std::uint32_t,std::span<std::uint8_t>){return true;};
    check(flash.write(4,0x08000000,100,ram,rom),"flash address");
    check(flash.write(12,7,100,ram,rom,transfer,nullptr,true) && flash.deadline()==4196U,
      "native ROM policy leaves FlashRAM timing unchanged");
    check(flash.rom_transfers().calls==0U,"FlashRAM is excluded from ROM counters");
  }
  jfg::boot::OriginalTimingPiDma pi;
  std::vector<std::uint8_t> ram(128),rom(128);rom[0]=0x12;rom[1]=0x34;
  check(pi.write(0,0,0,ram,rom) && pi.write(4,0x10000000,0,ram,rom),"PI address setup");
  check(pi.write(12,1,0,ram,rom),"PI short transfer");
  check(ram[3]==0x12 && ram[2]==0x34 && *pi.deadline()==14,"PI data byte order and cartridge clock");
  check(!pi.write(12,1,0,ram,rom),"PI rejects concurrent DMA");
  pi.advance(13);check(!pi.interrupt(),"PI cannot complete early");
  pi.advance(14);check(pi.interrupt() && !pi.deadline(),"PI completion at bus deadline");
  check(pi.write(16,2,14,ram,rom) && !pi.interrupt(),"PI interrupt acknowledgment");
  return 0;
}
