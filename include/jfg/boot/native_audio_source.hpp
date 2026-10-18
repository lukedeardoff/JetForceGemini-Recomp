#pragma once
#include "jfg/boot/hle.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace jfg::boot {
// US retail synthesis DMA callback. In the native timing path the ROM is
// already resident. Publish a cache entry only after its bytes are available;
// do not enqueue a simulated cartridge request whose descriptor can outlive
// the audio frame. The original frame-based cache reclamation remains owner
// of the free list. Instruction-clock reference execution keeps its callback.
inline constexpr std::uint32_t kAudioSourceCallback = 0x800024C4U;
inline constexpr std::uint32_t kAudioCacheControl = 0x800F1B68U;
inline constexpr std::uint32_t kAudioCacheNodes = 0x800F1B78U;
inline constexpr std::uint32_t kAudioCacheFrame = 0x800A06D0U;
inline constexpr std::uint32_t kAudioDmaCount = 0x800A06D4U;
inline constexpr std::uint32_t kAudioCacheCount = 76U;
inline constexpr std::uint32_t kAudioCacheBytes = 640U;
inline constexpr std::array<std::uint32_t,4> kAudioSourceSignature{
  0x3C09800FU,0x25291B68U,0x27BDFFB0U,0x8D280004U};

struct NativeAudioSourceResult {
  std::uint32_t physical_address = 0;
  bool cache_hit = false;
};

[[nodiscard]] inline std::optional<NativeAudioSourceResult>
provide_native_audio_source(hle::GuestMemory& memory,
                            std::span<const std::uint8_t> rom,
                            std::uint32_t source, std::uint32_t length) noexcept {
  struct Node { std::uint32_t next=0,prev=0,start=0,frame=0,buffer=0; };
  std::array<Node,kAudioCacheCount> nodes{};
  std::array<bool,kAudioCacheCount> seen{};
  const auto index=[](std::uint32_t address)->std::optional<std::size_t> {
    if(address<kAudioCacheNodes || (address-kAudioCacheNodes)%20U!=0U) return {};
    const auto i=(address-kAudioCacheNodes)/20U;
    if(i>=kAudioCacheCount) return {};
    return i;
  };
  const auto span_ok=[&](std::uint32_t address,std::uint32_t size) {
    return address>=0x80000000U && (address&3U)==0U &&
      std::uint64_t(address-0x80000000U)+size<=memory.size();
  };
  const auto overlaps=[](std::uint32_t a,std::uint32_t n,std::uint32_t b,std::uint32_t m) {
    return std::uint64_t(a)<std::uint64_t(b)+m && std::uint64_t(b)<std::uint64_t(a)+n;
  };
  if(length==0U || length>kAudioCacheBytes-(source&1U) ||
     source>rom.size() || length>rom.size()-source) return {};
  for(std::uint32_t i=0;i<kAudioSourceSignature.size();++i) {
    std::uint32_t word=0;
    if(!memory.read_u32(kAudioSourceCallback+i*4U,word) || word!=kAudioSourceSignature[i]) return {};
  }
  std::uint32_t used=0,free=0,frame=0,pending=0;
  if(!memory.read_u32(kAudioCacheControl+4U,used) ||
     !memory.read_u32(kAudioCacheControl+8U,free) ||
     !memory.read_u32(kAudioCacheFrame,frame) ||
     !memory.read_u32(kAudioDmaCount,pending) || pending!=0U) return {};
  // Preflight both lists and destination ranges before any guest-memory write.
  // Native mode is chosen before the first callback, never over outstanding
  // requests from the original asynchronous provider.
  for(std::uint32_t i=0;i<kAudioCacheCount;++i) {
    auto& n=nodes[i];const auto p=kAudioCacheNodes+i*20U;
    if(!memory.read_u32(p,n.next) || !memory.read_u32(p+4U,n.prev) ||
       !memory.read_u32(p+8U,n.start) || !memory.read_u32(p+12U,n.frame) ||
       !memory.read_u32(p+16U,n.buffer) || !span_ok(n.buffer,kAudioCacheBytes) ||
       (n.buffer&7U)!=0U || overlaps(n.buffer,kAudioCacheBytes,kAudioCacheControl,16U+kAudioCacheCount*20U) ||
       overlaps(n.buffer,kAudioCacheBytes,kAudioCacheFrame,8U)) return {};
    for(std::uint32_t j=0;j<i;++j)
      if(overlaps(n.buffer,kAudioCacheBytes,nodes[j].buffer,kAudioCacheBytes)) return {};
  }
  std::size_t visited=0;
  for(unsigned list=0;list<2U;++list) {
    auto p=list==0U?used:free;std::uint32_t previous=0,last_start=0;
    while(p!=0U) {
      const auto i=index(p);if(!i || seen[*i]) return {};
      const auto& n=nodes[*i];if(n.prev!=previous) return {};
      if(list==0U && (n.start<last_start || n.start>rom.size() || kAudioCacheBytes>rom.size()-n.start)) return {};
      seen[*i]=true;++visited;previous=p;last_start=n.start;p=n.next;
    }
  }
  if(visited!=kAudioCacheCount) return {};
  std::uint32_t previous=0,next=used;
  while(next!=0U) {
    const auto& n=nodes[*index(next)];
    if(source<n.start) break;
    if(std::uint64_t(source)+length<=std::uint64_t(n.start)+kAudioCacheBytes) {
      if(!memory.write_u32(next+12U,frame)) return {};
      return NativeAudioSourceResult{n.buffer-0x80000000U+source-n.start,true};
    }
    previous=next;next=n.next;
  }
  if(free==0U) return {}; // Never return an unrelated sample on exhaustion.
  const auto aligned=source&~1U;
  if(aligned>rom.size() || kAudioCacheBytes>rom.size()-aligned) return {};
  const auto& slot=nodes[*index(free)];
  // Copy first. There is no deferred request or completion message to reuse.
  for(std::uint32_t off=0;off<kAudioCacheBytes;off+=4U) {
    const auto at=aligned+off;
    const auto word=(std::uint32_t(rom[at])<<24U)|(std::uint32_t(rom[at+1U])<<16U)|
                    (std::uint32_t(rom[at+2U])<<8U)|std::uint32_t(rom[at+3U]);
    if(!memory.write_u32(slot.buffer+off,word)) return {};
  }
  if(!memory.write_u32(kAudioCacheControl+8U,slot.next) ||
     (slot.next!=0U && !memory.write_u32(slot.next+4U,0U)) ||
     !memory.write_u32(free, next) || !memory.write_u32(free+4U,previous) ||
     !memory.write_u32(free+8U,aligned) || !memory.write_u32(free+12U,frame) ||
     !memory.write_u32(previous!=0U?previous:kAudioCacheControl+4U,free) ||
     (next!=0U && !memory.write_u32(next+4U,free))) return {};
  return NativeAudioSourceResult{slot.buffer-0x80000000U+(source&1U),false};
}
} // namespace jfg::boot
