#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "jfg/runtime/snapshot_copy.hpp"
#include "funcs.h"
#if __has_include("jfg_original_timing.h")
#include "jfg_original_timing.h"
#else
#define JFG_ORIGINAL_TIMING_GENERATION 0
#include "jfg/boot/original_timing_fast.h"
#endif
#ifndef JFG_NATIVE_TIMING_SPAN_GENERATION
#define JFG_NATIVE_TIMING_SPAN_GENERATION 0
#endif
#if defined(JFG_EXPERIMENTAL_NATIVE_PLAY_TIMING) && JFG_NATIVE_TIMING_SPAN_GENERATION != 1
#error Native timing preview requires a qualified generated span root
#endif
#include "jfg/boot/hle.hpp"
#include "jfg/boot/sync_print_sink.hpp"
#include "jfg/boot/native_audio_source.hpp"
#include "jfg/boot/sound_queue_recovery.hpp"
#include "jfg/boot/reset_handoff.hpp"
#include "jfg/boot/ipl_handoff.hpp"
#include "jfg/boot/gameplay_trace.hpp"
#include "jfg/boot/function_profile.hpp"
#include "jfg/renderer/submission_profile.hpp"
#include "jfg/boot/native_resident_math.h"
#include "jfg/boot/native_collision.hpp"
#include "jfg/renderer/interpolation_bridge.hpp"
#include "jfg/renderer/frame_diagnostics_state.hpp"
#include "jfg/boot/host_count_clock.hpp"
#include "jfg/boot/native_frame_recovery.hpp"
#include "jfg/boot/count_sample_tape.hpp"
#include "jfg/boot/recorded_input_timeline.hpp"
#include "jfg/mod/navigation_mod.hpp"
#include "jfg/runtime/async_bounded_trace.hpp"
#include "jfg/runtime/async_snapshots.hpp"
#include "jfg/runtime/async_diagnostic_file.hpp"
#include "jfg/runtime/async_volume_file.hpp"
#include "jfg/audio/master_volume.hpp"
#include "jfg/audio/playback_buffer.hpp"
#include "jfg/audio/vi_playback_clock.hpp"
#include "jfg/runtime/window_input_mailbox.hpp"
#include <future>
#include <atomic>
#include "jfg/boot/runlink_module_table.hpp"
#include "jfg/boot/runlink_native_identity.hpp"
#include "jfg/boot/thread_scheduler.hpp"
#include "jfg/boot/guest_thread_transport.hpp"
#include "jfg/boot/tlb.hpp"
#include "jfg/boot/reference_cache.hpp"
#include "jfg/boot/cpu_status.hpp"
#include "jfg/boot/mi_interrupt_mask.hpp"
#include "jfg/boot/sp_status.hpp"
#include "jfg/boot/reference_sp_dma.hpp"
#include "jfg/boot/reference_pif_boot.hpp"
#include "jfg/boot/original_timing_pi_dma.hpp"
#include "jfg/boot/reference_compare.hpp"
#include "jfg/boot/reference_si_dma.hpp"
#include "jfg/boot/reference_event_commit.hpp"
#include "jfg/boot/native_event_batch.hpp"
#include "generated_lookup_entry.h"
#include "jfg/boot/native_periodic_deadline.hpp"
#include "jfg/boot/native_audio_playback.hpp"
#include "jfg/boot/reference_flash_bus.hpp"
#include "jfg/boot/original_timing_ai_dma.hpp"
#include "jfg/boot/point_probe.hpp"
#include "jfg/boot/device_event_probe.h"
#include "jfg/boot/eret_transfer_probe.hpp"
#include "jfg/boot/instruction_effect_trace.hpp"
#include "jfg/runtime/cpu_operation_bridge.h"
#include "jfg/boot/timers.hpp"
#include "jfg/boot/si_deadline.hpp"
#include "jfg/runtime/generated_overlay_runtime.hpp"
#include "jfg/runtime/input_stick.hpp"
#include "jfg/runtime/controller_mapping.hpp"
#include "jfg/runtime/controller_ports.hpp"
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
#include "jfg/runtime/sdl_controller_backend.hpp"
#endif
#include "jfg/runtime/pc_input.hpp"
#include "jfg/runtime/modern_controls.hpp"
#include "jfg/runtime/pc_rumble.hpp"
#include "jfg/runtime/pc_rumble_output.hpp"
#include "jfg/runtime/pc_camera_visibility.hpp"
#include "jfg/runtime/support_log.hpp"
#include "jfg/runtime/support_snapshot.hpp"
#include <iterator>
#include "jfg/runtime/rt64_overlay_address.hpp"
#include "jfg/renderer/rt64_f3ddkr_address.hpp"
#include "../renderer/rt64_f3ddkr.hpp"
#include "jfg/runtime/cic_nus_6105.hpp"
#include "jfg/testkernel/sha256.hpp"
#if defined(JFG_PHASE8_LIVE_RUNTIME)
#include "jfg/evidence/g2_private_task_adapter.hpp"
#include "jfg/renderer/rt64_shell.hpp"
#include "jfg/renderer/video_settings.hpp"
#include "jfg/renderer/presentation_identity.hpp"
#include "jfg/renderer/interpolation_history.hpp"
#include "jfg/renderer/vi_presentation.hpp"
#include "jfg/runtime/audio_task_bridge.hpp"
#include "jfg/runtime/input_replay.hpp"
#include "jfg/runtime/save_device_runtime.hpp"
#endif

#include <algorithm>
#include "jfg/runtime/controller_status_query.hpp"
#include <array>
#include <bit>
#include <chrono>
#include <charconv>
#include <cfenv>
#include <cstdint>
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <Xinput.h>
#include <fcntl.h>
#include <io.h>
#if defined(JFG_PHASE8_LIVE_RUNTIME)
#include <SDL.h>
#endif
extern "C" BOOLEAN WINAPI SystemFunction036(PVOID, ULONG);
#endif

#include "jfg/boot/host_deadline_wait.hpp"

#if !defined(JFG_PHASE6_BUILD_IDENTITY)
#error "JFG_PHASE6_BUILD_IDENTITY must bind the native runner build"
#endif
#if !defined(JFG_PHASE6_SANITIZER_ID)
#error "JFG_PHASE6_SANITIZER_ID must bind the native runner instrumentation"
#endif

extern "C" { JfgOriginalTimingFast* jfg_original_timing_state = nullptr; }

namespace jfg::boot::native {
extern "C" {
#include "jfg/boot/original_timing_cache.h"
}
namespace {
constexpr std::size_t kRomSize = 32U * 1024U * 1024U;
constexpr std::size_t kRdramSize = 4U * 1024U * 1024U;
constexpr std::size_t kKseg1AliasOffset = 0x20000000U;
constexpr std::size_t kCartCachedAliasOffset = 0x10000000U;
constexpr std::size_t kCartUncachedAliasOffset = 0x30000000U;
constexpr std::size_t kGuestAddressSpan = 0x8A000000U;
constexpr std::size_t kMmioPageSize = 0x1000U;
constexpr std::size_t kWindowsAllocationGranularity = 0x10000U;
constexpr std::array<std::size_t, 12U> kMmioHostOffsets = {
    0x24040000U, 0x24100000U, 0x24300000U, 0x24400000U,
    0x24500000U, 0x24600000U, 0x24700000U, 0x24800000U,
    0x24080000U, 0x3fc00000U, 0x28000000U, 0x28010000U,
};
constexpr std::uint32_t kKseg0 = 0x80000000U;
constexpr std::uint32_t kRunlinkModuleTablePointer = 0x800FEAA0U;
constexpr std::size_t kRunlinkOverlaySlotCount = 157U;
constexpr std::uint32_t kHintTextOverlaySection = 7U;
constexpr std::uint64_t kCountTicksPerViFrame = 781250U;
constexpr std::uint64_t kCountTicksPerDispatch = 64U;
// A guest may wait for asynchronous hardware by polling an empty libultra
// queue without blocking. Bound only that repeated poll pattern so ordinary
// generated execution keeps its established cooperative timing.
constexpr std::uint32_t kEmptyReceivePollTimeslice = 2048U;
constexpr std::array<std::uint8_t, 20U> kSupportedRomSha1 = {
    0x49U, 0x3cU, 0xedU, 0x90U, 0x08U, 0xdbU, 0xe9U, 0x32U, 0xd6U, 0xe9U,
    0x11U, 0x79U, 0xb6U, 0x8eU, 0x86U, 0x30U, 0xcfU, 0x23U, 0xa0U, 0x23U,
};
struct Mapping {
  std::uint32_t vram;
  const char *name;
};
constexpr Mapping kMappings[] = {
#include JFG_PHASE6_DISPATCH_TABLE_INC
};
constexpr std::size_t kMappingCount = sizeof(kMappings) / sizeof(kMappings[0]);
consteval std::uint32_t original_os_entry(std::string_view name) {
  for (const auto& mapping : kMappings)
    if (mapping.name == name) return mapping.vram;
  throw "Unknown original OS observer";
}

constexpr bool mappings_are_strictly_sorted() noexcept {
  for (std::size_t index = 1U; index < kMappingCount; ++index) {
    if (kMappings[index - 1U].vram >= kMappings[index].vram)
      return false;
  }
  return true;
}
static_assert(mappings_are_strictly_sorted(),
              "native HLE dispatch table must be sorted by guest address");

enum class NativeEventKind : std::uint32_t {
  kDispatch = 1U,
  kThreadCreated = 2U,
  kThreadStarted = 3U,
  kQueueCreated = 4U,
  kViConfigured = 5U,
  kViFrame = 6U,
  kViInterruptRaised = 7U,
  kViMessageDelivered = 8U,
  kViMessageConsumed = 9U,
  kMmioRead = 10U,
  kMmioWrite = 11U,
  kCacheMaintenance = 12U,
  kInterruptMask = 13U,
  kPiDma = 14U,
  kOverlayPublished = 15U,
  kController = 16U,
  kThreadPaused = 17U,
};

struct NativeEvent final {
  NativeEventKind kind = NativeEventKind::kDispatch;
  std::uint32_t a = 0U;
  std::uint32_t b = 0U;
  std::uint32_t c = 0U;
};

struct EventJournal final {
  static constexpr std::size_t kCapacity = 16384U;
  std::array<NativeEvent, kCapacity> entries{};
  std::array<std::uint64_t, 18U> observed_counts{};
  std::uint64_t observed_total = 0U;
  std::size_t count = 0U;
  bool overflow = false;

  void append(const NativeEventKind kind, const std::uint32_t a = 0U,
              const std::uint32_t b = 0U,
              const std::uint32_t c = 0U) noexcept {
    const std::size_t kind_index = static_cast<std::size_t>(kind);
    if (kind_index < observed_counts.size())
      ++observed_counts[kind_index];
    ++observed_total;
#if defined(JFG_PHASE8_LIVE_RUNTIME)
    // Live execution can run indefinitely, so its fixed-capacity journal keeps
    // structural events while the aggregate counters above retain exact totals
    // for high-frequency activity. Non-live Phase 6 evidence continues to
    // retain and hash every event exactly as before.
    switch (kind) {
    case NativeEventKind::kThreadCreated:
    case NativeEventKind::kThreadStarted:
    case NativeEventKind::kQueueCreated:
    case NativeEventKind::kViConfigured:
    case NativeEventKind::kOverlayPublished:
    case NativeEventKind::kThreadPaused:
      break;
    default:
      return;
    }
#endif
    if (count == entries.size()) {
      overflow = true;
      return;
    }
    entries[count++] = NativeEvent{kind, a, b, c};
  }
};

constexpr std::array<std::uint32_t, 12U> kMmioGuestBases = {
    0xA4040000U, 0xA4100000U, 0xA4300000U, 0xA4400000U,
    0xA4500000U, 0xA4600000U, 0xA4700000U, 0xA4800000U,
    0xA4080000U, 0xbfc00000U, 0xa8000000U, 0xa8010000U,
};
constexpr std::array<std::uint32_t, 12U> kMmioLastRegister = {
    0x1CU, 0x1CU, 0x0CU, 0x34U, 0x14U, 0x30U, 0x1CU, 0x1CU, 0U, 0x7fcU, 0U, 0U,
};

struct MmioTrace final {
  // Explicit diagnostic reference-device ownership. The cooperative control
  // continues using its existing register behavior unless this is enabled.
  bool guest_execution = false;
  bool native_elapsed_time = false;
  bool native_rom_completion = false;
  std::uint64_t guest_count = 0U;
  std::uint64_t vi_next = 0U;
  std::uint64_t execution_io_revision = 0U;
  std::uint32_t vi_period = 500000U;
  std::uint32_t vi_v_sync = 0U;
  std::uint32_t vi_h_sync = 0U;
  std::uint32_t mi_pending = 0U;
  std::uint64_t vi_acknowledgements = 0U;
  jfg::boot::ReferenceSiDma si_dma;
  jfg::boot::OriginalTimingPiDma pi_dma;
  jfg::boot::ReferenceFlashBus flash_bus;
  jfg::boot::OriginalTimingAiDma ai_dma;
  std::uint64_t persisted_flash_mutations = 0;
  std::optional<std::uint64_t> sp_launch, sp_deadline, dp_deadline;
  std::span<std::uint8_t> device_rdram;
  std::span<const std::uint8_t> device_rom;
  const char* device_error = nullptr;
  std::uint32_t device_write_page = 0U, device_write_offset = 0U, device_write_value = 0U;
  std::uint32_t device_pi_offset = 0U, device_pi_value = 0U;
  std::uint32_t device_si_offset = 0U, device_si_value = 0U;
  std::uint64_t device_si_sequence = 0U, traced_si_sequence = 0U;
  std::optional<std::uint64_t> device_si_prior_deadline;
  std::uint64_t traced_pi_transfers = 0U;
  struct AiSubmission final {
    std::uint32_t address = 0U;
    std::uint32_t length = 0U;
    std::uint32_t dac_rate = 0U;
    std::uint64_t submitted_count = 0U, payload_hash = 0U;
  };
  // PCM-capture-only ownership diagnostic. Compare the buffer at AI submission
  // and at host consumption; never change guest memory, clocks or queue order.
  bool trace_audio_payloads = false;
  std::uint64_t audio_payload_hash(std::uint32_t address, std::uint32_t length) const noexcept {
    address &= 0x00ffffffU;
    if (address > device_rdram.size() || length > device_rdram.size()-address) return 0U;
    std::uint64_t hash = 14695981039346656037ULL;
    for (auto byte : device_rdram.subspan(address,length)) {
      hash ^= byte; hash *= 1099511628211ULL;
    }
    return hash;
  }
  static constexpr std::size_t kAiSubmissionCapacity = 2048U;

  EventJournal *journal = nullptr;
  std::array<void *, kMmioHostOffsets.size()> pages{};
  void *pending_page = nullptr;
  std::size_t pending_page_index = SIZE_MAX;
  std::uintptr_t pending_address = 0U;
  bool pending_write = false;
  // The handler is process-wide. Only the thread that touched the guarded
  // page is single-stepping on our behalf; a trap-flag exception on any
  // other thread (debugger, GPU driver) must be left alone.
  std::uint32_t pending_thread = 0U;
  std::uint64_t accesses = 0U;
  std::uint64_t unsupported_accesses = 0U;
  std::uint64_t ai_register_writes = 0U;
  std::uint64_t ai_buffer_submissions = 0U;
  std::uint32_t ai_dram_address = 0U;
  std::uint32_t ai_length = 0U;
  std::uint32_t ai_control = 0U;
  std::uint32_t ai_dac_rate = 0U;
  std::uint32_t ai_bit_rate = 0U;
  std::uint32_t ai_emulated_remaining_length = 0U;
  std::array<AiSubmission, kAiSubmissionCapacity> ai_submissions{};
  std::uint64_t ai_submission_write = 0U;
  std::uint64_t ai_submission_read = 0U;
  std::uint64_t ai_submission_overflows = 0U;
  bool guest_mi_mask = false;
  jfg::boot::MiInterruptMask mi_mask;
  bool guest_sp_status = false;
  jfg::boot::SpStatus sp_status;
  jfg::boot::ReferenceSpDma sp_dma;
  std::span<const std::uint8_t> sp_rdram;
  bool guest_pif_boot = false;
  jfg::boot::ReferencePifBoot pif_boot;

  void record(const std::size_t page_index, const std::uintptr_t address,
              const bool write) noexcept {
    ++accesses;
    const auto offset = static_cast<std::uint32_t>(
        address - reinterpret_cast<std::uintptr_t>(pages[page_index]));
    const bool supported = page_index == 9U ? guest_pif_boot && offset == 0x7fcU :
        (offset & 3U) == 0U && offset <= kMmioLastRegister[page_index];
    if (!supported)
      ++unsupported_accesses;
    if (journal != nullptr) {
      journal->append(write ? NativeEventKind::kMmioWrite
                            : NativeEventKind::kMmioRead,
                      kMmioGuestBases[page_index] + offset,
                      supported ? 1U : 0U, 4U);
    }
  }
};

#if defined(_WIN32)
MmioTrace *g_mmio_trace = nullptr;

LONG CALLBACK mmio_exception_handler(EXCEPTION_POINTERS *exception) {
  if (g_mmio_trace == nullptr || exception == nullptr ||
      exception->ExceptionRecord == nullptr ||
      exception->ContextRecord == nullptr)
    return EXCEPTION_CONTINUE_SEARCH;
  const DWORD code = exception->ExceptionRecord->ExceptionCode;
  if (code == EXCEPTION_SINGLE_STEP &&
      g_mmio_trace->pending_page != nullptr &&
      g_mmio_trace->pending_thread == GetCurrentThreadId()) {
    if (g_mmio_trace->pending_page_index == 9U && g_mmio_trace->pending_write) {
      std::uint32_t command = 0U;
      std::memcpy(&command, reinterpret_cast<const void*>(g_mmio_trace->pending_address), 4U);
      if (!g_mmio_trace->guest_pif_boot ||
          g_mmio_trace->pending_address - reinterpret_cast<std::uintptr_t>(g_mmio_trace->pending_page) != 0x7fcU ||
          !g_mmio_trace->pif_boot.write(command)) {
        ++g_mmio_trace->unsupported_accesses;
        return EXCEPTION_CONTINUE_SEARCH;
      }
      const auto control = g_mmio_trace->pif_boot.read();
      std::memcpy(reinterpret_cast<void*>(g_mmio_trace->pending_address), &control, 4U);
      if (g_mmio_trace->guest_execution) {
        if (!g_mmio_trace->si_dma.boot_command(g_mmio_trace->guest_count)) {
          ++g_mmio_trace->unsupported_accesses;
          return EXCEPTION_CONTINUE_SEARCH;
        }
        // Independently phased direct PIF-command-8 probe: SI assertion is
        // 2304 Count ticks after retirement, not immediate acknowledgement.
      }
    }
    if (g_mmio_trace->guest_sp_status && g_mmio_trace->pending_write &&
        (g_mmio_trace->pending_page_index == 0U ||
         g_mmio_trace->pending_page_index == 8U)) {
      std::uint32_t command = 0U;
      std::memcpy(&command, reinterpret_cast<const void*>(g_mmio_trace->pending_address), 4U);
      const auto offset = static_cast<std::uint32_t>(g_mmio_trace->pending_address -
          reinterpret_cast<std::uintptr_t>(g_mmio_trace->pending_page));
      const bool status_register = g_mmio_trace->pending_page_index == 0U && offset == 0x10U;
      const bool stopped = (g_mmio_trace->sp_status.read() & 0x1dU) == 1U;
      const bool valid = status_register ? g_mmio_trace->sp_status.command(command) :
          g_mmio_trace->pending_page_index == 8U ?
            (stopped && offset == 0U && g_mmio_trace->sp_dma.write_pc(command)) :
            (stopped && g_mmio_trace->sp_dma.write(offset, command, g_mmio_trace->sp_rdram));
      if (!valid) {
        ++g_mmio_trace->unsupported_accesses;
        return EXCEPTION_CONTINUE_SEARCH;
      }
      if (status_register) {
        const auto status = g_mmio_trace->sp_status.read();
        if (g_mmio_trace->guest_execution) {
          g_mmio_trace->mi_pending = (g_mmio_trace->mi_pending & ~1U) |
              (g_mmio_trace->sp_status.interrupt() ? 1U : 0U);
          if (stopped && (status & 1U) == 0U) {
            if (g_mmio_trace->sp_launch || g_mmio_trace->sp_deadline)
              g_mmio_trace->device_error = "overlapping-sp-launch";
            else g_mmio_trace->sp_launch = g_mmio_trace->guest_count;
          }
        }
        std::memcpy(reinterpret_cast<void*>(g_mmio_trace->pending_address), &status, 4U);
      }
    }
    if (g_mmio_trace->guest_mi_mask && g_mmio_trace->pending_write &&
        g_mmio_trace->pending_page_index == 2U &&
        g_mmio_trace->pending_address ==
            reinterpret_cast<std::uintptr_t>(g_mmio_trace->pending_page) + 0x0cU) {
      std::uint32_t command = 0U;
      std::memcpy(&command, reinterpret_cast<const void*>(g_mmio_trace->pending_address), 4U);
      if (!g_mmio_trace->mi_mask.command(command)) {
        ++g_mmio_trace->unsupported_accesses;
        return EXCEPTION_CONTINUE_SEARCH;
      }
      const auto mask = g_mmio_trace->mi_mask.read();
      std::memcpy(reinterpret_cast<void*>(g_mmio_trace->pending_address), &mask, 4U);
    }
    if (g_mmio_trace->guest_execution && g_mmio_trace->pending_write) {
      ++g_mmio_trace->execution_io_revision;
      const auto offset = static_cast<std::uint32_t>(g_mmio_trace->pending_address -
          reinterpret_cast<std::uintptr_t>(g_mmio_trace->pending_page));
      std::uint32_t value = 0U;
      std::memcpy(&value, reinterpret_cast<const void*>(g_mmio_trace->pending_address), 4U);
      g_mmio_trace->device_write_page = static_cast<std::uint32_t>(g_mmio_trace->pending_page_index);
      g_mmio_trace->device_write_offset = offset;
      g_mmio_trace->device_write_value = value;
      if (g_mmio_trace->pending_page_index == 2U && offset == 0U && (value & 0x800U))
        g_mmio_trace->mi_pending &= ~32U;
      if (g_mmio_trace->pending_page_index == 3U) {
        if (offset == 0x10U) {
          if (g_mmio_trace->mi_pending & 8U) ++g_mmio_trace->vi_acknowledgements;
          g_mmio_trace->mi_pending &= ~8U;
        }
        if (offset == 0x18U) g_mmio_trace->vi_v_sync = value;
        if (offset == 0x1cU) g_mmio_trace->vi_h_sync = value;
      }
      if (g_mmio_trace->pending_page_index == 7U) {
        g_mmio_trace->device_si_offset = offset;
        g_mmio_trace->device_si_value = value;
        g_mmio_trace->device_si_prior_deadline = g_mmio_trace->si_dma.deadline();
        ++g_mmio_trace->device_si_sequence;
        if (!g_mmio_trace->si_dma.write(offset, value, g_mmio_trace->guest_count,
                g_mmio_trace->device_rdram))
          g_mmio_trace->device_error = "unsupported-si-transaction";
        g_mmio_trace->mi_pending = (g_mmio_trace->mi_pending & ~2U) |
            (g_mmio_trace->si_dma.interrupt() ? 2U : 0U);
      }
      if (g_mmio_trace->pending_page_index == 4U) {
        if (!g_mmio_trace->ai_dma.write(offset, value, g_mmio_trace->guest_count,
                g_mmio_trace->device_rdram.size(), g_mmio_trace->vi_period))
          g_mmio_trace->device_error = "unsupported-ai-transaction";
        g_mmio_trace->mi_pending = (g_mmio_trace->mi_pending & ~4U) |
            (g_mmio_trace->ai_dma.interrupt() ? 4U : 0U);
      }
      if (g_mmio_trace->pending_page_index == 5U) {

        g_mmio_trace->device_pi_offset = offset;
        g_mmio_trace->device_pi_value = value;
        if (!g_mmio_trace->pi_dma.write(offset, value, g_mmio_trace->guest_count,
                g_mmio_trace->device_rdram, g_mmio_trace->device_rom,
                [](void* device, bool to_ram, std::uint32_t cart, std::uint32_t dram,
                   std::uint32_t length, std::span<std::uint8_t> ram) {
                  return static_cast<jfg::boot::ReferenceFlashBus*>(device)->transfer(to_ram, cart, dram, length, ram);
                }, &g_mmio_trace->flash_bus,
                g_mmio_trace->native_rom_completion && g_mmio_trace->native_elapsed_time))
          g_mmio_trace->device_error = "unsupported-pi-transaction";
        g_mmio_trace->mi_pending = (g_mmio_trace->mi_pending & ~16U) |
            (g_mmio_trace->pi_dma.interrupt() ? 16U : 0U);
      }
      if (g_mmio_trace->pending_page_index >= 10U) {
        const bool valid = offset == 0U && (g_mmio_trace->pending_page_index == 10U ?
            g_mmio_trace->flash_bus.write_status(value) :
            g_mmio_trace->pending_page_index == 11U &&
              g_mmio_trace->flash_bus.command(value, g_mmio_trace->device_rdram));
        if (!valid) g_mmio_trace->device_error = "unsupported-flash-command";
      }
    }
    if (g_mmio_trace->pending_write &&
        g_mmio_trace->pending_page_index == 4U) {
      const auto page = reinterpret_cast<std::uintptr_t>(
          g_mmio_trace->pending_page);
      const auto offset = static_cast<std::uint32_t>(
          g_mmio_trace->pending_address - page);
      std::uint32_t value = 0U;
      std::memcpy(&value,
                  reinterpret_cast<const void *>(
                      g_mmio_trace->pending_address),
                  sizeof(value));
      ++g_mmio_trace->ai_register_writes;
      switch (offset) {
      case 0x00U:
        g_mmio_trace->ai_dram_address = value;
        break;
      case 0x04U:
        g_mmio_trace->ai_length = value;
        ++g_mmio_trace->ai_buffer_submissions;
        if (value != 0U) {
          if (g_mmio_trace->ai_submission_write -
                  g_mmio_trace->ai_submission_read >=
              MmioTrace::kAiSubmissionCapacity) {
            ++g_mmio_trace->ai_submission_overflows;
          } else {
            const std::size_t slot = static_cast<std::size_t>(
                g_mmio_trace->ai_submission_write %
                MmioTrace::kAiSubmissionCapacity);
            g_mmio_trace->ai_submissions[slot] = {
                g_mmio_trace->ai_dram_address, value,
                g_mmio_trace->ai_dac_rate};
            if (g_mmio_trace->trace_audio_payloads) {
              auto &submission = g_mmio_trace->ai_submissions[slot];
              submission.submitted_count = g_mmio_trace->guest_count;
              submission.payload_hash = g_mmio_trace->audio_payload_hash(submission.address,submission.length);
            }
            ++g_mmio_trace->ai_submission_write;
          }
        }
        break;
      case 0x08U:
        g_mmio_trace->ai_control = value;
        break;
      case 0x10U:
        g_mmio_trace->ai_dac_rate = value;
        break;
      case 0x14U:
        g_mmio_trace->ai_bit_rate = value;
        break;
      default:
        break;
      }
    }
    DWORD previous = 0U;
    (void)VirtualProtect(g_mmio_trace->pending_page, kMmioPageSize,
                         PAGE_READWRITE | PAGE_GUARD, &previous);
    g_mmio_trace->pending_page = nullptr;
    g_mmio_trace->pending_page_index = SIZE_MAX;
    g_mmio_trace->pending_address = 0U;
    g_mmio_trace->pending_write = false;
    g_mmio_trace->pending_thread = 0U;
    exception->ContextRecord->EFlags &= ~DWORD{0x100U};
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  if (code != STATUS_GUARD_PAGE_VIOLATION ||
      exception->ExceptionRecord->NumberParameters < 2U)
    return EXCEPTION_CONTINUE_SEARCH;
  const auto address = static_cast<std::uintptr_t>(
      exception->ExceptionRecord->ExceptionInformation[1]);
  for (std::size_t index = 0U; index < g_mmio_trace->pages.size(); ++index) {
    const auto page = reinterpret_cast<std::uintptr_t>(
        g_mmio_trace->pages[index]);
    if (address < page || address >= page + kMmioPageSize)
      continue;
    const bool write =
        exception->ExceptionRecord->ExceptionInformation[0] == 1U;
    const auto offset = static_cast<std::uint32_t>(address - page);
    if (g_mmio_trace->guest_execution && !write && index >= 10U) {
      if (index != 10U || offset != 0U) g_mmio_trace->device_error = "unsupported-flash-read";
      const auto value = g_mmio_trace->flash_bus.status();
      std::memcpy(reinterpret_cast<void*>(address), &value, 4U);
    }
    if (g_mmio_trace->guest_execution && !write && index == 5U) {
      const auto value = g_mmio_trace->pi_dma.read(offset);
      if (!value) g_mmio_trace->device_error = "unsupported-pi-register";
      const auto result = value.value_or(0U);
      std::memcpy(reinterpret_cast<void*>(address), &result, 4U);
    }
    if (g_mmio_trace->guest_execution && !write && index == 4U) {
      const auto value = g_mmio_trace->ai_dma.read(offset, g_mmio_trace->guest_count);
      if (!value) g_mmio_trace->device_error = "unsupported-ai-register";
      const auto result = value.value_or(0U);
      std::memcpy(reinterpret_cast<void*>(address), &result, 4U);
    }
    if (g_mmio_trace->guest_execution && !write && index == 7U) {
      const auto value = g_mmio_trace->si_dma.read(offset);
      if (!value) g_mmio_trace->device_error = "unsupported-si-register";
      const auto result = value.value_or(0U);
      std::memcpy(reinterpret_cast<void*>(address), &result, 4U);
    }
    if (g_mmio_trace->guest_execution && !write && index == 2U && offset == 8U)
      std::memcpy(reinterpret_cast<void*>(address), &g_mmio_trace->mi_pending, 4U);
    if (g_mmio_trace->guest_execution && !write && index == 3U && offset == 0x10U) {
      const auto origin = g_mmio_trace->vi_next - g_mmio_trace->vi_period;
      const auto elapsed = g_mmio_trace->native_elapsed_time
          ? jfg::boot::native_periodic_phase(g_mmio_trace->guest_count,
                g_mmio_trace->vi_next, g_mmio_trace->vi_period)
          : g_mmio_trace->guest_count-origin;
      const auto line = g_mmio_trace->vi_period == 783520U ?
          static_cast<std::uint32_t>((elapsed*263U)/g_mmio_trace->vi_period)*2U :
          static_cast<std::uint32_t>(elapsed/1500U)&~1U;
      std::memcpy(reinterpret_cast<void*>(address), &line, 4U);
    }
    if (index == 9U && (!g_mmio_trace->guest_pif_boot || offset != 0x7fcU)) {
      ++g_mmio_trace->unsupported_accesses;
      return EXCEPTION_CONTINUE_SEARCH;
    }
    if (!write && index == 9U) {
      const auto control = g_mmio_trace->pif_boot.read();
      std::memcpy(reinterpret_cast<void*>(address), &control, 4U);
    }
    if (!write && g_mmio_trace->guest_sp_status && (index == 0U || index == 8U)) {
      const auto value = index == 8U ? (offset == 0U ?
          std::optional<std::uint32_t>{g_mmio_trace->sp_dma.pc()} : std::nullopt) :
          offset == 0x10U ? std::optional<std::uint32_t>{g_mmio_trace->sp_status.read()} :
          g_mmio_trace->sp_dma.read(offset);
      if (!value.has_value()) {
        ++g_mmio_trace->unsupported_accesses;
        return EXCEPTION_CONTINUE_SEARCH;
      }
      std::memcpy(reinterpret_cast<void*>(address), &*value, 4U);
    }
    if (!write && g_mmio_trace->guest_mi_mask && index == 2U && offset == 0x0cU) {
      const auto mask = g_mmio_trace->mi_mask.read();
      std::memcpy(reinterpret_cast<void*>(address), &mask, 4U);
    }
    if (!g_mmio_trace->guest_execution && !write && index == 4U && offset == 0x04U) {
      std::memcpy(reinterpret_cast<void *>(address),
                  &g_mmio_trace->ai_emulated_remaining_length,
                  sizeof(g_mmio_trace->ai_emulated_remaining_length));
    }
    g_mmio_trace->record(index, address, write);
    g_mmio_trace->pending_page = g_mmio_trace->pages[index];
    g_mmio_trace->pending_page_index = index;
    g_mmio_trace->pending_address = address;
    g_mmio_trace->pending_write = write;
    g_mmio_trace->pending_thread = GetCurrentThreadId();
    exception->ContextRecord->EFlags |= DWORD{0x100U};
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
class HostAudioDevice final {
public:
  HostAudioDevice() = default;
  ~HostAudioDevice() { close(); }
  HostAudioDevice(const HostAudioDevice &) = delete;
  HostAudioDevice &operator=(const HostAudioDevice &) = delete;

  void set_gameplay_trace(GameplayTrace *trace) noexcept { gameplay_trace_ = trace; }
  void frontend_pause(bool paused) noexcept {
    if (device_ != 0U && device_playing_) SDL_PauseAudioDevice(device_, paused ? 1 : 0);
  }
  void set_vi_counter(const std::uint64_t *vi) noexcept { capture_vi_ = vi; }
  void renderer_ready() noexcept { renderer_ready_ = true; update_playback_state(); }

  bool configure_capture(const std::string &path) {
    capture_pcm_.open(path, std::ios::binary | std::ios::trunc);
    capture_events_.open(path + ".events.csv",
                         std::ios::binary | std::ios::trunc);
    if (!capture_pcm_ || !capture_events_)
      return false;
    capture_started_ = std::chrono::steady_clock::now();
    capture_events_ << "wall_us,event,queued_bytes,total_queued_bytes,"
                       "consumed_bytes,vi,steady_us\n";
    return true;
  }

  bool queue(const std::uint8_t *const rdram, std::uint32_t address,
             const std::uint32_t length, const std::uint32_t dac_rate) {
    constexpr std::uint32_t kNtscVideoClock = 48681812U;
    address &= 0x00FFFFFFU;
    if (rdram == nullptr || length == 0U || (length & 3U) != 0U ||
        address > kRdramSize || length > kRdramSize - address ||
        dac_rate == UINT32_MAX)
      return false;
    const std::uint32_t frequency =
        kNtscVideoClock / (dac_rate + 1U);
    if (frequency < 8000U || frequency > 96000U ||
        (!initialized_ && !initialize(frequency)) || frequency_ != frequency)
      return false;

    refresh_volume(frequency);
    update_metrics();
    converted_.resize(length);
    for (std::uint32_t offset = 0U; offset < length; offset += 2U) {
      const std::uint16_t sample = static_cast<std::uint16_t>(
          (static_cast<std::uint16_t>(
               rdram[(address + offset) ^ 3U]) << 8U) |
          rdram[(address + offset + 1U) ^ 3U]);
      converted_[offset] = static_cast<std::uint8_t>(sample);
      converted_[offset + 1U] = static_cast<std::uint8_t>(sample >> 8U);
    }
    if (capture_pcm_.is_open()) {
      capture_pcm_.write(
          reinterpret_cast<const char *>(converted_.data()),
          static_cast<std::streamsize>(converted_.size()));
      if (!capture_pcm_)
        return false;
    }
    master_volume_.apply(converted_);
    if (device_ != 0U &&
        SDL_QueueAudio(device_, converted_.data(), length) != 0)
      return false;
    ++submitted_buffers_;
    queued_bytes_ += length;
    emulated_dma_lengths_.push_back(length);
    update_metrics();
    capture_event("queue");
    update_playback_state();
    return true;
  }

  // Shader startup may accumulate more than the configured priming cushion.
  // Establish the wall clock only after that excess has played, so startup
  // work cannot permanently choose a larger audio/video phase offset.
  void settle_startup_queue() {
    if (!device_playing_ || device_ == 0U) return;
    update_metrics();
    capture_event("anchor-drain-begin");
    while (device_playing_ && current_queued_bytes_ > prebuffer_bytes_) {
      SDL_Delay(1U);
      update_metrics();
      update_playback_state();
    }
    capture_event("anchor-drain-end");
  }

  void service(const bool advance_emulated_clock, const bool throttle) {
    if (!initialized_)
      return;
    if (advance_emulated_clock)
      advance_emulated_dma();
    update_metrics();
    update_playback_state();
    while (throttle && device_playing_ &&
           current_queued_bytes_ > pacing_bytes_) {
      SDL_Delay(1U);
      update_metrics();
      update_playback_state();
    }
    if (!advance_emulated_clock) {
      if (capture_pcm_.is_open())
        capture_pcm_.flush();
      if (capture_events_.is_open())
        capture_events_.flush();
    }
  }

  [[nodiscard]] bool capturing_pcm() const noexcept { return capture_pcm_.is_open(); }
  void finish_capture() noexcept { capture_pcm_.finish(); capture_events_.finish(); }

  void close() noexcept {
    if (device_ != 0U) {
      update_metrics();
      SDL_PauseAudioDevice(device_, 1);
      SDL_CloseAudioDevice(device_);
      device_ = 0U;
    }
    initialized_ = false;
  }

  [[nodiscard]] bool initialized() const noexcept { return initialized_; }
  [[nodiscard]] bool started() const noexcept { return started_; }
  [[nodiscard]] std::uint32_t frequency() const noexcept { return frequency_; }
  [[nodiscard]] std::uint64_t submitted_buffers() const noexcept {
    return submitted_buffers_;
  }
  [[nodiscard]] std::uint64_t queued_bytes() const noexcept {
    return queued_bytes_;
  }
  [[nodiscard]] std::uint64_t consumed_bytes() const noexcept {
    return consumed_bytes_;
  }
  [[nodiscard]] std::uint64_t underruns() const noexcept {
    return underruns_;
  }
  [[nodiscard]] std::uint64_t overruns() const noexcept { return overruns_; }
  [[nodiscard]] std::uint32_t current_queued_bytes() const noexcept {
    return current_queued_bytes_;
  }
  [[nodiscard]] std::uint64_t consumed_milliseconds() const noexcept {
    return frequency_ == 0U
               ? 0U
               : consumed_bytes_ * 1000U /
                     (static_cast<std::uint64_t>(frequency_) * 4U);
  }
  [[nodiscard]] std::uint32_t current_buffer_remaining() const noexcept {
    return emulated_dma_lengths_.empty() ? 0U
                                         : emulated_dma_lengths_.front();
  }

private:
  void refresh_volume(const std::uint32_t frequency) {
    if (!volume_path_checked_) {
      volume_path_checked_ = true;
      wchar_t *path = nullptr;
      std::size_t length = 0;
      if (_wdupenv_s(&path, &length, L"JFG_MASTER_VOLUME_CONFIG") == 0 && path) {
        volume_path_ = std::filesystem::path(path);
        std::free(path);
        volume_file_.start(volume_path_);
      }
    }
    if (volume_path_.empty()) return;
    const auto now = std::chrono::steady_clock::now();
    if (now < volume_next_read_) return;
    volume_next_read_ = now + std::chrono::milliseconds(100);
    // Runs on the existing producer thread, never an SDL audio callback.
    // Missing/partial replacements keep the last accepted preference.
    const auto text = volume_file_.latest();
    if (!text) return;
    if (const auto settings = jfg::audio::parse_volume(*text)) {
      master_volume_.configure(*settings, frequency, !volume_received_);
      volume_received_ = true;
    }
  }

  bool initialize(const std::uint32_t frequency) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
      return false;
    SDL_AudioSpec desired{};
    desired.freq = static_cast<int>(frequency);
    desired.format = AUDIO_S16SYS;
    desired.channels = 2U;
    desired.samples = 256U;
    SDL_AudioSpec obtained{};
    device_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (device_ == 0U || obtained.freq != desired.freq ||
        obtained.format != desired.format ||
        obtained.channels != desired.channels) {
      if (device_ != 0U)
        SDL_CloseAudioDevice(device_);
      device_ = 0U;
      // A missing or mismatched host device is a correctness failure by
      // default so a manual run cannot silently lose audio. Headless soak
      // hosts opt in to a null sink whose consumption follows the emulated
      // AI DMA clock, keeping the simulation deterministic without SDL.
      if (!null_sink_requested())
        return false;
      null_sink_ = true;
    }
    frequency_ = frequency;
    // Give the first renderer/shader work a generous cushion, then keep normal
    // host latency bounded independently of the emulated AI DMA clock.
    prebuffer_bytes_ = frequency_ * 4U / 10U;
    resume_bytes_ = frequency_ * 4U / 10U;
    pacing_bytes_ = frequency_ * 4U / 5U;
    overrun_bytes_ = frequency_ * 4U * 2U;
    initialized_ = true;
    return true;
  }

  [[nodiscard]] static bool null_sink_requested() noexcept {
    char *value = nullptr;
    std::size_t length = 0U;
    if (_dupenv_s(&value, &length, "JFG_PHASE9_NULL_AUDIO") != 0 ||
        value == nullptr)
      return false;
    const bool requested = value[0] == '1' && value[1] == '\0';
    std::free(value);
    return requested;
  }

  void update_metrics() noexcept {
    if (device_ == 0U) {
      if (!null_sink_)
        return;
      std::uint64_t remaining = 0U;
      for (const std::uint32_t length : emulated_dma_lengths_)
        remaining += length;
      current_queued_bytes_ = static_cast<std::uint32_t>(
          (std::min)(remaining, std::uint64_t{UINT32_MAX}));
      consumed_bytes_ = queued_bytes_ >= current_queued_bytes_
                            ? queued_bytes_ - current_queued_bytes_
                            : 0U;
      return;
    }
    current_queued_bytes_ = SDL_GetQueuedAudioSize(device_);
    consumed_bytes_ = queued_bytes_ >= current_queued_bytes_
                          ? queued_bytes_ - current_queued_bytes_
                          : 0U;
    if (device_playing_ && current_queued_bytes_ == 0U) {
      if (!empty_episode_) {
        ++underruns_;
        empty_episode_ = true;
        capture_event("underrun");
      }
    } else if (current_queued_bytes_ != 0U) {
      empty_episode_ = false;
    }
    if (current_queued_bytes_ > overrun_bytes_) {
      if (!overrun_episode_) {
        ++overruns_;
        overrun_episode_ = true;
      }
    } else {
      overrun_episode_ = false;
    }
  }

  void update_playback_state() noexcept {
    if (device_ == 0U) {
      if (null_sink_ && !started_ &&
          current_queued_bytes_ >= prebuffer_bytes_)
        started_ = true;
      return;
    }
    if (!started_ && !renderer_ready_) return;
    switch (jfg::audio::playback_buffer_action(started_, device_playing_,
                current_queued_bytes_, prebuffer_bytes_, resume_bytes_)) {
    case jfg::audio::PlaybackBufferAction::start:
      SDL_PauseAudioDevice(device_, 0);
      started_ = true;
      device_playing_ = true;
      empty_episode_ = false;
      capture_event("start");
      break;
    case jfg::audio::PlaybackBufferAction::pause_empty:
      SDL_PauseAudioDevice(device_, 1);
      device_playing_ = false;
      capture_event("pause-empty");
      break;
    case jfg::audio::PlaybackBufferAction::resume:
      SDL_PauseAudioDevice(device_, 0);
      device_playing_ = true;
      empty_episode_ = false;
      capture_event("resume");
      break;
    case jfg::audio::PlaybackBufferAction::none:
      break;
    }
  }

  void capture_event(const char *event) noexcept {
    if (gameplay_trace_ != nullptr && *gameplay_trace_) {
      std::array<char, 48> name{};
      (void)std::snprintf(name.data(), name.size(), "audio-%s", event);
      gameplay_trace_->event(name.data(), {current_queued_bytes_, queued_bytes_,
          consumed_bytes_, frequency_, device_playing_ ? 1U : 0U});
    }
    if (!capture_events_.is_open())
      return;
    const auto wall_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - capture_started_).count();
    capture_events_ << wall_us << ',' << event << ','
                    << current_queued_bytes_ << ',' << queued_bytes_ << ','
                    << consumed_bytes_ << ',' << (capture_vi_ ? *capture_vi_ : 0U) << ','
                    << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() << '\n';
  }

  void advance_emulated_dma() noexcept {
    constexpr std::uint32_t kViRetracesPerSecond = 60U;
    emulated_byte_accumulator_ +=
        static_cast<std::uint64_t>(frequency_) * 4U;
    std::uint64_t bytes =
        emulated_byte_accumulator_ / kViRetracesPerSecond;
    emulated_byte_accumulator_ %= kViRetracesPerSecond;
    while (bytes != 0U && !emulated_dma_lengths_.empty()) {
      std::uint32_t &remaining = emulated_dma_lengths_.front();
      if (bytes < remaining) {
        remaining -= static_cast<std::uint32_t>(bytes);
        bytes = 0U;
      } else {
        bytes -= remaining;
        emulated_dma_lengths_.pop_front();
      }
    }
  }

  GameplayTrace *gameplay_trace_ = nullptr;
  const std::uint64_t *capture_vi_ = nullptr;
  SDL_AudioDeviceID device_ = 0U;
  jfg::audio::MasterVolume master_volume_;
  std::filesystem::path volume_path_;
  jfg::AsyncVolumeFile volume_file_;
  std::chrono::steady_clock::time_point volume_next_read_{};
  bool volume_path_checked_ = false, volume_received_ = false;
  std::vector<std::uint8_t> converted_;
  jfg::AsyncDiagnosticFile capture_pcm_;
  jfg::AsyncDiagnosticFile capture_events_;
  std::chrono::steady_clock::time_point capture_started_{};
  std::deque<std::uint32_t> emulated_dma_lengths_;
  std::uint64_t emulated_byte_accumulator_ = 0U;
  std::uint32_t frequency_ = 0U;
  std::uint32_t prebuffer_bytes_ = 0U;
  std::uint32_t resume_bytes_ = 0U;
  std::uint32_t pacing_bytes_ = 0U;
  std::uint32_t overrun_bytes_ = 0U;
  std::uint32_t current_queued_bytes_ = 0U;
  bool renderer_ready_ = false;
  std::uint64_t submitted_buffers_ = 0U;
  std::uint64_t queued_bytes_ = 0U;
  std::uint64_t consumed_bytes_ = 0U;
  std::uint64_t underruns_ = 0U;
  std::uint64_t overruns_ = 0U;
  bool initialized_ = false;
  bool started_ = false;
  bool device_playing_ = false;
  bool empty_episode_ = false;
  bool overrun_episode_ = false;
  bool null_sink_ = false;
};
#endif

class GuestBacking final {
public:
  GuestBacking() = default;
  ~GuestBacking() {
#if defined(_WIN32)
    if (exception_handler_ != nullptr)
      (void)RemoveVectoredExceptionHandler(exception_handler_);
    if (g_mmio_trace == &mmio_trace_)
      g_mmio_trace = nullptr;
    if (alias_ != nullptr)
      UnmapViewOfFile(alias_);
    if (cart_cached_ != nullptr)
      UnmapViewOfFile(cart_cached_);
    if (cart_uncached_ != nullptr)
      UnmapViewOfFile(cart_uncached_);
    if (base_ != nullptr)
      UnmapViewOfFile(base_);
    for (void *page : mmio_)
      if (page != nullptr)
        (void)VirtualFree(page, 0U, MEM_RELEASE);
    for (void *region : overlays_)
      if (region != nullptr)
        (void)VirtualFree(region, 0U, MEM_RELEASE);
    release_gaps();
    if (mapping_ != nullptr)
      CloseHandle(mapping_);
    if (rom_mapping_ != nullptr)
      CloseHandle(rom_mapping_);
#endif
  }
  GuestBacking(const GuestBacking &) = delete;
  GuestBacking &operator=(const GuestBacking &) = delete;

  bool create() {
#if defined(_WIN32)
    mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr,
                                  PAGE_READWRITE, 0U,
                                  static_cast<DWORD>(kRdramSize), nullptr);
    rom_mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr,
                                      PAGE_READWRITE, 0U,
                                      static_cast<DWORD>(kRomSize), nullptr);
    if (mapping_ == nullptr || rom_mapping_ == nullptr)
      return false;
    for (unsigned attempt = 0U; attempt < 16U; ++attempt) {
      void *candidate = VirtualAlloc(
          nullptr, kGuestAddressSpan, MEM_RESERVE, PAGE_NOACCESS);
      if (candidate == nullptr)
        return false;
      (void)VirtualFree(candidate, 0U, MEM_RELEASE);
      base_ = static_cast<std::uint8_t *>(MapViewOfFileEx(
          mapping_, FILE_MAP_ALL_ACCESS, 0U, 0U, kRdramSize, candidate));
      if (base_ == nullptr)
        continue;
      void *alias_address = reinterpret_cast<void *>(
          reinterpret_cast<std::uintptr_t>(base_) + kKseg1AliasOffset);
      alias_ = static_cast<std::uint8_t *>(MapViewOfFileEx(
          mapping_, FILE_MAP_ALL_ACCESS, 0U, 0U, kRdramSize, alias_address));
      void *const cart_cached_address = reinterpret_cast<void *>(
          reinterpret_cast<std::uintptr_t>(base_) + kCartCachedAliasOffset);
      cart_cached_ = static_cast<std::uint8_t *>(MapViewOfFileEx(
          rom_mapping_, FILE_MAP_ALL_ACCESS, 0U, 0U, kRomSize,
          cart_cached_address));
      void *const cart_uncached_address = reinterpret_cast<void *>(
          reinterpret_cast<std::uintptr_t>(base_) + kCartUncachedAliasOffset);
      cart_uncached_ = static_cast<std::uint8_t *>(MapViewOfFileEx(
          rom_mapping_, FILE_MAP_READ, 0U, 0U, kRomSize,
          cart_uncached_address));
      bool mmio_ok = alias_ != nullptr && cart_cached_ == cart_cached_address &&
                     cart_uncached_ == cart_uncached_address;
      for (std::size_t index = 0U; mmio_ok && index < mmio_.size(); ++index) {
        void *const page_address = reinterpret_cast<void *>(
            reinterpret_cast<std::uintptr_t>(base_) + kMmioHostOffsets[index]);
        mmio_[index] = VirtualAlloc(page_address, kMmioPageSize,
                                    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        mmio_ok = mmio_[index] == page_address;
      }
      if (mmio_ok)
        {
          overlays_.assign(jfg_generated_section_count(), nullptr);
          bool overlays_ok = true;
          for (std::uint32_t section = 0U;
               overlays_ok && section < overlays_.size(); ++section) {
            JfgGeneratedSectionMetadata metadata{};
            if (jfg_generated_section_metadata(section, &metadata) == 0) {
              overlays_ok = false;
              break;
            }
            if (metadata.is_overlay != 1U)
              continue;
            const std::uint64_t extent = std::uint64_t{metadata.text_size} +
                                         metadata.data_size +
                                         metadata.bss_size;
            const std::uint64_t allocation =
                (extent + kWindowsAllocationGranularity - 1U) &
                ~(std::uint64_t{kWindowsAllocationGranularity} - 1U);
            const std::uint64_t host_offset =
                std::uint64_t{0x80000000U} + metadata.linked_vram;
            if (extent == 0U || allocation > SIZE_MAX ||
                host_offset > kGuestAddressSpan ||
                allocation > kGuestAddressSpan - host_offset) {
              overlays_ok = false;
              break;
            }
            void *const overlay_address = reinterpret_cast<void *>(
                reinterpret_cast<std::uintptr_t>(base_) + host_offset);
            overlays_[section] = VirtualAlloc(
                overlay_address, static_cast<std::size_t>(allocation),
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            overlays_ok = overlays_[section] == overlay_address;
          }
          // The span was released so the views could be placed; anything
          // the host allocated into a gap since then would be reachable by
          // a wild guest pointer instead of faulting. Re-reserve every
          // remaining free gap as no-access memory, and treat a gap that is
          // no longer free as a failed attempt.
          if (overlays_ok && reserve_gaps())
            return true;
        }
      release_gaps();
      for (void *&region : overlays_) {
        if (region != nullptr)
          (void)VirtualFree(region, 0U, MEM_RELEASE);
        region = nullptr;
      }
      overlays_.clear();
      for (void *&page : mmio_) {
        if (page != nullptr)
          (void)VirtualFree(page, 0U, MEM_RELEASE);
        page = nullptr;
      }
      if (alias_ != nullptr)
        UnmapViewOfFile(alias_);
      alias_ = nullptr;
      if (cart_cached_ != nullptr)
        UnmapViewOfFile(cart_cached_);
      cart_cached_ = nullptr;
      if (cart_uncached_ != nullptr)
        UnmapViewOfFile(cart_uncached_);
      cart_uncached_ = nullptr;
      UnmapViewOfFile(base_);
      base_ = nullptr;
    }
    return false;
#else
    bytes_.resize(kRdramSize);
    return true;
#endif
  }
  std::span<std::uint8_t> bytes() noexcept {
#if defined(_WIN32)
    return {base_, base_ == nullptr ? 0U : kRdramSize};
#else
    return bytes_;
#endif
  }

#if defined(_WIN32)
  [[nodiscard]] std::size_t reserved_gap_count() const noexcept {
    return gaps_.size();
  }

private:
  bool reserve_gaps() {
    const std::uintptr_t begin = reinterpret_cast<std::uintptr_t>(base_);
    const std::uintptr_t end = begin + kGuestAddressSpan;
    std::uintptr_t cursor = begin;
    while (cursor < end) {
      MEMORY_BASIC_INFORMATION info{};
      if (VirtualQuery(reinterpret_cast<void *>(cursor), &info,
                       sizeof(info)) == 0U)
        return false;
      const std::uintptr_t region_begin =
          reinterpret_cast<std::uintptr_t>(info.BaseAddress);
      const std::uintptr_t region_end = region_begin + info.RegionSize;
      if (info.State == MEM_FREE) {
        // Reservations are placed at allocation granularity. A free tail
        // below the next 64 KiB boundary cannot be claimed by any other
        // allocation either, so it is safe to leave unreserved.
        const std::uintptr_t gap_begin =
            ((std::max)(region_begin, cursor) +
             kWindowsAllocationGranularity - 1U) &
            ~(std::uintptr_t{kWindowsAllocationGranularity} - 1U);
        const std::uintptr_t gap_end = (std::min)(region_end, end);
        if (gap_begin < gap_end) {
          void *const reserved = VirtualAlloc(
              reinterpret_cast<void *>(gap_begin), gap_end - gap_begin,
              MEM_RESERVE, PAGE_NOACCESS);
          if (reserved != reinterpret_cast<void *>(gap_begin)) {
            if (reserved != nullptr)
              (void)VirtualFree(reserved, 0U, MEM_RELEASE);
            return false;
          }
          gaps_.push_back(reserved);
        }
      }
      if (region_end <= cursor)
        return false;
      cursor = region_end;
    }
    return true;
  }

  void release_gaps() noexcept {
    for (void *gap : gaps_)
      if (gap != nullptr)
        (void)VirtualFree(gap, 0U, MEM_RELEASE);
    gaps_.clear();
  }

public:
#endif

  bool arm_mmio_monitor(EventJournal &journal) noexcept {
#if defined(_WIN32)
    mmio_trace_.journal = &journal;
    mmio_trace_.pages = mmio_;
    g_mmio_trace = &mmio_trace_;
    exception_handler_ = AddVectoredExceptionHandler(1U,
                                                      mmio_exception_handler);
    if (exception_handler_ == nullptr) {
      g_mmio_trace = nullptr;
      return false;
    }
    for (void *page : mmio_) {
      DWORD previous = 0U;
      if (page == nullptr ||
          VirtualProtect(page, kMmioPageSize, PAGE_READWRITE | PAGE_GUARD,
                         &previous) == 0)
        return false;
    }
    return true;
#else
    (void)journal;
    return false;
#endif
  }

  [[nodiscard]] const MmioTrace &mmio_trace() const noexcept {
    return mmio_trace_;
  }
  [[nodiscard]] MmioTrace &mmio_trace() noexcept { return mmio_trace_; }

  bool load_overlay_images(
      const std::span<const std::uint8_t> rom) noexcept {
#if defined(_WIN32)
    if (rom.size() != kRomSize || cart_cached_ == nullptr ||
        cart_uncached_ == nullptr ||
        overlays_.size() != jfg_generated_section_count())
      return false;
    for (std::size_t index = 0U; index < rom.size(); ++index)
      cart_cached_[index ^ 3U] = rom[index];
    DWORD previous = 0U;
    if (VirtualProtect(cart_cached_, kRomSize, PAGE_READONLY, &previous) == 0 ||
        VirtualProtect(cart_uncached_, kRomSize, PAGE_READONLY, &previous) == 0)
      return false;
    for (std::uint32_t section = 0U; section < overlays_.size(); ++section) {
      JfgGeneratedSectionMetadata metadata{};
      if (jfg_generated_section_metadata(section, &metadata) == 0)
        return false;
      if (metadata.is_overlay != 1U)
        continue;
      const std::uint64_t initialized =
          std::uint64_t{metadata.text_size} + metadata.data_size;
      if (overlays_[section] == nullptr || metadata.rom_start > rom.size() ||
          initialized > rom.size() - metadata.rom_start)
        return false;
      auto *destination = static_cast<std::uint8_t *>(overlays_[section]);
      for (std::size_t index = 0U;
           index < static_cast<std::size_t>(initialized); ++index) {
        destination[index ^ 3U] = rom[metadata.rom_start + index];
      }
    }
    return true;
#else
    (void)rom;
    return false;
#endif
  }

private:
#if defined(_WIN32)
  HANDLE mapping_ = nullptr;
  HANDLE rom_mapping_ = nullptr;
  std::uint8_t *base_ = nullptr;
  std::uint8_t *alias_ = nullptr;
  std::uint8_t *cart_cached_ = nullptr;
  std::uint8_t *cart_uncached_ = nullptr;
  std::array<void *, kMmioHostOffsets.size()> mmio_{};
  std::vector<void *> overlays_;
  std::vector<void *> gaps_;
  PVOID exception_handler_ = nullptr;
#else
  std::vector<std::uint8_t> bytes_;
#endif
  MmioTrace mmio_trace_{};
};

class Table final : public jfg::GeneratedOverlayTable {
public:
  std::size_t section_count() const override {
    return jfg_generated_section_count();
  }
  bool initialize_sections(std::span<std::int32_t> addresses) override {
    return jfg_generated_initialize_sections(addresses.data(),
                                             addresses.size()) != 0;
  }
  bool section_extents(std::uint32_t section,
                       jfg::GeneratedSectionExtents &output) const override {
    JfgGeneratedSectionMetadata metadata{};
    if (jfg_generated_section_metadata(section, &metadata) == 0 ||
        metadata.is_overlay > 1U)
      return false;
    output = {metadata.text_size, metadata.data_size, metadata.bss_size,
              metadata.is_overlay == 1U};
    return true;
  }
  bool update_section_lifecycle(jfg::GeneratedSectionLifecycle operation,
                                std::uint32_t section,
                                std::int32_t base) override {
    return jfg_generated_section_lifecycle(
               operation == jfg::GeneratedSectionLifecycle::load ? 1U : 2U,
               section, base) == 0;
  }
  std::size_t relocation_count(std::uint32_t section) const override {
    return jfg_generated_relocation_count(section);
  }
  bool relocation_sites(std::uint32_t section,
                        std::span<const std::uint32_t> &output) const override {
    const std::uint32_t *sites = nullptr;
    std::size_t count = 0U;
    if (jfg_generated_relocation_sites(section, &sites, &count) == 0 ||
        count != relocation_count(section) || (count != 0U && sites == nullptr))
      return false;
    output = {sites, count};
    return true;
  }
  bool
  relocation_descriptors(std::uint32_t section,
                         std::span<const jfg::GeneratedR32RelocationDescriptor>
                             &output) const override {
    const JfgGeneratedR32Descriptor *raw = nullptr;
    std::size_t count = 0U;
    if (jfg_generated_relocation_descriptors(section, &raw, &count) == 0 ||
        count != relocation_count(section) || (count != 0U && raw == nullptr))
      return false;
    descriptors_.clear();
    descriptors_.reserve(count);
    for (std::size_t index = 0U; index < count; ++index)
      descriptors_.push_back({raw[index].site_offset, raw[index].target_section,
                              raw[index].target_offset});
    output = descriptors_;
    return true;
  }
  jfg::GeneratedRelocationResult
  apply_relocations_checked(std::span<std::uint8_t> rdram,
                            std::uint32_t section,
                            std::size_t expected) override {
    const bool applied = jfg_generated_apply_relocations_checked(
                             rdram.data(), rdram.size(), section) != 0;
    return {applied, applied ? expected : 0U};
  }
  jfg::GeneratedOverlayFunction
  lookup_function(std::int32_t address) const override {
    return jfg_generated_lookup_function(address);
  }

private:
  mutable std::vector<jfg::GeneratedR32RelocationDescriptor> descriptors_;
};

#if defined(JFG_PHASE8_LIVE_RUNTIME)
struct PendingLiveGraphicsTask final {
  jfg::Rt64GraphicsTask task{};
  std::array<std::uint32_t, 6U> descriptor{};
  std::vector<std::byte> rdram;
  bool skipped = false;
  // Community ranged import (see community_upper_ranges): one flag per
  // graphics overlay shadow, set when this snapshot's copy of that shadow
  // may differ from the initial overlay image; the extra last flag means a
  // display-list patch landed outside every shadow. Empty = unknown.
  std::vector<std::uint8_t> community_upper_dirty;
};

struct LiveGraphicsOverlayShadow final {
  std::uint32_t section = 0U;
  std::uint32_t linked_base = 0U;
  std::uint32_t extent = 0U;
  std::uint32_t physical_base = 0U;
};

struct LiveViFieldRegisters final {
  std::uint32_t origin = 640U;
  std::uint32_t y_scale = 0x00000400U;
  std::uint32_t vertical_start = 0x002501FFU;
  std::uint32_t vertical_burst = 0x000E0204U;
  std::uint32_t intr = 2U;
};
#endif

struct State : JfgOriginalTimingFast {
  State() : JfgOriginalTimingFast{} { execution_fast_revision = UINT64_MAX; }
  bool ledgered = false;
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  bool presentation_identity_enabled = false;
  jfg::PresentationIdentityTracker presentation_identities;
#endif
  bool os_initialized = false;
  jfg::GeneratedOverlayRuntime *runtime = nullptr;
  const std::uint8_t *rom = nullptr;
  std::size_t rom_size = 0U;
  std::unique_ptr<ThreadScheduler> scheduler;
  std::unordered_map<std::uint32_t, int> threads;
  std::vector<std::pair<std::uint32_t, int>> thread_order;
  std::vector<std::uint32_t> thread_entries;
  std::unordered_set<std::uint32_t> queues;
  std::unordered_set<std::uint32_t> active_overlay_sections;
  // Linked section metadata is immutable for this generated root. Dynamic
  // allocation/publication checks still run for every dispatched call.
  std::unordered_map<std::uint32_t,
      std::pair<std::uint32_t, JfgGeneratedSectionMetadata>> generated_section_cache;
  std::unordered_set<std::uint32_t> guest_overlay_loads_in_progress;
  // Fresh guest loads recreate DATA/BSS before their init callback. Suspended
  // modules resume the existing state and must never enter this set.
  std::unordered_set<std::uint32_t> pending_cold_overlay_images;
  jfg::ControllerMapping controller_mapping{};
  jfg::ControllerPorts controller_ports;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  jfg::ControllerCatalog controller_catalog;
  jfg::SdlControllerBackend sdl_controllers;
#endif
  jfg::ControllerInputGate controller_input_gate;
  std::filesystem::path controller_config_path;
  std::chrono::steady_clock::time_point controller_config_check{};
  std::uint32_t executing_generated_section = UINT32_MAX;
  bool executing_generated_overlay = false;
  const char *last_overlay_publication_failure = "none";
  std::unordered_set<std::uint32_t> generated_relocation_targets;
  jfg::boot::NativeRunlinkIdentityIndex native_overlay_identity_index;
  std::unordered_map<std::uint32_t, std::pair<std::uint32_t, std::uint32_t>>
      events;
  std::uint32_t pi_command_queue = 0U;
  bool vi_manager_thread_created = false;
  std::uint32_t vi_mode = 0U;
  std::uint32_t vi_queue = 0U;
  std::uint32_t vi_message = 0U;
  std::uint32_t vi_retrace_interval = 0U;
  std::uint32_t vi_current_framebuffer = 0U;
  std::uint32_t vi_next_framebuffer = 0U;
  std::uint32_t vi_special_features = 0U;
  bool vi_blacked = true;
  std::uint64_t vi_retraces = 0U;
  std::uint64_t vi_frames = 0U;
  std::uint64_t vi_interrupts = 0U;
  std::uint64_t vi_messages_delivered = 0U;
  std::uint32_t retrace_target = 3U;
  std::uint32_t poll_target = 0U;
  std::uint32_t vi_internal_queue = 0U;
  std::uint32_t vi_internal_message = 0U;
  EventJournal journal;
  MmioTrace *mmio_trace = nullptr;
  bool scheduler_running = false;
  std::uint32_t root_priority = 0U;
  std::uint32_t interrupt_mask = 0U;
  Timers timers;
  std::uint64_t os_time_base = 0U;
  std::uint64_t os_time_count_origin = 0U;
  std::uint32_t last_pi_rom = 0U;
  std::uint32_t last_pi_dram = 0U;
  std::uint32_t last_pi_size = 0U;
  bool controller_initialized = false;
  bool controller_guest_init_probe = false;
  std::uint32_t pending_si_completions = 0U;
  SiDeadline si_deadline;
  // Private generator-hook observer. This does not feed Count or deadlines.
  bool si_count_probe = false;
  bool renderer_writeback_probe = false;
  bool guest_leaf_probe = false;
  bool guest_initializing = false;
  bool guest_os_probe = false;
  std::uint64_t diagnostic_input_suspend_begin=0,diagnostic_input_suspend_end=0;
  bool native_audio_source = false;
  bool native_audio_playback = false;
  std::uint64_t native_audio_source_requests = 0;
  bool timing_fixture = false;
  bool original_os_host_pacing = false;
  std::uint64_t original_window_pump_vi = UINT64_MAX;
  bool original_os_host_anchor_ready = false;
  jfg::audio::ViPlaybackClock original_os_host_clock;

  std::unique_ptr<GuestThreadTransport<recomp_context>> guest_transport;
  std::uint32_t guest_epc = 0U, guest_cause = 0U;
  jfg::boot::ReferenceCompare guest_timer;
  std::uint32_t guest_active_rsp_task = 0U, guest_active_rsp_type = 0U;
  std::uint64_t guest_exceptions = 0U;
  jfg::AsyncDiagnosticFile guest_clock_trace;
  struct FileCloser { void operator()(FILE* file) const noexcept { std::fclose(file); } };
  std::unique_ptr<FILE, FileCloser> device_event_stream;
  jfg_device_event_probe device_event_probe{};
  std::ofstream eret_transfer_stream;
  EretTransferProbe eret_transfer_probe;
  std::ofstream instruction_effect_stream;
  InstructionEffectTrace instruction_effect_trace;
  std::uint32_t instruction_effect_update = 0U, instruction_effect_section = 0U;
  std::uint32_t device_event_previous_pc = 0U, device_event_previous_word = 0U;
  std::uint32_t device_event_previous_thread = 0U;
  bool device_event_successor = false;
  jfg::boot::TlbRegisters32 tlb_registers;
  std::vector<std::uint32_t> saved_rcp_masks;
  std::array<std::uint8_t, jfg::kPifRamBytes> pif_ram{};
  bool cic_response_pending = false;
  std::uint64_t generated_calls = 0U;
  std::uint64_t mapped_calls = 0U;
  std::uint64_t dispatch_calls = 0U;
  std::uint64_t sync_print_sink_calls = 0U;
  std::uint64_t sound_queue_recoveries = 0U;
  std::uint64_t phase9_player_control_calls = 0U;
  std::uint64_t phase9_weapon_update_calls = 0U;
  std::uint64_t phase9_weapon_fire_held_updates = 0U;
  std::uint64_t phase9_weapon_fire_pressed_updates = 0U;
  std::uint64_t phase9_weapon_dummy_fire_calls = 0U;
  std::uint64_t phase9_weapon_fire_calls = 0U;
  std::uint64_t phase9_weapon_shot_calls = 0U;
  std::uint64_t phase9_weapon_hit_calls = 0U;
  std::uint64_t phase9_enemy_kill_calls = 0U;
  std::uint64_t phase9_collision_calls = 0U;
  std::uint64_t phase9_particle_update_calls = 0U;
  std::uint64_t phase9_cutscene_active_calls = 0U;
  std::uint64_t phase9_cutscene_camera_calls = 0U;
  std::uint64_t phase9_level_change_calls = 0U;
  std::uint64_t phase9_death_restart_calls = 0U;
  std::uint64_t health_overlay_death_reload_generation = 0U;
  std::uint64_t phase9_player_hit_check_calls = 0U;
  std::uint32_t phase9_player_actor = 0U;
  jfg::mod::NavigationMod navigation_mod;
  std::filesystem::path navigation_output;
  jfg::AsyncBoundedTrace movement_trace, jump_trace;
  jfg::AsyncSnapshots snapshot_writer;
  std::shared_ptr<const std::string> navigation_mesh_export;
  std::uint64_t phase9_hints_control_calls = 0U;
  std::uint64_t phase9_hints_talk_calls = 0U;
  std::uint32_t phase9_hints_actor = 0U;
  std::uint64_t phase9_king_bear_control_calls = 0U;
  std::uint32_t phase9_king_bear_actor = 0U;
  std::uint32_t phase9_king_bear_callback = 0U;
  std::uint64_t phase9_hinttext_start_calls = 0U;
  std::uint64_t phase9_hinttext_stop_calls = 0U;
  std::uint64_t phase9_hinttext_active_calls = 0U;
  std::uint64_t phase9_hinttext_active_zero_returns = 0U;
  std::uint64_t phase9_hinttext_active_nonzero_returns = 0U;
  std::uint64_t phase9_hinttext_update_calls = 0U;
  std::uint64_t phase9_hinttext_advance_calls = 0U;
  std::uint64_t phase9_hinttext_accept_calls = 0U;
  std::uint32_t phase9_hinttext_active_last_return = 0U;
  std::uint64_t interrupt_timeslices = 0U;
  std::uint64_t rsp_yield_requests = 0U, rsp_yield_queries = 0U;
  std::unordered_map<std::uint32_t, std::uint32_t> empty_receive_polls;
  std::uint64_t receive_successes = 0U;
  std::uint64_t receive_empties = 0U;
  std::uint64_t receive_blocks = 0U;
  std::uint32_t last_receive_queue = 0U;
  std::uint32_t last_receive_block = 0U;
  std::int32_t last_receive_result = 0;
  std::uint64_t queue_create_calls = 0U;
  std::uint64_t queue_send_calls = 0U;
  std::uint64_t queue_recv_calls = 0U;
  std::uint64_t sp_task_load_calls = 0U;
  std::uint64_t sp_task_start_calls = 0U;
  std::uint64_t controller_read_start_calls = 0U;
  std::uint64_t controller_get_data_calls = 0U;
  std::uint64_t vi_swap_calls = 0U;
  std::uint64_t graphics_tasks = 0U;
  std::uint64_t graphics_commands = 0U;
  std::uint64_t graphics_triangles = 0U;
  std::uint64_t presented_frames = 0U;
  bool frame_captured = false;
  std::uint64_t controller_samples = 0U;
  std::uint64_t non_neutral_controller_samples = 0U;
  std::uint64_t non_neutral_controller_writes = 0U;
  std::uint64_t controller_disconnects = 0U;
  std::uint64_t controller_reconnects = 0U;
  std::uint64_t game_pressed_observations = 0U;
  std::uint16_t last_game_pressed_buttons = 0U;
  std::uint64_t game_stick_observations = 0U;
  std::int8_t last_game_stick_x = 0;
  std::int8_t last_game_stick_y = 0;
  std::uint64_t menu_stick_observations = 0U;
  std::int8_t last_menu_stick_x = 0;
  std::int8_t last_menu_stick_y = 0;
  std::size_t input_replay_event_count = 0U;
  std::uint64_t flash_persist_count = 0U;
  bool flash_image_loaded = false;
  bool flash_image_created = false;
  std::uint32_t active_bzero_address = 0U;
  std::uint32_t active_bzero_length = 0U;
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  jfg::FlashRamStore flashram;
  bool fast_replay = false;
  bool realtime_replay = false;
  bool exit_requested = false;
  bool modern_controls_active = false;
  std::array<jfg::PcInputConfig,4> pc_input{};
  std::array<jfg::ModernControlsConfig,4> modern_controls{};
  std::array<jfg::ModernAimLatch,4> modern_aim_latch{};
  std::array<jfg::ModernScopeZoom,4> modern_scope_zoom{};
  std::array<std::uint8_t,4> modern_live_flags{};
  std::array<bool,4> modern_physical_neutral{true,true,true,true};
  std::array<jfg::ControllerReplaySample,4> pc_frames{};
  std::array<bool,4> pc_mouse_used{};
  bool pc_record_extended = false;
  std::array<jfg::PcOrbit,4> pc_orbits{};
  std::array<jfg::PcShoulder,4> pc_shoulders{};
  std::uint32_t pc_shoulder_actor=0;
  std::uint32_t pc_animation_actor=0;
  float pc_camera_ticks=0;
  bool pc_camera_resolved_valid=false;
  jfg::PcOrbitPoint pc_camera_resolved{};
  std::uint32_t pc_camera_actor=0,pc_camera_control=0,pc_camera_stack=0;
  jfg::PcLookDelta pc_camera_look{};
  std::uint64_t pc_aim_calls = 0, pc_direct_aim_calls = 0;
  std::array<jfg::PcLookDelta,4> pc_aim{};
  std::array<jfg::PcStickMotion,4> pc_stick_motion{};
  std::array<jfg::PcScopeMotion,4> pc_scope_motion{};
  std::array<jfg::ModernScopeSensitivity,4> modern_scope_sensitivity{};
  std::array<jfg::PcDeviceReleaseGate,4> pc_device_release{};
  std::array<int,4> pc_devices{-2,-2,-2,-2};
  std::array<jfg::PcRumbleEffect,4> pc_rumble_effects{};
  std::array<std::uint16_t,4> pc_rumble_levels{};
  std::array<int,4> pc_rumble_devices{-2,-2,-2,-2};
  std::array<bool,4> pc_device_ready{};
  std::unique_ptr<jfg::PcRumbleOutput> pc_rumble_output;
  jfg::PcWheelPulses pc_wheel;
  jfg::PcMouseMotion pc_mouse;
  int pc_wheel_key = 0;
  std::atomic<bool> pc_mouse_captured{false};
  bool pc_input_blocked = true, pc_raw_available = false;
  jfg::WindowInputMailbox window_input;
  jfg::MenuPointerFrame menu_pointer_frame{};
  jfg::GuestMenuState guest_menu{};
  jfg::GuestMenuPointer menu_pointer;
  std::array<jfg::GuestMenuConfirm,4> menu_confirm;
  std::optional<jfg::GuestMenuState> menu_selection_feedback;
  jfg::MenuPointerReplay menu_pointer_replay;

  std::array<bool, 256U> host_keys{};
  std::array<bool, 256U> host_key_presses{};
  std::uint16_t latched_controller_buttons = 0U;
  std::int8_t latched_controller_stick_x = 0;
  std::int8_t latched_controller_stick_y = 0;
  bool latched_controller_connected = true;
  jfg::DeterministicInputReplay input_replay;
  bool input_replay_loaded = false;
  bool input_replay_by_poll = false;
  bool input_replay_recorded_time = false;
  bool input_replay_gameplay_time = false, replay_gameplay_clock_ready = false;
  std::size_t replay_gameplay_first_poll = 0;
  std::uint64_t replay_gameplay_origin_vi = 0;
  RecordedInputTimeline recorded_input_timeline;
  std::string frame_capture_path;
  std::string rdram_capture_path;
  const recomp_context* private_fault_cpu = nullptr; // Explicit RAM-capture sessions only.
  bool private_model_pointer_watch = false;
  bool private_model_hardware_watch = false;
  bool private_model_page_watch = false;
  bool private_model_pages_ready = false;
  std::uintptr_t private_memory_writer_pc = 0U;
  bool private_model_write_trap = false;
  std::uint32_t private_model_write_offset = 0xf5b0cU;

  std::uint32_t private_model_pointer = 0U;
  std::array<std::uint32_t,64> private_model_probe_pcs{};
  std::uint64_t private_model_probe_position = 0U;

  std::string progress_path;
  std::string input_record_path;
  std::string retrace_hash_path;
  jfg::AsyncDiagnosticFile input_record;
  std::ofstream phase95_poll_trace;
  std::ofstream retrace_hash_trace;
  std::ofstream update_hash_trace;
  std::ofstream update_word_trace;
  std::uint32_t update_word_address = 0U;
  std::ofstream controller_return_trace;
  std::uint32_t controller_return_rows = 0U;
  std::ofstream poll_hash_trace;
  std::ofstream event_trace;
  std::array<std::pair<std::uint64_t, std::uint64_t>, 2U> event_ranges{};
  std::size_t event_range_count = 0U;
  std::uint64_t event_trace_rows = 0U;
  std::uint64_t completed_game_updates = 0U;
  std::uint64_t timing_game_updates = 0U;
  std::uint64_t focus_update_start = 0U;
  std::uint64_t focus_update_end = 0U;
  std::uint32_t watch_word_address = 0U;
  std::ofstream watch_word_trace;
  std::uint32_t watch_dispatch_depth = 0U;
  std::uint32_t watch_dispatch_changes = 0U;
  std::uint32_t entry_probe_target = 0U;
  std::ofstream entry_probe_trace;
  std::uint32_t entry_probe_hits = 0U;
  std::ofstream entry_fpu_trace;
  std::ofstream entry_memory_trace;
  std::ofstream entry_gpr_trace;
  std::ofstream instruction_probe_trace;
  std::uint32_t instruction_probe_hits = 0U;
  std::vector<std::uint32_t> point_probe_pcs;
  std::vector<std::uint32_t> point_probe_words;
  std::ofstream point_probe_trace;
  std::uint32_t point_probe_hits = 0U;
  jfg::AsyncDiagnosticFile timing_trace;
  jfg::AsyncBoundedTrace reticle_trace;
  bool reticle_trace_started = false, reticle_trace_active = false, reticle_trace_enabled = false;
  std::uint32_t reticle_actor = 0U, reticle_control = 0U;
  GameplayTrace gameplay_trace;
  // Optional timing diagnostics; never modify the guest clock or memory.
  bool gameplay_trace_actors = true;
  // Internal qualification only. No launcher/profile switch selects this policy.
  bool diagnostic_host_clock_requested = false;
  std::uint64_t diagnostic_clock_boundary_vi = 0;
  bool diagnostic_clock_boundary_seen = false;
  unsigned diagnostic_boundary_stall_ms = 0;
  // IO/exception boundaries force the full handler; ordinary instructions
  // share a bounded host-clock sampling quantum without modeled time charges.
  static constexpr unsigned kHostClockPollInstructions = 128U;
  HostCountClock host_count_clock;
  CountSampleTape native_clock_samples;
  std::uint64_t diagnostic_clock_release_vi = 0;
  bool diagnostic_clock_released = false;
  std::vector<char> native_clock_sample_buffer;
  std::ofstream native_clock_sample_output;
  std::ifstream native_clock_sample_input;
  HostDeadlineWait host_deadline_wait;
  std::uint64_t profile_host_wait_us = 0U;
  std::uint64_t profile_idle_count_ticks = 0U;
  bool diagnostic_cpu_costs = false;
  bool resident_math_enabled=false;
  bool diagnostic_bsp_enabled=false;
  std::uint64_t bsp_native_calls=0,bsp_clock_refreshes=0;
  std::uint64_t pure_angle_calls=0,pure_diff_calls=0,pure_trig_calls=0;
  bool diagnostic_update_cycles = false;
  unsigned diagnostic_function_profile=0;
  bool diagnostic_gpu_costs=false;
  std::uint64_t diagnostic_profile_first_vi=3300,diagnostic_profile_last_vi=4500;
  struct FunctionCost { std::uint64_t self_ns=0, inclusive_ns=0, calls=0; };
  std::unordered_map<std::uint32_t, FunctionCost> profile_function_costs;
  std::uint64_t profile_function_self_ns = 0;
  std::uint64_t profile_out_of_line_instructions = 0U;
  std::uint64_t profile_full_execution_probes = 0U;
  std::uint64_t profile_native_handler_ns = 0U;
  std::uint64_t profile_dispatch_prefix_ns = 0U, profile_dispatch_prefix_calls = 0U;
  std::uint64_t profile_overlay_inventory_ns = 0U, profile_overlay_inventory_calls = 0U;
  std::uint64_t profile_relocation_ns = 0U, profile_relocation_calls = 0U, profile_relocation_probes = 0U;
  std::uint64_t gameplay_trace_vi = UINT64_MAX;
  std::uint64_t gameplay_trace_updates = 0U;
  std::array<std::array<std::uint64_t, 56>, 256> gameplay_actor_previous{};
  std::array<bool, 256> gameplay_actor_seen{};
  bool actor_timing_trace = false;
  std::uint64_t last_actor_trace_retrace =
      (std::numeric_limits<std::uint64_t>::max)();
  bool host_frame_start_initialized = false;
  std::chrono::steady_clock::time_point host_frame_start{};
  bool present_time_initialized = false;
  std::chrono::steady_clock::time_point present_time{};
  std::uint64_t host_frame_interval_max_us = 0U;
  std::uint64_t present_interval_max_us = 0U;
  std::uint64_t graphics_prepare_max_us = 0U;
  std::uint64_t graphics_submit_max_us = 0U;
  std::uint64_t graphics_present_max_us = 0U;
  std::uint64_t host_frames_over_25ms = 0U;
  std::uint64_t host_frames_over_50ms = 0U;
  std::uint64_t host_frames_over_100ms = 0U;
  std::uint64_t last_progress_retrace =
      (std::numeric_limits<std::uint64_t>::max)();
  std::filesystem::path flash_path;
  std::filesystem::path controller_pak_path;
  std::unique_ptr<jfg::ControllerAccessoryBus> controller_accessory_bus;
  jfg::ControllerPakSession controller_pak_session;
  bool controller_pak_loaded = false;
  bool controller_pak_created = false;
  std::array<LiveViFieldRegisters, 2U> vi_fields{};
  std::uint32_t vi_mode_horizontal_start = 0x006C02ECU;
  bool vi_mode_configured = false;
  bool host_frame_deadline_initialized = false;
  std::chrono::steady_clock::time_point host_frame_deadline{};
  jfg::Rt64ViRegisters rt64_vi;
  std::uint32_t vi_mode_control = 0U;
  void *native_window = nullptr;
  jfg::ViPresentationSize window_presentation{320U, 240U};
  bool window_widescreen=false;
  std::unique_ptr<jfg::Rt64Shell> rt64_shell;
  std::vector<std::byte> rt64_rdram;
  std::uint32_t graphics_color_image = 0U;
  bool graphics_frame_ready = false;
  std::array<std::uint32_t, 64U> recent_graphics_color_images{};
  std::array<std::uint32_t, 64U> recent_graphics_command_addresses{};
  std::array<std::uint32_t, 64U> recent_graphics_vi_selections{};
  std::array<std::uint32_t, 64U> recent_graphics_matrix_commands{};
  std::array<std::uint32_t, 64U> recent_graphics_vertices_loaded{};
  std::array<std::uint32_t, 64U> recent_graphics_vertices_in_clip{};
  std::array<std::uint32_t, 64U> recent_graphics_triangles{};
  std::size_t recent_graphics_position = 0U;
  std::vector<PendingLiveGraphicsTask> pending_graphics_tasks;
  std::vector<LiveGraphicsOverlayShadow> graphics_overlay_shadows;
  // Immutable ROM-derived overlay shadows. Restore these bytes for each task;
  // display-list traversal still patches that task's private snapshot.
  std::vector<std::byte> graphics_overlay_initial_data;
  std::vector<std::vector<std::byte>> graphics_snapshot_pool;
  // Community ranged import: dirty flags travelling with each pooled
  // snapshot (same order as graphics_snapshot_pool), the flags of the
  // snapshot RT64 imported last, and whether RT64's upper half is known.
  std::vector<std::vector<std::uint8_t>> community_snapshot_pool_dirty;
  std::vector<std::uint8_t> community_rt64_upper_dirty;
  bool community_rt64_upper_known = false;
  bool community_ranged_import = true;
  // Community native romCopy: 0 = not decided yet, 1 = on, 2 = off.
  std::uint8_t community_native_romcopy = 0U;
  std::uint64_t community_romcopy_calls = 0U, community_romcopy_bytes = 0U;
  // Community profile counters (PF lines, written once a second).
  std::uint64_t community_prof_prepare_us = 0U, community_prof_import_us = 0U,
                community_prof_rt64_us = 0U, community_prof_writeback_us = 0U,
                community_prof_tasks = 0U, community_prof_ranged = 0U,
                community_prof_import_kb = 0U, community_prof_present_us = 0U;
  std::array<std::uint64_t, 10U> community_prof_last{};
  std::chrono::steady_clock::time_point community_prof_time{};
  HostAudioDevice host_audio;
  std::uint64_t audio_task_microseconds = 0U;
  std::uint64_t graphics_prepare_microseconds = 0U;
  std::uint64_t graphics_submit_microseconds = 0U;
  std::uint64_t graphics_present_microseconds = 0U;
  std::array<std::uint32_t, 64U> recent_matrix_conversion_sources{};
  std::array<std::uint32_t, 64U> recent_matrix_conversion_destinations{};
  std::array<std::array<std::uint32_t, 4U>, 64U>
      recent_matrix_conversion_first_rows{};
  std::array<std::array<std::uint32_t, 4U>, 64U>
      recent_matrix_conversion_output_words{};
  std::size_t recent_matrix_conversion_position = 0U;
  std::array<std::array<std::uint32_t, 3U>, 64U> recent_matrix_cat_args{};
  std::array<std::array<std::uint32_t, 9U>, 64U>
      recent_matrix_cat_words{};
  std::size_t recent_matrix_cat_position = 0U;
  std::array<std::uint32_t, 8U> matrix_watch_addresses{
      0x801FDA80U, 0x801FDAC0U, 0x802239A0U, 0x802239E0U,
      0x800FAAF0U, 0x800FAF7CU};
  std::array<std::uint32_t, 8U> matrix_watch_words{};
  std::array<std::uint32_t, 64U> recent_matrix_watch_addresses{};
  std::array<std::uint32_t, 64U> recent_matrix_watch_before{};
  std::array<std::uint32_t, 64U> recent_matrix_watch_after{};
  std::array<std::uint32_t, 64U> recent_matrix_watch_targets{};
  std::array<std::uint32_t, 64U> recent_matrix_watch_callers{};
  std::array<std::uint32_t, 64U> recent_matrix_watch_current_targets{};
  std::array<std::uint32_t, 64U> recent_matrix_watch_return_addresses{};
  std::size_t recent_matrix_watch_position = 0U;
  std::array<std::uint32_t, 6U> poison_matrix_watch{};
  std::array<std::uint32_t, 16U> poison_dispatch_history{};
  jfg::Rt64ShellError last_rt64_error = jfg::Rt64ShellError::none;
  std::array<std::uint32_t, 6U> last_graphics_descriptor{};
  std::uint32_t last_graphics_command_address = 0U;
  std::uint32_t last_graphics_command_word = 0U;
  std::uint32_t materialize_reject_reason = 0U;
  std::uint32_t materialize_reject_address = 0U;
  std::uint32_t materialize_reject_word0 = 0U;
  std::uint32_t materialize_reject_word1 = 0U;
  std::uint32_t materialize_parent_address = 0U;
  std::uint32_t materialize_parent_word0 = 0U;
  std::uint32_t materialize_parent_word1 = 0U;
  std::uint32_t materialize_block_address = 0U;
  std::uint32_t materialize_parent_segment_base = 0U;
#endif
  std::array<std::uint64_t, kMappingCount> mapped_call_counts{};
  std::uint32_t loaded_rsp_task = 0U;
  std::uint32_t loaded_rsp_task_type = 0U;
  std::uint64_t decoded_audio_tasks = 0U;
  bool audio_device_initialized = false;
  bool audio_device_started = false;
  std::uint32_t audio_device_frequency = 0U;
  std::uint64_t audio_device_buffers = 0U;
  std::uint64_t audio_device_queued_bytes = 0U;
  std::uint64_t audio_device_consumed_bytes = 0U;
  std::uint64_t audio_device_consumed_ms = 0U;
  std::uint64_t audio_device_underruns = 0U;
  std::uint64_t audio_device_overruns = 0U;
  int controller_read_thread = -1;
  std::uint64_t si_receive_calls = 0U;
  std::uint32_t last_si_receive_return = 0U;
  std::uint32_t last_si_receive_block = 0U;
  std::array<std::uint32_t, 32U> recent_dispatches{};
  std::size_t recent_dispatch_position = 0U;
  std::array<std::uint32_t, 16U> last_si_dispatch_trace{};
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  // Community effect recorder and teleport pacing (community_effect_*).
  std::ofstream community_effect_stream;
  bool community_effect_log_tried = false;
  std::chrono::steady_clock::time_point community_effect_start{};
  std::unordered_map<std::uint32_t, std::uint32_t> community_effect_counts;
  std::uint64_t community_effect_frame = 0U;
  std::uint32_t community_teleport_pacing = 0U;
  std::uint32_t community_teleport_n64_rate = 0U;
  std::unordered_map<std::uint32_t, std::uint32_t> community_teleport_pending;
  std::unordered_set<std::uint32_t> community_teleport_seen;
  std::uint32_t community_teleport_speed_percent = 100U;
  std::uint32_t community_beam_flash_ticks = 0U;
  std::uint32_t community_beam_strand_percent = 100U;
  std::unordered_map<std::uint32_t, std::pair<std::uint32_t, std::uint32_t>>
      community_flash_gates;
  std::uint32_t community_beam_bolt_style = 1U;
  std::unordered_set<std::uint32_t> community_beam_has_bolt;
  std::uint32_t community_capture_left = 0U;
  std::uint32_t community_capture_count = 0U;
  std::uint32_t community_capture_limit = 0U;
  std::uint64_t community_beam_seen_frame = 0U;
  std::filesystem::path community_capture_dir;
  std::uint32_t community_last_presented_vi = 0U;
  bool community_skip_stale_presents = true;
  std::uint64_t community_event_ties = 0U;
  std::uint64_t community_watch_hash = 0U;
  std::uint32_t community_watch_dumps = 0U;
  std::string community_fb_summary;
  bool community_hide_splats = true;
  bool community_hide_reflections = false;
  std::uint32_t community_capture_every = 0U;
  // F9 recording (community_hotkey_poll): every Nth presented frame while on.
  bool community_hotkey_down = false;
  bool community_hotkey_on = false;
  std::uint32_t community_hotkey_every = 2U;
  std::uint32_t community_hotkey_presses = 0U;
  std::uint64_t community_audio_underruns = 0U;
  std::int64_t community_capture_from_ms = 0;
  std::int64_t community_capture_to_ms = 0;
  std::uint64_t community_blur_state = ~0ULL;
  std::uint32_t community_reflect_lines = 0U;
  std::uint32_t community_blood_lines = 0U;
  bool community_blood_base_logged = false;
#endif
};

void stop_pc_rumble(State& state) noexcept {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if(state.pc_rumble_output)state.pc_rumble_output->stop();
#else
  (void)state;
#endif
}

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
State *g_active_child_state = nullptr;
// Community crash report: the guest thread that made the latest guest call,
// so a crash can name its registers (set in dispatch()), and the guest
// instruction about to run (set by the per-instruction probe).
recomp_context *g_community_crash_context = nullptr;
volatile unsigned g_community_crash_pc = 0U;

class LiveRt64Window final {
public:
  static constexpr UINT kAspectMessage = WM_APP + 22U;
  LiveRt64Window(State &state, const bool visible) : input_(state.window_input), captured_(state.pc_mouse_captured) {
    // No guest state is accessed by this thread. A window move/resize can enter
    // Win32's modal loop for arbitrarily long without parking the guest CPU.
    std::promise<void> ready;
    auto initialized = ready.get_future();
    owner_ = std::thread([this, visible, &ready] {
      thread_id_ = GetCurrentThreadId();
      instance_ = GetModuleHandleW(nullptr);
      WNDCLASSW window_class{};
      window_class.lpfnWndProc = window_proc;
      window_class.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(1));
      window_class.hInstance = instance_;
      window_class.hCursor = LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));
      window_class.lpszClassName = kClassName;
      atom_ = RegisterClassW(&window_class);
      if (atom_ != 0U) {
        wchar_t parent_text[32]{};
        HWND parent=nullptr;
        if(GetEnvironmentVariableW(L"JFG_FRONTEND_PARENT",parent_text,32)>0) {
          wchar_t* end=nullptr;const auto value=wcstoull(parent_text,&end,10);
          parent=reinterpret_cast<HWND>(static_cast<std::uintptr_t>(value));
          if(!end || *end || !IsWindow(parent) || !GetPropW(GetAncestor(parent,GA_ROOT),L"JfgFrontend")) {
            ready.set_value();UnregisterClassW(kClassName,instance_);return;
          }
        }
        const DWORD style=parent ? WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS : WS_OVERLAPPEDWINDOW;
        RECT rectangle{0,0,640,480};
        if(parent)GetClientRect(parent,&rectangle);
        if (parent || AdjustWindowRectEx(&rectangle,style,FALSE,0U)) {
          window_ = CreateWindowExW(0U,kClassName,L"Jet Force Gemini Recomp",style,
              parent?0:CW_USEDEFAULT,parent?0:CW_USEDEFAULT,rectangle.right-rectangle.left,
              rectangle.bottom-rectangle.top,parent,nullptr,instance_,this);
        }
        if(window_) {
          RAWINPUTDEVICE mouse{0x01,0x02,0,window_};
          raw_available_=RegisterRawInputDevices(&mouse,1,sizeof(mouse))!=FALSE;
          if(parent)PostMessageW(GetAncestor(parent,GA_ROOT),WM_APP+20,reinterpret_cast<WPARAM>(window_),0);
          if(visible){ShowWindow(window_,SW_SHOW);UpdateWindow(window_);}
        }
      }
      ready.set_value();
      if (window_) {
        MSG message{};
        while (GetMessageW(&message,nullptr,0U,0U)>0) {
          TranslateMessage(&message);
          DispatchMessageW(&message);
        }
        ClipCursor(nullptr);
        DestroyWindow(window_);
      }
      if (atom_) UnregisterClassW(kClassName,instance_);
    });
    initialized.get(); // Publishes the initialized HWND and owner thread ID.
    state.native_window = window_;
    state.pc_raw_available = raw_available_;
  }
  ~LiveRt64Window() {
    if (window_) PostMessageW(window_,kStopMessage,0,0);
    if (owner_.joinable()) owner_.join();
  }
  LiveRt64Window(const LiveRt64Window&) = delete;
  LiveRt64Window& operator=(const LiveRt64Window&) = delete;
  [[nodiscard]] HWND get() const noexcept { return window_; }
  [[nodiscard]] DWORD thread_id() const noexcept { return thread_id_; }
private:
  static LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    if (message == WM_NCCREATE) {
      const auto *create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
      SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto *self = reinterpret_cast<LiveRt64Window*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if (!self) return DefWindowProcW(window,message,wparam,lparam);
    if(message==WM_INPUT) {
      RAWINPUT raw{};UINT size=sizeof(raw);
      if(GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam),RID_INPUT,&raw,&size,sizeof(RAWINPUTHEADER))!=UINT(-1) &&
         raw.header.dwType==RIM_TYPEMOUSE && !(raw.data.mouse.usFlags&MOUSE_MOVE_ABSOLUTE) && self->captured_.load())
        self->input_.motion(raw.data.mouse.lLastX,raw.data.mouse.lLastY);
    }
    // The render child can receive activation messages while the launcher
    // root is already foreground. Keep those clicks; only a click returning
    // from another application is consumed to avoid firing on focus return.
    if(message==WM_MOUSEACTIVATE && LOWORD(lparam)==HTCLIENT)
      return GetForegroundWindow()==GetAncestor(window,GA_ROOT)?MA_ACTIVATE:MA_ACTIVATEANDEAT;
    // Always restore the client cursor after a nonclient resize hit. Leave
    // resize borders to DefWindowProc, including during gameplay capture.
    if(message==WM_SETCURSOR && LOWORD(lparam)==HTCLIENT) {
      SetCursor(self->captured_.load()?nullptr:LoadCursorW(nullptr,MAKEINTRESOURCEW(32512)));
      return TRUE;
    }
    if(message==WM_MOUSELEAVE || message==WM_SIZE) self->input_.pointer_leave();
    if(message==WM_MOUSEMOVE) {
      RECT r{};GetClientRect(window,&r);
      self->input_.pointer(static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam)),r.right,r.bottom);
      TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT),TME_LEAVE,window,0};TrackMouseEvent(&track);
    }
    if(message>=WM_MOUSEFIRST && message<=WM_MOUSELAST) {
      int key=0;bool down=false;
      switch(message) {
      case WM_LBUTTONDOWN:case WM_LBUTTONUP:key=VK_LBUTTON;down=message==WM_LBUTTONDOWN;break;
      case WM_RBUTTONDOWN:case WM_RBUTTONUP:key=VK_RBUTTON;down=message==WM_RBUTTONDOWN;break;
      case WM_MBUTTONDOWN:case WM_MBUTTONUP:key=VK_MBUTTON;down=message==WM_MBUTTONDOWN;break;
      case WM_XBUTTONDOWN:case WM_XBUTTONUP:key=HIWORD(wparam)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2;down=message==WM_XBUTTONDOWN;break;
      case WM_MOUSEWHEEL:self->input_.wheel(static_cast<short>(HIWORD(wparam)));break;
      default:break;
      }
      if(key) {
        if(down && GetFocus()!=window) {
          const bool foreground=GetForegroundWindow()==GetAncestor(window,GA_ROOT);
          SetFocus(window);
          if(!foreground)return message==WM_XBUTTONDOWN?TRUE:0;
        }
        // Reacquiring our own capture emits WM_CAPTURECHANGED and clears
        // held buttons. A second button (RMB aim + LMB fire) shares capture.
        if(down && GetCapture()!=window)SetCapture(window);
        self->buttons_[static_cast<std::size_t>(key)]=down;
        if(key==VK_LBUTTON || key==VK_RBUTTON) {
          RECT r{};GetClientRect(window,&r);
          self->input_.pointer(static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam)),
                               r.right,r.bottom,key,down);
        } else self->input_.key(static_cast<std::size_t>(key),down);
        if(!down && GetCapture()==window && !self->buttons_[VK_LBUTTON] && !self->buttons_[VK_RBUTTON] &&
            !self->buttons_[VK_MBUTTON] && !self->buttons_[VK_XBUTTON1] && !self->buttons_[VK_XBUTTON2])ReleaseCapture();
        return message==WM_XBUTTONDOWN || message==WM_XBUTTONUP?TRUE:0;
      }
    }
    if(message==WM_CAPTURECHANGED)for(auto key:{VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2}){
      self->buttons_[static_cast<std::size_t>(key)]=false;self->input_.key(static_cast<std::size_t>(key),false);
    }
    if ((message == WM_KEYDOWN || message == WM_KEYUP ||
         message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && wparam < 256U) {
      // Alt is bindable, but Alt+F4 remains a window-manager shortcut.
      if ((message==WM_SYSKEYDOWN || message==WM_SYSKEYUP) && wparam==VK_F4)
        return DefWindowProcW(window,message,wparam,lparam);
      const bool down=message==WM_KEYDOWN || message==WM_SYSKEYDOWN;
      if(GetParent(window) && down && (wparam==VK_F11 || wparam==VK_ESCAPE)) {
        PostMessageW(GetAncestor(window,GA_ROOT),WM_APP+21,wparam,0);return 0;
      }
      self->input_.key(static_cast<std::size_t>(wparam),down,(lparam & (LPARAM(1)<<30))!=0);
      return 0;
    }
    if (message == WM_KILLFOCUS || (message==WM_ACTIVATEAPP && !wparam)) {
      self->captured_.store(false);self->buttons_.fill(false);
      if(GetCapture()==window)ReleaseCapture();ClipCursor(nullptr);
      self->input_.lose_focus();return 0;
    }
    if (message == WM_CLOSE) { self->input_.close(); return 0; }
    if (message == kStopMessage) { PostQuitMessage(0); return 0; }
    if (message == kAspectMessage) {
      if(wparam==320U && (lparam==180 || lparam==240)) {
        SetPropW(window,L"JfgGameAspectWidth",reinterpret_cast<HANDLE>(wparam));
        SetPropW(window,L"JfgGameAspectHeight",reinterpret_cast<HANDLE>(lparam));
        if(GetParent(window))
          PostMessageW(GetAncestor(window,GA_ROOT),WM_APP+24,reinterpret_cast<WPARAM>(window),0);
      }
      RECT client{};
      if (!GetParent(window) && wparam != 0U && lparam > 0 && !IsZoomed(window) && !IsIconic(window) &&
          GetClientRect(window,&client) && client.bottom > client.top) {
        const LONG height = client.bottom-client.top;
        const LONG width = static_cast<LONG>((std::uint64_t(height)*wparam+
            static_cast<std::uint64_t>(lparam)/2U)/static_cast<std::uint64_t>(lparam));
        RECT outer{0,0,width,height};
        if (AdjustWindowRectEx(&outer,static_cast<DWORD>(GetWindowLongPtrW(window,GWL_STYLE)),
            FALSE,static_cast<DWORD>(GetWindowLongPtrW(window,GWL_EXSTYLE))))
          SetWindowPos(window,nullptr,0,0,outer.right-outer.left,outer.bottom-outer.top,
              SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
      }
      return 0;
    }
    return DefWindowProcW(window,message,wparam,lparam);
  }
  static constexpr UINT kStopMessage = WM_APP+23U;
  static constexpr wchar_t kClassName[] = L"JfgPhase8LiveRt64Window";
  jfg::WindowInputMailbox& input_;
  std::atomic<bool>& captured_;
  std::array<bool,256> buttons_{};
  bool raw_available_ = false;
  HINSTANCE instance_ = nullptr;
  ATOM atom_ = 0U;
  HWND window_ = nullptr;
  DWORD thread_id_ = 0;
  std::thread owner_;
};

bool host_key_down(const State &state, const int virtual_key) noexcept {
  return virtual_key >= 0 && virtual_key < 256 &&
         (state.host_keys[static_cast<std::size_t>(virtual_key)] ||
          state.host_key_presses[static_cast<std::size_t>(virtual_key)]);
}

void trace_phase9_event(State &state, const char *event,
                        const std::uint64_t poll) {
  if (!state.event_trace.is_open())
    return;
  bool selected = false;
  for (std::size_t index = 0U; index < state.event_range_count; ++index) {
    const auto [first, last] = state.event_ranges[index];
    selected |= poll >= first && poll <= last;
  }
  if (!selected)
    return;
  if (++state.event_trace_rows > 4096U) {
    state.exit_requested = true;
    return;
  }
  state.event_trace << state.event_trace_rows << '\t' << event << '\t'
                    << poll << '\t' << state.completed_game_updates << '\t'
                    << state.vi_frames << '\t' << state.vi_retraces << '\t'
                    << state.controller_read_start_calls << '\t'
                    << state.controller_get_data_calls << '\t'
                    << (state.latched_controller_connected ? 1 : 0) << '\t'
                    << state.latched_controller_buttons << '\t'
                    << static_cast<int>(state.latched_controller_stick_x) << '\t'
                    << static_cast<int>(state.latched_controller_stick_y) << '\n';
}

struct XInputApi {
  using GetState=DWORD(WINAPI *)(DWORD,XINPUT_STATE*);
  using SetState=DWORD(WINAPI *)(DWORD,XINPUT_VIBRATION*);
  GetState get=nullptr;SetState set=nullptr;
};
const XInputApi& xinput_api() noexcept {
  static const XInputApi api=[]() noexcept {
    for(const wchar_t* library:{L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"}) {
      const auto module=LoadLibraryW(library);
      if(!module)continue;
      const auto get=reinterpret_cast<XInputApi::GetState>(GetProcAddress(module,"XInputGetState"));
      const auto set=reinterpret_cast<XInputApi::SetState>(GetProcAddress(module,"XInputSetState"));
      if(get)return XInputApi{get,set};
      FreeLibrary(module);
    }
    return XInputApi{};
  }();
  return api;
}

bool read_xinput_device(int device,
                        jfg::StandardControllerSample &sample) noexcept {
  const auto get_state=xinput_api().get;
  if (get_state == nullptr)
    return false;
  if (device < 0 || device >= 4)
    return false;
  XINPUT_STATE state{};
  bool connected = false;
  static std::array<ULONGLONG, 4> retry_after{};
  const auto now = GetTickCount64();
  for (DWORD port = 0U; port < 4U; ++port) {
    if (now < retry_after[port])
      continue;
    if (port != static_cast<DWORD>(device))
      continue;
    if (get_state(port, &state) == ERROR_SUCCESS) {
      connected = true;
      break;
    }
    retry_after[port] = now + 500;
  }
  if (!connected)
    return false;
  constexpr std::array<WORD, 15> masks{0x1000, 0x2000, 0x4000, 0x8000, 0x20,
                                       0,      0x10,   0x40,   0x80,   0x100,
                                       0x200,  1,      2,      4,      8};
  for (std::size_t i = 0U; i < masks.size(); ++i)
    sample.buttons[i] = (state.Gamepad.wButtons & masks[i]) != 0;
  sample.axes = {state.Gamepad.sThumbLX,
                 -static_cast<int>(state.Gamepad.sThumbLY),
                 state.Gamepad.sThumbRX,
                 -static_cast<int>(state.Gamepad.sThumbRY),
                 state.Gamepad.bLeftTrigger * 32767 / 255,
                 state.Gamepad.bRightTrigger * 32767 / 255};
  return true;
}

jfg::PcInputConfig active_pc_input(const State &state,std::size_t port);

bool pc_rumble_eligible(const State& state,const jfg::mod::Memory& ram,std::size_t port) {
  return state.modern_controls_active && !state.pc_input_blocked && !state.exit_requested &&
    (state.play_mode || state.input_replay_loaded) && ram.u8(0x800A3470U)!=0 &&
    ram.u8(0x800A346CU)!=0 && ram.u8(0x800FD7BDU)==0 &&
    active_pc_input(state,port).rumble_strength>0 && state.controller_ports.samples[port].connected &&
    (state.input_replay_loaded || (state.pc_devices[port]>=0 && state.pc_device_ready[port]));
}

void sync_pc_rumble(State& state) {
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  for(std::size_t port=0;port<4;++port) {
    if(state.pc_rumble_devices[port]!=state.pc_devices[port] || !pc_rumble_eligible(state,ram,port))
      state.pc_rumble_effects[port].clear();
    state.pc_rumble_devices[port]=state.pc_devices[port];
    const auto level=state.pc_rumble_effects[port].sample(state.vi_retraces,active_pc_input(state,port).rumble_strength);
    if(level!=state.pc_rumble_levels[port])state.gameplay_trace.event("pc-rumble-envelope",{
      state.vi_retraces,port,level,static_cast<std::uint32_t>(state.pc_devices[port]+3)});
    state.pc_rumble_levels[port]=level;
  }
  // Replays qualify guest requests and envelopes without touching host motors.
  auto desired=jfg::route_pc_rumble(state.pc_rumble_levels,state.pc_devices);
  if(!state.play_mode || state.input_replay_loaded)desired.fill(0);
  const auto set=xinput_api().set;
  if(!state.pc_rumble_output && set && std::any_of(desired.begin(),desired.end(),[](auto value){return value!=0;})) {
    state.pc_rumble_output=std::make_unique<jfg::PcRumbleOutput>(
      [set](unsigned device,std::uint16_t value) {
        XINPUT_VIBRATION vibration{value,value};
        return set(device,&vibration)==ERROR_SUCCESS;
      },[](std::uintptr_t token) {
        const auto root=reinterpret_cast<HWND>(token);
        return root && IsWindow(root) && GetForegroundWindow()==root && GetPropW(root,L"JfgSettingsOpen")==nullptr;
      });
  }
  if(state.pc_rumble_output) {
    const auto root=state.native_window?GetAncestor(static_cast<HWND>(state.native_window),GA_ROOT):nullptr;
    state.pc_rumble_output->publish(desired,reinterpret_cast<std::uintptr_t>(root));
  }
}

void request_pc_rumble(State& state,std::uint32_t target,const recomp_context& context) {
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  const auto player=static_cast<std::uint32_t>(context.r4);
  const auto third=target==0x8004B204U?0x0480001FU:target==0x8004B3D8U?0xAFA60020U:0xAFA5001CU;
  if(player>=4 || ram.u32(target)!=0x27BDFFE8U || ram.u32(target+4)!=0xAFBF0014U ||
     ram.u32(target+8)!=third || ram.u32(0x8004B070U)!=0x3C02800AU || ram.u32(0x8004B074U)!=0x90423470U ||
     ram.u32(0x800434D0U)!=0x3C028010U || ram.u32(0x800434D4U)!=0x00441021U ||
     ram.u32(0x800434D8U)!=0x9042B100U)return;
  const auto port=ram.u8(0x800FB100U+player);
  if(port>=4)return;
  if(target==0x8004B204U)state.pc_rumble_effects[port].clear();
  else if(pc_rumble_eligible(state,ram,port)) {
    const auto kind=target==0x8004B0F4U?jfg::PcRumbleRequest::start:
      target==0x8004B29CU?jfg::PcRumbleRequest::alter:jfg::PcRumbleRequest::maximum;
    state.pc_rumble_effects[port].request(kind,static_cast<std::int32_t>(context.r5),
      std::bit_cast<float>(static_cast<std::uint32_t>(context.r6)),state.vi_retraces);
  }
  sync_pc_rumble(state);
}

bool modern_controls_selected(const State &state,std::size_t port) {
  if(port!=0 || !state.modern_controls_active || !state.modern_controls[port].enabled || !state.rdram)return false;
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  return ram.u8(0x800A4FC4U)==0;
}
jfg::PcInputConfig active_pc_input(const State &state,std::size_t port) {
  return modern_controls_selected(state,port)?state.modern_controls[port].look:jfg::effective_pc_input(false,state.pc_input[port]);
}
// Modern gameplay owns its view heading before the guest chooses movement.
// Original mappings and old replay formats never enter this adapter.
bool modern_shoulder_current(const State &state,std::size_t port,std::uint32_t actor) {
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  return port<4 && state.pc_shoulder_actor==actor && state.pc_shoulders[port].active &&
    state.pc_shoulders[port].actor==actor && jfg::mod::pc_gameplay_active(ram,actor);
}
int dispatch(void *opaque,std::int32_t address,std::uint8_t *rdram,recomp_context *context);
bool modern_air_aim_active(const State &state,std::uint32_t actor,std::uint32_t control) {
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  if(!ram.valid(actor,0xA4U) || !ram.valid(control,0x5D0U) || ram.u32(actor+0x68U)!=control)return false;
  const auto port=ram.u8(control);
  return port==0U && state.modern_controls_active && state.pc_shoulders[port].active &&
    state.pc_shoulders[port].actor==actor && state.pc_shoulders[port].control==control &&
    (state.pc_frames[port].modern_flags&jfg::kModernActionsFlag) &&
    (state.pc_frames[port].pc_mode==3 || state.pc_frames[port].pc_mode==5) &&
    state.controller_ports.samples[port].connected && !state.pc_input_blocked &&
    jfg::mod::pc_gameplay_active(ram,actor) && !ram.u8(0x800FD7BDU) &&
    !jfg::mod::scripted_camera_active(ram,actor) &&
    jfg::pc_air_aim((ram.u32(0x800F6DA0U)&0x10U)!=0,ram.u8(control+0x568U),state.pc_shoulders[port].air_aim);
}
void modern_air_aim_update(State &state,std::uint32_t actor,std::uint32_t control,
                           const recomp_context &context) {
  if(!modern_air_aim_active(state,actor,control))return;
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  const auto stack=static_cast<std::uint32_t>(context.r29)-0x40U;
  if(!ram.valid(stack-0x600U,0x640U) || ram.u32(0x8003AABCU)!=0x27BDFFA8U)return;
  const auto port=ram.u8(control);auto &view=state.pc_shoulders[port];
  // Invoke only the shared native aiming/zoom update, never its grounded
  // locomotion handler. Its scratch frame lives below the suspended caller.
  hle::GuestMemory memory({state.rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
  auto aim=context;aim.r29=static_cast<std::int32_t>(stack);aim.r4=static_cast<std::int32_t>(actor);aim.r5=static_cast<std::int32_t>(control);aim.r6=aim.r7=0xE38;
  (void)memory.write_u32(stack+0x10U,static_cast<std::uint32_t>(-10922));
  (void)memory.write_u32(stack+0x14U,10922U);
  (void)memory.write_u32(stack+0x18U,stack+0x30U);(void)memory.write_u32(stack+0x1CU,stack+0x32U);
  (void)memory.write_u32(stack+0x20U,static_cast<std::uint32_t>(context.r6));
  const auto heading=ram.u16(control+0x11CU);
  ram.put16(control+0x11CU,static_cast<std::uint16_t>(view.yaw));
  ram.put16(control+0x1E2U,static_cast<std::uint16_t>(-view.pitch));
  dispatch(&state,static_cast<std::int32_t>(0x8003AABCU),state.rdram,&aim);
  ram.put16(control+0x11CU,heading); // Air steering retains its momentum heading.
  // Native grounded aim publishes these weapon directions after shared look.
  // Airborne animation remains the jump clip; its weapon uses the same view.
  for(auto offset:{0x1C6U,0x1CAU})ram.put16(control+offset,static_cast<std::uint16_t>(view.yaw));
  for(auto offset:{0x1C8U,0x1CCU,0x1D0U,0x1DEU})ram.put16(control+offset,static_cast<std::uint16_t>(-view.pitch));
  ram.put16(control+0x1CEU,0);ram.put16(control+0x1DCU,0);ram.put16(control+0x1E0U,0);
  if(aim.r2) {
    auto zoom=context;zoom.r29=static_cast<std::int32_t>(stack);zoom.r4=port;zoom.r5=ram.u32(control+0x1ECU);
    dispatch(&state,static_cast<std::int32_t>(0x80042438U),state.rdram,&zoom);
  }
  view.air_aim=view.aiming=true;
  state.gameplay_trace.event("pc-air-aim",{state.vi_retraces,actor,ram.u8(control+0x568U),
    ram.u16(control+0x11CU),static_cast<std::uint16_t>(view.yaw),ram.u16(control+0x1E2U)});
}
// The stock camera, weapon, reticle and fade APIs encode Aim in the locomotion byte.
// Adapt that legacy parameter only for those calls. Gravity, collisions,
// airborne animation and landing always see the real locomotion state.
class ModernAirAimPresentationScope {
  State &state_;std::uint32_t control_=0;std::uint16_t movement_=0,heading_=0;
public:
  ModernAirAimPresentationScope(State &state,std::uint32_t target,const recomp_context &context):state_(state) {
    if(target!=0x8002B418U && target!=0x80037550U && target!=0x80033BB8U && target!=0x00E00B30U)return;
    jfg::mod::Memory ram({state.rdram,kRdramSize});
    const auto actor=target==0x80033BB8U?state.pc_shoulder_actor:static_cast<std::uint32_t>(context.r4);
    const auto control=target==0x00E00B30U?(ram.valid(actor,0xA4U)?ram.u32(actor+0x68U):0U):
      static_cast<std::uint32_t>(target==0x80033BB8U?context.r4:context.r5);
    if(!modern_air_aim_active(state,actor,control))return;
    const auto port=ram.u8(control);control_=control;movement_=ram.u16(control+0x568U);heading_=ram.u16(control+0x11CU);
    ram.put16(control+0x568U,static_cast<std::uint16_t>(0xB00U|(movement_&0xFFU)));
    ram.put16(control+0x11CU,static_cast<std::uint16_t>(state.pc_shoulders[port].yaw));
    state.gameplay_trace.event("pc-air-aim-consumer",{state.vi_retraces,actor,target,static_cast<unsigned>(movement_>>8U)});
  }
  ~ModernAirAimPresentationScope() {
    if(!control_)return;
    jfg::mod::Memory ram({state_.rdram,kRdramSize});
    ram.put16(control_+0x568U,movement_);ram.put16(control_+0x11CU,heading_);
  }
};
class ModernShoulderScope {
  State &state_;
  std::uint32_t actor_=0,control_=0,update_ticks_=0;
  std::size_t port_=0;
public:
  ModernShoulderScope(State &state,std::uint32_t target,const recomp_context &context):state_(state) {
    if(target!=0x80032A48U)return;
    state.pc_shoulder_actor=0;
    jfg::mod::Memory ram({state.rdram,kRdramSize});
    const auto actor=static_cast<std::uint32_t>(context.r4);
    const auto control=ram.valid(actor,0xA4U)?ram.u32(actor+0x68U):0U;
    if(!ram.valid(control,0x5D0U))return;
    const auto port=ram.u8(control);
    if(port>=4)return;
    auto &view=state.pc_shoulders[port];
    const auto &frame=state.pc_frames[port];
    const auto movement=ram.u8(control+0x568U);
    if(port!=0 || !state.modern_controls_active || (state.pc_input_blocked && !view.active) ||
       !(frame.modern_flags&jfg::kModernActionsFlag) ||
       (frame.pc_mode!=3 && frame.pc_mode!=5) || !state.controller_ports.samples[port].connected ||
       (ram.u8(control+1U)&3U)>=3U || !jfg::pc_shoulder_state(movement) ||
       !jfg::mod::pc_gameplay_active(ram,actor) || ram.u8(0x800FD7BDU)!=0 ||
       jfg::mod::scripted_camera_active(ram,actor) ||
       ram.u32(target)!=0x27BDFF70U || ram.u32(target+4U)!=0xAFBF002CU) {view={};return;}
    actor_=actor;control_=control;port_=port;state.pc_shoulder_actor=actor;
    update_ticks_=static_cast<std::uint32_t>(context.r5);
    // The native landing-to-aim transition clears its pitch field. Retain
    // the airborne view when it reaches grounded aim on the following update.
    if(view.air_aim && (movement==10U || movement==11U) &&
       (ram.u32(0x800F6DA0U)&0x10U)!=0 && !state.pc_input_blocked)
      ram.put16(control+0x1E2U,static_cast<std::uint16_t>(-view.pitch));
    view.air_aim=jfg::pc_air_aim((ram.u32(0x800F6DA0U)&0x10U)!=0,movement,view.air_aim) && !state.pc_input_blocked;
    const bool aiming=movement==10U || movement==11U || view.air_aim;
    const auto camera=ram.u32(0x800F6DC0U);
    const int native_pitch=aiming?-ram.s16(control+0x1E2U):
      ram.valid(camera,0x4CU)?ram.s16(camera+2U):0;
    // A new area's camera may still be looking back at its entrance. The
    // native player owns entry facing; focus loss is not a new player/view.
    if(view.acquire(actor,control,ram.s16(control+0x11CU),native_pitch))
      state.gameplay_trace.event("pc-shoulder-acquire",{state.vi_retraces,actor,control,
        static_cast<std::uint16_t>(view.yaw),static_cast<std::uint16_t>(view.pitch)});
    view.movement_before_state=movement;
    view.movement_before_yaw=ram.s16(control+0x11CU);
    // Capture before the native action handler eases its own velocity. The
    // optional modern easing must start here, never from the already-eased value.
    view.movement_before_update={std::bit_cast<float>(ram.u32(control+0x10U)),
                                 std::bit_cast<float>(ram.u32(control+4U))};
    view.retrace=state.vi_retraces;view.aiming=aiming;
    if(!aiming && !state.pc_input_blocked) {
      auto look=jfg::PcLookDelta{frame.look_x,frame.look_y};
      if(frame.pc_mode==3) {
        if(state.pc_mouse_used[port])look={};
        state.pc_mouse_used[port]=true;
      } else look=state.pc_stick_motion[port].take(look,
        static_cast<float>(static_cast<std::uint32_t>(context.r5)),actor);
      const auto after=jfg::pc_direct_aim(view.yaw,view.pitch,look,-10922,10922);
      view.yaw=after.x;view.pitch=after.y;
    }
    // The original airborne handler owns momentum heading and air steering.
    // Body/view orientation is restored after physics consumes that heading.
    if(movement==3U)ram.put16(actor,ram.u16(control+0x11CU));
    else {
      view.movement_before_update=jfg::pc_velocity_in_view(view.movement_before_update,
        view.movement_before_yaw,view.yaw);
      view.movement_before_yaw=view.yaw;
      ram.put16(actor,static_cast<std::uint16_t>(view.yaw));
      ram.put16(control+0x11CU,static_cast<std::uint16_t>(view.yaw));
      ram.put16(control+0x10AU,0);
    }
  }
  ~ModernShoulderScope() {
    if(!actor_)return;
    jfg::mod::Memory ram({state_.rdram,kRdramSize});
    const auto &view=state_.pc_shoulders[port_];
    const auto camera=ram.u32(0x800F6DC0U);
    state_.gameplay_trace.event("pc-shoulder",{state_.vi_retraces,actor_,
      ram.u8(control_+0x568U),static_cast<std::uint16_t>(view.yaw),
      static_cast<std::uint16_t>(view.pitch),ram.u16(actor_),ram.u16(control_+0x11CU),
      ram.u32(actor_+0xCU),ram.u32(actor_+0x10U),ram.u32(actor_+0x14U),
      ram.u32(control_+4U),ram.u32(control_+0x10U),
      ram.valid(camera,0x4CU)?ram.u16(camera):0U,
      ram.valid(camera,0x4CU)?ram.u16(camera+2U):0U,update_ticks_});
    state_.pc_shoulder_actor=0;
  }
};
// Supply final ground velocity before the original animation update as well as
// physics. Animation and collision must consume the same local movement.
void modern_shoulder_physics(State &state,std::uint32_t target,const recomp_context &context) {
  const bool animation=target==0x00F05328U || target==0x01005120U || target==0x01104C50U;
  if(!animation && target!=0x00F05ED4U && target!=0x01005BB8U && target!=0x011050BCU)return;
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  const auto actor=static_cast<std::uint32_t>(context.r4),control=static_cast<std::uint32_t>(context.r5);
  if(!ram.valid(actor,0xA4U) || !ram.valid(control,0x5D0U) || ram.u32(actor+0x68U)!=control)return;
  const auto port=ram.u8(control);
  if(!modern_shoulder_current(state,port,actor))return;
  const auto movement=ram.u8(control+0x568U);
  if(!jfg::pc_shoulder_state(movement) || (animation && movement!=0U))return;
  auto &view=state.pc_shoulders[port];
  const bool landing_aim=view.air_aim && (movement==10U || movement==11U);
  view.air_aim=jfg::pc_air_aim((ram.u32(0x800F6DA0U)&0x10U)!=0,movement,view.air_aim) && !state.pc_input_blocked;
  const bool aiming=movement==10U || movement==11U || view.air_aim;
  if(aiming && (!view.aiming || landing_aim)) {
    // The stock run-to-aim transition clears pitch and copies the old body
    // orientation into the weapon camera. Seed it from the persistent view.
    for(auto offset:{0x1E2U,0x1D0U,0x1DEU,0x1C8U,0x1CCU})
      ram.put16(control+offset,static_cast<std::uint16_t>(-view.pitch));
    for(auto offset:{0x1C6U,0x1CAU})ram.put16(control+offset,static_cast<std::uint16_t>(view.yaw));
    ram.put16(control+0x1CEU,0);ram.put16(control+0x1DCU,0);ram.put16(control+0x1E0U,0);
    state.gameplay_trace.event("pc-shoulder-transition",{state.vi_retraces,actor,
      static_cast<std::uint16_t>(view.yaw),static_cast<std::uint16_t>(view.pitch)});
  }
  view.aiming=aiming;
  if(movement==3U) {
    if(view.movement_before_state!=3U) {
      const auto momentum=jfg::pc_native_airborne_motion(view.movement_before_update,
        view.movement_before_yaw,ram.u8(control+0x569U)!=0);
      hle::GuestMemory memory({state.rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
      (void)memory.write_u32(control+4U,std::bit_cast<std::uint32_t>(momentum.forward));
      (void)memory.write_u32(control+0x10U,0U);
      ram.put16(control+0x11CU,static_cast<std::uint16_t>(momentum.yaw));
      state.gameplay_trace.event("pc-air-takeoff",{state.vi_retraces,actor,
        static_cast<std::uint16_t>(momentum.yaw),std::bit_cast<std::uint32_t>(momentum.forward)});
      view.movement_before_state=3U;
    }
    modern_air_aim_update(state,actor,control,context);
    return;
  }
  ram.put16(actor,static_cast<std::uint16_t>(view.yaw));
  ram.put16(control+0x11CU,static_cast<std::uint16_t>(view.yaw));
  ram.put16(control+0x10AU,0);
  // Retain the guest's jump/roll/crouch momentum. Normal ground motion uses
  // camera-relative forward/back and lateral strafe without turning the body.
  if(movement==0U) {
    // Use the guest's already filtered/locked axes, not the raw host sample.
    // This retains its deadzone and its 65/16 normal running-speed limit.
    auto velocity=jfg::pc_shoulder_movement(
      static_cast<std::int32_t>(ram.u32(0x800F6DACU)),
      static_cast<std::int32_t>(ram.u32(0x800F6DB0U)));
    if(state.pc_frames[port].pc_mode==3 && state.modern_controls[port].mouse_original_acceleration)
      velocity=jfg::pc_original_acceleration(view.movement_before_update,velocity,
        std::bit_cast<float>(static_cast<std::uint32_t>(animation?context.r7:context.r6)));
    hle::GuestMemory memory({state.rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
    (void)memory.write_u32(control+4U,std::bit_cast<std::uint32_t>(velocity.forward));
    (void)memory.write_u32(control+0x10U,std::bit_cast<std::uint32_t>(velocity.side));
    // Native backward animation selection depends on this direction flag.
    // The original handler instead derives it from lock-on and turn intent.
    ram.put16(control+0x568U,static_cast<std::uint16_t>((movement<<8U)|(velocity.forward>0.0F?1U:0U)));
    // Discard only the bank generated by a body turn which modern controls
    // replace with the view heading. Terrain, damage and airborne poses remain.
    ram.put16(control+0x120U,0);
  }
}

// The native physics routine computes world velocity, collision and gravity
// using its own airborne heading. Only restore visual facing after it returns.
class ModernAirPhysicsScope {
  State& state_;std::uint32_t actor_=0;std::size_t port_=0;
public:
  ModernAirPhysicsScope(State& state,std::uint32_t target,const recomp_context& context):state_(state) {
    if(target!=0x00F05ED4U && target!=0x01005BB8U && target!=0x011050BCU)return;
    jfg::mod::Memory ram({state.rdram,kRdramSize});
    const auto actor=static_cast<std::uint32_t>(context.r4),control=static_cast<std::uint32_t>(context.r5);
    if(!ram.valid(actor,0xA4U) || !ram.valid(control,0x5D0U) || ram.u32(actor+0x68U)!=control ||
       ram.u8(control+0x568U)!=3U)return;
    const auto port=ram.u8(control);
    if(modern_shoulder_current(state,port,actor)){actor_=actor;port_=port;}
  }
  ~ModernAirPhysicsScope() {
    if(!actor_ || !modern_shoulder_current(state_,port_,actor_))return;
    jfg::mod::Memory ram({state_.rdram,kRdramSize});
    ram.put16(actor_,static_cast<std::uint16_t>(state_.pc_shoulders[port_].yaw));
  }
};

// Read the native animation remapping table from the active character overlay.
// Its first column identifies the ordinary clip behind an armed variant. Never
// infer humanoid sidestep IDs for Lupus, whose animation catalog is different.
unsigned modern_animation_family(State &state,unsigned character,unsigned requested) {
  if(character>2U)return UINT32_MAX;
  constexpr std::array<unsigned,3> offsets{0x8130U,0x7870U,0x62C8U};
  constexpr std::array<unsigned,3> counts{53U,51U,28U};
  if(requested>=counts[character])return UINT32_MAX;
  const unsigned section=15U+character;
  const auto base=section_addresses?static_cast<std::uint32_t>(section_addresses[section]):0U;
  const auto table=offsets[character];
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  const auto decode=[&](const auto &memory)->unsigned {
    if(!memory.valid(base,table+counts[character]*5U) ||
       memory.u8(base+table)!=0U || memory.u8(base+table+5U)!=1U ||
       memory.u8(base+table+15U)!=3U)return UINT32_MAX;
    return memory.u8(base+table+requested*5U);
  };
  if(ram.valid(base,table+counts[character]*5U))return decode(ram);
  JfgGeneratedSectionMetadata metadata{};
  if(!state.active_overlay_sections.contains(section) ||
     !jfg_generated_section_metadata(section,&metadata) || metadata.is_overlay!=1U ||
     base!=metadata.linked_vram || (base&3U))return UINT32_MAX;
  const std::uint64_t extent=std::uint64_t(metadata.text_size)+metadata.data_size+metadata.bss_size;
  const std::uint32_t offset=base-kKseg0;
  if(extent>0x10000U || extent<table+counts[character]*5U ||
     offset>kGuestAddressSpan || extent>kGuestAddressSpan-offset || (extent&3U))return UINT32_MAX;
  const jfg::mod::DialogueMemory memory(ram,base,
    std::span<const std::uint8_t>(state.rdram+offset,static_cast<std::size_t>(extent)));
  return decode(memory);
}

// Retain explicit nesting: generated overlay calls do not carry a reliable RA.
class ModernAnimationScope {
  State &state_;std::uint32_t previous_;
public:
  ModernAnimationScope(State &state,std::uint32_t target,recomp_context &context):state_(state),previous_(state.pc_animation_actor) {
    if(target==0x00F05328U || target==0x01005120U || target==0x01104C50U)
      state.pc_animation_actor=static_cast<std::uint32_t>(context.r4);
    if(target!=0x00F05180U && target!=0x01004F78U && target!=0x01104B84U)return;
    jfg::mod::Memory ram({state.rdram,kRdramSize});
    const auto actor=static_cast<std::uint32_t>(context.r4),control=static_cast<std::uint32_t>(context.r5);
    if(!ram.valid(actor,0xA4U) || !ram.valid(control,0x5D0U) || ram.u32(actor+0x68U)!=control ||
       actor!=state.pc_animation_actor || !modern_shoulder_current(state,ram.u8(control),actor))return;
    const auto requested=static_cast<std::uint32_t>(context.r6);
    const unsigned character=ram.u8(control+1U)&3U;
    const auto family=modern_animation_family(state,character,requested);
    if(ram.u8(control+0x568U)==0U)
      context.r6=jfg::pc_ground_animation(character,requested,family,
        {std::bit_cast<float>(ram.u32(control+0x10U)),std::bit_cast<float>(ram.u32(control+4U))});
    state.gameplay_trace.event("pc-locomotion",{state.vi_retraces,actor,ram.u8(control+0x568U),
      ram.u8(actor+0x3BU),static_cast<std::uint32_t>(context.r6),ram.u8(control+0x569U),
      ram.u32(control+4U),ram.u32(control+0x10U),ram.u16(control+0x120U),ram.u32(actor+0x28U),requested,family});
  }
  ~ModernAnimationScope(){state_.pc_animation_actor=previous_;}
};

// Normal ground aim retains the guest's position, collision, zoom and FOV.
// Its old pitch exaggeration and follow easing must not change the view angle
// when switching between third-person look and the aiming camera.
void modern_shoulder_aim_camera(State &state,std::uint32_t target,
                                std::uint32_t actor,std::uint32_t control) {
  if(target!=0x8002C118U)return;
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  if(!ram.valid(actor,0xA4U) || !ram.valid(control,0x5D0U) || ram.u32(actor+0x68U)!=control)return;
  const auto port=ram.u8(control);
  if(!modern_shoulder_current(state,port,actor) || !state.pc_shoulders[port].aiming)return;
  const auto camera=ram.u32(0x800F6DC0U);
  if(!ram.valid(camera,0x4CU))return;
  const auto &view=state.pc_shoulders[port];
  ram.put16(camera,static_cast<std::uint16_t>(jfg::pc_shoulder_camera_yaw(view.yaw)));
  ram.put16(camera+2U,static_cast<std::uint16_t>(view.pitch));
}

bool diagnostic_reticle_lines() {
  static const bool enabled=[] {
    char value[2]{};
    return GetEnvironmentVariableA("JFG_DIAGNOSTIC_RETICLE_LINES",value,2U)==1U && value[0]=='1';
  }();
  return enabled;
}
// Optional bounded trace of the original deferred software-line writer.
class DiagnosticLineOutputScope {
  State& state_; std::uint32_t address_=0;
  std::vector<std::uint8_t> before_;
public:
  DiagnosticLineOutputScope(State& state,std::uint32_t target):state_(state) {
    if(target!=0x8006E188U || !state.gameplay_trace || !diagnostic_reticle_lines())return;
    jfg::mod::Memory ram({state.rdram,kRdramSize});
    const auto address=ram.u32(0x800FECB0U);
    if(!ram.valid(address,320U*240U*2U))return;
    address_=address&0x1FFFFFFFU;
    before_.assign(state.rdram+address_,state.rdram+address_+320U*240U*2U);
    state.gameplay_trace.event("reticle-line-queue",{state.vi_retraces,address,
        ram.u32(0x80103B90U),ram.u32(0x80103B80U),ram.u32(0x80103B84U)});
  }
  ~DiagnosticLineOutputScope() {
    if(before_.empty())return;
    unsigned changed=0;
    for(std::size_t i=0;i<before_.size();i+=2U)
      changed+=before_[i]!=state_.rdram[address_+i] || before_[i+1U]!=state_.rdram[address_+i+1U];
    state_.gameplay_trace.event("reticle-line-output",{state_.vi_retraces,address_,changed});
  }
};

// Rendering-only private fixture. Exercise the original shuriken reticle
// drawer on a copied replay without granting a weapon or editing a save.
// The current weapon is restored before this draw call returns.
class DiagnosticReticleDrawScope {
  jfg::mod::Memory ram_;
  std::uint8_t *bytes_;
  std::uint32_t address_ = 0;
  std::uint8_t saved_ = 0;
public:
  DiagnosticReticleDrawScope(State &state,std::uint32_t target,const recomp_context &context):
      ram_({state.rdram,kRdramSize}), bytes_(state.rdram) {
    if (target != 0x00D004A8U || state.play_mode || !state.input_replay_loaded ||
        jfg::requested_capture_phase() < 0) return;
    static const bool enabled = [] {
      char value[2]{};
      return GetEnvironmentVariableA("JFG_DIAGNOSTIC_SHURIKEN_DRAW",value,2U)==1U && value[0]=='1';
    }();
    if (!enabled) return;
    const auto control=static_cast<std::uint32_t>(context.r4);
    const auto game=ram_.u32(0x800FD7D4U);
    if (!ram_.valid(control,0x5C4U) || !ram_.valid(game,0x538U) || ram_.u8(control)!=0) return;
    const auto character=ram_.u8(control+1U)&3U;
    if (character>=3U) return;
    address_=game+0x15CU+character*0x76U+0x70U;
    saved_=ram_.u8(address_);bytes_[(address_ & 0x1FFFFFFFU) ^ 3U]=9U;
    state.gameplay_trace.event("reticle-draw-fixture",{state.vi_retraces,control,address_,saved_,9U});
  }
  ~DiagnosticReticleDrawScope() { if(address_)bytes_[(address_ & 0x1FFFFFFFU) ^ 3U]=saved_; }
};

// Temporarily supply a canonical action table to the original player update.
// Both stock tables are restored before returning; menus and original mapping
// never see the override, and the saved Normal/Expert option is left intact.
class ModernGuestTableScope {
  hle::GuestMemory memory_;
  bool active_=false;
public:
  ModernGuestTableScope(State &state,std::uint32_t target,std::uint32_t actor):
      memory_({state.rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian) {
    if(target!=0x80032A48U || !state.modern_controls_active || state.pc_input_blocked ||
       !(state.pc_frames[0].modern_flags&jfg::kModernActionsFlag))return;
    jfg::mod::Memory ram({state.rdram,kRdramSize});
    const auto control=ram.valid(actor,0xA4U)?ram.u32(actor+0x68U):0U;
    if(!ram.valid(actor,0xA4U) || !ram.valid(control,0x5C4U) || ram.u8(control)!=0 ||
       !jfg::mod::pc_actions_active(ram,actor) || ram.u8(0x800FD7BDU)!=0 ||
       jfg::mod::scripted_camera_active(ram,actor) ||
       ram.u32(target)!=0x27BDFF70U || ram.u32(target+4U)!=0xAFBF002CU)return;
    for(std::size_t i=0;i<9;++i)
      if(ram.u32(0x800A18B4U+static_cast<std::uint32_t>(i*4))!=jfg::kNormalGuestTable[i] ||
         ram.u32(0x800A18D8U+static_cast<std::uint32_t>(i*4))!=jfg::kExpertGuestTable[i])return;
    active_=true;
    for(std::size_t i=0;i<9;++i) {
      (void)memory_.write_u32(0x800A18B4U+static_cast<std::uint32_t>(i*4),jfg::kModernGuestTable[i]);
      (void)memory_.write_u32(0x800A18D8U+static_cast<std::uint32_t>(i*4),jfg::kModernGuestTable[i]);
    }
    state.gameplay_trace.event("modern-actions",{state.vi_retraces,actor,state.pc_frames[0].modern_flags});
  }
  ~ModernGuestTableScope() {
    if(!active_)return;
    for(std::size_t i=0;i<9;++i) {
      (void)memory_.write_u32(0x800A18B4U+static_cast<std::uint32_t>(i*4),jfg::kNormalGuestTable[i]);
      (void)memory_.write_u32(0x800A18D8U+static_cast<std::uint32_t>(i*4),jfg::kExpertGuestTable[i]);
    }
  }
};
void reload_controller_ports(State &state) {
  const auto now = std::chrono::steady_clock::now();
  if (now - state.controller_config_check < std::chrono::milliseconds(250))
    return;
  state.controller_config_check = now;
  if (state.controller_config_path.empty())
    return;
  for (std::size_t p = 0; p < 4; ++p) {
    const auto modern_path=state.controller_config_path.parent_path() /
      (p==0?"modern-controls.ini":"modern-controls-"+std::to_string(p+1)+".ini");
    std::ifstream modern_file(modern_path,std::ios::binary|std::ios::ate);
    if(modern_file && modern_file.tellg()>0 && modern_file.tellg()<=8192) {
      modern_file.seekg(0);const std::string text{std::istreambuf_iterator<char>(modern_file),{}};
      auto candidate=state.modern_controls[p];
      if(jfg::parse_modern_controls(text,candidate) &&
         jfg::serialize_modern_controls(candidate)!=jfg::serialize_modern_controls(state.modern_controls[p])) {
        state.modern_controls[p]=candidate;state.modern_aim_latch[p].reset();
        state.modern_scope_zoom[p].reset();state.pc_device_release[p]={};state.pc_device_ready[p]=false;
        (void)state.controller_input_gate.blocked(true,state.controller_ports.samples);
      }
    } else if(!std::filesystem::exists(modern_path) && state.modern_controls[p].enabled) {
      state.modern_controls[p].enabled=false;state.modern_aim_latch[p].reset();state.modern_scope_zoom[p].reset();
      state.pc_device_release[p]={};state.pc_device_ready[p]=false;
      (void)state.controller_input_gate.blocked(true,state.controller_ports.samples);
    }
    const auto pc_path=state.controller_config_path.parent_path() /
      (p==0?"pc-input.ini":"pc-input-"+std::to_string(p+1)+".ini");
    std::ifstream pc_file(pc_path,std::ios::binary|std::ios::ate);
    if(pc_file && pc_file.tellg()>0 && pc_file.tellg()<=4096) {
      pc_file.seekg(0);const std::string text{std::istreambuf_iterator<char>(pc_file),{}};
      (void)jfg::parse_pc_input(text,state.pc_input[p]);
    }
    const auto path =
        p == 0 ? state.controller_config_path
               : state.controller_config_path.parent_path() /
                     ("controller-" + std::to_string(p + 1) + ".ini");
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file || file.tellg() <= 0 || file.tellg() > 4096)
      continue;
    file.seekg(0);
    const std::string text{std::istreambuf_iterator<char>(file), {}};
    jfg::ControllerMapping mapping;
    if (jfg::parse_controller_mapping(text, mapping)) {
      if(mapping.device==-4)mapping.device=state.controller_catalog.reserve(mapping.device_key);
      state.controller_ports.configure(p, mapping);
    }
  }
  // The selected style is the only opt-in. The obsolete profile switch is ignored.
  const bool pc_enabled=state.modern_controls[0].enabled;
  if(pc_enabled!=state.modern_controls_active) {
    state.modern_controls_active=pc_enabled;state.pc_aim.fill({});
    for(auto &latch:state.modern_aim_latch)latch.reset();
    for(auto &scope:state.modern_scope_zoom)scope.reset();
    state.pc_mouse.clear();state.pc_wheel.clear();state.pc_wheel_key=0;
    const bool was_captured=state.pc_mouse_captured.exchange(false);
    if(was_captured && state.play_mode && state.native_window) {
      ClipCursor(nullptr);SetCursor(LoadCursorW(nullptr,MAKEINTRESOURCEW(32512)));
    }
    (void)state.controller_input_gate.blocked(true,state.controller_ports.samples);
  }

}
jfg::ControllerPortSample sample_keyboard_port(const State &state,std::size_t port) {
  const auto down=[&](int key) {
    return key>=256?state.pc_wheel_key==key:host_key_down(state,key);
  };
  auto sample=jfg::map_pc_keyboard(jfg::effective_pc_input(false,state.pc_input[port]),down);
  return sample;
}
void sample_controller_ports(State &state,bool discover_sdl=true) {
  reload_controller_ports(state);
  state.pc_wheel_key=state.pc_wheel.sample();
  state.pc_aim.fill({});state.modern_live_flags.fill(0);state.modern_physical_neutral.fill(true);
  std::array<bool, jfg::kControllerDeviceCount> available{};
  std::array<jfg::StandardControllerSample, jfg::kControllerDeviceCount> hardware{};
  // Replay boot reaches this function before replay loading; defer SDK startup
  // until live polling so recordings never discover or open physical SDL pads.
  if(discover_sdl && !state.input_replay_loaded) {
    state.sdl_controllers.open(jfg::SdlControllerBackend::executable_directory());
    state.controller_catalog.update(state.sdl_controllers.poll());
    for(int device=4;device<static_cast<int>(jfg::kControllerDeviceCount);++device) {
      const auto* info=state.controller_catalog.get(device);
      available[static_cast<std::size_t>(device)]=info->connected && !info->session_only;
      hardware[static_cast<std::size_t>(device)]=info->sample;
    }
  }
  for (std::size_t p = 0; p < 4; ++p)
    available[p] = read_xinput_device(static_cast<int>(p), hardware[p]);
  const auto devices = state.controller_ports.devices(available);
  state.pc_devices=devices;
  jfg::mod::Memory pc_memory({state.rdram,kRdramSize});
  for (std::size_t p = 0; p < 4; ++p) {
    auto &sample = state.controller_ports.samples[p];
    sample = {};
    const int device = devices[p];
    if(modern_controls_selected(state,p) &&
       (device==-3 || (device>=0 && available[static_cast<std::size_t>(device)]))) {
      const auto &config=state.modern_controls[p];
      auto input=device==-3?jfg::sample_modern_keyboard(config,[&](int key) {
          if(state.guest_menu.kind!=jfg::GuestMenuKind::none && (key==VK_LBUTTON || key==VK_RBUTTON))return false;
          return key>=256?state.pc_wheel_key==key:host_key_down(state,key);
        }):jfg::sample_modern_pad(config,hardware[static_cast<std::size_t>(device)]);
      state.modern_physical_neutral[p]=input.physically_neutral;
      const bool boss=jfg::mod::pc_boss_gameplay_active(pc_memory,state.phase9_player_actor);
      const bool menu=pc_memory.u8(0x800FD7BDU)!=0 ||
        !(boss || jfg::mod::pc_gameplay_active(pc_memory,state.phase9_player_actor)) ||
        jfg::mod::scripted_camera_active(pc_memory,state.phase9_player_actor);
      state.modern_aim_latch[p].apply(input,config.aim_toggle,
        !menu && !state.pc_input_blocked && state.pc_device_ready[p]);
      sample=boss && !menu ? jfg::map_modern_boss_actions(input) :
        jfg::map_modern_actions(input,menu,state.guest_menu.kind==jfg::GuestMenuKind::pause_root &&
          state.guest_menu.ready && (state.guest_menu.enabled&0x100U)!=0);
      sample.buttons=state.menu_confirm[p].apply(menu?state.guest_menu:jfg::GuestMenuState{},sample.buttons);
      state.modern_live_flags[p]=jfg::modern_scope_flags(menu?0:input.held);
      if(device>=0 && !menu)state.pc_aim[p]=jfg::pc_stick_look(config.look,
        hardware[static_cast<std::size_t>(device)],config.movement_stick,(sample.buttons&0x10)!=0);
      continue;
    }
    if (device == -3) {
      sample = sample_keyboard_port(state,p);
      continue;
    }
    if (device < 0 || !available[static_cast<std::size_t>(device)])
      continue;
    const auto mapped=jfg::map_pc_controller(state.controller_ports.mappings[p],
        hardware[static_cast<std::size_t>(device)],jfg::effective_pc_input(false,state.pc_input[p]),false);
    sample = {mapped.buttons, static_cast<std::int8_t>(mapped.stick.x),
              static_cast<std::int8_t>(mapped.stick.y), true};

  }
  for(std::size_t p=0;p<4;++p) {
    auto &sample=state.controller_ports.samples[p];
    auto release_sample=sample;
    if(modern_controls_selected(state,p) && !state.modern_physical_neutral[p])release_sample.buttons|=1;
    state.pc_device_ready[p]=!state.pc_device_release[p].blocked(devices[p],release_sample,state.pc_aim[p]);
    if(!state.pc_device_ready[p]) {
      sample.buttons=0;sample.x=sample.y=0;state.pc_aim[p]={};state.modern_live_flags[p]=0;
      state.modern_aim_latch[p].reset();state.modern_scope_zoom[p].reset();
      state.pc_stick_motion[p].clear();state.pc_scope_motion[p].clear();state.modern_scope_sensitivity[p].clear();state.pc_orbits[p]={};state.pc_shoulders[p]={};
      if(devices[p]==-3){state.pc_mouse.clear();state.pc_wheel.clear();}
    }
  }
  state.controller_mapping = state.controller_ports.mappings[0];
  state.controller_mapping.device = devices[0];
}

jfg::mod::DialogueState navigation_dialogue(State &state) {
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  const auto base=section_addresses!=nullptr?static_cast<std::uint32_t>(section_addresses[7U]):0U;
  if(ram.valid(base,0x2C44U))return jfg::mod::read_dialogue(ram,base);
  JfgGeneratedSectionMetadata metadata{};
  if(!state.active_overlay_sections.contains(7U) ||
     !jfg_generated_section_metadata(7U,&metadata) || metadata.is_overlay!=1U ||
     base!=metadata.linked_vram || (base&3U))return {};
  const std::uint64_t extent=std::uint64_t(metadata.text_size)+metadata.data_size+metadata.bss_size;
  const std::uint32_t offset=base-kKseg0;
  if(extent<0x2C44U || extent>0x10000U || offset>kGuestAddressSpan ||
     extent>kGuestAddressSpan-offset || (extent&3U))return {};
  jfg::mod::DialogueMemory memory(ram,base,
      std::span<const std::uint8_t>(state.rdram+offset,static_cast<std::size_t>(extent)));
  return jfg::mod::read_dialogue(memory,base);
}

class SlowHostWork {
 public:
  SlowHostWork(State &state, const char *label, bool selected = true)
      : state_(state), label_(label),
        enabled_(selected && state.timing_trace.is_open()),
        start_(enabled_ ? std::chrono::steady_clock::now()
                        : std::chrono::steady_clock::time_point{}) {}
  ~SlowHostWork() {
    if (!enabled_) return;
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start_).count();
    if (us >= 4000)
      state_.timing_trace << label_ << '\t' << state_.vi_frames << '\t'
                          << state_.vi_retraces << '\t' << us << '\n';
  }
 private:
  State &state_;
  const char *label_;
  bool enabled_;
  std::chrono::steady_clock::time_point start_;
};




// Each overlay view is qualified against the generated section and its mapped
// extent. Canonical overlay globals may reside outside physical 4 MiB RDRAM.
template<class Decode>
jfg::GuestMenuState decode_menu_overlay(State &state,unsigned section,std::uint32_t need,Decode decode) {
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  const auto base=section_addresses?static_cast<std::uint32_t>(section_addresses[section]):0U;
  if(!state.active_overlay_sections.contains(section))return {};
  if(ram.valid(base,need))return decode(ram,base);
  JfgGeneratedSectionMetadata metadata{};
  if(!jfg_generated_section_metadata(section,&metadata)||metadata.is_overlay!=1U||base!=metadata.linked_vram||(base&3U))return {};
  const std::uint64_t extent=std::uint64_t(metadata.text_size)+metadata.data_size+metadata.bss_size;
  const std::uint32_t offset=base-kKseg0;
  if(extent<need||extent>0x10000U||offset>kGuestAddressSpan||extent>kGuestAddressSpan-offset||(extent&3U))return {};
  const jfg::mod::DialogueMemory memory(ram,base,std::span<const std::uint8_t>(state.rdram+offset,static_cast<std::size_t>(extent)));
  return decode(memory,base);
}
jfg::GuestMenuState read_pause_menu(State &state) {
  using Kind=jfg::GuestMenuKind;
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  if(ram.u8(0x800FD7BDU)==0 || ram.u8(0x800A51B0U)!=16)return {};
  return decode_menu_overlay(state,19U,0x1400U,[&](const auto &manager,std::uint32_t mb)->jfg::GuestMenuState {
    if(manager.u32(mb)!=0x27BDFFE8U)return {};
    const auto screen=manager.u32(mb+0x13F4U);
    state.gameplay_trace.event("pause-page",{state.vi_retraces,screen,manager.u32(mb+0x13F8U),manager.u32(mb+0x13C4U)});
    if(manager.u32(mb+0x13F8U)!=0 || manager.u32(mb+0x13C4U)!=2)return {};
    if(screen==6) {
      return decode_menu_overlay(state,36U,0x2060U,[&](const auto& m,std::uint32_t base)->jfg::GuestMenuState {
        if(m.u32(base+0x1A0U)!=0x27BDFED8U || m.u32(base+0x2038U)!=0 || m.u32(base+0x2050U)!=0)return {};
        const auto page=m.u32(base+0x1F34U),selected=m.u32(base+0x2030U);
        state.gameplay_trace.event("options-page",{state.vi_retraces,page,selected,m.u32(base+0x2040U)});
        if(page>9 || selected>=8)return {};
        unsigned enabled=255U;
        if(ram.u32(0x800FF384U)&0x40000000U)enabled&=~8U; // unavailable screen-size option
        return {page==8?Kind::options:Kind::option_detail,static_cast<int>(selected),enabled,true,base,page};
      });
    }
    // These native pages expose B as Back; the modern Confirm adapter also
    // dismisses informational pages. Map keeps its native travel/confirm action.
    if(screen==2)return {Kind::pause_map,0,0,true,mb,screen};
    if(screen==3 || screen==4 || screen==5 || screen==7 || screen==9)
      return {Kind::pause_info,0,0,true,mb,screen};
    if(screen!=1 && screen!=8)return {};
    return decode_menu_overlay(state,18U,0x6D18U,[&](const auto &m,std::uint32_t base)->jfg::GuestMenuState {
      if(m.u32(base+0x2464U)!=0x27BDFE60U || m.u32(base+0x6680U)!=0)return {};
      if(m.u8(base+0x67B4U)!=0)
        return {Kind::pause_character,0,7U,m.u8(base+0x6D07U)==255 && m.u8(base+0x6D08U)==0,base};
      if(m.u8(base+0x6D05U)!=255 || m.u8(base+0x6D04U)>=8)return {};
      const auto player=ram.u32(0x800FD7D4U);
      if(!ram.valid(player,0xC0U))return {};
      const auto flag=[&](unsigned n){return (ram.u8(player+0x30U+(n>>3U))&(1U<<(n&7U)))!=0;};
      unsigned enabled=255U;
      if(flag(0x64U))enabled&=~2U;
      if(!flag(0x5FU))enabled&=~128U;
      if(m.s16(base+0x6D14U)==0 && m.s16(base+0x6D16U)==0)enabled|=256U;
      return {Kind::pause_root,m.u8(base+0x6D04U),enabled,true,base};
    });
  });
}

jfg::GuestMenuState read_guest_menu(State &state) noexcept {
  using Kind=jfg::GuestMenuKind;
  jfg::mod::Memory ram({state.rdram,kRdramSize});
  const auto mode=ram.u8(0x800A51B0U);
  if(mode==16U) { try{return read_pause_menu(state);}catch(...){return {};} }
  if(mode!=3U && mode!=24U) return {};
  const unsigned section=mode==3U?61U:41U;
  const auto base=section_addresses?static_cast<std::uint32_t>(section_addresses[section]):0U;
  const auto decode=[&](const auto &m)->jfg::GuestMenuState {
    try {
      if(mode==3U) {
        if(!m.valid(base,0xEF0U)||m.u32(base+0x140U)!=0x27BDFF30U) return {};
        const auto stage=m.u32(base+0xECCU);
        if(stage==1U) return {Kind::title,0,1U,true,base};
        const int selected=ram.u8(0x800A5060U);
        if(stage==2U && selected<3 && m.u32(base+0xEDCU)==255U)
          return {Kind::start,selected,ram.u32(0x800FB10CU)>=2U?7U:5U,true,base};
      } else {
        if(!m.valid(base,0x4F0CU)||m.u32(base+0x178U)!=0x27BDFE50U||
           m.u32(base+0x2CB0U)==0U) return {};
        const unsigned stage=m.u8(base+0x2FA0U);
        const int selected=m.u8(base+0x2F5FU);
        if(m.s16(base+0x2F5CU)<=0x7A00 || m.s16(base+0x4EF8U)!=255) return {};
        if(stage==0U && selected<6) return {Kind::saves,selected,63U,true,base};
        if(stage==4U && selected<4) {
          // The native save action page keeps the Play icon first.
          // Copy/erase and their confirmation pages remain keyboard-driven.
          const float slide=std::bit_cast<float>(m.u32(base+0x2FA4U));
          if(slide>=21.5f && slide<=22.1f)
            return {Kind::save_actions,selected,1U,true,base};
        }
      }
    } catch(...) {}
    return {};
  };
  if(ram.valid(base,mode==3U?0xEF0U:0x4F0CU)) return decode(ram);
  JfgGeneratedSectionMetadata metadata{};
  if(!state.active_overlay_sections.contains(section) ||
     !jfg_generated_section_metadata(section,&metadata) || metadata.is_overlay!=1U ||
     base!=metadata.linked_vram || (base&3U)) return {};
  const std::uint64_t extent=std::uint64_t(metadata.text_size)+metadata.data_size+metadata.bss_size;
  const std::uint32_t offset=base-kKseg0;
  if(extent>0x10000U || extent<0xEF0U || offset>kGuestAddressSpan ||
     extent>kGuestAddressSpan-offset || (extent&3U)) return {};
  const jfg::mod::DialogueMemory memory(ram,base,
      std::span<const std::uint8_t>(state.rdram+offset,static_cast<std::size_t>(extent)));
  return decode(memory);
}

// Called only at the controller-poll boundary, after checking the live native
// menu and its overlay identity. Preview/render/activation remain native.
bool select_guest_menu(State& state, jfg::GuestMenuKind kind, int target) noexcept {
  const auto live=read_guest_menu(state);
  if(!live.ready || live.kind!=kind || live.identity!=state.guest_menu.identity ||
     target<0 || target>=(kind==jfg::GuestMenuKind::pause_root?9:kind==jfg::GuestMenuKind::options?8:6) || !(live.enabled&(1U<<target)))return false;
  if(kind==jfg::GuestMenuKind::pause_root && target==8)return true;
  std::uint32_t address=0;
  if(kind==jfg::GuestMenuKind::start)address=0x800A5060U;
  else if(kind==jfg::GuestMenuKind::saves || kind==jfg::GuestMenuKind::save_actions)
    address=live.identity+0x2F5FU;
  else if(kind==jfg::GuestMenuKind::pause_root)address=live.identity+0x6D04U;
  else if(kind==jfg::GuestMenuKind::options)address=live.identity+0x2030U;
  if(!address)return false;
  // Relocated overlay addresses can be below KSEG0. Use the same wrapping
  // host offset as read_guest_menu's validated overlay view, not physical RAM
  // aliasing. The live decoder above has proved this exact field is mapped.
  const std::uint32_t offset=address-kKseg0;
  if(offset>=kGuestAddressSpan)return false;
  if(kind==jfg::GuestMenuKind::options) {
    for(unsigned i=0;i<4;++i)state.rdram[(offset+i)^3U]=i==3?static_cast<std::uint8_t>(target):0;
  } else state.rdram[offset^3U]=static_cast<std::uint8_t>(target);
  if(target!=live.selected && (kind==jfg::GuestMenuKind::pause_root || kind==jfg::GuestMenuKind::options))
    state.menu_selection_feedback=live;
  if(kind==jfg::GuestMenuKind::pause_root && target!=live.selected) {
    for(unsigned i=0;i<4;++i) {
      state.rdram[((live.identity+0x6D00U-kKseg0+i)^3U)]=i==3?255:0;
      state.rdram[((live.identity+0x6CFCU-kKseg0+i)^3U)]=0;
    }
  }
  state.gameplay_trace.event("menu-select",{state.vi_retraces,static_cast<unsigned>(kind),
    static_cast<unsigned>(live.selected),static_cast<unsigned>(target)});
  state.guest_menu.selected=target;
  return true;
}

[[noreturn]] void fail_closed_dispatch(State &state, std::string_view category,
                                      std::string_view operation, std::uint32_t guest_target);

void sample_live_controller(State &state) noexcept {
  {
    const auto input=state.window_input.consume_frame(state.host_keys,state.host_key_presses,state.exit_requested);
    state.menu_pointer_frame=input.pointer;
    if(input.focus_lost){state.menu_pointer.reset();for(auto &effect:state.pc_rumble_effects)effect.clear();state.pc_mouse.clear();state.pc_wheel.clear();state.pc_input_blocked=true;(void)state.controller_input_gate.blocked(true,state.controller_ports.samples);}
    if(state.pc_mouse_captured.load())state.pc_mouse.add(input.x,input.y);
    state.pc_wheel.add(input.wheel);
  }
  const SlowHostWork timing(state, "host-controller");
  constexpr std::uint16_t kButtonA = 0x8000U;
  std::uint16_t buttons = 0U;
  int stick_x = 0;
  int stick_y = 0;
  std::uint8_t menu_kind=0,menu_target=0;
  const auto previous_menu=state.guest_menu;
  state.guest_menu=read_guest_menu(state);
  if(!(state.guest_menu==previous_menu))
    state.gameplay_trace.event("menu-state",{state.vi_retraces,static_cast<unsigned>(state.guest_menu.kind),
      static_cast<unsigned>(state.guest_menu.selected),state.guest_menu.enabled,state.guest_menu.identity});
  if(!state.input_replay_loaded)sample_controller_ports(state);
  else reload_controller_ports(state);
  state.pc_frames.fill({});state.pc_mouse_used.fill(false);
  bool connected = state.controller_ports.samples[0].connected;
  if (state.input_replay_loaded) {
    jfg::ControllerReplaySample replay;
    if (state.input_replay_gameplay_time && state.controller_samples >= state.replay_gameplay_first_poll) {
      const auto events = std::span<const jfg::InputReplayEvent>(state.input_replay.events()).subspan(state.replay_gameplay_first_poll);
      if (!state.replay_gameplay_clock_ready) {
        jfg::mod::Memory memory({state.rdram,kRdramSize});
        if (!jfg::mod::pc_gameplay_active(memory,state.phase9_player_actor) ||
            jfg::mod::scripted_camera_active(memory,state.phase9_player_actor))
          fail_closed_dispatch(state,"recorded-input","gameplay-landmark-mismatch",0U);
        state.replay_gameplay_origin_vi = state.vi_retraces;
        state.replay_gameplay_clock_ready = true;
        state.gameplay_trace.event("replay-landmark", {state.vi_retraces,events.front().first_retrace,
            state.controller_samples,state.phase9_player_actor,memory.u32(0x800FB114U)});
      }
      const auto elapsed_vi = state.vi_retraces-state.replay_gameplay_origin_vi;
      if (events.front().first_retrace > UINT64_MAX-elapsed_vi)
        fail_closed_dispatch(state,"recorded-input","gameplay-time-overflow",0U);
      const auto recorded_vi = events.front().first_retrace + elapsed_vi;
      const auto sample = state.recorded_input_timeline.sample(recorded_vi,events);
      if (!sample) fail_closed_dispatch(state,"recorded-input","unrepresentable-gameplay-poll",0U);
      replay = *sample;
    } else if (state.input_replay_recorded_time) {
      const auto sample = state.recorded_input_timeline.sample(state.vi_retraces,state.input_replay.events());
      if (!sample) fail_closed_dispatch(state,"recorded-input","unrepresentable-poll",0U);
      replay = *sample;
    } else {
      replay = state.input_replay_by_poll
          ? state.input_replay.sample_by_poll(state.controller_samples)
          : state.input_replay.sample_at(state.vi_retraces);
    }
    if(state.modern_controls_active)state.pc_frames[0]=replay;
    menu_kind=replay.menu_kind;menu_target=replay.menu_target;
    connected = replay.connected;
    buttons = replay.buttons;
    stick_x = replay.stick_x;
    stick_y = replay.stick_y;
  } else {
    const auto sample=state.controller_ports.samples[0];
    buttons=sample.buttons;stick_x=sample.x;stick_y=sample.y;
  }

  const HWND root=state.native_window ? GetAncestor(static_cast<HWND>(state.native_window),GA_ROOT) : nullptr;
  const bool settings_capture=state.play_mode && root && GetPropW(root,L"JfgSettingsOpen")!=nullptr;
  const bool ui_capture=state.play_mode && root && (GetForegroundWindow()!=root || settings_capture);
  auto release_samples=state.controller_ports.samples;
  for(std::size_t p=0;p<4;++p) {
    if(modern_controls_selected(state,p)) {
      release_samples[p].buttons=state.modern_physical_neutral[p]?0:1;
      if(state.modern_physical_neutral[p])release_samples[p].x=release_samples[p].y=0;
    }
    if(state.pc_aim[p].x || state.pc_aim[p].y)release_samples[p].buttons|=1;
  }
  const bool block_physical=state.controller_input_gate.blocked(ui_capture,release_samples);
  const bool replay_suspended=state.input_replay_loaded &&
    state.vi_retraces>=state.diagnostic_input_suspend_begin &&
    state.vi_retraces<state.diagnostic_input_suspend_end;
  const bool previously_blocked=state.pc_input_blocked;
  state.pc_input_blocked=jfg::pc_input_motion_blocked(block_physical,state.input_replay_loaded,replay_suspended);
  if(previously_blocked!=state.pc_input_blocked)
    state.gameplay_trace.event("pc-input-suspended",{state.vi_retraces,state.pc_input_blocked?1U:0U});
  jfg::mod::Memory pc_memory({state.rdram,kRdramSize});
  bool want_mouse=false;
  for(std::size_t p=0;p<4;++p)
    want_mouse|=state.pc_devices[p]==-3 && active_pc_input(state,p).mouse_aim && state.controller_ports.samples[p].connected &&
      (active_pc_input(state,p).modern || (state.controller_ports.samples[p].buttons&0x10)!=0);
  want_mouse=want_mouse && state.modern_controls_active && state.pc_raw_available && !block_physical && !state.input_replay_loaded && (state.input_record_path.empty() || state.pc_record_extended) &&
    jfg::mod::pc_actions_active(pc_memory,state.phase9_player_actor) && pc_memory.u8(0x800FD7BDU)==0 &&
    !jfg::mod::scripted_camera_active(pc_memory,state.phase9_player_actor);
  if(want_mouse && state.native_window) {
    const HWND window=static_cast<HWND>(state.native_window);RECT rect{};GetClientRect(window,&rect);
    MapWindowPoints(window,nullptr,reinterpret_cast<POINT*>(&rect),2);ClipCursor(&rect);SetCursor(nullptr);
  } else {
    if(state.pc_mouse_captured){ClipCursor(nullptr);SetCursor(LoadCursorW(nullptr,MAKEINTRESOURCEW(32512)));}
    state.pc_mouse.clear();
  }
  state.pc_mouse_captured=want_mouse;
  if(state.pc_input_blocked){state.pc_aim.fill({});state.pc_wheel.clear();state.pc_mouse.clear();for(auto &motion:state.pc_stick_motion)motion.clear();for(auto &motion:state.pc_scope_motion)motion.clear();for(auto &motion:state.modern_scope_sensitivity)motion.clear();}
  if(!state.modern_controls_active || pc_memory.u8(0x800FD7BDU)!=0 || !jfg::mod::pc_actions_active(pc_memory,state.phase9_player_actor) ||
     jfg::mod::scripted_camera_active(pc_memory,state.phase9_player_actor)) {
    state.pc_orbits.fill({});state.pc_shoulders.fill({});for(auto &motion:state.pc_stick_motion)motion.clear();for(auto &motion:state.pc_scope_motion)motion.clear();for(auto &motion:state.modern_scope_sensitivity)motion.clear();
  } else if(!jfg::mod::pc_gameplay_active(pc_memory,state.phase9_player_actor)) {
    // Boss aiming still needs its fractional look accumulator. Relinquish
    // only free-camera ownership while its native fixed camera is in control.
    state.pc_orbits.fill({});state.pc_shoulders.fill({});
  }
  if(state.modern_controls_active && !state.input_replay_loaded &&
     (state.input_record_path.empty() || state.pc_record_extended)) {
    for(std::size_t p=0;p<4;++p) {
      auto &frame=state.pc_frames[p];const auto &config=active_pc_input(state,p);
      frame.modern_flags=state.modern_live_flags[p];
      if(block_physical) {
        // Keep view identity while discarding input. The release gate below
        // still neutralizes buttons and movement until physical controls rest.
        if(modern_controls_selected(state,p) && state.controller_ports.samples[p].connected) {
          frame.modern_flags=jfg::kModernActionsFlag;
          frame.pc_mode=state.pc_devices[p]==-3?3:5;
        }
        continue;
      }
      if(state.pc_devices[p]==-3 && config.mouse_aim && state.pc_mouse_captured) {
        frame.pc_mode=config.modern?3:1;
        jfg::PcLookDelta motion;
        if(config.modern)motion=state.pc_mouse.take_angles(config,(state.controller_ports.samples[p].buttons&0x10)!=0);
        else {const auto raw=state.pc_mouse.take(config);motion={raw.x,raw.y};}
        frame.look_x=static_cast<std::int16_t>(motion.x);frame.look_y=static_cast<std::int16_t>(motion.y);
      } else if(state.pc_devices[p]>=0 && config.dual_stick) {
        frame.pc_mode=config.modern?5:2;frame.look_x=static_cast<std::int16_t>(state.pc_aim[p].x);frame.look_y=static_cast<std::int16_t>(state.pc_aim[p].y);
      }
    }
  }
  if((block_physical && !state.input_replay_loaded) || replay_suspended) {
    for(auto& frame:state.pc_frames) {
      frame.look_x=frame.look_y=0;
      frame.modern_flags&=jfg::kModernActionsFlag;
    }
    buttons=0;stick_x=stick_y=0;
    for(auto& sample:state.controller_ports.samples){sample.buttons=0;sample.x=sample.y=0;}
  }
  const bool pointer_probe=state.menu_pointer_replay.active() && state.input_replay_loaded && !state.play_mode;
  auto pointer=pointer_probe?state.menu_pointer_replay.sample(state.vi_retraces):state.menu_pointer_frame;
  if(pointer.left_press||pointer.right_press){pointer.x=pointer.press_x;pointer.y=pointer.press_y;}
  // v1 fixtures store logical menu coordinates; v2 stores actual client
  // pixels and exercises the same displayed-VI conversion as a live mouse.
  const bool logical_probe=pointer_probe && !state.menu_pointer_replay.pixel_coordinates();
  const auto pointer_presentation=pointer_probe?
    jfg::vi_presentation_size(state.vi_mode_horizontal_start,state.rt64_vi.vertical_start):state.window_presentation;
  const auto point=jfg::menu_pointer_point(pointer,
    logical_probe?4U:(pointer_presentation.valid()?pointer_presentation.width:4U),
    logical_probe?3U:(pointer_presentation.valid()?pointer_presentation.height:3U));
  const bool pointer_allowed=pointer_probe || (!state.input_replay_loaded && !block_physical &&
    modern_controls_selected(state,0) && state.pc_devices[0]==-3 && connected);
  const auto pointer_packet=state.menu_pointer.sample(state.guest_menu,point,pointer.moved,
    pointer.left_press,pointer.right_press,pointer_allowed,buttons!=0 || stick_x!=0 || stick_y!=0);
  if(pointer_packet.buttons || pointer_packet.target>=0 || pointer_packet.x || pointer_packet.y) {
    buttons=pointer_packet.buttons;stick_x=pointer_packet.x;stick_y=pointer_packet.y;
    if(pointer_packet.target>=0) {
      menu_kind=static_cast<std::uint8_t>(state.guest_menu.kind);
      menu_target=static_cast<std::uint8_t>(pointer_packet.target+1);
    }
    state.gameplay_trace.event("menu-pointer",{state.vi_retraces,static_cast<unsigned>(state.guest_menu.kind),
      static_cast<unsigned>(state.guest_menu.selected),buttons,menu_target});
  }
  if(menu_kind && !select_guest_menu(state,static_cast<jfg::GuestMenuKind>(menu_kind),int(menu_target)-1)) {
    // Stale/wrong-screen replay events must not activate an unrelated selection.
    menu_kind=menu_target=0;buttons=0;
  }
  // Map-driven automation may run while its tool window has focus. Settings
  // still suppress synthesized inputs below, as well as physical controller input.
  if (state.navigation_mod.enabled) {
    const bool escape = host_key_down(state, VK_ESCAPE) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    const bool manual = buttons != 0U || stick_x != 0 || stick_y != 0 || escape;
    // Retain manual cancellation across room resets and between map exports.
    if (manual) ++state.navigation_mod.manual_inputs;
    if (escape) state.navigation_mod.pilot.stop("manual_takeover");
    const auto ai = state.navigation_mod.pilot.sample(manual, state.input_replay_loaded, true);
    if (!manual && !state.input_replay_loaded && state.navigation_mod.pilot.active()) {
      buttons = ai.buttons; stick_x = ai.x; stick_y = ai.y;
    }
    if (state.controller_samples % 6U == 0U) {
      jfg::mod::Memory memory({state.rdram,kRdramSize});
      const auto dialogue=navigation_dialogue(state);
      if(dialogue.known&&dialogue.active) {
        state.navigation_mod.pilot.stop("dialogue_active");
        if(!manual&&!state.input_replay_loaded){buttons=0;stick_x=stick_y=0;}
      }
      std::ifstream request(state.navigation_output/L"ai-dialogue.txt",std::ios::binary|std::ios::ate);
      if(request&&request.tellg()<=256) {
        request.seekg(0);
        const auto now=std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
        const int input=state.navigation_mod.dialogue_input.command(request,dialogue,manual,
          state.input_replay_loaded,memory.u32(0x800FB114U),state.navigation_mod.generation,
          state.navigation_mod.manual_inputs,now);
        if(input!=0){buttons=input==1?kButtonA:0;stick_x=0;stick_y=input==2?60:input==3?-60:0;}
      }
    }
    if (state.controller_samples % 6U == 0U) {
      const auto path = state.navigation_output / L"ai-confirm.txt";
      std::ifstream command(path, std::ios::binary | std::ios::ate);
      if (command && command.tellg() <= 256) {
        command.seekg(0);
        jfg::mod::Memory memory({state.rdram, kRdramSize});
        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (state.navigation_mod.confirmation.command(command,
              jfg::mod::transition_confirmation(memory), manual, state.input_replay_loaded,
              memory.u32(0x800FB114U), state.navigation_mod.generation,
              state.navigation_mod.manual_inputs, now)) {
          buttons = kButtonA; stick_x = stick_y = 0;
        }
      }
    }
  }
  if(settings_capture && !state.input_replay_loaded) {
    buttons=0;stick_x=stick_y=0;
  }
  state.controller_ports.samples[0]={buttons,static_cast<std::int8_t>(stick_x),static_cast<std::int8_t>(stick_y),connected};
  for(std::size_t p=0;p<4;++p) {
    if(block_physical && !state.input_replay_loaded) {
      state.pc_frames[p].modern_flags=0;state.modern_aim_latch[p].cancel(false);
    }
    if(!(state.pc_frames[p].modern_flags&jfg::kModernActionsFlag)) {
      state.modern_scope_zoom[p].reset();state.modern_scope_sensitivity[p].clear();continue;
    }
    const auto actor=state.phase9_player_actor;
    const auto control=pc_memory.valid(actor,0xA4U)?pc_memory.u32(actor+0x68U):0U;
    const auto game=pc_memory.u32(0x800FD7D4U);
    const auto character=pc_memory.valid(control,0x5C4U)?pc_memory.u8(control+1U)&3U:3U;
    const bool scoped=p==0 && state.controller_ports.samples[p].connected &&
      (state.controller_ports.samples[p].buttons&0x10)!=0 &&
      jfg::mod::pc_actions_active(pc_memory,actor) && pc_memory.u8(0x800FD7BDU)==0 &&
      !jfg::mod::scripted_camera_active(pc_memory,actor) &&
      character<3U && pc_memory.valid(game,0x538U) &&
      pc_memory.u8(game+0x15CU+character*0x76U+0x70U)==7U;
    if(!state.input_replay_loaded) {
      auto &frame=state.pc_frames[p];const auto &config=state.modern_controls[p];
      const int sensitivity=frame.pc_mode==3?config.mouse_scope_sensitivity:config.pad_scope_sensitivity;
      const auto look=state.modern_scope_sensitivity[p].take({frame.look_x,frame.look_y},
        scoped && (jfg::modern_scope_sensitivity_active(true,pc_memory.u8(control+0x568U)) ||
          jfg::pc_air_aim(true,pc_memory.u8(control+0x568U),state.pc_shoulders[p].air_aim)) &&
        (frame.pc_mode==3 || frame.pc_mode==5),sensitivity,
        (std::uint64_t{actor}<<8U)|frame.pc_mode);
      frame.look_x=static_cast<std::int16_t>(look.x);frame.look_y=static_cast<std::int16_t>(look.y);
    } else state.modern_scope_sensitivity[p].clear();
    state.modern_scope_zoom[p].observe(state.pc_frames[p].modern_flags,scoped,actor);
  }
  // Even a short aim tap relinquishes free-camera orientation. Reacquire from
  // the guest aim camera when it ends, rather than snapping to a stale orbit.
  for(std::size_t p=0;p<4;++p)if(state.controller_ports.samples[p].buttons&0x10)state.pc_orbits[p]={};
  if(state.input_replay_loaded) for(std::size_t p=1;p<4;++p)state.controller_ports.samples[p]={};
  if (connected != state.latched_controller_connected) {
    if (connected)
      ++state.controller_reconnects;
    else
      ++state.controller_disconnects;
  }
  state.latched_controller_connected = connected;
  state.latched_controller_buttons = buttons;
  state.latched_controller_stick_x = static_cast<std::int8_t>(stick_x);
  state.latched_controller_stick_y = static_cast<std::int8_t>(stick_y);
  sync_pc_rumble(state);
  trace_phase9_event(state, "input-poll", state.controller_samples);
  // Record the effective packet and separate look channel, including an
  // explicitly requested replay-to-recording conversion for qualification.
  if (state.input_record.is_open()) {
    state.input_record << state.vi_retraces << ',' << state.vi_retraces + 1U
                       << ',' << (connected ? 1 : 0) << ',' << std::hex
                       << buttons << std::dec << ',' << stick_x << ','
                       << stick_y;
    if(state.pc_record_extended) {
      const auto &pc=state.pc_frames[0];
      state.input_record << ',' << int(pc.pc_mode) << ',' << pc.look_x << ',' << pc.look_y << ',' << int(pc.modern_flags) << ',' << int(menu_kind) << ',' << int(menu_target);
    }
    state.input_record << '\n';
  }
  if (state.phase95_poll_trace.is_open()) {
    hle::GuestMemory memory({state.rdram, kRdramSize},
                            hle::GuestMemory::Layout::native_word_big_endian);
    std::uint32_t level = 0U;
    std::uint32_t rng = 0U;
    (void)memory.read_u32(0x800FB114U, level);
    (void)memory.read_u32(0x800A33E4U, rng);
    state.phase95_poll_trace
        << state.controller_samples << '\t' << state.vi_retraces << '\t'
        << state.vi_frames << '\t'
        << static_cast<unsigned>(state.rdram[0x000A51B0U ^ 3U]) << '\t'
        << level << '\t' << rng << '\t' << buttons << '\t'
        << stick_x << '\t' << stick_y << '\n';
  }
  ++state.controller_samples;
  state.gameplay_trace.event("input", {state.vi_retraces, state.controller_samples,
      connected ? 1U : 0U, buttons, static_cast<std::uint8_t>(stick_x),
      static_cast<std::uint8_t>(stick_y)});
  if (buttons != 0U || stick_x != 0 || stick_y != 0)
    ++state.non_neutral_controller_samples;
  if (!GetParent(static_cast<HWND>(state.native_window)) && !state.input_replay_loaded && !state.navigation_mod.enabled && host_key_down(state, VK_ESCAPE))
    state.exit_requested = true;
  state.host_key_presses.fill(false);
}

bool service_host_audio(State &state, const bool pace) {
  if (state.mmio_trace == nullptr || state.rdram == nullptr ||
      state.mmio_trace->ai_submission_overflows != 0U)
    return false;
  while (state.mmio_trace->ai_submission_read <
         state.mmio_trace->ai_submission_write) {
    const std::size_t slot = static_cast<std::size_t>(
        state.mmio_trace->ai_submission_read %
        MmioTrace::kAiSubmissionCapacity);
    const MmioTrace::AiSubmission submission =
        state.mmio_trace->ai_submissions[slot];
    if (state.mmio_trace->trace_audio_payloads) {
      state.gameplay_trace.event("audio-payload", {state.mmio_trace->ai_submission_read,
          submission.address,submission.length,submission.submitted_count,state.cpu_count,
          submission.payload_hash,state.mmio_trace->audio_payload_hash(submission.address,submission.length)});
    }
    if (!state.host_audio.queue(state.rdram, submission.address,
                                submission.length,
                                submission.dac_rate))
      return false;
    ++state.mmio_trace->ai_submission_read;
  }
  // Interactive mode is already paced by the VI/window clock.  Waiting on
  // the SDL queue as well creates a second, drifting clock and visible frame
  // hitches whenever a graphics frame arrives late.  Headless probes have no
  // window clock, so retain audio throttling there unless a diagnostic replay
  // explicitly uses the same VI clock as interactive playback.
  state.host_audio.service(
      pace, pace && !state.play_mode && !state.fast_replay &&
                !state.realtime_replay &&
                !(state.original_os_host_pacing && state.original_os_host_anchor_ready));
  state.audio_device_initialized = state.host_audio.initialized();
  state.audio_device_started = state.host_audio.started();
  state.audio_device_frequency = state.host_audio.frequency();
  state.audio_device_buffers = state.host_audio.submitted_buffers();
  state.audio_device_queued_bytes = state.host_audio.queued_bytes();
  state.audio_device_consumed_bytes = state.host_audio.consumed_bytes();
  state.audio_device_consumed_ms =
      state.host_audio.consumed_milliseconds();
  state.audio_device_underruns = state.host_audio.underruns();
  state.audio_device_overruns = state.host_audio.overruns();
  state.mmio_trace->ai_emulated_remaining_length =
      state.host_audio.current_buffer_remaining();
  return true;
}

void service_live_window(State &state) {
  {
    const auto input=state.window_input.consume_frame(state.host_keys,state.host_key_presses,state.exit_requested,false);
    state.menu_pointer_frame=input.pointer;
    if(input.focus_lost){state.menu_pointer.reset();for(auto &effect:state.pc_rumble_effects)effect.clear();state.pc_mouse.clear();state.pc_wheel.clear();state.pc_input_blocked=true;(void)state.controller_input_gate.blocked(true,state.controller_ports.samples);}
    if(state.pc_mouse_captured.load())state.pc_mouse.add(input.x,input.y);
    state.pc_wheel.add(input.wheel);
  }
  const SlowHostWork window_service_timing(state,"host-window-service");
  const auto frame_start = std::chrono::steady_clock::now();
  if (state.host_frame_start_initialized) {
    const auto interval = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            frame_start - state.host_frame_start)
            .count());
    state.host_frame_interval_max_us =
        (std::max)(state.host_frame_interval_max_us, interval);
    state.host_frames_over_25ms += interval > 25000U ? 1U : 0U;
    state.host_frames_over_50ms += interval > 50000U ? 1U : 0U;
    state.host_frames_over_100ms += interval > 100000U ? 1U : 0U;
    if (state.timing_trace) {
      state.timing_trace << "frame\t" << state.vi_frames << '\t'
                         << state.vi_retraces << '\t' << interval << '\t'
                         << state.presented_frames << '\t'
                         << state.pending_graphics_tasks.size() << '\n';
    }
  }
  state.host_frame_start = frame_start;
  state.host_frame_start_initialized = true;
  const auto presentation = jfg::vi_presentation_size(
      state.vi_mode_horizontal_start, state.rt64_vi.vertical_start);
  // Pointer coordinates follow the actual displayed VI, including widescreen.
  // The launcher canvas follows the selected video mode instead: a temporary
  // cinematic crop must not resize the canvas and zoom away its black bars.
  if(presentation.valid())state.window_presentation=presentation;
  jfg::mod::Memory video_memory({state.rdram,kRdramSize});
  const auto resolution=video_memory.u8(0x800FECA8U);
  const bool widescreen=resolution<12U && (resolution&1U)!=0;
  if(state.native_window && widescreen!=state.window_widescreen) {
    state.window_widescreen=widescreen;
    PostMessageW(static_cast<HWND>(state.native_window),LiveRt64Window::kAspectMessage,
      320U,widescreen?180:240);
    state.gameplay_trace.event("display-canvas",{state.vi_retraces,320U,widescreen?180U:240U});
  }
  const auto frontend = state.native_window ? GetAncestor(static_cast<HWND>(state.native_window), GA_ROOT) : nullptr;
  bool frontend_paused = false;
  while (!state.exit_requested && frontend && IsWindow(frontend) &&
         GetPropW(frontend, L"JfgFrontendPause") != nullptr) {
    if (!frontend_paused) {
      if (state.diagnostic_host_clock &&
          !state.host_count_clock.pause(std::chrono::steady_clock::now()))
        fail_closed_dispatch(state,"native-clock","pause",0U);
      state.gameplay_trace.event("host-pause", {state.vi_retraces,
          state.diagnostic_host_clock ?
              state.host_count_clock.sample(std::chrono::steady_clock::now()).value_or(UINT64_MAX) : state.cpu_count});
      state.host_audio.frontend_pause(true);
      SetPropW(frontend,L"JfgFrontendPaused",reinterpret_cast<HANDLE>(1));
      frontend_paused = true;
    }
    {
    const auto input=state.window_input.consume_frame(state.host_keys,state.host_key_presses,state.exit_requested);
    state.menu_pointer.reset();state.menu_pointer_frame={};
    if(input.focus_lost){state.menu_pointer.reset();for(auto &effect:state.pc_rumble_effects)effect.clear();state.pc_mouse.clear();state.pc_wheel.clear();state.pc_input_blocked=true;(void)state.controller_input_gate.blocked(true,state.controller_ports.samples);}
    if(state.pc_mouse_captured.load())state.pc_mouse.add(input.x,input.y);
    state.pc_wheel.add(input.wheel);
  }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  if (frontend_paused) {
    RemovePropW(frontend,L"JfgFrontendPaused");
    if (state.diagnostic_host_clock &&
        !state.host_count_clock.resume(std::chrono::steady_clock::now()))
      fail_closed_dispatch(state,"native-clock","resume",0U);
    state.gameplay_trace.event("host-resume", {state.vi_retraces,
        state.diagnostic_host_clock ?
            state.host_count_clock.sample(std::chrono::steady_clock::now()).value_or(UINT64_MAX) : state.cpu_count});
    state.host_audio.frontend_pause(false);
    state.host_frame_deadline_initialized = false;
    state.host_frame_start_initialized = false;
    state.original_os_host_anchor_ready = false;
  }
  if (state.original_os_host_pacing) return;
  if (!state.host_frame_deadline_initialized) {
    state.host_frame_deadline = std::chrono::steady_clock::now();
    state.host_frame_deadline_initialized = true;
  }
  state.host_frame_deadline += std::chrono::microseconds(16667);
  const auto deadline_now = std::chrono::steady_clock::now();
  if (state.host_frame_deadline > deadline_now) {
    std::this_thread::sleep_until(state.host_frame_deadline);
  } else if (deadline_now - state.host_frame_deadline >
             std::chrono::microseconds(66668)) {
    // Do not run an unbounded catch-up burst after shader compilation, window
    // movement, or another long host stall.  Resume from the current VI edge.
    state.host_frame_deadline = deadline_now;
  }
}

bool write_live_frame_capture(State &state) {
  if (state.frame_capture_path.empty())
    return true;
  if (state.rt64_shell == nullptr)
    return false;
  auto frame = state.rt64_shell->last_presented_frame();
  if (!frame.valid() && !state.play_mode && state.presented_frames != 0U) {
    // A bounded probe can end more than two VIs after its last presentation.
    // Read back the already selected display buffer at the stop boundary;
    // do not run more guest instructions merely to obtain a diagnostic image.
    state.last_rt64_error = state.rt64_shell->present(true);
    if (state.last_rt64_error != jfg::Rt64ShellError::none)
      return false;
    frame = state.rt64_shell->last_presented_frame();
    state.gameplay_trace.event("final-frame-capture", {state.vi_retraces,
        state.controller_samples, state.presented_frames});
  }
  if (!frame.valid())
    return false;
  std::ofstream stream(state.frame_capture_path, std::ios::binary);
  if (!stream)
    return false;
  stream << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
  for (std::size_t y = 0U; y < frame.height; ++y) {
    const std::size_t row = y * frame.row_pitch_bytes;
    for (std::size_t x = 0U; x < frame.width; ++x) {
      const std::size_t pixel = row + x * 4U;
      const std::array<char, 3U> rgb = {
          static_cast<char>(
              std::to_integer<unsigned char>(frame.bgra8[pixel + 2U])),
          static_cast<char>(
              std::to_integer<unsigned char>(frame.bgra8[pixel + 1U])),
          static_cast<char>(
              std::to_integer<unsigned char>(frame.bgra8[pixel]))};
      stream.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
    }
  }
  state.frame_captured = static_cast<bool>(stream);
  return state.frame_captured;
}

bool write_logical_rdram_capture(const State &state,
                                 const std::filesystem::path &path) {
  std::ofstream stream(path, std::ios::binary);
  if (!stream)
    return false;
  std::array<char, 4096U> logical_bytes{};
  for (std::size_t base = 0U; base < kRdramSize;
       base += logical_bytes.size()) {
    for (std::size_t index = 0U; index < logical_bytes.size(); ++index) {
      logical_bytes[index] =
          static_cast<char>(state.rdram[(base + index) ^ 3U]);
    }
    stream.write(logical_bytes.data(),
                 static_cast<std::streamsize>(logical_bytes.size()));
  }
  return static_cast<bool>(stream);
}

bool write_private_rdram_capture(const State &state) {
  if (state.rdram_capture_path.empty())
    return true;
  return write_logical_rdram_capture(state, state.rdram_capture_path);
}

bool write_private_rt64_snapshot(
    const State &state, const std::span<const std::byte> snapshot) {
  if (state.rdram_capture_path.empty())
    return true;
  std::ofstream stream(state.rdram_capture_path + ".rt64",
                       std::ios::binary);
  if (!stream || snapshot.empty() || snapshot.size() % 4U != 0U)
    return false;
  std::array<char, 4096U> logical_bytes{};
  for (std::size_t base = 0U; base < snapshot.size();
       base += logical_bytes.size()) {
    const std::size_t chunk =
        (std::min)(logical_bytes.size(), snapshot.size() - base);
    for (std::size_t index = 0U; index < chunk; ++index) {
      logical_bytes[index] = static_cast<char>(
          std::to_integer<std::uint8_t>(snapshot[(base + index) ^ 3U]));
    }
    stream.write(logical_bytes.data(), static_cast<std::streamsize>(chunk));
  }
  return static_cast<bool>(stream);
}

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
bool write_mod_export(State &state, const std::filesystem::path &path, const std::string &text) {
  const auto copy = std::make_shared<const std::string>(text);
  if (path.filename() == L"mesh.json") {
    state.navigation_mesh_export = copy;
    return true;
  }
  jfg::AsyncSnapshots::Batch files;
  if (state.navigation_mesh_export)
    files.push_back({state.navigation_output/L"mesh.json",state.navigation_mesh_export});
  files.push_back({path,copy});
  return state.snapshot_writer.submit(1U,std::move(files));
}

void update_navigation_mod(State &state) {
  if(!state.navigation_mod.observing())return;
  auto &mod=state.navigation_mod;
  mod.dialogue=navigation_dialogue(state);
  jfg::mod::Memory memory({state.rdram,kRdramSize});
  try {
    const auto list=jfg::mod::actors(memory);
    if (mod.updates % 6U == 0U) {
      std::ifstream settings(state.navigation_output / L"mods.txt", std::ios::binary | std::ios::ate);
      if (settings && settings.tellg() <= 128) { settings.seekg(0); (void)mod.configure_mods(settings); }
      std::ifstream warp(state.navigation_output / L"warp-exit.txt", std::ios::binary | std::ios::ate);
      if (warp && warp.tellg() <= 256) {
        warp.seekg(0);
        const auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if(mod.warp_to_exit(warp,memory,list,stamp) && state.gameplay_trace)
          state.gameplay_trace.event("exit-warp",{state.vi_retraces,mod.generation,
            mod.warp_nonce,memory.u32(0x800FB114U),mod.player,
            memory.u32(memory.u32(mod.player+0x68U)+0xF4U)});
      }
    }
    bool player_present=false;
    for(const auto &actor:list) {
      if(actor.address==mod.player)player_present=true;
      (void)mod.clear_enemy(memory,actor.address);
    }
    if(!player_present)mod.player=0;
    else (void)mod.full_health(memory,mod.player);
    ++mod.updates;
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto ai_level = memory.u32(0x800FB114U);
    if (mod.enabled && mod.updates % 6U == 0U) {
      const auto command_path = state.navigation_output / L"ai-command.txt";
      std::ifstream command(command_path, std::ios::binary | std::ios::ate);
      if (command && command.tellg() <= 32768) { command.seekg(0); (void)mod.pilot.command(command, ai_level, mod.generation, now); }
    }
    if (mod.enabled && mod.player != 0U) {
      const auto p = jfg::mod::position(memory, mod.player);
      const auto camera = jfg::mod::control_camera(memory);
      mod.pilot.camera_heading(camera.known, camera.yaw);
      mod.pilot.tick({p.x,p.y,p.z}, jfg::mod::gameplay_active(memory,mod.player),ai_level,mod.generation,now);
      const auto movement=jfg::mod::player_motion(memory,mod.player);
      if(movement.known && (movement.state==6U||movement.state==12U) &&
         mod.pilot.active() && std::string_view(mod.pilot.state).starts_with("jump_"))
        mod.pilot.stop("jump_unexpected_grab_state");
      // Diagnostic rows are immutable copies; the bounded worker never reads
      // guest memory. Slow storage must not stall simulation or audio.
      if(jfg::mod::gameplay_active(memory,mod.player)) {
        std::ostringstream trace;
        trace<<mod.updates<<','<<now<<','<<ai_level<<','<<p.x<<','<<p.y<<','<<p.z
             <<','<<movement.known<<','<<movement.state<<','<<movement.animation
             <<','<<movement.animation_frame<<','<<memory.u32(0x800F6DA0U)<<'\n';
        (void)state.movement_trace.append(trace.str());
      }
      if(std::string_view(mod.pilot.state).starts_with("jump_") && mod.pilot.active()) {
        std::ostringstream trace;const auto input=mod.pilot.observed_input();
        trace<<mod.updates<<','<<now<<','<<ai_level<<','<<p.x<<','<<p.y<<','<<p.z
             <<','<<input.x<<','<<input.y<<','<<input.buttons<<','<<mod.pilot.state
             <<','<<camera.yaw<<','<<memory.s16(mod.player)<<'\n';
        (void)state.jump_trace.append(trace.str());
      }
    } else mod.pilot.stop("player_unavailable");
    if(mod.updates%6U!=0U)return;
    const auto track=memory.u32(0x800A0D60U), level=memory.u32(0x800FB114U);
    bool mesh_ready=mod.player!=0U && track==mod.last_track && level==mod.last_level;
    if(mod.player!=0U && !mesh_ready) {
      try {
        const auto mesh=jfg::mod::decode_mesh(memory,track);
        std::ostringstream output;jfg::mod::write_mesh(output,mesh,level,mod.generation);
        mesh_ready=write_mod_export(state, state.navigation_output/L"mesh.json",output.str());
        if(mesh_ready){mod.last_track=track;mod.last_level=level;}
      }catch(const std::runtime_error &){++mod.invalid;}
    }
    std::ostringstream output;mod.write_state(output,memory,list,level,mesh_ready);
    if(!write_mod_export(state, state.navigation_output/L"live.json",output.str()))++mod.invalid;
  }catch(const std::runtime_error &){++mod.invalid;}
}
#endif

void trace_gameplay_state(State &state, bool update) {
  if (!state.gameplay_trace || state.rdram == nullptr) return;
  if (!update && state.gameplay_trace_vi == state.vi_retraces) return;
  if (update) ++state.gameplay_trace_updates;
  else state.gameplay_trace_vi = state.vi_retraces;
  hle::GuestMemory memory({state.rdram, kRdramSize},
                          hle::GuestMemory::Layout::native_word_big_endian);
  std::uint32_t level = 0, actors = 0, count = 0;
  (void)memory.read_u32(0x800FB114U, level);
  (void)memory.read_u32(0x800F2CA4U, actors);
  (void)memory.read_u32(0x800F2CA8U, count);
  state.gameplay_trace.event(update ? "update" : "tick", {
      state.vi_retraces, state.controller_samples, state.gameplay_trace_updates,
      state.rdram[0xA51B0U ^ 3U], level, count, state.presented_frames,
      state.graphics_tasks, state.pending_graphics_tasks.size(), state.dispatch_calls,
      state.audio_device_queued_bytes, state.audio_device_consumed_bytes,
      state.audio_device_underruns, state.audio_device_overruns});
  if (!update || !state.gameplay_trace_actors) return;
  if (count > 256U) {
    state.gameplay_trace.event("actor-table-invalid", {state.vi_retraces, actors, count});
    return;
  }
  for (std::uint32_t index = 0; index < 256U; ++index) {
    std::uint32_t actor = 0, header = 0;
    if (index >= count || !memory.read_u32(actors + index * 4U, actor) || actor == 0U) {
      if (state.gameplay_actor_seen[index])
        state.gameplay_trace.event("actor-removed", {state.vi_retraces, state.gameplay_trace_updates, index});
      state.gameplay_actor_seen[index] = false;
      continue;
    }
    // All actor types, sampled after the game update, only when state changes.
    // Payload: identity/name, actor[0..0x7f], properties[0..0x1f], control[0..0x1f].
    std::array<std::uint64_t, 56> row{};
    row[0] = state.vi_retraces; row[1] = state.gameplay_trace_updates;
    row[2] = index; row[3] = actor;
    (void)memory.read_u32(actor + 0x40U, header);
    for (std::uint32_t n = 0; n < 4U; ++n) {
      std::uint32_t word = 0;
      row[4U + n] = memory.read_u32(header + 4U + n * 4U, word) ? word : UINT32_MAX;
    }
    for (std::uint32_t n = 0; n < 32U; ++n) {
      std::uint32_t word = 0;
      row[8U + n] = memory.read_u32(actor + n * 4U, word) ? word : UINT32_MAX;
    }
    for (std::uint32_t block = 0; block < 2U; ++block) {
      std::uint32_t pointer = 0;
      (void)memory.read_u32(actor + (block == 0U ? 0x4CU : 0x68U), pointer);
      for (std::uint32_t n = 0; n < 8U; ++n) {
        std::uint32_t word = 0;
        row[40U + block * 8U + n] = pointer != 0U && memory.read_u32(pointer + n * 4U, word) ? word : UINT32_MAX;
      }
    }
    const auto &previous = state.gameplay_actor_previous[index];
    if (!state.gameplay_actor_seen[index] ||
        !std::equal(row.begin() + 2, row.end(), previous.begin() + 2)) {
      state.gameplay_trace.event("actor", row);
      state.gameplay_actor_previous[index] = row;
      state.gameplay_actor_seen[index] = true;
    }
  }
}

void write_actor_timing_trace(State &state) {
  if (!state.actor_timing_trace || !state.timing_trace.is_open() ||
      state.rdram == nullptr ||
      state.last_actor_trace_retrace == state.vi_retraces)
    return;
  state.last_actor_trace_retrace = state.vi_retraces;
  hle::GuestMemory actor_memory(
      {state.rdram, kRdramSize},
      hle::GuestMemory::Layout::native_word_big_endian);
  std::uint32_t actor_list = 0U;
  std::uint32_t actor_count = 0U;
  if (!actor_memory.read_u32(0x800F2CA4U, actor_list) ||
      !actor_memory.read_u32(0x800F2CA8U, actor_count) || actor_count > 256U)
    return;
  for (std::uint32_t index = 0U; index < actor_count; ++index) {
    std::uint32_t actor = 0U;
    std::uint32_t header = 0U;
    std::uint32_t name0 = 0U;
    std::uint32_t name1 = 0U;
    if (!actor_memory.read_u32(actor_list + index * 4U, actor) ||
        actor == 0U || !actor_memory.read_u32(actor + 0x40U, header) ||
        header == 0U || !actor_memory.read_u32(header + 4U, name0) ||
        !actor_memory.read_u32(header + 8U, name1) ||
        name0 != 0x616E696DU || name1 != 0x426C7565U)
      continue;
    state.timing_trace << "actor\t" << state.vi_frames << '\t'
                       << state.vi_retraces << "\t0\t" << std::hex
                       << actor << '\t' << name0 << name1 << "\t0\t";
    for (std::uint32_t offset = 0U; offset < 0x200U; offset += 4U) {
      std::uint32_t word = 0U;
      if (offset != 0U)
        state.timing_trace << ':';
      if (actor_memory.read_u32(actor + offset, word))
        state.timing_trace << word;
      else
        state.timing_trace << "invalid";
    }
    state.timing_trace << std::dec << '\n';
  }
}

std::optional<std::string> hash_logical_rdram_region(
    const State &state, const std::uint32_t guest_address,
    const std::size_t length) {
  if (state.rdram == nullptr)
    return std::nullopt;
  const std::uint64_t physical = guest_address & 0x1FFFFFFFU;
  if (physical > kRdramSize || length > kRdramSize - physical)
    return std::nullopt;
  jfg::testkernel::Sha256 hasher;
  std::array<std::uint8_t, 1024U> logical{};
  std::size_t consumed = 0U;
  while (consumed < length) {
    const std::size_t chunk =
        (std::min)(logical.size(), length - consumed);
    for (std::size_t index = 0U; index < chunk; ++index)
      logical[index] = state.rdram[
          (static_cast<std::size_t>(physical) + consumed + index) ^ 3U];
    hasher.update_bytes(logical.data(), chunk);
    consumed += chunk;
  }
  return jfg::testkernel::hex_digest(hasher.finish());
}

void write_retrace_semantic_hash(State &state, std::ostream *update_output = nullptr,
                                std::ostream *poll_output = nullptr) {
  if ((update_output != nullptr && poll_output != nullptr) ||
      (update_output == nullptr && poll_output == nullptr &&
       !state.retrace_hash_trace.is_open()) ||
      state.rdram == nullptr)
    return;
  hle::GuestMemory memory({state.rdram, kRdramSize},
                          hle::GuestMemory::Layout::native_word_big_endian);
  constexpr std::uint32_t kObjectListAddress = 0x800F2CA4U;
  constexpr std::uint32_t kObjectCountAddress = 0x800F2CA8U;
  constexpr std::uint32_t kRngSeedAddress = 0x800A33E4U;
  constexpr std::uint32_t kFrontModeAddress = 0x800A51B0U;
  constexpr std::uint32_t kGameGlobalsAddress = 0x800A4FCCU;
  constexpr std::size_t kGameGlobalsBytes = 0x90U;
  constexpr std::uint32_t kCameraStateAddress = 0x801045B0U;
  constexpr std::size_t kCameraStateBytes = 0x158U;
  constexpr std::size_t kActorStateBytes = 0x200U;
  constexpr std::uint32_t kMaximumActors = 256U;

  std::uint32_t actor_list = 0U;
  std::uint32_t actor_count = 0U;
  std::uint32_t rng_seed = 0U;
  (void)memory.read_u32(kObjectListAddress, actor_list);
  (void)memory.read_u32(kObjectCountAddress, actor_count);
  (void)memory.read_u32(kRngSeedAddress, rng_seed);
  const unsigned front_mode = state.rdram[(kFrontModeAddress - kKseg0) ^ 3U];
  const bool actor_table_valid = actor_count <= kMaximumActors &&
      (actor_count == 0U ||
       hash_logical_rdram_region(state, actor_list,
                                 actor_count * sizeof(std::uint32_t))
           .has_value());
  const auto actor_table_hash = actor_table_valid
      ? hash_logical_rdram_region(state, actor_list,
                                  actor_count * sizeof(std::uint32_t))
      : std::nullopt;
  const auto player_hash = state.phase9_player_actor == 0U
      ? std::nullopt
      : hash_logical_rdram_region(state, state.phase9_player_actor,
                                  kActorStateBytes);
  const auto globals_hash = hash_logical_rdram_region(
      state, kGameGlobalsAddress, kGameGlobalsBytes);
  const auto camera_hash = hash_logical_rdram_region(
      state, kCameraStateAddress, kCameraStateBytes);

  auto &stream = poll_output != nullptr ? *poll_output
      : update_output != nullptr ? *update_output : state.retrace_hash_trace;
  stream << "{\"kind\":\""
         << (poll_output != nullptr ? "jfg-phase9-poll-hash"
             : update_output == nullptr ? "jfg-phase9-retrace-hash"
                                        : "jfg-phase9-update-hash")
         << "\",\"schema\":1,\""
         << (poll_output != nullptr ? "poll"
             : update_output == nullptr ? "retrace" : "update") << "\":"
         << (poll_output != nullptr ? state.controller_samples - 1U
             : update_output == nullptr ? state.vi_retraces
                                        : state.completed_game_updates)
         << ",\"front_mode\":" << front_mode
         << ",\"rng_seed\":\"0x" << std::hex << std::setw(8)
         << std::setfill('0') << rng_seed
         << "\",\"player_actor\":\"0x" << std::setw(8)
         << state.phase9_player_actor << "\"" << std::dec
         << ",\"player_sha256\":";
  if (player_hash.has_value())
    stream << '"' << *player_hash << '"';
  else
    stream << "null";
  stream << ",\"actor_list\":\"0x" << std::hex << std::setw(8)
         << std::setfill('0') << actor_list << "\"" << std::dec
         << ",\"actor_count\":" << actor_count
         << ",\"actor_table_sha256\":";
  if (actor_table_hash.has_value())
    stream << '"' << *actor_table_hash << '"';
  else
    stream << "null";
  stream << ",\"globals_sha256\":";
  if (globals_hash.has_value())
    stream << '"' << *globals_hash << '"';
  else
    stream << "null";
  stream << ",\"camera_sha256\":";
  if (camera_hash.has_value())
    stream << '"' << *camera_hash << '"';
  else
    stream << "null";
  stream << ",\"actors\":[";
  bool first = true;
  if (actor_table_valid) {
    for (std::uint32_t index = 0U; index < actor_count; ++index) {
      std::uint32_t actor = 0U;
      (void)memory.read_u32(actor_list + index * sizeof(std::uint32_t), actor);
      const auto digest = actor == 0U
          ? std::nullopt
          : hash_logical_rdram_region(state, actor, kActorStateBytes);
      if (!first)
        stream << ',';
      first = false;
      stream << "{\"index\":" << index << ",\"address\":\"0x"
             << std::hex << std::setw(8) << std::setfill('0') << actor
             << "\",\"sha256\":" << std::dec;
      if (digest.has_value())
        stream << '"' << *digest << '"';
      else
        stream << "null";
      stream << '}';
    }
  }
  stream << ']';
  if (poll_output != nullptr) {
    std::uint32_t level_word = 0U;
    (void)memory.read_u32(0x800FB114U, level_word);
    stream << ",\"level_word\":" << level_word
           << ",\"connected\":" << (state.latched_controller_connected ? 1 : 0)
           << ",\"buttons\":" << state.latched_controller_buttons
           << ",\"stick_x\":" << static_cast<int>(state.latched_controller_stick_x)
           << ",\"stick_y\":" << static_cast<int>(state.latched_controller_stick_y)
           << ",\"update_counter_valid\":"
           << (state.update_hash_trace.is_open() ? "true" : "false")
           << ",\"completed_updates\":";
    if (state.update_hash_trace.is_open())
      stream << state.completed_game_updates;
    else
      stream << "null";
    stream << ",\"vi_retraces\":" << state.vi_retraces
           << ",\"controller_read_start_calls\":"
           << state.controller_read_start_calls
           << ",\"controller_get_data_calls\":"
           << state.controller_get_data_calls;
  } else if (update_output != nullptr)
    stream << ",\"controller_polls\":" << state.controller_samples
           << ",\"vi_retraces\":" << state.vi_retraces;
  stream << "}\n";
}

void write_private_progress(State &state,
                            const char *stage = "retrace") {
  constexpr std::uint64_t kProgressIntervalRetraces = 30U;
  // Actor motion needs per-retrace sampling to expose short freeze/jump
  // patterns. Deduplication keeps the many progress call sites from writing
  // the same actor state more than once in a retrace.
  trace_gameplay_state(state, false);
  write_actor_timing_trace(state);
  // This diagnostic contains sizeable rolling renderer traces.  Rewriting it
  // at every begin/end marker can turn optional observability into several
  // synchronous filesystem writes per video frame and make an otherwise
  // smooth build hitch.  One atomic snapshot per half-second is sufficient
  // for watchdog diagnosis and keeps instrumentation off the critical path.
  if (state.progress_path.empty() ||
      state.vi_retraces % kProgressIntervalRetraces != 0U ||
      state.last_progress_retrace == state.vi_retraces)
    return;
  state.last_progress_retrace = state.vi_retraces;
  std::ostringstream stream;
  const unsigned front_mode = state.rdram[0x000A51B0U ^ 3U];
  const std::uint32_t last_dispatch =
      state.recent_dispatch_position == 0U
          ? 0U
          : state.recent_dispatches[(state.recent_dispatch_position - 1U) %
                                    state.recent_dispatches.size()];
  stream << "{\"stage\":\"" << stage << "\""
         << ",\"vi_retraces\":" << state.vi_retraces
         << ",\"front_mode\":" << front_mode
         << ",\"dispatches\":" << state.dispatch_calls
         << ",\"last_dispatch\":\"0x" << std::hex << last_dispatch
         << std::dec << '"'
         << ",\"generated_calls\":" << state.generated_calls
         << ",\"sync_print_sink_calls\":" << state.sync_print_sink_calls
         << ",\"sound_queue_recoveries\":" << state.sound_queue_recoveries
         << ",\"timing_game_updates\":" << state.timing_game_updates
         << ",\"audio_device_underruns\":" << state.audio_device_underruns
         << ",\"audio_device_overruns\":" << state.audio_device_overruns
         << ",\"mapped_calls\":" << state.mapped_calls
         << ",\"interrupt_timeslices\":" << state.interrupt_timeslices
         << ",\"receive_successes\":" << state.receive_successes
         << ",\"receive_empties\":" << state.receive_empties
         << ",\"receive_blocks\":" << state.receive_blocks
         << ",\"last_receive_queue\":\"0x" << std::hex
         << state.last_receive_queue << "\""
         << ",\"last_receive_block\":" << std::dec
         << state.last_receive_block
         << ",\"last_receive_result\":" << state.last_receive_result
         << ",\"graphics_tasks\":" << state.graphics_tasks
         << ",\"presented_frames\":" << state.presented_frames
         << ",\"vi_current_framebuffer\":\"0x" << std::hex
         << state.vi_current_framebuffer
         << "\",\"vi_next_framebuffer\":\"0x"
         << state.vi_next_framebuffer
         << "\",\"graphics_color_image\":\"0x"
         << state.graphics_color_image
         << "\",\"recent_graphics_targets\":\"";
  const std::size_t graphics_recent_count =
      (std::min)(state.recent_graphics_position,
                 state.recent_graphics_color_images.size());
  for (std::size_t index = 0U; index < graphics_recent_count; ++index) {
    const std::size_t distance = graphics_recent_count - index;
    const std::size_t slot =
        (state.recent_graphics_position - distance) %
        state.recent_graphics_color_images.size();
    if (index != 0U)
      stream << ',';
    stream << state.recent_graphics_command_addresses[slot] << ':'
           << state.recent_graphics_color_images[slot] << ':'
           << state.recent_graphics_vi_selections[slot] << ':'
           << state.recent_graphics_matrix_commands[slot] << ':'
           << state.recent_graphics_vertices_loaded[slot] << ':'
           << state.recent_graphics_vertices_in_clip[slot] << ':'
           << state.recent_graphics_triangles[slot];
  }
  stream << "\"" << std::dec
         << ",\"decoded_audio_tasks\":" << state.decoded_audio_tasks
         << ",\"controller_samples\":" << state.controller_samples
         << ",\"non_neutral_controller_samples\":"
         << state.non_neutral_controller_samples
         << ",\"non_neutral_controller_writes\":"
         << state.non_neutral_controller_writes
         << ",\"game_pressed_observations\":"
         << state.game_pressed_observations
         << ",\"game_stick_observations\":"
         << state.game_stick_observations
         << ",\"last_game_stick_x\":"
         << static_cast<int>(state.last_game_stick_x)
         << ",\"last_game_stick_y\":"
         << static_cast<int>(state.last_game_stick_y)
         << ",\"menu_stick_observations\":"
         << state.menu_stick_observations
         << ",\"last_menu_stick_x\":"
         << static_cast<int>(state.last_menu_stick_x)
         << ",\"last_menu_stick_y\":"
         << static_cast<int>(state.last_menu_stick_y)
         << ",\"ai_register_writes\":"
         << (state.mmio_trace == nullptr
                 ? 0U
                 : state.mmio_trace->ai_register_writes)
         << ",\"ai_buffer_submissions\":"
         << (state.mmio_trace == nullptr
                 ? 0U
                 : state.mmio_trace->ai_buffer_submissions)
         << ",\"ai_dram_address\":\"0x" << std::hex
         << (state.mmio_trace == nullptr
                 ? 0U
                 : state.mmio_trace->ai_dram_address)
         << "\",\"ai_length\":" << std::dec
         << (state.mmio_trace == nullptr ? 0U
                                         : state.mmio_trace->ai_length)
         << ",\"ai_control\":"
         << (state.mmio_trace == nullptr ? 0U
                                         : state.mmio_trace->ai_control)
         << ",\"ai_dac_rate\":"
         << (state.mmio_trace == nullptr ? 0U
                                         : state.mmio_trace->ai_dac_rate)
         << ",\"ai_bit_rate\":"
         << (state.mmio_trace == nullptr ? 0U
                                         : state.mmio_trace->ai_bit_rate)
         << ",\"audio_task_us\":" << state.audio_task_microseconds
         << ",\"graphics_prepare_us\":"
         << state.graphics_prepare_microseconds
         << ",\"graphics_submit_us\":"
         << state.graphics_submit_microseconds
         << ",\"graphics_present_us\":"
         << state.graphics_present_microseconds
         << ",\"host_frame_interval_max_us\":"
         << state.host_frame_interval_max_us
         << ",\"present_interval_max_us\":"
         << state.present_interval_max_us
         << ",\"graphics_prepare_max_us\":"
         << state.graphics_prepare_max_us
         << ",\"graphics_submit_max_us\":"
         << state.graphics_submit_max_us
         << ",\"graphics_present_max_us\":"
         << state.graphics_present_max_us
         << ",\"host_frames_over_25ms\":"
         << state.host_frames_over_25ms
         << ",\"host_frames_over_50ms\":"
         << state.host_frames_over_50ms
         << ",\"host_frames_over_100ms\":"
         << state.host_frames_over_100ms
         << ",\"recent_matrix_conversions\":\"" << std::hex;
  const std::size_t matrix_recent_count =
      (std::min)(state.recent_matrix_conversion_position,
                 state.recent_matrix_conversion_sources.size());
  for (std::size_t index = 0U; index < matrix_recent_count; ++index) {
    const std::size_t distance = matrix_recent_count - index;
    const std::size_t slot =
        (state.recent_matrix_conversion_position - distance) %
        state.recent_matrix_conversion_sources.size();
    if (index != 0U)
      stream << ',';
    stream << state.recent_matrix_conversion_sources[slot] << ':'
           << state.recent_matrix_conversion_destinations[slot];
    for (const std::uint32_t word :
         state.recent_matrix_conversion_first_rows[slot])
      stream << ':' << word;
    for (const std::uint32_t word :
         state.recent_matrix_conversion_output_words[slot])
      stream << ':' << word;
  }
  stream << '"' << std::dec
         << ",\"recent_matrix_cats\":\"" << std::hex;
  const std::size_t matrix_cat_recent_count =
      (std::min)(state.recent_matrix_cat_position,
                 state.recent_matrix_cat_args.size());
  for (std::size_t index = 0U; index < matrix_cat_recent_count; ++index) {
    const std::size_t distance = matrix_cat_recent_count - index;
    const std::size_t slot =
        (state.recent_matrix_cat_position - distance) %
        state.recent_matrix_cat_args.size();
    if (index != 0U)
      stream << ',';
    for (const std::uint32_t argument : state.recent_matrix_cat_args[slot])
      stream << argument << ':';
    for (std::size_t word = 0U;
         word < state.recent_matrix_cat_words[slot].size(); ++word) {
      if (word != 0U)
        stream << ':';
      stream << state.recent_matrix_cat_words[slot][word];
    }
  }
  stream << '"' << std::dec
         << ",\"recent_matrix_watch\":\"" << std::hex;
  const std::size_t watch_recent_count =
      (std::min)(state.recent_matrix_watch_position,
                 state.recent_matrix_watch_addresses.size());
  for (std::size_t index = 0U; index < watch_recent_count; ++index) {
    const std::size_t distance = watch_recent_count - index;
    const std::size_t slot =
        (state.recent_matrix_watch_position - distance) %
        state.recent_matrix_watch_addresses.size();
    if (index != 0U)
      stream << ',';
    stream << state.recent_matrix_watch_addresses[slot] << ':'
           << state.recent_matrix_watch_before[slot] << ':'
           << state.recent_matrix_watch_after[slot] << ':'
           << state.recent_matrix_watch_targets[slot] << ':'
           << state.recent_matrix_watch_callers[slot] << ':'
           << state.recent_matrix_watch_current_targets[slot] << ':'
           << state.recent_matrix_watch_return_addresses[slot];
  }
  stream << '"' << std::dec
         << ",\"poison_matrix_watch\":\"" << std::hex;
  for (std::size_t index = 0U; index < state.poison_matrix_watch.size();
       ++index) {
    if (index != 0U)
      stream << ':';
    stream << state.poison_matrix_watch[index];
  }
  stream << '"' << std::dec
         << ",\"poison_dispatch_history\":\"" << std::hex;
  for (std::size_t index = 0U; index < state.poison_dispatch_history.size();
       ++index) {
    if (index != 0U)
      stream << ',';
    stream << state.poison_dispatch_history[index];
  }
  stream << '"' << std::dec
         << ",\"materialize_reject_reason\":"
         << state.materialize_reject_reason
         << ",\"materialize_reject_address\":\"0x" << std::hex
         << state.materialize_reject_address << "\""
         << ",\"materialize_reject_word0\":\"0x"
         << state.materialize_reject_word0 << "\""
         << ",\"materialize_reject_word1\":\"0x"
         << state.materialize_reject_word1 << "\"" << std::dec
         << ",\"materialize_parent_address\":\"0x" << std::hex
         << state.materialize_parent_address << "\""
         << ",\"materialize_parent_word0\":\"0x"
         << state.materialize_parent_word0 << "\""
         << ",\"materialize_parent_word1\":\"0x"
         << state.materialize_parent_word1 << "\"" << std::dec
         << ",\"materialize_block_address\":\"0x" << std::hex
         << state.materialize_block_address << "\""
         << ",\"materialize_parent_segment_base\":\"0x"
         << state.materialize_parent_segment_base << "\"" << std::dec
         << ",\"recent_dispatches\":\"";
  const std::size_t recent_count =
      (std::min)(state.recent_dispatch_position,
                 state.recent_dispatches.size());
  for (std::size_t index = 0U; index < recent_count; ++index) {
    const std::size_t distance = recent_count - index;
    if (index != 0U)
      stream << ',';
    stream << std::hex
           << state.recent_dispatches[(state.recent_dispatch_position -
                                       distance) %
                                      state.recent_dispatches.size()];
  }
  stream << std::dec << "\"}\n";
  (void)state.snapshot_writer.submit(0U,{{state.progress_path,
      std::make_shared<const std::string>(stream.str())}});
  // Keep the replay and timing streams recoverable even if the host console
  // or launcher disappears without giving the runtime a normal shutdown.
  // This runs on the existing half-second progress cadence, off the
  // per-frame hot path used by the renderer diagnostics.
  if (state.input_record.is_open())
    state.input_record.flush();
  if (state.native_clock_sample_output.is_open())
    state.native_clock_sample_output.flush();
  if (state.timing_trace.is_open())
    state.timing_trace.flush();
  state.gameplay_trace.flush();
  if (state.retrace_hash_trace.is_open())
    state.retrace_hash_trace.flush();
  if (state.update_hash_trace.is_open())
    state.update_hash_trace.flush();
  if (state.update_word_trace.is_open())
    state.update_word_trace.flush();
  if (state.controller_return_trace.is_open())
    state.controller_return_trace.flush();
  if (state.poll_hash_trace.is_open())
    state.poll_hash_trace.flush();
  if (state.event_trace.is_open())
    state.event_trace.flush();
}
#endif

#if defined(JFG_PHASE8_LIVE_RUNTIME)
bool configure_live_vi_mode(State &state, hle::GuestMemory &memory,
                            const std::uint32_t address) {
  // libultra's OSViMode is 0x50 bytes: a padded type byte, nine common
  // registers at +0x04, then five registers for each of the two fields.
  std::array<std::uint32_t, 9U> common{};
  std::array<LiveViFieldRegisters, 2U> fields{};
  for (std::size_t index = 0U; index < common.size(); ++index) {
    if (!memory.read_u32(address + 4U +
                             static_cast<std::uint32_t>(index * 4U),
                         common[index]))
      return false;
  }
  for (std::size_t field = 0U; field < fields.size(); ++field) {
    const std::uint32_t base =
        address + 0x28U + static_cast<std::uint32_t>(field * 0x14U);
    if (!memory.read_u32(base, fields[field].origin) ||
        !memory.read_u32(base + 4U, fields[field].y_scale) ||
        !memory.read_u32(base + 8U, fields[field].vertical_start) ||
        !memory.read_u32(base + 12U, fields[field].vertical_burst) ||
        !memory.read_u32(base + 16U, fields[field].intr))
      return false;
  }
  const std::uint32_t pixel_type = common[0U] & 0x03U;
  if ((pixel_type != 2U && pixel_type != 3U) || common[1U] == 0U ||
      common[1U] > 4096U || (common[7U] & 0x0FFFU) == 0U ||
      fields[0U].origin >= kRdramSize ||
      fields[1U].origin >= kRdramSize ||
      (fields[0U].y_scale & 0x0FFFU) == 0U ||
      (fields[1U].y_scale & 0x0FFFU) == 0U)
    return false;

  state.vi_mode_control = common[0U];
  state.rt64_vi.status = common[0U];
  state.rt64_vi.width = common[1U];
  state.rt64_vi.timing = common[2U];
  state.rt64_vi.vertical_sync = common[3U];
  state.rt64_vi.horizontal_sync = common[4U];
  state.rt64_vi.leap = common[5U];
  state.vi_mode_horizontal_start = common[6U];
  state.rt64_vi.horizontal_start =
      state.vi_blacked ? 0U : state.vi_mode_horizontal_start;
  state.rt64_vi.x_scale = common[7U];
  state.rt64_vi.current_line = common[8U] & 0x03FFU;
  state.vi_fields = fields;
  state.rt64_vi.y_scale = fields[0U].y_scale;
  state.rt64_vi.vertical_start = fields[0U].vertical_start;
  state.rt64_vi.vertical_burst = fields[0U].vertical_burst;
  state.rt64_vi.intr = fields[0U].intr;
  state.vi_mode_configured = true;
  state.gameplay_trace.event("vi-mode", {state.vi_retraces, common[0U], common[1U],
      common[6U], fields[0U].vertical_start, common[7U], fields[0U].y_scale});
  return true;
}

bool apply_live_vi_field(State &state, const std::uint32_t framebuffer) {
  const bool serrate = (state.rt64_vi.status & 0x40U) != 0U;
  const std::size_t field = serrate
      ? static_cast<std::size_t>((state.rt64_vi.current_line >> 1U) & 1U)
      : 0U;
  const LiveViFieldRegisters &registers = state.vi_fields[field];
  const std::uint64_t origin =
      std::uint64_t{framebuffer & 0x00FF'FFFFU} + registers.origin;
  if (origin >= jfg::kRt64RequiredRdramBytes)
    return false;
  state.rt64_vi.origin = static_cast<std::uint32_t>(origin);
  state.rt64_vi.y_scale = registers.y_scale;
  state.rt64_vi.vertical_start = registers.vertical_start;
  state.rt64_vi.vertical_burst = registers.vertical_burst;
  state.rt64_vi.intr = registers.intr;
  return true;
}

bool configure_flash_path(State &state, const char *path) {
  if (path == nullptr || *path == '\0')
    return true;
  state.flash_path = std::filesystem::path(path);
  if (state.flash_path.empty() || state.flash_path.filename().empty())
    return false;
  std::error_code error;
  const std::filesystem::path parent = state.flash_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, error);
    if (error)
      return false;
  }
  const bool exists = std::filesystem::exists(state.flash_path, error);
  if (error)
    return false;
  if (exists) {
    if (!std::filesystem::is_regular_file(state.flash_path, error) || error ||
        !state.flashram.reload(state.flash_path).ok())
      return false;
    state.flash_image_loaded = true;
    return true;
  }
  if (!state.flashram.persist_atomic(state.flash_path).ok())
    return false;
  state.flash_image_created = true;
  return true;
}

bool configure_controller_pak_path(State &state, const char *path) {
  if (path == nullptr || *path == '\0')
    return true;
  state.controller_pak_path = std::filesystem::path(path);
  if (state.controller_pak_path.empty() ||
      state.controller_pak_path.filename().empty())
    return false;
  std::error_code error;
  const std::filesystem::path parent =
      state.controller_pak_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, error);
    if (error)
      return false;
  }
  state.controller_accessory_bus =
      std::make_unique<jfg::ControllerAccessoryBus>(
          jfg::ControllerPakGeometry{32U * 1024U, 16U});
  if (!state.controller_accessory_bus
           ->set_accessory(0U, jfg::ControllerAccessoryKind::controller_pak)
           .ok())
    return false;
  const jfg::ControllerPakProbeResult probe =
      state.controller_accessory_bus->probe_controller_pak(0U);
  if (!probe.ok())
    return false;
  state.controller_pak_session = probe.session;
  const bool exists = std::filesystem::exists(state.controller_pak_path, error);
  if (error)
    return false;
  if (exists) {
    if (!std::filesystem::is_regular_file(state.controller_pak_path, error) ||
        error ||
        !state.controller_accessory_bus
             ->reload(state.controller_pak_session, state.controller_pak_path)
             .ok())
      return false;
    state.controller_pak_loaded = true;
    return true;
  }
  if (!state.controller_accessory_bus
           ->persist(state.controller_pak_session, state.controller_pak_path)
           .ok())
    return false;
  state.controller_pak_created = true;
  return true;
}

bool persist_controller_pak(State &state) {
  return state.controller_pak_path.empty() ||
         (state.controller_accessory_bus != nullptr &&
          state.controller_accessory_bus
              ->persist(state.controller_pak_session,
                        state.controller_pak_path)
              .ok());
}

bool mirror_synthetic_overlay_in_runlink(
    State &state, const JfgGeneratedSectionMetadata &metadata) {
  if (state.rdram == nullptr)
    return false;
  hle::GuestMemory memory({state.rdram, kRdramSize},
                          hle::GuestMemory::Layout::native_word_big_endian);
  const RunlinkPublicationResult result = publish_runlink_module(
      memory, kRunlinkModuleTablePointer, kRunlinkOverlaySlotCount,
      RunlinkModuleIdentity{metadata.rom_start, metadata.text_size,
                            metadata.data_size, metadata.bss_size},
      metadata.linked_vram);
  return result == RunlinkPublicationResult::published ||
         result == RunlinkPublicationResult::already_published ||
         result == RunlinkPublicationResult::table_uninitialized;
}

enum class SyntheticOverlayPublication : std::uint8_t {
  ensure_active,
  reload,
};

// Observe the original cold loader without publishing a synthetic base into
// its still-empty record. Publication is deferred until the guest allocated
// the module, so its allocator, relocation work and init ordering stay intact.
bool note_guest_cold_overlay_load(State &state, const std::uint32_t slot) {
  if (slot == 0U)
    return true;
  if (slot >= kRunlinkOverlaySlotCount)
    return false;
  hle::GuestMemory memory({state.rdram, kRdramSize},
                         hle::GuestMemory::Layout::native_word_big_endian);
  std::uint32_t table = 0U, base = 0U;
  if (!memory.read_u32(kRunlinkModuleTablePointer, table) || table == 0U ||
      (table & 3U) != 0U || table < kKseg0 ||
      std::uint64_t{table} + (slot + 1U) * kRunlinkModuleRecordSize >
          std::uint64_t{kKseg0} + kRdramSize)
    return false;
  const auto record = table + slot * kRunlinkModuleRecordSize;
  if (!memory.read_u32(record, base))
    return false;
  if (base != 0U)
    return true; // The original loader also returns without loading anything.
  const auto suspension = resolve_runlink_suspension(memory, 0x800FEAD8U, 16U, slot);
  if (suspension != RunlinkSuspensionResult::absent)
    return suspension == RunlinkSuspensionResult::suspended;
  RunlinkModuleIdentity identity{};
  if (!memory.read_u32(record + 4U, identity.rom_start) ||
      !memory.read_u32(record + 8U, identity.text_size) ||
      !memory.read_u32(record + 12U, identity.data_size) ||
      !memory.read_u32(record + 16U, identity.bss_size))
    return false;
  std::uint32_t linked = 0U;
  for (std::uint32_t section = 0U; section < jfg_generated_section_count(); ++section) {
    JfgGeneratedSectionMetadata metadata{};
    if (jfg_generated_section_metadata(section, &metadata) == 0)
      return false;
    if (metadata.is_overlay != 1U || metadata.rom_start != identity.rom_start ||
        metadata.text_size != identity.text_size || metadata.data_size != identity.data_size ||
        metadata.bss_size != identity.bss_size)
      continue;
    if (linked != 0U)
      return false;
    linked = metadata.linked_vram;
  }
  if (linked != 0U) {
    state.pending_cold_overlay_images.insert(linked);
    state.gameplay_trace.event("overlay-cold-prepare",
        {state.vi_retraces, state.controller_samples, slot, linked});
  }
  return true;
}

bool ensure_guest_overlay_allocation(
    State &state, const JfgGeneratedSectionMetadata &metadata,
    recomp_context &caller_context) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  const auto inventory_started = state.diagnostic_cpu_costs ? std::chrono::steady_clock::now()
      : std::chrono::steady_clock::time_point{};
#endif
  hle::GuestMemory memory({state.rdram, kRdramSize},
                         hle::GuestMemory::Layout::native_word_big_endian);
  std::uint32_t table = 0U;
  if (!memory.read_u32(kRunlinkModuleTablePointer, table))
    return false;
  if (table == 0U)
    return true; // The guest has not initialized its module inventory yet.
  // Validate the complete word-aligned inventory once. Its native-word
  // layout allows a byte-exact inventory comparison on every call. The
  // derived identity index is invalidated by any changed byte, preserving
  // unloads, duplicates and malformed-table rejection. Bases stay live below.
  const std::uint64_t first_record = std::uint64_t{table} + kRunlinkModuleRecordSize;
  const std::uint64_t inventory_end = std::uint64_t{table} +
      kRunlinkOverlaySlotCount * kRunlinkModuleRecordSize + 20U;
  if ((table & 3U) != 0U || first_record + 4U < kKseg0 ||
      inventory_end > std::uint64_t{kKseg0} + kRdramSize)
    return false;
  static_assert(kRunlinkOverlaySlotCount == NativeRunlinkIdentityIndex::kSlotCount);
  const auto matched_slot = state.native_overlay_identity_index.find(
      {state.rdram + static_cast<std::size_t>(first_record - kKseg0),
       NativeRunlinkIdentityIndex::kRecordBytes},
      {metadata.rom_start, metadata.text_size, metadata.data_size, metadata.bss_size});
  const auto matched_record = table + matched_slot * kRunlinkModuleRecordSize;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (state.diagnostic_cpu_costs) {
    state.profile_overlay_inventory_ns += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now()-inventory_started).count());
    ++state.profile_overlay_inventory_calls;
  }
#endif
  if (matched_slot == 0U)
    return false;
  std::uint32_t base = 0U;
  if (!memory.read_u32(matched_record, base))
    return false;
  if (base != 0U && base != metadata.linked_vram)
    return true; // Preserve the allocation chosen by the guest loader.
  if (!state.guest_overlay_loads_in_progress.insert(matched_slot).second)
    return false;
  // A suspended module retains its data/BSS allocation. The cold loader
  // deliberately rejects its pending entry; resume through the generated
  // guest routine so it can reacquire text without resetting that state.
  const auto suspension = resolve_runlink_suspension(
      memory, 0x800FEAD8U, 16U, matched_slot);
  if (suspension == RunlinkSuspensionResult::invalid) {
    state.guest_overlay_loads_in_progress.erase(matched_slot);
    return false;
  }
  const bool resume = suspension == RunlinkSuspensionResult::suspended;
  auto *loader = jfg_generated_lookup_function(static_cast<std::int32_t>(
      resume ? 0x80053D2CU : 0x80052CFCU));
  if (loader == nullptr || !memory.write_u32(matched_record, 0U)) {
    state.guest_overlay_loads_in_progress.erase(matched_slot);
    return false;
  }
  // Use the real generated loader, including temporary relocation allocations,
  // PI completion and module lifetime state. Preserve the pending call's CPU
  // arguments; the loader uses the current guest stack below its incoming SP.
  if (!resume)
    state.pending_cold_overlay_images.insert(metadata.linked_vram);
  recomp_context loader_context = caller_context;
  // A context copy retains f_odd's host pointer into its source. The detached
  // loader must own every FPR; otherwise odd-register writes leak to the caller.
  loader_context.f_odd = loader_context.mips3_float_mode
      ? &loader_context.f1.u32l : &loader_context.f0.u32h;
  loader_context.r4 = matched_slot;
  loader(state.rdram, &loader_context);
  set_cop1_cs(caller_context.fcr31);
  state.guest_overlay_loads_in_progress.erase(matched_slot);
  // Resume has a void guest ABI; its published base is the success condition.
  return (resume || static_cast<std::uint32_t>(loader_context.r2) != 0U) &&
         memory.read_u32(matched_record, base) && base != 0U &&
         base != metadata.linked_vram;
}

bool publish_synthetic_overlay(
    State &state, const std::uint32_t section,
    const JfgGeneratedSectionMetadata &metadata,
    SyntheticOverlayPublication publication =
        SyntheticOverlayPublication::ensure_active) {
  const auto reject = [&state](const char *reason) {
    state.last_overlay_publication_failure = reason;
    return false;
  };
  state.last_overlay_publication_failure = "none";
  if (state.rdram == nullptr || state.rom == nullptr ||
      metadata.is_overlay != 1U)
    return reject("invalid-input");
  const bool cold_image = state.pending_cold_overlay_images.contains(metadata.linked_vram);
  if (cold_image)
    publication = SyntheticOverlayPublication::reload;
  const bool was_active = state.active_overlay_sections.contains(section);
  if (was_active &&
      publication == SyntheticOverlayPublication::ensure_active) {
    if (!mirror_synthetic_overlay_in_runlink(state, metadata))
      return reject("active-runlink");
    return true;
  }
  const std::uint64_t initialized =
      std::uint64_t{metadata.text_size} + metadata.data_size;
  const std::uint64_t extent = initialized + metadata.bss_size;
  const std::uint32_t host_offset = metadata.linked_vram - kKseg0;
  if (initialized > SIZE_MAX || extent > SIZE_MAX || extent == 0U ||
      metadata.rom_start > state.rom_size ||
      initialized > state.rom_size - metadata.rom_start ||
      host_offset > kGuestAddressSpan ||
      extent > kGuestAddressSpan - host_offset)
    return reject("range");

  const std::size_t expected = jfg_generated_relocation_count(section);
  const std::uint32_t *sites = nullptr;
  const JfgGeneratedR32Descriptor *descriptors = nullptr;
  std::size_t site_count = 0U, descriptor_count = 0U;
  if (jfg_generated_relocation_sites(section, &sites, &site_count) == 0 ||
      jfg_generated_relocation_descriptors(section, &descriptors,
                                           &descriptor_count) == 0 ||
      site_count != expected || descriptor_count != expected ||
      (expected != 0U && (sites == nullptr || descriptors == nullptr)))
    return reject("relocation-table");

  std::vector<std::pair<std::size_t, std::uint32_t>> patches;
  patches.reserve(expected);
  std::unordered_set<std::uint32_t> unique_sites;
  for (std::size_t index = 0U; index < expected; ++index) {
    const auto &descriptor = descriptors[index];
    JfgGeneratedSectionMetadata target{};
    if (sites[index] != descriptor.site_offset ||
        !unique_sites.insert(descriptor.site_offset).second ||
        descriptor.site_offset > initialized ||
        initialized - descriptor.site_offset < sizeof(std::uint32_t) ||
        jfg_generated_section_metadata(descriptor.target_section, &target) ==
            0)
      return reject("relocation-descriptor");
    const std::uint64_t target_extent = std::uint64_t{target.text_size} +
                                        target.data_size + target.bss_size;
    // The normalized ABI assigns every overlay a stable synthetic address.
    // Cross-overlay R_MIPS_32 values must therefore be published even while
    // the target module is inactive: the game stores these as deferred
    // function/data references and loads the selected target later. Requiring
    // every referenced cutscene module to be active made the shared animation
    // overlay impossible to load at all.
    if (descriptor.target_offset >= target_extent)
      return reject("relocation-target");
    const std::uint64_t value =
        std::uint64_t{target.linked_vram} + descriptor.target_offset;
    if (value > UINT32_MAX)
      return reject("relocation-value");
    patches.emplace_back(static_cast<std::size_t>(host_offset) +
                             descriptor.site_offset,
                         static_cast<std::uint32_t>(value));
  }

  // Direct calls merely ensure that a normalized overlay is available. An
  // exact PI load of the module ROM start is different: it is the game's
  // genuine overlay load boundary. Honor the generated lifecycle inventory
  // there so mutable data and BSS (including the health-meter state in
  // section 6) cannot survive a death/respawn reload.
  if (was_active &&
      jfg_generated_section_lifecycle(
          2U, section, static_cast<std::int32_t>(metadata.linked_vram)) != 0)
    return reject("lifecycle-unload");

  auto *const destination = state.rdram + host_offset;
  for (std::size_t index = 0U; index < static_cast<std::size_t>(initialized);
       ++index)
    destination[index ^ 3U] = state.rom[metadata.rom_start + index];
  std::fill_n(destination + static_cast<std::size_t>(initialized),
              static_cast<std::size_t>(metadata.bss_size), 0U);
  if (jfg_generated_section_lifecycle(
          1U, section, static_cast<std::int32_t>(metadata.linked_vram)) != 0)
    return reject("lifecycle");
  for (const auto &[offset, value] : patches)
    std::memcpy(state.rdram + offset, &value, sizeof(value));
  if (!mirror_synthetic_overlay_in_runlink(state, metadata))
    return reject("runlink");
  state.active_overlay_sections.insert(section);
  if (cold_image) {
    state.pending_cold_overlay_images.erase(metadata.linked_vram);
    state.gameplay_trace.event("overlay-cold-publish",
        {state.vi_retraces, state.controller_samples, section, metadata.linked_vram,
         was_active ? 1U : 0U});
  }
  return true;
}

#endif

bool resolve_bzero_host_offset(const State &state, const std::uint32_t address,
                               const std::uint32_t length,
                               std::size_t &host_offset) {
  const std::uint32_t segment = address & 0xE0000000U;
  const std::uint64_t physical = address & 0x1FFFFFFFU;
  if ((segment == 0x80000000U || segment == 0xA0000000U) &&
      physical <= kRdramSize && length <= kRdramSize - physical) {
    host_offset = static_cast<std::size_t>(physical);
    return true;
  }

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  // Generated overlay-local references use their normalized synthetic VRAM
  // addresses. GuestBacking reserves and commits those exact windows at the
  // offset used by the generated MEM_* helpers, so HLE memory primitives must
  // recognize an active overlay's bounded window as well as ordinary RDRAM.
  for (const std::uint32_t section : state.active_overlay_sections) {
    JfgGeneratedSectionMetadata metadata{};
    if (jfg_generated_section_metadata(section, &metadata) == 0 ||
        metadata.is_overlay != 1U)
      continue;
    const std::uint64_t extent = std::uint64_t{metadata.text_size} +
                                 metadata.data_size + metadata.bss_size;
    const std::uint64_t begin = metadata.linked_vram;
    const std::uint64_t requested = address;
    if (requested < begin || requested - begin > extent ||
        length > extent - (requested - begin))
      continue;
    const std::uint64_t synthetic_host_offset =
        std::uint64_t{0x80000000U} + requested;
    if (synthetic_host_offset > kGuestAddressSpan ||
        length > kGuestAddressSpan - synthetic_host_offset)
      return false;
    host_offset = static_cast<std::size_t>(synthetic_host_offset);
    return true;
  }
#else
  (void)state;
#endif
  return false;
}

#if defined(JFG_PHASE8_LIVE_RUNTIME)
class LiveAudioMemory final : public jfg::AudioMemoryAccess {
public:
  explicit LiveAudioMemory(std::uint8_t *rdram) : rdram_(rdram) {}

  [[nodiscard]] bool read(const std::size_t region_index,
                          const std::size_t offset,
                          const std::span<std::byte> destination) noexcept
      override {
    if (region_index != 0U || destination.empty() || offset > kRdramSize ||
        destination.size() > kRdramSize - offset)
      return false;
    for (std::size_t index = 0U; index < destination.size(); ++index)
      destination[index] =
          static_cast<std::byte>(rdram_[(offset + index) ^ 3U]);
    return true;
  }

  [[nodiscard]] bool write_region(
      const std::size_t region_index, const std::size_t offset,
      const std::span<const std::byte> source) noexcept override {
    if (region_index != 0U || source.empty() || offset > kRdramSize ||
        source.size() > kRdramSize - offset)
      return false;
    for (std::size_t index = 0U; index < source.size(); ++index)
      rdram_[(offset + index) ^ 3U] =
          static_cast<std::uint8_t>(source[index]);
    ++write_operations_;
    written_bytes_ += source.size();
    return true;
  }

  [[nodiscard]] bool write_output(
      std::size_t, std::span<const std::byte>) noexcept override {
    return false;
  }

  [[nodiscard]] std::size_t write_operations() const noexcept {
    return write_operations_;
  }
  [[nodiscard]] std::size_t written_bytes() const noexcept {
    return written_bytes_;
  }

private:
  std::uint8_t *rdram_ = nullptr;
  std::size_t write_operations_ = 0U;
  std::size_t written_bytes_ = 0U;
};

bool guest_task_blob(const State &state, const std::uint32_t address,
                     const std::size_t size,
                     std::vector<std::byte> &output) {
  const std::uint32_t physical = address & 0x1FFFFFFFU;
  if (size == 0U || physical > kRdramSize || size > kRdramSize - physical)
    return false;
  output.resize(size);
  for (std::size_t index = 0U; index < size; ++index) {
    output[index] =
        static_cast<std::byte>(state.rdram[(physical + index) ^ 3U]);
  }
  return true;
}

std::uint32_t translate_live_graphics_address(
    const State &state, const std::uint32_t address) noexcept {
  // The single-range translator always preserves real RDRAM aliases. Check
  // that invariant once instead of rechecking it for every overlay range.
  const auto segment = address & 0xE0000000U;
  if ((segment == 0U || segment == 0x80000000U || segment == 0xA0000000U) &&
      (address & 0x1FFFFFFFU) < kRdramSize)
    return address;
  for (const LiveGraphicsOverlayShadow &shadow :
       state.graphics_overlay_shadows) {
    const std::uint32_t translated = jfg::translate_rt64_overlay_address(
        address, static_cast<std::uint32_t>(kRdramSize),
        {shadow.linked_base, shadow.extent, shadow.physical_base});
    if (translated != address)
      return translated;
  }
  return address;
}

// Index of the graphics overlay shadow holding snapshot byte `offset`, or
// the shadow count when no shadow holds it. Shadows are sorted by base.
std::size_t community_shadow_index(const State &state, const std::uint64_t offset) {
  const auto &shadows = state.graphics_overlay_shadows;
  const auto after = std::upper_bound(shadows.begin(), shadows.end(), offset,
      [](const std::uint64_t value, const LiveGraphicsOverlayShadow &shadow) {
        return value < shadow.physical_base;
      });
  if (after == shadows.begin()) return shadows.size();
  const auto &shadow = *(after - 1);
  if (offset - shadow.physical_base >= shadow.extent) return shadows.size();
  return static_cast<std::size_t>((after - 1) - shadows.begin());
}

bool materialize_live_graphics_overlays(
    State &state, std::vector<std::byte> &snapshot,
    const std::uint32_t command_address,
    const std::uint32_t command_count,
    std::vector<std::uint8_t> *community_dirty = nullptr) {
  // Community ranged import: flags must cover every shadow plus the
  // "outside all shadows" slot; anything else means unknown = all dirty.
  const std::size_t shadow_count = state.graphics_overlay_shadows.size();
  if (community_dirty != nullptr && community_dirty->size() != shadow_count + 1U)
    community_dirty->assign(shadow_count + 1U, 1U);
  const auto mark_dirty = [&](const std::uint64_t offset) {
    if (community_dirty == nullptr || offset < kRdramSize) return;
    (*community_dirty)[community_shadow_index(state, offset)] = 1U;
  };
  state.materialize_reject_reason = 0U;
  state.materialize_reject_address = 0U;
  state.materialize_reject_word0 = 0U;
  state.materialize_reject_word1 = 0U;
  state.materialize_parent_address = 0U;
  state.materialize_parent_word0 = 0U;
  state.materialize_parent_word1 = 0U;
  state.materialize_block_address = 0U;
  state.materialize_parent_segment_base = 0U;
  const auto reject = [&state](const std::uint32_t reason,
                               const std::uint32_t address,
                               const std::uint32_t word0,
                               const std::uint32_t word1) {
    state.materialize_reject_reason = reason;
    state.materialize_reject_address = address;
    state.materialize_reject_word0 = word0;
    state.materialize_reject_word1 = word1;
    return false;
  };
  if (state.rom == nullptr ||
      snapshot.size() != jfg::kRt64RequiredRdramBytes || command_count == 0U)
    return reject(1U, command_address, 0U, 0U);
  if (state.graphics_overlay_initial_data.empty()) {
    for (const LiveGraphicsOverlayShadow &shadow :
         state.graphics_overlay_shadows) {
      JfgGeneratedSectionMetadata metadata{};
      if (jfg_generated_section_metadata(shadow.section, &metadata) == 0)
        return false;
      const std::uint64_t initialized =
          std::uint64_t{metadata.text_size} + metadata.data_size;
      if (
          initialized > shadow.extent || metadata.rom_start > state.rom_size ||
          initialized > state.rom_size - metadata.rom_start ||
          shadow.physical_base > snapshot.size() ||
          shadow.extent > snapshot.size() - shadow.physical_base)
        return false;
      for (std::size_t index = 0U; index < initialized; ++index)
        snapshot[(shadow.physical_base + index) ^ 3U] =
            static_cast<std::byte>(state.rom[metadata.rom_start + index]);
      std::fill_n(snapshot.begin() + shadow.physical_base + initialized,
                  metadata.bss_size, std::byte{0});
    }
    for (const LiveGraphicsOverlayShadow &shadow :
         state.graphics_overlay_shadows) {
      const JfgGeneratedR32Descriptor *descriptors = nullptr;
      std::size_t count = 0U;
      if (jfg_generated_relocation_descriptors(shadow.section, &descriptors,
                                               &count) == 0 ||
          (count != 0U && descriptors == nullptr))
        return false;
      for (std::size_t index = 0U; index < count; ++index) {
        JfgGeneratedSectionMetadata target{};
        if (jfg_generated_section_metadata(descriptors[index].target_section,
                                           &target) == 0 ||
            shadow.extent < 4U ||
            descriptors[index].site_offset > shadow.extent - 4U ||
            descriptors[index].target_offset >
                UINT32_MAX - target.linked_vram)
          return false;
        const std::uint32_t linked =
            target.linked_vram + descriptors[index].target_offset;
        const std::uint32_t translated =
            translate_live_graphics_address(state, linked);
        std::memcpy(snapshot.data() + shadow.physical_base +
                        descriptors[index].site_offset,
                    &translated, sizeof(translated));
      }
    }
    state.graphics_overlay_initial_data.assign(snapshot.begin() + kRdramSize,
                                                snapshot.end());
    if (community_dirty != nullptr)
      community_dirty->assign(shadow_count + 1U, 0U);
  } else {
    for (std::size_t shadow_index = 0U; shadow_index < shadow_count; ++shadow_index) {
      const auto &shadow = state.graphics_overlay_shadows[shadow_index];
      // Community: a shadow this buffer never changed since its last
      // restore already holds the initial image.
      if (community_dirty != nullptr && (*community_dirty)[shadow_index] == 0U)
        continue;
      if (community_dirty != nullptr)
        (*community_dirty)[shadow_index] = 0U;
      if (shadow.physical_base < kRdramSize ||
          shadow.physical_base - kRdramSize > state.graphics_overlay_initial_data.size() ||
          shadow.extent > state.graphics_overlay_initial_data.size() -
                              (shadow.physical_base - kRdramSize))
        return reject(1U, command_address, 0U, 0U);
      std::copy_n(state.graphics_overlay_initial_data.data() +
                      (shadow.physical_base - kRdramSize),
                  shadow.extent, snapshot.data() + shadow.physical_base);
    }
  }

  // Planet meshes and other overlay-local graphics are generated into
  // writable DATA/BSS. Keep cached text and inactive-module fallbacks, but
  // submit the current CPU-owned data for every published allocation.
  for (const auto &shadow : state.graphics_overlay_shadows) {
    if (!state.active_overlay_sections.contains(shadow.section))
      continue;
    if (community_dirty != nullptr)
      (*community_dirty)[static_cast<std::size_t>(
          &shadow - state.graphics_overlay_shadows.data())] = 1U;
    JfgGeneratedSectionMetadata metadata{};
    if (jfg_generated_section_metadata(shadow.section, &metadata) == 0 ||
        metadata.text_size > shadow.extent ||
        shadow.physical_base > snapshot.size() ||
        shadow.extent > snapshot.size() - shadow.physical_base)
      return reject(1U, shadow.linked_base, 0U, 0U);
    const std::uint32_t host_offset = shadow.linked_base - kKseg0;
    if (host_offset > kGuestAddressSpan ||
        shadow.extent > kGuestAddressSpan - host_offset)
      return reject(1U, shadow.linked_base, 0U, 0U);
    std::memcpy(snapshot.data() + shadow.physical_base + metadata.text_size,
                state.rdram + host_offset + metadata.text_size,
                shadow.extent - metadata.text_size);

    // Live pointer fields contain CPU addresses. Translate the current value,
    // preserving pointers that the game has changed since module loading.
    const std::uint32_t *sites = nullptr;
    std::size_t count = 0U;
    if (jfg_generated_relocation_sites(shadow.section, &sites, &count) == 0 ||
        (count != 0U && sites == nullptr))
      return reject(1U, shadow.linked_base, 0U, 0U);
    for (std::size_t index = 0U; index < count; ++index) {
      const std::uint32_t offset = sites[index];
      if (shadow.extent < sizeof(std::uint32_t) ||
          offset > shadow.extent - sizeof(std::uint32_t))
        return reject(1U, shadow.linked_base, 0U, 0U);
      if (offset < metadata.text_size)
        continue;
      auto *const site = snapshot.data() + shadow.physical_base + offset;
      std::uint32_t value = 0U;
      std::memcpy(&value, site, sizeof(value));
      value = translate_live_graphics_address(state, value);
      std::memcpy(site, &value, sizeof(value));
    }
  }

  struct DisplayListFrame final {
    std::uint32_t address;
    std::uint32_t count;
    std::uint32_t index;
    std::uint32_t parent_address;
    std::uint32_t parent_word0;
    std::uint32_t parent_word1;
    std::uint32_t parent_segment_base;
    bool counted_dma;
  };
  std::array<bool, 16U> configured_segments{};
  std::array<std::uint32_t, 16U> segment_bases{};
  constexpr std::uint32_t kRt64DisplayListDmaMask =
      static_cast<std::uint32_t>(jfg::kRt64RequiredRdramBytes - 8U);
  const auto display_list_dma_address =
      [kRt64DisplayListDmaMask](const std::uint32_t address) {
    return address & kRt64DisplayListDmaMask;
  };
  constexpr std::uint32_t kUnboundedCommandLimit = 65536U;
  constexpr std::uint64_t kTraversalCommandLimit = 4U * 1024U * 1024U;
  std::vector<DisplayListFrame> frames{{
      display_list_dma_address(command_address), command_count, 0U,
      0U, 0U, 0U, 0U, false}};
  std::unordered_set<std::uint64_t> active_blocks;
  const auto block_key = [](const DisplayListFrame &frame) {
    return (std::uint64_t{frame.address} << 32U) |
           (std::uint64_t{frame.count} << 1U) |
           static_cast<std::uint64_t>(frame.counted_dma);
  };
  active_blocks.insert(block_key(frames.back()));
  std::uint64_t traversed_commands = 0U;
  while (!frames.empty()) {
    DisplayListFrame &frame = frames.back();
    state.materialize_parent_address = frame.parent_address;
    state.materialize_parent_word0 = frame.parent_word0;
    state.materialize_parent_word1 = frame.parent_word1;
    state.materialize_block_address = frame.address;
    state.materialize_parent_segment_base = frame.parent_segment_base;
    const std::uint32_t limit =
        frame.count == 0U ? kUnboundedCommandLimit : frame.count;
    if (frame.index >= limit) {
      if (frame.count == 0U)
        return reject(13U, frame.address, 0U, 0U);
      active_blocks.erase(block_key(frame));
      frames.pop_back();
      continue;
    }
    if (++traversed_commands > kTraversalCommandLimit)
      return reject(14U, frame.address, 0U, 0U);

    const std::uint64_t offset =
        std::uint64_t{frame.address} + frame.index * 8U;
    ++frame.index;
    if (offset > snapshot.size() || snapshot.size() - offset < 8U)
      return reject(10U, static_cast<std::uint32_t>(offset), 0U, 0U);
    std::uint32_t words[2]{};
    std::memcpy(words, snapshot.data() + offset, sizeof(words));
    const std::uint8_t opcode = static_cast<std::uint8_t>(words[0] >> 24U);

    // F3DDKR's counted DMA-list handler dispatches only RDP commands. Low
    // opcodes inside that fixed-size block are data/no-ops, not nested lists
    // or segment updates. Treating them as normal GBI commands caused the old
    // breadth-first walker to patch unrelated memory as display-list data.
    if (frame.counted_dma && (opcode & 0xC0U) != 0xC0U)
      continue;

    if (!frame.counted_dma && opcode == 0xBCU &&
        static_cast<std::uint8_t>(words[0]) == 0x06U) {
      const std::uint32_t segment = ((words[0] >> 8U) & 0xFFFFU) >> 2U;
      if (segment >= configured_segments.size())
        return reject(11U, static_cast<std::uint32_t>(offset), words[0],
                      words[1]);
      configured_segments[segment] = true;
      segment_bases[segment] = display_list_dma_address(
          translate_live_graphics_address(state, words[1]));
    }

    const bool address_command =
        (!frame.counted_dma &&
         (opcode == 0x01U || opcode == 0x02U || opcode == 0x03U ||
          opcode == 0x04U || opcode == 0x05U || opcode == 0x06U ||
          opcode == 0x07U)) ||
        opcode == 0xFDU || opcode == 0xFEU || opcode == 0xFFU;
    const std::uint32_t original_address = words[1];
    const std::uint32_t segment = original_address >> 24U;
    const std::uint32_t translated_address = address_command
        ? translate_live_graphics_address(state, original_address)
        : original_address;
    // A matched synthetic overlay pointer is absolute, even when its high
    // byte happens to name a configured RSP segment. The options overlay
    // links above 32 MiB; treating that address as segment 2 skips its RDP
    // state lists whenever the scene has installed a nonzero segment base.
    const std::uint32_t materialized_address = translated_address;
    if (address_command) {
      const bool overlay_payload_command =
          opcode == 0x01U || opcode == 0x02U || opcode == 0x03U ||
          opcode == 0x04U || opcode == 0x05U || opcode == 0xFDU;
      const bool translated_display_list_edge =
          (opcode == 0x06U || opcode == 0x07U) &&
          translated_address != original_address;
      if ((overlay_payload_command &&
           translated_address != original_address) ||
          translated_display_list_edge ||
          segment >= configured_segments.size() ||
          !configured_segments[segment])
        words[1] = materialized_address;
      std::memcpy(snapshot.data() + offset, words, sizeof(words));
      mark_dirty(offset);
    }

    if (!frame.counted_dma && (opcode == 0x06U || opcode == 0x07U)) {
      const std::uint32_t segment_base =
          segment < segment_bases.size() && configured_segments[segment]
              ? segment_bases[segment]
              : 0U;
      const std::uint32_t traversal_address = jfg::resolve_rt64_display_list_edge(
          original_address, materialized_address, segment_base);
      const bool counted_dma = opcode == 0x07U;
      const std::uint32_t nested_count =
          counted_dma ? ((words[0] >> 16U) & 0xFFU) : 0U;
      if (counted_dma &&
          (nested_count == 0U ||
           (words[0] & 0xFFFFU) != nested_count * 8U))
        return reject(12U, static_cast<std::uint32_t>(offset), words[0],
                      words[1]);

      // G_DL with the no-push flag is a branch: the parent never resumes.
      if (!counted_dma && ((words[0] >> 16U) & 1U) != 0U) {
        active_blocks.erase(block_key(frames.back()));
        frames.pop_back();
      }
      const DisplayListFrame nested{
          traversal_address, nested_count, 0U,
          static_cast<std::uint32_t>(offset), words[0], words[1],
          segment_base, counted_dma};
      const std::uint64_t key = block_key(nested);
      if (!active_blocks.insert(key).second)
        return reject(15U, traversal_address, words[0], words[1]);
      frames.push_back(nested);
    } else if (!frame.counted_dma && opcode == 0xB8U) {
      active_blocks.erase(block_key(frames.back()));
      frames.pop_back();
    }
  }
  return true;
}

bool execute_live_audio_task(State &state, hle::GuestMemory &memory) {
  std::array<std::byte, 64U> descriptor{};
  for (std::size_t offset = 0U; offset < descriptor.size(); offset += 4U) {
    std::uint32_t word = 0U;
    if (!memory.read_u32(state.loaded_rsp_task +
                             static_cast<std::uint32_t>(offset),
                         word))
      return false;
    descriptor[offset] = static_cast<std::byte>(word >> 24U);
    descriptor[offset + 1U] = static_cast<std::byte>(word >> 16U);
    descriptor[offset + 2U] = static_cast<std::byte>(word >> 8U);
    descriptor[offset + 3U] = static_cast<std::byte>(word);
  }
  std::uint32_t program_address = 0U, program_size = 0U,
                data_address = 0U, data_size = 0U,
                commands_address = 0U, commands_size = 0U;
  if (!memory.read_u32(state.loaded_rsp_task + 0x10U, program_address) ||
      !memory.read_u32(state.loaded_rsp_task + 0x14U, program_size) ||
      !memory.read_u32(state.loaded_rsp_task + 0x18U, data_address) ||
      !memory.read_u32(state.loaded_rsp_task + 0x1CU, data_size) ||
      !memory.read_u32(state.loaded_rsp_task + 0x30U, commands_address) ||
      !memory.read_u32(state.loaded_rsp_task + 0x34U, commands_size) ||
      program_size == 0U ||
      program_size > jfg::kMaximumAudioActiveProgramBytes ||
      data_size == 0U || data_size > jfg::kMaximumAudioProgramDataBytes ||
      commands_size == 0U ||
      commands_size > jfg::kMaximumAudioCommandStreamBytes ||
      program_size % jfg::kAudioInstructionAlignmentBytes != 0U ||
      commands_size % jfg::kAudioCommandAlignmentBytes != 0U)
    return false;
  std::vector<std::byte> program, program_data, commands;
  if (!guest_task_blob(state, program_address, program_size, program) ||
      !guest_task_blob(state, data_address, data_size, program_data) ||
      !guest_task_blob(state, commands_address, commands_size, commands))
    return false;
  std::unique_ptr<jfg::evidence::G2PrivateAudioProgram> generated_program(
      jfg_g2_private_audio_program_v2(
          {descriptor, program, program_data, 0U, 4U}));
  if (!generated_program)
    return false;
  LiveAudioMemory live_memory(state.rdram);
  const jfg::AudioBackendTaskView task{
      jfg::AudioProgramVariant::primary,
      program,
      program_data,
      commands,
      1U,
      1U,
      4U,
  };
  // Opt-in, bounded private DSP fixtures. Normal playback never copies RAM.
  struct AudioTaskCaptureConfig {
    std::filesystem::path root;
    std::uint64_t begin = 0U;
    std::uint64_t count = 1U;
  };
  static const AudioTaskCaptureConfig capture_config = [] {
    AudioTaskCaptureConfig config;
    auto read = [](const char* name) {
      char* value = nullptr; std::size_t size = 0U; std::string result;
      if (_dupenv_s(&value, &size, name) == 0 && value != nullptr) result = value;
      std::free(value); return result;
    };
    config.root = read("JFG_PHASE8_AUDIO_TASK_CAPTURE");
    if (!config.root.empty()) {
      const auto begin = read("JFG_PHASE8_AUDIO_TASK_CAPTURE_BEGIN");
      const auto count = read("JFG_PHASE8_AUDIO_TASK_CAPTURE_COUNT");
      if (!begin.empty()) config.begin = std::strtoull(begin.c_str(), nullptr, 10);
      if (!count.empty()) config.count = (std::min)(std::uint64_t{8U},
          static_cast<std::uint64_t>(std::strtoull(count.c_str(), nullptr, 10)));
    }
    return config;
  }();
  const bool capture_task = !capture_config.root.empty() &&
      state.decoded_audio_tasks >= capture_config.begin &&
      state.decoded_audio_tasks - capture_config.begin < capture_config.count;
  std::filesystem::path capture_task_root;
  auto write_fixture = [&](const char* name, const void* bytes, std::size_t size) {
    std::ofstream file(capture_task_root / name, std::ios::binary | std::ios::trunc);
    file.write(static_cast<const char*>(bytes), static_cast<std::streamsize>(size));
    return static_cast<bool>(file);
  };
  if (capture_task) {
    capture_task_root = capture_config.root / std::to_string(state.decoded_audio_tasks);
    std::error_code error;
    std::filesystem::create_directories(capture_task_root, error);
    if (error || !write_fixture("before.bin", state.rdram, kRdramSize) ||
        !write_fixture("descriptor.bin", descriptor.data(), descriptor.size())) return false;
    std::ofstream(capture_task_root / "vi.txt") << state.vi_retraces;
  }
  const jfg::AudioRspProgramReport report =
      generated_program->execute(task, live_memory);
  if (capture_task && !write_fixture("after.bin", state.rdram, kRdramSize)) return false;
  if (report.exit_reason != jfg::AudioRspExitReason::broke ||
      !report.parse_complete ||
      report.selected_variant != jfg::AudioProgramVariant::primary ||
      report.unsupported_command_count != 0U ||
      report.parsed_command_count !=
          commands.size() / jfg::kAudioCommandAlignmentBytes ||
      report.input_read_operation_count == 0U ||
      report.region_write_operation_count != live_memory.write_operations() ||
      report.region_bytes_written != live_memory.written_bytes())
    return false;
  ++state.decoded_audio_tasks;
  return true;
}

#if defined(_WIN32)
bool initialize_live_renderer(State &state, LiveRt64Window &window) {
  if (state.rdram == nullptr || state.rom == nullptr ||
      state.rom_size < jfg::kRt64RequiredHeaderBytes ||
      window.get() == nullptr)
    return false;
  state.rt64_rdram.assign(jfg::kRt64RequiredRdramBytes, std::byte{0});
  std::copy_n(reinterpret_cast<const std::byte *>(state.rdram), kRdramSize,
              state.rt64_rdram.begin());
  state.rt64_vi.status = 0x0000320EU;
  state.rt64_vi.width = 320U;
  state.rt64_vi.intr = 2U;
  state.rt64_vi.timing = 0x03E52239U;
  state.rt64_vi.vertical_sync = 525U;
  state.rt64_vi.horizontal_sync = 0x00000C15U;
  state.rt64_vi.leap = 0x0C150C15U;
  state.rt64_vi.horizontal_start = 0x006C02ECU;
  state.rt64_vi.vertical_start = 0x002501FFU;
  state.rt64_vi.vertical_burst = 0x000E0204U;
  state.rt64_vi.x_scale = 0x00000200U;
  state.rt64_vi.y_scale = 0x00000400U;
  jfg::Rt64ShellError error = jfg::Rt64ShellError::none;
  const int saved_stdout = _dup(_fileno(stdout));
  HANDLE null_handle = CreateFileW(
      L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  const int null_stdout =
      null_handle == INVALID_HANDLE_VALUE
          ? -1
          : _open_osfhandle(reinterpret_cast<std::intptr_t>(null_handle),
                            _O_WRONLY);
  if (null_stdout < 0 && null_handle != INVALID_HANDLE_VALUE)
    CloseHandle(null_handle);
  if (saved_stdout < 0 || null_stdout < 0 ||
      _dup2(null_stdout, _fileno(stdout)) != 0) {
    if (saved_stdout >= 0)
      _close(saved_stdout);
    if (null_stdout >= 0)
      _close(null_stdout);
    return false;
  }
  // Opt-in RT64 developer inspector (press F1 in game): JFG_RT64_DEVELOPER=1.
  char rt64_developer_flag[2]{};
  const bool rt64_developer_mode =
      GetEnvironmentVariableA("JFG_RT64_DEVELOPER", rt64_developer_flag, 2U) == 1U &&
      rt64_developer_flag[0] == '1';
  const auto video_settings = []() -> std::optional<jfg::VideoSettings> {
    wchar_t* config_path = nullptr;
    std::size_t size = 0;
    (void)_wdupenv_s(&config_path, &size, L"JFG_VIDEO_CONFIG");
    auto settings = jfg::load_video_settings(config_path == nullptr ? std::filesystem::path{} : std::filesystem::path(config_path));
    std::free(config_path);
    if (!settings) return std::nullopt;
    char* text = nullptr;
    (void)_dupenv_s(&text, &size, "JFG_EXPERIMENTAL_RENDER_FPS");
    if (text != nullptr) {
      const std::string value(text);
      std::free(text);
      if (value == "unlimited") settings->mode = jfg::FrameRateMode::unlimited;
      else {
        const auto fps = jfg::parse_render_fps(value);
        if (!fps) return std::nullopt;
        settings->mode = *fps == 0 ? jfg::FrameRateMode::original : jfg::FrameRateMode::custom;
        if (*fps) settings->fps = *fps;
      }
    }
    text = nullptr;
    (void)_dupenv_s(&text, &size, "JFG_RENDER_VSYNC");
    if (text != nullptr) {
      const std::string value(text);
      std::free(text);
      if (value != "0" && value != "1") return std::nullopt;
      settings->vsync = value == "1";
    }
    return settings;
  }();
  state.presentation_identity_enabled = video_settings &&
      video_settings->mode != jfg::FrameRateMode::original;
  if (!video_settings) {
    (void)_dup2(saved_stdout, _fileno(stdout));
    _close(null_stdout);
    _close(saved_stdout);
    std::fprintf(stderr, "Invalid video settings: FPS must be 0, 60-1000 or unlimited; VSync must be 0 or 1. Check JFG_VIDEO_CONFIG.\n");
    return false;
  }
  state.rt64_shell = jfg::Rt64Shell::create(
      {window.get(), window.thread_id(),
       std::span<const std::byte>(
           reinterpret_cast<const std::byte *>(state.rom),
           jfg::kRt64RequiredHeaderBytes),
       state.rt64_rdram, &state.rt64_vi, rt64_developer_mode,
       // Software overlays read and modify completed color pixels.
       // RT64 already completes its RAM readback before DP completion.
       jfg::Rt64MemoryLayout::host_word_swapped, true, true,
       video_settings->mode == jfg::FrameRateMode::custom ? video_settings->fps : 0U,
       video_settings->mode == jfg::FrameRateMode::unlimited, video_settings->vsync},
      error);
  std::fflush(stdout);
  (void)_dup2(saved_stdout, _fileno(stdout));
  _close(null_stdout);
  _close(saved_stdout);
  state.last_rt64_error = error;
  return state.rt64_shell != nullptr && error == jfg::Rt64ShellError::none;
}

constexpr bool is_immediate_offscreen_task_candidate(
    const std::uint32_t command_size) noexcept {
  // Both observed tiny actor passes finish within the same VI in BizHawk.
  // The renderer still verifies that their color target is not displayed.
  return command_size == 0x1BU || command_size == 0x1FU;
}

bool execute_live_graphics_task(State &state, hle::GuestMemory &memory) {
  if (state.rt64_shell == nullptr ||
      state.rt64_rdram.size() != jfg::kRt64RequiredRdramBytes)
    return false;
  std::uint32_t ucode_address = 0U, ucode_size = 0U,
                ucode_data_address = 0U, ucode_data_size = 0U,
                command_address = 0U, command_size = 0U;
  if (!memory.read_u32(state.loaded_rsp_task + 0x10U, ucode_address) ||
      !memory.read_u32(state.loaded_rsp_task + 0x14U, ucode_size) ||
      !memory.read_u32(state.loaded_rsp_task + 0x18U, ucode_data_address) ||
      !memory.read_u32(state.loaded_rsp_task + 0x1CU, ucode_data_size) ||
      !memory.read_u32(state.loaded_rsp_task + 0x30U, command_address) ||
      !memory.read_u32(state.loaded_rsp_task + 0x34U, command_size))
    return false;
  state.last_graphics_descriptor = {ucode_address, ucode_size,
                                    ucode_data_address, ucode_data_size,
                                    command_address, command_size};
  const auto physical_range = [](const std::uint32_t address,
                                 const std::uint32_t bytes) {
    const std::uint32_t physical = address & 0x00FFFFFFU;
    return physical <= kRdramSize && bytes <= kRdramSize - physical;
  };
  // libultra does not require graphics tasks to populate ucode_size, and the
  // game uses data_size as a display-list count for some tiny setup tasks.
  // RT64 consumes the addresses and terminates on the GBI end command, so
  // validate the fixed apertures that its shell will inspect instead.
  if (!physical_range(ucode_address, 4096U) ||
      !physical_range(ucode_data_address, 2048U) ||
      !physical_range(command_address, sizeof(std::uint64_t)))
    return false;
  state.last_graphics_command_address = command_address;
  if (!memory.read_u32(command_address, state.last_graphics_command_word))
    return false;
  const auto community_prepare_start = std::chrono::steady_clock::now();
  std::vector<std::byte> snapshot;
  std::vector<std::uint8_t> community_dirty;
  if (!state.graphics_snapshot_pool.empty()) {
    snapshot = std::move(state.graphics_snapshot_pool.back());
    state.graphics_snapshot_pool.pop_back();
    if (!state.community_snapshot_pool_dirty.empty()) {
      community_dirty = std::move(state.community_snapshot_pool_dirty.back());
      state.community_snapshot_pool_dirty.pop_back();
    }
  }
  // A snapshot without matching flags (new, or pools out of step) is
  // restored in full by materialize (flags of the wrong size = all dirty).
  if (state.community_snapshot_pool_dirty.size() != state.graphics_snapshot_pool.size())
    state.community_snapshot_pool_dirty.clear();
  snapshot.resize(jfg::kRt64RequiredRdramBytes);
  PendingLiveGraphicsTask pending{
      {ucode_address, ucode_data_address, command_address, state.vi_retraces},
      state.last_graphics_descriptor, std::move(snapshot)};
  jfg::copy_changed_snapshot_bytes(std::span(reinterpret_cast<const std::byte *>(state.rdram), kRdramSize),
      std::span(pending.rdram).first(kRdramSize));
  if (!materialize_live_graphics_overlays(
          state, pending.rdram, command_address, command_size,
          state.community_ranged_import ? &community_dirty : nullptr)) {
    (void)write_private_rdram_capture(state);
    (void)write_private_rt64_snapshot(state, pending.rdram);
    return false;
  }
  if (state.presentation_identity_enabled)
    pending.task.presentation_scene = std::make_shared<const jfg::PresentationSceneIdentity>(
        state.presentation_identities.capture(
            {reinterpret_cast<const std::uint8_t*>(pending.rdram.data()),pending.rdram.size()},
            state.vi_retraces));
  if (state.community_ranged_import)
    pending.community_upper_dirty = std::move(community_dirty);
  state.pending_graphics_tasks.push_back(std::move(pending));
  state.community_prof_prepare_us += static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - community_prepare_start).count());
  ++state.community_prof_tasks;
  return true;
}
#endif
#endif

void hash_u32(jfg::testkernel::Sha256 &hasher, const std::uint32_t value) {
  const std::array<std::uint8_t, 4U> bytes = {
      static_cast<std::uint8_t>(value >> 24U),
      static_cast<std::uint8_t>(value >> 16U),
      static_cast<std::uint8_t>(value >> 8U),
      static_cast<std::uint8_t>(value)};
  hasher.update_bytes(bytes.data(), bytes.size());
}

void hash_u64(jfg::testkernel::Sha256 &hasher, const std::uint64_t value) {
  hash_u32(hasher, static_cast<std::uint32_t>(value >> 32U));
  hash_u32(hasher, static_cast<std::uint32_t>(value));
}

std::string journal_hash(const State &state) {
  jfg::testkernel::Sha256 hasher;
  static constexpr std::string_view domain = "jfg-phase6-native-journal-v1";
  hasher.update_bytes(domain.data(), domain.size());
  hash_u64(hasher, state.journal.count);
  for (std::size_t index = 0U; index < state.journal.count; ++index) {
    const NativeEvent &event = state.journal.entries[index];
    hash_u32(hasher, static_cast<std::uint32_t>(event.kind));
    hash_u32(hasher, event.a);
    hash_u32(hasher, event.b);
    hash_u32(hasher, event.c);
  }
  return jfg::testkernel::hex_digest(hasher.finish());
}

std::string mmio_trace_text(const State &state) {
  std::string trace;
  for (std::size_t index = 0U; index < state.journal.count; ++index) {
    const NativeEvent &event = state.journal.entries[index];
    if (event.kind != NativeEventKind::kMmioRead &&
        event.kind != NativeEventKind::kMmioWrite)
      continue;
    if (!trace.empty())
      trace.push_back(',');
    char entry[13U]{};
    (void)std::snprintf(entry, sizeof(entry), "%c:0x%08x",
                        event.kind == NativeEventKind::kMmioWrite ? 'w' : 'r',
                        event.a);
    trace += entry;
  }
  return trace;
}

std::string state_hash(const State &state) {
  jfg::testkernel::Sha256 hasher;
  static constexpr std::string_view domain = "jfg-phase6-native-state-v1";
  hasher.update_bytes(domain.data(), domain.size());
  hasher.update_bytes(state.rdram, kRdramSize);
  hash_u32(hasher, state.root_priority);
  hash_u32(hasher, state.interrupt_mask);
  hash_u32(hasher, state.vi_mode);
  hash_u32(hasher, state.vi_queue);
  hash_u32(hasher, state.vi_message);
  hash_u32(hasher, state.vi_retrace_interval);
  hash_u32(hasher, state.vi_current_framebuffer);
  hash_u32(hasher, state.vi_next_framebuffer);
  hash_u32(hasher, state.vi_special_features);
  hash_u32(hasher, state.vi_blacked ? 1U : 0U);
  hash_u64(hasher, state.vi_frames);
  hash_u64(hasher, state.vi_interrupts);
  hash_u64(hasher, state.vi_messages_delivered);
  hash_u64(hasher, state.vi_retraces);
  hash_u64(hasher, state.scheduler == nullptr
                       ? 0U
                       : state.scheduler->dispatch_count());
  hash_u64(hasher, state.thread_order.size());
  for (const auto &[guest, id] : state.thread_order) {
    hash_u32(hasher, guest);
    hash_u32(hasher, static_cast<std::uint32_t>(id));
    const auto snapshot = state.scheduler->snapshot_of(id);
    hash_u32(hasher, snapshot.has_value() ? snapshot->priority : 0U);
    hash_u32(hasher, snapshot.has_value()
                         ? static_cast<std::uint32_t>(snapshot->state)
                         : 0U);
    hash_u32(hasher, snapshot.has_value() ? snapshot->blocked_queue : 0U);
  }
  return jfg::testkernel::hex_digest(hasher.finish());
}

void ledger(State &state, std::string_view category, std::string_view operation,
            std::uint32_t guest_target) {
  if (!state.ledgered) {
    state.ledgered = true;
    jfg::support_event("failure=" + std::string(category) + "/" + std::string(operation));
    std::fprintf(stdout,
                 "{\"kind\":\"jfg-phase6-native-first-trap\",\"category\":\"%.*"
                 "s\",\"guest_target\":\"0x%08x\",\"operation\":\"%.*s\","
                 "\"disposition\":\"fail-closed-trap\",\"count\":1}\n",
                 static_cast<int>(category.size()), category.data(),
                 guest_target, static_cast<int>(operation.size()),
                 operation.data());
    std::fflush(stdout);
  }
}

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
// Community crash report (2026-10-11). The jfg-crash-site console line names
// only the host code location. This also writes
// %LOCALAPPDATA%\JFGRecomp\diag\crash-report.txt with what the game was doing:
// the last guest instruction, the guest address behind an access violation,
// the registers of the running guest thread and the last guest calls. Guest
// RAM is saved next to it as crash-rdram.bin (4 MiB, 32-bit words in host
// byte order). Local diagnostics only; both files are overwritten per crash.
void community_crash_where(const std::uintptr_t address, char *out,
                           const std::size_t size) noexcept {
  HMODULE owner = nullptr;
  char path[MAX_PATH]{};
  if (address != 0U &&
      GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCSTR>(address), &owner) &&
      owner != nullptr && GetModuleFileNameA(owner, path, MAX_PATH) != 0U) {
    const char *base = path;
    for (const char *c = path; *c != '\0'; ++c)
      if (*c == '\\' || *c == '/')
        base = c + 1;
    (void)std::snprintf(out, size, "%s+%llx", base,
                        static_cast<unsigned long long>(
                            address - reinterpret_cast<std::uintptr_t>(owner)));
  } else {
    (void)std::snprintf(out, size, "?%llx",
                        static_cast<unsigned long long>(address));
  }
}

HANDLE community_crash_open(const wchar_t *name) noexcept {
  wchar_t base[MAX_PATH]{};
  const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
  if (length == 0U || length >= MAX_PATH)
    return INVALID_HANDLE_VALUE;
  wchar_t path[MAX_PATH + 64]{};
  (void)std::swprintf(path, MAX_PATH + 64, L"%ls\\JFGRecomp", base);
  (void)CreateDirectoryW(path, nullptr);
  (void)std::swprintf(path, MAX_PATH + 64, L"%ls\\JFGRecomp\\diag", base);
  (void)CreateDirectoryW(path, nullptr);
  (void)std::swprintf(path, MAX_PATH + 64, L"%ls\\JFGRecomp\\diag\\%ls", base,
                      name);
  return CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

void community_crash_text(const HANDLE file, const char *text) noexcept {
  DWORD written = 0U;
  (void)WriteFile(file, text, static_cast<DWORD>(std::strlen(text)), &written,
                  nullptr);
}

void community_crash_report(const State &state,
                            const EXCEPTION_POINTERS *exception) noexcept {
  static LONG entered = 0;
  if (InterlockedCompareExchange(&entered, 1, 0) != 0)
    return;
  const EXCEPTION_RECORD *const record =
      exception != nullptr ? exception->ExceptionRecord : nullptr;
  const auto guest_base = reinterpret_cast<std::uintptr_t>(state.rdram);
  const char *access = "none";
  std::uintptr_t data = 0U;
  bool guest_data = false;
  std::uint32_t guest_address = 0U;
  if (record != nullptr &&
      record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
      record->NumberParameters >= 2U) {
    access = record->ExceptionInformation[0] == 1U   ? "write"
             : record->ExceptionInformation[0] == 8U ? "execute"
                                                     : "read";
    data = static_cast<std::uintptr_t>(record->ExceptionInformation[1]);
    if (guest_base != 0U && data >= guest_base &&
        data - guest_base < kGuestAddressSpan) {
      // Inverse of MEM_W: host = rdram + (guest - 0xFFFFFFFF80000000).
      guest_data = true;
      guest_address = static_cast<std::uint32_t>(
          static_cast<std::uint64_t>(data - guest_base) + 0x80000000ULL);
    }
  }
  char line[512]{};
  char guest_text[24]{};
  if (guest_data)
    (void)std::snprintf(guest_text, sizeof(guest_text), "0x%08x",
                        guest_address);
  else
    (void)std::snprintf(guest_text, sizeof(guest_text), "outside");
  (void)std::snprintf(
      line, sizeof(line),
      "{\"kind\":\"jfg-crash-detail\",\"guest_pc\":\"0x%08x\","
      "\"access\":\"%s\",\"guest_address\":\"%s\","
      "\"report\":\"%%LOCALAPPDATA%%\\\\JFGRecomp\\\\diag\\\\crash-report.txt\"}\n",
      static_cast<unsigned>(g_community_crash_pc), access, guest_text);
  std::fputs(line, stderr);
  std::fflush(stderr);

  const HANDLE file = community_crash_open(L"crash-report.txt");
  if (file != INVALID_HANDLE_VALUE) {
    SYSTEMTIME utc{};
    GetSystemTime(&utc);
    (void)std::snprintf(line, sizeof(line),
                        "JFG community crash report\n"
                        "utc=%04u-%02u-%02uT%02u:%02u:%02uZ\n",
                        utc.wYear, utc.wMonth, utc.wDay, utc.wHour,
                        utc.wMinute, utc.wSecond);
    community_crash_text(file, line);
    char where[192]{};
    community_crash_where(
        record != nullptr
            ? reinterpret_cast<std::uintptr_t>(record->ExceptionAddress)
            : 0U,
        where, sizeof(where));
    (void)std::snprintf(line, sizeof(line), "code=0x%08lx\nat=%s\n",
                        record != nullptr
                            ? static_cast<unsigned long>(record->ExceptionCode)
                            : 0UL,
                        where);
    community_crash_text(file, line);
    (void)std::snprintf(line, sizeof(line),
                        "access=%s host_address=0x%llx guest_address=%s\n",
                        access, static_cast<unsigned long long>(data),
                        guest_text);
    community_crash_text(file, line);
    (void)std::snprintf(line, sizeof(line),
                        "guest_pc=0x%08x (timing model: pc 0x%08x word 0x%08x)\n",
                        static_cast<unsigned>(g_community_crash_pc),
                        state.guest_last_pc, state.guest_last_word);
    community_crash_text(file, line);
    community_crash_text(file, "guest_calls_oldest_first=");
    const std::size_t ring = state.recent_dispatches.size();
    const std::size_t count = (std::min)(state.recent_dispatch_position, ring);
    for (std::size_t index = 0U; index < count; ++index) {
      (void)std::snprintf(
          line, sizeof(line), " %08x",
          state.recent_dispatches[(state.recent_dispatch_position - count +
                                   index) %
                                  ring]);
      community_crash_text(file, line);
    }
    community_crash_text(file, "\n");
    const recomp_context *const cpu = g_community_crash_context;
    if (cpu != nullptr) {
      const gpr registers[32] = {
          cpu->r0,  cpu->r1,  cpu->r2,  cpu->r3,  cpu->r4,  cpu->r5,
          cpu->r6,  cpu->r7,  cpu->r8,  cpu->r9,  cpu->r10, cpu->r11,
          cpu->r12, cpu->r13, cpu->r14, cpu->r15, cpu->r16, cpu->r17,
          cpu->r18, cpu->r19, cpu->r20, cpu->r21, cpu->r22, cpu->r23,
          cpu->r24, cpu->r25, cpu->r26, cpu->r27, cpu->r28, cpu->r29,
          cpu->r30, cpu->r31};
      community_crash_text(file, "guest_registers (latest guest thread)\n");
      for (unsigned index = 0U; index < 32U; ++index) {
        (void)std::snprintf(line, sizeof(line), "r%u=%08x%s", index,
                            static_cast<unsigned>(registers[index]),
                            index % 8U == 7U ? "\n" : " ");
        community_crash_text(file, line);
      }
    }
    community_crash_text(file, "stack=");
    void *frames[32]{};
    const USHORT frame_count = RtlCaptureStackBackTrace(0U, 32U, frames, nullptr);
    for (USHORT index = 0U; index < frame_count; ++index) {
      community_crash_where(reinterpret_cast<std::uintptr_t>(frames[index]),
                            where, sizeof(where));
      (void)std::snprintf(line, sizeof(line), " %s", where);
      community_crash_text(file, line);
    }
    community_crash_text(
        file, "\nrdram=crash-rdram.bin (4 MiB guest RAM, 32-bit words in host "
              "byte order)\n");
    (void)FlushFileBuffers(file);
    CloseHandle(file);
  }
  if (state.rdram != nullptr) {
    const HANDLE memory = community_crash_open(L"crash-rdram.bin");
    if (memory != INVALID_HANDLE_VALUE) {
      DWORD written = 0U;
      (void)WriteFile(memory, state.rdram, static_cast<DWORD>(kRdramSize),
                      &written, nullptr);
      (void)FlushFileBuffers(memory);
      CloseHandle(memory);
    }
  }
}

LONG WINAPI child_exception_filter(EXCEPTION_POINTERS *exception);

// Explicit private diagnostics only: abort otherwise bypasses the unhandled
// exception filter and loses the guest state at a fail-closed switch trap.
void private_abort_capture(int signal_number) {
  (void)signal_number;
  CONTEXT private_abort_context{};
  RtlCaptureContext(&private_abort_context);
  EXCEPTION_RECORD private_abort_record{};
  private_abort_record.ExceptionCode=0xe04a4647U;
  private_abort_record.ExceptionAddress=
      reinterpret_cast<void*>(private_abort_context.Rip);
  EXCEPTION_POINTERS private_abort_pointers{&private_abort_record,&private_abort_context};
  (void)child_exception_filter(&private_abort_pointers);
}

constexpr std::array<std::uint32_t,10> kPrivatePageWatchWords{
    0xf5b0cU,0xaef50U,0xaef54U,0xaef58U,0xaef5cU,
    0xaef60U,0xaef64U,0xaef68U,0xaef6cU,0xaef70U};
struct PrivatePageWriteStep {
  void* page = nullptr;
  std::uintptr_t instruction = 0U;
  std::array<std::uint32_t,10> before{};
};
thread_local PrivatePageWriteStep private_page_write_step{};
bool arm_private_memory_pages(State& state) {
  const auto base=reinterpret_cast<std::uintptr_t>(state.rdram);
  constexpr std::array<std::uintptr_t,4> offsets{0xae000U,0xf5000U,0x200ae000U,0x200f5000U};
  for (const auto offset:offsets) {
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(reinterpret_cast<void*>(base+offset),&info,sizeof(info))==0U ||
        info.State!=MEM_COMMIT || info.Protect!=PAGE_READWRITE)return false;
  }
  std::size_t protected_pages=0U;
  for (const auto offset:offsets) {
    DWORD previous=0U;
    if (!VirtualProtect(reinterpret_cast<void*>(base+offset),4096U,PAGE_READONLY,&previous)) {
      for (std::size_t restore=0U;restore<protected_pages;++restore)
        (void)VirtualProtect(reinterpret_cast<void*>(base+offsets[restore]),4096U,PAGE_READWRITE,&previous);
      return false;
    }
    ++protected_pages;
  }
  state.private_model_pages_ready=true;
  FILE* page_watch_receipt=nullptr;
  (void)fopen_s(&page_watch_receipt,(state.rdram_capture_path+".watch-setup.txt").c_str(),"ab");
  if (page_watch_receipt!=nullptr) {
    std::fprintf(page_watch_receipt,"page-watch cached-and-uncached pages=4 words=10 vi=%llu\n",
                 static_cast<unsigned long long>(state.vi_retraces));
    std::fclose(page_watch_receipt);
  }
  return true;
}
LONG CALLBACK private_page_write_observer(EXCEPTION_POINTERS* exception) {
  auto* const state=g_active_child_state;
  if (state==nullptr || !state->private_model_page_watch || exception==nullptr ||
      exception->ExceptionRecord==nullptr || exception->ContextRecord==nullptr)
    return EXCEPTION_CONTINUE_SEARCH;
  auto& record=*exception->ExceptionRecord;
  auto& host=*exception->ContextRecord;
  auto& pending=private_page_write_step;
  if (record.ExceptionCode==EXCEPTION_SINGLE_STEP && pending.page!=nullptr) {
    DWORD previous=0U;
    if (!VirtualProtect(pending.page,4096U,PAGE_READONLY,&previous))
      fail_closed_dispatch(*state,"private-memory-watch","page-reprotect",state->guest_last_pc);
    pending.page=nullptr;
    host.EFlags&=~0x100U;
    for (std::size_t watch_index=0U;watch_index<kPrivatePageWatchWords.size();++watch_index) {
      std::uint32_t current=0U;
      std::memcpy(&current,state->rdram+kPrivatePageWatchWords[watch_index],4U);
      const auto before=pending.before[watch_index];
      const bool invalid=watch_index==0U
          ? before>=0x80000000U && before<0x80400000U && current!=0U &&
            (current<0x80000000U || current>=0x80400000U)
          : current!=before;
      if (invalid) {
        state->private_model_pointer=before;
        state->private_model_write_offset=kPrivatePageWatchWords[watch_index];
        state->private_memory_writer_pc=pending.instruction;
        state->private_model_write_trap=true;
        return child_exception_filter(exception);
      }
    }
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  if (record.ExceptionCode!=EXCEPTION_ACCESS_VIOLATION ||
      record.NumberParameters<2U || record.ExceptionInformation[0]!=1U)
    return EXCEPTION_CONTINUE_SEARCH;
  const auto address=record.ExceptionInformation[1];
  const auto base=reinterpret_cast<std::uintptr_t>(state->rdram);
  if (address<base)return EXCEPTION_CONTINUE_SEARCH;
  const auto mapped_offset=address-base;
  if (!(mapped_offset<kRdramSize ||
        (mapped_offset>=0x20000000U && mapped_offset<0x20000000U+kRdramSize)))
    return EXCEPTION_CONTINUE_SEARCH;
  const auto physical_page=(mapped_offset&0x1fffffffU)&~std::uintptr_t{4095U};
  if (physical_page!=0xae000U && physical_page!=0xf5000U)
    return EXCEPTION_CONTINUE_SEARCH;
  if (pending.page!=nullptr || (host.EFlags&0x100U)!=0U)
    fail_closed_dispatch(*state,"private-memory-watch","page-step-overlap",state->guest_last_pc);
  for (std::size_t watch_index=0U;watch_index<kPrivatePageWatchWords.size();++watch_index)
    std::memcpy(&pending.before[watch_index],state->rdram+kPrivatePageWatchWords[watch_index],4U);
  pending.page=reinterpret_cast<void*>(address&~std::uintptr_t{4095U});
  pending.instruction=host.Rip;
  DWORD previous=0U;
  if (!VirtualProtect(pending.page,4096U,PAGE_READWRITE,&previous))
    fail_closed_dispatch(*state,"private-memory-watch","page-write-enable",state->guest_last_pc);
  host.EFlags|=0x100U;
  return EXCEPTION_CONTINUE_EXECUTION;
}

thread_local volatile bool private_model_watch_armed = false;
constexpr std::array<std::uint32_t,4> kPrivateMemoryWatchOffsets{0xf5b0cU,0xaef50U,0xaef5cU,0xaef70U};
thread_local std::array<std::uint32_t,4> private_model_watch_previous{};
bool arm_private_model_write_watch(State& state) {
  const DWORD requesting_thread=GetCurrentThreadId();
  const auto watched_address=reinterpret_cast<DWORD64>(state.rdram+0xf5b0cU);
  bool configured=false;
  DWORD watch_error=0U;
  DWORD64 original_dr0=0U,original_dr7=0U;
  std::thread private_watch_helper([&] {
    HANDLE requesting_handle=OpenThread(
        THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT,FALSE,requesting_thread);
    if (requesting_handle==nullptr) { watch_error=GetLastError();return; }
    CONTEXT debug_context{};
    debug_context.ContextFlags=CONTEXT_DEBUG_REGISTERS;
    if (SuspendThread(requesting_handle)!=DWORD(-1)) {
      if (GetThreadContext(requesting_handle,&debug_context)) {
        original_dr0=debug_context.Dr0;original_dr7=debug_context.Dr7;
        if ((debug_context.Dr7&255U)==0U) {
          debug_context.Dr0=watched_address;
          debug_context.Dr1=reinterpret_cast<DWORD64>(state.rdram+kPrivateMemoryWatchOffsets[1]);
          debug_context.Dr2=reinterpret_cast<DWORD64>(state.rdram+kPrivateMemoryWatchOffsets[2]);
          debug_context.Dr3=reinterpret_cast<DWORD64>(state.rdram+kPrivateMemoryWatchOffsets[3]);
          debug_context.Dr6=0U;
          debug_context.Dr7=(debug_context.Dr7&~DWORD64{0xffff00ffU})|0xdddd0055U;
          configured=SetThreadContext(requesting_handle,&debug_context)!=FALSE;
          if (!configured) watch_error=GetLastError();
        } else watch_error=ERROR_BUSY;
      } else watch_error=GetLastError();
      // Do not perform allocation, logging, or other host work while this
      // requesting thread is suspended. Only the kernel context operations run.
      if (ResumeThread(requesting_handle)==DWORD(-1)) std::_Exit(4);
    } else watch_error=GetLastError();
    CloseHandle(requesting_handle);
  });
  private_watch_helper.join();
  FILE* private_watch_setup_log=nullptr;
  (void)fopen_s(&private_watch_setup_log,(state.rdram_capture_path+".watch-setup.txt").c_str(),"ab");
  if (private_watch_setup_log!=nullptr) {
    std::fprintf(private_watch_setup_log,
        "thread=%lu original_dr0=%llx original_dr7=%llx configured=%u error=%lu watched=%llx\n",
        requesting_thread,original_dr0,original_dr7,configured?1U:0U,watch_error,watched_address);
    std::fclose(private_watch_setup_log);
  }
  if (configured) {
    for (std::size_t watch_index=0;watch_index<kPrivateMemoryWatchOffsets.size();++watch_index)
      std::memcpy(&private_model_watch_previous[watch_index],
                  state.rdram+kPrivateMemoryWatchOffsets[watch_index],4U);
    private_model_watch_armed=true;
  }
  return configured;
}
LONG CALLBACK private_model_write_observer(EXCEPTION_POINTERS* exception) {
  auto* const state=g_active_child_state;
  if (state==nullptr || !state->private_model_hardware_watch ||
      exception==nullptr || exception->ExceptionRecord==nullptr ||
      exception->ContextRecord==nullptr) return EXCEPTION_CONTINUE_SEARCH;
  auto& host=*exception->ContextRecord;
  const auto code=exception->ExceptionRecord->ExceptionCode;
  if (code!=EXCEPTION_SINGLE_STEP || !private_model_watch_armed ||
      (host.Dr6&15U)==0U) return EXCEPTION_CONTINUE_SEARCH;
  const std::array<DWORD64,4> watched_addresses{host.Dr0,host.Dr1,host.Dr2,host.Dr3};
  for (std::size_t watch_index=0;watch_index<kPrivateMemoryWatchOffsets.size();++watch_index) {
    const DWORD64 watch_bit=DWORD64{1}<<watch_index;
    if ((host.Dr6&watch_bit)==0U)continue;
    const auto offset=kPrivateMemoryWatchOffsets[watch_index];
    if (watched_addresses[watch_index]!=reinterpret_cast<DWORD64>(state->rdram+offset))
      return EXCEPTION_CONTINUE_SEARCH;
    std::uint32_t current=0U;
    std::memcpy(&current,state->rdram+offset,4U);
    const auto previous=private_model_watch_previous[watch_index];
    private_model_watch_previous[watch_index]=current;
    host.ContextFlags|=CONTEXT_DEBUG_REGISTERS;host.Dr6&=~watch_bit;
    const bool invalid=watch_index==0U
        ? previous>=0x80000000U && previous<0x80400000U &&
          current!=0U && (current<0x80000000U || current>=0x80400000U)
        : current!=previous;
    if (invalid) {
      state->private_model_pointer=previous;
      state->private_model_write_offset=offset;
      state->private_model_write_trap=true;
      return child_exception_filter(exception);
    }
  }
  return EXCEPTION_CONTINUE_EXECUTION;
}
LONG WINAPI child_exception_filter(EXCEPTION_POINTERS *exception) {
  jfg::support_crash(exception);
  State *const state = g_active_child_state;
  if (state != nullptr) {
    community_crash_report(*state, exception);
    if (state->native_clock_sample_output.is_open())
      state->native_clock_sample_output.flush();
    const std::uint32_t target =
        state->recent_dispatch_position == 0U
            ? 0U
            : state->recent_dispatches[(state->recent_dispatch_position - 1U) %
                                       state->recent_dispatches.size()];
    const DWORD code =
        exception != nullptr && exception->ExceptionRecord != nullptr
            ? exception->ExceptionRecord->ExceptionCode
            : 0U;
    char support_exception[40]{};
    std::snprintf(support_exception, sizeof(support_exception), "native_exception=0x%08lx", static_cast<unsigned long>(code));
    jfg::support_event(support_exception);
    // Private, explicit RAM-capture sessions also retain the otherwise lost
    // native-fault context. Ordinary Play neither captures RAM nor logs host
    // addresses. This observes the failed execution; it never resumes it.
    if (!state->rdram_capture_path.empty()) {
      FILE* private_fault_capture_stream = nullptr;
      (void)fopen_s(&private_fault_capture_stream,
                   (state->rdram_capture_path+".fault.txt").c_str(),"wb");
      FILE* const private_fault_output = private_fault_capture_stream != nullptr
                                            ? private_fault_capture_stream : stderr;

      if (state->private_model_write_trap) {
        std::uint32_t after=0U;
        std::memcpy(&after,state->rdram+state->private_model_write_offset,4U);
        std::fprintf(private_fault_output,"private-memory-write method=%s address=%08x old=%08x new=%08x writer_pc=%llx\n",
                     state->private_model_page_watch ? "pages" : "hardware",
                     0x80000000U+state->private_model_write_offset,state->private_model_pointer,after,
                     static_cast<unsigned long long>(state->private_memory_writer_pc));
      }
      hle::GuestMemory memory({state->rdram, kRdramSize},
                             hle::GuestMemory::Layout::native_word_big_endian);
      std::uint32_t running_thread = 0U;
      const bool thread_read = memory.read_u32(0x800a9e90U, running_thread);
      const auto* record = exception != nullptr ? exception->ExceptionRecord : nullptr;
      const auto access_kind = record != nullptr && record->NumberParameters >= 2U
                                   ? record->ExceptionInformation[0] : 0U;
      const auto access_address = record != nullptr && record->NumberParameters >= 2U
                                      ? record->ExceptionInformation[1] : 0U;
      std::fprintf(private_fault_output,
          "private-native-fault vi=%llu poll=%llu epc=%08x last_pc=%08x last_word=%08x "
          "running_thread=%08x thread_read=%u access_kind=%llu access_address=%llx rdram=%llx\n",
          static_cast<unsigned long long>(state->vi_retraces),
          static_cast<unsigned long long>(state->controller_samples),
          state->guest_epc, state->guest_last_pc, state->guest_last_word,
          running_thread, thread_read ? 1U : 0U,
          static_cast<unsigned long long>(access_kind),
          static_cast<unsigned long long>(access_address),
          static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(state->rdram)));
#if defined(_M_X64) || defined(__x86_64__)
      if (exception != nullptr && exception->ContextRecord != nullptr) {
        const auto& host = *exception->ContextRecord;
        std::fprintf(private_fault_output,
            "private-native-registers rax=%llx rbx=%llx rcx=%llx rdx=%llx rsi=%llx rdi=%llx "
            "rbp=%llx rsp=%llx r8=%llx r9=%llx r10=%llx r11=%llx r12=%llx r13=%llx "
            "r14=%llx r15=%llx rip=%llx\n",
            host.Rax, host.Rbx, host.Rcx, host.Rdx, host.Rsi, host.Rdi, host.Rbp, host.Rsp,
            host.R8, host.R9, host.R10, host.R11, host.R12, host.R13, host.R14, host.R15, host.Rip);
      }
#endif
      if (state->private_fault_cpu != nullptr) {
        const auto& cpu = *state->private_fault_cpu;
        const std::array<std::uint64_t,32> registers{
            0, cpu.r1, cpu.r2, cpu.r3, cpu.r4, cpu.r5, cpu.r6, cpu.r7,
            cpu.r8, cpu.r9, cpu.r10, cpu.r11, cpu.r12, cpu.r13, cpu.r14, cpu.r15,
            cpu.r16, cpu.r17, cpu.r18, cpu.r19, cpu.r20, cpu.r21, cpu.r22, cpu.r23,
            cpu.r24, cpu.r25, cpu.r26, cpu.r27, cpu.r28, cpu.r29, cpu.r30, cpu.r31};
        std::fprintf(private_fault_output, "private-last-observed-cpu status=%08x fcr31=%08x",
                     cpu.status_reg, cpu.fcr31);
        for (std::size_t index=0; index<registers.size(); ++index)
          std::fprintf(private_fault_output, " r%zu=%016llx", index,
                       static_cast<unsigned long long>(registers[index]));
        std::fputc('\n', private_fault_output);
      }
      const bool captured = write_private_rdram_capture(*state);
      std::fprintf(private_fault_output, "private-native-rdram-captured=%u\n", captured ? 1U : 0U);
      std::fflush(private_fault_output);
      if (private_fault_capture_stream != nullptr) std::fclose(private_fault_capture_stream);
    }
    if (state->private_model_write_trap) {
      ledger(*state,"private-memory-watch","hardware-model-pointer",state->guest_last_pc);
      std::_Exit(4);
    }
    if (state->active_bzero_address != 0U) {
      std::array<char, 64U> operation{};
      (void)std::snprintf(operation.data(), operation.size(),
                          "bzero-%08x-%08x", state->active_bzero_address,
                          state->active_bzero_length);
      ledger(*state, "native-crash", operation.data(), target);
    } else {
      std::array<char, 64U> operation{};
      const auto module = reinterpret_cast<std::uintptr_t>(
          GetModuleHandleW(nullptr));
      const auto instruction =
          exception != nullptr && exception->ExceptionRecord != nullptr
              ? reinterpret_cast<std::uintptr_t>(
                    exception->ExceptionRecord->ExceptionAddress)
              : 0U;
      (void)std::snprintf(operation.data(), operation.size(),
                          code == EXCEPTION_ACCESS_VIOLATION
                              ? "access-rva-%08llx"
                              : "exception-rva-%08llx",
                          static_cast<unsigned long long>(instruction - module));
      // Name the module that faulted plus a short call stack, so crashes in
      // driver/system DLLs can be attributed (module+offset per frame).
      {
        auto describe = [](const std::uintptr_t address, char* out, const std::size_t size) {
          HMODULE owner = nullptr;
          char path[MAX_PATH]{};
          if (address != 0U &&
              GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                     GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                 reinterpret_cast<LPCSTR>(address), &owner) &&
              owner != nullptr &&
              GetModuleFileNameA(owner, path, MAX_PATH) != 0U) {
            const char* base = path;
            for (const char* c = path; *c != '\0'; ++c)
              if (*c == '\\' || *c == '/') base = c + 1;
            (void)std::snprintf(out, size, "%s+%llx", base,
                                static_cast<unsigned long long>(
                                    address - reinterpret_cast<std::uintptr_t>(owner)));
          } else {
            (void)std::snprintf(out, size, "?%llx",
                                static_cast<unsigned long long>(address));
          }
        };
        char where[160]{};
        describe(instruction, where, sizeof(where));
        std::fprintf(stderr, "{\"kind\":\"jfg-crash-site\",\"code\":\"0x%08lx\",\"at\":\"%s\",\"stack\":[",
                     static_cast<unsigned long>(code), where);
        void* frames[32]{};
        const USHORT count = RtlCaptureStackBackTrace(0U, 32U, frames, nullptr);
        for (USHORT i = 0U; i < count; ++i) {
          char frame[160]{};
          describe(reinterpret_cast<std::uintptr_t>(frames[i]), frame, sizeof(frame));
          std::fprintf(stderr, "%s\"%s\"", i == 0U ? "" : ",", frame);
        }
        std::fprintf(stderr, "]}\n");
        std::fflush(stderr);
      }
      ledger(*state, "native-crash", operation.data(), target);
    }
  }
  std::_Exit(4);
}
#endif

[[noreturn]] void fail_closed_dispatch(State &state, std::string_view category,
                                       std::string_view operation,
                                       std::uint32_t guest_target) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (state.input_record.is_open())
    state.input_record.flush();
  if (state.native_clock_sample_output.is_open())
    state.native_clock_sample_output.flush();
  if (state.timing_trace.is_open())
    state.timing_trace.flush();
  state.gameplay_trace.flush();
  if (!state.rdram_capture_path.empty())
    (void)write_private_rdram_capture(state);
#endif
  stop_pc_rumble(state);
  ledger(state, category, operation, guest_target);
  std::_Exit(4);
}

[[noreturn]] void stable_vi_gate(State &state) {
  if (state.journal.overflow || state.mmio_trace == nullptr ||
      state.mmio_trace->unsupported_accesses != 0U ||
      state.vi_interrupts < state.vi_retraces ||
      state.vi_messages_delivered < state.vi_retraces) {
    fail_closed_dispatch(state, "native", "invalid-stable-gate", 0U);
  }
  const std::string state_digest = state_hash(state);
  const std::string journal_digest = journal_hash(state);
  const std::string mmio_trace = mmio_trace_text(state);
  std::fprintf(stdout,
               "{\"kind\":\"jfg-phase6-native-boot\",\"status\":\"stable-vi\","
               "\"build_identity\":\"%s\",\"sanitizer\":\"%s\","
               "\"vi_retraces\":%llu,\"vi_frames\":%llu,"
               "\"vi_interrupts\":%llu,\"vi_messages_delivered\":%llu,"
               "\"vi_queue\":\"0x%08x\","
               "\"threads_created\":%zu,\"rdram_bytes\":%zu,"
               "\"mmio_accesses\":%llu,\"unsupported_accesses\":%llu,"
               "\"mmio_trace\":\"%s\","
               "\"journal_entries\":%zu,\"state_hash\":\"%s\","
               "\"journal_hash\":\"%s\"}\n",
               JFG_PHASE6_BUILD_IDENTITY, JFG_PHASE6_SANITIZER_ID,
               static_cast<unsigned long long>(state.vi_retraces),
               static_cast<unsigned long long>(state.vi_frames),
               static_cast<unsigned long long>(state.vi_interrupts),
               static_cast<unsigned long long>(state.vi_messages_delivered),
               state.vi_queue,
               state.threads.size(), kRdramSize,
               static_cast<unsigned long long>(state.mmio_trace->accesses),
               static_cast<unsigned long long>(
                   state.mmio_trace->unsupported_accesses),
               mmio_trace.c_str(), state.journal.count, state_digest.c_str(),
               journal_digest.c_str());
  std::fflush(stdout);
  jfg::frame_diagnostic_state().finish();
  jfg::finish_interpolation_trace();
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  state.host_audio.finish_capture();
  state.timing_trace.finish();
  state.input_record.finish();
  state.gameplay_trace.finish();
#endif
  state.guest_clock_trace.finish();
  state.snapshot_writer.finish();
  state.movement_trace.finish();
  state.jump_trace.finish();
  stop_pc_rumble(state);
  std::_Exit(0);
}

#if defined(JFG_PHASE8_LIVE_RUNTIME)
[[noreturn]] void probe_target_gate(State &state) {
#if defined(_WIN32)
  if (state.diagnostic_clock_release_vi != 0U && !state.diagnostic_clock_released)
    fail_closed_dispatch(state,"native-clock-tape","prefix-release-not-reached",0U);
  if (state.native_clock_samples.active()) {
    if (!state.native_clock_samples.finish())
      fail_closed_dispatch(state,"native-clock-tape","incomplete-or-invalid-transcript",0U);
    state.gameplay_trace.event("native-clock-tape",{state.native_clock_samples.samples(),
        state.cpu_count,state.observed_guest_instructions,state.native_clock_samples.replaying()});
  }
#endif
  if (state.mmio_trace == nullptr)
    fail_closed_dispatch(state, "probe", "missing-mmio-trace", 0U);
  if (state.guest_os_probe) {
    if (state.poll_target != 0 || state.vi_retraces != state.retrace_target ||
        state.mmio_trace->unsupported_accesses != 0 || state.mmio_trace->device_error)
      fail_closed_dispatch(state, "guest-probe", "invalid-target", 0U);
    // Explicitly separate this diagnostic from legacy HLE scheduler evidence.
    // In particular, do not invent HLE thread snapshots for original OS threads.

    if (!service_host_audio(state,false) || !write_private_rdram_capture(state))
      fail_closed_dispatch(state,"guest-capture","final-state",0U);
    if (state.presented_frames != 0U && !write_live_frame_capture(state))
      fail_closed_dispatch(state,"guest-capture","final-image",0U);
    if (state.timing_trace) state.timing_trace << "fast-hook\t"
        << state.execution_fast_hits << '\t' << state.observed_guest_instructions << '\n';
    if (state.timing_trace) state.timing_trace << "original-os-health\t" << state.vi_frames << '\t'
        << state.vi_retraces << '\t' << state.presented_frames << '\t'
        << state.audio_device_underruns << '\t' << state.audio_device_overruns << '\t'
        << state.mmio_trace->unsupported_accesses << '\t' << state.observed_guest_instructions << '\n';
    state.retrace_hash_trace.flush(); state.update_hash_trace.flush();
    state.poll_hash_trace.flush(); state.guest_clock_trace.flush();
    state.timing_trace.flush();
    state.gameplay_trace.flush();
    std::fprintf(stdout,
        "{\"kind\":\"jfg-phase9-original-os-probe\",\"acceptance\":false,"
        "\"status\":\"retrace-target\",\"retrace_target\":%u,\"vi_retraces\":%llu,"
        "\"controller_samples\":%llu,\"completed_updates\":%llu,\"cpu_count\":%llu,"
        "\"instructions\":%llu,\"graphics_tasks\":%llu,\"decoded_audio_tasks\":%llu,"
        "\"unsupported_accesses\":0,\"runtime_snapshot_complete\":false,\"state_hash\":\"%s\"}\n",
        state.retrace_target, static_cast<unsigned long long>(state.vi_retraces),
        static_cast<unsigned long long>(state.controller_samples),
        static_cast<unsigned long long>(state.completed_game_updates),
        static_cast<unsigned long long>(state.cpu_count),
        static_cast<unsigned long long>(state.observed_guest_instructions),
        static_cast<unsigned long long>(state.graphics_tasks),
        static_cast<unsigned long long>(state.decoded_audio_tasks), state_hash(state).c_str());
    std::fflush(stdout); jfg::frame_diagnostic_state().finish(); jfg::finish_interpolation_trace(); state.host_audio.finish_capture(); state.timing_trace.finish(); state.guest_clock_trace.finish(); state.input_record.finish(); state.gameplay_trace.finish(); state.snapshot_writer.finish(); state.movement_trace.finish(); state.jump_trace.finish(); stop_pc_rumble(state); std::_Exit(0);
  }
  const auto &event_counts = state.journal.observed_counts;
  std::string overlay_sequence;
  for (std::size_t index = 0U; index < state.journal.count; ++index) {
    const NativeEvent &event = state.journal.entries[index];
    if (event.kind != NativeEventKind::kOverlayPublished)
      continue;
    if (!overlay_sequence.empty())
      overlay_sequence.push_back(',');
    overlay_sequence += std::to_string(event.a);
  }
  std::string hle_counts;
  for (std::size_t index = 0U; index < kMappingCount; ++index) {
    if (state.mapped_call_counts[index] == 0U)
      continue;
    if (!hle_counts.empty())
      hle_counts.push_back(',');
    hle_counts += kMappings[index].name;
    hle_counts.push_back('=');
    hle_counts += std::to_string(state.mapped_call_counts[index]);
  }
  std::array<std::size_t, 4U> thread_states{};
  std::string thread_summary;
  if (state.scheduler != nullptr) {
    for (std::size_t thread_index = 0U;
         thread_index < state.thread_order.size(); ++thread_index) {
      const auto &[guest, id] = state.thread_order[thread_index];
      (void)guest;
      const auto snapshot = state.scheduler->snapshot_of(id);
      if (snapshot.has_value()) {
        const auto index = static_cast<std::size_t>(snapshot->state);
        if (index < thread_states.size())
          ++thread_states[index];
        if (!thread_summary.empty())
          thread_summary.push_back(',');
        thread_summary += std::to_string(id);
        thread_summary.push_back(':');
        thread_summary += std::to_string(snapshot->priority);
        thread_summary.push_back(':');
        thread_summary +=
            std::to_string(static_cast<std::uint32_t>(snapshot->state));
        thread_summary.push_back(':');
        char queue[11U]{};
        (void)std::snprintf(queue, sizeof(queue), "%08x",
                            snapshot->blocked_queue);
        thread_summary += queue;
        thread_summary.push_back(':');
        char entry[11U]{};
        (void)std::snprintf(
            entry, sizeof(entry), "%08x",
            thread_index < state.thread_entries.size()
                ? state.thread_entries[thread_index]
                : 0U);
        thread_summary += entry;
      }
    }
  }
  std::uint32_t si_queue = 0U, si_queue_valid = 0U;
  const auto si = state.events.find(5U);
  if (si != state.events.end()) {
    si_queue = si->second.first;
    hle::GuestMemory memory({state.rdram, kRdramSize},
                            hle::GuestMemory::Layout::native_word_big_endian);
    (void)memory.read_u32(si_queue + hle::kMqOffValidCount, si_queue_valid);
  }
  std::string si_dispatch_trace;
  for (const std::uint32_t dispatch_target : state.last_si_dispatch_trace) {
    if (!si_dispatch_trace.empty())
      si_dispatch_trace.push_back(',');
    char entry[11U]{};
    (void)std::snprintf(entry, sizeof(entry), "%08x", dispatch_target);
    si_dispatch_trace += entry;
  }
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (!service_host_audio(state, false))
    fail_closed_dispatch(state, "audio", "host-device-final", 0U);
  if (!persist_controller_pak(state))
    fail_closed_dispatch(state, "save", "controller-pak-persist", 0U);
  if (!write_live_frame_capture(state))
    fail_closed_dispatch(state, "renderer", "frame-capture", 0U);
  if (!write_private_rdram_capture(state))
    fail_closed_dispatch(state, "runtime", "rdram-capture", 0U);
#endif
  const std::string state_digest = state_hash(state);
  const std::string journal_digest = journal_hash(state);
  const unsigned front_mode = state.rdram[0x000A51B0U ^ 3U];
  hle::GuestMemory probe_memory(
      {state.rdram, kRdramSize},
      hle::GuestMemory::Layout::native_word_big_endian);
  std::uint32_t player_position_x = 0U;
  std::uint32_t player_position_y = 0U;
  std::uint32_t player_position_z = 0U;
  std::uint16_t player_yaw = 0U;
  if (state.phase9_player_actor != 0U) {
    std::uint32_t rotation_pair = 0U;
    if (probe_memory.read_u32(state.phase9_player_actor, rotation_pair))
      player_yaw = static_cast<std::uint16_t>(rotation_pair >> 16U);
    (void)probe_memory.read_u32(state.phase9_player_actor + 0x0CU,
                                player_position_x);
    (void)probe_memory.read_u32(state.phase9_player_actor + 0x10U,
                                player_position_y);
    (void)probe_memory.read_u32(state.phase9_player_actor + 0x14U,
                                player_position_z);
  }
  std::uint32_t hints_position_x = 0U;
  std::uint32_t hints_position_y = 0U;
  std::uint32_t hints_position_z = 0U;
  std::uint32_t hints_state = 0U;
  std::array<std::uint32_t, 12U> hints_state_words{};
  std::uint16_t hints_yaw = 0U;
  if (state.phase9_hints_actor != 0U) {
    std::uint32_t rotation_pair = 0U;
    if (probe_memory.read_u32(state.phase9_hints_actor, rotation_pair))
      hints_yaw = static_cast<std::uint16_t>(rotation_pair >> 16U);
    (void)probe_memory.read_u32(state.phase9_hints_actor + 0x0CU,
                                hints_position_x);
    (void)probe_memory.read_u32(state.phase9_hints_actor + 0x10U,
                                hints_position_y);
    (void)probe_memory.read_u32(state.phase9_hints_actor + 0x14U,
                                hints_position_z);
    if (probe_memory.read_u32(state.phase9_hints_actor + 0x68U,
                              hints_state) && hints_state != 0U) {
      for (std::size_t word = 0U; word < hints_state_words.size(); ++word)
        (void)probe_memory.read_u32(
            hints_state + static_cast<std::uint32_t>(word * 4U),
            hints_state_words[word]);
    }
  }
  std::uint32_t king_bear_actor = 0U;
  std::uint32_t object_list = 0U;
  std::uint32_t object_count = 0U;
  if (probe_memory.read_u32(0x800F2CA4U, object_list) &&
      probe_memory.read_u32(0x800F2CA8U, object_count) &&
      object_list != 0U && object_count <= 1024U) {
    for (std::uint32_t index = 0U; index < object_count; ++index) {
      std::uint32_t actor = 0U;
      std::uint32_t header = 0U;
      std::uint32_t name_word_0 = 0U;
      std::uint32_t name_word_1 = 0U;
      if (!probe_memory.read_u32(object_list + index * 4U, actor) ||
          actor == 0U ||
          !probe_memory.read_u32(actor + 0x40U, header) || header == 0U ||
          !probe_memory.read_u32(header + 4U, name_word_0) ||
          !probe_memory.read_u32(header + 8U, name_word_1))
        continue;
      if (name_word_0 == 0x4B696E67U && name_word_1 == 0x42656172U) {
        king_bear_actor = actor;
        break;
      }
    }
  }
  std::uint32_t king_bear_position_x = 0U;
  std::uint32_t king_bear_position_y = 0U;
  std::uint32_t king_bear_position_z = 0U;
  std::uint32_t king_bear_state = 0U;
  std::uint32_t king_bear_state_word_0 = 0U;
  std::uint32_t king_bear_sound_handle = 0U;
  std::uint32_t king_bear_state_word_30 = 0U;
  std::uint32_t king_bear_state_word_34 = 0U;
  std::uint16_t king_bear_yaw = 0U;
  if (king_bear_actor != 0U) {
    std::uint32_t rotation_pair = 0U;
    if (probe_memory.read_u32(king_bear_actor, rotation_pair))
      king_bear_yaw = static_cast<std::uint16_t>(rotation_pair >> 16U);
    (void)probe_memory.read_u32(king_bear_actor + 0x0CU,
                                king_bear_position_x);
    (void)probe_memory.read_u32(king_bear_actor + 0x10U,
                                king_bear_position_y);
    (void)probe_memory.read_u32(king_bear_actor + 0x14U,
                                king_bear_position_z);
    if (probe_memory.read_u32(king_bear_actor + 0x68U, king_bear_state) &&
        king_bear_state != 0U) {
      (void)probe_memory.read_u32(king_bear_state, king_bear_state_word_0);
      (void)probe_memory.read_u32(king_bear_state + 0x2CU,
                                  king_bear_sound_handle);
      (void)probe_memory.read_u32(king_bear_state + 0x30U,
                                  king_bear_state_word_30);
      (void)probe_memory.read_u32(king_bear_state + 0x34U,
                                  king_bear_state_word_34);
    }
  }
  std::uint32_t king_bear_callback_section = UINT32_MAX;
  std::uint32_t king_bear_callback_offset = 0U;
  if (state.phase9_king_bear_callback == 0x020002E8U) {
    king_bear_callback_section = 32U;
    king_bear_callback_offset = 0x2E8U;
  } else if (state.phase9_king_bear_callback != 0U) {
    RunlinkAddressResolution resolution{};
    if (resolve_runlink_text_address(
            probe_memory, kRunlinkModuleTablePointer,
            kRunlinkOverlaySlotCount, state.phase9_king_bear_callback,
            resolution) == RunlinkAddressResult::resolved) {
      for (std::uint32_t section = 0U;
           section < jfg_generated_section_count(); ++section) {
        JfgGeneratedSectionMetadata metadata{};
        if (jfg_generated_section_metadata(section, &metadata) != 0 &&
            metadata.is_overlay == 1U &&
            metadata.rom_start == resolution.identity.rom_start &&
            metadata.text_size == resolution.identity.text_size &&
            metadata.data_size == resolution.identity.data_size &&
            metadata.bss_size == resolution.identity.bss_size) {
          king_bear_callback_section = section;
          king_bear_callback_offset = resolution.text_offset;
          break;
        }
      }
    }
  }
  std::array<std::uint32_t, 8U> runlink_module7_words{};
  constexpr std::array<std::uint32_t, 16U> kHintTextStateOffsets{
      0x2A20U, 0x2A24U, 0x2A28U, 0x2A2CU,
      0x2A38U, 0x2A3CU, 0x2A40U, 0x2A44U,
      0x2A4CU, 0x2A58U, 0x2A5CU, 0x2A70U,
      0x2C30U, 0x2C34U, 0x2C3CU, 0x2C40U};
  const std::uint32_t hinttext_section_base =
      section_addresses != nullptr
          ? static_cast<std::uint32_t>(section_addresses[7U])
          : 0U;
  std::array<std::uint32_t, kHintTextStateOffsets.size()>
      hinttext_state_words{};
  if (hinttext_section_base != 0U) {
    const std::uint64_t sign_extended_base = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(
            static_cast<std::int32_t>(hinttext_section_base)));
    for (std::size_t word = 0U; word < hinttext_state_words.size(); ++word) {
      const std::uint64_t host_offset = sign_extended_base +
          kHintTextStateOffsets[word] - UINT64_C(0xFFFFFFFF80000000);
      if (host_offset <= kGuestAddressSpan - sizeof(std::uint32_t))
        std::memcpy(&hinttext_state_words[word], state.rdram + host_offset,
                    sizeof(std::uint32_t));
    }
  }
  std::uint32_t runlink_module_table = 0U;
  if (probe_memory.read_u32(kRunlinkModuleTablePointer,
                            runlink_module_table) &&
      runlink_module_table != 0U) {
    const std::uint32_t record = runlink_module_table +
        7U * kRunlinkModuleRecordSize;
    for (std::size_t word = 0U; word < runlink_module7_words.size(); ++word)
      (void)probe_memory.read_u32(
          record + static_cast<std::uint32_t>(word * sizeof(std::uint32_t)),
          runlink_module7_words[word]);
  }
  std::uint64_t weapon_stat_shots = 0U;
  std::uint64_t weapon_stat_hits = 0U;
  std::uint64_t weapon_stat_kills = 0U;
  std::uint32_t player_data = 0U;
  if (probe_memory.read_u32(0x800FD7D4U, player_data) && player_data != 0U) {
    for (std::uint32_t weapon = 0U; weapon < 16U; ++weapon) {
      std::uint32_t shots = 0U;
      std::uint32_t hits = 0U;
      std::uint32_t kills = 0U;
      const std::uint32_t slot = weapon * sizeof(std::uint32_t);
      if (probe_memory.read_u32(player_data + 0x474U + slot, shots))
        weapon_stat_shots += shots;
      if (probe_memory.read_u32(player_data + 0x4B4U + slot, hits))
        weapon_stat_hits += hits;
      if (probe_memory.read_u32(player_data + 0x434U + slot, kills))
        weapon_stat_kills += kills;
    }
  }
  bool emit_actor_trace = false;
#if defined(_WIN32)
  char *actor_trace = nullptr;
  std::size_t actor_trace_size = 0U;
  if (_dupenv_s(&actor_trace, &actor_trace_size, "JFG_PHASE9_ACTOR_TRACE") ==
          0 &&
      actor_trace != nullptr && std::strcmp(actor_trace, "1") == 0)
    emit_actor_trace = true;
  std::free(actor_trace);
#else
  if (const char *actor_trace = std::getenv("JFG_PHASE9_ACTOR_TRACE");
      actor_trace != nullptr && std::strcmp(actor_trace, "1") == 0)
    emit_actor_trace = true;
#endif
  if (emit_actor_trace) {
    std::uint32_t actor_list = 0U;
    std::uint32_t actor_count = 0U;
    if (probe_memory.read_u32(0x800F2CA4U, actor_list) &&
        probe_memory.read_u32(0x800F2CA8U, actor_count) &&
        actor_count <= 256U) {
      std::fprintf(stdout, "{\"kind\":\"jfg-phase9-actor-trace\","
                           "\"actors\":[");
      bool first_actor = true;
      for (std::uint32_t index = 0U; index < actor_count; ++index) {
        std::uint32_t actor = 0U;
        std::uint32_t header = 0U;
        std::uint32_t name0 = 0U;
        std::uint32_t name1 = 0U;
        std::uint32_t x = 0U;
        std::uint32_t y = 0U;
        std::uint32_t z = 0U;
        if (!probe_memory.read_u32(actor_list + index * 4U, actor) ||
            actor == 0U || !probe_memory.read_u32(actor + 0x40U, header) ||
            header == 0U || !probe_memory.read_u32(header + 4U, name0) ||
            !probe_memory.read_u32(header + 8U, name1) ||
            !probe_memory.read_u32(actor + 0x0CU, x) ||
            !probe_memory.read_u32(actor + 0x10U, y) ||
            !probe_memory.read_u32(actor + 0x14U, z))
          continue;
        if (!first_actor)
          std::fputc(',', stdout);
        first_actor = false;
        std::fprintf(stdout,
                     "{\"index\":%u,\"actor\":\"0x%08x\","
                     "\"name_words\":\"%08x,%08x\","
                     "\"position_words\":\"%08x,%08x,%08x\"}",
                     index, actor, name0, name1, x, y, z);
      }
      std::fputs("]}\n", stdout);
    }
  }
  // Host wall-clock totals are diagnostics, not simulation state. They live
  // in the timing side channel so the canonical probe record below is
  // byte-identical across deterministic runs.
  if (state.timing_trace) {
    state.timing_trace << "totals\t" << state.vi_retraces << '\t'
                       << state.audio_task_microseconds << '\t'
                       << state.graphics_prepare_microseconds << '\t'
                       << state.graphics_submit_microseconds << '\t'
                       << state.graphics_present_microseconds << '\n';
  }
  std::fprintf(
      stdout,
      "{\"kind\":\"jfg-phase8-native-probe\",\"status\":\"%s\","
      "\"%s\":%u,\"vi_retraces\":%llu,\"vi_frames\":%llu,"
      "\"vi_interrupts\":%llu,\"vi_messages_delivered\":%llu,"
      "\"mmio_accesses\":%llu,\"unsupported_accesses\":%llu,"
      "\"dispatches\":%zu,\"pi_dma\":%llu,"
      "\"overlay_publications\":%llu,"
      "\"overlay_sequence\":\"%s\","
      "\"phase9_route\":{\"player_control_calls\":%llu,"
      "\"weapon_update_calls\":%llu,\"weapon_fire_held_updates\":%llu,"
      "\"weapon_fire_pressed_updates\":%llu,"
      "\"weapon_dummy_fire_calls\":%llu,\"weapon_fire_calls\":%llu,"
      "\"weapon_shot_calls\":%llu,"
      "\"weapon_hit_calls\":%llu,\"enemy_kill_calls\":%llu,"
      "\"weapon_stat_shots\":%llu,\"weapon_stat_hits\":%llu,"
      "\"weapon_stat_kills\":%llu,"
      "\"collision_calls\":%llu,\"particle_update_calls\":%llu,"
      "\"cutscene_active_calls\":%llu,\"cutscene_camera_calls\":%llu,"
      "\"level_change_calls\":%llu,\"death_restart_calls\":%llu,"
      "\"player_hit_check_calls\":%llu,"
      "\"player_actor\":\"0x%08x\","
      "\"player_yaw\":\"0x%04x\","
      "\"player_position_bits\":\"%08x,%08x,%08x\","
      "\"hints_control_calls\":%llu,\"hints_talk_calls\":%llu,"
      "\"hints_actor\":\"0x%08x\",\"hints_yaw\":\"0x%04x\","
      "\"hints_position_bits\":\"%08x,%08x,%08x\","
      "\"hints_state\":\"0x%08x\","
      "\"hints_state_words\":"
      "\"%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,"
      "%08x,%08x,%08x,%08x\","
      "\"king_bear_control_calls\":%llu,"
      "\"king_bear_callback\":\"0x%08x\","
      "\"king_bear_callback_section\":%u,"
      "\"king_bear_callback_offset\":\"0x%08x\","
      "\"king_bear_actor\":\"0x%08x\","
      "\"king_bear_yaw\":\"0x%04x\","
      "\"king_bear_position_bits\":\"%08x,%08x,%08x\","
      "\"king_bear_state\":\"0x%08x\","
      "\"king_bear_state_words\":\"%08x,%08x,%08x,%08x\","
      "\"hinttext_start_calls\":%llu,\"hinttext_stop_calls\":%llu,"
      "\"hinttext_active_calls\":%llu,"
      "\"hinttext_active_zero_returns\":%llu,"
      "\"hinttext_active_nonzero_returns\":%llu,"
      "\"hinttext_active_last_return\":%u,"
      "\"hinttext_update_calls\":%llu,"
      "\"hinttext_advance_calls\":%llu,"
      "\"hinttext_accept_calls\":%llu,"
      "\"hinttext_section_base\":\"0x%08x\","
      "\"hinttext_state_words\":"
      "\"%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,"
      "%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\","
      "\"runlink_module7_words\":"
      "\"%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\"},"
      "\"controller_events\":%llu,"
      "\"generated_calls\":%llu,\"mapped_calls\":%llu,"
      "\"queue_create_calls\":%llu,\"queue_send_calls\":%llu,"
      "\"queue_recv_calls\":%llu,\"sp_task_load_calls\":%llu,"
      "\"sp_task_start_calls\":%llu,"
      "\"controller_read_start_calls\":%llu,"
      "\"controller_get_data_calls\":%llu,\"vi_swap_calls\":%llu,"
      "\"graphics_tasks\":%llu,\"graphics_commands\":%llu,"
      "\"graphics_triangles\":%llu,\"presented_frames\":%llu,"
      "\"decoded_audio_tasks\":%llu,"
      "\"audio_device_initialized\":%s,\"audio_device_started\":%s,"
      "\"audio_device_frequency\":%u,\"audio_device_buffers\":%llu,"
      "\"audio_device_queued_bytes\":%llu,"
      "\"audio_device_consumed_bytes\":%llu,"
      "\"audio_device_consumed_ms\":%llu,"
      "\"audio_device_underruns\":%llu,\"audio_device_overruns\":%llu,"
      "\"ai_submission_overflows\":%llu,\"frame_captured\":%s,"
      "\"input_replay_events\":%zu,\"controller_samples\":%llu,"
      "\"non_neutral_controller_samples\":%llu,"
      "\"controller_disconnects\":%llu,\"controller_reconnects\":%llu,"
      "\"game_pressed_observations\":%llu,"
      "\"game_stick_observations\":%llu,"
      "\"last_game_stick_x\":%d,\"last_game_stick_y\":%d,"
      "\"menu_stick_observations\":%llu,"
      "\"last_menu_stick_x\":%d,\"last_menu_stick_y\":%d,"
      "\"front_mode\":%u,"
      "\"flash_path_configured\":%s,\"flash_image_loaded\":%s,"
      "\"flash_image_created\":%s,\"flash_persist_count\":%llu,"
      "\"threads_stopped\":%zu,\"threads_runnable\":%zu,"
      "\"threads_blocked\":%zu,\"threads_finished\":%zu,"
      "\"hle_counts\":\"%s\","
      "\"thread_summary\":\"%s\",\"controller_read_thread\":%d,"
      "\"si_queue\":\"0x%08x\",\"si_queue_valid\":%u,"
      "\"si_receive_calls\":%llu,\"last_si_receive_return\":\"0x%08x\","
      "\"last_si_receive_block\":%u,"
      "\"last_si_dispatch_trace\":\"%s\","
      "\"journal_events\":%llu,\"journal_entries\":%zu,"
      "\"journal_overflow\":%s,"
      "\"state_hash\":\"%s\",\"journal_hash\":\"%s\"}\n",
      state.poll_target != 0U ? "poll-target" : "retrace-target",
      state.poll_target != 0U ? "poll_target" : "retrace_target",
      state.poll_target != 0U ? state.poll_target : state.retrace_target,
      static_cast<unsigned long long>(state.vi_retraces),
      static_cast<unsigned long long>(state.vi_frames),
      static_cast<unsigned long long>(state.vi_interrupts),
      static_cast<unsigned long long>(state.vi_messages_delivered),
      static_cast<unsigned long long>(state.mmio_trace->accesses),
      static_cast<unsigned long long>(state.mmio_trace->unsupported_accesses),
      static_cast<std::size_t>(state.dispatch_calls),
      static_cast<unsigned long long>(
          event_counts[static_cast<std::size_t>(NativeEventKind::kPiDma)]),
      static_cast<unsigned long long>(event_counts[static_cast<std::size_t>(
          NativeEventKind::kOverlayPublished)]),
      overlay_sequence.c_str(),
      static_cast<unsigned long long>(state.phase9_player_control_calls),
      static_cast<unsigned long long>(state.phase9_weapon_update_calls),
      static_cast<unsigned long long>(state.phase9_weapon_fire_held_updates),
      static_cast<unsigned long long>(state.phase9_weapon_fire_pressed_updates),
      static_cast<unsigned long long>(state.phase9_weapon_dummy_fire_calls),
      static_cast<unsigned long long>(state.phase9_weapon_fire_calls),
      static_cast<unsigned long long>(state.phase9_weapon_shot_calls),
      static_cast<unsigned long long>(state.phase9_weapon_hit_calls),
      static_cast<unsigned long long>(state.phase9_enemy_kill_calls),
      static_cast<unsigned long long>(weapon_stat_shots),
      static_cast<unsigned long long>(weapon_stat_hits),
      static_cast<unsigned long long>(weapon_stat_kills),
      static_cast<unsigned long long>(state.phase9_collision_calls),
      static_cast<unsigned long long>(state.phase9_particle_update_calls),
      static_cast<unsigned long long>(state.phase9_cutscene_active_calls),
      static_cast<unsigned long long>(state.phase9_cutscene_camera_calls),
      static_cast<unsigned long long>(state.phase9_level_change_calls),
      static_cast<unsigned long long>(state.phase9_death_restart_calls),
      static_cast<unsigned long long>(state.phase9_player_hit_check_calls),
      state.phase9_player_actor, static_cast<unsigned>(player_yaw),
      player_position_x, player_position_y, player_position_z,
      static_cast<unsigned long long>(state.phase9_hints_control_calls),
      static_cast<unsigned long long>(state.phase9_hints_talk_calls),
      state.phase9_hints_actor, static_cast<unsigned>(hints_yaw),
      hints_position_x, hints_position_y, hints_position_z,
      hints_state, hints_state_words[0], hints_state_words[1],
      hints_state_words[2], hints_state_words[3], hints_state_words[4],
      hints_state_words[5], hints_state_words[6], hints_state_words[7],
      hints_state_words[8], hints_state_words[9], hints_state_words[10],
      hints_state_words[11],
      static_cast<unsigned long long>(state.phase9_king_bear_control_calls),
      state.phase9_king_bear_callback, king_bear_callback_section,
      king_bear_callback_offset,
      king_bear_actor, static_cast<unsigned>(king_bear_yaw),
      king_bear_position_x, king_bear_position_y, king_bear_position_z,
      king_bear_state, king_bear_state_word_0, king_bear_sound_handle,
      king_bear_state_word_30, king_bear_state_word_34,
      static_cast<unsigned long long>(state.phase9_hinttext_start_calls),
      static_cast<unsigned long long>(state.phase9_hinttext_stop_calls),
      static_cast<unsigned long long>(state.phase9_hinttext_active_calls),
      static_cast<unsigned long long>(
          state.phase9_hinttext_active_zero_returns),
      static_cast<unsigned long long>(
          state.phase9_hinttext_active_nonzero_returns),
      state.phase9_hinttext_active_last_return,
      static_cast<unsigned long long>(state.phase9_hinttext_update_calls),
      static_cast<unsigned long long>(state.phase9_hinttext_advance_calls),
      static_cast<unsigned long long>(state.phase9_hinttext_accept_calls),
      hinttext_section_base,
      hinttext_state_words[0], hinttext_state_words[1],
      hinttext_state_words[2], hinttext_state_words[3],
      hinttext_state_words[4], hinttext_state_words[5],
      hinttext_state_words[6], hinttext_state_words[7],
      hinttext_state_words[8], hinttext_state_words[9],
      hinttext_state_words[10], hinttext_state_words[11],
      hinttext_state_words[12], hinttext_state_words[13],
      hinttext_state_words[14], hinttext_state_words[15],
      runlink_module7_words[0], runlink_module7_words[1],
      runlink_module7_words[2], runlink_module7_words[3],
      runlink_module7_words[4], runlink_module7_words[5],
      runlink_module7_words[6], runlink_module7_words[7],
      static_cast<unsigned long long>(event_counts[static_cast<std::size_t>(
          NativeEventKind::kController)]),
      static_cast<unsigned long long>(state.generated_calls),
      static_cast<unsigned long long>(state.mapped_calls),
      static_cast<unsigned long long>(state.queue_create_calls),
      static_cast<unsigned long long>(state.queue_send_calls),
      static_cast<unsigned long long>(state.queue_recv_calls),
      static_cast<unsigned long long>(state.sp_task_load_calls),
      static_cast<unsigned long long>(state.sp_task_start_calls),
      static_cast<unsigned long long>(state.controller_read_start_calls),
      static_cast<unsigned long long>(state.controller_get_data_calls),
      static_cast<unsigned long long>(state.vi_swap_calls),
      static_cast<unsigned long long>(state.graphics_tasks),
      static_cast<unsigned long long>(state.graphics_commands),
      static_cast<unsigned long long>(state.graphics_triangles),
      static_cast<unsigned long long>(state.presented_frames),
      static_cast<unsigned long long>(state.decoded_audio_tasks),
      state.audio_device_initialized ? "true" : "false",
      state.audio_device_started ? "true" : "false",
      state.audio_device_frequency,
      static_cast<unsigned long long>(state.audio_device_buffers),
      static_cast<unsigned long long>(state.audio_device_queued_bytes),
      static_cast<unsigned long long>(state.audio_device_consumed_bytes),
      static_cast<unsigned long long>(state.audio_device_consumed_ms),
      static_cast<unsigned long long>(state.audio_device_underruns),
      static_cast<unsigned long long>(state.audio_device_overruns),
      static_cast<unsigned long long>(
          state.mmio_trace == nullptr
              ? 0U
              : state.mmio_trace->ai_submission_overflows),
      state.frame_captured ? "true" : "false",
      state.input_replay_event_count,
      static_cast<unsigned long long>(state.controller_samples),
      static_cast<unsigned long long>(state.non_neutral_controller_samples),
      static_cast<unsigned long long>(state.controller_disconnects),
      static_cast<unsigned long long>(state.controller_reconnects),
      static_cast<unsigned long long>(state.game_pressed_observations),
      static_cast<unsigned long long>(state.game_stick_observations),
      static_cast<int>(state.last_game_stick_x),
      static_cast<int>(state.last_game_stick_y),
      static_cast<unsigned long long>(state.menu_stick_observations),
      static_cast<int>(state.last_menu_stick_x),
      static_cast<int>(state.last_menu_stick_y),
      front_mode,
      state.flash_path.empty() ? "false" : "true",
      state.flash_image_loaded ? "true" : "false",
      state.flash_image_created ? "true" : "false",
      static_cast<unsigned long long>(state.flash_persist_count),
      thread_states[static_cast<std::size_t>(ThreadState::kStopped)],
      thread_states[static_cast<std::size_t>(ThreadState::kRunnable)],
      thread_states[static_cast<std::size_t>(ThreadState::kBlocked)],
      thread_states[static_cast<std::size_t>(ThreadState::kFinished)],
      hle_counts.c_str(),
      thread_summary.c_str(), state.controller_read_thread, si_queue,
      si_queue_valid,
      static_cast<unsigned long long>(state.si_receive_calls),
      state.last_si_receive_return, state.last_si_receive_block,
      si_dispatch_trace.c_str(),
      static_cast<unsigned long long>(state.journal.observed_total),
      state.journal.count, state.journal.overflow ? "true" : "false",
      state_digest.c_str(), journal_digest.c_str());
  std::fflush(stdout);
  jfg::frame_diagnostic_state().finish();
  jfg::finish_interpolation_trace();
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  state.host_audio.finish_capture();
  state.timing_trace.finish();
  state.input_record.finish();
  state.gameplay_trace.finish();
#endif
  state.guest_clock_trace.finish();
  state.snapshot_writer.finish();
  state.movement_trace.finish();
  state.jump_trace.finish();
  stop_pc_rumble(state);
  std::_Exit(0);
}
#endif

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
[[noreturn]] void play_exit(State &state) {
  jfg::support_event("native=closed");
  if (state.mmio_trace == nullptr ||
      state.mmio_trace->unsupported_accesses != 0U) {
    fail_closed_dispatch(state, "play", "invalid-clean-exit", 0U);
  }
  if (!service_host_audio(state, false))
    fail_closed_dispatch(state, "audio", "host-device-final", 0U);
  if (!persist_controller_pak(state))
    fail_closed_dispatch(state, "save", "controller-pak-persist", 0U);
  if (!write_live_frame_capture(state))
    fail_closed_dispatch(state, "renderer", "frame-capture", 0U);
  if (!write_private_rdram_capture(state))
    fail_closed_dispatch(state, "runtime", "rdram-capture", 0U);
  // Retain final counters even if the detailed trace capped and the launcher
  // filtered the long stdout record.
  if (state.timing_trace.is_open())
    state.timing_trace << "play-health\t" << state.vi_frames << '\t'
                       << state.vi_retraces << "\t0\t"
                       << state.audio_device_underruns << '\t'
                       << state.audio_device_overruns << '\t'
                       << state.timing_game_updates << '\t'
                       << state.controller_samples << '\n';
  const unsigned front_mode = state.rdram[0x000A51B0U ^ 3U];
  std::fprintf(
      stdout,
      "{\"kind\":\"jfg-phase8-play\",\"status\":\"window-closed\","
      "\"vi_retraces\":%llu,\"graphics_tasks\":%llu,"
      "\"presented_frames\":%llu,\"decoded_audio_tasks\":%llu,"
      "\"audio_device_initialized\":%s,\"audio_device_started\":%s,"
      "\"audio_device_frequency\":%u,\"audio_device_buffers\":%llu,"
      "\"audio_device_consumed_bytes\":%llu,"
      "\"audio_device_consumed_ms\":%llu,"
      "\"audio_device_underruns\":%llu,\"audio_device_overruns\":%llu,"
      "\"frame_captured\":%s,"
      "\"controller_samples\":%llu,"
      "\"non_neutral_controller_samples\":%llu,"
      "\"non_neutral_controller_writes\":%llu,"
      "\"controller_disconnects\":%llu,\"controller_reconnects\":%llu,"
      "\"game_pressed_observations\":%llu,"
      "\"last_game_pressed_buttons\":\"0x%04x\","
      "\"game_stick_observations\":%llu,"
      "\"last_game_stick_x\":%d,\"last_game_stick_y\":%d,"
      "\"menu_stick_observations\":%llu,"
      "\"last_menu_stick_x\":%d,\"last_menu_stick_y\":%d,"
      "\"front_mode\":%u,"
      "\"unsupported_accesses\":0,\"journal_overflow\":%s}\n",
      static_cast<unsigned long long>(state.vi_retraces),
      static_cast<unsigned long long>(state.graphics_tasks),
      static_cast<unsigned long long>(state.presented_frames),
      static_cast<unsigned long long>(state.decoded_audio_tasks),
      state.audio_device_initialized ? "true" : "false",
      state.audio_device_started ? "true" : "false",
      state.audio_device_frequency,
      static_cast<unsigned long long>(state.audio_device_buffers),
      static_cast<unsigned long long>(state.audio_device_consumed_bytes),
      static_cast<unsigned long long>(state.audio_device_consumed_ms),
      static_cast<unsigned long long>(state.audio_device_underruns),
      static_cast<unsigned long long>(state.audio_device_overruns),
      state.frame_captured ? "true" : "false",
      static_cast<unsigned long long>(state.controller_samples),
      static_cast<unsigned long long>(state.non_neutral_controller_samples),
      static_cast<unsigned long long>(state.non_neutral_controller_writes),
      static_cast<unsigned long long>(state.controller_disconnects),
      static_cast<unsigned long long>(state.controller_reconnects),
      static_cast<unsigned long long>(state.game_pressed_observations),
      state.last_game_pressed_buttons,
      static_cast<unsigned long long>(state.game_stick_observations),
      static_cast<int>(state.last_game_stick_x),
      static_cast<int>(state.last_game_stick_y),
      static_cast<unsigned long long>(state.menu_stick_observations),
      static_cast<int>(state.last_menu_stick_x),
      static_cast<int>(state.last_menu_stick_y),
      front_mode,
      state.journal.overflow ? "true" : "false");
  std::fflush(stdout);
  jfg::frame_diagnostic_state().finish();
  jfg::finish_interpolation_trace();
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  state.host_audio.finish_capture();
  state.timing_trace.finish();
  state.input_record.finish();
  state.gameplay_trace.finish();
#endif
  state.guest_clock_trace.finish();
  state.snapshot_writer.finish();
  state.movement_trace.finish();
  state.jump_trace.finish();
  stop_pc_rumble(state);
  std::_Exit(0);
}
#endif

[[noreturn]] void retrace_gate(State &state) {
  if (state.poll_target != 0U)
    fail_closed_dispatch(state, "probe", "poll-not-reached", 0U);
  if (state.retrace_target == 3U)
    stable_vi_gate(state);
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  probe_target_gate(state);
#else
  fail_closed_dispatch(state, "native", "unexpected-retrace-target", 0U);
#endif
}

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
void complete_guest_sp_status(State &state, const std::uint32_t target) {
  if (!state.guest_leaf_probe) return;
  if (state.mmio_trace == nullptr || !state.mmio_trace->guest_sp_status ||
      !state.mmio_trace->sp_status.complete_task())
    fail_closed_dispatch(state, "rsp", "completion-status", target);
  // The current HLE interrupt service acknowledges before posting its OS
  // message. It still has no guest interrupt-handler execution/timing.
  state.mmio_trace->sp_status.acknowledge_interrupt();
}

// Community ranged import (2026-10-19). Every graphics task used to import
// the whole 8 MiB snapshot into RT64 (a 4 MiB guest half and a 4 MiB half of
// overlay shadows), and every present imported a snapshot whose upper half
// was zero, so the shadows were wiped and re-copied each frame. RT64 only
// writes framebuffers (guest half). Outside the shadows flagged dirty in the
// new snapshot or in the one RT64 imported last, both upper halves equal the
// initial overlay image, so those bytes need no comparison.
// JFG_RANGED_IMPORT=0 restores the full imports.
bool community_upper_ranges(const State &state, const PendingLiveGraphicsTask &pending,
                            std::vector<jfg::Rt64RdramRange> &ranges) {
  const std::size_t count = state.graphics_overlay_shadows.size();
  if (!state.community_ranged_import || !state.community_rt64_upper_known ||
      state.rt64_shell == nullptr || !state.rt64_shell->ranged_import_available() ||
      pending.community_upper_dirty.size() != count + 1U ||
      state.community_rt64_upper_dirty.size() != count + 1U ||
      pending.community_upper_dirty[count] != 0U ||
      state.community_rt64_upper_dirty[count] != 0U)
    return false;
  ranges.clear();
  ranges.push_back({0U, kRdramSize});
  for (std::size_t index = 0U; index < count; ++index) {
    if (pending.community_upper_dirty[index] == 0U &&
        state.community_rt64_upper_dirty[index] == 0U)
      continue;
    const auto &shadow = state.graphics_overlay_shadows[index];
    const std::size_t begin = shadow.physical_base;
    const std::size_t end = begin + shadow.extent;
    if (!ranges.empty() && ranges.back().end >= begin)
      ranges.back().end = (std::max)(ranges.back().end, end);
    else
      ranges.push_back({begin, end});
  }
  return true;
}

void complete_pending_live_graphics_tasks(
    State &state, const std::uint32_t target,
    const bool require_offscreen_target = false) {
  if (state.pending_graphics_tasks.empty())
    return;
  const auto sp = state.events.find(4U);
  const auto dp = state.events.find(9U);
  if (state.rt64_shell == nullptr || (!state.guest_os_probe && (state.scheduler == nullptr ||
      sp == state.events.end() || dp == state.events.end())))
    fail_closed_dispatch(state, "renderer", "completion-events", target);
  const std::uint32_t selected_vi_framebuffer =
      (state.vi_next_framebuffer != 0U ? state.vi_next_framebuffer
                                      : state.vi_current_framebuffer) &
      0x00FFFFFFU;
  for (PendingLiveGraphicsTask &pending : state.pending_graphics_tasks) {
    if (pending.skipped) {
      if (state.guest_os_probe)
        fail_closed_dispatch(state, "renderer", "guest-task-skipped", target);
      ++state.graphics_tasks;
      complete_guest_sp_status(state, target);
      if (state.scheduler->send(sp->second.first, sp->second.second, false) !=
          hle::kOsSuccess)
        fail_closed_dispatch(state, "rsp", "sp-completion", target);
      const std::int32_t dp_delivery = state.scheduler->send(
          dp->second.first, dp->second.second, false);
      if (dp_delivery != hle::kOsSuccess && dp_delivery != hle::kOsFull)
        fail_closed_dispatch(state, "rsp", "dp-completion", target);
      continue;
    }
    if (!state.rdram_capture_path.empty() &&
        state.vi_retraces + 1U >= state.retrace_target &&
        !write_private_rt64_snapshot(state, pending.rdram))
      fail_closed_dispatch(state, "renderer", "rt64-rdram-capture", target);
    write_private_progress(state, "graphics-submit-begin");
    struct SubmitThreadSample {std::uint64_t cycles=0,cpu100ns=0;DWORD thread=0;bool valid=false;};
    const auto sample_submit_thread=[&] {
      SubmitThreadSample sample{};
      if(!state.diagnostic_function_profile)return sample;
      ULONG64 cycles=0;FILETIME created{},exited{},kernel{},user{};
      sample.thread=GetCurrentThreadId();
      sample.valid=QueryThreadCycleTime(GetCurrentThread(),&cycles)!=0 &&
          GetThreadTimes(GetCurrentThread(),&created,&exited,&kernel,&user)!=0;
      if(sample.valid) {
        sample.cycles=cycles;
        sample.cpu100ns=(std::uint64_t(kernel.dwHighDateTime)<<32U)+kernel.dwLowDateTime+
            (std::uint64_t(user.dwHighDateTime)<<32U)+user.dwLowDateTime;
      }
      return sample;
    };
    const auto submit_thread_start=sample_submit_thread();
    const auto submit_start = std::chrono::steady_clock::now();
    jfg::SubmissionProfileScope sync_scope(state.diagnostic_function_profile!=0U);
    const auto sync_stamp=[&]{return sync_scope.enabled ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};};
    const auto community_import_start = std::chrono::steady_clock::now();
    thread_local std::vector<jfg::Rt64RdramRange> community_ranges;
    const bool community_ranged = community_upper_ranges(state, pending, community_ranges);
    if (community_ranged) {
      state.last_rt64_error = state.rt64_shell->replace_rdram_snapshot_ranges(
          pending.rdram, jfg::Rt64MemoryLayout::host_word_swapped, community_ranges);
      ++state.community_prof_ranged;
      for (const auto &range : community_ranges)
        state.community_prof_import_kb += (range.end - range.begin) / 1024U;
    } else {
      state.last_rt64_error = state.rt64_shell->replace_rdram_snapshot(
          pending.rdram, jfg::Rt64MemoryLayout::host_word_swapped);
      state.community_prof_import_kb += jfg::kRt64RequiredRdramBytes / 1024U;
    }
    // RT64's upper half now equals this snapshot's upper half (both import
    // kinds); remember its flags, or mark it unknown when there are none.
    if (state.last_rt64_error == jfg::Rt64ShellError::none) {
      state.community_rt64_upper_known =
          state.community_ranged_import &&
          pending.community_upper_dirty.size() == state.graphics_overlay_shadows.size() + 1U;
      state.community_rt64_upper_dirty = pending.community_upper_dirty;
    } else {
      state.community_rt64_upper_known = false;
    }
    const auto community_import_end = std::chrono::steady_clock::now();
    state.community_prof_import_us += static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            community_import_end - community_import_start).count());
    const auto snapshot_done=sync_stamp();
    const auto snapshot_thread=sample_submit_thread();
    if (state.last_rt64_error == jfg::Rt64ShellError::none)
      state.last_rt64_error = state.rt64_shell->submit(pending.task);
    const auto community_rt64_end = std::chrono::steady_clock::now();
    state.community_prof_rt64_us += static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            community_rt64_end - community_import_end).count());
    const auto decode_done=sync_stamp();
    const auto decode_thread=sample_submit_thread();
    // Return completed intensity masks before delivering DP/SP completion:
    // the guest blurs and attenuates its shadow mask on the CPU.
    if (state.last_rt64_error == jfg::Rt64ShellError::none)
      state.last_rt64_error = state.rt64_shell->commit_cpu_writeback(pending.rdram,
          std::as_writable_bytes(std::span(state.rdram, kRdramSize)));
    state.community_prof_writeback_us += static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - community_rt64_end).count());
    const auto writeback_done=sync_stamp();
    const auto writeback_thread=sample_submit_thread();
    const jfg::Rt64GraphicsDiagnostics submission_graphics =
        state.rt64_shell->last_graphics_diagnostics();
    if (state.last_rt64_error == jfg::Rt64ShellError::none)
      state.host_audio.renderer_ready();

    const auto submit_us = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - submit_start)
            .count());
    state.gameplay_trace.event("submit", {state.vi_retraces, state.controller_samples,
        state.gameplay_trace_updates, submit_us, pending.descriptor[4], pending.descriptor[5],
        submission_graphics.rdram_check_microseconds, submission_graphics.full_sync_microseconds,
        submission_graphics.color_image_address});
    if(state.diagnostic_function_profile) {
      const auto timing=sync_scope.delta();
      const std::array<SubmitThreadSample,4> cpu{submit_thread_start,snapshot_thread,decode_thread,writeback_thread};
      bool cpu_valid=true;std::array<std::uint64_t,3> cycles{},ticks{};
      for(unsigned i=0;i<3;++i) {
        cpu_valid=cpu_valid && cpu[i].valid && cpu[i+1].valid && cpu[i].thread==cpu[i+1].thread &&
            cpu[i+1].cycles>=cpu[i].cycles && cpu[i+1].cpu100ns>=cpu[i].cpu100ns;
        if(cpu_valid){cycles[i]=cpu[i+1].cycles-cpu[i].cycles;ticks[i]=cpu[i+1].cpu100ns-cpu[i].cpu100ns;}
      }
      if(!cpu_valid){cycles={};ticks={};}
      state.gameplay_trace.event("submit-thread",{state.vi_retraces,state.controller_samples,state.gameplay_trace_updates,
        cpu[0].thread,cpu_valid?1U:0U,cycles[0],cycles[1],cycles[2],ticks[0],ticks[1],ticks[2]});
      const auto ns=[](auto a,auto z){return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(z-a).count());};
      state.gameplay_trace.event("submit-host",{
        state.vi_retraces,state.controller_samples,state.gameplay_trace_updates,
        ns(submit_start,snapshot_done),ns(snapshot_done,decode_done),ns(decode_done,writeback_done)});
      state.gameplay_trace.event("submit-sync",{
        state.vi_retraces,state.controller_samples,state.gameplay_trace_updates,
        timing.ns[0],timing.ns[1],timing.ns[2],timing.ns[3],timing.ns[4],timing.ns[5],
        timing.calls[0],timing.calls[1],timing.calls[2],timing.calls[3],timing.calls[4],timing.calls[5]});
      const auto& stages=submission_graphics.history_stage_microseconds;
      state.gameplay_trace.event("submit-detail",{state.vi_retraces,state.controller_samples,
        state.gameplay_trace_updates,submission_graphics.history_prepare_microseconds,
        submission_graphics.rdp_sync_microseconds,stages[0],stages[1],stages[2],stages[3],stages[4],
        submission_graphics.vertices_loaded,submission_graphics.triangles_drawn});
    }
    if (state.diagnostic_cpu_costs || state.diagnostic_gpu_costs) {
      const auto timing = state.rt64_shell->framebuffer_gpu_timing();
      state.gameplay_trace.event("gpu-framebuffer-time", {state.vi_retraces, state.controller_samples,
          timing.submissions, timing.total_nanoseconds, timing.last_nanoseconds,
          timing.maximum_nanoseconds, timing.failed});
      const auto workload = state.rt64_shell->workload_gpu_timing();
      state.gameplay_trace.event("gpu-workload-time", {state.vi_retraces, state.controller_samples,
          workload.submissions, workload.total_nanoseconds, workload.last_nanoseconds,
          workload.maximum_nanoseconds, workload.failed});
    }
    state.graphics_submit_microseconds += submit_us;
    state.graphics_submit_max_us =
        (std::max)(state.graphics_submit_max_us, submit_us);
    if (state.timing_trace) {
      state.timing_trace << "submit\t" << state.vi_frames << '\t'
                         << state.vi_retraces << '\t' << submit_us << '\t'
                         << std::hex << pending.descriptor[4] << '\t'
                         << pending.descriptor[5] << std::dec << '\t'
                         << submission_graphics.rdram_check_microseconds
                         << '\t'
                         << submission_graphics.full_sync_microseconds << '\t'
                         << std::hex << submission_graphics.color_image_address
                         << std::dec << '\n';
    }
    write_private_progress(state, "graphics-submit-end");
    if (state.last_rt64_error != jfg::Rt64ShellError::none) {
      const jfg::Rt64GraphicsDiagnostics rejected = submission_graphics;
      if (!state.rdram_capture_path.empty())
        (void)write_private_rt64_snapshot(state, pending.rdram);
      std::array<char, 192U> operation{};
      (void)std::snprintf(
          operation.data(), operation.size(),
          "submit-%u-c%08x-n%x-r%08x-w%08x-%08x-q%u-d%08x-"
          "s%08x-t%u-l%016llx-%04x",
          static_cast<unsigned>(state.last_rt64_error), pending.descriptor[4],
          pending.descriptor[5], rejected.rejected_command_address,
          rejected.rejected_command_word0, rejected.rejected_command_word1,
          rejected.rejection_reason, rejected.rejection_detail,
          rejected.rejection_source_address, rejected.rejection_triangle,
          static_cast<unsigned long long>(rejected.loaded_vertices_low),
          static_cast<unsigned>(rejected.loaded_vertices_high));
      fail_closed_dispatch(state, "renderer", operation.data(), target);
    }
    const jfg::Rt64GraphicsDiagnostics graphics = submission_graphics;
    const std::uint32_t current_target =
        state.vi_current_framebuffer & 0x00FFFFFFU;
    const std::uint32_t next_target =
        state.vi_next_framebuffer & 0x00FFFFFFU;
    if (require_offscreen_target && !graphics.renderer_paused &&
        (graphics.color_image_address == 0U ||
         graphics.color_image_address == current_target ||
         graphics.color_image_address == next_target))
      fail_closed_dispatch(state, "renderer", "auxiliary-target", target);
    ++state.graphics_tasks;
    state.graphics_commands += state.rt64_shell->last_command_count();
    state.graphics_triangles += graphics.triangles_drawn;
    const std::size_t graphics_slot =
        state.recent_graphics_position %
        state.recent_graphics_color_images.size();
    state.recent_graphics_command_addresses[graphics_slot] =
        pending.descriptor[4];
    state.recent_graphics_color_images[graphics_slot] =
        graphics.color_image_address;
    state.recent_graphics_vi_selections[graphics_slot] =
        selected_vi_framebuffer;
    state.recent_graphics_matrix_commands[graphics_slot] =
        static_cast<std::uint32_t>(graphics.matrix_commands);
    state.recent_graphics_vertices_loaded[graphics_slot] =
        static_cast<std::uint32_t>(graphics.vertices_loaded);
    state.recent_graphics_vertices_in_clip[graphics_slot] =
        static_cast<std::uint32_t>(graphics.vertices_in_clip);
    state.recent_graphics_triangles[graphics_slot] =
        static_cast<std::uint32_t>(graphics.triangles_drawn);
    ++state.recent_graphics_position;
    if (graphics.color_image_address != 0U) {
      state.graphics_color_image = graphics.color_image_address;
      state.graphics_frame_ready = true;

    }
    // Submission and writeback have consumed this snapshot. RT64 owns the
    // resident render targets; keeping a snapshot is not proof of residency.
    state.graphics_snapshot_pool.push_back(std::move(pending.rdram));
    state.community_snapshot_pool_dirty.push_back(std::move(pending.community_upper_dirty));
    if (state.guest_os_probe) continue; // device event owns status/IRQ; original OS owns messages
    complete_guest_sp_status(state, target);
    if (state.scheduler->send(sp->second.first, sp->second.second, false) !=
        hle::kOsSuccess)
      fail_closed_dispatch(state, "rsp", "sp-completion", target);
    const std::int32_t dp_delivery =
        state.scheduler->send(dp->second.first, dp->second.second, false);
    if (dp_delivery != hle::kOsSuccess && dp_delivery != hle::kOsFull)
      fail_closed_dispatch(state, "rsp", "dp-completion", target);
  }
  state.pending_graphics_tasks.clear();
}
#endif

void set_result(recomp_context *context, std::int32_t result) {
  context->r2 = static_cast<std::uint64_t>(static_cast<std::int64_t>(result));
}

#if defined(JFG_PHASE8_LIVE_RUNTIME)
bool complete_flash_operation(State &state) {
  const jfg::FlashRamResult result =
      state.flash_path.empty()
          ? state.flashram.complete()
          : state.flashram.complete_and_persist(state.flash_path);
  if (result.ok() && !state.flash_path.empty())
    ++state.flash_persist_count;
  return result.ok();
}
#endif

bool guest_byte_offset(const std::uint32_t address, std::size_t &offset) {
  const std::uint32_t segment = address & 0xE0000000U;
  const std::uint32_t physical = address & 0x1FFFFFFFU;
  if ((segment != 0x80000000U && segment != 0xA0000000U) ||
      physical >= kRdramSize)
    return false;
  offset = static_cast<std::size_t>(physical) ^ 3U;
  return true;
}
bool write_guest_byte(State &state, const std::uint32_t address,
                      const std::uint8_t value) {
  std::size_t offset = 0U;
  if (!guest_byte_offset(address, offset))
    return false;
  state.rdram[offset] = value;
  return true;
}
// Every path that publishes a synthetic overlay reports a failure the same
// way: the publisher's recorded reason, under the generated category.
[[noreturn]] void fail_closed_overlay_publication(State &state,
                                                 const std::uint32_t target) {
  std::array<char, 96U> operation{};
  (void)std::snprintf(operation.data(), operation.size(),
                      "overlay-publication-%s",
                      state.last_overlay_publication_failure);
  fail_closed_dispatch(state, "generated", operation.data(), target);
}

// One VI frame of host service while the scheduler is quiescent: frame
// budget, journal, window/audio service, presentation of the completed
// workload, pending graphics-task completion, VI field selection, SI and VI
// message delivery. Extracted from the osStartThread handler so the
// scheduler loop reads as a loop; behavior is unchanged.
// Read-only SI timing evidence. Result is a queue return code, not a claim
// about hardware interrupt latency. Enabled only by focused update capture.
void trace_si_timing(State &state, const char *event, std::uint32_t queue,
                     std::uint32_t caller, std::int32_t result) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  const auto si = state.events.find(5U);
  if (si == state.events.end() || queue != si->second.first ||
      state.focus_update_start == 0U || !state.timing_trace.is_open() ||
      state.completed_game_updates + 1U < state.focus_update_start ||
      state.completed_game_updates + 1U > state.focus_update_end + 1U) return;
  state.timing_trace << "si-timing\t" << state.vi_frames << '\t'
      << state.vi_retraces << '\t' << state.controller_samples << '\t'
      << state.completed_game_updates << '\t' << event << '\t'
      << std::hex << queue << '\t' << caller << std::dec << '\t'
      << result << '\t' << state.cpu_count << '\n';
#else
  (void)state; (void)event; (void)queue; (void)caller; (void)result;
#endif
}

void service_si_deadline(State &state, const std::uint32_t target) {
  const auto completion = state.si_deadline.take(state.cpu_count);
  if (!completion) return;
  if (state.scheduler == nullptr || state.pending_si_completions != 1U)
    fail_closed_dispatch(state, "si", "deadline-ownership", target);
  state.pending_si_completions = 0U;
  const auto result = state.scheduler->send(completion->queue, completion->message, false);
  trace_si_timing(state, "deadline-deliver", completion->queue, 0U, result);
  state.journal.append(NativeEventKind::kController, 4U, completion->queue,
                       result == hle::kOsSuccess ? 1U : 0U);
  if (result == hle::kOsSuccess && state.scheduler_running &&
      state.scheduler->current_thread_id() >= 0)
    state.scheduler->yield_current();
}

void schedule_si_deadline(State &state, const std::uint32_t target) {
  const auto event = state.events.find(5U);
  // Measured oracle DMA-return-to-ack interval for the pinned early case.
  // This is an opt-in causal experiment, NOT an N64 hardware latency claim.
  constexpr std::uint64_t observed_ack_ticks = 2568U;
  if (event == state.events.end() || state.pending_si_completions != 1U ||
      !state.si_deadline.start(state.cpu_count, observed_ack_ticks,
                               event->second.first, event->second.second))
    fail_closed_dispatch(state, "si", "deadline-submission", target);
}

void service_timers(State &state, const std::uint32_t target) {
  if (!state.timers.next().has_value()) return;
  if (state.scheduler == nullptr)
    fail_closed_dispatch(state, "timer", "missing-scheduler", target);
  hle::GuestMemory memory({state.rdram, kRdramSize},
                          hle::GuestMemory::Layout::native_word_big_endian);
  bool delivered = false;
  if (!state.timers.service(memory, state.cpu_count,
      [&](std::uint32_t queue, std::uint32_t message) {
        delivered |= state.scheduler->send(queue, message, false) == hle::kOsSuccess;
      }))
    fail_closed_dispatch(state, "timer", "invalid-timer-state", target);
  if (delivered && state.scheduler_running &&
      state.scheduler->current_thread_id() >= 0)
    state.scheduler->yield_current();
}

// Diagnostic only: the dispatch-based Count is not hardware CPU timing.
void trace_guest_clock(State &state, const char *event, std::uint32_t address) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (state.focus_update_start == 0U || !state.timing_trace.is_open() ||
      state.completed_game_updates + 1U < state.focus_update_start - 1U ||
      state.completed_game_updates + 1U > state.focus_update_end + 1U) return;
  state.timing_trace << "guest-clock\t" << state.completed_game_updates << '\t'
      << state.controller_samples << '\t' << state.vi_retraces << '\t'
      << event << '\t' << std::hex << address << std::dec << '\t'
      << state.cpu_count << '\n';
  state.timing_trace << "execution-work\t" << state.completed_game_updates << '\t'
      << state.controller_samples << '\t' << state.vi_retraces << '\t'
      << event << '\t' << std::hex << address << std::dec << '\t'
      << state.observed_guest_instructions << '\n';
  if (state.observed_guest_instructions != 0U) {
    state.timing_trace << "execution-hle\t" << state.completed_game_updates << '\t'
        << event << '\t' << std::hex << address << std::dec;
    for (std::size_t index = 0U; index < kMappingCount; ++index) {
      if (state.mapped_call_counts[index] != 0U)
        state.timing_trace << '\t' << kMappings[index].name << '='
            << state.mapped_call_counts[index];
    }
    state.timing_trace << '\n';
    state.timing_trace << "execution-dispatches\t" << state.completed_game_updates
        << '\t' << event << '\t' << std::hex << address;
    const auto count = (std::min)(state.recent_dispatch_position, state.recent_dispatches.size());
    for (std::size_t offset = 0U; offset < count; ++offset)
      state.timing_trace << '\t' << state.recent_dispatches[
          (state.recent_dispatch_position - count + offset) % state.recent_dispatches.size()];
    state.timing_trace << std::dec << '\n';
  }
#else
  (void)state; (void)event; (void)address;
#endif
}

// Legacy cooperative clock only. Keep this out of the device/presentation
// operations below so an execution-driven dispatcher can call those at
// independent deadlines without adding a frame of time a second time.
void advance_legacy_vi_clock(State &state, const std::uint32_t target) {
  trace_guest_clock(state, "vi-service-entry", target);
  const std::uint64_t frame_budget =
      static_cast<std::uint64_t>(state.retrace_target) * 20U;
  if (++state.vi_frames > frame_budget) {
    fail_closed_dispatch(state, "vi", "frame-budget", target);
  }
  const auto frame_end = state.cpu_count + kCountTicksPerViFrame;
  // A blocked receiver can resume at a timer deadline between VI events.
  // Do not round the controller power-on wait to whole video frames.
  while (true) {
    auto deadline = state.timers.next();
    const auto si_deadline = state.si_deadline.next();
    if (si_deadline && (!deadline || *si_deadline < *deadline)) deadline = si_deadline;
    if (!deadline) break;
    if (*deadline > frame_end) break;
    state.cpu_count = (std::max)(state.cpu_count, *deadline);
    service_si_deadline(state, target);
    service_timers(state, target);
    const auto outcome = state.scheduler->run(1'000'000U);
    if (outcome == ScheduleOutcome::kUnbounded)
      fail_closed_dispatch(state, "timer", "scheduler-step-budget", target);
    if (state.cpu_count >= frame_end) break;
  }
  state.cpu_count = (std::max)(state.cpu_count, frame_end);
  trace_guest_clock(state, "vi-deadline", target);
}

// Presentation consumes already completed work. It does not complete a
// graphics task, advance Count, or deliver a guest interrupt.
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
// ---------------------------------------------------------------------------
// Community change (AI-assisted, Claude, 2026-10): no stale re-presents,
// optional shrink-beam bolt pacing, and an effect recorder.
//
// Stale presents. The VI keeps showing the buffer it already shows until the
// game swaps buffers. In the intro's ship teleport the game's frame takes
// four VIs and three graphics tasks (scene, then two off-screen passes), and
// each completed task presented the VI buffer again. RT64 showed the frame
// still being prepared on the first of those presents and the older frame on
// the second, so every game frame went new, old, new and the lightning
// flickered back and forth (seen in captured frames, effect log runs of
// 2026-10-06). A present is now skipped when the VI still selects the buffer
// presented last and the task that just completed did not draw into it; the
// next present waits for the buffer swap. JFG_SKIP_STALE_PRESENTS=0 restores
// the old behaviour.
//
// Shrink-beam bolt pacing (off by default). shrinkbeamControl (overlay 28)
// rolls mathRnd(0, 2) once per update, and on a 0 shows a lightning sprite
// (objPrintCustomObject draws it while the u8 timer at +2687 is non-zero) for
// 4-6 ticks at a random angle (+2678) and size (+2644). Setting
// JFG_BEAM_FLASH_TICKS (ticks, default 0 = the game's own per-update roll)
// allows the roll only once per that many ticks, keeping the game's 1-in-3
// chance and random angle and size. JFG_BEAM_BOLT_STYLE picks what is shown
// between rolls: 1 (default) steady, a bolt stays up, slowly turning, until
// the next successful roll replaces it; 0 N64 flash, a bolt stays up for
// JFG_BEAM_FLASH_TICKS ticks, then none until the next roll.
// JFG_BEAM_STRAND_SPEED (percent, default 100) scales the strands' twist.
//
// teleportEffect (overlay 126) can be paced the same way with
// JFG_TELEPORT_PACING (ticks; default 0 = off); JFG_TELEPORT_N64_RATE and
// JFG_TELEPORT_SPEED are experimental ring-speed knobs (off by default).
//
// The recorder writes %LOCALAPPDATA%\JFGRecomp\diag\effect-activity.tsv
// (JFG_EFFECT_ACTIVITY_LOG=0 disables it):
//   F  one line per game update: overlay entry points called, as
//      address*count (overlay N is linked at N MiB: overlay = address >> 20)
//   TI teleportEffectInit: object, setup bytes, recent dispatch chain
//   TC state after teleportEffectControl (per ring: bob phase, bob speed,
//      spin, spin speed, pulse, y, radius, fall speed, alpha, delay, rgb)
//   TD teleportEffectDraw
//   SI shrinkbeamInit: object, setup bytes, recent dispatch chain
//   SC shrinkbeamControl: rate, gate mode, flash timer before/after, gate
//      state (ticks since roll, hold, steady bolt), charge/max, flags,
//      bolt angle/size, glow, strand phase/speed/step
//   P  presented framebuffer: frame, ms, VI count, VI buffer, last color image
//   PS skipped stale present (same fields)
//   C  captured frame: game update, ms, presented frame, size, file
//   W  watched texture changed: game update, ms, hash, alpha texels, alpha sum
//   FB RT64 framebuffers overlapping the watched texture (when that changes)
//   BS blood/coffee spurt before bloodSpurtsDraw: game update, ms, slot,
//      state, splat flag, life, texture, colour, position, surface normal,
//      corner offsets, hidden (1 when the splat quad was hidden)
//
// Splats (coffee-throw investigation, 2026-10-07). Overlay 8 (blood spurts,
// also Juno's coffee in the intro) flies drops as tracers; when a drop's
// track intersection test hits a surface it becomes a flat splat quad laid
// on that surface (flag +82). In the intro the coffee splat shows up as a
// flat sheet hanging in the air, which the N64 does not show.
// JFG_HIDE_SPLATS=1 (default) hides the splat quads (drops still show) by
// clearing the flag around the draw call only; 0 shows them.
//
// Reflections (elevator investigation, 2026-10-07). Overlay 137
// realReflectPrint (+0x25C) redraws the player character mirrored in
// reflective track surfaces (flag 0x20000000, collected by
// realReflectSurfaces). In the intro elevator a second Juno head shows on
// the glass tube. Hiding it did not remove the second head (Luke, v12 run),
// so JFG_HIDE_REFLECTIONS now defaults to 0 (draw reflections); 1 skips the
// call. RR lines: game update, ms, reflective surface count,
// hidden.
//
// Frame capture (JFG_EFFECT_CAPTURE = maximum frames, default 0 = off):
// while a shrink beam is charged, and for 24 presented frames after, every
// presented frame is saved, shrunk to about 640 pixels wide, as a PPM in
// %LOCALAPPDATA%\JFGRecomp\diag\frames\run-<time>\. While beams exist but
// are not charged, every 16th presented frame is saved for context.
// Since 2026-10-18 the beam and timed captures are off unless
// JFG_EFFECT_CAPTURE and JFG_CAPTURE_EVERY are set.
//
// F9 recording (2026-10-18): F9 in the game window starts a recording, F9
// again stops it. The effect log gets "K" lines (frame, ms, VI, 1 = start /
// 0 = stop, recording number), so the present timings (P lines) of that
// stretch can be read out. While recording, every Nth presented frame is
// saved (JFG_RECORD_EVERY, default 2; 0 = timing marks only) into a new
// frames\run-<time>\ folder per recording, at most 1800 frames each.
// ---------------------------------------------------------------------------
// Overlays are matched by link address (overlay N is linked at N MiB). The
// dispatcher's section number is an index into the generated section table,
// which skips overlays with no code, so it is not the overlay number.
constexpr std::uint32_t kTeleportEffectOverlay = 126U;
constexpr std::uint32_t kTeleportEffectTextSize = 0x1250U;
constexpr std::uint32_t kTeleportEffectInitOffset = 0x000U;
constexpr std::uint32_t kTeleportEffectControlOffset = 0x3D0U;
constexpr std::uint32_t kTeleportEffectDrawOffset = 0xE74U;
constexpr std::uint32_t kBeamOverlay = 28U;
constexpr std::uint32_t kBeamTextSize = 0x1BB0U;
constexpr std::uint32_t kShrinkbeamInitOffset = 0x000U;
constexpr std::uint32_t kShrinkbeamControlOffset = 0x31CU;
constexpr std::uint32_t kBeamFlashTimer = 2687U;
constexpr std::uint32_t kBeamCharge = 2680U;
constexpr std::uint32_t kBeamChargeMax = 2682U;
constexpr std::uint32_t kGameUpdateEntry = 0x80044FACU;
constexpr std::uint32_t kDelayDatAddress = 0x800A3374U;

struct CommunityEffectCall {
  bool control = false;
  bool blood = false;
  std::uint64_t splat_mask = 0U;
  std::uint32_t blood_base = 0U;
  std::array<std::uint8_t, 64U> splat_flags{};
  bool skip = false;
  bool restore = false;
  bool beam = false;
  bool strand_restore = false;
  std::uint8_t flash_mode = 0U;  // 1 hold bolt, 2 skip roll, 3 allow roll
  std::uint8_t flash_before = 0U;
  std::array<std::int16_t, 3U> strand_speed{};
  std::uint32_t object = 0U;
  std::uint32_t block = 0U;
  std::uint32_t rate = 0U;
  std::array<std::int16_t, 4U> bob_speed{};
  std::array<std::int16_t, 4U> spin_speed{};
  std::array<std::int16_t, 4U> applied_bob{};
  std::array<std::int16_t, 4U> applied_spin{};
};

class CommunityGuest final {
public:
  explicit CommunityGuest(std::uint8_t *rdram) noexcept
      : memory_({rdram, kRdramSize},
                hle::GuestMemory::Layout::native_word_big_endian) {}

  std::uint32_t u32(const std::uint32_t address) const noexcept {
    std::uint32_t value = 0U;
    if (!memory_.read_u32(address & ~3U, value))
      return 0U;
    return value;
  }
  std::int16_t s16(const std::uint32_t address) const noexcept {
    const std::uint32_t word = u32(address);
    return static_cast<std::int16_t>(
        (address & 2U) != 0U ? (word & 0xFFFFU) : (word >> 16U));
  }
  std::uint8_t u8(const std::uint32_t address) const noexcept {
    const std::uint32_t word = u32(address);
    return static_cast<std::uint8_t>(word >> ((3U - (address & 3U)) * 8U));
  }
  float f32(const std::uint32_t address) const noexcept {
    return std::bit_cast<float>(u32(address));
  }
  bool write_u8(const std::uint32_t address,
                const std::uint8_t value) noexcept {
    std::uint32_t word = 0U;
    if (!memory_.read_u32(address & ~3U, word))
      return false;
    const std::uint32_t shift = (3U - (address & 3U)) * 8U;
    word = (word & ~(0xFFU << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
    return memory_.write_u32(address & ~3U, word);
  }
  bool write_s16(const std::uint32_t address,
                 const std::int16_t value) noexcept {
    std::uint32_t word = 0U;
    if (!memory_.read_u32(address & ~3U, word))
      return false;
    const auto half = static_cast<std::uint32_t>(
        static_cast<std::uint16_t>(value));
    if ((address & 2U) != 0U)
      word = (word & 0xFFFF0000U) | half;
    else
      word = (word & 0x0000FFFFU) | (half << 16U);
    return memory_.write_u32(address & ~3U, word);
  }

private:
  hle::GuestMemory memory_;
};

std::uint32_t community_env_number(const char *name,
                                   const std::uint32_t fallback) {
  std::array<char, 16U> text{};
  const DWORD length = GetEnvironmentVariableA(
      name, text.data(), static_cast<DWORD>(text.size()));
  if (length == 0U || length >= text.size())
    return fallback;
  std::uint32_t value = 0U;
  const auto parsed =
      std::from_chars(text.data(), text.data() + length, value);
  return parsed.ec == std::errc{} ? value : fallback;
}

std::int64_t community_elapsed_ms(const State &state) {
  return static_cast<std::int64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - state.community_effect_start)
          .count());
}

std::ofstream *community_effect_log(State &state) {
  if (!state.community_effect_log_tried) {
    state.community_effect_log_tried = true;
    state.community_effect_start = std::chrono::steady_clock::now();
    state.community_teleport_pacing =
        (std::min)(community_env_number("JFG_TELEPORT_PACING", 0U), 6U);
    state.community_beam_flash_ticks =
        (std::min)(community_env_number("JFG_BEAM_FLASH_TICKS", 0U), 60U);
    state.community_beam_bolt_style =
        (std::min)(community_env_number("JFG_BEAM_BOLT_STYLE", 1U), 1U);
    state.community_beam_strand_percent =
        (std::min)(community_env_number("JFG_BEAM_STRAND_SPEED", 100U), 400U);
    state.community_capture_limit =
        (std::min)(community_env_number("JFG_EFFECT_CAPTURE", 0U), 2000U);
    state.community_skip_stale_presents =
        community_env_number("JFG_SKIP_STALE_PRESENTS", 1U) != 0U;
    state.community_hide_splats =
        community_env_number("JFG_HIDE_SPLATS", 1U) != 0U;
    state.community_hide_reflections =
        community_env_number("JFG_HIDE_REFLECTIONS", 0U) != 0U;
    // Timed capture (elevator investigation): every Nth presented frame
    // between JFG_CAPTURE_FROM_S and JFG_CAPTURE_TO_S seconds of the run.
    state.community_capture_every =
        (std::min)(community_env_number("JFG_CAPTURE_EVERY", 0U), 600U);
    state.community_hotkey_every =
        (std::min)(community_env_number("JFG_RECORD_EVERY", 2U), 60U);
    state.community_ranged_import =
        community_env_number("JFG_RANGED_IMPORT", 1U) != 0U;
    state.community_capture_from_ms =
        static_cast<std::int64_t>(community_env_number("JFG_CAPTURE_FROM_S", 105U)) * 1000;
    state.community_capture_to_ms =
        static_cast<std::int64_t>(community_env_number("JFG_CAPTURE_TO_S", 168U)) * 1000;
    state.community_teleport_n64_rate =
        (std::min)(community_env_number("JFG_TELEPORT_N64_RATE", 0U), 6U);
    state.community_teleport_speed_percent =
        (std::min)(community_env_number("JFG_TELEPORT_SPEED", 100U), 400U);
    std::array<char, MAX_PATH> base{};
    const DWORD length = GetEnvironmentVariableA(
        "LOCALAPPDATA", base.data(), static_cast<DWORD>(base.size()));
    if (community_env_number("JFG_EFFECT_ACTIVITY_LOG", 1U) != 0U &&
        length != 0U && length < base.size()) {
      std::error_code error;
      const auto directory =
          std::filesystem::path(base.data()) / "JFGRecomp" / "diag";
      std::filesystem::create_directories(directory, error);
      state.community_effect_stream.open(directory / "effect-activity.tsv",
                                      std::ios::out | std::ios::trunc);
      if (state.community_effect_stream.is_open())
        state.community_effect_stream
            << "# effect-activity v10\tskip_stale_presents="
            << (state.community_skip_stale_presents ? 1 : 0)
            << "\thide_splats=" << (state.community_hide_splats ? 1 : 0)
            << "\thide_reflections="
            << (state.community_hide_reflections ? 1 : 0)
            << "\tbeam_flash_ticks="
            << state.community_beam_flash_ticks << "\tbeam_bolt_style="
            << state.community_beam_bolt_style << "\tbeam_strand_speed="
            << state.community_beam_strand_percent << "\tteleport_pacing="
            << state.community_teleport_pacing << "\tn64_rate="
            << state.community_teleport_n64_rate << "\tspeed_percent="
            << state.community_teleport_speed_percent << '\n';
    }
  }
  return state.community_effect_stream.is_open() ? &state.community_effect_stream
                                              : nullptr;
}

std::int64_t community_wrap16(const std::int64_t value) noexcept {
  std::int64_t wrapped = value % 65536;
  if (wrapped >= 32768)
    wrapped -= 65536;
  else if (wrapped < -32768)
    wrapped += 65536;
  return wrapped;
}

std::int16_t community_clamp_s16(const double value) noexcept {
  if (value >= 32767.0)
    return static_cast<std::int16_t>(32767);
  if (value <= -32768.0)
    return static_cast<std::int16_t>(-32768);
  if (!(value > -32768.0 && value < 32767.0))
    return static_cast<std::int16_t>(0); // NaN
  return static_cast<std::int16_t>(value >= 0.0 ? value + 0.5 : value - 0.5);
}

// Speed that reproduces, per game tick, the motion the N64 showed for a
// per-tick speed whose per-frame step wrapped around a full turn.
std::int16_t community_n64_speed(const std::int16_t speed, const double scale,
                                 const std::uint32_t n64_rate) noexcept {
  if (n64_rate == 0U || !(scale > 0.0001))
    return speed;
  const auto n64_step = static_cast<std::int64_t>(
      static_cast<double>(speed) * static_cast<double>(n64_rate) * scale);
  if (n64_step > -32768 && n64_step < 32768)
    return speed;
  const double apparent = static_cast<double>(community_wrap16(n64_step));
  return community_clamp_s16(apparent /
                             (static_cast<double>(n64_rate) * scale));
}

// Texture watch (coffee-splash investigation, 2026-10-07): RT64 showed the
// splash's liquid sheet using a 32x32 IA8 texture at 0x80358460 (1 KiB),
// grey and speckled where the N64 shows nothing. Each game update this
// hashes those bytes; on a change it writes a W line (hash, texels with
// non-zero alpha, alpha sum) and, for the first 60 changes, the raw bytes
// to diag\watch\f<frame>.bin.
constexpr std::uint32_t kWatchAddress = 0x80358460U;
constexpr std::uint32_t kWatchBytes = 1024U;

void community_watch_texture(State &state, const CommunityGuest &guest,
                             std::ofstream &effect_log) {
  std::array<char, kWatchBytes> bytes{};
  std::uint64_t hash = 1469598103934665603ULL;
  std::uint32_t alpha_texels = 0U, alpha_sum = 0U;
  for (std::uint32_t offset = 0U; offset < kWatchBytes; offset += 4U) {
    const std::uint32_t word = guest.u32(kWatchAddress + offset);
    for (std::uint32_t lane = 0U; lane < 4U; ++lane) {
      const auto value =
          static_cast<std::uint8_t>(word >> ((3U - lane) * 8U));
      bytes[offset + lane] = static_cast<char>(value);
      hash = (hash ^ value) * 1099511628211ULL;
      const std::uint32_t alpha = value & 0x0FU;
      alpha_sum += alpha;
      if (alpha != 0U)
        ++alpha_texels;
    }
  }
  if (hash == state.community_watch_hash)
    return;
  state.community_watch_hash = hash;
  effect_log << "W\t" << state.community_effect_frame << '\t'
             << community_elapsed_ms(state) << '\t' << std::hex << hash
             << std::dec << '\t' << alpha_texels << '\t' << alpha_sum
             << '\n';
  if (state.community_watch_dumps >= 60U)
    return;
  std::array<char, MAX_PATH> base{};
  const DWORD length = GetEnvironmentVariableA(
      "LOCALAPPDATA", base.data(), static_cast<DWORD>(base.size()));
  if (length == 0U || length >= base.size())
    return;
  const auto directory =
      std::filesystem::path(base.data()) / "JFGRecomp" / "diag" / "watch";
  std::error_code error;
  std::filesystem::create_directories(directory, error);
  std::ofstream dump(directory / ("f" + std::to_string(
                                            state.community_effect_frame) +
                                  ".bin"),
                     std::ios::binary);
  dump.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  ++state.community_watch_dumps;
}

void community_effect_frame(State &state, std::uint8_t *rdram) {
  std::ofstream *effect_log = community_effect_log(state);
  if (effect_log == nullptr) {
    state.community_effect_counts.clear();
    return;
  }
  community_watch_texture(state, CommunityGuest(rdram), *effect_log);
  if (state.rt64_shell != nullptr) {
    // RT64 framebuffers that claim the watched texture's memory.
    std::string overlap = state.rt64_shell->debug_framebuffers_overlapping(
        kWatchAddress & 0x00FFFFFFU, (kWatchAddress & 0x00FFFFFFU) + kWatchBytes);
    if (overlap != state.community_fb_summary) {
      *effect_log << "FB\t" << state.community_effect_frame << '\t'
                  << community_elapsed_ms(state) << '\t' << overlap << '\n';
      state.community_fb_summary = std::move(overlap);
    }
  }
  const CommunityGuest guest(rdram);
  {
    // G line: blurOn / blurTaskActive changed.
    const std::uint64_t blur =
        (static_cast<std::uint64_t>(guest.u32(0x800A3338U)) << 32U) |
        guest.u32(0x800A3630U);
    if (blur != state.community_blur_state) {
      state.community_blur_state = blur;
      *effect_log << "G\t" << state.community_effect_frame << '\t'
                  << community_elapsed_ms(state) << '\t' << std::hex
                  << (blur >> 32U) << '\t' << (blur & 0xFFFFFFFFULL)
                  << std::dec << '\n';
    }
  }
  std::vector<std::pair<std::uint32_t, std::uint32_t>> entries(
      state.community_effect_counts.begin(),
      state.community_effect_counts.end());
  std::sort(entries.begin(), entries.end());
  *effect_log << "F\t" << state.community_effect_frame << '\t'
       << community_elapsed_ms(state) << '\t' << state.vi_retraces << '\t'
       << guest.u32(kDelayDatAddress) << '\t' << std::hex;
  for (const auto &[address, count] : entries)
    *effect_log << address << '*' << count << ' ';
  *effect_log << std::dec << '\n';
  if ((state.community_effect_frame % 30U) == 0U)
    effect_log->flush();
  ++state.community_effect_frame;
  state.community_effect_counts.clear();
}

constexpr std::uint32_t kBloodOverlay = 8U;
constexpr std::uint32_t kBloodTextSize = 0x1660U;
constexpr std::uint32_t kBloodDrawOffset = 0xAB4U;
constexpr std::uint32_t kBloodSpurtTable = 0x17C0U;
constexpr std::uint32_t kBloodSpurtCount = 64U;
constexpr std::uint32_t kBloodSpurtBytes = 84U;

// Overlay data lives at its link address (overlay 8 at 0x00800000), which
// the host maps outside the 8 MiB RDRAM span, so it is read the same way the
// generated code reads it (MEM_W/MEM_BU in recomp.h) rather than through
// CommunityGuest.
std::uint8_t *community_guest_host(std::uint8_t *rdram,
                                   const std::uint32_t address) noexcept {
  const auto extended = static_cast<std::uint64_t>(static_cast<std::int64_t>(
      static_cast<std::int32_t>(address)));
  return rdram + (extended - 0xFFFFFFFF80000000ULL);
}
std::uint8_t community_raw_u8(std::uint8_t *rdram, const std::uint32_t address) noexcept {
  return *community_guest_host(rdram, address ^ 3U);
}
void community_raw_write_u8(std::uint8_t *rdram, const std::uint32_t address,
                            const std::uint8_t value) noexcept {
  *community_guest_host(rdram, address ^ 3U) = value;
}
std::int16_t community_raw_s16(std::uint8_t *rdram, const std::uint32_t address) noexcept {
  std::int16_t value = 0;
  std::memcpy(&value, community_guest_host(rdram, address ^ 2U), sizeof(value));
  return value;
}
float community_raw_f32(std::uint8_t *rdram, const std::uint32_t address) noexcept {
  float value = 0.0F;
  std::memcpy(&value, community_guest_host(rdram, address), sizeof(value));
  return value;
}

void community_blood_before(State &state, std::uint8_t *rdram,
                            const std::uint32_t section,
                            const JfgGeneratedSectionMetadata &metadata,
                            CommunityEffectCall &call) {
  const std::uint32_t section_base =
      section_addresses != nullptr
          ? static_cast<std::uint32_t>(section_addresses[section])
          : metadata.linked_vram;
  const std::uint32_t base = section_base + kBloodSpurtTable;
  std::ofstream *effect_log = community_effect_log(state);
  if (effect_log != nullptr && !state.community_blood_base_logged) {
    state.community_blood_base_logged = true;
    *effect_log << "BB\t" << state.community_effect_frame << '\t' << std::hex
                << section_base << '\t' << metadata.linked_vram << std::dec
                << '\n';
  }
  for (std::uint32_t slot = 0U; slot < kBloodSpurtCount; ++slot) {
    const std::uint32_t spurt = base + slot * kBloodSpurtBytes;
    const std::uint8_t spurt_state = community_raw_u8(rdram, spurt + 74U);
    if (spurt_state == 0U)
      continue;
    const std::uint8_t splat = community_raw_u8(rdram, spurt + 82U);
    const bool hide = splat != 0U && state.community_hide_splats &&
                      (spurt_state == 2U || spurt_state == 3U);
    if (hide) {
      community_raw_write_u8(rdram, spurt + 82U, 0U);
      call.splat_mask |= 1ULL << slot;
      call.splat_flags[slot] = splat;
    }
    if (effect_log == nullptr || state.community_blood_lines >= 4000U)
      continue;
    ++state.community_blood_lines;
    *effect_log << "BS\t" << state.community_effect_frame << '\t'
                << community_elapsed_ms(state) << '\t' << slot << '\t'
                << static_cast<unsigned>(spurt_state) << '\t'
                << static_cast<unsigned>(splat) << '\t'
                << community_raw_s16(rdram, spurt + 60U) << '\t'
                << static_cast<unsigned>(community_raw_u8(rdram, spurt + 81U)) << '\t'
                << static_cast<unsigned>(community_raw_u8(rdram, spurt + 80U)) << '\t'
                << community_raw_f32(rdram, spurt + 0U) << ','
                << community_raw_f32(rdram, spurt + 4U) << ','
                << community_raw_f32(rdram, spurt + 8U) << '\t'
                << community_raw_f32(rdram, spurt + 44U) << ','
                << community_raw_f32(rdram, spurt + 48U) << ','
                << community_raw_f32(rdram, spurt + 52U) << '\t';
    for (std::uint32_t corner = 0U; corner < 6U; ++corner)
      *effect_log << community_raw_s16(rdram, spurt + 62U + corner * 2U)
                  << (corner == 5U ? '\t' : ',');
    *effect_log << (hide ? 1 : 0) << '\n';
  }
  call.blood = call.splat_mask != 0U;
  call.blood_base = base;
}

void community_blood_after(std::uint8_t *rdram,
                           const CommunityEffectCall &call) {
  for (std::uint32_t slot = 0U; slot < kBloodSpurtCount; ++slot)
    if ((call.splat_mask & (1ULL << slot)) != 0U)
      community_raw_write_u8(rdram,
                             call.blood_base + slot * kBloodSpurtBytes + 82U,
                             call.splat_flags[slot]);
}

void community_log_chain(const State &state, std::ofstream &effect_log) {
  effect_log << "\tchain" << std::hex;
  const std::size_t ring = state.recent_dispatches.size();
  const std::size_t depth =
      (std::min)(state.recent_dispatch_position, std::size_t{16U});
  for (std::size_t back = depth; back >= 1U; --back)
    effect_log << ' '
               << state.recent_dispatches[(state.recent_dispatch_position -
                                           back) %
                                          ring];
  effect_log << std::dec;
}

// Shrink-beam lightning: gate the per-update bolt roll to the N64 cadence.
// Gate state per object: first = ticks since the last allowed roll,
// second = ticks the current bolt stays up (N64 flash style only).
// community_beam_has_bolt holds the beams that show a steady bolt.
void community_beam_before(State &state, std::uint8_t *rdram,
                           const recomp_context &context,
                           const std::uint32_t offset,
                           CommunityEffectCall &call) {
  CommunityGuest guest(rdram);
  if (offset == kShrinkbeamInitOffset) {
    const auto object = static_cast<std::uint32_t>(context.r4);
    const auto setup = static_cast<std::uint32_t>(context.r5);
    state.community_flash_gates.erase(object);
    state.community_beam_has_bolt.erase(object);
    std::ofstream *effect_log = community_effect_log(state);
    if (effect_log == nullptr)
      return;
    *effect_log << "SI\t" << state.community_effect_frame << '\t'
                << community_elapsed_ms(state) << '\t' << std::hex << object
                << '\t' << setup << '\t' << std::setfill('0');
    for (std::uint32_t setup_offset = 0U; setup_offset < 48U;
         setup_offset += 4U)
      *effect_log << std::setw(8) << guest.u32(setup + setup_offset);
    *effect_log << std::setfill(' ') << std::dec;
    community_log_chain(state, *effect_log);
    *effect_log << '\n';
    return;
  }
  if (offset != kShrinkbeamControlOffset)
    return;
  call.beam = true;
  call.object = static_cast<std::uint32_t>(context.r4);
  call.rate = static_cast<std::uint32_t>(context.r5);
  call.block = guest.u32(call.object + 104U);
  if (call.block < 0x80000000U)
    return;
  call.flash_before = guest.u8(call.block + kBeamFlashTimer);
  if (state.community_beam_strand_percent != 100U) {
    const double scale = state.community_beam_strand_percent / 100.0;
    for (std::uint32_t strand = 0U; strand < 3U; ++strand) {
      const std::uint32_t field = call.block + 2666U + strand * 2U;
      call.strand_speed[strand] = guest.s16(field);
      (void)guest.write_s16(
          field, community_clamp_s16(call.strand_speed[strand] * scale));
    }
    call.strand_restore = true;
  }
  const std::uint32_t ticks = state.community_beam_flash_ticks;
  if (ticks == 0U || call.rate == 0U || call.rate >= 64U)
    return;
  auto &gate = state.community_flash_gates[call.object];
  if (state.community_beam_bolt_style == 1U) {
    // Steady style: no hold phase; after() keeps the timer at 1 while a
    // bolt is up, so only a value from an ungated roll is 2 or more.
    if (call.flash_before >= 2U)
      state.community_beam_has_bolt.insert(call.object);
    gate.second = 0U;
  } else if (gate.second == 0U && call.flash_before != 0U) {
    gate.second = ticks; // a bolt the gate did not start (spawn update)
  }
  if (gate.second != 0U) {
    // Keep the game on its no-roll path while the bolt stays up.
    (void)guest.write_u8(call.block + kBeamFlashTimer, 0xFFU);
    call.flash_mode = 1U;
    return;
  }
  gate.first += call.rate;
  if (gate.first < ticks) {
    (void)guest.write_u8(call.block + kBeamFlashTimer, 1U); // no roll yet
    call.flash_mode = 2U;
    return;
  }
  gate.first = 0U;
  (void)guest.write_u8(call.block + kBeamFlashTimer, 0U); // allow the roll
  call.flash_mode = 3U;
}

void community_beam_after(State &state, std::uint8_t *rdram,
                          const CommunityEffectCall &call) {
  if (call.block < 0x80000000U)
    return;
  CommunityGuest guest(rdram);
  const std::uint32_t block = call.block;
  if (call.strand_restore)
    for (std::uint32_t strand = 0U; strand < 3U; ++strand)
      (void)guest.write_s16(block + 2666U + strand * 2U,
                            call.strand_speed[strand]);
  std::uint8_t flash_after = guest.u8(block + kBeamFlashTimer);
  std::uint32_t since_roll = 0U;
  std::uint32_t hold = 0U;
  if (call.flash_mode != 0U && state.community_beam_bolt_style == 1U) {
    const auto &gate = state.community_flash_gates[call.object];
    const int charge = guest.s16(block + kBeamCharge);
    const int charge_max = guest.s16(block + kBeamChargeMax);
    if (call.flash_mode == 3U && flash_after != 0U)
      state.community_beam_has_bolt.insert(call.object); // new bolt rolled
    if (charge <= 0)
      state.community_beam_has_bolt.erase(call.object);
    // Same charge test the game uses before it rolls a bolt.
    const bool steady =
        charge > charge_max - 10 &&
        state.community_beam_has_bolt.count(call.object) != 0U;
    flash_after = steady ? 1U : 0U;
    (void)guest.write_u8(block + kBeamFlashTimer, flash_after);
    since_roll = gate.first;
  } else if (call.flash_mode != 0U) {
    auto &gate = state.community_flash_gates[call.object];
    const bool charged = guest.s16(block + kBeamCharge) > 0;
    if (call.flash_mode == 1U) {
      gate.second = gate.second > call.rate ? gate.second - call.rate : 0U;
      if (!charged)
        gate.second = 0U;
      flash_after = gate.second != 0U ? 1U : 0U;
    } else if (call.flash_mode == 2U) {
      flash_after = 0U;
    } else if (flash_after != 0U) {
      gate.second = state.community_beam_flash_ticks; // new bolt
      flash_after = 1U;
    }
    (void)guest.write_u8(block + kBeamFlashTimer, flash_after);
    since_roll = gate.first;
    hold = gate.second;
  }
  state.community_beam_seen_frame = state.community_effect_frame + 1U;
  if (guest.s16(block + kBeamCharge) > 0)
    state.community_capture_left = 24U;
  std::ofstream *effect_log = community_effect_log(state);
  if (effect_log == nullptr)
    return;
  *effect_log << "SC\t" << state.community_effect_frame << '\t'
              << community_elapsed_ms(state) << '\t' << std::hex
              << call.object << std::dec << '\t' << call.rate << '\t'
              << static_cast<unsigned>(call.flash_mode) << '\t'
              << static_cast<unsigned>(call.flash_before) << '\t'
              << static_cast<unsigned>(flash_after) << '\t' << since_roll
              << ',' << hold << ','
              << state.community_beam_has_bolt.count(call.object) << '\t'
              << guest.s16(block + kBeamCharge) << '/'
              << guest.s16(block + kBeamChargeMax) << '\t'
              << static_cast<unsigned>(guest.u8(block + 2685U)) << ','
              << static_cast<unsigned>(guest.u8(block + 2686U)) << ','
              << static_cast<unsigned>(guest.u8(block + 2689U)) << '\t'
              << guest.s16(block + 2678U) << ',' << guest.f32(block + 2644U)
              << '\t' << guest.f32(block + 2648U) << ','
              << guest.f32(block + 2652U);
  for (std::uint32_t strand = 0U; strand < 3U; ++strand)
    *effect_log << "\ts" << strand << ':'
                << guest.s16(block + 2660U + strand * 2U) << ','
                << guest.s16(block + 2666U + strand * 2U) << ','
                << guest.s16(block + 2672U + strand * 2U);
  *effect_log << '\n';
}

CommunityEffectCall community_effect_before(
    State &state, std::uint8_t *rdram, recomp_context &context,
    const std::uint32_t target, const std::uint32_t section,
    const JfgGeneratedSectionMetadata &metadata) {
  CommunityEffectCall call{};
  if (target == kGameUpdateEntry) {
    community_effect_frame(state, rdram);
    return call;
  }
  if (metadata.is_overlay != 1U)
    return call;
  if (state.community_effect_stream.is_open())
    ++state.community_effect_counts[target];
  static_cast<void>(section);
  const std::uint32_t overlay = metadata.linked_vram >> 20U;
  if (overlay == 137U && metadata.text_size == 0x1750U) {
    if (target - metadata.linked_vram == 0x25CU) {
      std::ofstream *effect_log = community_effect_log(state);
      call.skip = state.community_hide_reflections;
      if (effect_log != nullptr && state.community_reflect_lines < 3000U) {
        ++state.community_reflect_lines;
        const std::uint32_t section_base =
            section_addresses != nullptr
                ? static_cast<std::uint32_t>(section_addresses[section])
                : metadata.linked_vram;
        std::int32_t surfaces = 0;
        std::memcpy(&surfaces,
                    community_guest_host(rdram, section_base + 0x1770U + 132U),
                    sizeof(surfaces));
        *effect_log << "RR\t" << state.community_effect_frame << '\t'
                    << community_elapsed_ms(state) << '\t' << surfaces
                    << '\t' << (call.skip ? 1 : 0) << '\n';
      }
    }
    return call;
  }
  if (overlay == kBloodOverlay && metadata.text_size == kBloodTextSize) {
    if (target - metadata.linked_vram == kBloodDrawOffset)
      community_blood_before(state, rdram, section, metadata, call);
    return call;
  }
  if (overlay == kBeamOverlay && metadata.text_size == kBeamTextSize) {
    community_beam_before(state, rdram, context,
                          target - metadata.linked_vram, call);
    return call;
  }
  if (overlay != kTeleportEffectOverlay ||
      metadata.text_size != kTeleportEffectTextSize)
    return call;
  std::ofstream *effect_log = community_effect_log(state);
  const std::uint32_t offset = target - metadata.linked_vram;
  CommunityGuest guest(rdram);
  if (offset == kTeleportEffectInitOffset) {
    const auto object = static_cast<std::uint32_t>(context.r4);
    const auto setup = static_cast<std::uint32_t>(context.r5);
    state.community_teleport_pending.erase(object);
    state.community_teleport_seen.erase(object);
    if (effect_log == nullptr)
      return call;
    *effect_log << "TI\t" << state.community_effect_frame << '\t'
         << community_elapsed_ms(state) << '\t' << std::hex << object << '\t'
         << setup << '\t' << std::setfill('0');
    for (std::uint32_t setup_offset = 0U; setup_offset < 48U;
         setup_offset += 4U)
      *effect_log << std::setw(8) << guest.u32(setup + setup_offset);
    *effect_log << std::setfill(' ') << std::dec;
    community_log_chain(state, *effect_log);
    *effect_log << '\n';
    effect_log->flush();
    return call;
  }
  if (offset == kTeleportEffectDrawOffset) {
    if (effect_log == nullptr)
      return call;
    const auto object = static_cast<std::uint32_t>(context.r7);
    const std::uint32_t block = guest.u32(object + 104U);
    *effect_log << "TD\t" << state.community_effect_frame << '\t'
         << community_elapsed_ms(state) << '\t' << std::hex << object
         << std::dec << '\t'
         << static_cast<unsigned>(guest.u8(block + 4040U)) << '\t'
         << guest.s16(block + 4042U) << '\t' << guest.f32(block + 4028U)
         << '\n';
    return call;
  }
  if (offset != kTeleportEffectControlOffset)
    return call;
  call.control = true;
  call.object = static_cast<std::uint32_t>(context.r4);
  call.rate = static_cast<std::uint32_t>(context.r5);
  call.block = guest.u32(call.object + 104U);
  if (call.block < 0x80000000U)
    return call;
  const std::uint32_t pacing = state.community_teleport_pacing;
  if (pacing != 0U && call.rate != 0U && call.rate < 64U) {
    std::uint32_t &pending = state.community_teleport_pending[call.object];
    pending += call.rate;
    // Wait for the next frame only when that lands closer to the N64 step.
    const std::int64_t short_by = static_cast<std::int64_t>(pacing) -
                                  static_cast<std::int64_t>(pending);
    const std::int64_t over_by = static_cast<std::int64_t>(pending) +
                                 static_cast<std::int64_t>(call.rate) -
                                 static_cast<std::int64_t>(pacing);
    if (short_by > 0 && short_by > over_by) {
      call.skip = true;
      return call;
    }
    call.rate = pending;
    context.r5 = pending;
    pending = 0U;
  }
  const std::uint32_t n64_rate =
      pacing != 0U ? 0U : state.community_teleport_n64_rate;
  const double speed = state.community_teleport_speed_percent / 100.0;
  const double ramp = static_cast<double>(guest.f32(call.block + 4028U));
  for (std::uint32_t ring = 0U; ring < 4U; ++ring) {
    call.bob_speed[ring] = guest.s16(call.block + 3848U + ring * 2U);
    call.spin_speed[ring] = guest.s16(call.block + 3864U + ring * 2U);
    call.applied_bob[ring] = community_clamp_s16(
        community_n64_speed(call.bob_speed[ring], ramp, n64_rate) * speed);
    call.applied_spin[ring] = community_clamp_s16(
        community_n64_speed(call.spin_speed[ring], 1.0, n64_rate) * speed);
    if (call.applied_bob[ring] != call.bob_speed[ring] ||
        call.applied_spin[ring] != call.spin_speed[ring])
      call.restore = true;
  }
  if (call.restore) {
    for (std::uint32_t ring = 0U; ring < 4U; ++ring) {
      (void)guest.write_s16(call.block + 3848U + ring * 2U,
                            call.applied_bob[ring]);
      (void)guest.write_s16(call.block + 3864U + ring * 2U,
                            call.applied_spin[ring]);
    }
  }
  return call;
}

void community_effect_after(State &state, std::uint8_t *rdram,
                            const CommunityEffectCall &call) {
  if (call.blood) {
    community_blood_after(rdram, call);
    return;
  }
  if (call.beam) {
    community_beam_after(state, rdram, call);
    return;
  }
  if (!call.control || call.skip || call.block < 0x80000000U)
    return;
  CommunityGuest guest(rdram);
  if (call.restore) {
    for (std::uint32_t ring = 0U; ring < 4U; ++ring) {
      (void)guest.write_s16(call.block + 3848U + ring * 2U,
                            call.bob_speed[ring]);
      (void)guest.write_s16(call.block + 3864U + ring * 2U,
                            call.spin_speed[ring]);
    }
  }
  std::ofstream *effect_log = community_effect_log(state);
  if (effect_log == nullptr)
    return;
  const std::uint32_t block = call.block;
  if (state.community_teleport_seen.insert(call.object).second) {
    // Colour-cycle tables: frame count, total ticks, then rgba:ticks.
    *effect_log << "TT\t" << state.community_effect_frame << '\t' << std::hex
                << call.object;
    for (std::uint32_t ring = 0U; ring < 4U; ++ring) {
      const std::uint32_t table = guest.u32(block + 3924U + ring * 16U);
      const std::uint32_t frames = guest.u32(table);
      *effect_log << "\tr" << ring << ':' << table << ',' << std::dec
                  << frames << ',' << guest.u32(table + 4U) << std::hex;
      for (std::uint32_t frame = 0U; frame < (std::min)(frames, 16U);
           ++frame)
        *effect_log << ' ' << guest.u32(table + 8U + frame * 8U) << ':'
                    << std::dec << guest.u32(table + 12U + frame * 8U)
                    << std::hex;
    }
    *effect_log << std::dec << '\n';
  }
  *effect_log << "TC\t" << state.community_effect_frame << '\t'
       << community_elapsed_ms(state) << '\t' << std::hex << call.object
       << '\t' << block << std::dec << '\t' << call.rate << '\t'
       << guest.s16(block + 4042U) << '\t' << guest.f32(block + 4028U)
       << '\t' << static_cast<unsigned>(guest.u8(block + 4040U)) << '\t'
       << guest.f32(call.object + 12U) << ',' << guest.f32(call.object + 16U)
       << ',' << guest.f32(call.object + 20U) << '\t'
       << guest.s16(block + 3984U) << ',' << guest.f32(block + 3988U) << ','
       << guest.f32(block + 3992U);
  for (std::uint32_t ring = 0U; ring < 4U; ++ring) {
    *effect_log << "\tr" << ring << ':' << guest.s16(block + 3840U + ring * 2U)
         << ',' << call.bob_speed[ring] << ',' << call.applied_bob[ring]
         << ',' << guest.s16(block + 3856U + ring * 2U) << ','
         << call.spin_speed[ring] << ',' << call.applied_spin[ring] << ','
         << guest.s16(block + 3976U + ring * 2U) << ','
         << guest.f32(block + 3872U + ring * 4U) << ','
         << guest.f32(block + 3996U + ring * 4U) << ','
         << guest.f32(block + 4012U + ring * 4U) << ','
         << static_cast<unsigned>(guest.u8(block + 3904U + ring)) << ','
         << static_cast<unsigned>(guest.u8(block + 3908U + ring)) << ','
         << static_cast<unsigned>(guest.u8(block + 3920U + ring * 16U))
         << '/'
         << static_cast<unsigned>(guest.u8(block + 3921U + ring * 16U))
         << '/'
         << static_cast<unsigned>(guest.u8(block + 3922U + ring * 16U));
  }
  *effect_log << '\n';
}

// True when presenting now would only re-show the buffer presented last
// (see "Stale presents" in the header comment). Called before each present.
bool community_skip_stale_present(State &state,
                                  const std::uint32_t framebuffer) {
  if (!state.community_skip_stale_presents ||
      framebuffer != state.community_last_presented_vi ||
      state.graphics_color_image == framebuffer)
    return false;
  if (state.community_effect_stream.is_open())
    state.community_effect_stream
        << "PS\t" << state.community_effect_frame << '\t'
        << community_elapsed_ms(state) << '\t' << state.vi_retraces << '\t'
        << std::hex << framebuffer << '\t' << state.graphics_color_image
        << std::dec << '\n';
  return true;
}

void community_effect_present(State &state,
                              const std::uint32_t framebuffer) {
  state.community_last_presented_vi = framebuffer;
  if (!state.community_effect_stream.is_open())
    return;
  state.community_effect_stream
      << "P\t" << state.community_effect_frame << '\t'
      << community_elapsed_ms(state) << '\t' << state.vi_retraces << '\t'
      << std::hex << framebuffer << '\t' << state.graphics_color_image
      << std::dec << '\n';
}

// F9 recording (see the header comment). Polled once per presented frame,
// only while a window of this process has the focus.
void community_hotkey_poll(State &state) {
  DWORD owner = 0U;
  const HWND foreground = GetForegroundWindow();
  if (foreground != nullptr)
    (void)GetWindowThreadProcessId(foreground, &owner);
  const bool down = owner == GetCurrentProcessId() &&
                    (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
  const bool pressed = down && !state.community_hotkey_down;
  state.community_hotkey_down = down;
  if (!pressed)
    return;
  state.community_hotkey_on = !state.community_hotkey_on;
  if (state.community_hotkey_on) {
    ++state.community_hotkey_presses;
    // JFG_GEOMDUMP=N: also write the next N graphics tasks' geometry.
    if (const auto tasks = (std::min)(community_env_number("JFG_GEOMDUMP", 0U), 20U);
        tasks != 0U)
      jfg::request_geometry_dump(tasks);
    if (state.community_hotkey_every != 0U) {
      // A new frames folder per recording, up to 1800 frames.
      state.community_capture_dir.clear();
      state.community_capture_limit = state.community_capture_count + 1800U;
    }
  }
  if (state.community_effect_stream.is_open()) {
    state.community_effect_stream
        << "K\t" << state.community_effect_frame << '\t'
        << community_elapsed_ms(state) << '\t' << state.vi_retraces << '\t'
        << (state.community_hotkey_on ? 1 : 0) << '\t'
        << state.community_hotkey_presses << '\n';
    state.community_effect_stream.flush();
  }
  std::fprintf(stderr, "F9 recording %u %s\n", state.community_hotkey_presses,
               state.community_hotkey_on ? "started" : "stopped");
}

// Audio watch (2026-10-19). Called once per VI from the original-clock host
// pacing, after the audio device was polled, so it also runs while the game
// is loading and presents nothing. AU lines: game frame, ms, VI, underruns,
// overruns, queued audio in ms, host lateness against the VI deadline in ms.
// Written when an underrun is counted, when less than 30 ms of audio is
// queued, when the host is more than 20 ms late, and once a second.
void community_audio_watch(State &state, const std::int64_t late_us) {
  if (!state.community_effect_stream.is_open())
    return;
  const std::uint64_t underruns = state.host_audio.underruns();
  const std::uint64_t frequency = state.host_audio.frequency();
  const std::uint64_t queued_ms =
      frequency == 0U ? 0U
                      : static_cast<std::uint64_t>(
                            state.host_audio.current_queued_bytes()) *
                            1000U / (frequency * 4U);
  const bool counted = underruns != state.community_audio_underruns;
  if ((state.vi_frames % 60U) == 0U) {
    // PF line: work per second since the last PF line, in ms: task prepare
    // (snapshot + overlay materialisation), RT64 import, RT64 submit (DL +
    // fullSync), CPU writeback, present import; then tasks, ranged imports,
    // imported MiB, wall ms, native romCopy calls and native romCopy KiB.
    const auto now = std::chrono::steady_clock::now();
    const std::array<std::uint64_t, 10U> current{
        state.community_prof_prepare_us, state.community_prof_import_us,
        state.community_prof_rt64_us, state.community_prof_writeback_us,
        state.community_prof_present_us, state.community_prof_tasks,
        state.community_prof_ranged, state.community_prof_import_kb,
        state.community_romcopy_calls, state.community_romcopy_bytes};
    if (state.community_prof_time != std::chrono::steady_clock::time_point{}) {
      const auto &last = state.community_prof_last;
      state.community_effect_stream
          << "PF\t" << state.community_effect_frame << '\t'
          << community_elapsed_ms(state) << '\t' << state.vi_frames << '\t'
          << (current[0] - last[0]) / 1000U << '\t'
          << (current[1] - last[1]) / 1000U << '\t'
          << (current[2] - last[2]) / 1000U << '\t'
          << (current[3] - last[3]) / 1000U << '\t'
          << (current[4] - last[4]) / 1000U << '\t'
          << (current[5] - last[5]) << '\t' << (current[6] - last[6]) << '\t'
          << (current[7] - last[7]) / 1024U << '\t'
          << std::chrono::duration_cast<std::chrono::milliseconds>(
                 now - state.community_prof_time).count()
          << '\t' << (current[8] - last[8]) << '\t'
          << (current[9] - last[9]) / 1024U << '\n';
    }
    state.community_prof_last = current;
    state.community_prof_time = now;
  }
  if (!counted && queued_ms >= 30U && late_us <= 20000 &&
      (state.vi_frames % 60U) != 0U)
    return;
  state.community_audio_underruns = underruns;
  state.community_effect_stream
      << "AU\t" << state.community_effect_frame << '\t'
      << community_elapsed_ms(state) << '\t' << state.vi_frames << '\t'
      << underruns << '\t' << state.host_audio.overruns() << '\t'
      << queued_ms << '\t' << late_us / 1000 << '\n';
}

// Community native romCopy (2026-10-19, loading-transition audio hitch).
// romCopy(romOffset, ramAddress, numBytes) at 0x8004D640 reads cartridge ROM
// with osPiStartDma + osRecvMesg. On this host one call costs roughly
// 150-250 us (guarded-page MMIO exceptions for the PI/MI registers, the PI
// interrupt and about five guest thread switches, each two baton hand-offs),
// for a copy that usually moves a few dozen bytes. A level load makes about
// 2000 of these calls. The ~1100 tiny object reads at its end
// (objGetControlNo, texLoadTexture) run the guest at half speed for 0.1-0.3 s,
// and the audio queue runs dry.
// This copies the same bytes directly and drops the matching lines from the
// modelled D-cache (osInvalDCache). It skips the mainPreNMI poll: this port
// has no reset/pre-NMI source, so mainResetPressed always returns false.
// The DMA time is not modelled, so loads finish sooner in game time.
// Play mode only, after an exact check of the routine's code. Anything
// unexpected falls back to the original routine. JFG_NATIVE_ROMCOPY=0
// restores the original path.
bool community_native_romcopy(State &state, const recomp_context &context) {
  if (state.community_native_romcopy == 0U) {
    static constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 6U>
        kCode{{{0x4D640U, 0x27BDFFB0U},   // addiu sp, sp, -0x50
               {0x4D644U, 0xAFB30030U},   // sw s3, 0x30(sp)
               {0x4D678U, 0x0C0260ACU},   // jal osInvalDCache
               {0x4D6C8U, 0x0C025C20U},   // jal osPiStartDma
               {0x4D6D8U, 0x0C025A44U},   // jal osRecvMesg
               {0x4D6F0U, 0x0C011308U}}}; // jal mainPreNMI
    bool identical = state.rdram != nullptr && state.rom != nullptr;
    for (const auto &[offset, expected] : kCode) {
      std::uint32_t word = 0U;
      if (identical)
        std::memcpy(&word, state.rdram + offset, sizeof(word));
      identical = identical && word == expected;
    }
    const bool wanted = community_env_number("JFG_NATIVE_ROMCOPY", 1U) != 0U;
    state.community_native_romcopy =
        identical && wanted && state.play_mode ? 1U : 2U;
  }
  if (state.community_native_romcopy != 1U)
    return false;
  const auto rom_address = static_cast<std::uint32_t>(context.r4);
  const auto ram_address = static_cast<std::uint32_t>(context.r5);
  const auto length = static_cast<std::int32_t>(context.r6);
  if (length <= 0)
    return true; // osInvalDCache and the copy loop both do nothing.
  const std::uint64_t size = static_cast<std::uint32_t>(length);
  const std::uint32_t segment = ram_address & 0xE0000000U;
  const std::uint64_t physical = ram_address & 0x1FFFFFFFU;
  if (rom_address >= 0x10000000U ||
      (segment != 0x80000000U && segment != 0xA0000000U) ||
      physical > kRdramSize || size > kRdramSize - physical ||
      rom_address > state.rom_size || size > state.rom_size - rom_address)
    return false;
  std::uint8_t *const destination = state.rdram;
  const std::uint8_t *const source = state.rom + rom_address;
  std::uint64_t index = 0U;
  if ((physical & 3U) == 0U) {
    for (; index + 4U <= size; index += 4U) {
      const std::uint32_t word = (std::uint32_t{source[index]} << 24U) |
                                 (std::uint32_t{source[index + 1U]} << 16U) |
                                 (std::uint32_t{source[index + 2U]} << 8U) |
                                 std::uint32_t{source[index + 3U]};
      std::memcpy(destination + physical + index, &word, sizeof(word));
    }
  }
  for (; index < size; ++index)
    destination[(physical + index) ^ 3U] = source[index];
  if (state.data_cache != nullptr) {
    // Same effect on the model as osInvalDCache: whole cache from 8 KiB up,
    // otherwise the lines of the range (16-byte lines, 512 entries).
    if (size >= 8192U) {
      for (std::size_t line = 0U; line < 512U; ++line)
        state.data_cache[line].valid = state.data_cache[line].dirty = 0U;
    } else {
      for (std::uint64_t line = physical & ~std::uint64_t{15U};
           line < physical + size; line += 16U) {
        auto &entry = state.data_cache[(line >> 4U) & 511U];
        if (entry.valid != 0U && entry.tag == (line & ~std::uint64_t{0xFFFU}))
          entry.valid = entry.dirty = 0U;
      }
    }
  }
  ++state.community_romcopy_calls;
  state.community_romcopy_bytes += size;
  return true;
}

// Frame capture around the shrink beam (see the header comment).
bool community_capture_wanted(const State &state) {
  if (state.community_hotkey_on && state.community_hotkey_every != 0U &&
      state.community_capture_count < state.community_capture_limit)
    return (state.presented_frames % state.community_hotkey_every) == 0U;
  if (state.community_capture_limit == 0U ||
      state.community_capture_count >= state.community_capture_limit)
    return false;
  if (state.community_capture_left != 0U)
    return true;
  if (state.community_capture_every != 0U) {
    const std::int64_t ms = community_elapsed_ms(state);
    if (ms >= state.community_capture_from_ms &&
        ms <= state.community_capture_to_ms)
      return (state.presented_frames % state.community_capture_every) == 0U;
  }
  const bool beams_exist =
      state.community_beam_seen_frame != 0U &&
      state.community_effect_frame <= state.community_beam_seen_frame + 1U;
  return beams_exist && (state.presented_frames % 16U) == 0U;
}

// `captured` is the flag present() was called with; community_capture_wanted
// decided it before presented_frames was advanced.
void community_capture_after_present(State &state, const bool captured) {
  if (state.community_capture_left != 0U)
    --state.community_capture_left;
  if (!captured || state.rt64_shell == nullptr ||
      state.community_capture_limit == 0U ||
      state.community_capture_count >= state.community_capture_limit)
    return;
  const jfg::Rt64FrameView frame = state.rt64_shell->last_presented_frame();
  if (!frame.valid())
    return;
  if (state.community_capture_dir.empty()) {
    std::array<char, MAX_PATH> base{};
    const DWORD length = GetEnvironmentVariableA(
        "LOCALAPPDATA", base.data(), static_cast<DWORD>(base.size()));
    if (length == 0U || length >= base.size()) {
      state.community_capture_limit = 0U;
      return;
    }
    const auto stamp = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
    state.community_capture_dir = std::filesystem::path(base.data()) /
                                  "JFGRecomp" / "diag" / "frames" /
                                  ("run-" + std::to_string(stamp));
    std::error_code error;
    std::filesystem::create_directories(state.community_capture_dir, error);
    if (error) {
      state.community_capture_limit = 0U;
      return;
    }
  }
  const std::size_t factor = (std::max)(std::size_t{1U},
                                        (frame.width + 639U) / 640U);
  const std::size_t out_width = frame.width / factor;
  const std::size_t out_height = frame.height / factor;
  if (out_width == 0U || out_height == 0U)
    return;
  std::vector<char> pixels(out_width * out_height * 3U);
  const std::size_t area = factor * factor;
  for (std::size_t y = 0U; y < out_height; ++y) {
    for (std::size_t x = 0U; x < out_width; ++x) {
      std::size_t blue = 0U, green = 0U, red = 0U;
      for (std::size_t dy = 0U; dy < factor; ++dy) {
        const std::size_t row = (y * factor + dy) * frame.row_pitch_bytes;
        for (std::size_t dx = 0U; dx < factor; ++dx) {
          const std::size_t pixel = row + (x * factor + dx) * 4U;
          blue += std::to_integer<std::size_t>(frame.bgra8[pixel]);
          green += std::to_integer<std::size_t>(frame.bgra8[pixel + 1U]);
          red += std::to_integer<std::size_t>(frame.bgra8[pixel + 2U]);
        }
      }
      char *out = pixels.data() + (y * out_width + x) * 3U;
      out[0] = static_cast<char>(static_cast<unsigned char>(red / area));
      out[1] = static_cast<char>(static_cast<unsigned char>(green / area));
      out[2] = static_cast<char>(static_cast<unsigned char>(blue / area));
    }
  }
  const std::string name = "f" + std::to_string(state.community_effect_frame) +
                           "_p" + std::to_string(state.presented_frames) +
                           ".ppm";
  std::ofstream file(state.community_capture_dir / name, std::ios::binary);
  if (!file)
    return;
  file << "P6\n" << out_width << ' ' << out_height << "\n255\n";
  file.write(pixels.data(), static_cast<std::streamsize>(pixels.size()));
  if (!file)
    return;
  ++state.community_capture_count;
  if (state.community_effect_stream.is_open())
    state.community_effect_stream
        << "C\t" << state.community_effect_frame << '\t'
        << community_elapsed_ms(state) << '\t' << state.presented_frames
        << '\t' << out_width << 'x' << out_height << '\t' << name << '\n';
}
#endif

void present_completed_video(State &state, const std::uint32_t target) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (state.play_mode || state.realtime_replay) {
    service_live_window(state);
    if (state.exit_requested)
      play_exit(state);
  }
  if (!service_host_audio(state, true))
    fail_closed_dispatch(state, "audio", "host-device", target);
  const std::uint32_t selected_vi_framebuffer =
      (state.vi_next_framebuffer != 0U
           ? state.vi_next_framebuffer
           : state.vi_current_framebuffer) &
      0x00FFFFFFU;
  // The task completed during this scheduler slice targets JFG's
  // off-screen buffer. Present the previously completed workload before
  // submitting that new task, so the normal double-buffer pipeline needs
  // one RT64 submission per displayed frame instead of two.
  if (state.rt64_shell != nullptr && state.graphics_frame_ready &&
      selected_vi_framebuffer != 0U) {
    // Effects can submit more than eight off-screen targets between VI
    // presentations. Ask RT64 whether the selected color target exists instead
    // of letting an unrelated snapshot-cache eviction suppress the frame.
    bool presentation_ready =
        state.rt64_shell->has_color_framebuffer(selected_vi_framebuffer);
    if (!presentation_ready) {
      state.gameplay_trace.event("present-skip", {state.vi_retraces,
          state.controller_samples, state.gameplay_trace_updates,
          selected_vi_framebuffer, state.graphics_color_image});
    }
    if (presentation_ready &&
        community_skip_stale_present(state, selected_vi_framebuffer))
      presentation_ready = false;
    if (presentation_ready)
      community_hotkey_poll(state);
    if (presentation_ready) {
      state.rt64_vi.current_line =
          state.rt64_vi.current_line == 0U ? 2U : 0U;
      if (!apply_live_vi_field(state, selected_vi_framebuffer))
        fail_closed_dispatch(state, "renderer", "vi-field", target);
      // CPU overlays can modify the completed framebuffer between DP and VI.
      // Refresh the displayed pixels before RT64 tests its framebuffer hash.
      // Preserve the upper RDRAM scratch space used by captured display lists.
      std::memcpy(state.rt64_rdram.data(), state.rdram, kRdramSize);
      // Community ranged import: the present needs only the guest half.
      // The full import used to replace RT64's overlay shadows with this
      // buffer's zero upper half every frame.
      const auto community_present_start = std::chrono::steady_clock::now();
      if (state.community_ranged_import && state.rt64_shell->ranged_import_available()) {
        static const jfg::Rt64RdramRange guest_half[1]{{0U, kRdramSize}};
        state.last_rt64_error = state.rt64_shell->replace_rdram_snapshot_ranges(
            state.rt64_rdram, jfg::Rt64MemoryLayout::host_word_swapped, guest_half);
      } else {
        state.last_rt64_error = state.rt64_shell->replace_rdram_snapshot(
            state.rt64_rdram, jfg::Rt64MemoryLayout::host_word_swapped);
        state.community_rt64_upper_known = false;
      }
      state.community_prof_present_us += static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::steady_clock::now() - community_present_start).count());
      if (state.last_rt64_error != jfg::Rt64ShellError::none)
        fail_closed_dispatch(state, "renderer", "present-cpu-import", target);
      write_private_progress(state, "graphics-present-begin");
      const auto present_start = std::chrono::steady_clock::now();
      const bool capture_frame = (!state.frame_capture_path.empty() &&
          (state.play_mode
               ? (state.presented_frames + 1U) % 60U == 0U
               : state.vi_retraces + 2U >= state.retrace_target)) ||
          community_capture_wanted(state);
      state.last_rt64_error =
          state.rt64_shell->present(capture_frame);
      const auto present_end = std::chrono::steady_clock::now();
      const auto present_us = static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::microseconds>(
              present_end - present_start)
              .count());
      state.graphics_present_microseconds += present_us;
      state.graphics_present_max_us =
          (std::max)(state.graphics_present_max_us, present_us);
      std::uint64_t present_interval_us = 0U;
      if (state.present_time_initialized) {
        present_interval_us = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                present_end - state.present_time)
                .count());
        state.present_interval_max_us =
            (std::max)(state.present_interval_max_us,
                       present_interval_us);
      }
      state.gameplay_trace.event("present", {state.vi_retraces, state.controller_samples,
          state.gameplay_trace_updates, present_us, present_interval_us, selected_vi_framebuffer});
      state.present_time = present_end;
      state.present_time_initialized = true;
      if (state.timing_trace) state.timing_trace << "present-qpc\t"
          << state.vi_frames << '\t' << state.vi_retraces << '\t'
          << std::chrono::duration_cast<std::chrono::microseconds>(
                 present_end.time_since_epoch()).count() << '\n';
      if (state.timing_trace) {
        state.timing_trace << "present\t" << state.vi_frames << '\t'
                           << state.vi_retraces << '\t' << present_us
                           << '\t' << present_interval_us << '\t'
                           << std::hex << selected_vi_framebuffer
                           << std::dec << '\n';
      }
      write_private_progress(state, "graphics-present-end");
      if (state.last_rt64_error != jfg::Rt64ShellError::none) {
        std::array<char, 96U> operation{};
        (void)std::snprintf(
            operation.data(), operation.size(),
            "present-%u-%08x-%08x-%08x",
            static_cast<unsigned>(state.last_rt64_error),
            state.rt64_vi.origin, state.graphics_color_image,
            state.rt64_vi.width);
        fail_closed_dispatch(state, "renderer", operation.data(),
                             target);
      }
      ++state.presented_frames;
      community_effect_present(state, selected_vi_framebuffer);
      community_capture_after_present(state, capture_frame);
      state.graphics_frame_ready = false;
    }
  }
#else
  (void)state; (void)target;
#endif
}

void latch_next_vi_framebuffer(State &state, const std::uint32_t target) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (state.vi_next_framebuffer != 0U) {
    state.vi_current_framebuffer = state.vi_next_framebuffer;
    if (!apply_live_vi_field(state, state.vi_current_framebuffer))
      fail_closed_dispatch(state, "renderer", "vi-field", target);
  }
#else
  (void)state; (void)target;
#endif
}

void deliver_legacy_si_completion(State &state, const std::uint32_t target) {
  if (!state.si_count_probe && state.pending_si_completions != 0U) {
    const auto si = state.events.find(5U);
    if (si == state.events.end()) {
      fail_closed_dispatch(state, "controller", "si-delivery", target);
    }
    const std::int32_t delivery = state.scheduler->send(
        si->second.first, si->second.second, false);
    if (delivery != hle::kOsSuccess && delivery != hle::kOsFull)
      fail_closed_dispatch(state, "controller", "si-delivery", target);
    --state.pending_si_completions;
    trace_si_timing(state, "deliver", si->second.first, 0U, delivery);
    state.journal.append(NativeEventKind::kController, 4U,
                         si->second.first,
                         delivery == hle::kOsSuccess ? 1U : 0U);
  }
}

void deliver_vi_interrupt(State &state, const std::uint32_t target) {
  if (state.vi_frames % state.vi_retrace_interval == 0U) {
    if (state.scheduler->send(state.vi_internal_queue,
                              state.vi_internal_message, false) !=
        hle::kOsSuccess) {
      fail_closed_dispatch(state, "vi", "interrupt-delivery", target);
    }
    ++state.vi_interrupts;
    state.journal.append(NativeEventKind::kViInterruptRaised,
                         state.vi_internal_queue,
                         state.vi_internal_message,
                         static_cast<std::uint32_t>(
                             state.vi_interrupts));
  }
}

// Preserve the cooperative profile's exact order as a control. Device
// completion is explicitly separate from VI service; this wrapper still
// schedules them together and does NOT claim execution-driven timing.
void service_vi_frame(State &state, const std::uint32_t target) {
  advance_legacy_vi_clock(state, target);
  state.empty_receive_polls.clear();
  state.journal.append(NativeEventKind::kViFrame,
                       static_cast<std::uint32_t>(state.vi_frames));
  present_completed_video(state, target);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  complete_pending_live_graphics_tasks(state, target);
#endif
  latch_next_vi_framebuffer(state, target);
  deliver_legacy_si_completion(state, target);
  deliver_vi_interrupt(state, target);
}

// Overlay 32's mrhintsControl entry (text offset 0x2E8) doubles as the King
// Bear hint callback. Three dispatch paths reach it (direct, generated
// overlay lookup, runlink-resolved lookup) and must record identical route
// diagnostics.
[[maybe_unused]] void note_hints_control_call(State &state, const std::uint32_t target,
                             const recomp_context &context) {
  ++state.phase9_hints_control_calls;
  state.phase9_hints_actor = static_cast<std::uint32_t>(context.r4);
  ++state.phase9_king_bear_control_calls;
  state.phase9_king_bear_actor = static_cast<std::uint32_t>(context.r4);
  state.phase9_king_bear_callback = target;
}

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
class FunctionProfileScope {
  using Profile=jfg::boot::FunctionProfile;
  struct ThreadData {
    Profile profile;
    std::uint64_t start_vi=0,start_poll=0;
    std::uint32_t thread=0;
    ULONG64 process_cycles=0;
    bool process_valid=false;
    std::uint64_t handoffs=0;
    BatonExecutor::Timing transport{};
  };
  State* state_=nullptr;
  ThreadData* data_=nullptr;
  static Profile::Sample sample(State& state) {
    ULONG64 cycles=0;
    const bool valid=QueryThreadCycleTime(GetCurrentThread(),&cycles)!=0;
    const auto wall=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
    return {wall,cycles,state.profile_host_wait_us,state.observed_guest_instructions,
      GetCurrentThreadId(),valid};
  }
public:
  FunctionProfileScope(State& state,std::uint32_t target) {
    if(!state.diagnostic_function_profile)return;
    static thread_local ThreadData local;
    const bool root=target==0x80044FACU && !local.profile.active();
    if(root) {
      if(state.vi_retraces<state.diagnostic_profile_first_vi ||
         state.vi_retraces>=state.diagnostic_profile_last_vi)return;
      local.profile.reset();local.start_vi=state.vi_retraces;
      local.start_poll=state.controller_samples;local.thread=GetCurrentThreadId();
      local.process_valid=QueryProcessCycleTime(GetCurrentProcess(),&local.process_cycles)!=0;
      local.handoffs=state.guest_transport?state.guest_transport->handoffs():0U;
      local.transport=state.guest_transport?state.guest_transport->timing():BatonExecutor::Timing{};
    } else if(!local.profile.active())return;
    const bool stage=target==0x80044FACU || target==0x80008FF0U ||
      target==0x00310600U || target==0x09B02FD0U || target==0x8003D21CU ||
      target==0x800121C0U || target==0x8004DA10U || target==0x8004DCB0U;
    if(state.diagnostic_function_profile==1U && !stage)return;
    state_=&state;data_=&local;local.profile.enter(target,sample(state));
  }
  ~FunctionProfileScope() {
    if(!state_)return;
    if(!data_->profile.leave(sample(*state_)))return;
    const auto emit_started=sample(*state_);
    ULONG64 process_cycles=0;
    const bool process_valid=data_->process_valid &&
      QueryProcessCycleTime(GetCurrentProcess(),&process_cycles) &&
      process_cycles>=data_->process_cycles;
    const auto handoffs=state_->guest_transport?state_->guest_transport->handoffs():0U;
    const auto transport=state_->guest_transport?state_->guest_transport->timing():BatonExecutor::Timing{};
    auto& trace=state_->gameplay_trace;
    trace.event("function-profile-host",{state_->vi_retraces,state_->controller_samples,
      data_->start_vi,data_->start_poll,data_->thread,process_valid,
      process_valid?process_cycles-data_->process_cycles:0U,
      handoffs-data_->handoffs,transport.wakes-data_->transport.wakes,
      transport.wake_ns-data_->transport.wake_ns});
    std::uint64_t used=0;
    for(const auto& row:data_->profile.rows()) {
      if(!row.calls)continue;
      ++used;
      trace.event("function-profile",{state_->vi_retraces,state_->controller_samples,
        data_->start_vi,data_->start_poll,data_->thread,row.target,row.parent,row.calls,
        row.wall,row.self_wall,row.cycles,row.self_cycles,row.wait,row.instructions,
        row.max_wall,row.max_cycles,row.valid});
    }
    trace.event("function-profile-end",{state_->vi_retraces,state_->controller_samples,
      data_->start_vi,data_->start_poll,data_->thread,state_->diagnostic_function_profile,
      data_->profile.valid(),data_->profile.calls(),used,data_->profile.overflow()});
    const auto emit_finished=sample(*state_);
    const bool emit_valid=emit_started.valid && emit_finished.valid &&
      emit_started.thread==emit_finished.thread && emit_finished.cycles>=emit_started.cycles &&
      emit_finished.wall>=emit_started.wall;
    trace.event("function-profile-emission",{state_->vi_retraces,state_->controller_samples,
      data_->start_vi,data_->start_poll,data_->thread,emit_valid,
      emit_valid?emit_finished.wall-emit_started.wall:0U,
      emit_valid?emit_finished.cycles-emit_started.cycles:0U});
  }
};
#endif

int dispatch(void *opaque, std::int32_t address, std::uint8_t *rdram,
             recomp_context *context) {
  auto &state = *static_cast<State *>(opaque);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  const auto dispatch_prefix_started = state.diagnostic_cpu_costs ? std::chrono::steady_clock::now()
      : std::chrono::steady_clock::time_point{};
#endif
  const auto target = static_cast<std::uint32_t>(address);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  FunctionProfileScope function_profile_scope(state,target);
#endif
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  for (std::size_t watch = 0U;
       !state.progress_path.empty() &&
       watch < state.matrix_watch_addresses.size(); ++watch) {
    const std::uint32_t watched = state.matrix_watch_addresses[watch];
    if (watched == 0U)
      continue;
    // These observations use the native-word layout. Validate the same
    // aligned RDRAM range before loading directly, avoiding an out-of-line
    // GuestMemory call for each of the eight words on every dispatch.
    if ((watched & 3U) != 0U || watched < kKseg0 ||
        watched - kKseg0 > kRdramSize - sizeof(std::uint32_t))
      continue;
    std::uint32_t current = 0U;
    std::memcpy(&current, rdram + (watched - kKseg0), sizeof(current));
    if (current == state.matrix_watch_words[watch])
      continue;
    const std::size_t slot = state.recent_matrix_watch_position %
        state.recent_matrix_watch_addresses.size();
    state.recent_matrix_watch_addresses[slot] = watched;
    state.recent_matrix_watch_before[slot] = state.matrix_watch_words[watch];
    state.recent_matrix_watch_after[slot] = current;
    state.recent_matrix_watch_targets[slot] =
        state.recent_dispatch_position == 0U
            ? 0U
            : state.recent_dispatches[(state.recent_dispatch_position - 1U) %
                                      state.recent_dispatches.size()];
    state.recent_matrix_watch_callers[slot] =
        state.recent_dispatch_position < 2U
            ? 0U
            : state.recent_dispatches[(state.recent_dispatch_position - 2U) %
                                      state.recent_dispatches.size()];
    state.recent_matrix_watch_current_targets[slot] = target;
    state.recent_matrix_watch_return_addresses[slot] =
        static_cast<std::uint32_t>(context->r31);
    if (watched == 0x800FAF7CU) {
      state.poison_matrix_watch = {
          state.matrix_watch_words[watch], current,
          state.recent_matrix_watch_targets[slot],
          state.recent_matrix_watch_callers[slot], target,
          state.recent_matrix_watch_return_addresses[slot]};
      const std::size_t history_count =
          (std::min)(state.recent_dispatch_position,
                     state.poison_dispatch_history.size());
      state.poison_dispatch_history.fill(0U);
      for (std::size_t index = 0U; index < history_count; ++index) {
        const std::size_t distance = history_count - index;
        state.poison_dispatch_history[index] =
            state.recent_dispatches[(state.recent_dispatch_position - distance) %
                                    state.recent_dispatches.size()];
      }
    }
    ++state.recent_matrix_watch_position;
    state.matrix_watch_words[watch] = current;
  }
#endif
  if (target == 0xFFFFFFFEU) {
    if (state.scheduler == nullptr || !state.scheduler_running ||
        state.scheduler->current_thread_id() < 0) {
      fail_closed_dispatch(state, "scheduler", "pause-outside-thread",
                           target);
    }
    state.journal.append(NativeEventKind::kThreadPaused, target,
                         static_cast<std::uint32_t>(
                             state.scheduler->current_thread_id()));
    state.scheduler->pause_current();
    fail_closed_dispatch(state, "scheduler", "pause-resumed", target);
  }
  if (state.mmio_trace != nullptr &&
      state.mmio_trace->unsupported_accesses != 0U)
    fail_closed_dispatch(state, "mmio", "unsupported-register", target);
  ++state.dispatch_calls;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  // Read-only, explicitly requested private gameplay diagnostics. Keep the
  // original weapon handler and all ammunition/charge logic untouched.
  if(target==0x80037550U && state.gameplay_trace) {
    jfg::mod::Memory ram({rdram,kRdramSize});
    const auto actor=static_cast<std::uint32_t>(context->r4);
    const auto control=static_cast<std::uint32_t>(context->r5);
    const auto game=ram.u32(0x800FD7D4U);
    if(ram.u32(target)==0x444EF800U && ram.u32(target+4U)==0x27BDFF70U &&
       ram.u32(target+8U)==0x35C10003U && ram.valid(actor,0xA4U) &&
       ram.valid(control,0x5C4U) && ram.u32(actor+0x68U)==control &&
       ram.valid(game,0x538U) && jfg::mod::gameplay_active(ram,actor)) {
      const auto character=ram.u8(control+1U)&3U;
      const auto port=ram.u8(control);
      if(character<3U && port<4U) {
        const auto saved=game+0x15CU+character*0x76U;
        const auto weapon=ram.u8(saved+0x70U);
        if(weapon<16U)state.gameplay_trace.event("pc-weapon-state",{
          state.vi_retraces,actor,port,character,weapon,
          ram.u16(saved+0x14U+weapon*2U),ram.u16(saved+0x34U+weapon*2U),
          ram.u8(control+0x568U),ram.u32(game+0x474U+weapon*4U),
          ram.u16(control+0x53AU),ram.u16(control+0x53EU),
          ram.u32(0x800A317CU+port*4U),ram.u32(0x800FB078U)});
      }
    }
  }
  // Apply proximity fading only to the opacity returned for the current
  // modern-camera player. Calling the guest first preserves its death/aim fades;
  // no actor flags or persistent control fields are changed.
  if(target==0x80015E3CU && state.modern_controls_active && !state.pc_input_blocked &&
     (state.play_mode || state.input_replay_loaded) &&
     (state.input_record_path.empty() || state.pc_record_extended)) {
    jfg::mod::Memory ram({rdram,kRdramSize});
    const auto actor=static_cast<std::uint32_t>(context->r4);
    const auto control=ram.valid(actor,0xA4U)?ram.u32(actor+0x68U):0U;
    if(ram.valid(control,0x5C4U) && ram.u32(target)==0x27BDFFE0U &&
       ram.u32(target+4U)==0xAFBF0014U && ram.u32(target+8U)==0x8C850068U &&
       jfg::mod::gameplay_active(ram,actor) && ram.u8(0x800FD7BDU)==0 &&
       !jfg::mod::scripted_camera_active(ram,actor)) {
      const auto port=ram.u8(control);
      if(port<4 && state.controller_ports.samples[port].connected &&
         state.pc_orbits[port].active && state.pc_orbits[port].actor==actor &&
         state.vi_retraces-state.pc_orbits[port].retrace<=15U &&
         jfg::pc_normal_camera_owns_look((state.controller_ports.samples[port].buttons&0x10)!=0,ram.u8(control+0x568U))) {
        const auto mode=state.pc_frames[port].pc_mode;
        const auto camera=jfg::mod::control_camera(ram);
        auto* original=jfg_generated_lookup_function(static_cast<std::int32_t>(target));
        if((mode==3 || mode==4 || mode==5) && camera.known && original) {
          const jfg::PcOrbitPoint body{ram.f32(actor+0xCU),ram.f32(actor+0x10U),ram.f32(actor+0x14U)};
          original(rdram,context);
          const int before=static_cast<std::int32_t>(context->r2);
          const int after=jfg::pc_camera_player_alpha(body,
              {camera.position.x,camera.position.y,camera.position.z},before);
          context->r2=static_cast<gpr>(after);
          if(after!=before)state.gameplay_trace.event("pc-camera-fade",{
            state.vi_retraces,actor,static_cast<std::uint32_t>(before),static_cast<std::uint32_t>(after)});
          return 1;
        }
      }
    }
  }
  // The stock camera moves the player to maintain 32 horizontal units of
  // separation. At an upward view limit its floor collision can repeatedly
  // push an idle modern player forward. Keep camera collision/clearance and
  // ordinary player physics; suppress only this qualified camera-body push.
  if(target==0x80009A04U && state.pc_camera_actor!=0 &&
     static_cast<std::uint32_t>(context->r4)==state.pc_camera_actor &&
     static_cast<std::uint32_t>(context->r31)==0x8002EB5CU) {
    jfg::mod::Memory ram({rdram,kRdramSize});
    const auto port=ram.u8(state.pc_camera_control);
    if(ram.u32(0x8002EB54U)==0x0C002681U &&
       modern_shoulder_current(state,port,state.pc_camera_actor)) {
      state.gameplay_trace.event("pc-camera-push-suppressed",{state.vi_retraces,
        state.pc_camera_actor,static_cast<std::uint32_t>(context->r5),
        static_cast<std::uint32_t>(context->r6),static_cast<std::uint32_t>(context->r7)});
      return 1;
    }
  }
  // The normal camera has computed its desired point, but has not yet run
  // scenery constraints. Feed the orbit into that existing solver, never over
  // its final position. Qualify both the owning routine and this call site.
  if(target==0x800446E4U && state.pc_camera_actor!=0 &&
     static_cast<std::uint32_t>(context->r29)==state.pc_camera_stack) {
    jfg::mod::Memory ram({rdram,kRdramSize});
    const auto stack=static_cast<std::uint32_t>(context->r29);
    const auto actor=state.pc_camera_actor,control=state.pc_camera_control;
    const auto port=ram.u8(control);
    if(port<4 && ram.valid(stack,0xF4U) && ram.u32(stack+0xF0U)==actor && ram.u32(0x8002E158U)==0x0C0111B9U) {
      auto &orbit=state.pc_orbits[port];
      const auto camera=ram.u32(0x800F6DC0U);
      const float px=ram.f32(actor+0xCU),py=ram.f32(actor+0x10U)+40.0F,pz=ram.f32(actor+0x14U);
      const float dx=ram.f32(stack+0x8CU)-px,dy=ram.f32(stack+0x90U)-py,dz=ram.f32(stack+0x94U)-pz;
      const float native_radius=std::sqrt(dx*dx+dy*dy+dz*dz);
      const float radius=modern_shoulder_current(state,port,actor)?jfg::kPcShoulderDistance:native_radius;
      if(ram.valid(camera,0x4CU) && std::isfinite(px) && std::isfinite(py) && std::isfinite(pz) &&
         std::isfinite(radius) && radius>=40.0F && radius<=1000.0F) {
        if(!orbit.active || orbit.actor!=actor || state.vi_retraces-orbit.retrace>15U) {
          orbit.yaw=ram.s16(camera);
          orbit.pitch=static_cast<int>(std::lround(std::atan2(dy,std::sqrt(dx*dx+dz*dz))*65536.0F/6.283185307179586F));
        }
        orbit.active=true;orbit.actor=actor;orbit.retrace=state.vi_retraces;
        if(modern_shoulder_current(state,port,actor)) {
          orbit.yaw=jfg::pc_shoulder_camera_yaw(state.pc_shoulders[port].yaw);
          orbit.pitch=state.pc_shoulders[port].pitch;
        } else {
          orbit.yaw=jfg::pc_wrap_angle(orbit.yaw+state.pc_camera_look.x);
          orbit.pitch=std::clamp(orbit.pitch-state.pc_camera_look.y,-8192,12288);
        }
        const auto point=jfg::pc_orbit_point(orbit.yaw,orbit.pitch,radius);
        hle::GuestMemory memory({rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
        (void)memory.write_u32(stack+0x8CU,std::bit_cast<std::uint32_t>(px+point.x));
        (void)memory.write_u32(stack+0x90U,std::bit_cast<std::uint32_t>(py+point.y));
        (void)memory.write_u32(stack+0x94U,std::bit_cast<std::uint32_t>(pz+point.z));
        state.gameplay_trace.event("pc-orbit",{state.vi_retraces,state.controller_samples,port,
          static_cast<std::uint16_t>(orbit.yaw),static_cast<std::uint16_t>(orbit.pitch),
          std::bit_cast<std::uint32_t>(radius),static_cast<std::uint32_t>(state.pc_camera_look.x),
          static_cast<std::uint32_t>(state.pc_camera_look.y)});
      }
    }
  }
  // The scenery solver can leave its point exactly on a wall. Keep a small
  // near-plane clearance toward its focus, only when collision moved the
  // requested orbit point. Preserve the solver's unblocked result verbatim.
  if(target==0x8002CC70U && state.pc_camera_actor!=0 &&
     static_cast<std::uint32_t>(context->r29)==state.pc_camera_stack) {
    jfg::mod::Memory ram({rdram,kRdramSize});
    const auto focus=static_cast<std::uint32_t>(context->r4),point=static_cast<std::uint32_t>(context->r5);
    const auto stack=state.pc_camera_stack;
    if(focus==stack+0x98U && point==stack+0x8CU && ram.valid(stack,0xA4U) &&
       ram.u32(target)==0x27BDFF60U && ram.u32(target+4U)==0xAFBF002CU) {
      const jfg::PcOrbitPoint from{ram.f32(focus),ram.f32(focus+4U),ram.f32(focus+8U)};
      const jfg::PcOrbitPoint wanted{ram.f32(point),ram.f32(point+4U),ram.f32(point+8U)};
      auto* original=jfg_generated_lookup_function(static_cast<std::int32_t>(target));
      if(original) {
        const auto entry=*context;
        const auto camera_started = state.gameplay_trace ? std::chrono::steady_clock::now()
                                                        : std::chrono::steady_clock::time_point{};
        const auto camera_count = state.cpu_count;
        original(rdram,context);
        const auto returned=*context;
        const auto camera_body_started = state.gameplay_trace ? std::chrono::steady_clock::now()
                                                             : std::chrono::steady_clock::time_point{};
        const auto camera_body_count = state.cpu_count;
        const auto camera_body_wait = state.profile_host_wait_us;
        unsigned camera_queries = 0U;
        const jfg::PcOrbitPoint resolved{ram.f32(point),ram.f32(point+4U),ram.f32(point+8U)};
        auto clear=jfg::pc_camera_clearance(from,wanted,resolved);
        hle::GuestMemory memory({rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
        const auto put=[&](std::uint32_t address,jfg::PcOrbitPoint p) {
          (void)memory.write_u32(address,std::bit_cast<std::uint32_t>(p.x));
          (void)memory.write_u32(address+4U,std::bit_cast<std::uint32_t>(p.y));
          (void)memory.write_u32(address+8U,std::bit_cast<std::uint32_t>(p.z));
        };
        const auto port=ram.u8(state.pc_camera_control);
        const bool shoulder=modern_shoulder_current(state,port,state.pc_camera_actor);
        if(shoulder) {
          auto& boom=state.pc_shoulders[port].camera_boom;
          std::optional<jfg::PcCameraBroadphase> geometry;
          const auto validate_height=[&](jfg::PcOrbitPoint candidate,jfg::PcOrbitPoint result) {
            if(std::abs(result.x-candidate.x)<=0.01F &&
               std::abs(result.z-candidate.z)<=0.01F &&
               std::abs(result.y-candidate.y)>0.01F) {
              if(!geometry)geometry.emplace(ram);
              // The guest height query can extrapolate a steep rim's plane
              // beyond its finite triangle. Ignore only a height-only result
              // when the complete scene and registered model bounds certify
              // both the view segment and the 16-unit camera volume as clear.
              if(!geometry->volume_possible(candidate,16.0F) &&
                 !geometry->possible(from,candidate)) {
                state.gameplay_trace.event("pc-camera-height-clear",{state.vi_retraces,
                  std::bit_cast<std::uint32_t>(candidate.x),std::bit_cast<std::uint32_t>(candidate.y),
                  std::bit_cast<std::uint32_t>(candidate.z),std::bit_cast<std::uint32_t>(result.y)});
                return candidate;
              }
            }
            return result;
          };
          clear=boom.solve(from,wanted,validate_height(wanted,resolved),state.pc_camera_ticks/60.0F,
            [&](jfg::PcOrbitPoint candidate) {
              put(point,candidate);*context=entry;original(rdram,context);
              return validate_height(candidate,
                {ram.f32(point),ram.f32(point+4U),ram.f32(point+8U)});
            });
          if(boom.fallback)clear=jfg::pc_camera_clearance(from,wanted,clear);
          camera_queries=boom.queries;
          state.gameplay_trace.event("pc-camera-boom",{state.vi_retraces,
            std::bit_cast<std::uint32_t>(boom.distance),boom.queries,
            boom.constrained?1U:0U,boom.fallback?1U:0U,
            std::bit_cast<std::uint32_t>(state.pc_camera_ticks),
            std::bit_cast<std::uint32_t>(from.x),std::bit_cast<std::uint32_t>(from.y),std::bit_cast<std::uint32_t>(from.z)});
          *context=returned;
        }
        // The guest helper tests the head-height focus and can then raise an
        // inward contact back to the requested camera height (0x8002CF54).
        // Query trackNearestIntersection directly for body visibility. It
        // retains the game's own scenery and moving-model collision rules.
        constexpr std::uint32_t ray_target=0x80018808U;
        auto* ray=jfg_generated_lookup_function(static_cast<std::int32_t>(ray_target));
        if(!shoulder && ray && stack>=0x80000400U && ram.valid(stack-0x400U,0x400U) &&
           ram.u32(ray_target)==0x27BDFD58U && ram.u32(ray_target+4U)==0x3C03800AU) {
          const std::uint32_t scratch=stack-0x80U;
          // A torso ray can pass above a narrow tree edge while the legs stay
          // hidden. Start just above the actor's feet, in world coordinates.
          const jfg::PcOrbitPoint body{ram.f32(state.pc_camera_actor+0xCU),
            ram.f32(state.pc_camera_actor+0x10U)+8.0F,ram.f32(state.pc_camera_actor+0x14U)};
          const jfg::PcCameraBroadphase broadphase(ram);
          for(unsigned pass=0;pass<2;++pass) {
            auto candidate=clear;
            jfg::PcOrbitPoint nearest_hit{};
            const auto distance2=[&](jfg::PcOrbitPoint p) {
              const float x=p.x-from.x,y=p.y-from.y,z=p.z-from.z;return x*x+y*y+z*z;
            };
            for(const auto probe:jfg::pc_camera_body_probes(body,clear)) {
              if(!broadphase.possible(probe,clear))continue;
              put(scratch+0x20U,probe);put(scratch+0x30U,clear);
              (void)memory.write_u32(scratch+0x10U,0U);
              (void)memory.write_u32(scratch+0x14U,0U);
              // Recompiled memory operands require sign-extended addresses;
              // timing/dispatch hooks must retain the live thread context.
              const auto guest_address=[](std::uint32_t value){return static_cast<gpr>(static_cast<std::int32_t>(value));};
              *context=entry;context->r29=guest_address(scratch);context->r4=0;
              context->r5=guest_address(scratch+0x20U);context->r6=guest_address(scratch+0x30U);context->r7=guest_address(scratch+0x40U);
              ++camera_queries;
              ray(rdram,context);
              if(context->r2==0)continue;
              const jfg::PcOrbitPoint hit{ram.f32(scratch+0x44U),ram.f32(scratch+0x48U),ram.f32(scratch+0x4CU)};
              const auto shortened=jfg::pc_camera_body_clearance(from,clear,probe,hit);
              if(distance2(shortened)<distance2(candidate)){candidate=shortened;nearest_hit=hit;}
            }
            if(candidate.x==clear.x && candidate.y==clear.y && candidate.z==clear.z)break;
            put(point,candidate);*context=entry;original(rdram,context);
            const jfg::PcOrbitPoint checked{ram.f32(point),ram.f32(point+4U),ram.f32(point+8U)};
            clear=jfg::pc_camera_clearance(from,candidate,checked);
            state.gameplay_trace.event("pc-camera-body",{state.vi_retraces,pass,
              std::bit_cast<std::uint32_t>(nearest_hit.x),std::bit_cast<std::uint32_t>(nearest_hit.y),std::bit_cast<std::uint32_t>(nearest_hit.z),
              std::bit_cast<std::uint32_t>(clear.x),std::bit_cast<std::uint32_t>(clear.y),std::bit_cast<std::uint32_t>(clear.z)});
          }
          *context=returned;
        }
        if (state.gameplay_trace) {
          const auto end = std::chrono::steady_clock::now();
          state.gameplay_trace.event("pc-camera-work", {state.vi_retraces,
              camera_body_count - camera_count, state.cpu_count - camera_body_count,
              static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                  camera_body_started-camera_started).count()),
              static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                  end-camera_body_started).count()),
              state.profile_host_wait_us-camera_body_wait, camera_queries});
        }
        state.pc_camera_resolved=clear;state.pc_camera_resolved_valid=true;
        (void)memory.write_u32(point,std::bit_cast<std::uint32_t>(clear.x));
        (void)memory.write_u32(point+4U,std::bit_cast<std::uint32_t>(clear.y));
        (void)memory.write_u32(point+8U,std::bit_cast<std::uint32_t>(clear.z));
        state.gameplay_trace.event("pc-camera-clearance",{state.vi_retraces,
          std::bit_cast<std::uint32_t>(wanted.x),std::bit_cast<std::uint32_t>(wanted.y),std::bit_cast<std::uint32_t>(wanted.z),
          std::bit_cast<std::uint32_t>(resolved.x),std::bit_cast<std::uint32_t>(resolved.y),std::bit_cast<std::uint32_t>(resolved.z),
          std::bit_cast<std::uint32_t>(clear.x),std::bit_cast<std::uint32_t>(clear.y),std::bit_cast<std::uint32_t>(clear.z)});
        return 1;
      }
    }
  }
  if(target==0x8002CF6CU && state.modern_controls_active && (state.play_mode || state.input_replay_loaded) &&
     (state.input_record_path.empty() || state.pc_record_extended) &&
     state.pc_camera_actor==0) {
    jfg::mod::Memory ram({rdram,kRdramSize});
    const auto actor=static_cast<std::uint32_t>(context->r4),control=static_cast<std::uint32_t>(context->r5);
    if(ram.valid(control,0x5C4U) && ram.valid(actor,0xA4U) && ram.u32(actor+0x68U)==control &&
       ram.u32(target)==0x27BDFF10U && ram.u32(target+4)==0xAFBF002CU &&
       jfg::mod::pc_gameplay_active(ram,actor) && ram.u8(0x800FD7BDU)==0 && !jfg::mod::scripted_camera_active(ram,actor)) {
      const auto port=ram.u8(control);
      if(port<4 && state.controller_ports.samples[port].connected) {
        const auto &frame=state.pc_frames[port];
        const bool shoulder=modern_shoulder_current(state,port,actor);
        // Keep the normal view through the first aim-press update, until the
        // native state has actually handed ownership to its aiming camera.
        const bool normal_view=(shoulder && !state.pc_shoulders[port].aiming) ||
          jfg::pc_normal_camera_owns_look((state.controller_ports.samples[port].buttons&0x10)!=0,ram.u8(control+0x568U));
        if((frame.pc_mode==3 || frame.pc_mode==4 || frame.pc_mode==5) && normal_view &&
           (!state.pc_input_blocked || shoulder)) {
          auto* original=jfg_generated_lookup_function(static_cast<std::int32_t>(target));
          if(original) {
            auto look=jfg::PcLookDelta{frame.look_x,frame.look_y};
            if(frame.pc_mode==3) {
              if(state.pc_mouse_used[port])look={};
              state.pc_mouse_used[port]=true;
            }
            state.pc_scope_motion[port].clear();
            state.pc_camera_actor=actor;state.pc_camera_control=control;
            state.pc_camera_stack=static_cast<std::uint32_t>(context->r29)-0xF0U;
            const float ticks=std::bit_cast<float>(static_cast<std::uint32_t>(context->r6));
            state.pc_camera_look=shoulder?jfg::PcLookDelta{}:
              frame.pc_mode==5?state.pc_stick_motion[port].take(look,ticks,actor):
              jfg::pc_camera_delta(frame.pc_mode,look,ticks);
            state.pc_camera_ticks=ticks;
            state.pc_camera_resolved_valid=false;
            original(rdram,context);
            if(shoulder && state.pc_camera_resolved_valid) {
              // Use the collision-resolved position this update. The original
              // camera's follow interpolation otherwise adds drift to raw look.
              const auto camera=ram.u32(0x800F6DC0U);
              if(ram.valid(camera,0x4CU)) {
                const auto point=state.pc_camera_resolved;
                hle::GuestMemory memory({rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
                (void)memory.write_u32(camera+0xCU,std::bit_cast<std::uint32_t>(point.x));
                (void)memory.write_u32(camera+0x10U,std::bit_cast<std::uint32_t>(point.y));
                (void)memory.write_u32(camera+0x14U,std::bit_cast<std::uint32_t>(point.z));
                // Scenery may move the camera off its ideal orbit. Preserve
                // that safe position without rotating away from the input view;
                // otherwise entering aim changes direction beside a wall.
                const auto &view=state.pc_shoulders[port];
                const int yaw=jfg::pc_shoulder_camera_yaw(view.yaw);
                const int pitch=view.pitch;
                ram.put16(camera,static_cast<std::uint16_t>(yaw));ram.put16(camera+2U,static_cast<std::uint16_t>(pitch));
                state.gameplay_trace.event("pc-shoulder-camera",{state.vi_retraces,actor,
                  static_cast<std::uint16_t>(yaw),static_cast<std::uint16_t>(pitch),
                  ram.u32(camera+0xCU),ram.u32(camera+0x10U),ram.u32(camera+0x14U)});
              }
            }
            state.gameplay_trace.event("pc-camera-context",{state.vi_retraces,actor,
              ram.u8(control+0x568U),ram.u16(actor+0x48U),ram.u32(control+0x4U),
              ram.u32(control+0x8U),ram.u32(control+0x10U)});
            state.pc_camera_actor=state.pc_camera_control=state.pc_camera_stack=0;state.pc_camera_look={};
            return 1;
          }
        } else state.pc_orbits[port].active=false;
      }
    }
  }
  if(target==0x80039FCCU && modern_air_aim_active(state,static_cast<std::uint32_t>(context->r4),static_cast<std::uint32_t>(context->r5))) {
    jfg::mod::Memory ram({rdram,kRdramSize});
    const auto control=static_cast<std::uint32_t>(context->r5);
    // Match the character dispatcher's aimed targeting policy: keep recoil
    // countdown, but do not replace the player's aim with a hip-fire target.
    const int ticks=static_cast<int>(ram.u32(static_cast<std::uint32_t>(context->r29)+0x1CU));
    const auto counter=ram.u8(control+0x1F8U);
    if(ram.u8(control+0x1F4U))
      ram.put16(control+0x1F8U,static_cast<std::uint16_t>((std::max(0,int(counter)-ticks)<<8U)|ram.u8(control+0x1F9U)));
    ram.put16(control+0x1F4U,static_cast<std::uint16_t>(0x100U|ram.u8(control+0x1F5U)));
    return 1;
  }
  // Modern Jump is a locomotion action even while Aim is held. Reuse the
  // character's complete native ground jump handler (animation, collision
  // eligibility, takeoff and weapon logic); never synthesize vertical velocity.
  // Its ABI matches the aim handler's actor/control/movement/heading/ticks.
  const std::uint32_t aim_jump_ground=
    (target==0x00F0418CU || target==0x00F04658U)?0x00F02B2CU:
    (target==0x01003F30U || target==0x01004440U)?0x01002708U:
    (target==0x0110430CU || target==0x01104614U)?0x01102A54U:0U;
  if(aim_jump_ground && state.modern_controls_active && !state.pc_input_blocked) {
    jfg::mod::Memory ram({rdram,kRdramSize});
    const auto actor=static_cast<std::uint32_t>(context->r4),control=static_cast<std::uint32_t>(context->r5);
    if(ram.valid(actor,0xA4U) && ram.valid(control,0x5D0U) && ram.u32(actor+0x68U)==control) {
      const auto port=ram.u8(control),movement=ram.u8(control+0x568U);
      const auto table=ram.u32(0x800A18B0U);
      if(modern_shoulder_current(state,port,actor) &&
         (movement==10U || movement==11U) && ram.u8(control+0x534U)==0 &&
         (ram.u32(0x800F6DA4U)&8U) && ram.valid(table,0x24U) && ram.u32(table+0xCU)==8U &&
         !jfg::mod::scripted_camera_active(ram,actor)) {
        if(auto* ground=jfg_generated_lookup_function(static_cast<std::int32_t>(aim_jump_ground))) {
          hle::GuestMemory memory({rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
          const auto held=ram.u32(0x800F6DA0U);
          // Ground's Aim-first branch otherwise prevents its native Jump test.
          // Mask only that branch for this action dispatch; retain the user's
          // held/toggled Aim intent so ordinary landing re-enters aiming.
          (void)memory.write_u32(0x800F6DA0U,held&~0x10U);
          ram.put16(control+0x568U,ram.u8(control+0x569U));
          ground(rdram,context);
          (void)memory.write_u32(0x800F6DA0U,held);
          const auto after=ram.u8(control+0x568U);
          if(after==0U)ram.put16(control+0x568U,static_cast<std::uint16_t>((movement<<8U)|ram.u8(control+0x569U)));
          state.gameplay_trace.event("modern-aim-jump",{state.vi_retraces,actor,movement,after,held});
          return 1;
        }
      }
    }
  }
  // Separate look input is recorded alongside movement; older replays remain stock.
  if(target==0x8003AABCU && state.modern_controls_active && (state.play_mode || state.input_replay_loaded) &&
     (state.input_record_path.empty() || state.pc_record_extended) && !state.pc_input_blocked) {
    jfg::mod::Memory pc_memory({rdram,kRdramSize});
    const auto actor=static_cast<std::uint32_t>(context->r4);
    const auto control=static_cast<std::uint32_t>(context->r5);
    if(pc_memory.valid(control,0x5C4U) && pc_memory.valid(actor,0xA4U) &&
       pc_memory.u32(actor+0x68U)==control && pc_memory.u32(target)==0x27BDFFA8U &&
       pc_memory.u32(target+4)==0xAFB00018U &&
       jfg::mod::pc_actions_active(pc_memory,actor) && pc_memory.u8(0x800FD7BDU)==0 && !jfg::mod::scripted_camera_active(pc_memory,actor)) {
      const auto port=pc_memory.u8(control);
      if(port<4U && state.controller_ports.samples[port].connected) {
        const auto &frame=state.pc_frames[port];
        const bool mouse=frame.pc_mode==1 || frame.pc_mode==3;
        const bool direct=frame.pc_mode==3 || frame.pc_mode==4 || frame.pc_mode==5;
        if(frame.pc_mode!=0) {
          auto look=jfg::PcLookDelta{frame.look_x,frame.look_y};
          if(mouse && state.pc_mouse_used[port])look={};
          if(mouse)state.pc_mouse_used[port]=true;
          const auto stack=static_cast<std::uint32_t>(context->r29);
          const bool valid_limits=pc_memory.valid(stack,0x24U);
          const float ticks=valid_limits?std::bit_cast<float>(pc_memory.u32(stack+0x20U)):0.0F;
          if(frame.pc_mode==5)look=state.pc_stick_motion[port].take(look,ticks,actor|1U);
          else if(frame.pc_mode==4)look=jfg::pc_camera_delta(frame.pc_mode,look,ticks);
          const int minimum=valid_limits?static_cast<std::int32_t>(pc_memory.u32(stack+0x10U)):0;
          const int maximum=valid_limits?static_cast<std::int32_t>(pc_memory.u32(stack+0x14U)):0;
          const auto before=jfg::PcLookDelta{pc_memory.s16(control+0x11CU),pc_memory.s16(control+0x1E2U)};
          hle::GuestMemory memory({rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
          std::uint32_t x=0,y=0;
          if(memory.read_u32(0x800F6DB4U,x) && memory.read_u32(0x800F6DB8U,y)) {
            auto* original=jfg_generated_lookup_function(static_cast<std::int32_t>(target));
            if(original) {
              (void)memory.write_u32(0x800F6DB4U,direct?0U:static_cast<std::uint32_t>(look.x));
              (void)memory.write_u32(0x800F6DB8U,direct?0U:static_cast<std::uint32_t>(look.y));
              const auto modern_game=pc_memory.u32(0x800FD7D4U);
              const auto modern_character=pc_memory.u8(control+1U)&3U;
              const bool modern_scope=(frame.modern_flags&jfg::kModernActionsFlag) &&
                modern_character<3U && pc_memory.valid(modern_game,0x538U) &&
                pc_memory.u8(modern_game+0x15CU+modern_character*0x76U+0x70U)==7U &&
                pc_memory.u8(control+0x18EU)==0;
              std::uint32_t held=0;
              if(modern_scope && memory.read_u32(0x800F6DA0U,held)) {
                (void)memory.write_u32(0x800F6DA0U,held&~0xCU);
                (void)memory.write_u32(control+0x1F0U,0);
                const auto before_zoom=std::bit_cast<float>(pc_memory.u32(control+0x1ECU));
                const auto after_zoom=state.modern_scope_zoom[port].take(before_zoom);
                if(after_zoom!=before_zoom && std::isfinite(after_zoom)) {
                  (void)memory.write_u32(control+0x1ECU,std::bit_cast<std::uint32_t>(after_zoom));
                  state.gameplay_trace.event("modern-scope",{state.vi_retraces,port,
                    std::bit_cast<std::uint32_t>(before_zoom),std::bit_cast<std::uint32_t>(after_zoom)});
                }
              }
              original(rdram,context);
              if(modern_scope)(void)memory.write_u32(0x800F6DA0U,held);
              (void)memory.write_u32(0x800F6DB4U,x);(void)memory.write_u32(0x800F6DB8U,y);
              ++state.pc_aim_calls;
              if(direct && valid_limits && minimum>=-32768 && maximum<=32767 && minimum<=maximum) {
                const auto raw_look=look;
                float zoom=1.0F;
                const auto game=pc_memory.u32(0x800FD7D4U);
                const auto character=pc_memory.u8(control+1U)&3U;
                if(character<3U && pc_memory.valid(game,0x538U) &&
                   pc_memory.u8(game+0x15CU+character*0x76U+0x70U)==7U)
                  zoom=std::bit_cast<float>(pc_memory.u32(control+0x1ECU));
                look=state.pc_scope_motion[port].take(look,zoom,(std::uint64_t{actor}<<8U)|frame.pc_mode);
                if(zoom>1.0F && zoom<=5.0F)
                  state.gameplay_trace.event("pc-scope",{state.vi_retraces,port,
                    std::bit_cast<std::uint32_t>(zoom),static_cast<std::uint32_t>(raw_look.x),
                    static_cast<std::uint32_t>(raw_look.y),static_cast<std::uint32_t>(look.x),static_cast<std::uint32_t>(look.y)});
                // The guest aim pitch is positive up; the normal camera's
                // world pitch is positive down. Keep one mouse convention.
                const auto aim_look=jfg::pc_guest_aim_look(look,
                    (frame.modern_flags&jfg::kModernActionsFlag)!=0);
                const auto after=jfg::pc_direct_aim(before.x,before.y,aim_look,minimum,maximum);
                pc_memory.put16(control+0x11CU,static_cast<std::uint16_t>(after.x));
                pc_memory.put16(control+0x1E2U,static_cast<std::uint16_t>(after.y));
                if(modern_shoulder_current(state,port,actor)) {
                  state.pc_shoulders[port].yaw=after.x;state.pc_shoulders[port].pitch=-after.y;
                }
                (void)memory.write_u32(control+0x1E4U,0);(void)memory.write_u32(control+0x1E8U,0);
                ++state.pc_direct_aim_calls;
              }
              // Ground aiming otherwise ignores the movement stick. The guest
              // consumes these local velocities in its ordinary physics/collision
              // path. Leave jumps, swimming, damage and scripted states alone.
              const auto movement_state=pc_memory.u8(control+0x568U);
              if((frame.pc_mode==3 || frame.pc_mode==4 || frame.pc_mode==5) && (movement_state==10 || movement_state==11) && (pc_memory.u8(control+1U)&3U)<3U) {
                const auto &move=state.controller_ports.samples[port];
                const auto velocity=jfg::pc_aim_movement(move.x,move.y);
                (void)memory.write_u32(control+0x10U,std::bit_cast<std::uint32_t>(velocity.side));
                (void)memory.write_u32(control+0x4U,std::bit_cast<std::uint32_t>(velocity.forward));
              }
              state.gameplay_trace.event("pc-aim",{state.vi_retraces,state.controller_samples,port,frame.pc_mode,
                static_cast<std::uint32_t>(look.x),static_cast<std::uint32_t>(look.y),
                pc_memory.u16(control+0x11CU),pc_memory.u16(control+0x1E2U),movement_state,
                pc_memory.u32(control+4U),pc_memory.u32(control+0x10U)});
              return 1;
            }
          }
        }
      }
    }
  }
  // mainChangeLevel is the explicit lifetime boundary, including transitions
  // which recycle the same player/control addresses. Never carry the outgoing
  // shoulder heading into a new native entrance sequence.
  if(target==0x8004665CU && state.modern_controls_active) {
    state.pc_shoulders.fill({});state.pc_orbits.fill({});
    state.gameplay_trace.event("pc-view-area-reset",{state.vi_retraces,
      static_cast<std::uint32_t>(context->r4),static_cast<std::uint32_t>(context->r5)});
  }
  if (state.navigation_mod.observing()) {
    jfg::mod::Memory memory({rdram,kRdramSize});
    if (target==0x8004665CU) {
      state.navigation_mod.transition();
      // Invalidate the old map before the game starts loading another room.
      std::ostringstream output;
      state.navigation_mod.write_state(output,memory,{},UINT32_MAX,false);
      if(!write_mod_export(state, state.navigation_output/L"live.json",output.str()))
        ++state.navigation_mod.invalid;
    }
    if (target==0x80032A48U) {
      (void)state.navigation_mod.observe_player(memory,static_cast<std::uint32_t>(context->r4));
      (void)state.navigation_mod.full_health(memory,static_cast<std::uint32_t>(context->r4));
    }
    if (target==0x00310600U)
      (void)state.navigation_mod.clear_enemy(memory,static_cast<std::uint32_t>(context->r4));
  }
#endif
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  if (target == 0x80050154U || target == 0x80001EF8U || target == 0x80001EE8U || target == 0x8008AD10U) {
    static const bool audio_scheduler_trace = [] {
      char value[2]{};
      return GetEnvironmentVariableA("JFG_DIAGNOSTIC_AUDIO_SCHEDULER", value, 2U)==1U && value[0]=='1';
    }();
    if (audio_scheduler_trace && state.gameplay_trace && state.mmio_trace) {
      hle::GuestMemory m({state.rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
      std::uint32_t countdown=0,frames=0,maximum=0,minimum=0,queued=0,mode=0;
      if(m.read_u32(0x800A38C8U,countdown) && m.read_u32(0x800A06ECU,frames) &&
         m.read_u32(0x800F2168U,maximum) && m.read_u32(0x800F216CU,minimum)) {
        (void)m.read_u32(0x800F1AFCU,queued);(void)m.read_u32(0x800A06DCU,mode);
        state.gameplay_trace.event("audio-scheduler",{
          state.vi_retraces,state.controller_samples,target,state.cpu_count,
          state.mmio_trace->ai_dma.read(4U,state.cpu_count).value_or(UINT32_MAX),
          state.mmio_trace->ai_dma.deadline().value_or(0U),countdown>>24U,frames,maximum,minimum,
          state.host_audio.queued_bytes(),state.host_audio.consumed_bytes(),
          static_cast<std::uint32_t>(context->r7),queued,mode,
          0U,state.guest_active_rsp_type});
      }
    }
  }
  if(state.diagnostic_host_clock && state.native_audio_playback) {
    // Run a completed audio command list as soon as its RSP slot is free.
    // Waiting for the next notification retrace can strand the audio thread.
    if(target==0x80050C90U && (static_cast<std::uint32_t>(context->r7)&2U)) {
      hle::GuestMemory m({state.rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
      const auto scheduler=static_cast<std::uint32_t>(context->r4);
      std::uint32_t expected=0,head=0,active=0,kind=0,mode=0,frames=0;
      if(m.read_u32(0x800E97A0U,expected) && scheduler==expected &&
          m.read_u32(scheduler+0x2E4U,head) && head &&
          m.read_u32(scheduler+0x304U,active) && active==0U &&
          m.read_u32(head+0x10U,kind) && kind==2U &&
          m.read_u32(0x800A06DCU,mode) && mode==0U &&
          m.read_u32(0x800A06ECU,frames) && frames==2U) {
        if(!m.write_u32(scheduler+0x304U,1U))
          fail_closed_dispatch(state,"audio","ready-task-state",target);
        if(state.gameplay_trace)state.gameplay_trace.event("audio-ready-task",{
            state.vi_retraces,state.cpu_count,scheduler,head,state.host_audio.queued_bytes(),
            state.host_audio.consumed_bytes()});
      }
    }
    if(target==0x80050154U && state.host_audio.started()) {
      hle::GuestMemory m({state.rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
      std::uint32_t countdown=0,frames=0,mode=0,queued=0;
      const auto buffered=state.host_audio.queued_bytes()-state.host_audio.consumed_bytes();
      if(m.read_u32(0x800A38C8U,countdown) && m.read_u32(0x800A06ECU,frames) &&
          m.read_u32(0x800A06DCU,mode) && m.read_u32(0x800F1AFCU,queued)) {
        const auto adjusted=native_audio_countdown(countdown,frames,mode,queued,
            state.mmio_trace->ai_dma.read(12U,state.cpu_count).value_or(UINT32_MAX),
            state.host_audio.frequency(),buffered);
        if(adjusted) {
          if(!m.write_u32(0x800A38C8U,*adjusted))
            fail_closed_dispatch(state,"audio","demand-countdown",target);
          if(state.gameplay_trace)state.gameplay_trace.event("audio-demand",{
              state.vi_retraces,state.cpu_count,buffered,
              native_audio_target_bytes(state.host_audio.frequency())});
        }
      }
    }
  }
  // Private fixture travel request; let mainChangeLevel prepare all globals.
  if (target == 0x8004665CU && state.timing_fixture) {
    if (state.timing_trace)
      state.timing_trace << "travel-request\t" << state.vi_frames << '\t'
          << context->r4 << '\t' << context->r5 << '\t'
          << context->r6 << '\t' << context->r7 << '\t' << context->r31 << '\n';
    if (context->r4 == 127U || context->r4 == 214U) context->r5 = 2U;
    if (context->r4 == 214U) {
      char value[16]{};
      const auto length = GetEnvironmentVariableA("QUARRY_FIXTURE_DESTINATION", value, 16U);
      const auto destination = length > 0U && length < 16U ? std::stoul(value) : 294U;
      if (destination != 294U && destination != 272U && destination != 283U && destination != 354U && destination != 411U && destination != 357U)
        fail_closed_dispatch(state, "diagnostic", "fixture-destination", target);
      context->r4 = destination;
      char group[16]{};
      const auto group_length = GetEnvironmentVariableA("QUARRY_FIXTURE_GROUP", group, 16U);
      if (group_length > 0U && group_length < 16U) context->r7 = std::stoul(group);


      if (state.timing_trace)
        state.timing_trace << "travel-fixture\t" << state.vi_frames << '\t'
            << destination << '\t' << context->r5 << '\t' << context->r6 << '\t'
            << context->r7 << '\n';
    }
  }
  switch (target) {
  case 0x020002E8U: // overlay 32: mrhintsControl / KingBear
    note_hints_control_call(state, target, *context);
    break;
  case 0x02002540U: // overlay 32: mrHintsTalk
    ++state.phase9_hints_talk_calls;
    break;
  case 0x00700148U: // overlay 7: hinttextStart
    ++state.phase9_hinttext_start_calls;
    break;
  case 0x00700D6CU: // overlay 7: hinttextStop
    ++state.phase9_hinttext_stop_calls;
    break;
  case 0x0070070CU: // overlay 7: advance queued hint page
    ++state.phase9_hinttext_advance_calls;
    break;
  case 0x00700D80U: // overlay 7: hinttextActive
    ++state.phase9_hinttext_active_calls;
    break;
  case 0x00701CCCU: // overlay 7: hinttextUpdate
    ++state.phase9_hinttext_update_calls;
    break;
  case 0x007016B4U: // overlay 7: accept/parse current hint page
    ++state.phase9_hinttext_accept_calls;
    break;
  case 0x8004B0F4U: // rumbleStart
  case 0x8004B204U: // rumbleStop
  case 0x8004B29CU: // rumbleAlter
  case 0x8004B3D8U: { // rumbleMax
    jfg::mod::Memory ram({rdram,kRdramSize});
    const auto player=static_cast<std::uint32_t>(context->r4);
    state.gameplay_trace.event("pc-rumble-request",{
      state.vi_retraces,target,player,static_cast<std::uint32_t>(context->r5),
      static_cast<std::uint32_t>(context->r6),ram.u8(0x800A3470U),
      ram.u8(0x800FD7BDU),player<4U?ram.u8(0x800FB100U+player):255U});
    request_pc_rumble(state,target,*context);
    break;
  }
  case 0x80032A48U: // controlPlayer
    ++state.phase9_player_control_calls;
    state.phase9_player_actor = static_cast<std::uint32_t>(context->r4);
    break;
  case 0x80037550U: { // controlUpdateWeapon
    ++state.phase9_weapon_update_calls;
    hle::GuestMemory memory(
        {rdram, kRdramSize},
        hle::GuestMemory::Layout::native_word_big_endian);
    std::uint32_t held = 0U;
    std::uint32_t pressed = 0U;
    if (memory.read_u32(0x800F6DA0U, held) && (held & 0x2000U) != 0U)
      ++state.phase9_weapon_fire_held_updates;
    if (memory.read_u32(0x800F6DA4U, pressed) && (pressed & 0x2000U) != 0U)
      ++state.phase9_weapon_fire_pressed_updates;
    break;
  }
  case 0x800386B0U: // func_80038488 (shooting_func)
    ++state.phase9_weapon_fire_calls;
    break;
  case 0x80038630U: // controlFireDummyShot
    ++state.phase9_weapon_dummy_fire_calls;
    break;
  case 0x800478A4U: // mainIncreaseWeaponShots
    ++state.phase9_weapon_shot_calls;
    break;
  case 0x800478E0U: // mainIncreaseWeaponHits
    ++state.phase9_weapon_hit_calls;
    break;
  case 0x80047868U: // mainIncreaseWeaponKills
    ++state.phase9_enemy_kill_calls;
    break;
  case 0x800163FCU: // trackGetPlayerIntersect
  case 0x80034BB0U: // controlGroundHits
    ++state.phase9_collision_calls;
    break;
  case 0x80063358U: // partUpdateParticles
    ++state.phase9_particle_update_calls;
    break;
  case 0x8004665CU: // mainChangeLevel
    ++state.phase9_level_change_calls;
    if(state.gameplay_trace)state.gameplay_trace.event("route-transition", {
      state.vi_retraces,state.controller_samples,target,
      static_cast<std::uint32_t>(context->r4),static_cast<std::uint32_t>(context->r5),
      state.phase9_level_change_calls});
    break;
  case 0x80046378U: // mainRestartAfterDeath
    ++state.phase9_death_restart_calls;
    if(state.gameplay_trace)state.gameplay_trace.event("route-transition", {
      state.vi_retraces,state.controller_samples,target,
      static_cast<std::uint32_t>(context->r4),static_cast<std::uint32_t>(context->r5),
      state.phase9_death_restart_calls});
    break;
  case 0x80079890U: // animseqCamera
    ++state.phase9_cutscene_camera_calls;
    break;
  case 0x8007E150U: // hitPlayer
    ++state.phase9_player_hit_check_calls;
    break;
  default:
    break;
  }
#endif
#if !defined(JFG_PHASE8_LIVE_RUNTIME)
  state.journal.append(NativeEventKind::kDispatch, target);
#endif
  if (!state.guest_os_probe) {
    state.cpu_count += kCountTicksPerDispatch;
    service_si_deadline(state, target);
    service_timers(state, target);
  }
  state.recent_dispatches[state.recent_dispatch_position %
                          state.recent_dispatches.size()] = target;
  ++state.recent_dispatch_position;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  g_community_crash_context = context;
  if (state.vi_retraces >= 5683U && state.dispatch_calls % 1024U == 0U)
    write_private_progress(state, "dispatch");
#endif

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  // Qualified bounded resident leaves cannot call, yield, write
  // memory or change section ownership. Preserve dispatch/watch history above.
  // Original timing, instruction observers and focused dispatch diagnostics
  // retain the complete generated path. Helper admission cannot cross a poll.
  if(state.resident_math_enabled &&
     (target==0x80049C84U || target==0x80049D80U || target==0x80049C30U || target==0x80049C34U) &&
     state.guest_os_probe && !state.diagnostic_cpu_costs && state.focus_update_start==0U &&
     !state.guest_clock_trace.is_open() && state.point_probe_pcs.empty() &&
     !state.device_event_stream && !state.eret_transfer_stream.is_open() &&
     !state.instruction_effect_stream.is_open()) {
    if(target==0x80049C84U && jfg_native_angle_try(rdram,context)) {
      ++state.generated_calls;++state.pure_angle_calls;return 1;
    }

    if((target==0x80049C30U || target==0x80049C34U) &&
       jfg_native_trig_try(rdram,context,target==0x80049C30U)) {
      ++state.generated_calls;++state.pure_trig_calls;return 1;
    }
    if(target==0x80049D80U && jfg_native_angle_diff_try(rdram,context)) {
      ++state.generated_calls;++state.pure_diff_calls;return 1;
    }
  }
  // Community: romCopy without the PI/thread round trip (see
  // community_native_romcopy).
  if (target == 0x8004D640U && community_native_romcopy(state, *context)) {
    ++state.generated_calls;
    return 1;
  }
#endif
  if (target == jfg::boot::kSyncPrintSink) {
    hle::GuestMemory memory({rdram, kRdramSize},
                           hle::GuestMemory::Layout::native_word_big_endian);
    if (!jfg::boot::invoke_sync_print_sink(memory, *context))
      fail_closed_dispatch(state, "guest-leaf", "sync-print-sink-state", target);
    ++state.sync_print_sink_calls;
    return 1;
  }
  if (state.guest_os_probe && state.native_audio_source &&
      target == jfg::boot::kAudioSourceCallback) {
    hle::GuestMemory memory({rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
    const auto source=static_cast<std::uint32_t>(context->r4);
    const auto length=static_cast<std::uint32_t>(context->r5);
    const auto loaded=jfg::boot::provide_native_audio_source(memory,
        {state.rom,state.rom_size},source,length);
    if(!loaded) fail_closed_dispatch(state,"native-audio","source-cache-contract",target);
    context->r2=loaded->physical_address;
    ++state.native_audio_source_requests;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    if((state.native_audio_source_requests&1023U)==1U)
      state.gameplay_trace.event("native-audio-source",{state.vi_retraces,
          state.native_audio_source_requests,source,length,loaded->physical_address});
#endif
    return 1;
  }
  const auto mapping = std::lower_bound(
      std::begin(kMappings), std::end(kMappings), target,
      [](const Mapping &candidate, const std::uint32_t lookup_address) {
        return candidate.vram < lookup_address;
      });
  // Normalized overlay imports (including original CIC routines) must use
  // the same checked publication/lease path as other generated overlays.
  // The OS-execution profile never substitutes a mapped HLE overlay call.
  const bool mapped = mapping != std::end(kMappings) && mapping->vram == target &&
      !(state.guest_os_probe && target < kKseg0);
  const char *name = mapped ? mapping->name : nullptr;
  const std::size_t mapping_index =
      mapped ? static_cast<std::size_t>(mapping - std::begin(kMappings))
             : kMappingCount;
  if (name == nullptr) {
    if (recomp_func_t *const generated =
            jfg_generated_lookup_function(address)) {
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      // Generated calls do not preserve a meaningful guest return address, so
      // retain the host dispatch nesting explicitly. The shared hint-text
      // overlay is updated by a resident loop guarded by
      // runlinkIsModuleLoaded(). Its stable cross-overlay entry points can
      // otherwise execute while that guard still reports it as unloaded.
      const std::uint32_t caller_section = state.executing_generated_section;
      const bool caller_is_overlay = state.executing_generated_overlay;
      std::uint32_t target_section = UINT32_MAX;
      JfgGeneratedSectionMetadata target_metadata{};
      const auto cached_section = state.generated_section_cache.find(target);
      if (cached_section != state.generated_section_cache.end()) {
        target_section = cached_section->second.first;
        target_metadata = cached_section->second.second;
      } else {
        for (std::uint32_t section = 0U;
             section < jfg_generated_section_count(); ++section) {
          JfgGeneratedSectionMetadata metadata{};
          if (jfg_generated_section_metadata(section, &metadata) == 0)
            fail_closed_dispatch(state, "generated", "section-metadata",
                                 target);
          const std::uint64_t text_begin = metadata.linked_vram;
          const std::uint64_t text_end = text_begin + metadata.text_size;
          if (target < text_begin || target >= text_end)
            continue;
          target_section = section;
          target_metadata = metadata;
          break;
        }
        state.generated_section_cache.emplace(target,
            std::make_pair(target_section, target_metadata));
      }
      if (target == 0x80052CFCU &&
          !note_guest_cold_overlay_load(state, static_cast<std::uint32_t>(context->r4)))
        fail_closed_dispatch(state, "runlink", "cold-load-identity", target);
      if (target_metadata.is_overlay == 1U) {
        if (!ensure_guest_overlay_allocation(state, target_metadata, *context))
          fail_closed_dispatch(state, "runlink", "guest-overlay-load", target);
        if (state.pending_cold_overlay_images.contains(target_metadata.linked_vram) &&
            !publish_synthetic_overlay(state, target_section, target_metadata))
          fail_closed_overlay_publication(state, target);
      }
      if (caller_is_overlay && target_metadata.is_overlay == 1U &&
          target_section == kHintTextOverlaySection &&
          target_section != caller_section) {
        const bool was_active =
            state.active_overlay_sections.contains(target_section);
        if (!publish_synthetic_overlay(state, target_section,
                                       target_metadata)) {
          fail_closed_overlay_publication(state, target);
        }
        if (!was_active)
          state.journal.append(NativeEventKind::kOverlayPublished,
                               target_section, target_metadata.linked_vram,
                               target);
      }
#endif
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      const bool trace_matrix_conversion = target == 0x80048D84U;
      const std::uint32_t matrix_source =
          static_cast<std::uint32_t>(context->r4);
      const std::uint32_t matrix_destination =
          static_cast<std::uint32_t>(context->r5);
      std::array<std::uint32_t, 4U> matrix_first_row{};
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      const bool trace_matrix_cat = target == 0x80048C7CU;
      const std::size_t matrix_cat_slot =
          state.recent_matrix_cat_position % state.recent_matrix_cat_args.size();
      if (trace_matrix_cat) {
        state.recent_matrix_cat_args[matrix_cat_slot] = {
            static_cast<std::uint32_t>(context->r4),
            static_cast<std::uint32_t>(context->r5),
            static_cast<std::uint32_t>(context->r6)};
        hle::GuestMemory memory(
            {rdram, kRdramSize},
            hle::GuestMemory::Layout::native_word_big_endian);
        for (std::size_t word = 0U; word < 4U; ++word)
          (void)memory.read_u32(
              static_cast<std::uint32_t>(context->r4) +
                  static_cast<std::uint32_t>(word * 4U),
              state.recent_matrix_cat_words[matrix_cat_slot][word]);
        for (std::size_t word = 0U; word < 4U; ++word)
          (void)memory.read_u32(
              static_cast<std::uint32_t>(context->r5) +
                  static_cast<std::uint32_t>(word * 16U),
              state.recent_matrix_cat_words[matrix_cat_slot][word + 4U]);
      }
#endif
      if (trace_matrix_conversion) {
        hle::GuestMemory memory(
            {rdram, kRdramSize},
            hle::GuestMemory::Layout::native_word_big_endian);
        for (std::size_t index = 0U; index < matrix_first_row.size(); ++index)
          (void)memory.read_u32(
              matrix_source + static_cast<std::uint32_t>(index * 4U),
              matrix_first_row[index]);
      }
#endif
      context->r0 = 0;
      if (state.generated_relocation_targets.contains(target)) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        const bool measure_relocation = state.diagnostic_cpu_costs;
        const auto relocation_start = measure_relocation ? std::chrono::steady_clock::now()
            : std::chrono::steady_clock::time_point{};
        std::uint64_t relocation_probes = 0U;
#endif
        // Resolve the continuation with the same table/lifecycle as its function.
        context->r0 = jfg_generated_lookup_entry_offset(static_cast<std::int32_t>(target), nullptr);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (measure_relocation) relocation_probes = 1U;
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (measure_relocation) {
          ++state.profile_relocation_calls;
          state.profile_relocation_probes += relocation_probes;
          state.profile_relocation_ns += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::steady_clock::now()-relocation_start).count());
        }
#endif
      }
      ++state.generated_calls;
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      state.executing_generated_section = target_section;
      state.executing_generated_overlay =
          target_metadata.is_overlay == 1U;
#endif
      [[maybe_unused]] const std::uint32_t allocation_size =
          static_cast<std::uint32_t>(context->r4);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      if (target == 0x80044FACU)
        trace_phase9_event(state, "update-begin", state.controller_samples);
      const bool focus_window = state.focus_update_start != 0U &&
          state.completed_game_updates + 1U >= state.focus_update_start - 1U &&
          state.completed_game_updates + 1U <= state.focus_update_end + 1U;
      const bool watch_active = state.watch_word_address != 0U &&
          focus_window;
      std::uint32_t watch_before = 0U;
      const std::uint32_t watch_return_address =
          static_cast<std::uint32_t>(context->r31);
      if (watch_active) {
        hle::GuestMemory watch_memory(
            {rdram, kRdramSize},
            hle::GuestMemory::Layout::native_word_big_endian);
        if (!watch_memory.read_u32(state.watch_word_address, watch_before))
          fail_closed_dispatch(state, "diagnostic", "watch-word-read", target);
        ++state.watch_dispatch_depth;
      }
      if (state.entry_probe_target == target && focus_window) {
        trace_guest_clock(state, "entry", target);
        if (++state.entry_probe_hits > 1024U)
          fail_closed_dispatch(state, "diagnostic", "entry-probe-overflow", target);
        state.entry_probe_trace << state.completed_game_updates + 1U << '\t'
            << state.vi_retraces << '\t' << state.controller_samples << '\t'
            << std::hex << "0x" << target << '\t' << "0x"
            << static_cast<std::uint32_t>(context->r4) << '\t' << "0x"
            << static_cast<std::uint32_t>(context->r5) << '\t' << "0x"
            << static_cast<std::uint32_t>(context->r6) << '\t' << "0x"
            << static_cast<std::uint32_t>(context->r7) << std::dec << '\n';
        state.entry_probe_trace.flush();
        if (state.entry_fpu_trace.is_open()) {
          state.entry_fpu_trace << state.completed_game_updates + 1U << '\t'
              << state.vi_retraces << '\t' << state.controller_samples << '\t'
              << std::hex << "0x" << target;
          const fpr *registers = &context->f0;
          for (std::size_t index = 0U; index < 32U; ++index) {
            state.entry_fpu_trace << '\t' << "0x" << std::setw(8)
                << std::setfill('0') << registers[index].u32l << '\t'
                << "0x" << std::setw(8) << registers[index].u32h;
          }
          state.entry_fpu_trace << std::setfill(' ') << std::dec << '\n';
          state.entry_fpu_trace.flush();
        }
        if (state.entry_gpr_trace.is_open()) {
          state.entry_gpr_trace << state.completed_game_updates + 1U << '\t'
              << state.vi_retraces << '\t' << state.controller_samples << '\t'
              << std::hex << "0x" << target;
          const gpr *registers = &context->r0;
          for (std::size_t index = 0U; index < 32U; ++index) {
            const auto value = static_cast<std::uint64_t>(registers[index]);
            state.entry_gpr_trace << '\t' << "0x" << std::setw(8)
                << std::setfill('0') << static_cast<std::uint32_t>(value)
                << '\t' << "0x" << std::setw(8)
                << static_cast<std::uint32_t>(value >> 32U);
          }
          state.entry_gpr_trace << std::setfill(' ') << std::dec << '\n';
          state.entry_gpr_trace.flush();
        }
        if (state.entry_memory_trace.is_open()) {
          hle::GuestMemory entry_memory(
              {rdram, kRdramSize},
              hle::GuestMemory::Layout::native_word_big_endian);
          state.entry_memory_trace << state.completed_game_updates + 1U << '\t'
              << state.vi_retraces << '\t' << state.controller_samples << '\t'
              << std::hex << "0x" << target;
          for (const std::uint64_t argument :
               {context->r5, context->r6, context->r7}) {
            const auto pointer_address = static_cast<std::uint32_t>(argument);
            if (pointer_address < 0x80000000U ||
                pointer_address > 0x803FFF00U)
              fail_closed_dispatch(state, "diagnostic", "entry-memory-range", target);
            state.entry_memory_trace << '\t';
            for (std::uint32_t offset = 0U; offset < 256U; offset += 4U) {
              std::uint32_t word = 0U;
              if (!entry_memory.read_u32(pointer_address + offset, word))
                fail_closed_dispatch(state, "diagnostic", "entry-memory-read", target);
              state.entry_memory_trace << std::setw(8) << std::setfill('0')
                  << word;
            }
          }
          state.entry_memory_trace << std::setfill(' ') << std::dec << '\n';
          state.entry_memory_trace.flush();
        }
      }
#endif
      [[maybe_unused]] const auto mod_actor_argument=static_cast<std::uint32_t>(context->r4);
      const auto sound_event_argument = static_cast<std::uint32_t>(context->r5);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      if (state.navigation_mod.enabled && target == 0x01002708U) {
        jfg::mod::Memory memory({rdram, kRdramSize});
        const auto original = static_cast<std::uint32_t>(context->r6);
        const auto boosted = state.navigation_mod.juno_run_target(
            memory, mod_actor_argument, sound_event_argument, original);
        context->r6 = boosted;
        if (boosted != original && state.navigation_mod.juno_run_boosts == 1U)
          jfg::support_event("mod=juno-run-speed-2x-applied");
      }
      const CommunityEffectCall community_effect = community_effect_before(
          state, rdram, *context, target, target_section, target_metadata);
#endif
      {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        // Sound must run on the native menu update stack, never on the input
        // polling/audio thread. A stationary cursor queues no new event.
        if(state.menu_selection_feedback && (target==0x01202464U || target==0x025001A0U)) {
          const auto pending=*state.menu_selection_feedback;state.menu_selection_feedback.reset();
          const auto current=read_guest_menu(state);
          jfg::mod::Memory menu_ram({rdram,kRdramSize});
          const auto stack=static_cast<std::uint32_t>(context->r29);
          if(current.ready && current.kind==pending.kind && current.identity==pending.identity &&
             menu_ram.valid(stack-0x600U,0x600U) && menu_ram.u32(0x80001248U)==0x3C0F800FU) {
            if(auto* sound=jfg_generated_lookup_function(static_cast<std::int32_t>(0x80001248U))) {
              auto sound_context=*context;
              sound_context.f_odd=sound_context.mips3_float_mode
                  ? &sound_context.f1.u32l : &sound_context.f0.u32h;
              sound_context.r29=stack-0x40U;
              sound_context.r4=0x16BU;sound_context.r5=0;
              sound(rdram,&sound_context);
              set_cop1_cs(context->fcr31);
              state.gameplay_trace.event("menu-hover-sound",{state.vi_retraces,static_cast<unsigned>(current.kind),0x16BU});
            }
          }
        }
        ModernShoulderScope modern_shoulder(state,target,*context);
        modern_shoulder_physics(state,target,*context);
        ModernAirPhysicsScope modern_air_physics(state,target,*context);
        ModernAirAimPresentationScope modern_air_aim(state,target,*context);
        DiagnosticReticleDrawScope diagnostic_reticle(state,target,*context);
        DiagnosticLineOutputScope diagnostic_lines(state,target);
        // Diagnostic observation only: distinguish missing native reticle
        // draws, off-screen coordinates, and lost framebuffer presentation.
        if(state.gameplay_trace && (target==0x00D00008U || target==0x00D004A8U ||
                                   (target==0x8006D390U && diagnostic_reticle_lines()))) {
          jfg::mod::Memory ram({rdram,kRdramSize});
          const auto a=static_cast<std::uint32_t>(context->r4);
          const auto b=static_cast<std::uint32_t>(context->r5);
          const auto c=static_cast<std::uint32_t>(context->r6);
          const auto d=static_cast<std::uint32_t>(context->r7);
          const auto control=target==0x00D00008U?b:target==0x00D004A8U?a:0U;
          if(control && ram.valid(control,0x5C4U)) {
            const auto game=ram.u32(0x800FD7D4U);
            const auto character=ram.u8(control+1U)&3U;
            const auto weapon=character<3U && ram.valid(game,0x538U)?
                ram.u8(game+0x15CU+character*0x76U+0x70U):255U;
            state.gameplay_trace.event(target==0x00D00008U?"reticle-player":"reticle-draw",
                {state.vi_retraces,control,weapon,a,b,c,d,ram.u8(control+0x568U),
                 ram.u16(control+0x1CAU),ram.u16(control+0x1CCU),ram.u32(control+0x540U),
                 ram.u16(control+0x11CU),ram.u16(control+0x1E2U)});
          } else if(target==0x8006D390U) {
            const auto stack=static_cast<std::uint32_t>(context->r29);
            state.gameplay_trace.event("reticle-line-append",{state.vi_retraces,a,b,c,d,
                ram.u32(stack+0x10U),ram.u32(stack+0x14U)});
          }
        }
        ModernAnimationScope modern_animation(state,target,*context);
        ModernGuestTableScope modern_table(state,target,static_cast<std::uint32_t>(context->r4));
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
#if defined(JFG_EXPERIMENTAL_NATIVE_PLAY_TIMING)
        if (target == 0x80054FBCU && state.diagnostic_host_clock) {
          // Preserve viFrameSync's real-overload test, waits and elapsed delta.
          // Remove only the console's sticky 31-fast-frame recovery delay.
          auto& recovery = rdram[0xFECABU ^ 3U];
          recovery = jfg::boot::native_frame_recovery_counter(true,
              rdram[0xFECACU ^ 3U], recovery);
        }
#endif
        if (state.diagnostic_cpu_costs) {
          state.profile_dispatch_prefix_ns += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::steady_clock::now()-dispatch_prefix_started).count());
          ++state.profile_dispatch_prefix_calls;
        }
        const bool measure_update = target == 0x80044FACU && state.gameplay_trace;
        const auto pure_angle_before=state.pure_angle_calls,pure_diff_before=state.pure_diff_calls,pure_trig_before=state.pure_trig_calls;
        const auto bsp_before=measure_update && state.diagnostic_bsp_enabled ?
            jfg::boot::native_collision_counters() : jfg::boot::NativeCollisionCounters{};
        const auto bsp_calls_before=state.bsp_native_calls,bsp_refresh_before=state.bsp_clock_refreshes;
        if(measure_update && state.diagnostic_bsp_enabled)jfg::boot::native_collision_reset_max();


        if (measure_update && state.diagnostic_cpu_costs) state.profile_function_costs.clear();
        const auto update_started = measure_update ? std::chrono::steady_clock::now()
                                                  : std::chrono::steady_clock::time_point{};
        const auto update_count = measure_update ? state.cpu_count : 0U;
        const auto rom_before=measure_update && state.mmio_trace ?
            state.mmio_trace->pi_dma.rom_transfers() : jfg::boot::OriginalTimingPiDma::RomTransfers{};
        const auto update_idle = measure_update ? state.profile_idle_count_ticks : 0U;
        const auto update_wait = measure_update ? state.profile_host_wait_us : 0U;
        const auto update_instructions = measure_update ? state.observed_guest_instructions : 0U;
        const bool measure_update_cycles = measure_update && state.diagnostic_update_cycles;
        ULONG64 update_cycles_before = 0U;
        const auto update_thread = measure_update_cycles ? GetCurrentThreadId() : 0U;
        const bool update_cycles_valid = measure_update_cycles &&
            QueryThreadCycleTime(GetCurrentThread(), &update_cycles_before);
        const auto callback_start = state.profile_out_of_line_instructions;
        const auto full_probe_start = state.profile_full_execution_probes;
        const auto native_handler_start = measure_update ? state.profile_native_handler_ns : 0U;
        const auto dispatch_prefix_ns_start = state.profile_dispatch_prefix_ns;
        const auto dispatch_prefix_calls_start = state.profile_dispatch_prefix_calls;
        const auto inventory_ns_start = state.profile_overlay_inventory_ns;
        const auto inventory_calls_start = state.profile_overlay_inventory_calls;
        const auto span_instructions_start = state.native_span_instructions;
        const auto span_commits_start = state.native_span_commits;
        const auto relocation_ns_start = state.profile_relocation_ns;
        const auto relocation_calls_start = state.profile_relocation_calls;
        const auto relocation_probes_start = state.profile_relocation_probes;
        const auto dispatch_start = measure_update ? state.dispatch_calls : 0U;
        const auto mmio_start = measure_update && state.mmio_trace ? state.mmio_trace->accesses : 0U;
        const auto graphics_start = measure_update ? state.graphics_submit_microseconds : 0U;
        const auto handoff_start = state.guest_transport ? state.guest_transport->handoffs() : 0U;
        const auto transport_start = measure_update && state.diagnostic_cpu_costs && state.guest_transport
            ? state.guest_transport->timing() : BatonExecutor::Timing{};
        FILETIME process_created{}, process_exited{}, kernel_start{}, user_start{};
        ULONG64 process_cycles_start = 0;
        const bool process_costs = measure_update && state.diagnostic_cpu_costs &&
            GetProcessTimes(GetCurrentProcess(), &process_created, &process_exited, &kernel_start, &user_start) &&
            QueryProcessCycleTime(GetCurrentProcess(), &process_cycles_start);
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (state.presentation_identity_enabled) {
          if (target == 0x800079B8U)
            state.presentation_identities.retire(static_cast<std::uint32_t>(context->r4));
          else if (target == 0x800610CCU)
            state.presentation_identities.particle_retire(static_cast<std::uint32_t>(context->r4));
          else if (target == 0x8005CE60U)
            state.presentation_identities.particles_clear();
          else if (target == 0x80041174U) {
            const jfg::PresentationGuestView ram({rdram,kRdramSize});
            const auto output = static_cast<std::uint32_t>(context->r6);
            const auto owner = static_cast<std::uint32_t>(context->r7);
            const auto asset = ram.u32(static_cast<std::uint32_t>(context->r29)+0x10U);
            if (static_cast<std::uint32_t>(context->r31) == 0x8005D59CU)
              state.presentation_identities.particle_sprite({rdram,kRdramSize},
                  output,owner,asset,state.vi_retraces);
            else
              state.presentation_identities.sprite({rdram,kRdramSize},
                  output,owner,asset,state.vi_retraces);
          }
          else if (target == 0x80040AA4U)
            state.presentation_identities.camera({rdram,kRdramSize},
                static_cast<std::uint32_t>(context->r5),state.vi_retraces);
        }
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        static const bool audio_buffer_trace=[] {
          char value[2]{};
          return GetEnvironmentVariableA("JFG_DIAGNOSTIC_AUDIO_SCHEDULER",value,2U)==1U && value[0]=='1';
        }();
        const bool observe_audio_buffer=audio_buffer_trace && target==0x80096DF0U;
        const auto ai_buffer_address=observe_audio_buffer ? static_cast<std::uint32_t>(context->r4) : 0U;
        const auto ai_buffer_length=observe_audio_buffer ? static_cast<std::uint32_t>(context->r5) : 0U;
        const bool observe_camera=state.presentation_identity_enabled && target==0x80040AA4U;
        const auto camera_output=observe_camera ? static_cast<std::uint32_t>(context->r5) : 0U;
        const auto camera_matrix=observe_camera ? jfg::PresentationGuestView({rdram,kRdramSize}).u32(camera_output) : 0U;
        // US ShirukenPushMtx has the same output/actor ABI as camPushModelMtx,
        // but builds its rolling/attached world matrix through camPushFloatModelMtx.
        // That call also publishes world at 0x800FAAB0. Bind after the complete
        // custom draw transform, before another actor reuses the scratch matrix.
        const bool observe_shuriken = state.presentation_identity_enabled && target == 0x07A00F38U;
        const bool observe_rigid = state.presentation_identity_enabled &&
            (target == 0x80041CA0U || observe_shuriken);
        const auto rigid_actor = observe_rigid ? static_cast<std::uint32_t>(context->r6) : 0U;
        const auto rigid_output = observe_rigid ? static_cast<std::uint32_t>(context->r5) : 0U;
        const auto rigid_matrix = observe_rigid ? jfg::PresentationGuestView({rdram,kRdramSize}).u32(rigid_output) : 0U;
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        const auto function_started = state.diagnostic_cpu_costs ? std::chrono::steady_clock::now()
            : std::chrono::steady_clock::time_point{};
        const auto function_children = state.profile_function_self_ns;
        const auto function_wait = state.profile_host_wait_us;
#endif

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if(target==0x00317174U && target_section==3U && target_metadata.is_overlay==1U &&
           state.diagnostic_bsp_enabled && !jfg::boot::native_collision_in_fallback() && state.diagnostic_host_clock &&
           state.native_spans_enabled && !state.native_clock_sample_input.is_open() &&
           state.native_fast_ready && state.native_fast_status==context->status_reg &&
           state.execution_fast_revision==*state.io_revision &&
           !state.diagnostic_cpu_costs && state.focus_update_start==0U &&
           !state.guest_clock_trace.is_open() && state.point_probe_pcs.empty() &&
           !state.device_event_stream && !state.eret_transfer_stream.is_open() &&
           !state.instruction_effect_stream.is_open()) {
          ++state.bsp_native_calls;
          if(jfg::boot::native_collision_execute(generated,rdram,context)) {
            state.native_fast_ready=false;state.host_clock_poll_budget=0U;
            ++state.bsp_clock_refreshes;
          }
        } else
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
          if (!community_effect.skip)  // community: hide the coffee splat
#endif
          generated(rdram, context);

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (state.diagnostic_cpu_costs) {
          const auto wall_ns = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::steady_clock::now()-function_started).count());
          const auto wait_ns = (state.profile_host_wait_us-function_wait)*1000U;
          const auto active_ns = wall_ns > wait_ns ? wall_ns-wait_ns : 0U;
          const auto children_ns = state.profile_function_self_ns-function_children;
          const auto self_ns = active_ns > children_ns ? active_ns-children_ns : 0U;
          state.profile_function_self_ns += self_ns;
          auto& cost = state.profile_function_costs[target];
          cost.self_ns += self_ns;cost.inclusive_ns += active_ns;++cost.calls;
        }
        if(target==0x80096DF0U && state.diagnostic_host_clock && state.native_audio_playback &&
            static_cast<std::uint32_t>(context->r2)==0U) {
          // Deliver completed PCM before returning to guest/renderer work.
          // SDL conversion must not alter the guest floating-point context.
          std::fenv_t audio_environment{};
          if(std::feholdexcept(&audio_environment)!=0)
            fail_closed_dispatch(state,"audio","save-fenv",target);
          const bool rounded=std::fesetround(FE_TONEAREST)==0;
          const bool delivered=rounded && service_host_audio(state,false);
          const bool restored=std::fesetenv(&audio_environment)==0;
          if(!delivered || !restored)
            fail_closed_dispatch(state,"audio","immediate-host-delivery",target);
        }
        if(observe_audio_buffer && state.gameplay_trace)
          state.gameplay_trace.event("audio-ai-buffer",{state.vi_retraces,state.cpu_count,
              ai_buffer_address,ai_buffer_length,static_cast<std::uint32_t>(context->r2),
              state.mmio_trace->ai_dma.read(12U,state.cpu_count).value_or(UINT32_MAX),
              state.host_audio.queued_bytes(),state.host_audio.consumed_bytes()});
        if (observe_camera && camera_matrix &&
            jfg::PresentationGuestView({rdram,kRdramSize}).u32(camera_output)==camera_matrix+64U)
          state.presentation_identities.camera_completed({rdram,kRdramSize},camera_matrix,state.vi_retraces);
        if (observe_rigid && (!observe_shuriken ||
            jfg::PresentationGuestView({rdram,kRdramSize}).u32(rigid_output)==rigid_matrix+64U)) {
          state.presentation_identities.rigid({rdram,kRdramSize},rigid_matrix,rigid_actor,state.vi_retraces);
          if(observe_shuriken)
            state.gameplay_trace.event("shuriken-matrix",{state.vi_retraces,rigid_actor,rigid_matrix});
        }
        if (state.presentation_identity_enabled && target == 0x80060EC4U)
          state.presentation_identities.particle_allocate({rdram,kRdramSize},
              static_cast<std::uint32_t>(context->r2));
        if (measure_update) {
          if(state.mmio_trace) {
            const auto rom_after=state.mmio_trace->pi_dma.rom_transfers();
            if(rom_after.calls!=rom_before.calls)
              state.gameplay_trace.event("rom-dma",{
                state.vi_retraces,state.controller_samples,rom_after.calls-rom_before.calls,
                rom_after.bytes-rom_before.bytes,rom_after.bus_ticks-rom_before.bus_ticks,
                rom_after.scheduled_ticks-rom_before.scheduled_ticks,
                rom_after.native_calls-rom_before.native_calls});
          }
          if(state.diagnostic_bsp_enabled) {
            const auto bsp_after=jfg::boot::native_collision_counters();
            state.gameplay_trace.event("bsp-execution",{
            state.vi_retraces,state.controller_samples,state.bsp_native_calls-bsp_calls_before,
            bsp_after.native_nodes-bsp_before.native_nodes,
            bsp_after.native_edges-bsp_before.native_edges,
            bsp_after.fallbacks-bsp_before.fallbacks,
            bsp_after.expired-bsp_before.expired,
            bsp_after.invalid-bsp_before.invalid,
            bsp_after.max_nanoseconds,state.bsp_clock_refreshes-bsp_refresh_before});
          }
          if(state.resident_math_enabled)state.gameplay_trace.event("resident-math",{
            state.vi_retraces,state.controller_samples,state.pure_angle_calls-pure_angle_before,
            state.pure_diff_calls-pure_diff_before,state.pure_trig_calls-pure_trig_before});

          state.gameplay_trace.event("update-work", {state.vi_retraces, state.controller_samples,
              state.cpu_count - update_count, state.profile_idle_count_ticks - update_idle,
              state.observed_guest_instructions - update_instructions,
              static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::steady_clock::now() - update_started).count()),
              state.profile_host_wait_us - update_wait});
          if (measure_update_cycles) {
            ULONG64 update_cycles_after = 0U;
            const bool valid = update_cycles_valid && GetCurrentThreadId() == update_thread &&
                QueryThreadCycleTime(GetCurrentThread(), &update_cycles_after) &&
                update_cycles_after >= update_cycles_before;
            // Cycles describe this host thread only; never convert them to
            // milliseconds or include work performed by other guest threads.
            state.gameplay_trace.event("update-thread-cycles", {state.vi_retraces,
                state.controller_samples, valid, update_thread,
                valid ? update_cycles_after - update_cycles_before : 0U,
                state.observed_guest_instructions - update_instructions});
          }
          if (state.native_spans_enabled) {
            state.gameplay_trace.event("native-span-work", {state.vi_retraces,state.controller_samples,
                state.native_span_instructions-span_instructions_start,
                state.native_span_commits-span_commits_start});
          }
          if (state.diagnostic_cpu_costs) {
            std::vector<std::pair<std::uint32_t,State::FunctionCost>> function_costs(
                state.profile_function_costs.begin(),state.profile_function_costs.end());
            std::sort(function_costs.begin(),function_costs.end(),[](const auto& a,const auto& b) {
              return a.second.self_ns>b.second.self_ns;
            });
            for (std::size_t index=0;index<function_costs.size() && index<20U;++index) {
              const auto& item=function_costs[index];
              state.gameplay_trace.event("function-self-cost",{state.vi_retraces,state.controller_samples,
                  item.first,item.second.self_ns,item.second.inclusive_ns,item.second.calls});
            }
            FILETIME kernel_end{}, user_end{}; ULONG64 process_cycles_end = 0;
            const bool valid = process_costs &&
                GetProcessTimes(GetCurrentProcess(), &process_created, &process_exited, &kernel_end, &user_end) &&
                QueryProcessCycleTime(GetCurrentProcess(), &process_cycles_end);
            const auto ticks = [](FILETIME v) {
              return (std::uint64_t(v.dwHighDateTime)<<32U) | v.dwLowDateTime;
            };
            state.gameplay_trace.event("host-cpu-cost", {state.vi_retraces, state.controller_samples,
                state.observed_guest_instructions-update_instructions,
                state.profile_out_of_line_instructions-callback_start,
                state.profile_full_execution_probes-full_probe_start,
                state.guest_transport ? state.guest_transport->handoffs()-handoff_start : 0U,
                valid ? (ticks(kernel_end)-ticks(kernel_start))/10U : 0U,
                valid ? (ticks(user_end)-ticks(user_start))/10U : 0U,
                valid ? process_cycles_end-process_cycles_start : 0U, valid});
            if (state.guest_transport) {
              const auto transport_end = state.guest_transport->timing();
              state.gameplay_trace.event("transport-wake-cost", {state.vi_retraces, state.controller_samples,
                  transport_end.wakes - transport_start.wakes,
                  transport_end.wake_ns - transport_start.wake_ns, transport_end.max_wake_ns});
            }
            state.gameplay_trace.event("dispatch-prefix-cost", {state.vi_retraces, state.controller_samples,
                state.profile_dispatch_prefix_ns-dispatch_prefix_ns_start,
                state.profile_dispatch_prefix_calls-dispatch_prefix_calls_start,
                state.profile_overlay_inventory_ns-inventory_ns_start,
                state.profile_overlay_inventory_calls-inventory_calls_start});
            state.gameplay_trace.event("relocation-resolution-cost", {state.vi_retraces, state.controller_samples,
                state.profile_relocation_ns-relocation_ns_start,
                state.profile_relocation_calls-relocation_calls_start,
                state.profile_relocation_probes-relocation_probes_start});
            state.gameplay_trace.event("native-handler-cost", {state.vi_retraces, state.controller_samples,
                state.profile_native_handler_ns-native_handler_start,
                state.dispatch_calls-dispatch_start,
                state.mmio_trace ? state.mmio_trace->accesses-mmio_start : 0U,
                state.graphics_submit_microseconds-graphics_start,
                state.profile_host_wait_us-update_wait});
          }
        }
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        modern_shoulder_aim_camera(state,target,mod_actor_argument,sound_event_argument);
#endif
      }
      if (target == jfg::boot::kSoundNextEvent) {
        hle::GuestMemory memory({rdram, kRdramSize},
                               hle::GuestMemory::Layout::native_word_big_endian);
        if (jfg::boot::recover_empty_sound_queue(
                memory, *context, mod_actor_argument, sound_event_argument))
          ++state.sound_queue_recoveries;
      }
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      if (state.navigation_mod.observing() && target==0x80032A48U) {
        jfg::mod::Memory memory({rdram,kRdramSize});
        (void)state.navigation_mod.observe_player(memory,mod_actor_argument);
        (void)state.navigation_mod.full_health(memory,mod_actor_argument);
      }
      if (target==0x80044FACU) update_navigation_mod(state);
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      if (state.entry_probe_target == target && focus_window)
        trace_guest_clock(state, "entry-return", target);
      if (watch_active) {
        hle::GuestMemory watch_memory(
            {rdram, kRdramSize},
            hle::GuestMemory::Layout::native_word_big_endian);
        std::uint32_t watch_after = 0U;
        if (!watch_memory.read_u32(state.watch_word_address, watch_after))
          fail_closed_dispatch(state, "diagnostic", "watch-word-read", target);
        if (watch_before != watch_after) {
          if (++state.watch_dispatch_changes > 1024U)
            fail_closed_dispatch(state, "diagnostic", "watch-word-overflow", target);
          state.watch_word_trace << state.completed_game_updates + 1U << '\t'
              << state.vi_retraces << '\t' << state.controller_samples << '\t'
              << state.watch_dispatch_depth << '\t' << std::hex << "0x"
              << target << '\t' << "0x" << watch_return_address << '\t'
              << "0x" << watch_before << '\t' << "0x" << watch_after
              << std::dec << '\n';
          state.watch_word_trace.flush();
        }
        --state.watch_dispatch_depth;
      }
      if (target == 0x80044FACU && state.diagnostic_clock_boundary_vi != 0U &&
          !state.diagnostic_clock_boundary_seen && state.vi_retraces >= state.diagnostic_clock_boundary_vi) {
        // Qualification only: preserve the entire original-clock prefix and
        // fingerprint it before selecting the candidate policy. Never edit RAM.
        jfg::testkernel::Sha256 hasher;
        hasher.update_bytes(state.rdram,kRdramSize);
        const auto digest = hasher.finish();
        std::array<std::uint32_t,8> words{};
        for (std::size_t i=0;i<digest.size();++i)
          words[i/4U] = (words[i/4U]<<8U) | std::to_integer<unsigned>(digest[i]);
        state.gameplay_trace.event("clock-boundary",{state.vi_retraces,state.controller_samples,
            state.cpu_count,state.diagnostic_host_clock_requested,words[0],words[1],words[2],words[3],
            words[4],words[5],words[6],words[7]});
        state.diagnostic_clock_boundary_seen = true;
        if (state.input_replay_gameplay_time) {
          if (state.controller_samples >= state.input_replay.events().size())
            fail_closed_dispatch(state,"recorded-input","boundary-past-recording",target);
          state.replay_gameplay_first_poll = static_cast<std::size_t>(state.controller_samples);
        }
        if (state.diagnostic_host_clock_requested) {
          state.diagnostic_host_clock = true;
          state.mmio_trace->native_elapsed_time = true;
          if(state.native_audio_playback && !state.mmio_trace->ai_dma.native_queue_capacity(4U))
            fail_closed_dispatch(state,"audio","native-queue-capacity",target);
          state.execution_fast_profile_ready = false;
          state.host_clock_poll_budget = 0U;
          if (!state.host_count_clock.start(state.cpu_count,std::chrono::steady_clock::now()))
            fail_closed_dispatch(state,"native-clock","boundary-initialization",target);
          if (state.diagnostic_boundary_stall_ms != 0U) {
            state.gameplay_trace.event("native-stall-begin",{state.vi_retraces,state.cpu_count,state.diagnostic_boundary_stall_ms});
            std::this_thread::sleep_for(std::chrono::milliseconds(state.diagnostic_boundary_stall_ms));
            state.gameplay_trace.event("native-stall-end",{state.vi_retraces,
                state.host_count_clock.sample(std::chrono::steady_clock::now()).value_or(UINT64_MAX)});
          }
        }
      }
      if (target == 0x80044FACU && state.diagnostic_clock_release_vi != 0U &&
          !state.diagnostic_clock_released && state.vi_retraces >= state.diagnostic_clock_release_vi) {
        // Qualification-only transition, after a complete game update. Hash
        // the exact shared starting state before reanchoring elapsed time.
        // No guest RAM, CPU Count, input ordinal or device deadline is changed.
        jfg::testkernel::Sha256 hasher;hasher.update_bytes(state.rdram,kRdramSize);
        const auto digest=hasher.finish();std::array<std::uint32_t,8> words{};
        for (std::size_t i=0;i<digest.size();++i)
          words[i/4U]=(words[i/4U]<<8U)|std::to_integer<unsigned>(digest[i]);
        if (!state.diagnostic_host_clock || !state.native_clock_samples.release_prefix(state.cpu_count))
          fail_closed_dispatch(state,"native-clock-tape","invalid-prefix-release",target);
        state.native_clock_sample_input.close();
        state.gameplay_trace.event("native-clock-release",{state.vi_retraces,state.controller_samples,
            state.cpu_count,state.observed_guest_instructions,state.native_clock_samples.samples(),
            words[0],words[1],words[2],words[3],words[4],words[5],words[6],words[7]});
        state.host_count_clock=HostCountClock{};
        state.native_fast_ready=false;state.host_clock_poll_budget=0U;
        if (!state.host_count_clock.start(state.cpu_count,std::chrono::steady_clock::now()))
          fail_closed_dispatch(state,"native-clock-tape","release-clock-start",target);
        state.diagnostic_clock_released=true;
      }
      if (target == 0x80044FACU) {
        trace_gameplay_state(state, true);
        if (state.timing_trace.is_open())
          state.timing_trace << "game-update\t" << state.vi_frames << '\t'
                             << state.vi_retraces << "\t0\t"
                             << ++state.timing_game_updates << "\t0\t0\t0\n";
      }
      if (target == 0x80044FACU && state.update_hash_trace.is_open()) {
        ++state.completed_game_updates;
        trace_phase9_event(state, "update-end", state.controller_samples);
        write_retrace_semantic_hash(state, &state.update_hash_trace);
        if (state.update_word_trace.is_open()) {
          hle::GuestMemory word_memory(
              {rdram, kRdramSize},
              hle::GuestMemory::Layout::native_word_big_endian);
          std::uint32_t value = 0U;
          if (!word_memory.read_u32(state.update_word_address, value))
            fail_closed_dispatch(state, "diagnostic", "update-word-read", target);
          state.update_word_trace << state.completed_game_updates << '\t'
              << state.controller_samples << '\t' << state.vi_retraces
              << '\t' << state.vi_frames << '\t' << std::hex << "0x"
              << std::setw(8) << std::setfill('0') << state.update_word_address
              << '\t' << "0x" << std::setw(8) << value << std::setfill(' ')
              << std::dec << '\n';
        }
        if (state.focus_update_start != 0U &&
            state.completed_game_updates >= state.focus_update_start &&
            state.completed_game_updates <= state.focus_update_end) {
          const auto path = std::filesystem::path(state.retrace_hash_path).parent_path() /
              ("focus-update-" + std::to_string(state.completed_game_updates) + ".rdram");
          if (!write_logical_rdram_capture(state, path)) {
            std::fputs("native focus RDRAM capture failed\n", stderr);
            state.exit_requested = true;
          }
        }

      }
      if (target == 0x8004A57CU && state.actor_timing_trace &&
          state.vi_retraces <= 100U && state.timing_trace.is_open()) {
        state.timing_trace << "mm-alloc\t" << state.vi_frames << '\t'
                           << state.vi_retraces << "\t0\t" << allocation_size
                           << '\t' << static_cast<std::uint32_t>(context->r2)
                           << "\t0\t0\n";
      }
#endif
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      state.executing_generated_section = caller_section;
      state.executing_generated_overlay = caller_is_overlay;
#endif
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      if (target == 0x80011C2CU && context->r2 != 0) // objCutCameraActive
        ++state.phase9_cutscene_active_calls;
      if (target == 0x00700D80U) {
        state.phase9_hinttext_active_last_return =
            static_cast<std::uint32_t>(context->r2);
        if (context->r2 == 0U)
          ++state.phase9_hinttext_active_zero_returns;
        else
          ++state.phase9_hinttext_active_nonzero_returns;
      }
#endif
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      if (trace_matrix_cat) {
        hle::GuestMemory memory(
            {rdram, kRdramSize},
            hle::GuestMemory::Layout::native_word_big_endian);
        (void)memory.read_u32(state.recent_matrix_cat_args[matrix_cat_slot][2],
                              state.recent_matrix_cat_words[matrix_cat_slot][8]);
        ++state.recent_matrix_cat_position;
      }
#endif
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      if (trace_matrix_conversion) {
        const std::size_t slot = state.recent_matrix_conversion_position %
            state.recent_matrix_conversion_sources.size();
        state.recent_matrix_conversion_sources[slot] = matrix_source;
        state.recent_matrix_conversion_destinations[slot] =
            matrix_destination;
        state.recent_matrix_conversion_first_rows[slot] = matrix_first_row;
        hle::GuestMemory memory(
            {rdram, kRdramSize},
            hle::GuestMemory::Layout::native_word_big_endian);
        (void)memory.read_u32(matrix_destination,
                              state.recent_matrix_conversion_output_words[slot][0]);
        (void)memory.read_u32(matrix_destination + 4U,
                              state.recent_matrix_conversion_output_words[slot][1]);
        (void)memory.read_u32(matrix_destination + 32U,
                              state.recent_matrix_conversion_output_words[slot][2]);
        (void)memory.read_u32(matrix_destination + 36U,
                              state.recent_matrix_conversion_output_words[slot][3]);
        if (matrix_source == 0x800FAAF0U) {
          std::size_t watch = 0U;
          while (watch < state.matrix_watch_addresses.size() &&
                 state.matrix_watch_addresses[watch] != 0U &&
                 state.matrix_watch_addresses[watch] != matrix_destination)
            ++watch;
          if (watch < state.matrix_watch_addresses.size()) {
            state.matrix_watch_addresses[watch] = matrix_destination;
            state.matrix_watch_words[watch] =
                state.recent_matrix_conversion_output_words[slot][0];
          }
        }
        ++state.recent_matrix_conversion_position;
      }
#endif
      return 1;
    }
    if (state.runtime != nullptr &&
        state.runtime->invoke_leased_function_by_guest_address(
            target, rdram, context) == jfg::GeneratedOverlayError::none)
      return 1;
    // The normalized generated ABI gives overlays stable synthetic call
    // addresses. A direct transfer to an exact generated overlay is therefore
    // its publication boundary in this native profile; arbitrary addresses
    // remain fail-closed.
    for (std::uint32_t section = 0U;
         section < jfg_generated_section_count(); ++section) {
      JfgGeneratedSectionMetadata metadata{};
      if (jfg_generated_section_metadata(section, &metadata) == 0)
        fail_closed_dispatch(state, "generated", "section-metadata",
                             target);
      const std::uint64_t text_begin = metadata.linked_vram;
      const std::uint64_t text_end = text_begin + metadata.text_size;
      if (metadata.is_overlay != 1U || target < text_begin || target >= text_end)
        continue;
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      if (!ensure_guest_overlay_allocation(state, metadata, *context) ||
          !publish_synthetic_overlay(state, section, metadata)) {
#else
      if (jfg_generated_relocation_count(section) != 0U ||
          jfg_generated_section_lifecycle(
              1U, section, static_cast<std::int32_t>(metadata.linked_vram)) !=
              0) {
#endif
        fail_closed_overlay_publication(state, target);
      }
      state.journal.append(NativeEventKind::kOverlayPublished, section,
                           metadata.linked_vram, target);
      recomp_func_t *const published =
          jfg_generated_lookup_function(address);
      if (published == nullptr)
        fail_closed_dispatch(state, "generated", "overlay-lookup", target);
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      const std::uint32_t overlay_offset = target - metadata.linked_vram;
      if (section == 32U && overlay_offset == 0x2E8U) {
        note_hints_control_call(state, target, *context);
      } else if (section == 32U && overlay_offset == 0x2540U) {
        ++state.phase9_hints_talk_calls;
      }
#endif
      context->r0 = 0;
      if (state.generated_relocation_targets.contains(target)) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        const bool measure_relocation = state.diagnostic_cpu_costs;
        const auto relocation_start = measure_relocation ? std::chrono::steady_clock::now()
            : std::chrono::steady_clock::time_point{};
        std::uint64_t relocation_probes = 0U;
#endif
        // Resolve the continuation with the same table/lifecycle as its function.
        context->r0 = jfg_generated_lookup_entry_offset(static_cast<std::int32_t>(target), nullptr);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (measure_relocation) relocation_probes = 1U;
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (measure_relocation) {
          ++state.profile_relocation_calls;
          state.profile_relocation_probes += relocation_probes;
          state.profile_relocation_ns += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::steady_clock::now()-relocation_start).count());
        }
#endif
      }
      published(rdram, context);
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      if (target == 0x00700D80U) {
        state.phase9_hinttext_active_last_return =
            static_cast<std::uint32_t>(context->r2);
        if (context->r2 == 0U)
          ++state.phase9_hinttext_active_zero_returns;
        else
          ++state.phase9_hinttext_active_nonzero_returns;
      }
#endif
      return 1;
    }
#if defined(JFG_PHASE8_LIVE_RUNTIME)
    // Runlink may preserve its own compact load base instead of the normalized
    // synthetic address. Translate calls through the live module table back to
    // the generated section identity and offset. Matching the full identity
    // prevents a nearby asset DMA from being mistaken for executable code.
    hle::GuestMemory runlink_memory(
        {rdram, kRdramSize},
        hle::GuestMemory::Layout::native_word_big_endian);
    RunlinkAddressResolution resolution{};
    const RunlinkAddressResult resolution_result =
        resolve_runlink_text_address(runlink_memory,
                                     kRunlinkModuleTablePointer,
                                     kRunlinkOverlaySlotCount, target,
                                     resolution);
    if (resolution_result == RunlinkAddressResult::resolved) {
      std::uint32_t matched_section = UINT32_MAX;
      JfgGeneratedSectionMetadata matched_metadata{};
      for (std::uint32_t section = 0U;
           section < jfg_generated_section_count(); ++section) {
        JfgGeneratedSectionMetadata metadata{};
        if (jfg_generated_section_metadata(section, &metadata) == 0)
          fail_closed_dispatch(state, "generated", "section-metadata",
                               target);
        if (metadata.is_overlay != 1U ||
            metadata.rom_start != resolution.identity.rom_start ||
            metadata.text_size != resolution.identity.text_size ||
            metadata.data_size != resolution.identity.data_size ||
            metadata.bss_size != resolution.identity.bss_size)
          continue;
        if (matched_section != UINT32_MAX)
          fail_closed_dispatch(state, "generated", "runlink-identity-duplicate",
                               target);
        matched_section = section;
        matched_metadata = metadata;
      }
      if (matched_section == UINT32_MAX ||
          resolution.text_offset >= matched_metadata.text_size ||
          resolution.text_offset > UINT32_MAX - matched_metadata.linked_vram)
        fail_closed_dispatch(state, "generated", "runlink-identity",
                             target);
      const std::uint32_t translated_target =
          matched_metadata.linked_vram + resolution.text_offset;
      if (!publish_synthetic_overlay(state, matched_section,
                                     matched_metadata)) {
        fail_closed_overlay_publication(state, target);
      }
      recomp_func_t *const published = jfg_generated_lookup_function(
          static_cast<std::int32_t>(translated_target));
      if (published == nullptr)
        fail_closed_dispatch(state, "generated", "runlink-overlay-lookup",
                             target);
      if (matched_section == 32U && resolution.text_offset == 0x2E8U) {
        note_hints_control_call(state, target, *context);
      } else if (matched_section == 32U &&
                 resolution.text_offset == 0x2540U) {
        ++state.phase9_hints_talk_calls;
      } else if (matched_section == 7U) {
        switch (resolution.text_offset) {
        case 0x148U:
          ++state.phase9_hinttext_start_calls;
          break;
        case 0xD6CU:
          ++state.phase9_hinttext_stop_calls;
          break;
        case 0x70CU:
          ++state.phase9_hinttext_advance_calls;
          break;
        case 0xD80U:
          ++state.phase9_hinttext_active_calls;
          break;
        case 0x1CCCU:
          ++state.phase9_hinttext_update_calls;
          break;
        case 0x16B4U:
          ++state.phase9_hinttext_accept_calls;
          break;
        default:
          break;
        }
      }
      state.journal.append(NativeEventKind::kOverlayPublished,
                           matched_section, matched_metadata.linked_vram,
                           target);
      context->r0 = 0;
      if (state.generated_relocation_targets.contains(translated_target)) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        const bool measure_relocation = state.diagnostic_cpu_costs;
        const auto relocation_start = measure_relocation ? std::chrono::steady_clock::now()
            : std::chrono::steady_clock::time_point{};
        std::uint64_t relocation_probes = 0U;
#endif
        // Resolve the continuation with the same table/lifecycle as its function.
        context->r0 = jfg_generated_lookup_entry_offset(static_cast<std::int32_t>(translated_target), nullptr);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (measure_relocation) relocation_probes = 1U;
#endif
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (measure_relocation) {
          ++state.profile_relocation_calls;
          state.profile_relocation_probes += relocation_probes;
          state.profile_relocation_ns += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::steady_clock::now()-relocation_start).count());
        }
#endif
      }
      ++state.generated_calls;
      published(rdram, context);
      if (matched_section == 7U && resolution.text_offset == 0xD80U) {
        state.phase9_hinttext_active_last_return =
            static_cast<std::uint32_t>(context->r2);
        if (context->r2 == 0U)
          ++state.phase9_hinttext_active_zero_returns;
        else
          ++state.phase9_hinttext_active_nonzero_returns;
      }
      return 1;
    }
    if (resolution_result == RunlinkAddressResult::memory_fault ||
        resolution_result == RunlinkAddressResult::duplicate_module ||
        resolution_result == RunlinkAddressResult::invalid_argument)
      fail_closed_dispatch(state, "generated", "runlink-address-table",
                           target);
#endif
    if (state.last_pi_size != 0U) {
      std::array<char, 96U> operation{};
      (void)std::snprintf(operation.data(), operation.size(),
                          "unresolved-dma-%08x-%08x-%08x",
                          state.last_pi_rom, state.last_pi_dram,
                          state.last_pi_size);
      fail_closed_dispatch(state, "generated", operation.data(), target);
    }
    fail_closed_dispatch(state, "generated", "unresolved", target);
  }
  ++state.mapped_calls;
  ++state.mapped_call_counts[mapping_index];
  hle::GuestMemory memory({rdram, kRdramSize},
                          hle::GuestMemory::Layout::native_word_big_endian);
  const auto a0 = static_cast<std::uint32_t>(context->r4),
             a1 = static_cast<std::uint32_t>(context->r5),
             a2 = static_cast<std::uint32_t>(context->r6),
             a3 = static_cast<std::uint32_t>(context->r7);
  if (state.guest_os_probe) {
    // No HLE CPU shortcuts in this profile: every mapped OS call must have
    // an original, instrumented body. Missing device behavior fails closed.
    recomp_func_t *const generated = jfg_generated_lookup_function(address);
    if (generated == nullptr)
      fail_closed_dispatch(state, "guest-os", "original-body-unavailable", target);
    // Input is latched at the same SDK poll boundary as the recorded route.
    // Original code still builds/reads every PIF packet and delivers messages.
    if (target == original_os_entry("osContStartReadData")) {
      ++state.controller_read_start_calls;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      sample_live_controller(state);
#endif
      state.mmio_trace->si_dma.sample(state.controller_ports.samples);
    }
    if (target == original_os_entry("osContGetReadData"))
      ++state.controller_get_data_calls;
    if (target == original_os_entry("osSpTaskLoad")) {
      if (state.mmio_trace->sp_deadline || state.mmio_trace->sp_launch ||
          !memory.read_u32(a0, state.loaded_rsp_task_type))
        fail_closed_dispatch(state, "guest-rsp", "task-load-state", target);
      state.loaded_rsp_task = a0;
      ++state.sp_task_load_calls;
    }
    if (target == original_os_entry("osSpTaskStartGo")) ++state.sp_task_start_calls;
    if (target == original_os_entry("osRecvMesg")) ++state.queue_recv_calls;
    if (target == original_os_entry("osSendMesg")) ++state.queue_send_calls;
    if (state.guest_clock_trace.is_open() && state.observed_guest_instructions < 1000000U)
      state.guest_clock_trace << "call\t" << std::hex << target << '\t' << a0 << '\t'
          << a1 << '\t' << a2 << std::dec << '\t' << state.cpu_count << '\n';
    generated(rdram, context);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    if(a0==0x800F1AF4U && (target==original_os_entry("osSendMesg") ||
        target==original_os_entry("osRecvMesg"))) {
      static const bool observe=[] {
        char value[2]{};
        return GetEnvironmentVariableA("JFG_DIAGNOSTIC_AUDIO_SCHEDULER",value,2U)==1U && value[0]=='1';
      }();
      if(observe && state.gameplay_trace) {
        std::uint32_t queued=0;(void)memory.read_u32(a0+8U,queued);
        state.gameplay_trace.event("audio-notify",{state.vi_retraces,state.cpu_count,target,
            a0,a1,a2,static_cast<std::uint32_t>(context->r2),queued,state.guest_active_rsp_type});
      }
    }
#endif
    if (target == 0x80097520U) state.os_initialized = true;
    if (target == original_os_entry("osContInit"))
      state.controller_initialized = static_cast<std::uint32_t>(context->r2) == 0U;
    // Read-only host observers. The original bodies above own all guest
    // register, queue and thread mutations, including interrupt delivery.
    if (target == original_os_entry("osCreateMesgQueue")) {
      state.queues.insert(a0);
      ++state.queue_create_calls;
    }
    if (target == original_os_entry("osSetEventMesg")) state.events[a0] = {a1, a2};
    if (target == original_os_entry("osViSetEvent")) {
      state.vi_queue = a0; state.vi_message = a1; state.vi_retrace_interval = a2;
    }
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    if (target == original_os_entry("osViSetMode")) {
      if (!configure_live_vi_mode(state, memory, a0))
        fail_closed_dispatch(state, "guest-vi", "invalid-mode", target);
      state.vi_mode = a0;
    }
    if (target == original_os_entry("osViSetSpecialFeatures")) {
      state.vi_special_features = a0;
      state.rt64_vi.status = jfg::vi_apply_special_features(
          state.rt64_vi.status, state.vi_mode_control, a0);
    }
    if (target == original_os_entry("osViBlack")) {
      state.vi_blacked = a0 != 0;
      state.rt64_vi.horizontal_start = state.vi_blacked ? 0 : state.vi_mode_horizontal_start;
    }
#endif
    if (target == original_os_entry("osViSwapBuffer")) {
      ++state.vi_swap_calls;
      state.vi_current_framebuffer = state.vi_next_framebuffer;
      state.vi_next_framebuffer = a0;
    }
    if (target == original_os_entry("osSendMesg") && context->r2 == 0 &&
        a0 == state.vi_queue && a1 == state.vi_message) {
      ++state.vi_messages_delivered;
      present_completed_video(state,target);
    }
    if (target == original_os_entry("osRecvMesg") && context->r2 == 0 && a0 == state.vi_queue && a1 != 0) {
      std::uint32_t message = 0;
      if (!memory.read_u32(a1, message)) fail_closed_dispatch(state, "guest-vi", "message-unreadable", target);
      if (message == state.vi_message) {
        ++state.vi_retraces;
        jfg::support_breadcrumb(state.vi_frames, state.vi_retraces, state.controller_samples);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        write_retrace_semantic_hash(state);
        write_private_progress(state);
#endif
        if (state.vi_retraces >= state.retrace_target) retrace_gate(state);
      }
    }
    if (state.guest_clock_trace.is_open() && state.observed_guest_instructions < 1000000U) {
      state.guest_clock_trace << "return\t" << std::hex << target << std::dec << '\t'
          << state.cpu_count << '\n';
      state.guest_clock_trace.flush();
    }
    return 1;
  }
  if (state.guest_initializing) {
    // The independently executed cold-boot closure. Mapped private helpers
    // must execute their original bodies as well as the public OS entry.
    constexpr std::array<std::uint32_t, 15> helpers{
      0x80097788U,0x80098050U,0x800980d0U,0x80098360U,0x80099648U,
      0x80099748U,0x8009af40U,0x8009af50U,0x8009af60U,0x8009af70U,
      0x8009afc0U,0x8009b010U,0x8009b060U,0x8009b0c0U,0x8009cae0U};
    recomp_func_t *const generated = jfg_generated_lookup_function(address);
    if (!std::binary_search(helpers.begin(), helpers.end(), target) || generated == nullptr)
      fail_closed_dispatch(state, "guest-leaf", "initialize-closure", target);
    generated(rdram, context);
    return 1;
  }
  // These two leaf HLE calls account for most mapped dispatches in JFG. Use
  // their supported-ROM addresses before the descriptive name chain so normal
  // play does not repeat dozens of string comparisons for each call.
  if (target == 0x80096EA0U) {  // osVirtualToPhysical
    const std::uint32_t segment = a0 & 0xE0000000U;
    if (state.guest_leaf_probe) {
      // Execute the original supported-ROM routine, including its TLB
      // helper. The diagnostic CP0 bridge supplies explicit reference state;
      // unsupported operations still trap rather than bypassing the helper.
      recomp_func_t *const generated = jfg_generated_lookup_function(address);
      if (generated == nullptr)
        fail_closed_dispatch(state, "guest-leaf", "unavailable", target);
      trace_guest_clock(state, "guest-leaf-entry", target);
      generated(rdram, context);
      trace_guest_clock(state, "guest-leaf-return", target);
      if ((segment == 0x80000000U || segment == 0xA0000000U) &&
          static_cast<std::uint32_t>(context->r2) != (a0 & 0x1FFFFFFFU))
        fail_closed_dispatch(state, "guest-leaf", "translation-result", target);
      return 1;
    }
    const std::uint32_t physical =
        segment == 0x80000000U || segment == 0xA0000000U
            ? a0 & 0x1FFFFFFFU
            : a0;
    set_result(context, static_cast<std::int32_t>(physical));
    return 1;
  }
  if (target == 0x80098EA0U) {  // osSetIntMask
    if (state.guest_leaf_probe) {
      recomp_func_t *const generated = jfg_generated_lookup_function(address);
      if (generated == nullptr || state.mmio_trace == nullptr ||
          !state.mmio_trace->guest_mi_mask || !state.mmio_trace->mi_mask.initialized())
        fail_closed_dispatch(state, "guest-leaf", "interrupt-mask-unavailable", target);
      trace_guest_clock(state, "guest-mask-entry", target);
      generated(rdram, context);
      trace_guest_clock(state, "guest-mask-return", target);
      state.interrupt_mask =
          (static_cast<std::uint32_t>(cop0_status_read(context)) & 0xff01U) |
          (state.mmio_trace->mi_mask.read() << 16U);
      state.journal.append(NativeEventKind::kInterruptMask,
                           static_cast<std::uint32_t>(context->r2), state.interrupt_mask);
      return 1;
    }
    const std::uint32_t previous = state.interrupt_mask;
    state.interrupt_mask = a0;
    set_result(context, static_cast<std::int32_t>(previous));
    state.journal.append(NativeEventKind::kInterruptMask, previous, a0);
    return 1;
  }
  // osInitialize has no arguments or return value in the public libultra ABI.
  // The native runner has already established the memory image and generated
  // dispatch before guest entry, so its observable contract at this boundary
  // is to make initialization idempotent and allow execution to continue.
  if (std::strcmp(name, "osInitialize") == 0) {
    if (state.guest_leaf_probe) {
      recomp_func_t *const generated = jfg_generated_lookup_function(address);
      if (state.os_initialized || generated == nullptr || state.mmio_trace == nullptr ||
          !state.mmio_trace->guest_pif_boot)
        fail_closed_dispatch(state, "guest-leaf", "initialize-state", target);
      state.guest_initializing = true;
      generated(rdram, context);
      state.guest_initializing = false;
      if (!state.mmio_trace->pif_boot.acknowledged())
        fail_closed_dispatch(state, "guest-leaf", "initialize-pif", target);
    }
    state.os_initialized = true;
    return 1;
  }
  if (std::strcmp(name, "osCreateThread") == 0) {
    const std::uint32_t stack = static_cast<std::uint32_t>(context->r29);
    std::uint32_t guest_sp = 0U;
    std::uint32_t priority = 0U;
    if (!state.os_initialized || state.scheduler == nullptr ||
        !memory.read_u32(stack + 0x10U, guest_sp) ||
        !memory.read_u32(stack + 0x14U, priority) || a0 == 0U ||
        a2 == 0U || guest_sp < 16U || state.threads.contains(a0)) {
      fail_closed_dispatch(state, "hle", "malformed-thread", target);
    }
    recomp_func_t *const entry_function =
        jfg_generated_lookup_function(static_cast<std::int32_t>(a2));
    const bool synthetic_vi_manager =
        // The diagnostic inventory now contains this worker's original body.
        // Keep the cooperative control profile explicit, not dependent on a
        // missing symbol. The guest-OS scheduler will own its separate path.
        a2 == 0x80098bb8U && !state.vi_manager_thread_created &&
        state.events.contains(7U) && state.events.contains(3U);
    if (entry_function == nullptr && !synthetic_vi_manager) {
      fail_closed_dispatch(state, "generated", "thread-entry-unresolved", a2);
    }
    const std::uint32_t vi_internal_queue =
        synthetic_vi_manager ? state.events.at(7U).first : 0U;
    constexpr std::uint32_t kThreadStatus = 0x0400FF03U;
    const std::uint32_t thread_status =
        static_cast<std::int32_t>(a1) < 0
            ? kThreadStatus & ~0x04000000U
            : kThreadStatus;
    const int thread_id = state.scheduler->create_thread(
        priority, [&state, entry = a2, argument = a3,
                   guest_sp, entry_function,
                   vi_internal_queue,
                   thread_status, synthetic_vi_manager](ThreadScheduler &scheduler, int) {
          if (synthetic_vi_manager) {
            for (;;) {
              (void)scheduler.recv_blocking(vi_internal_queue, 0U);
              if (state.vi_queue == 0U ||
                  scheduler.send(state.vi_queue, state.vi_message, false) !=
                      hle::kOsSuccess) {
                fail_closed_dispatch(state, "vi", "retrace-delivery", entry);
              }
              ++state.vi_messages_delivered;
              state.journal.append(NativeEventKind::kViMessageDelivered,
                                   state.vi_queue, state.vi_message,
                                   static_cast<std::uint32_t>(
                                       state.vi_messages_delivered));
            }
          }
          recomp_context thread_context{};
          thread_context.r4 = sign_extend_mips32(argument);
          thread_context.r29 = sign_extend_mips32(guest_sp - 16U);
          // osCreateThread stores an exception-return context, not the live
          // entry Status. The native handoff replaces __osDispatchThread's
          // ERET, so apply its Status transition before running guest code.
          cop0_status_write(&thread_context, status_after_exception_return(thread_status));
#if defined(JFG_PHASE8_LIVE_RUNTIME)
          if (state.guest_leaf_probe && state.timing_trace.is_open())
            state.timing_trace << "thread-entry-status\t" << std::hex << entry << '\t'
                               << cop0_status_read(&thread_context) << '\t'
                               << state.mmio_trace->mi_mask.read() << std::dec << '\n';
#endif
          entry_function(state.rdram, &thread_context);
        });
    state.vi_manager_thread_created |= synthetic_vi_manager;
    if (state.guest_leaf_probe) {
      if (static_cast<std::size_t>(thread_id) != state.saved_rcp_masks.size())
        fail_closed_dispatch(state, "guest-leaf", "thread-mask-index", target);
      // Independent original-OS thread-entry probe: all six sources enabled.
      state.saved_rcp_masks.push_back(0x3fU);
    }
    state.threads.emplace(a0, thread_id);
    state.thread_order.emplace_back(a0, thread_id);
    state.thread_entries.push_back(a2);
    state.journal.append(NativeEventKind::kThreadCreated, a0, a2, priority);
    return 1;
  }
  if (std::strcmp(name, "osStartThread") == 0) {
    const auto thread = state.threads.find(a0);
    if (state.scheduler == nullptr || thread == state.threads.end()) {
      fail_closed_dispatch(state, "hle", "unknown-thread", target);
    }
    state.scheduler->start_thread(thread->second);
    state.journal.append(NativeEventKind::kThreadStarted, a0,
                         static_cast<std::uint32_t>(thread->second));
    if (state.scheduler_running) {
      state.scheduler->yield_current();
    } else {
      state.scheduler_running = true;
      auto outcome = state.scheduler->run(1'000'000U);
      while ((outcome == ScheduleOutcome::kQuiescent ||
              outcome == ScheduleOutcome::kTimeslice) &&
             state.vi_retraces < state.retrace_target &&
             state.vi_retrace_interval != 0U &&
             state.vi_internal_queue != 0U) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (state.focus_update_start != 0U &&
            state.completed_game_updates + 1U >= state.focus_update_start - 1U &&
            state.completed_game_updates + 1U <= state.focus_update_end + 1U &&
            state.timing_trace.is_open()) {
          std::string blocked_threads;
          for (const auto &[guest, id] : state.thread_order) {
            (void)guest;
            const auto snapshot = state.scheduler->snapshot_of(id);
            if (!snapshot.has_value() ||
                snapshot->state != ThreadState::kBlocked)
              continue;
            if (!blocked_threads.empty())
              blocked_threads.push_back(',');
            char queue[11U]{};
            (void)std::snprintf(queue, sizeof(queue), "%08x",
                                snapshot->blocked_queue);
            blocked_threads += std::to_string(id) + "@" + queue;
          }
          std::vector<std::pair<std::uint32_t, std::uint32_t>> mapped_events;
          mapped_events.reserve(state.events.size());
          for (const auto &[event, queue_and_message] : state.events) {
            mapped_events.emplace_back(event, queue_and_message.first);
          }
          std::sort(mapped_events.begin(), mapped_events.end());
          std::string event_queues;
          for (const auto &[event, queue_address] : mapped_events) {
            if (!event_queues.empty())
              event_queues.push_back(',');
            char queue[11U]{};
            (void)std::snprintf(queue, sizeof(queue), "%08x",
                                queue_address);
            event_queues += std::to_string(event) + "@" + queue;
          }
          state.timing_trace << "vi-service\t" << state.vi_frames << '\t'
                             << state.vi_retraces << "\t0\t"
                             << (outcome == ScheduleOutcome::kTimeslice
                                     ? "timeslice"
                                     : "quiescent")
                             << '\t' << state.completed_game_updates << '\t'
                             << state.interrupt_timeslices << '\t'
                             << state.pending_graphics_tasks.size() << ':'
                             << state.pending_si_completions << ':'
                             << state.scheduler->dispatch_count() << ':'
                             << blocked_threads << ':' << event_queues << '\n';
        }
#endif
        service_vi_frame(state, target);
        outcome = state.scheduler->run(1'000'000U);
      }
      state.scheduler_running = false;
      if (outcome == ScheduleOutcome::kUnbounded)
        fail_closed_dispatch(state, "scheduler", "step-budget", target);
    }
    return 1;
  }
  if (std::strcmp(name, "osSetThreadPri") == 0) {
    if (a0 == 0U) {
      const int current = state.scheduler == nullptr
                              ? -1
                              : state.scheduler->current_thread_id();
      if (state.scheduler_running && current >= 0) {
        if (!state.scheduler->set_priority(current, a1)) {
          fail_closed_dispatch(state, "hle", "unknown-thread", target);
        }
        state.scheduler->yield_current();
      } else {
        state.root_priority = a1;
      }
      return 1;
    }
    const auto thread = state.threads.find(a0);
    if (state.scheduler == nullptr || thread == state.threads.end() ||
        !state.scheduler->set_priority(thread->second, a1)) {
      fail_closed_dispatch(state, "hle", "unknown-thread", target);
    }
    return 1;
  }
  if (std::strcmp(name, "osGetThreadPri") == 0) {
    if (a0 == 0U) {
      const int current = state.scheduler == nullptr
                              ? -1
                              : state.scheduler->current_thread_id();
      const auto priority =
          state.scheduler_running && current >= 0
              ? state.scheduler->priority_of(current)
              : std::optional<std::uint32_t>{state.root_priority};
      set_result(context, static_cast<std::int32_t>(*priority));
      return 1;
    }
    const auto thread = state.threads.find(a0);
    const auto priority =
        thread == state.threads.end() || state.scheduler == nullptr
            ? std::optional<std::uint32_t>{}
            : state.scheduler->priority_of(thread->second);
    if (!priority.has_value()) {
      fail_closed_dispatch(state, "hle", "unknown-thread", target);
    }
    set_result(context, static_cast<std::int32_t>(*priority));
    return 1;
  }
  if (std::strcmp(name, "osSetEventMesg") == 0) {
    if (!state.queues.contains(a1)) {
      fail_closed_dispatch(state, "hle", "unknown-queue", target);
    }
    state.events[a0] = {a1, a2};
    if (a0 == 7U) {
      state.vi_internal_queue = a1;
      state.vi_internal_message = a2;
    }
    return 1;
  }
  if (std::strcmp(name, "osCreatePiManager") == 0) {
    if (state.pi_command_queue != 0U) {
      set_result(context, static_cast<std::int32_t>(state.pi_command_queue));
      return 1;
    }
    if (state.scheduler == nullptr || a1 == 0U || a2 == 0U || a3 == 0U ||
        !state.scheduler->create_queue(a1, a2,
                                       static_cast<std::int32_t>(a3))) {
      fail_closed_dispatch(state, "hle", "pi-manager-setup", target);
    }
    state.queues.insert(a1);
    state.pi_command_queue = a1;
    state.journal.append(NativeEventKind::kQueueCreated, a1, a2, a3);
    set_result(context, static_cast<std::int32_t>(a1));
    return 1;
  }
  if (std::strcmp(name, "osPiGetCmdQueue") == 0) {
    if (state.pi_command_queue == 0U)
      fail_closed_dispatch(state, "hle", "pi-manager-uninitialized", target);
    set_result(context,
               static_cast<std::int32_t>(state.pi_command_queue));
    return 1;
  }
  if (std::strcmp(name, "osPiStartDma") == 0) {
    const std::uint32_t stack = static_cast<std::uint32_t>(context->r29);
    std::uint32_t dram_address = 0U;
    std::uint32_t size = 0U;
    std::uint32_t completion_queue = 0U;
    if (state.rom == nullptr || state.scheduler == nullptr || a0 == 0U ||
        a2 != 0U || !memory.read_u32(stack + 0x10U, dram_address) ||
        !memory.read_u32(stack + 0x14U, size) ||
        !memory.read_u32(stack + 0x18U, completion_queue) || size == 0U ||
        !state.queues.contains(completion_queue)) {
      fail_closed_dispatch(state, "hle", "invalid-pi-dma", target);
    }
    const std::uint32_t segment = dram_address & 0xE0000000U;
    const std::uint64_t dram_offset = dram_address & 0x1FFFFFFFU;
    const std::uint64_t rom_offset =
        a3 >= 0x10000000U ? std::uint64_t{a3} - 0x10000000U : a3;
    if ((segment != 0x80000000U && segment != 0xA0000000U) ||
        dram_offset > kRdramSize || size > kRdramSize - dram_offset ||
        rom_offset > state.rom_size || size > state.rom_size - rom_offset) {
      fail_closed_dispatch(state, "hle", "pi-dma-range", target);
    }
    for (std::uint32_t index = 0U; index < size; ++index) {
      state.rdram[(static_cast<std::size_t>(dram_offset) + index) ^ 3U] =
          state.rom[static_cast<std::size_t>(rom_offset) + index];
    }
    for (std::uint32_t section = 0U;
         section < jfg_generated_section_count(); ++section) {
      JfgGeneratedSectionMetadata metadata{};
      if (jfg_generated_section_metadata(section, &metadata) == 0 ||
          metadata.is_overlay != 1U || metadata.rom_start != rom_offset)
        continue;
      // A runlink module may load text and data with separate PI requests.
      // Matching the module's exact ROM start is the publication boundary;
      // publish_synthetic_overlay independently validates and materializes the
      // complete normalized image from the supported ROM.
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      SyntheticOverlayPublication publication =
          SyntheticOverlayPublication::ensure_active;
      if (section == 6U &&
          state.phase9_death_restart_calls >
              state.health_overlay_death_reload_generation &&
          state.active_overlay_sections.contains(section)) {
        // The same ROM range can be read for reasons other than loading its
        // overlay. Reload mutable data/BSS only when the DMA destination is
        // the base of the matching live runlink module. Treating every ROM
        // read as an unload/reload resets instrument state continuously and
        // can prevent the scene framebuffer from ever becoming presentable.
        RunlinkAddressResolution resolution{};
        const std::uint32_t dma_guest_address =
            kKseg0 | static_cast<std::uint32_t>(dram_offset);
        if (resolve_runlink_text_address(
                memory, kRunlinkModuleTablePointer,
                kRunlinkOverlaySlotCount, dma_guest_address,
                resolution) == RunlinkAddressResult::resolved &&
            resolution.text_offset == 0U &&
            resolution.identity.rom_start == metadata.rom_start &&
            resolution.identity.text_size == metadata.text_size &&
            resolution.identity.data_size == metadata.data_size &&
            resolution.identity.bss_size == metadata.bss_size)
          publication = SyntheticOverlayPublication::reload;
      }
#endif
      if (
#if defined(JFG_PHASE8_LIVE_RUNTIME)
          !publish_synthetic_overlay(state, section, metadata, publication)
#else
          jfg_generated_relocation_count(section) != 0U ||
          jfg_generated_section_lifecycle(
              1U, section, static_cast<std::int32_t>(metadata.linked_vram)) !=
              0
#endif
      ) {
        fail_closed_overlay_publication(state, metadata.linked_vram);
      }
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      if (publication == SyntheticOverlayPublication::reload)
        state.health_overlay_death_reload_generation =
            state.phase9_death_restart_calls;
#endif
      state.journal.append(NativeEventKind::kOverlayPublished, section,
                           metadata.linked_vram,
                           static_cast<std::uint32_t>(dram_offset));
      break;
    }
    if (!memory.write_u32(a0 + 0x00U, completion_queue) ||
        !memory.write_u32(a0 + 0x04U, a1) ||
        !memory.write_u32(a0 + 0x08U, 0U) ||
        !memory.write_u32(a0 + 0x0CU, dram_address) ||
        !memory.write_u32(a0 + 0x10U, a3) ||
        !memory.write_u32(a0 + 0x14U, size) ||
        state.scheduler->send(completion_queue, a0, false) !=
            hle::kOsSuccess) {
      fail_closed_dispatch(state, "hle", "pi-dma-completion", target);
    }
    state.journal.append(NativeEventKind::kPiDma, a3, dram_address, size);
    state.last_pi_rom = a3;
    state.last_pi_dram = dram_address;
    state.last_pi_size = size;
    set_result(context, hle::kOsSuccess);
    return 1;
  }
  if (std::strcmp(name, "osCic6105SendData") == 0 ||
      std::strcmp(name, "osCic6105StartGetData") == 0) {
    recomp_func_t *const generated =
        std::strcmp(name, "osCic6105SendData") == 0 ? fn_044_0000
                                                     : fn_044_0001;
    generated(rdram, context);
    return 1;
  }
  if (std::strcmp(name, "__osSiRawStartDma") == 0) {
    if (state.si_count_probe && state.si_deadline.next()) {
      set_result(context, -1);
      return 1;
    }
    recomp_func_t *const generated = jfg_generated_lookup_function(address);
    if (generated == nullptr)
      fail_closed_dispatch(state, "si", "raw-dma-unavailable", target);
    generated(rdram, context);
    if (static_cast<std::uint32_t>(context->r2) == 0U) {
      if (a0 > 1U)
        fail_closed_dispatch(state, "si", "raw-dma-direction", target);
      if (a0 == 1U) {
        std::array<std::uint8_t, jfg::kPifRamBytes> transfer{};
        for (std::size_t index = 0U; index < transfer.size(); ++index) {
          std::size_t guest_offset = 0U;
          if (!guest_byte_offset(
                  a1 + static_cast<std::uint32_t>(index), guest_offset)) {
            fail_closed_dispatch(state, "si", "raw-dma-range", target);
          }
          transfer[index] = state.rdram[guest_offset];
        }
        // The high-level controller path owns ordinary PIF channel traffic.
        // Retain only a real CIC challenge here; mirroring unimplemented
        // controller commands back into RDRAM would overwrite the controller
        // data produced by that HLE path.
        if (jfg::process_cic_nus_6105_challenge(transfer) ||
            (state.controller_guest_init_probe &&
             jfg::process_controller_status_query(transfer, state.controller_ports.mask()))) {
          state.pif_ram = transfer;
          state.cic_response_pending = true;
        }
      } else if (state.cic_response_pending) {
        for (std::size_t index = 0U; index < state.pif_ram.size(); ++index) {
          std::size_t guest_offset = 0U;
          if (!guest_byte_offset(
                  a1 + static_cast<std::uint32_t>(index), guest_offset)) {
            fail_closed_dispatch(state, "si", "raw-dma-range", target);
          }
          state.rdram[guest_offset] = state.pif_ram[index];
        }
        state.cic_response_pending = false;
      }
      if (state.pending_si_completions ==
          (std::numeric_limits<std::uint32_t>::max)())
        fail_closed_dispatch(state, "si", "completion-overflow", target);
      ++state.pending_si_completions;
      if (state.si_count_probe) schedule_si_deadline(state, target);
    }
    return 1;
  }
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  if (std::strcmp(name, "osSetTimer") == 0) {
    const auto sp = static_cast<std::uint32_t>(context->r29);
    std::uint32_t interval_hi = 0, interval_lo = 0, queue = 0, message = 0;
    if (sp > UINT32_MAX - 0x1CU ||
        !memory.read_u32(sp + 0x10U, interval_hi) ||
        !memory.read_u32(sp + 0x14U, interval_lo) ||
        !memory.read_u32(sp + 0x18U, queue) ||
        !memory.read_u32(sp + 0x1CU, message) ||
        !state.timers.set(memory, state.cpu_count, a0,
                         (std::uint64_t{a2} << 32U) | a3,
                         (std::uint64_t{interval_hi} << 32U) | interval_lo,
                         queue, message))
      fail_closed_dispatch(state, "timer", "invalid-timer-arguments", target);
    set_result(context, hle::kOsSuccess);
    return 1;
  }
  if (std::strcmp(name, "osStopTimer") == 0) {
    state.timers.stop(a0);
    return 1;
  }
  if (std::strcmp(name, "osGetCount") == 0) {
    set_result(context, static_cast<std::int32_t>(state.cpu_count));
    return 1;
  }
  if (std::strcmp(name, "osSetTime") == 0) {
    state.os_time_base = (std::uint64_t{a0} << 32U) | a1;
    state.os_time_count_origin = state.cpu_count;
    return 1;
  }
  if (std::strcmp(name, "osGetTime") == 0) {
    const std::uint64_t value =
        state.os_time_base + (state.cpu_count - state.os_time_count_origin);
    context->r2 = sign_extend_mips32(static_cast<std::uint32_t>(value >> 32U));
    context->r3 = sign_extend_mips32(static_cast<std::uint32_t>(value));
    return 1;
  }
  if (std::strcmp(name, "osFlashInit") == 0 ||
      std::strcmp(name, "osFlashReInit") == 0) {
    set_result(context, static_cast<std::int32_t>(0x80104EE0U));
    return 1;
  }
  if (std::strcmp(name, "osFlashReadStatus") == 0) {
    if (!write_guest_byte(state, a0, 0U))
      fail_closed_dispatch(state, "flashram", "status-output", target);
    set_result(context, hle::kOsSuccess);
    return 1;
  }
  if (std::strcmp(name, "osFlashReadId") == 0) {
    if (!memory.write_u32(a0, 0x11118001U) ||
        !memory.write_u32(a1, 0x00C2001EU))
      fail_closed_dispatch(state, "flashram", "id-output", target);
    set_result(context, hle::kOsSuccess);
    return 1;
  }
  if (std::strcmp(name, "osFlashClearStatus") == 0) {
    set_result(context, hle::kOsSuccess);
    return 1;
  }
  if (std::strcmp(name, "osFlashAllErase") == 0) {
    if (!state.flashram.begin_erase_all().ok() ||
        !complete_flash_operation(state)) {
      set_result(context, -1);
      return 1;
    }
    set_result(context, hle::kOsSuccess);
    return 1;
  }
  if (std::strcmp(name, "osFlashSectorErase") == 0) {
    if (!state.flashram.begin_erase_page(a0).ok() ||
        !complete_flash_operation(state)) {
      set_result(context, -1);
      return 1;
    }
    set_result(context, hle::kOsSuccess);
    return 1;
  }
  if (std::strcmp(name, "osFlashWriteBuffer") == 0) {
    std::array<std::uint8_t, jfg::kFlashRamPageBytes> page{};
    const std::uint32_t physical = a2 & 0x1FFFFFFFU;
    if (state.scheduler == nullptr || !state.queues.contains(a3) ||
        physical > kRdramSize || page.size() > kRdramSize - physical)
      fail_closed_dispatch(state, "flashram", "write-buffer-arguments", target);
    for (std::size_t index = 0U; index < page.size(); ++index)
      page[index] = state.rdram[(physical + index) ^ 3U];
    if (!state.flashram.stage_write(page).ok() ||
        state.scheduler->send(a3, 0U, false) != hle::kOsSuccess)
      fail_closed_dispatch(state, "flashram", "write-buffer-completion", target);
    set_result(context, hle::kOsSuccess);
    if (state.scheduler_running)
      state.scheduler->yield_current();
    return 1;
  }
  if (std::strcmp(name, "osFlashWriteArray") == 0) {
    if (!state.flashram.begin_program(a0).ok() ||
        !complete_flash_operation(state)) {
      set_result(context, -1);
      return 1;
    }
    set_result(context, hle::kOsSuccess);
    return 1;
  }
  if (std::strcmp(name, "osFlashReadArray") == 0) {
    const std::uint32_t stack = static_cast<std::uint32_t>(context->r29);
    std::uint32_t page_count = 0U, completion_queue = 0U;
    const std::uint64_t flash_offset =
        std::uint64_t{a2} * jfg::kFlashRamPageBytes;
    const std::uint32_t physical = a3 & 0x1FFFFFFFU;
    if (state.scheduler == nullptr ||
        !memory.read_u32(stack + 0x10U, page_count) ||
        !memory.read_u32(stack + 0x14U, completion_queue) || page_count == 0U ||
        !state.queues.contains(completion_queue))
      fail_closed_dispatch(state, "flashram", "read-array-arguments", target);
    const std::uint64_t byte_count =
        std::uint64_t{page_count} * jfg::kFlashRamPageBytes;
    if (flash_offset > jfg::kFlashRamSizeBytes ||
        byte_count > jfg::kFlashRamSizeBytes - flash_offset ||
        physical > kRdramSize || byte_count > kRdramSize - physical)
      fail_closed_dispatch(state, "flashram", "read-array-range", target);
    const auto read = state.flashram.read(static_cast<std::size_t>(flash_offset),
                                          static_cast<std::size_t>(byte_count));
    if (!read.ok())
      fail_closed_dispatch(state, "flashram", "read-array-device", target);
    for (std::size_t index = 0U; index < read.bytes.size(); ++index)
      state.rdram[(physical + index) ^ 3U] = read.bytes[index];
    if (state.scheduler->send(completion_queue, 0U, false) != hle::kOsSuccess)
      fail_closed_dispatch(state, "flashram", "read-array-completion", target);
    set_result(context, hle::kOsSuccess);
    if (state.scheduler_running)
      state.scheduler->yield_current();
    return 1;
  }
#endif
  if (std::strcmp(name, "osContInit") == 0) {
#if defined(_WIN32)
    char probe_value[2]{};
    const bool guest_init_probe = GetEnvironmentVariableA(
        "JFG_PHASE9_CONTROLLER_GUEST_INIT", probe_value, sizeof(probe_value)) == 1 &&
        probe_value[0] == '1';
#else
    const char *probe_value = std::getenv("JFG_PHASE9_CONTROLLER_GUEST_INIT");
    const bool guest_init_probe = probe_value != nullptr &&
                                  std::strcmp(probe_value, "1") == 0;
#endif
    if (guest_init_probe) {
#if defined(_WIN32)
      char si_probe_value[2]{};
      state.si_count_probe = GetEnvironmentVariableA("JFG_PHASE9_SI_COUNT_PROBE",
          si_probe_value, sizeof(si_probe_value)) == 1 && si_probe_value[0] == '1';
#else
      const char *si_probe_value = std::getenv("JFG_PHASE9_SI_COUNT_PROBE");
      state.si_count_probe = si_probe_value != nullptr && std::strcmp(si_probe_value, "1") == 0;
#endif
      // Bounded diagnostic candidate: original code establishes the guest
      // channel count, command state, access queues, and initialization guard.
      state.controller_guest_init_probe = true;
      recomp_func_t *const generated = jfg_generated_lookup_function(address);
      if (generated == nullptr)
        fail_closed_dispatch(state, "si", "controller-init-unavailable", target);
      generated(rdram, context);
      state.controller_initialized = static_cast<std::uint32_t>(context->r2) == 0U;
      return 1;
    }
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    sample_controller_ports(state);
    const auto connected_mask=state.input_replay_loaded ? std::uint8_t{1} : state.controller_ports.mask();
#else
    const std::uint8_t connected_mask=1;
#endif
    if (state.scheduler == nullptr || !state.queues.contains(a0) || a1 == 0U ||
        a2 == 0U || !write_guest_byte(state, a1, connected_mask)) {
      fail_closed_dispatch(state, "hle", "controller-init", target);
    }
    for (std::uint32_t controller = 0U; controller < 4U; ++controller) {
      const std::uint32_t status = a2 + controller * 4U;
      const bool connected = (connected_mask & (1U<<controller))!=0;
      if (!write_guest_byte(state, status, 0U) ||
          !write_guest_byte(state, status + 1U, connected ? 5U : 0U) ||
          !write_guest_byte(state, status + 2U, 0U) ||
          !write_guest_byte(state, status + 3U, connected ? 0U : 8U)) {
        fail_closed_dispatch(state, "hle", "controller-status", target);
      }
    }
    state.controller_initialized = true;
    state.journal.append(NativeEventKind::kController, 1U, a0);
    set_result(context, hle::kOsSuccess);
    return 1;
  }
  if (std::strcmp(name, "osContStartReadData") == 0) {
    if (state.si_count_probe && state.si_deadline.next()) {
      set_result(context, -1);
      return 1;
    }
    ++state.controller_read_start_calls;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    trace_phase9_event(state, "start-entry", state.controller_samples);
#endif
    state.controller_read_thread = state.scheduler == nullptr
                                       ? -1
                                       : state.scheduler->current_thread_id();
    const auto si = state.events.find(5U);
    if (!state.controller_initialized || state.scheduler == nullptr ||
        !state.queues.contains(a0) || si == state.events.end() ||
        si->second.first != a0 ||
        state.pending_si_completions ==
            (std::numeric_limits<std::uint32_t>::max)()) {
      fail_closed_dispatch(state, "hle", "controller-read-start", target);
    }
    ++state.pending_si_completions;
    if (state.si_count_probe) schedule_si_deadline(state, target);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    trace_si_timing(state, "read-start", a0, 0U, 0);
    if (state.play_mode || state.input_replay_loaded) {
      std::size_t pressed_high = 0U, pressed_low = 0U;
      std::size_t current_stick_x = 0U, current_stick_y = 0U;
      std::size_t menu_stick_x = 0U, menu_stick_y = 0U;
      std::uint16_t game_pressed = 0U;
      if (!guest_byte_offset(0x800FB0F0U, pressed_high) ||
          !guest_byte_offset(0x800FB0F1U, pressed_low) ||
          !guest_byte_offset(0x800FB0C2U, current_stick_x) ||
          !guest_byte_offset(0x800FB0C3U, current_stick_y) ||
          !guest_byte_offset(0x800FF3D8U, menu_stick_x) ||
          !guest_byte_offset(0x800FF3DCU, menu_stick_y))
        fail_closed_dispatch(state, "controller", "pressed-observation",
                             target);
      game_pressed = static_cast<std::uint16_t>(
          (static_cast<std::uint16_t>(state.rdram[pressed_high]) << 8U) |
          state.rdram[pressed_low]);
      if (game_pressed != 0U) {
        ++state.game_pressed_observations;
        state.last_game_pressed_buttons = game_pressed;
      }
      const auto game_stick_x =
          static_cast<std::int8_t>(state.rdram[current_stick_x]);
      const auto game_stick_y =
          static_cast<std::int8_t>(state.rdram[current_stick_y]);
      if (game_stick_x != 0 || game_stick_y != 0) {
        ++state.game_stick_observations;
        state.last_game_stick_x = game_stick_x;
        state.last_game_stick_y = game_stick_y;
      }
      const auto observed_menu_stick_x =
          static_cast<std::int8_t>(state.rdram[menu_stick_x]);
      const auto observed_menu_stick_y =
          static_cast<std::int8_t>(state.rdram[menu_stick_y]);
      if (observed_menu_stick_x != 0 || observed_menu_stick_y != 0) {
        ++state.menu_stick_observations;
        state.last_menu_stick_x = observed_menu_stick_x;
        state.last_menu_stick_y = observed_menu_stick_y;
      }
      sample_live_controller(state);
      if (state.poll_hash_trace.is_open())
        write_retrace_semantic_hash(state, nullptr, &state.poll_hash_trace);
      trace_phase9_event(state, "start-exit", state.controller_samples - 1U);
    }
#endif
    state.journal.append(NativeEventKind::kController, 2U, a0,
                         si->second.second);
    set_result(context, hle::kOsSuccess);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    // Stop after the Nth controller-read HLE is complete, before the guest
    // consumes its result. This is an input-poll boundary, not a VI boundary.
    if (state.poll_target != 0U &&
        state.controller_samples == state.poll_target)
      probe_target_gate(state);
#endif
    return 1;
  }
  if (std::strcmp(name, "osContGetReadData") == 0) {
    ++state.controller_get_data_calls;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    trace_phase9_event(state, "get-entry", state.controller_samples == 0U
                                             ? 0U : state.controller_samples - 1U);
#endif
    if (!state.controller_initialized || a0 == 0U) {
      fail_closed_dispatch(state, "hle", "controller-read-data", target);
    }
    for (std::uint32_t controller = 0U; controller < 4U; ++controller) {
      for (std::uint32_t byte = 0U; byte < 6U; ++byte) {
        std::uint8_t value = byte == 4U && controller != 0U ? 8U : 0U;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
        if (state.play_mode || state.input_replay_loaded) {
          const auto& port=state.controller_ports.samples[controller];
          if (!port.connected) {
            value = byte == 4U ? 8U : 0U;
          } else {
          switch (byte) {
          case 0U:
            value = static_cast<std::uint8_t>(
                port.buttons >> 8U);
            break;
          case 1U:
            value = static_cast<std::uint8_t>(
                port.buttons);
            break;
          case 2U:
            value = static_cast<std::uint8_t>(
                port.x);
            break;
          case 3U:
            value = static_cast<std::uint8_t>(
                port.y);
            break;
          default:
            value = 0U;
            break;
          }
          if (byte == 0U && port.buttons != 0U)
            ++state.non_neutral_controller_writes;
          }
        }
#endif
        if (!write_guest_byte(state, a0 + controller * 6U + byte, value)) {
          fail_closed_dispatch(state, "hle", "controller-pad-range", target);
        }
      }
    }
    state.journal.append(NativeEventKind::kController, 3U, a0);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    trace_phase9_event(state, "get-exit", state.controller_samples == 0U
                                            ? 0U : state.controller_samples - 1U);
    if (state.controller_return_trace.is_open()) {
      const std::uint64_t poll = state.controller_samples == 0U
                                     ? 0U : state.controller_samples - 1U;
      bool selected = false;
      for (std::size_t index = 0U; index < state.event_range_count; ++index) {
        const auto [first, last] = state.event_ranges[index];
        selected |= poll >= first && poll <= last;
      }
      if (selected) {
        if (++state.controller_return_rows > 256U)
          fail_closed_dispatch(state, "diagnostic", "controller-return-overflow",
                               target);
        state.controller_return_trace << state.controller_return_rows << '\t'
            << poll << '\t' << state.completed_game_updates << '\t'
            << state.vi_retraces << '\t' << state.vi_frames << '\t'
            << "0x" << std::hex << std::setw(8) << std::setfill('0') << a0
            << '\t';
        for (std::uint32_t byte = 0U; byte < 24U; ++byte) {
          std::size_t offset = 0U;
          if (!guest_byte_offset(a0 + byte, offset))
            fail_closed_dispatch(state, "diagnostic", "controller-return-range",
                                 target);
          state.controller_return_trace << std::setw(2)
                                        << static_cast<unsigned>(state.rdram[offset]);
        }
        state.controller_return_trace << std::setfill(' ') << std::dec << '\n';
      }
    }
#endif
    return 1;
  }
  if (std::strcmp(name, "bzero") == 0) {
    const auto length = static_cast<std::int32_t>(a1);
    const auto guest_bzero = [&]() {
      if (static_cast<std::int64_t>(context->r5) != static_cast<std::int64_t>(length))
        fail_closed_dispatch(state, "guest-leaf", "bzero-length-extension", target);
      recomp_func_t *const generated = jfg_generated_lookup_function(address);
      if (generated == nullptr)
        fail_closed_dispatch(state, "guest-leaf", "bzero-unavailable", target);
      trace_guest_clock(state, "guest-memory-entry", target);
      generated(rdram, context);
      trace_guest_clock(state, "guest-memory-return", target);
    };
    if (length <= 0) {
      if (state.guest_leaf_probe) guest_bzero();
      return 1;
    }
    state.active_bzero_address = a0;
    state.active_bzero_length = a1;
    std::size_t offset = 0U;
    if (!resolve_bzero_host_offset(state, a0,
                                   static_cast<std::uint32_t>(length),
                                   offset)) {
      const std::uint32_t caller =
          state.recent_dispatch_position < 2U
              ? 0U
              : state.recent_dispatches[
                    (state.recent_dispatch_position - 2U) %
                    state.recent_dispatches.size()];
      const std::uint32_t parent =
          state.recent_dispatch_position < 3U
              ? 0U
              : state.recent_dispatches[
                    (state.recent_dispatch_position - 3U) %
                    state.recent_dispatches.size()];
      std::array<char, 128U> operation{};
      (void)std::snprintf(operation.data(), operation.size(),
                          "bzero-%08x-%08x-ra%08x-sp%08x-c%08x-p%08x", a0,
                          a1,
                          static_cast<std::uint32_t>(context->r31),
                          static_cast<std::uint32_t>(context->r29), caller,
                          parent);
      fail_closed_dispatch(state, "memory", operation.data(), target);
    }
    if (state.guest_leaf_probe) {
      guest_bzero();
    } else {
      for (std::uint32_t index = 0U;
           index < static_cast<std::uint32_t>(length); ++index) {
        state.rdram[(offset + index) ^ 3U] = 0U;
      }
    }
    state.active_bzero_address = 0U;
    state.active_bzero_length = 0U;
    return 1;
  }
  if (std::strcmp(name, "osVirtualToPhysical") == 0) {
    const std::uint32_t segment = a0 & 0xE0000000U;
    const std::uint32_t physical =
        segment == 0x80000000U || segment == 0xA0000000U
            ? a0 & 0x1FFFFFFFU
            : a0;
    set_result(context, static_cast<std::int32_t>(physical));
    return 1;
  }
  if (std::strcmp(name, "osSetIntMask") == 0) {
    const std::uint32_t previous = state.interrupt_mask;
    state.interrupt_mask = a0;
    set_result(context, static_cast<std::int32_t>(previous));
    state.journal.append(NativeEventKind::kInterruptMask, previous, a0);
    return 1;
  }
  if (std::strcmp(name, "osWritebackDCacheAll") == 0) {
    state.journal.append(NativeEventKind::kCacheMaintenance, 1U);
    if (state.guest_leaf_probe) {
      recomp_func_t *const generated = jfg_generated_lookup_function(address);
      if (generated == nullptr)
        fail_closed_dispatch(state, "guest-leaf", "cache-unavailable", target);
      trace_guest_clock(state, "guest-cache-entry", target);
      generated(rdram, context);
      trace_guest_clock(state, "guest-cache-return", target);
    }
    return 1;
  }
  if (std::strcmp(name, "osWritebackDCache") == 0 ||
      std::strcmp(name, "osInvalDCache") == 0 ||
      std::strcmp(name, "osInvalICache") == 0) {
    const auto length = static_cast<std::int32_t>(a1);
    if (length > 0) {
      const std::uint32_t segment = a0 & 0xE0000000U;
      if ((segment != 0x80000000U && segment != 0xA0000000U) ||
          (a0 & 0x1FFFFFFFU) >= kRdramSize ||
          static_cast<std::uint64_t>(a0 & 0x1FFFFFFFU) +
                  static_cast<std::uint32_t>(length) >
              kRdramSize) {
        fail_closed_dispatch(state, "hle", "invalid-cache-range", target);
      }
    }
    const std::uint32_t operation =
        std::strcmp(name, "osWritebackDCache") == 0 ? 2U
        : std::strcmp(name, "osInvalDCache") == 0   ? 3U
                                                      : 4U;
    state.journal.append(NativeEventKind::kCacheMaintenance, operation, a0,
                         a1);
    if (state.guest_leaf_probe) {
      if (static_cast<std::int64_t>(context->r5) != static_cast<std::int64_t>(length))
        fail_closed_dispatch(state, "guest-leaf", "cache-length-extension", target);
      recomp_func_t *const generated = jfg_generated_lookup_function(address);
      if (generated == nullptr)
        fail_closed_dispatch(state, "guest-leaf", "cache-unavailable", target);
      trace_guest_clock(state, "guest-cache-entry", target);
      generated(rdram, context);
      trace_guest_clock(state, "guest-cache-return", target);
    }
    return 1;
  }
  if (std::strcmp(name, "osViSetMode") == 0) {
    if (a0 < kKseg0 || a0 - kKseg0 > kRdramSize - 0x50U) {
      fail_closed_dispatch(state, "hle", "invalid-vi-mode", target);
    }
#if defined(JFG_PHASE8_LIVE_RUNTIME)
    if (!configure_live_vi_mode(state, memory, a0))
      fail_closed_dispatch(state, "hle", "invalid-vi-mode", target);
#endif
    state.vi_mode = a0;
    return 1;
  }
  if (std::strcmp(name, "osViBlack") == 0) {
    state.vi_blacked = a0 != 0U;
#if defined(JFG_PHASE8_LIVE_RUNTIME)
    state.rt64_vi.horizontal_start =
        state.vi_blacked ? 0U : state.vi_mode_horizontal_start;
#endif
    return 1;
  }
  if (std::strcmp(name, "osViSetEvent") == 0) {
    if (!state.queues.contains(a0) || a2 == 0U) {
      fail_closed_dispatch(state, "hle", "invalid-vi-event", target);
    }
    state.vi_queue = a0;
    state.vi_message = a1;
    state.vi_retrace_interval = a2;
    state.journal.append(NativeEventKind::kViConfigured, a0, a1, a2);
    return 1;
  }
  if (std::strcmp(name, "osViSwapBuffer") == 0) {
    ++state.vi_swap_calls;
    if (a0 < kKseg0 || a0 - kKseg0 >= kRdramSize) {
      fail_closed_dispatch(state, "hle", "invalid-framebuffer", target);
    }
    state.vi_current_framebuffer = state.vi_next_framebuffer;
    state.vi_next_framebuffer = a0;
    return 1;
  }
  if (std::strcmp(name, "osViGetCurrentFramebuffer") == 0) {
    set_result(context,
               static_cast<std::int32_t>(state.vi_current_framebuffer));
    return 1;
  }
  if (std::strcmp(name, "osViGetNextFramebuffer") == 0) {
    set_result(context, static_cast<std::int32_t>(state.vi_next_framebuffer));
    return 1;
  }
  if (std::strcmp(name, "osViSetSpecialFeatures") == 0) {
    state.vi_special_features = a0;
#if defined(JFG_PHASE8_LIVE_RUNTIME)
    state.rt64_vi.status = jfg::vi_apply_special_features(
        state.rt64_vi.status, state.vi_mode_control, a0);
    state.gameplay_trace.event("vi-features", {state.vi_retraces, a0, state.rt64_vi.status});
#endif
    return 1;
  }
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  if (!state.guest_leaf_probe && std::strcmp(name, "osSpTaskYield") == 0) {
    // A yield request gives waiting audio a chance to run. Our RSP backend
    // cannot suspend a task, so finish the pending graphics work now through
    // its sole SP/DP completion path. Waiting for another VI here delays the
    // audio task and changes the guest scheduler's buffer/frame cadence.
    // This neither advances the VI clock nor resubmits a graphics task.
    if (++state.rsp_yield_requests == 1U)
      jfg::support_event("rsp=yield-request-run-to-completion");
#if defined(_WIN32)
    state.gameplay_trace.event("rsp-yield", {state.vi_retraces,
        state.controller_samples, state.gameplay_trace_updates,
        state.pending_graphics_tasks.size(), state.rsp_yield_requests});
    complete_pending_live_graphics_tasks(state, target);
#endif
    return 1;
  }
  if (!state.guest_leaf_probe && std::strcmp(name, "osSpTaskYielded") == 0) {
    // The scheduler asks after SP completion. No task was suspended, so it
    // must retire this task rather than enqueueing it again as yielded.
    set_result(context, 0);
    if (++state.rsp_yield_queries == 1U)
      jfg::support_event("rsp=yield-query-completed");
    return 1;
  }
#endif
  hle::HleFunction function{};
  if (std::strcmp(name, "osCreateMesgQueue") == 0) {
    function = hle::HleFunction::kOsCreateMesgQueue;
    ++state.queue_create_calls;
  } else if (std::strcmp(name, "osSendMesg") == 0) {
    function = hle::HleFunction::kOsSendMesg;
    ++state.queue_send_calls;
  } else if (std::strcmp(name, "osJamMesg") == 0) {
    function = hle::HleFunction::kOsJamMesg;
    ++state.queue_send_calls;
  } else if (std::strcmp(name, "osRecvMesg") == 0) {
    function = hle::HleFunction::kOsRecvMesg;
    ++state.queue_recv_calls;
  } else if (std::strcmp(name, "osSpTaskLoad") == 0) {
    ++state.sp_task_load_calls;
    std::uint32_t task_type = 0U;
    if (a0 == 0U || !memory.read_u32(a0, task_type) || task_type > 7U)
      fail_closed_dispatch(state, "hle", "invalid-rsp-task", target);
    if (state.guest_leaf_probe) {
      const auto stack = static_cast<std::uint32_t>(context->r29);
      std::uint32_t flags = 0U;
      recomp_func_t *const generated = jfg_generated_lookup_function(address);
      if (generated == nullptr || state.mmio_trace == nullptr ||
          !state.mmio_trace->guest_sp_status ||
          (state.mmio_trace->sp_status.read() & 0x1dU) != 1U ||
          !memory.read_u32(a0 + 4U, flags) || (flags & 1U) != 0U ||
          (stack & 7U) != 0U || stack < kKseg0 + 0x80U ||
          stack > kKseg0 + kRdramSize - 4U)
        fail_closed_dispatch(state, "guest-leaf", "rsp-load-state", target);
      const auto transfers = state.mmio_trace->sp_dma.transfers();
      trace_guest_clock(state, "guest-sp-load-entry", target);
      generated(rdram, context);
      trace_guest_clock(state, "guest-sp-load-return", target);
      if (static_cast<std::uint32_t>(context->r29) != stack ||
          (state.mmio_trace->sp_status.read() & 0x7dU) != 0x41U ||
          state.mmio_trace->sp_dma.transfers() - transfers != 2U)
        fail_closed_dispatch(state, "guest-leaf", "rsp-load-result", target);
    }
    state.loaded_rsp_task = a0;
    state.loaded_rsp_task_type = task_type;
    return 1;
  } else if (std::strcmp(name, "osSpTaskStartGo") == 0) {
    ++state.sp_task_start_calls;
    if (a0 == 0U || a0 != state.loaded_rsp_task)
      fail_closed_dispatch(state, "hle", "rsp-task-not-loaded", target);
    if (state.guest_leaf_probe) {
      const auto stack = static_cast<std::uint32_t>(context->r29);
      recomp_func_t *const generated = jfg_generated_lookup_function(address);
      if (generated == nullptr || state.mmio_trace == nullptr ||
          !state.mmio_trace->guest_sp_status ||
          (state.mmio_trace->sp_status.read() & 0x1dU) != 1U ||
          (stack & 7U) != 0U || stack < kKseg0 + 0x20U ||
          stack > kKseg0 + kRdramSize - 4U)
        fail_closed_dispatch(state, "guest-leaf", "rsp-start-state", target);
      trace_guest_clock(state, "guest-sp-start-entry", target);
      generated(rdram, context);
      trace_guest_clock(state, "guest-sp-start-return", target);
      if ((state.mmio_trace->sp_status.read() & 0x7fU) != 0x40U ||
          static_cast<std::uint32_t>(context->r29) != stack)
        fail_closed_dispatch(state, "guest-leaf", "rsp-start-result", target);
    }
#if defined(JFG_PHASE8_LIVE_RUNTIME)
    if (state.loaded_rsp_task_type == 2U) {
#if defined(_WIN32)
      write_private_progress(state, "audio-begin");
#endif
      const auto audio_start = std::chrono::steady_clock::now();
      const bool audio_ok = execute_live_audio_task(state, memory);
      state.audio_task_microseconds += static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::steady_clock::now() - audio_start)
              .count());
#if defined(_WIN32)
      write_private_progress(state, "audio-end");
#endif
      if (!audio_ok)
        fail_closed_dispatch(state, "rsp", "audio-task-rejected", target);
#if defined(_WIN32)
      complete_guest_sp_status(state, target);
      write_private_progress(state, "audio-completion-begin");
#endif
      const auto sp = state.events.find(4U);
      if (state.scheduler == nullptr || sp == state.events.end() ||
          state.scheduler->send(sp->second.first, sp->second.second, false) !=
              hle::kOsSuccess)
        fail_closed_dispatch(state, "rsp", "sp-completion", target);
#if defined(_WIN32)
      write_private_progress(state, "audio-completion-end");
#endif
      state.loaded_rsp_task = 0U;
      state.loaded_rsp_task_type = 0U;
      if (state.scheduler_running) {
#if defined(_WIN32)
        write_private_progress(state, "audio-yield-begin");
#endif
        state.scheduler->yield_current();
#if defined(_WIN32)
        write_private_progress(state, "audio-yield-end");
#endif
      }
      return 1;
    }
#if defined(_WIN32)
    if (state.loaded_rsp_task_type == 1U) {
      write_private_progress(state, "graphics-prepare-begin");
      const auto graphics_start = std::chrono::steady_clock::now();
      constexpr std::uint64_t kFastReplayRenderWindow = 240U;
      // A requested RDRAM capture is a renderer diagnostic and must preserve
      // the complete RT64 task history; otherwise a late failure can disappear
      // because all of its prerequisite workloads were intentionally skipped.
      const bool skip_graphics =
          state.fast_replay && !state.renderer_writeback_probe && state.poll_target == 0U &&
          state.rdram_capture_path.empty() &&
          state.vi_retraces + kFastReplayRenderWindow < state.retrace_target;
      bool graphics_ok = true;
      std::uint32_t command_size = 0U;
      if (skip_graphics &&
          !memory.read_u32(state.loaded_rsp_task + 0x34U, command_size))
        graphics_ok = false;
      const bool preserve_immediate_task =
          skip_graphics && is_immediate_offscreen_task_candidate(command_size);
      if (graphics_ok && skip_graphics && !preserve_immediate_task) {
        PendingLiveGraphicsTask pending{};
        pending.skipped = true;
        state.pending_graphics_tasks.push_back(std::move(pending));
      } else if (graphics_ok) {
        graphics_ok = execute_live_graphics_task(state, memory);
      }
      const auto prepare_us = static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::steady_clock::now() - graphics_start)
              .count());
      state.gameplay_trace.event("prepare", {state.vi_retraces, state.controller_samples,
          state.gameplay_trace_updates, prepare_us, state.last_graphics_command_address,
          state.last_graphics_descriptor[5]});
      state.graphics_prepare_microseconds += prepare_us;
      state.graphics_prepare_max_us =
          (std::max)(state.graphics_prepare_max_us, prepare_us);
      if (state.timing_trace) {
        state.timing_trace << "prepare\t" << state.vi_frames << '\t'
                           << state.vi_retraces << '\t' << prepare_us << '\t'
                           << std::hex << state.last_graphics_command_address
                           << std::dec << '\n';
      }
      write_private_progress(state, "graphics-prepare-end");
      if (!graphics_ok) {
        std::array<char, 192U> operation{};
        (void)std::snprintf(
            operation.data(), operation.size(),
            "gfx-%u-c%08x-n%x-m%u-a%08x-w%08x-%08x-"
            "p%08x-%08x-%08x-b%08x-s%08x",
            static_cast<unsigned>(state.last_rt64_error),
            state.last_graphics_command_address,
            state.last_graphics_descriptor[5],
            state.materialize_reject_reason,
            state.materialize_reject_address,
            state.materialize_reject_word0,
            state.materialize_reject_word1,
            state.materialize_parent_address,
            state.materialize_parent_word0,
            state.materialize_parent_word1,
            state.materialize_block_address,
            state.materialize_parent_segment_base);
        fail_closed_dispatch(state, "rsp", operation.data(), target);
      }
      // Tiny actor passes can finish within the same VI as in the oracle.
      // The completion path fails closed if one ever targets a displayed
      // framebuffer; main scene tasks retain VI-boundary completion pacing.
      if (!state.pending_graphics_tasks.empty() &&
          !state.pending_graphics_tasks.back().skipped &&
          is_immediate_offscreen_task_candidate(
              state.last_graphics_descriptor[5]))
        complete_pending_live_graphics_tasks(state, target, true);
      state.loaded_rsp_task = 0U;
      state.loaded_rsp_task_type = 0U;
      if (state.scheduler_running)
        state.scheduler->yield_current();
      return 1;
    }
#endif
#endif
    fail_closed_dispatch(state, "hle",
                         state.loaded_rsp_task_type == 1U
                             ? "osSpTaskStartGo-gfx"
                             : "osSpTaskStartGo-other",
                         target);
  } else {
    fail_closed_dispatch(state, "hle", name, target);
  }
  const bool recv = function == hle::HleFunction::kOsRecvMesg;
  const bool clock_queue = a0 == 0x800FE4B8U || a0 == 0x800FE8A8U ||
                           a0 == 0x800FEB80U;
  if (clock_queue) trace_guest_clock(state, recv ? "recv" : "queue-call", a0);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (state.focus_update_start != 0U &&
      state.completed_game_updates + 1U >= state.focus_update_start - 1U &&
      state.completed_game_updates + 1U <= state.focus_update_end + 1U &&
      state.timing_trace.is_open() &&
      (a0 == 0x800FE4B8U || a0 == 0x800FE8A8U ||
       a0 == 0x800FEB80U)) {
    state.timing_trace << "game-queue-call\t" << state.vi_frames << '\t'
                       << state.vi_retraces << "\t0\t"
                       << (recv ? "recv" :
                           function == hle::HleFunction::kOsCreateMesgQueue
                               ? "create" : "send")
                       << '\t' << state.completed_game_updates << '\t'
                       << state.scheduler->current_thread_id() << '\t'
                       << std::hex << a0 << ':' << a1 << ':' << a2 << ':'
                       << static_cast<std::uint32_t>(context->r31)
                       << std::dec << '\n';
  }
#endif
  if (recv) {
    trace_si_timing(state, "recv-entry", a0,
                    static_cast<std::uint32_t>(context->r31),
                    static_cast<std::int32_t>(a2));
    const auto si = state.events.find(5U);
    if (si != state.events.end() && a0 == si->second.first) {
      ++state.si_receive_calls;
      state.last_si_receive_return = static_cast<std::uint32_t>(context->r31);
      state.last_si_receive_block = a2;
      if (a2 == static_cast<std::uint32_t>(hle::kOsMesgBlock)) {
        for (std::size_t index = 0U;
             index < state.last_si_dispatch_trace.size(); ++index) {
          const std::size_t distance =
              state.last_si_dispatch_trace.size() - index;
          state.last_si_dispatch_trace[index] =
              state.recent_dispatch_position >= distance
                  ? state.recent_dispatches[
                        (state.recent_dispatch_position - distance) %
                        state.recent_dispatches.size()]
                  : 0U;
        }
      }
    }
  }
  const bool send = function == hle::HleFunction::kOsSendMesg ||
                    function == hle::HleFunction::kOsJamMesg;
  if (send) {
    const auto block = static_cast<std::int32_t>(a2);
    if (state.scheduler == nullptr || !state.queues.contains(a0) ||
        (block != hle::kOsMesgNoBlock && block != hle::kOsMesgBlock)) {
      fail_closed_dispatch(state, "hle", "invalid-send", target);
    }
    const auto result = state.scheduler->send(
        a0, a1, function == hle::HleFunction::kOsJamMesg);
    if (result == hle::kOsSuccess) {
      set_result(context, result);
      if (state.scheduler_running)
        state.scheduler->yield_current();
      return 1;
    }
    if (block == hle::kOsMesgNoBlock) {
      set_result(context, hle::kOsFull);
      return 1;
    }
    if (!state.scheduler_running || state.scheduler->current_thread_id() < 0)
      fail_closed_dispatch(state, "hle", "blocking-send-outside-thread", target);
    // Validate the full queue before parking; corrupt state must not become
    // an endless wait. No other guest runs between the failed send and here.
    if (hle::dispatch_queue_call(memory, {function, a0, a1, a2}).disposition !=
        hle::QueueDispatchDisposition::block)
      fail_closed_dispatch(state, "hle", "blocking-send-invalid-queue", target);
    set_result(context, state.scheduler->send_blocking(
        a0, a1, function == hle::HleFunction::kOsJamMesg));
    return 1;
  }
  const auto received_vi_message = [&]() {
    std::uint32_t message = 0U;
    return recv && a0 == state.vi_queue && a1 != 0U &&
           memory.read_u32(a1, message) && message == state.vi_message;
  };
  const auto trace_vi_queue_receive = [&]() {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
    const bool focused_receive = state.focus_update_start != 0U &&
        state.completed_game_updates + 1U >= state.focus_update_start - 1U &&
        state.completed_game_updates + 1U <= state.focus_update_end + 1U;
    if (recv && a0 == state.vi_queue &&
        (state.actor_timing_trace || focused_receive) &&
        state.timing_trace.is_open()) {
      std::uint32_t message = 0U;
      const bool readable = a1 != 0U && memory.read_u32(a1, message);
      state.timing_trace << "vi-queue-receive\t" << state.vi_frames << '\t'
                         << state.vi_retraces << "\t0\t"
                         << (readable ? std::to_string(message) : "unreadable")
                         << '\t' << state.vi_message << '\t'
                         << state.vi_messages_delivered << '\t'
                         << (received_vi_message() &&
                             state.vi_messages_delivered > state.vi_retraces)
                         << '\n';
    }
#endif
  };
  const auto result = hle::dispatch_queue_call(memory, {function, a0, a1, a2});
  if (state.guest_leaf_probe && recv && a1 == 0U && a2 == 0U &&
      result.disposition == hle::QueueDispatchDisposition::accepted &&
      result.return_value == hle::kOsEmpty) {
    // The validated empty nonblocking path made no queue mutation and never
    // follows guest wait lists. Execute its original Status/save/restore path.
    // Other queue paths still require a guest-thread bridge; they are NOT
    // silently declared instruction-accounted by this partial probe.
    const auto sp = static_cast<std::uint32_t>(context->r29);
    if ((sp & 7U) != 0U || sp < kKseg0 + 0x28U ||
        sp > kKseg0 + kRdramSize - 12U)
      fail_closed_dispatch(state, "guest-leaf", "queue-stack-range", target);
    recomp_func_t *const generated = jfg_generated_lookup_function(address);
    if (generated == nullptr)
      fail_closed_dispatch(state, "guest-leaf", "queue-unavailable", target);
    const auto status = cop0_status_read(context);
    trace_guest_clock(state, "guest-queue-empty-entry", target);
    generated(rdram, context);
    trace_guest_clock(state, "guest-queue-empty-return", target);
    if (static_cast<std::int64_t>(context->r2) != hle::kOsEmpty ||
        static_cast<std::uint32_t>(context->r29) != sp || cop0_status_read(context) != status)
      fail_closed_dispatch(state, "guest-leaf", "queue-empty-result", target);
  }
  if (recv && result.disposition == hle::QueueDispatchDisposition::accepted)
    trace_si_timing(state, "recv-return", a0,
                    static_cast<std::uint32_t>(context->r31), result.return_value);
  if (result.disposition == hle::QueueDispatchDisposition::accepted) {
    if (recv && clock_queue) trace_guest_clock(state, "recv-return", a0);
    if (function == hle::HleFunction::kOsCreateMesgQueue) {
      state.queues.insert(a0);
      state.journal.append(NativeEventKind::kQueueCreated, a0, a1, a2);
    }
    if (function != hle::HleFunction::kOsCreateMesgQueue) {
      set_result(context, result.return_value);
    }
    if (recv && result.return_value == hle::kOsSuccess && state.scheduler != nullptr)
      state.scheduler->notify_received(a0);
#if defined(JFG_PHASE8_LIVE_RUNTIME)
    if (recv) {
      state.last_receive_queue = a0;
      state.last_receive_block = a2;
      state.last_receive_result = result.return_value;
      if (result.return_value == hle::kOsSuccess)
        ++state.receive_successes;
      else
        ++state.receive_empties;
    }
    if (recv && result.return_value == hle::kOsSuccess) {
      state.empty_receive_polls.erase(a0);
    } else if (recv &&
               a2 == static_cast<std::uint32_t>(hle::kOsMesgNoBlock)) {
      std::uint32_t &poll_count = state.empty_receive_polls[a0];
      ++poll_count;
      if (poll_count >= kEmptyReceivePollTimeslice &&
          state.scheduler_running && state.scheduler != nullptr &&
          state.scheduler->current_thread_id() >= 0 &&
          state.vi_internal_queue != 0U) {
        state.empty_receive_polls.clear();
        ++state.interrupt_timeslices;
        state.scheduler->preempt_current();
      }
    }
#endif
    if (recv && result.return_value == hle::kOsSuccess)
      trace_vi_queue_receive();
    if (function == hle::HleFunction::kOsRecvMesg &&
        result.return_value == hle::kOsSuccess && received_vi_message() &&
        state.vi_messages_delivered > state.vi_retraces) {
      ++state.vi_retraces;
      jfg::support_breadcrumb(state.vi_frames, state.vi_retraces, state.controller_samples);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      trace_phase9_event(state, "vi-consumed", state.controller_samples);
#endif
      state.journal.append(NativeEventKind::kViMessageConsumed, a0, a1,
                           static_cast<std::uint32_t>(state.vi_retraces));
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      write_retrace_semantic_hash(state);
      write_private_progress(state);
#endif
      if (state.vi_retraces >= state.retrace_target)
        retrace_gate(state);
    }
    return 1;
  }
  if (result.disposition == hle::QueueDispatchDisposition::reject) {
    const auto block = static_cast<std::int32_t>(a2);
    if (!state.queues.contains(a0)) {
      std::array<char, 32U> operation{};
      (void)std::snprintf(operation.data(), operation.size(),
                          "unknown-queue-%08x", a0);
      fail_closed_dispatch(state, "hle", operation.data(), target);
    }
    const char *reason = block != hle::kOsMesgNoBlock &&
                                   block != hle::kOsMesgBlock
                             ? "invalid-block-mode"
                         : recv && a1 != 0U &&
                                   (a1 < kKseg0 || a1 - kKseg0 >
                                                       kRdramSize - 4U)
                             ? "invalid-message-output"
                             : "corrupt-queue";
    fail_closed_dispatch(state, "hle", reason, target);
  }
  if (recv && state.scheduler != nullptr && state.scheduler_running) {
#if defined(JFG_PHASE8_LIVE_RUNTIME)
    ++state.receive_blocks;
    state.last_receive_queue = a0;
    state.last_receive_block = a2;
#endif
    const std::int32_t receive_result =
        state.scheduler->recv_blocking(a0, a1);
    if (clock_queue) trace_guest_clock(state, "recv-resume", a0);
    trace_si_timing(state, "recv-resume", a0,
                    static_cast<std::uint32_t>(context->r31), receive_result);
    set_result(context, receive_result);
    if (receive_result == hle::kOsSuccess)
      trace_vi_queue_receive();
#if defined(JFG_PHASE8_LIVE_RUNTIME)
    state.last_receive_result = receive_result;
    if (receive_result == hle::kOsSuccess)
      ++state.receive_successes;
#endif
    if (receive_result == hle::kOsSuccess && received_vi_message() &&
        state.vi_messages_delivered > state.vi_retraces) {
      ++state.vi_retraces;
      jfg::support_breadcrumb(state.vi_frames, state.vi_retraces, state.controller_samples);
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      trace_phase9_event(state, "vi-consumed", state.controller_samples);
#endif
      state.journal.append(NativeEventKind::kViMessageConsumed, a0, a1,
                           static_cast<std::uint32_t>(state.vi_retraces));
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      write_retrace_semantic_hash(state);
      write_private_progress(state);
#endif
      if (state.vi_retraces >= state.retrace_target)
        retrace_gate(state);
    }
    return 1;
  }
  fail_closed_dispatch(state, "hle",
                       recv ? "blocking-recv"
                            : (function == hle::HleFunction::kOsJamMesg
                                   ? "blocking-jam"
                                   : "blocking-send"),
                       target);
}

int native_cpu_operation(void* opaque, void* guest_context, std::uint32_t operation,
                         std::uint32_t selector, std::uint64_t* value) {
  auto& state = *static_cast<State*>(opaque);
  if (!state.guest_leaf_probe || guest_context == nullptr || value == nullptr) return 0;
  auto* context = static_cast<recomp_context*>(guest_context);
  // This bridge has only a 32-bit virtual-address model. Do not pretend
  // that enabling extended addressing supplies a compatible EntryHi state.
  if ((static_cast<std::uint32_t>(cop0_status_read(context)) & 0xe0U) != 0U) return 0;
  if (state.guest_os_probe) {
    if (operation == JFG_CPU_READ_MI_MASK || operation == JFG_CPU_WRITE_MI_MASK) {
      auto* trace = state.mmio_trace;
      if (selector != 0xa430000cU || trace == nullptr ||
          !state.diagnostic_host_clock || !trace->native_elapsed_time ||
          !trace->guest_execution || !trace->guest_mi_mask ||
          !trace->mi_mask.initialized() || trace->pending_page != nullptr) return 0;
      const auto address = reinterpret_cast<std::uintptr_t>(trace->pages[2U]) + 0x0cU;
      if (operation == JFG_CPU_READ_MI_MASK) {
        trace->record(2U, address, false);
        *value = trace->mi_mask.read();
        return 1;
      }
      auto next = trace->mi_mask;
      if (*value > UINT32_MAX || !next.command(static_cast<std::uint32_t>(*value))) return 0;
      trace->record(2U, address, true);
      trace->mi_mask = next;
      ++trace->execution_io_revision;
      trace->device_write_page = 2U;
      trace->device_write_offset = 0x0cU;
      trace->device_write_value = next.read();
      // The device model owns this register. Any subsequent guarded access
      // refreshes the page mirror before the host load executes.
      return 1;
    }
    const auto extend = [](std::uint32_t word) {
      return static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(word)));
    };
    if (operation == JFG_CPU_READ_REGISTER) {
      if (selector == 9U) { *value = extend(static_cast<std::uint32_t>(state.cpu_count)); return 1; }
      if (selector == 11U) { *value = extend(state.guest_timer.read()); return 1; }
      if (selector == 13U) {
        *value = extend(state.guest_cause |
            (state.guest_timer.interrupt() ? 0x8000U : 0U) |
            ((state.mmio_trace->mi_pending & state.mmio_trace->mi_mask.read()) ? 0x400U : 0U));
        return 1;
      }
      if (selector == 14U) { *value = extend(state.guest_epc); return 1; }
    }
    if (operation == JFG_CPU_WRITE_REGISTER) {
      state.execution_fast_deadline = 0U;
      state.native_fast_ready = false;
      if (selector == 11U) {
        if (!state.guest_timer.write(state.cpu_count, static_cast<std::uint32_t>(*value)))
          fail_closed_dispatch(state, "guest-cpu", "unqualified-compare-write", state.guest_last_pc);
        if (state.guest_clock_trace)
          state.guest_clock_trace << "compare\t" << state.cpu_count << '\t'
              << static_cast<std::uint32_t>(*value) << '\n';
        return 1;
      }
      if (selector == 13U) {
        state.guest_cause = (state.guest_cause & ~0x300U) | (static_cast<std::uint32_t>(*value) & 0x300U);
        return 1;
      }
      if (selector == 14U) { state.guest_epc = static_cast<std::uint32_t>(*value); return 1; }
    }
    if (operation == JFG_CPU_EXCEPTION_RETURN) {
      std::uint32_t next = 0U;
      hle::GuestMemory memory({state.rdram, kRdramSize}, hle::GuestMemory::Layout::native_word_big_endian);
      if ((context->status_reg & 6U) != 2U || state.guest_transport == nullptr ||
          !memory.read_u32(0x800a9e90U, next) || next < kKseg0 || next > kKseg0 + kRdramSize - 0x1b0U)
        return 0;
      cop0_status_write(context, context->status_reg & ~2U);
      state.guest_previous_branch = state.guest_previous_delay = false;
      state.guest_eret_boundary = true;
      state.native_fast_ready = false;
      state.guest_transport->eret(next, state.guest_epc, *context);
      return 1;
    }
  }
  if (operation == JFG_CPU_CACHE_OPERATION)
    return reference_coherent_cache_operation(selector, *value, kRdramSize) ? 1 : 0;
  if (operation == JFG_CPU_READ_REGISTER) {
    // Only the independently qualified cold-boot osInitialize path. The
    // cooperative runtime does not model live Cause/exception delivery.
    if (selector == 13U && !state.os_initialized && !state.scheduler_running) {
      *value = 0U;
      return 1;
    }
    const auto result = state.tlb_registers.read(selector);
    if (!result) return 0;
    *value = static_cast<std::uint64_t>(static_cast<std::int64_t>(
        static_cast<std::int32_t>(*result)));
    return 1;
  }
  if (operation == JFG_CPU_WRITE_REGISTER)
    return state.tlb_registers.write(selector, static_cast<std::uint32_t>(*value)) ? 1 : 0;
  if (operation == JFG_CPU_TLB_OPERATION) {
    switch (selector) {
    case 0: return state.tlb_registers.probe() ? 1 : 0;
    case 1: return state.tlb_registers.read_indexed() ? 1 : 0;
    case 2: return state.tlb_registers.write_indexed() ? 1 : 0;
    default: return 0;
    }
  }
  return 0;
}

struct DispatchBinding {
  State &state;
  bool bound = false;
  bool cpu_bound = false;
  ~DispatchBinding() {
    if (cpu_bound)
      (void)jfg_minimal_runtime_unbind_cpu(native_cpu_operation, &state);
    if (bound)
      (void)jfg_minimal_runtime_unbind_dispatch(dispatch, &state);
  }
};
bool range(std::uint64_t offset, std::uint64_t bytes, std::size_t limit) {
  return offset <= limit && bytes <= limit - offset;
}
bool write_boot_word(std::vector<std::uint8_t> &bytes, std::uint32_t address,
                     std::uint32_t value) {
  if (address < kKseg0 || !range(address - kKseg0, 4U, bytes.size()))
    return false;
  const std::size_t offset = address - kKseg0;
  bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
  bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
  bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
  bytes[offset + 3U] = static_cast<std::uint8_t>(value);
  return true;
}
bool initialize_ipl_state(std::vector<std::uint8_t> &bytes,
                          std::span<const std::uint8_t> rom) {
  // Documented libultra boot globals populated by the IPL before the ROM
  // entry point. This runner admits the USA image, so the TV mode is NTSC;
  // it models a cold cartridge boot with the oracle-observed 4 MiB RDRAM.
  return initialize_6105_handoff(bytes, rom) &&
         write_boot_word(bytes, 0x80000300U, 1U) &&
         write_boot_word(bytes, 0x80000304U, 0U) &&
         write_boot_word(bytes, 0x80000308U, 0xB0000000U) &&
         write_boot_word(bytes, 0x8000030CU, 0U) &&
         write_boot_word(bytes, 0x80000318U,
                         static_cast<std::uint32_t>(kRdramSize));
}
bool load_static_sections(const std::vector<std::uint8_t> &rom,
                          std::vector<std::uint8_t> &bytes) {
  const std::size_t count = jfg_generated_section_count();
  for (std::size_t index = 0U; index < count; ++index) {
    JfgGeneratedSectionMetadata metadata{};
    if (jfg_generated_section_metadata(static_cast<std::uint32_t>(index),
                                       &metadata) == 0 ||
        metadata.is_overlay > 1U)
      return false;
    if (metadata.is_overlay == 1U)
      continue;
    if (metadata.linked_vram < kKseg0 || (metadata.linked_vram & 3U) != 0U ||
        (metadata.rom_start & 3U) != 0U ||
        metadata.text_rom_offset > metadata.text_size)
      return false;
    const std::uint64_t copy =
        std::uint64_t(metadata.text_size) + metadata.data_size;
    const std::uint64_t total = copy + metadata.bss_size;
    const std::uint64_t destination = metadata.linked_vram - kKseg0;
    if (!range(metadata.rom_start, copy, rom.size()) ||
        !range(destination, total, bytes.size()))
      return false;
    std::memcpy(bytes.data() + destination, rom.data() + metadata.rom_start,
                static_cast<std::size_t>(copy));
    std::memset(bytes.data() + destination + copy, 0, metadata.bss_size);
  }
  return true;
}

void invoke_generated(const jfg::GeneratedOverlayFunctionHandle &function,
                      std::uint8_t *rdram, recomp_context *context,
                      State &state, std::uint32_t transfer_target) {
  if (function.invoke(rdram, context) != jfg::GeneratedOverlayError::none) {
    ledger(state, "generated", "invoke-failed", transfer_target);
  }
}

std::array<std::uint8_t, 20U> sha1(std::span<const std::uint8_t> input) {
  std::array<std::uint32_t, 5U> hash = {0x67452301U, 0xefcdab89U, 0x98badcfeU,
                                        0x10325476U, 0xc3d2e1f0U};
  std::vector<std::uint8_t> padded(input.begin(), input.end());
  padded.push_back(0x80U);
  while ((padded.size() % 64U) != 56U)
    padded.push_back(0U);
  const std::uint64_t bits = static_cast<std::uint64_t>(input.size()) * 8U;
  for (int shift = 56; shift >= 0; shift -= 8)
    padded.push_back(static_cast<std::uint8_t>(bits >> shift));
  for (std::size_t block = 0U; block < padded.size(); block += 64U) {
    std::array<std::uint32_t, 80U> words{};
    for (std::size_t index = 0U; index < 16U; ++index)
      words[index] = (std::uint32_t(padded[block + index * 4U]) << 24U) |
                     (std::uint32_t(padded[block + index * 4U + 1U]) << 16U) |
                     (std::uint32_t(padded[block + index * 4U + 2U]) << 8U) |
                     padded[block + index * 4U + 3U];
    for (std::size_t index = 16U; index < words.size(); ++index)
      words[index] = std::rotl(words[index - 3U] ^ words[index - 8U] ^
                                   words[index - 14U] ^ words[index - 16U],
                               1);
    auto [a, b, c, d, e] = hash;
    for (std::size_t index = 0U; index < words.size(); ++index) {
      const std::uint32_t f = index < 20U   ? ((b & c) | (~b & d))
                              : index < 40U ? (b ^ c ^ d)
                              : index < 60U ? ((b & c) | (b & d) | (c & d))
                                            : (b ^ c ^ d);
      const std::uint32_t k = index < 20U   ? 0x5a827999U
                              : index < 40U ? 0x6ed9eba1U
                              : index < 60U ? 0x8f1bbcdcU
                                            : 0xca62c1d6U;
      const std::uint32_t next = std::rotl(a, 5) + f + e + k + words[index];
      e = d;
      d = c;
      c = std::rotl(b, 30);
      b = a;
      a = next;
    }
    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
    hash[4] += e;
  }
  std::array<std::uint8_t, 20U> result{};
  for (std::size_t index = 0U; index < hash.size(); ++index)
    for (std::size_t byte = 0U; byte < 4U; ++byte)
      result[index * 4U + byte] =
          static_cast<std::uint8_t>(hash[index] >> (24U - byte * 8U));
  return result;
}
} // namespace

// Called only by an explicitly instrumented, private generated-code copy.
// An ordinary generated root contains no call sites and is unaffected.
extern "C" void jfg_phase9_instruction_probe(uint32_t pc, uint8_t *rdram,
                                               recomp_context *context) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  State *const state = g_active_child_state;
  if (state == nullptr || !state->instruction_probe_trace.is_open() ||
      rdram != state->rdram || context == nullptr ||
      state->focus_update_start == 0U ||
      state->completed_game_updates + 1U < state->focus_update_start - 1U ||
      state->completed_game_updates + 1U > state->focus_update_end + 1U)
    return;
  if (++state->instruction_probe_hits > 1024U) {
    std::fputs("native instruction probe overflow\n", stderr);
    std::abort();
  }
  auto &out = state->instruction_probe_trace;
  out << state->completed_game_updates + 1U << '\t'
      << state->vi_retraces << '\t' << state->controller_samples << '\t'
      << "0x" << std::hex << std::setw(8) << std::setfill('0') << pc;
  const std::array<std::uint64_t, 11U> gprs = {
      context->r4, context->r5, context->r6, context->r7,
      context->r10, context->r15, context->r19, context->r20,
      context->r21, context->r23, context->r25};
  for (const auto value : gprs)
    out << '\t' << "0x" << std::setw(8)
        << static_cast<std::uint32_t>(value);
  out << '\t' << "0x" << std::setw(8) << context->f4.u32l
      << '\t' << "0x" << std::setw(8) << context->f22.u32l
      << '\t' << "0x" << std::setw(8)
      << (context->f_odd != nullptr ? context->f_odd[(23U - 1U) * 2U] : 0U)
      << std::dec << '\t' << std::fegetround() << '\n';
  out.flush();
#else
  (void)pc;
  (void)rdram;
  (void)context;
#endif
}

#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
// Host-only diagnostic data. Instruction-entry/successor samples do not by
// themselves prove retirement, a completed receive or caller equivalence.
static void emit_device_event(State& state, const recomp_context& cpu,
    unsigned phase, unsigned source, std::uint32_t pc, std::uint32_t word,
    std::optional<std::uint64_t> deadline = std::nullopt) {
  if (!state.device_event_stream || state.device_event_probe.complete) return;
  const auto invocation = state.completed_game_updates + 1U;
  if (invocation < state.device_event_probe.first) return;
  if (invocation > state.device_event_probe.last) {
    if (!jfg_device_event_finish(&state.device_event_probe))
      fail_closed_dispatch(state, "device-event-probe", "output-failed", pc);
    return;
  }
  jfg_device_event_row row{};
  row.invocation = static_cast<std::uint32_t>(invocation);
  row.phase = phase; row.source = source; row.pc = pc; row.opcode = word;
  row.previous_pc = state.device_event_previous_pc;
  row.previous_opcode = state.device_event_previous_word;
  row.clock_raw = static_cast<std::uint32_t>(state.cpu_count);
  row.clock_anchor_pc = pc;
  row.deadline_valid = deadline.has_value() ? 1U : 0U;
  row.deadline = static_cast<std::uint32_t>(deadline.value_or(0));
  row.status = cpu.status_reg;
  row.mi_pending = state.mmio_trace->mi_pending;
  row.mi_mask = state.mmio_trace->mi_mask.read();
  // Same pure value composition as JFG_CPU_READ_REGISTER selector 13; do not
  // report only the software latch while omitting pending hardware lines.
  row.cause = state.guest_cause | (state.guest_timer.interrupt() ? 0x8000U : 0U) |
      ((row.mi_pending & row.mi_mask) ? 0x400U : 0U);
  hle::GuestMemory memory({state.rdram, kRdramSize}, hle::GuestMemory::Layout::native_word_big_endian);
  if (!memory.read_u32(0x800a9e90U, row.thread))
    fail_closed_dispatch(state, "device-event-probe", "thread-read", pc);
  const std::array<std::uint64_t, 32> registers = {
    0, cpu.r1, cpu.r2, cpu.r3, cpu.r4, cpu.r5, cpu.r6, cpu.r7,
    cpu.r8, cpu.r9, cpu.r10, cpu.r11, cpu.r12, cpu.r13, cpu.r14, cpu.r15,
    cpu.r16, cpu.r17, cpu.r18, cpu.r19, cpu.r20, cpu.r21, cpu.r22, cpu.r23,
    cpu.r24, cpu.r25, cpu.r26, cpu.r27, cpu.r28, cpu.r29, cpu.r30, cpu.r31};
  std::copy(registers.begin(), registers.end(), row.gpr);
  if (!jfg_device_event_emit(&state.device_event_probe, &row))
    fail_closed_dispatch(state, "device-event-probe", "output-or-budget", pc);
}

static void emit_instruction_effect(State& state, const recomp_context& cpu,
    unsigned phase, unsigned section, unsigned pc, unsigned word,
    std::uint32_t target = 0U, std::uint32_t selected_owner = 0U) {
  if (!state.instruction_effect_stream.is_open()) return;
  const auto invocation = static_cast<std::uint32_t>(state.completed_game_updates + 1U);
  if (!state.instruction_effect_trace.advance(invocation))
    fail_closed_dispatch(state, "instruction-effect-probe", "incomplete-or-output", pc);
  if (invocation != state.instruction_effect_update) return;
  if (!state.guest_os_probe || !state.guest_transport || !state.device_event_stream ||
      state.device_event_probe.complete)
    fail_closed_dispatch(state, "instruction-effect-probe", "unqualified-runtime", pc);
  InstructionEffectRow row{};
  row.phase = phase; row.section = section; row.pc = pc; row.opcode = word;
  row.owner = state.guest_transport->current();
  row.count = static_cast<std::uint32_t>(state.cpu_count);
  row.status = cpu.status_reg;
  row.cause = state.guest_cause | (state.guest_timer.interrupt() ? 0x8000U : 0U) |
      ((state.mmio_trace->mi_pending & state.mmio_trace->mi_mask.read()) ? 0x400U : 0U);
  row.target = target; row.selected_owner = selected_owner;
  row.device_sequence = state.device_event_probe.rows;
  hle::GuestMemory memory({state.rdram, kRdramSize}, hle::GuestMemory::Layout::native_word_big_endian);
  if (!memory.read_u32(0x800a9e90U, row.thread_word))
    fail_closed_dispatch(state, "instruction-effect-probe", "thread-read", pc);
  row.gpr = {0, cpu.r1, cpu.r2, cpu.r3, cpu.r4, cpu.r5, cpu.r6, cpu.r7,
    cpu.r8, cpu.r9, cpu.r10, cpu.r11, cpu.r12, cpu.r13, cpu.r14, cpu.r15,
    cpu.r16, cpu.r17, cpu.r18, cpu.r19, cpu.r20, cpu.r21, cpu.r22, cpu.r23,
    cpu.r24, cpu.r25, cpu.r26, cpu.r27, cpu.r28, cpu.r29, cpu.r30, cpu.r31};
  if (!state.instruction_effect_trace.emit(invocation, row))
    fail_closed_dispatch(state, "instruction-effect-probe", "pair-boundary-or-budget", pc);
  if (phase == 0U) state.instruction_effect_section = section;
}
#endif

// The generated entry hook is after execution_probe returns: asynchronous
// exception processing must finish before this instruction becomes pending.
extern "C" void jfg_phase9_instruction_effect_entry(unsigned section, unsigned pc,
    unsigned word, unsigned qualification, void* context) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (g_active_child_state != nullptr && context != nullptr) {
    auto& state = *g_active_child_state;
    if (!state.instruction_effect_stream.is_open()) return;
    if (state.completed_game_updates + 1U == state.instruction_effect_update &&
        (qualification > 1U || (qualification == 1U) != (word == 0x42000018U)))
      fail_closed_dispatch(state, "instruction-effect-probe", "unqualified-instruction", pc);
    emit_instruction_effect(state, *static_cast<recomp_context*>(context), 0U, section, pc, word);
  }
#else
  (void)section; (void)pc; (void)word; (void)qualification; (void)context;
#endif
}

extern "C" void jfg_phase9_instruction_effect(unsigned section, unsigned pc,
    unsigned word, void* context) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (g_active_child_state != nullptr && context != nullptr)
    emit_instruction_effect(*g_active_child_state, *static_cast<recomp_context*>(context),
                            1U, section, pc, word);
#else
  (void)section; (void)pc; (void)word; (void)context;
#endif
}

// Only called by a separately regenerated private observation root. Count
// entered instruction sites, including loop iterations and executed slots.
// The default is observation-only. The separate guest-OS diagnostic executes
// original OS bodies, with the bounded reference CPU/device profile below.
// Keep the execution probe out of hot instruction sites on every compiler.
#if defined(_MSC_VER)
#define JFG_EXECUTION_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define JFG_EXECUTION_NOINLINE __attribute__((noinline))
#else
#define JFG_EXECUTION_NOINLINE
#endif
extern "C" JFG_EXECUTION_NOINLINE void jfg_phase9_execution_probe_slow(
    unsigned section, unsigned pc, unsigned word, void* context);

// Keep the common instruction path independent of the rare device/exception
// path's large stack frame. The same predicate, cost and state updates run.
extern "C" void jfg_phase9_execution_probe(unsigned section, unsigned pc,
                                          unsigned word, void* context) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (g_active_child_state != nullptr && context != nullptr) {
    auto& state = *g_active_child_state;
    if (state.diagnostic_cpu_costs) ++state.profile_out_of_line_instructions;
    if (state.diagnostic_host_clock) {
      jfg_phase9_execution_probe_slow(section,pc,word,context);
      return;
    }
    // Private optimization: ordinary instructions before the next event need
    // only the same cost and pipeline bookkeeping. Hardware writes, interrupt
    // eligibility, exception return and observers fall back to the full path.
    if ((state.execution_fast_profile_ready ||
         (state.diagnostic_host_clock && state.host_clock_poll_budget > 0U && --state.host_clock_poll_budget > 0U)) &&
        !state.guest_eret_boundary && word != 0x1000FFFFU &&
        pc != 0x8009AB80U && pc != 0x80098B74U &&
        (state.play_mode || state.observed_guest_instructions < 24000000000ULL) &&
        state.execution_fast_deadline > state.cpu_count &&
        state.execution_fast_revision == state.mmio_trace->execution_io_revision) {
      auto& cpu = *static_cast<recomp_context*>(context);
      const unsigned primary = word >> 26U;
      const bool fpu = primary == 0x11U || primary == 0x31U ||
          primary == 0x35U || primary == 0x39U || primary == 0x3DU;
      const bool eligible = (cpu.status_reg & 7U) == 1U &&
          (state.execution_fast_pending & cpu.status_reg & 0xFF00U) != 0U;
      const bool park = primary == 3U &&
          (((pc + 4U) & 0xF0000000U) | ((word & 0x03FFFFFFU) << 2U)) == 0x80075698U;
      if (!eligible && !(fpu && (cpu.status_reg & 0x20000000U) == 0U) && !park) {
        ++state.observed_guest_instructions;
        ++state.execution_fast_hits;
        const bool delay = state.guest_previous_branch && pc == state.guest_last_pc + 4U;
        if (!state.diagnostic_host_clock) {
          const auto cycles = jfg_full_cache_cost(pc,word,&cpu,state.rdram) + state.diagnostic_cpu_half_tick;
          state.cpu_count += cycles / 2U;
          state.diagnostic_cpu_half_tick = cycles % 2U;
        }
        state.mmio_trace->guest_count = state.cpu_count;
        state.guest_last_pc = pc;
        state.guest_last_word = word;
        state.guest_previous_delay = delay;
        state.guest_previous_branch = primary == 1U || (primary >= 2U && primary <= 7U) ||
            (primary >= 20U && primary <= 23U) ||
            (primary == 0U && ((word & 63U) == 8U || (word & 63U) == 9U)) ||
            (primary == 0x11U && ((word >> 21U) & 31U) == 8U);
        return;
      }
    }
  }
#endif
  jfg_phase9_execution_probe_slow(section,pc,word,context);
}

extern "C" void jfg_phase9_execution_probe_predecoded(unsigned metadata, unsigned pc,
                                          unsigned word, void* context) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  g_community_crash_pc = pc;  // community crash report: last guest instruction
#endif
  const unsigned section=metadata&255U;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (g_active_child_state != nullptr && context != nullptr) {
    auto& state = *g_active_child_state;
    if (state.diagnostic_cpu_costs) ++state.profile_out_of_line_instructions;
    if (state.diagnostic_host_clock) {
      jfg_phase9_execution_probe_slow(section,pc,word,context);
      return;
    }
    // Private optimization: ordinary instructions before the next event need
    // only the same cost and pipeline bookkeeping. Hardware writes, interrupt
    // eligibility, exception return and observers fall back to the full path.
    if ((state.execution_fast_profile_ready ||
         (state.diagnostic_host_clock && state.host_clock_poll_budget > 0U && --state.host_clock_poll_budget > 0U)) &&
        !state.guest_eret_boundary && (metadata&0x800U)==0U &&
        (state.play_mode || state.observed_guest_instructions < 24000000000ULL) &&
        state.execution_fast_deadline > state.cpu_count &&
        state.execution_fast_revision == state.mmio_trace->execution_io_revision) {
      auto& cpu = *static_cast<recomp_context*>(context);
      const bool fpu=(metadata&0x100U)!=0U;
      const bool eligible = (cpu.status_reg & 7U) == 1U &&
          (state.execution_fast_pending & cpu.status_reg & 0xFF00U) != 0U;
      const bool park=(metadata&0x200U)!=0U;
      if (!eligible && !(fpu && (cpu.status_reg & 0x20000000U) == 0U) && !park) {
        ++state.observed_guest_instructions;
        ++state.execution_fast_hits;
        const bool delay = state.guest_previous_branch && pc == state.guest_last_pc + 4U;
        if (!state.diagnostic_host_clock) {
          const auto cycles = jfg_full_cache_cost_predecoded(pc,word,&cpu,state.rdram,metadata) + state.diagnostic_cpu_half_tick;
          state.cpu_count += cycles / 2U;
          state.diagnostic_cpu_half_tick = cycles % 2U;
        }
        state.mmio_trace->guest_count = state.cpu_count;
        state.guest_last_pc = pc;
        state.guest_last_word = word;
        state.guest_previous_delay = delay;
        state.guest_previous_branch=(metadata&0x400U)!=0U;
        return;
      }
    }
  }
#endif
  jfg_phase9_execution_probe_slow(section,pc,word,context);
}

extern "C" JFG_EXECUTION_NOINLINE void jfg_phase9_execution_probe_slow(unsigned section, unsigned pc,
                                          unsigned word, void *context) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (g_active_child_state != nullptr && context != nullptr) {
    auto& state = *g_active_child_state;
    if (!state.rdram_capture_path.empty())
      state.private_fault_cpu = static_cast<recomp_context*>(context);
    if (state.private_model_pointer_watch && state.vi_retraces >= 2000U) {
      if (state.private_model_page_watch && !state.private_model_pages_ready &&
          !arm_private_memory_pages(state))
        fail_closed_dispatch(state,"private-memory-watch","page-setup",pc);
      if (state.private_model_hardware_watch && !private_model_watch_armed) {
        if (!arm_private_model_write_watch(state))
          fail_closed_dispatch(state,"private-memory-watch","debug-register-in-use",pc);
      }

      std::uint32_t current_model = 0U;
      std::memcpy(&current_model,state.rdram+0xf5b0cU,sizeof(current_model));
      const auto previous_model=state.private_model_pointer;
      if (current_model != previous_model) {
        if (previous_model >= 0x80000000U && previous_model < 0x80400000U &&
            current_model != 0U && (current_model < 0x80000000U || current_model >= 0x80400000U)) {
          FILE* private_model_capture_stream = nullptr;
          (void)fopen_s(&private_model_capture_stream,
                       (state.rdram_capture_path+".fault.txt").c_str(),"wb");
          FILE* const private_model_output = private_model_capture_stream != nullptr
                                               ? private_model_capture_stream : stderr;
        std::fprintf(private_model_output,
            "private-model-change vi=%llu poll=%llu pc=%08x last_pc=%08x word=%08x "
            "old=%08x new=%08x\n",
            static_cast<unsigned long long>(state.vi_retraces),
            static_cast<unsigned long long>(state.controller_samples),
            pc,state.guest_last_pc,word,previous_model,current_model);
          const auto count=(std::min)(state.private_model_probe_position,
                                      std::uint64_t{state.private_model_probe_pcs.size()});
          for (std::uint64_t distance=count; distance>0U; --distance)
            std::fprintf(private_model_output,"private-model-pc %08x\n",
                state.private_model_probe_pcs[(state.private_model_probe_position-distance)%
                                             state.private_model_probe_pcs.size()]);
          const auto& cpu=*static_cast<recomp_context*>(context);
          std::fprintf(private_model_output,
              "private-model-registers a0=%016llx a1=%016llx a2=%016llx a3=%016llx "
              "v0=%016llx v1=%016llx t0=%016llx t1=%016llx t2=%016llx t3=%016llx "
              "s0=%016llx s1=%016llx sp=%016llx ra=%016llx\n",
              cpu.r4,cpu.r5,cpu.r6,cpu.r7,cpu.r2,cpu.r3,cpu.r8,cpu.r9,cpu.r10,cpu.r11,
              cpu.r16,cpu.r17,cpu.r29,cpu.r31);
          std::fflush(private_model_output);
          if (private_model_capture_stream != nullptr) std::fclose(private_model_capture_stream);
          fail_closed_dispatch(state,"private-memory-watch","level-model-pointer-corrupt",pc);
        }
        state.private_model_pointer=current_model;
      }
      state.private_model_probe_pcs[state.private_model_probe_position++%
                                   state.private_model_probe_pcs.size()]=pc;
    }
    if (state.diagnostic_cpu_costs) ++state.profile_full_execution_probes;
    const bool measure_native_handler = state.diagnostic_cpu_costs && state.diagnostic_host_clock;
    const auto native_handler_began = measure_native_handler ? std::chrono::steady_clock::now()
        : std::chrono::steady_clock::time_point{};
    const auto native_handler_wait = measure_native_handler ? state.profile_host_wait_us : 0U;
    const auto native_handler_graphics = measure_native_handler ? state.graphics_submit_microseconds : 0U;
    if (state.eret_transfer_stream.is_open() &&
        !state.eret_transfer_probe.advance(static_cast<std::uint32_t>(state.completed_game_updates + 1U)))
      fail_closed_dispatch(state, "eret-transfer-probe", "output-or-order", pc);
    ++state.observed_guest_instructions;
    if (!state.guest_os_probe &&
        (word == 0x1000ffffU || ((word >> 26U) == 2U &&
          (((pc + 4U) & 0xf0000000U) | ((word & 0x03ffffffU) << 2U)) == pc))) {
      // Preserve the old cooperative control's permanent idle park even
      // when this root emits executable guest idle loops for the new CPU.
      pause_self(state.rdram);
    }
    if (state.guest_os_probe) {
      auto& cpu = *static_cast<recomp_context*>(context);
      auto& mmio = *state.mmio_trace;
      if (state.diagnostic_host_clock) {
        // Only the exhausted instruction quantum can take the checked poll
        // shortcut below. Every other reason for a full handler stays full.
        const bool poll_certificate = state.native_fast_ready &&
            state.host_clock_poll_budget == 1U;
        const auto now = std::chrono::steady_clock::now();
        if (!state.host_count_clock.ready() && !state.host_count_clock.start(state.cpu_count,now))
          fail_closed_dispatch(state,"native-clock","initialization",pc);
        auto count = state.native_clock_samples.replaying() ? std::optional<std::uint64_t>{}
            : state.host_count_clock.sample(now);
        count = state.native_clock_samples.sample(state.observed_guest_instructions,pc,word,0U,count);
        if (!count || *count < state.cpu_count)
          fail_closed_dispatch(state,"native-clock",state.native_clock_samples.limit_reached() ? "sample-limit" : "non-monotonic-time",pc);
        state.cpu_count = *count;
        mmio.guest_count = *count;
        state.host_clock_poll_budget = State::kHostClockPollInstructions;
        state.native_fast_ready = false;
        const unsigned poll_primary=word>>26U;
        const bool poll_fpu=poll_primary==0x11U || poll_primary==0x31U ||
            poll_primary==0x35U || poll_primary==0x39U || poll_primary==0x3dU;
        const bool poll_park=poll_primary==3U &&
            (((pc+4U)&0xf0000000U)|((word&0x03ffffffU)<<2U))==0x80075698U;
        // The clock is sampled at the identical instruction boundary above.
        // Reuse the existing device certificate only while no deadline has
        // arrived and no CPU or I/O owner has invalidated it. Hardware/cache,
        // idle parks and observer sites always retain the full path.
        if (poll_certificate && !state.guest_eret_boundary &&
            state.native_fast_status==cpu.status_reg &&
            state.execution_fast_revision==mmio.execution_io_revision &&
            state.cpu_count<state.execution_fast_deadline &&
            (state.play_mode || state.observed_guest_instructions<=24000000000ULL) &&
            poll_primary!=0x10U && poll_primary!=0x2fU &&
            word!=0x1000ffffU && pc!=0x8009ab80U && pc!=0x80098b74U &&
            !poll_park && (!poll_fpu || (cpu.status_reg&0x20000000U)!=0U) &&
            mmio.device_error==nullptr && !mmio.sp_launch &&
            mmio.flash_bus.mutations()==mmio.persisted_flash_mutations &&
            !state.guest_clock_trace.is_open() && state.point_probe_pcs.empty() &&
            !state.device_event_stream && !state.eret_transfer_stream.is_open() &&
            !state.instruction_effect_stream.is_open()) {
          if (measure_native_handler)
            state.profile_native_handler_ns+=static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now()-native_handler_began).count());
          const bool delay=state.guest_previous_branch && pc==state.guest_last_pc+4U;
          state.guest_last_pc=pc;state.guest_last_word=word;
          state.guest_previous_delay=delay;
          state.guest_previous_branch=poll_primary==1U ||
              (poll_primary>=2U && poll_primary<=7U) ||
              (poll_primary>=20U && poll_primary<=23U) ||
              (poll_primary==0U && ((word&63U)==8U || (word&63U)==9U)) ||
              (poll_primary==0x11U && ((word>>21U)&31U)==8U);
          // Refresh also clamps a diagnostic's remaining instruction budget.
          // Keep that boundary even when the device certificate is unchanged.
          jfg_native_timing_refresh(&state,&cpu);
          return;
        }
      }
      // A cached BEQ zero,zero,self + NOP loop has no architectural effect
      // between device events. Only skip after observing a complete warm
      // iteration and with no interrupt currently eligible for delivery.
      const auto pending_idle_interrupt =
          ((mmio.mi_pending & mmio.mi_mask.read()) ? 0x400U : 0U) |
          (state.guest_timer.interrupt() ? 0x8000U : 0U) | (state.guest_cause & 0x300U);
      if (word == 0x1000FFFFU && state.guest_last_pc == pc + 4U &&
          state.guest_last_word == 0U && state.guest_previous_delay &&
          (cpu.status_reg & 7U) == 1U &&
          (pending_idle_interrupt & cpu.status_reg & 0xFF00U) == 0U) {
        auto deadline = mmio.vi_next;
        const auto admit = [&](std::optional<std::uint64_t> next) {
          if (next) deadline = (std::min)(deadline,*next);
        };
        admit(mmio.pi_dma.deadline()); admit(mmio.si_dma.deadline());
        admit(state.guest_timer.deadline()); admit(mmio.sp_deadline);
        admit(mmio.dp_deadline); admit(mmio.ai_dma.deadline());
        if (deadline > state.cpu_count) {
          const auto ticks=deadline-state.cpu_count;
          if (state.diagnostic_host_clock) {
            const auto before = std::chrono::steady_clock::now();
            const auto target_time = state.native_clock_samples.replaying()
                ? std::optional<HostCountClock::Time>{before}
                : state.host_count_clock.deadline(deadline,before);
            if (!target_time) fail_closed_dispatch(state,"native-clock","idle-deadline",pc);
            if (!state.native_clock_samples.replaying() && !state.host_deadline_wait.until(*target_time))
              fail_closed_dispatch(state,"native-clock","host-deadline-wait",pc);
            const auto after = std::chrono::steady_clock::now();
            auto count = state.native_clock_samples.replaying() ? std::optional<std::uint64_t>{}
                : state.host_count_clock.sample(after);
            count = state.native_clock_samples.sample(state.observed_guest_instructions,pc,word,deadline,count);
            if (!count || *count < deadline)
              fail_closed_dispatch(state,"native-clock","early-idle-wakeup",pc);
            state.profile_host_wait_us += static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(after-before).count());
            if (state.gameplay_trace) {
              state.gameplay_trace.event("host-idle-wait", {state.vi_retraces,
                  state.cpu_count, deadline, *count,
                  static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(after-before).count()),
                  static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(after-*target_time).count())});
            }
            state.profile_idle_count_ticks += *count-state.cpu_count;
            state.cpu_count = *count;
            mmio.guest_count = *count;
          } else if (jfg_full_cache_idle_skip(pc,ticks,state.rdram)) {
            state.profile_idle_count_ticks += ticks;
            state.cpu_count=deadline;
            state.observed_guest_instructions+=ticks*2U;
            mmio.guest_count=state.cpu_count;
          }
        }
      }

      if (state.guest_clock_trace && mmio.traced_si_sequence != mmio.device_si_sequence) {
        state.guest_clock_trace << "si\t" << state.cpu_count << '\t' << std::hex
            << mmio.device_si_offset << '\t' << mmio.device_si_value << '\t'
            << mmio.si_dma.read(0).value_or(0) << std::dec << '\t'
            << mmio.device_si_prior_deadline.value_or(0) << '\t' << mmio.si_dma.transfers() << '\n';
        mmio.traced_si_sequence = mmio.device_si_sequence;
      }
      if (state.guest_clock_trace && (mmio.device_error ||
          mmio.traced_pi_transfers != mmio.pi_dma.transfers())) {
        state.guest_clock_trace << "pi\t" << state.cpu_count << '\t' << std::hex
            << mmio.device_pi_offset << '\t' << mmio.device_pi_value << '\t'
            << mmio.pi_dma.read(0U).value_or(0U) << '\t' << mmio.pi_dma.read(4U).value_or(0U)
            << std::dec << '\n';
        state.guest_clock_trace.flush();
        mmio.traced_pi_transfers = mmio.pi_dma.transfers();
      }
      if (state.guest_clock_trace && (pc == 0x8009ab80U || pc == 0x80098b74U))
        state.guest_clock_trace << "vi-site\t" << std::hex << pc << std::dec << '\t' << state.cpu_count << '\n';
      if (mmio.device_error != nullptr) {
        if (state.guest_clock_trace) {
          state.guest_clock_trace << "device-error\t" << mmio.device_error << '\t' << state.cpu_count
              << '\t' << std::hex << mmio.device_write_page << '\t' << mmio.device_write_offset
              << '\t' << mmio.device_write_value << std::dec << '\n';
          const auto dram = mmio.si_dma.read(0).value_or(UINT32_MAX);
          state.guest_clock_trace << "si-error\t" << std::hex << mmio.device_si_offset << '\t'
              << mmio.device_si_value << '\t' << dram << '\t';
          if (dram <= kRdramSize - 64U)
            for (unsigned i = 0; i < 64; ++i)
              state.guest_clock_trace << std::setw(2) << std::setfill('0') << unsigned(mmio.device_rdram[(dram + i) ^ 3U]);
          state.guest_clock_trace << std::dec << '\n';
          state.guest_clock_trace << "pif-error\t";
          for (const auto value : mmio.si_dma.pif())
            state.guest_clock_trace << std::hex << std::setw(2) << std::setfill('0') << unsigned(value);
          state.guest_clock_trace << std::dec << '\n';
          state.guest_clock_trace.flush();
        }
        fail_closed_dispatch(state, "guest-device", mmio.device_error, pc);
      }
      if (mmio.flash_bus.mutations() != mmio.persisted_flash_mutations) {
        if (!state.flash_path.empty() && !state.flashram.persist_atomic(state.flash_path).ok())
          fail_closed_dispatch(state, "guest-device", "flash-persist", pc);
        mmio.persisted_flash_mutations = mmio.flash_bus.mutations();
        ++state.flash_persist_count;
      }
      // No route-dependent budget or timing adjustment. Bound the diagnostic
      // until every reached device has an independently qualified owner.
      if (!state.play_mode && state.observed_guest_instructions > 24000000000ULL)
        fail_closed_dispatch(state, "guest-os", "instruction-budget", pc);
      if (mmio.sp_launch) {
        hle::GuestMemory memory({state.rdram, kRdramSize}, hle::GuestMemory::Layout::native_word_big_endian);
        std::uint32_t flags = 0, device_type = 0;
        std::memcpy(&device_type, mmio.sp_dma.memory().data() + 0xfc0U, 4U);
        if (state.guest_active_rsp_task || state.loaded_rsp_task == 0 ||
            (state.loaded_rsp_task_type != 1 && state.loaded_rsp_task_type != 2) ||
            device_type != state.loaded_rsp_task_type ||
            !memory.read_u32(state.loaded_rsp_task + 4, flags) || (flags & ~2U) != 0)
          fail_closed_dispatch(state, "guest-rsp", "unqualified-task-launch", pc);
        state.guest_active_rsp_task = state.loaded_rsp_task;
        state.guest_active_rsp_type = device_type;
        mmio.sp_deadline = *mmio.sp_launch + (device_type == 1 ? 1000U : 4000U);
        if (device_type == 1) {
          if (mmio.dp_deadline || !state.pending_graphics_tasks.empty() ||
              !execute_live_graphics_task(state, memory))
            fail_closed_dispatch(state, "guest-rsp", "graphics-task-rejected", pc);
          mmio.dp_deadline = mmio.sp_deadline;
        }
        mmio.sp_launch.reset();
      }
      const bool device_boundary = state.guest_previous_delay || state.guest_eret_boundary;
      if (device_boundary) {
        std::array<ReferenceEventCandidate, 7> candidates{};
        std::size_t event_count = 0;
        const auto add = [&](ReferenceEvent source, std::optional<std::uint64_t> deadline) {
          if (deadline) candidates[event_count++] = {source, *deadline};
        };
        add(ReferenceEvent::vi, mmio.vi_next);
        add(ReferenceEvent::pi, mmio.pi_dma.deadline());
        add(ReferenceEvent::si, mmio.si_dma.deadline());
        add(ReferenceEvent::compare, state.guest_timer.deadline());
        add(ReferenceEvent::sp, mmio.sp_deadline);
        add(ReferenceEvent::dp, mmio.dp_deadline);
        add(ReferenceEvent::ai, mmio.ai_dma.deadline());
        const auto events = std::span(candidates).first(event_count);
        const auto reference_selected = state.diagnostic_host_clock ? ReferenceEventSelection{}
            : select_reference_event(events, state.cpu_count);
        const auto native_batch = state.diagnostic_host_clock
            ? select_native_event_batch(events, state.cpu_count) : NativeEventBatch{};
        if (!native_batch.valid)
          fail_closed_dispatch(state, "native-clock", "invalid-device-batch", pc);
        // No guest CPU instruction or interrupt acceptance occurs within this
        // equal-deadline batch. Device transfer data is already visible; each
        // completion updates its own status/latch. SP work finishes before the
        // final CPU interrupt observation, including when DP shares its deadline.
        if (state.diagnostic_host_clock && native_batch.size > 1U)
          state.gameplay_trace.event("native-device-batch", {state.vi_retraces,
              state.cpu_count, native_batch.deadline, native_batch.size});
        const auto completions = state.diagnostic_host_clock
            ? (std::max)(native_batch.size, std::size_t{1}) : std::size_t{1};
        for (std::size_t completion = 0; completion < completions; ++completion) {
        auto selected = state.diagnostic_host_clock
            ? ReferenceEventSelection{true, native_batch.size ?
                std::optional<ReferenceEvent>{native_batch.sources[completion]} : std::nullopt}
            : reference_selected;
        if (!selected.qualified && state.play_mode &&
            community_env_number("JFG_STRICT_EVENT_TIES", 0U) == 0U) {
          // Community: two devices due on the same cycle (seen: SP and AI at
          // VI 31293 on the title, 2026-10-19) stopped the game. Complete the
          // lower source first, in the native batch's fixed order (VI, PI,
          // SI, Compare, SP, DP, AI); the other stays due and completes at the
          // next boundary. JFG_STRICT_EVENT_TIES=1 restores the stop.
          const auto tie = select_native_event_batch(events, state.cpu_count);
          if (tie.valid && tie.size != 0U) {
            selected = ReferenceEventSelection{true, tie.sources[0]};
            ++state.community_event_ties;
            if (state.community_effect_stream.is_open())
              state.community_effect_stream << "TIE\t" << state.community_effect_frame << '\t'
                  << state.vi_retraces << '\t' << state.cpu_count << '\t' << tie.size << '\t'
                  << static_cast<unsigned>(tie.sources[0]) << '\n';
          }
        }
        if (!selected.qualified) {
          // Preserve fail-closed ordering; capture evidence instead of guessing.
          std::cerr << "guest-event-tie Count=" << state.cpu_count << " VI=" << state.vi_retraces;
          for (const auto& candidate : std::span(candidates).first(event_count))
            std::cerr << " source=" << static_cast<unsigned>(candidate.source) << " deadline=" << candidate.deadline;
          std::cerr << '\n';
          fail_closed_dispatch(state, "guest-device", "unqualified-event-tie", pc);
        }
        std::optional<std::uint64_t> observed_deadline;
        unsigned observed_source = JFG_EVENT_NONE;
        if (state.device_event_stream && selected.source) {
          switch (*selected.source) {
          case ReferenceEvent::vi: observed_source = JFG_EVENT_VI; break;
          case ReferenceEvent::pi: observed_source = JFG_EVENT_PI; break;
          case ReferenceEvent::si: observed_source = JFG_EVENT_SI; break;
          case ReferenceEvent::compare: observed_source = JFG_EVENT_COMPARE; break;
          case ReferenceEvent::sp: observed_source = JFG_EVENT_SP; break;
          case ReferenceEvent::dp: observed_source = JFG_EVENT_DP; break;
          case ReferenceEvent::ai: observed_source = JFG_EVENT_AI; break;
          }
          for (const auto candidate : std::span(candidates).first(event_count))
            if (candidate.source == *selected.source) observed_deadline = candidate.deadline;
          emit_device_event(state, cpu, JFG_EVENT_DISPATCH, observed_source, pc, word, observed_deadline);
        }
        if (selected.source) switch (*selected.source) {
        case ReferenceEvent::vi:
          mmio.mi_pending |= 8U;
          ++state.vi_frames;
          // Host playback follows the original VI device clock. This does not
          // advance Count, invent a guest event, or adjust a cinematic cue.
          if (state.original_os_host_pacing && !state.diagnostic_host_clock && state.host_audio.started()) {
            if (!state.original_os_host_anchor_ready) {
              state.host_audio.settle_startup_queue();
              state.original_os_host_clock.anchor(mmio.vi_next,
                  std::chrono::steady_clock::now(), state.host_audio.underruns());
              state.original_os_host_anchor_ready = true;
            }
            // Poll actual output continuity before pacing, including stalls in
            // the window/renderer owner thread. Never catch up a stale clock
            // after the audio device has run dry.
            state.host_audio.service(false, false);
            const auto target_time = state.original_os_host_clock.deadline(
                mmio.vi_next, std::chrono::steady_clock::now(),
                state.host_audio.underruns());
            const auto before_wait = std::chrono::steady_clock::now();
            if (before_wait < target_time)
              std::this_thread::sleep_until(target_time);
            const auto now = std::chrono::steady_clock::now();
            community_audio_watch(state, static_cast<std::int64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    before_wait - target_time).count()));
            if (state.gameplay_trace) {
              state.profile_host_wait_us += static_cast<std::uint64_t>(
                  std::chrono::duration_cast<std::chrono::microseconds>(now-before_wait).count());
              state.gameplay_trace.event("vi-work", {state.vi_retraces, state.cpu_count,
                  state.observed_guest_instructions, state.profile_idle_count_ticks,
                  state.profile_host_wait_us, cache_stats[2], cache_stats[6], cache_stats[10]});
            }
            if (state.timing_trace) state.timing_trace << "os-host-vi\t"
                << state.vi_frames << '\t' << mmio.vi_next << '\t'
                << std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count()
                << '\t' << std::chrono::duration_cast<std::chrono::microseconds>(now-target_time).count()
                << '\n';
          }
          if (state.diagnostic_host_clock) {
            if (!state.original_os_host_anchor_ready && state.host_audio.started()) {
              state.host_audio.settle_startup_queue();
              state.original_os_host_anchor_ready = true;
            }
            state.gameplay_trace.event("vi-work", {state.vi_retraces, state.cpu_count,
                state.observed_guest_instructions, state.profile_idle_count_ticks,
                state.profile_host_wait_us, cache_stats[2], cache_stats[6], cache_stats[10]});
          }
          if (state.timing_trace) {
            hle::GuestMemory observed({state.rdram,kRdramSize},hle::GuestMemory::Layout::native_word_big_endian);
            std::uint32_t loading=0;
            if (observed.read_u32(0x800A3530U,loading) && loading!=0U) {
              std::uint16_t level=0,next=0;
              std::memcpy(&level,state.rdram+(0xA323CU^2U),sizeof(level));
              std::memcpy(&next,state.rdram+(0xA3250U^2U),sizeof(next));
              const auto original_address=[&](std::uint32_t value) {
                if(value<0x00100000U || value>=0x10000000U)return value;
                std::uint32_t table=0,base=0;
                if (!observed.read_u32(0x800FEAA0U,table) ||
                    !observed.read_u32(table+(value>>20U)*32U,base))return 0U;
                return base+(value&0xFFFFFU);
              };
              state.timing_trace << "load-pc\t" << state.vi_frames << '\t'
                  << loading << '\t' << level << '\t' << next << '\t'
                  << std::hex << original_address(pc) << '\t'
                  << original_address(static_cast<std::uint32_t>(cpu.r31)) << '\t'
                  << static_cast<std::uint32_t>(cpu.r29) << '\t'
                  << mmio.sp_status.read() << '\t' << mmio.pi_dma.read(0x10U).value_or(0U)
                  << std::dec << '\n';
            }
          }

          jfg::support_breadcrumb(state.vi_frames, state.vi_retraces, state.controller_samples);
          if (mmio.vi_v_sync != 0U && mmio.vi_v_sync != 525U)
            fail_closed_dispatch(state, "guest-os", "unqualified-vi-mode", pc);
          if (mmio.vi_v_sync != 0U && mmio.vi_h_sync != 3093U)
            fail_closed_dispatch(state, "guest-os", "unqualified-hardware-hsync", pc);
          // Same independently derived fixed progressive profile as the paired
          // Ares comparison; this does not implement arbitrary VI register modes.
          mmio.vi_period = mmio.vi_v_sync == 0U ? 500000U : 783520U;
          if (state.diagnostic_host_clock) {
            const auto advanced = advance_native_periodic_deadline(
                state.cpu_count, mmio.vi_next, mmio.vi_period);
            if (!advanced || advanced->elapsed == 0U)
              fail_closed_dispatch(state,"native-clock","vi-deadline-overflow",pc);
            if (advanced->elapsed > 1U)
              state.gameplay_trace.event("native-vi-coalesced", {state.vi_retraces,
                  state.cpu_count, mmio.vi_next, advanced->next,
                  advanced->elapsed, mmio.vi_period});
            mmio.vi_next = advanced->next;
          } else {
            mmio.vi_next += mmio.vi_period;
          }
          if (!service_host_audio(state, true))
            fail_closed_dispatch(state, "audio", "host-device", pc);
          break;
        case ReferenceEvent::pi: mmio.pi_dma.advance(state.cpu_count); break;
        case ReferenceEvent::si: mmio.si_dma.advance(state.cpu_count); break;
        case ReferenceEvent::compare: state.guest_timer.advance(state.cpu_count); break;
        case ReferenceEvent::ai: mmio.ai_dma.advance(state.cpu_count); break;
        case ReferenceEvent::sp: {
          if (state.guest_active_rsp_type == 1) complete_pending_live_graphics_tasks(state, pc);
          else if (state.guest_active_rsp_type == 2) {
            hle::GuestMemory memory({state.rdram, kRdramSize}, hle::GuestMemory::Layout::native_word_big_endian);
            const auto loaded = state.loaded_rsp_task;
            state.loaded_rsp_task = state.guest_active_rsp_task;
            if (!execute_live_audio_task(state, memory))
              fail_closed_dispatch(state, "guest-rsp", "audio-task-rejected", pc);
            state.loaded_rsp_task = loaded;
          } else fail_closed_dispatch(state, "guest-rsp", "completion-without-task", pc);
          if (!mmio.sp_status.complete_task()) fail_closed_dispatch(state, "guest-rsp", "completion-status", pc);
          mmio.sp_deadline.reset();
          state.guest_active_rsp_task = state.guest_active_rsp_type = 0;
          break;
        }
        case ReferenceEvent::dp: mmio.mi_pending |= 32U; mmio.dp_deadline.reset(); break;
        }
        mmio.mi_pending = (mmio.mi_pending & ~1U) | (mmio.sp_status.interrupt() ? 1U : 0U);
        mmio.mi_pending = (mmio.mi_pending & ~2U) | (mmio.si_dma.interrupt() ? 2U : 0U);
        mmio.mi_pending = (mmio.mi_pending & ~4U) | (mmio.ai_dma.interrupt() ? 4U : 0U);
        mmio.mi_pending = (mmio.mi_pending & ~16U) | (mmio.pi_dma.interrupt() ? 16U : 0U);
        if (observed_source != JFG_EVENT_NONE)
          emit_device_event(state, cpu, JFG_EVENT_STATE, observed_source, pc, word, observed_deadline);
        } // Equal-time completions are visible together at the CPU boundary.
      }
      state.vi_interrupts = mmio.vi_acknowledgements;
      if (measure_native_handler) {
        // Measure only the prefix before any guest exception handler or thread
        // handoff can run. Subtract the already measured explicit waits and
        // graphics submission; nested guest execution must not be double counted.
        const auto elapsed = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now()-native_handler_began).count());
        const auto excluded = (state.profile_host_wait_us-native_handler_wait+
            state.graphics_submit_microseconds-native_handler_graphics)*1000U;
        state.profile_native_handler_ns += elapsed > excluded ? elapsed-excluded : 0U;
      }
      bool delay = state.guest_previous_branch && pc == state.guest_last_pc + 4U;
      const auto cpu_pending = ((mmio.mi_pending & mmio.mi_mask.read()) ? 0x400U : 0U) |
          (state.guest_timer.interrupt() ? 0x8000U : 0U) | (state.guest_cause & 0x300U);
      const bool external = (cpu_pending & cpu.status_reg & 0xff00U) != 0U &&
          (cpu.status_reg & 7U) == 1U &&
          (device_boundary || (state.guest_last_word & 0xffe0ffffU) == 0x40806000U);
      state.guest_eret_boundary = false;
      const unsigned primary = word >> 26U;
      const bool fpu = primary == 0x11U || primary == 0x31U || primary == 0x35U || primary == 0x39U || primary == 0x3dU;
      const bool unusable = fpu && (cpu.status_reg & 0x20000000U) == 0U;
      if (external || unusable) {
        if (external)
          emit_device_event(state, cpu, JFG_EVENT_ACCEPT, JFG_EVENT_NONE, pc, word);
        if ((cpu.status_reg & 2U) != 0U || (unusable && delay))
          fail_closed_dispatch(state, "guest-os", "unqualified-exception-boundary", pc);
        const auto interrupted_pc = pc;
        state.guest_transport->park_at(pc);
        state.guest_epc = pc;
        state.guest_cause &= ~0x8000007cU;
        if (unusable) state.guest_cause = (state.guest_cause & ~0x30000000U) | 0x1000002cU;
        cop0_status_write(&cpu, cpu.status_reg | 2U);
        ++state.guest_exceptions;
        state.guest_previous_branch = state.guest_previous_delay = false;
        hle::GuestMemory memory({state.rdram, kRdramSize}, hle::GuestMemory::Layout::native_word_big_endian);
        for (std::uint32_t offset = 0U; offset < 16U; offset += 4U) {
          std::uint32_t source = 0U, installed = 0U;
          if (!memory.read_u32(0x80075020U + offset, source) ||
              !memory.read_u32(0x80000180U + offset, installed) || source != installed)
            fail_closed_dispatch(state, "guest-os", "exception-vector-mismatch", pc);
        }
        if (state.guest_clock_trace.is_open()) {
          state.guest_clock_trace << "exception\t" << std::hex << pc << '\t' << mmio.mi_pending
              << '\t' << state.guest_cause << std::dec << '\t' << state.cpu_count << '\n';
          state.guest_clock_trace.flush();
        }
        const auto handler = jfg_generated_lookup_function(static_cast<std::int32_t>(0x80075020U));
        if (handler == nullptr) fail_closed_dispatch(state, "guest-os", "exception-handler-unavailable", pc);
        handler(state.rdram, &cpu);
        if (state.guest_epc != interrupted_pc || (cpu.status_reg & 2U) != 0U)
          fail_closed_dispatch(state, "guest-os", "exception-resume-mismatch", pc);
        delay = false;
      }
      if (state.device_event_stream && !state.device_event_probe.complete) {
        const auto invocation = state.completed_game_updates + 1U;
        if (invocation > state.device_event_probe.last) {
          if (!jfg_device_event_finish(&state.device_event_probe))
            fail_closed_dispatch(state, "device-event-probe", "output-failed", pc);
        } else if (invocation >= state.device_event_probe.first) {
          hle::GuestMemory memory({state.rdram, kRdramSize}, hle::GuestMemory::Layout::native_word_big_endian);
          std::uint32_t thread = 0U;
          if (!memory.read_u32(0x800a9e90U, thread))
            fail_closed_dispatch(state, "device-event-probe", "thread-read", pc);
          if (thread != state.device_event_previous_thread)
            emit_device_event(state, cpu, JFG_EVENT_THREAD, JFG_EVENT_NONE, pc, word);
          if (state.device_event_successor)
            emit_device_event(state, cpu, JFG_EVENT_SUCCESSOR, JFG_EVENT_NONE, pc, word);
          const bool selected_pc = std::find(state.point_probe_pcs.begin(), state.point_probe_pcs.end(), pc) != state.point_probe_pcs.end();
          if (selected_pc) emit_device_event(state, cpu, JFG_EVENT_INSTRUCTION, JFG_EVENT_NONE, pc, word);
          state.device_event_successor = selected_pc;
          state.device_event_previous_thread = thread;
          state.device_event_previous_pc = pc;
          state.device_event_previous_word = word;
        }
      }
      // Mupen's annulled ordinary likely slots still advance Count. Other
      // likely families require their own profile coverage before use.
      if (!state.diagnostic_host_clock) {
        const auto cycles = jfg_full_cache_cost(pc,word,&cpu,state.rdram)
            + state.diagnostic_cpu_half_tick;
        state.cpu_count += cycles / 2U;
        state.diagnostic_cpu_half_tick = cycles % 2U;
      }
      mmio.guest_count = state.cpu_count;
      state.guest_last_pc = pc;
      state.guest_last_word = word;
      state.guest_previous_delay = delay;
      state.guest_previous_branch = primary == 1U || (primary >= 2U && primary <= 7U) ||
          (primary >= 20U && primary <= 23U) ||
          (primary == 0U && ((word & 63U) == 8U || (word & 63U) == 9U)) ||
          (primary == 0x11U && ((word >> 21U) & 31U) == 8U);
      if (primary == 3U && (((pc + 4U) & 0xf0000000U) | ((word & 0x03ffffffU) << 2U)) == 0x80075698U)
        state.guest_transport->park_at(pc + 8U);
      state.execution_fast_deadline = mmio.vi_next;
      const auto fast_admit = [&](std::optional<std::uint64_t> next) {
        if (next) state.execution_fast_deadline =
            (std::min)(state.execution_fast_deadline,*next);
      };
      fast_admit(mmio.pi_dma.deadline()); fast_admit(mmio.si_dma.deadline());
      fast_admit(state.guest_timer.deadline()); fast_admit(mmio.sp_deadline);
      fast_admit(mmio.dp_deadline); fast_admit(mmio.ai_dma.deadline());
      state.execution_fast_revision = mmio.execution_io_revision;
      state.execution_fast_pending =
          ((mmio.mi_pending & mmio.mi_mask.read()) ? 0x400U : 0U) |
          (state.guest_timer.interrupt() ? 0x8000U : 0U) | (state.guest_cause & 0x300U);
      if (state.diagnostic_host_clock) jfg_native_timing_refresh(&state,&cpu);

      // After exception resumption and Count accounting, before the current
      // guest instruction. No clock reads/changes or guest writes in this hook.
      // Raw Count is deliberately omitted: the oracle exposes lazy core Count.
      if (!state.point_probe_pcs.empty() &&
          state.completed_game_updates + 1U >= state.focus_update_start - 1U &&
          state.completed_game_updates + 1U <= state.focus_update_end + 1U &&
          std::find(state.point_probe_pcs.begin(), state.point_probe_pcs.end(), pc) != state.point_probe_pcs.end()) {
        if (++state.point_probe_hits > 4096U)
          fail_closed_dispatch(state, "point-probe", "hit-budget", pc);
        hle::GuestMemory memory({state.rdram, kRdramSize}, hle::GuestMemory::Layout::native_word_big_endian);
        std::uint32_t opcode = 0;
        if (!memory.read_u32(pc, opcode) || opcode != word)
          fail_closed_dispatch(state, "point-probe", "instruction-bytes", pc);
        auto& out = state.point_probe_trace;
        out << state.completed_game_updates + 1U << '\t' << state.vi_retraces << '\t'
            << state.controller_samples << '\t' << "0x" << std::hex << std::setfill('0')
            << std::setw(8) << pc << '\t' << "0x" << std::setw(8) << opcode;
        const std::array<std::uint64_t, 32> registers = {
            0, cpu.r1, cpu.r2, cpu.r3, cpu.r4, cpu.r5, cpu.r6, cpu.r7,
            cpu.r8, cpu.r9, cpu.r10, cpu.r11, cpu.r12, cpu.r13, cpu.r14, cpu.r15,
            cpu.r16, cpu.r17, cpu.r18, cpu.r19, cpu.r20, cpu.r21, cpu.r22, cpu.r23,
            cpu.r24, cpu.r25, cpu.r26, cpu.r27, cpu.r28, cpu.r29, cpu.r30, cpu.r31};
        for (const auto value : registers)
          out << '\t' << "0x" << std::setw(8) << static_cast<std::uint32_t>(value)
              << '\t' << "0x" << std::setw(8) << static_cast<std::uint32_t>(value >> 32U);
        for (const auto address : state.point_probe_words) {
          std::uint32_t value = 0;
          if (!memory.read_u32(address, value))
            fail_closed_dispatch(state, "point-probe", "word-read", pc);
          out << '\t' << "0x" << std::setw(8) << value;
        }
        out << std::dec << std::setfill(' ') << '\n';
        out.flush();
        if (!out) fail_closed_dispatch(state, "point-probe", "output-failed", pc);
      }
    }
  }
#endif
  (void)section; (void)pc; (void)word; (void)context;
}

bool read_supported_rom(const char *path, std::vector<std::uint8_t> &rom) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input || input.tellg() != static_cast<std::streamoff>(kRomSize))
    return false;
  input.seekg(0);
  rom.resize(kRomSize);
  return input.read(reinterpret_cast<char *>(rom.data()),
                    static_cast<std::streamsize>(rom.size())) &&
         sha1(rom) == kSupportedRomSha1;
}

int run_child(const char *path, const unsigned retrace_target,
              const unsigned poll_target, const bool play_mode) {
  std::vector<std::uint8_t> rom;
  if (!read_supported_rom(path, rom))
    return 2;
  if constexpr (std::endian::native != std::endian::little) {
    std::fputs("native boot setup failed: unsupported host endianness\n",
               stderr);
    return 3;
  }
  std::vector<std::uint8_t> bytes(kRdramSize);
  GuestBacking backing;
  if (!backing.create() || !backing.load_overlay_images(rom)) {
    std::fputs("native boot setup failed: guest address space\n", stderr);
    return 3;
  }
  const std::span<std::uint8_t> rdram = backing.bytes();
  if (!load_static_sections(rom, bytes)) {
    std::fputs("native boot setup failed: static section validation\n", stderr);
    return 3;
  }
  const auto handoff = interpret_reset_handoff(
      bytes, 0x80000400U, kRdramSize / sizeof(std::uint32_t) + 4096U);
  if (!handoff.ok()) {
    const char *reason =
        handoff.error == ResetHandoffError::unsupported_instruction
            ? "unsupported-instruction"
        : handoff.error == ResetHandoffError::budget_exhausted    ? "budget"
        : handoff.error == ResetHandoffError::unaligned_access    ? "unaligned"
        : handoff.error == ResetHandoffError::arithmetic_overflow ? "overflow"
                                                                  : "memory";
    std::fprintf(stderr, "native boot setup failed: reset handoff %s\n",
                 reason);
    return 3;
  }
  if (!initialize_ipl_state(bytes, rom)) {
    std::fputs("native boot setup failed: IPL state\n", stderr);
    return 3;
  }
  {
    // Community build: widescreen is the default for fresh saves. The game
    // copies its default global flags from 0x800A34AC when the flash copy is
    // missing or invalid; byte 108 bit 0x40 is the Options widescreen setting
    // (stock default 0x90). Existing saves keep the player's own choice.
    // JFG_DEFAULT_WIDESCREEN=0 restores the stock 4:3 default.
    char wide_flag[2]{};
    const bool stock_default =
        GetEnvironmentVariableA("JFG_DEFAULT_WIDESCREEN", wide_flag, 2U) == 1U &&
        wide_flag[0] == '0';
    constexpr std::uint32_t kDefaultScreenFlags = 0x800A34ACU + 108U;
    const std::size_t at = kDefaultScreenFlags - kKseg0;
    if (!stock_default && at < bytes.size() && bytes[at] == 0x90U)
      bytes[at] = static_cast<std::uint8_t>(bytes[at] | 0x40U);
  }
  for (std::size_t index = 0U; index < rdram.size(); index += 4U) {
    rdram[index] = bytes[index + 3U];
    rdram[index + 1U] = bytes[index + 2U];
    rdram[index + 2U] = bytes[index + 1U];
    rdram[index + 3U] = bytes[index];
  }
  jfg::GeneratedOverlayRuntime runtime(std::as_writable_bytes(rdram));
  State state{};
  // Controller setup can inspect gameplay context before the renderer starts.
  state.rdram = rdram.data();
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  char mod_flag[2]{};
  if (GetEnvironmentVariableA("JFG_NAVIGATION_MOD",mod_flag,2U)==1U && mod_flag[0]=='1') {
    wchar_t output[32768]{};
    const DWORD length=GetEnvironmentVariableW(L"JFG_MOD_OUTPUT",output,32768U);
    if(length==0U || length>=32768U) {
      std::fputs("navigation mod requires JFG_MOD_OUTPUT\n",stderr);return 2;
    }
    state.navigation_output=std::filesystem::path(output);
    std::error_code error;std::filesystem::create_directories(state.navigation_output,error);
    if(error)return 2;
    state.navigation_mod.enabled=true;
    // File open/flush stays on the diagnostic worker, never the guest thread.
    (void)state.movement_trace.start(state.navigation_output/L"movement-samples.csv", 4U*1024U*1024U);
    (void)state.jump_trace.start(state.navigation_output/L"jump-samples.csv", 4U*1024U*1024U);
    jfg::support_event("mod=navigation-enabled");
    char speed_flag[2]{};
    state.navigation_mod.juno_double_run =
        GetEnvironmentVariableA("JFG_MOD_JUNO_DOUBLE_RUN", speed_flag, 2U) == 1U &&
        speed_flag[0] == '1';
    if (state.navigation_mod.juno_double_run)
      jfg::support_event("mod=juno-run-speed-2x-enabled");
  }
  // Live tools observe by default; map Mods explicitly opt into individual changes.
  if (!state.navigation_mod.enabled) {
    wchar_t output[32768]{};
    const DWORD length=GetEnvironmentVariableW(L"JFG_LIVE_OUTPUT",output,32768U);
    if (length != 0U && length < 32768U) {
      state.navigation_output=std::filesystem::path(output);
      std::error_code error;std::filesystem::create_directories(state.navigation_output,error);
      if (!error) state.navigation_mod.telemetry_enabled=true;
    }
  }
  char memory_trace_flag[2]{};
  const auto memory_trace_length=GetEnvironmentVariableA("JFG_GAMEPLAY_TRACE_MEMORY",memory_trace_flag,2U);
  if(memory_trace_length!=0U && (memory_trace_length!=1U || memory_trace_flag[0]!='1'))return 3;
  wchar_t gameplay_trace_path[32768]{};
  const DWORD trace_path_length = GetEnvironmentVariableW(L"JFG_GAMEPLAY_TRACE", gameplay_trace_path, 32768U);
  if (trace_path_length != 0U && (trace_path_length >= 32768U ||
      !state.gameplay_trace.open(std::filesystem::path(gameplay_trace_path),256ULL*1024ULL*1024ULL,memory_trace_length==1U))) {
    std::fputs("native boot setup failed: gameplay trace path\n", stderr);
    return 3;
  }
  char trace_actors[2]{};
  const auto trace_actors_length = GetEnvironmentVariableA("JFG_GAMEPLAY_TRACE_ACTORS", trace_actors, 2U);
  if (trace_actors_length != 0U && (trace_actors_length != 1U ||
      (trace_actors[0] != '0' && trace_actors[0] != '1'))) {
    std::fputs("native boot setup failed: gameplay trace actors\n", stderr);
    return 3;
  }
  state.gameplay_trace_actors = trace_actors_length != 1U || trace_actors[0] != '0';
  state.host_audio.set_gameplay_trace(&state.gameplay_trace);
  state.host_audio.set_vi_counter(&state.vi_retraces);
  g_active_child_state = &state;
  jfg_original_timing_state = &state;
  state.instruction_cache = ic; state.data_cache = dc; state.cache_counters = cache_stats;
  jfg::support_event("native=rom-ready");
  wchar_t mapping_path[32768]{};
  const DWORD mapping_length = GetEnvironmentVariableW(L"JFG_CONTROLLER_CONFIG", mapping_path, 32768U);
  if (mapping_length != 0U) {
    state.controller_config_path=std::filesystem::path(mapping_path);
    std::ifstream mapping_file(std::filesystem::path(mapping_path), std::ios::binary | std::ios::ate);
    if (mapping_length >= 32768U || !mapping_file || mapping_file.tellg() > 4096 || mapping_file.tellg() <= 0) {
      jfg::support_event("native=controller-invalid");
      return 2;
    }
    mapping_file.seekg(0);
    const std::string mapping_text{std::istreambuf_iterator<char>(mapping_file), std::istreambuf_iterator<char>()};
    if (!jfg::parse_controller_mapping(mapping_text, state.controller_mapping)) {
      jfg::support_event("native=controller-invalid");
      return 2;
    }
  }
  state.controller_ports.configure(0,state.controller_mapping);
  sample_controller_ports(state,false);
  // One opt-in runtime mode owns CPU, OS, device clocks and host pacing.
  // Diagnostic traces and the travel fixture are independent and optional.
  char timing_fixture_flag[2]{};
  state.timing_fixture = GetEnvironmentVariableA("JFG_TIMING_TEST_FIXTURE",timing_fixture_flag,2U)==1U &&
      timing_fixture_flag[0]=='1';
  char original_timing_flag[2]{};
  const auto timing_override = GetEnvironmentVariableA("JFG_ORIGINAL_TIMING", original_timing_flag, 2U);
  const bool original_timing_mode = timing_override == 1U ? original_timing_flag[0] == '1' :
      JFG_ORIGINAL_TIMING_GENERATION == 1;
  if (original_timing_mode && JFG_ORIGINAL_TIMING_GENERATION != 1 && timing_override != 1U)
    return 3;
  char writeback_flag[2]{};
  state.renderer_writeback_probe = original_timing_mode || (
      GetEnvironmentVariableA("JFG_PHASE9_RENDERER_WRITEBACK_PROBE", writeback_flag, 2U) == 1U &&
      writeback_flag[0] == '1');
  char leaf_flag[2]{};
  state.guest_leaf_probe = original_timing_mode || (
      GetEnvironmentVariableA("JFG_PHASE9_GUEST_LEAF_PROBE", leaf_flag, 2U) == 1U &&
      leaf_flag[0] == '1');
  char os_flag[2]{};
  state.guest_os_probe = original_timing_mode || (
      GetEnvironmentVariableA("JFG_PHASE9_GUEST_OS_PROBE", os_flag, 2U) == 1U && os_flag[0] == '1');
  char original_pacing_flag[2]{};
  state.original_os_host_pacing = original_timing_mode || (
      GetEnvironmentVariableA("JFG_DIAGNOSTIC_OS_VI_PACING", original_pacing_flag, 2U) == 1U &&
      original_pacing_flag[0] == '1');
  if (state.original_os_host_pacing && !state.guest_os_probe) return 3;
  if (state.guest_os_probe && !state.guest_leaf_probe) {
    std::fputs("native boot setup failed: guest OS probe requires the guest CPU root\n", stderr);
    return 3;
  }
  if (state.guest_leaf_probe)
    state.tlb_registers = jfg::boot::TlbRegisters32::observed_mupen_boot();
  (void)SetUnhandledExceptionFilter(child_exception_filter);
#endif
  state.retrace_target =
      play_mode || poll_target != 0U
          ? (std::numeric_limits<std::uint32_t>::max)()
          : retrace_target;
  state.poll_target = poll_target;
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  state.play_mode = play_mode;
#if defined(_WIN32)
  char *fast_replay = nullptr;
  std::size_t fast_replay_size = 0U;
  if (_dupenv_s(&fast_replay, &fast_replay_size,
                "JFG_PHASE9_FAST_REPLAY") != 0) {
    std::fputs("native boot setup failed: fast replay environment\n",
               stderr);
    return 3;
  }
  if (fast_replay != nullptr && *fast_replay != '\0') {
    if (std::strcmp(fast_replay, "1") != 0) {
      std::free(fast_replay);
      std::fputs("native boot setup failed: fast replay value\n", stderr);
      return 3;
    }
    state.fast_replay = true;
  }
  std::free(fast_replay);
  char *capture = nullptr;
  std::size_t capture_size = 0U;
  if (_dupenv_s(&capture, &capture_size, "JFG_PHASE8_FRAME_CAPTURE") != 0) {
    std::fputs("native boot setup failed: frame capture environment\n",
               stderr);
    return 3;
  }
  if (capture != nullptr && *capture != '\0') {
    if (capture_size > 32768U) {
      std::free(capture);
      std::fputs("native boot setup failed: frame capture path\n", stderr);
      return 3;
    }
    state.frame_capture_path = capture;
  }
  std::free(capture);
  char *pcm_capture = nullptr;
  std::size_t pcm_capture_size = 0U;
  if (_dupenv_s(&pcm_capture, &pcm_capture_size,
                "JFG_PHASE8_PCM_CAPTURE") != 0) {
    std::fputs("native boot setup failed: PCM capture environment\n",
               stderr);
    return 3;
  }
  if (pcm_capture != nullptr && *pcm_capture != '\0' &&
      (pcm_capture_size > 32768U ||
       !state.host_audio.configure_capture(pcm_capture))) {
    std::free(pcm_capture);
    std::fputs("native boot setup failed: PCM capture path\n", stderr);
    return 3;
  }
  std::free(pcm_capture);
  char *rdram_capture = nullptr;
  std::size_t rdram_capture_size = 0U;
  if (_dupenv_s(&rdram_capture, &rdram_capture_size,
                "JFG_PHASE8_RDRAM_CAPTURE") != 0) {
    std::fputs("native boot setup failed: RDRAM capture environment\n",
               stderr);
    return 3;
  }
  if (rdram_capture != nullptr && *rdram_capture != '\0') {
    if (rdram_capture_size > 32768U) {
      std::free(rdram_capture);
      std::fputs("native boot setup failed: RDRAM capture path\n", stderr);
      return 3;
    }
    state.rdram_capture_path = rdram_capture;
  }
  std::free(rdram_capture);
  char *progress = nullptr;
  std::size_t progress_size = 0U;
  if (_dupenv_s(&progress, &progress_size, "JFG_PHASE8_PROGRESS") != 0) {
    std::fputs("native boot setup failed: progress environment\n", stderr);
    return 3;
  }
  if (progress != nullptr && *progress != '\0') {
    if (progress_size > 32768U) {
      std::free(progress);
      std::fputs("native boot setup failed: progress path\n", stderr);
      return 3;
    }
    state.progress_path = progress;
  }
  std::free(progress);
  char *record_path = nullptr;
  std::size_t record_path_size = 0U;
  if (_dupenv_s(&record_path, &record_path_size,
                "JFG_PHASE9_INPUT_RECORD") != 0) {
    std::fputs("native boot setup failed: input record environment\n", stderr);
    return 3;
  }
  if (record_path != nullptr && *record_path != '\0') {
    if (record_path_size > 32768U) {
      std::free(record_path);
      std::fputs("native boot setup failed: input record path\n", stderr);
      return 3;
    }
    state.input_record_path = record_path;
  }
  std::free(record_path);
  char *actor_trace = nullptr;
  std::size_t actor_trace_size = 0U;
  if (_dupenv_s(&actor_trace, &actor_trace_size,
                "JFG_PHASE9_ACTOR_TRACE") != 0) {
    std::fputs("native boot setup failed: actor trace environment\n", stderr);
    return 3;
  }
  if (actor_trace != nullptr && *actor_trace != '\0') {
    if (std::strcmp(actor_trace, "1") != 0) {
      std::free(actor_trace);
      std::fputs("native boot setup failed: actor trace value\n", stderr);
      return 3;
    }
    state.actor_timing_trace = true;
  }
  std::free(actor_trace);
  char *retrace_hash = nullptr;
  std::size_t retrace_hash_size = 0U;
  if (_dupenv_s(&retrace_hash, &retrace_hash_size,
                "JFG_PHASE9_RETRACE_HASH") != 0) {
    std::fputs("native boot setup failed: retrace hash environment\n",
               stderr);
    return 3;
  }
  if (retrace_hash != nullptr && *retrace_hash != '\0') {
    if (retrace_hash_size > 32768U) {
      std::free(retrace_hash);
      std::fputs("native boot setup failed: retrace hash path\n", stderr);
      return 3;
    }
    state.retrace_hash_path = retrace_hash;
  }
  std::free(retrace_hash);
  char *replay_by_poll = nullptr;
  std::size_t replay_by_poll_size = 0U;
  if (_dupenv_s(&replay_by_poll, &replay_by_poll_size,
                "JFG_PHASE9_REPLAY_BY_POLL") != 0) {
    std::fputs("native boot setup failed: replay poll environment\n", stderr);
    return 3;
  }
  if (replay_by_poll != nullptr && *replay_by_poll != '\0') {
    if (std::strcmp(replay_by_poll, "1") != 0) {
      std::free(replay_by_poll);
      std::fputs("native boot setup failed: replay poll value\n", stderr);
      return 3;
    }
    state.input_replay_by_poll = true;
  }
  std::free(replay_by_poll);
  char *replay_path = nullptr;
  std::size_t replay_path_size = 0U;
  if (_dupenv_s(&replay_path, &replay_path_size,
                "JFG_PHASE8_INPUT_REPLAY") != 0) {
    std::fputs("native boot setup failed: input replay environment\n",
               stderr);
    return 3;
  }
  if (replay_path != nullptr && *replay_path != '\0') {
    if (replay_path_size > 32768U ||
        jfg::DeterministicInputReplay::load(replay_path,
                                             state.input_replay, state.input_replay_by_poll
                  ? jfg::InputReplayOrder::recorded_poll
                  : jfg::InputReplayOrder::retrace) !=
            jfg::InputReplayError::none) {
      std::free(replay_path);
      std::fputs("native boot setup failed: input replay\n", stderr);
      return 3;
    }
    state.input_replay_loaded = true;
    state.input_replay_event_count = state.input_replay.events().size();
  }
  std::free(replay_path);
  // Bounded replay fixture for the same input-suspension path used by focus
  // loss. It never changes desktop focus and is rejected in interactive Play.
  char suspend_interval[48]{};
  const auto suspend_length=GetEnvironmentVariableA("JFG_DIAGNOSTIC_INPUT_SUSPEND_VI",suspend_interval,48U);
  if(suspend_length!=0U) {
    if(suspend_length>=48U || !state.input_replay_loaded || state.play_mode)return 3;
    const std::string_view spec(suspend_interval,suspend_length);
    const auto colon=spec.find(':');
    if(colon==std::string_view::npos)return 3;
    auto first=std::from_chars(spec.data(),spec.data()+colon,state.diagnostic_input_suspend_begin);
    auto last=std::from_chars(spec.data()+colon+1U,spec.data()+spec.size(),state.diagnostic_input_suspend_end);
    if(first.ec!=std::errc{} || first.ptr!=spec.data()+colon || last.ec!=std::errc{} ||
       last.ptr!=spec.data()+spec.size() || state.diagnostic_input_suspend_begin<3U ||
       state.diagnostic_input_suspend_begin>=state.diagnostic_input_suspend_end ||
       state.diagnostic_input_suspend_end>1000000U)return 3;
  }
  char *pointer_script=nullptr;std::size_t pointer_script_size=0;
  if(_dupenv_s(&pointer_script,&pointer_script_size,"JFG_PHASE9_MENU_POINTER_REPLAY")!=0)return 3;
  if(pointer_script && *pointer_script) {
    std::ifstream script(pointer_script);
    if(state.play_mode || !state.input_replay_loaded || !script || !state.menu_pointer_replay.load(script)) {
      std::free(pointer_script);
      std::fputs("native boot setup failed: menu pointer probe requires bounded input replay\n",stderr);
      return 3;
    }
  }
  std::free(pointer_script);
  if (state.input_replay_by_poll && !state.input_replay_loaded) {
    std::fputs("native boot setup failed: replay poll requires input replay\n", stderr);
    return 3;
  }
  // A bounded, hidden replay can exercise the interactive VI/audio cadence
  // without opening another game window or consuming live controller input.
  char *realtime_replay = nullptr;
  std::size_t realtime_replay_size = 0U;
  if (_dupenv_s(&realtime_replay, &realtime_replay_size,
                "JFG_PHASE9_REALTIME_REPLAY") != 0) {
    std::fputs("native boot setup failed: realtime replay environment\n", stderr);
    return 3;
  }
  if (realtime_replay != nullptr && *realtime_replay != '\0') {
    if (std::strcmp(realtime_replay, "1") != 0 ||
        !state.input_replay_loaded || state.play_mode || state.fast_replay ||
        (state.guest_leaf_probe && !state.original_os_host_pacing)) {
      std::free(realtime_replay);
      std::fputs("native boot setup failed: realtime replay value\n", stderr);
      return 3;
    }
    // The unified guest OS already owns its clock. service_live_window returns
    // before the cooperative frame pacer in this mode, so hidden replay can
    // exercise frontend pause without adding a second VI pacing policy.
    state.realtime_replay = true;
    jfg::support_event("replay=hidden-realtime-vi-clock");
  }
  std::free(realtime_replay);
  if (state.poll_target != 0U &&
      (!state.input_replay_loaded || !state.input_replay_by_poll)) {
    std::fputs("native boot setup failed: poll target requires replay-by-poll\n",
               stderr);
    return 3;
  }
  char *save_path = nullptr;
  std::size_t save_path_size = 0U;
  if (_dupenv_s(&save_path, &save_path_size, "JFG_PHASE8_SAVE_PATH") != 0) {
    std::fputs("native boot setup failed: save environment\n", stderr);
    return 3;
  }
  if (save_path != nullptr && *save_path != '\0' &&
      (save_path_size > 32768U || !configure_flash_path(state, save_path))) {
    std::free(save_path);
    std::fputs("native boot setup failed: save path\n", stderr);
    return 3;
  }
  std::free(save_path);
  char *controller_pak_path = nullptr;
  std::size_t controller_pak_path_size = 0U;
  if (_dupenv_s(&controller_pak_path, &controller_pak_path_size,
                "JFG_PHASE8_CONTROLLER_PAK_PATH") != 0) {
    std::fputs("native boot setup failed: controller pak environment\n",
               stderr);
    return 3;
  }
  if (controller_pak_path != nullptr && *controller_pak_path != '\0' &&
      (controller_pak_path_size > 32768U ||
       !configure_controller_pak_path(state, controller_pak_path))) {
    std::free(controller_pak_path);
    std::fputs("native boot setup failed: controller pak path\n", stderr);
    return 3;
  }
  std::free(controller_pak_path);
#else
  if (const char *fast_replay = std::getenv("JFG_PHASE9_FAST_REPLAY");
      fast_replay != nullptr && *fast_replay != '\0') {
    if (std::strcmp(fast_replay, "1") != 0) {
      std::fputs("native boot setup failed: fast replay value\n", stderr);
      return 3;
    }
    state.fast_replay = true;
  }
  if (const char *capture = std::getenv("JFG_PHASE8_FRAME_CAPTURE");
      capture != nullptr && *capture != '\0') {
    if (std::strlen(capture) > 32767U) {
      std::fputs("native boot setup failed: frame capture path\n", stderr);
      return 3;
    }
    state.frame_capture_path = capture;
  }
  if (const char *rdram_capture = std::getenv("JFG_PHASE8_RDRAM_CAPTURE");
      rdram_capture != nullptr && *rdram_capture != '\0') {
    if (std::strlen(rdram_capture) > 32767U) {
      std::fputs("native boot setup failed: RDRAM capture path\n", stderr);
      return 3;
    }
    state.rdram_capture_path = rdram_capture;
  }
  if (const char *progress = std::getenv("JFG_PHASE8_PROGRESS");
      progress != nullptr && *progress != '\0') {
    if (std::strlen(progress) > 32767U) {
      std::fputs("native boot setup failed: progress path\n", stderr);
      return 3;
    }
    state.progress_path = progress;
  }
  if (const char *retrace_hash = std::getenv("JFG_PHASE9_RETRACE_HASH");
      retrace_hash != nullptr && *retrace_hash != '\0') {
    if (std::strlen(retrace_hash) > 32767U) {
      std::fputs("native boot setup failed: retrace hash path\n", stderr);
      return 3;
    }
    state.retrace_hash_path = retrace_hash;
  }
  if (const char *replay_path = std::getenv("JFG_PHASE8_INPUT_REPLAY");
      replay_path != nullptr && *replay_path != '\0') {
    if (std::strlen(replay_path) > 32767U ||
        jfg::DeterministicInputReplay::load(replay_path,
                                             state.input_replay) !=
            jfg::InputReplayError::none) {
      std::fputs("native boot setup failed: input replay\n", stderr);
      return 3;
    }
    state.input_replay_loaded = true;
    state.input_replay_event_count = state.input_replay.events().size();
  }
  if (const char *save_path = std::getenv("JFG_PHASE8_SAVE_PATH");
      save_path != nullptr && *save_path != '\0' &&
      (std::strlen(save_path) > 32767U ||
       !configure_flash_path(state, save_path))) {
    std::fputs("native boot setup failed: save path\n", stderr);
    return 3;
  }
  if (const char *controller_pak_path =
          std::getenv("JFG_PHASE8_CONTROLLER_PAK_PATH");
      controller_pak_path != nullptr && *controller_pak_path != '\0' &&
      (std::strlen(controller_pak_path) > 32767U ||
       !configure_controller_pak_path(state, controller_pak_path))) {
    std::fputs("native boot setup failed: controller pak path\n", stderr);
    return 3;
  }
#endif
  if (!state.progress_path.empty()) {
    state.timing_trace.open(state.progress_path + ".timing.tsv",
                            std::ios::out | std::ios::trunc);
    if (!state.timing_trace) {
      std::fputs("native boot setup failed: timing trace path\n", stderr);
      return 3;
    }
    state.timing_trace
        << "kind\tvi_frame\tvi_retrace\tduration_us\tdetail_a\tdetail_b"
           "\tdetail_c\tdetail_d\n";
  }
  if (!state.input_record_path.empty()) {
    state.input_record.open(state.input_record_path, std::ios::trunc);
    if (!state.input_record) {
      std::fputs("native boot setup failed: input record open\n", stderr);
      return 3;
    }
#if defined(_WIN32)
    reload_controller_ports(state);
    state.pc_record_extended=state.modern_controls_active;
#endif
    state.input_record << (state.pc_record_extended?jfg::kInputReplayHeaderV8:jfg::kInputReplayHeader) << '\n';
  }
  if (!state.retrace_hash_path.empty()) {
    state.retrace_hash_trace.open(state.retrace_hash_path,
                                  std::ios::out | std::ios::trunc);
    if (!state.retrace_hash_trace) {
      std::fputs("native boot setup failed: retrace hash open\n", stderr);
      return 3;
    }
    state.retrace_hash_trace
        << "{\"kind\":\"jfg-phase9-retrace-hash-header\","
           "\"schema\":1}\n";
    char *updates = nullptr;
    std::size_t updates_size = 0U;
    if (_dupenv_s(&updates, &updates_size, "JFG_PHASE9_UPDATE_HASHES") != 0) {
      std::fputs("native boot setup failed: update hash environment\n", stderr);
      return 3;
    }
    const bool update_hash_enabled = updates != nullptr && std::strcmp(updates, "1") == 0;
    const bool update_hash_invalid = updates != nullptr && *updates != '\0' && !update_hash_enabled;
    std::free(updates);
    if (update_hash_invalid) {
      std::fputs("native boot setup failed: update hash value\n", stderr);
      return 3;
    }
    if (update_hash_enabled) {
      state.update_hash_trace.open(state.retrace_hash_path + ".updates.jsonl",
                                   std::ios::out | std::ios::trunc);
      if (!state.update_hash_trace) {
        std::fputs("native boot setup failed: update hash open\n", stderr);
        return 3;
      }
      state.update_hash_trace
          << "{\"kind\":\"jfg-phase9-update-hash-header\",\"schema\":1}\n";
      char *focus = nullptr;
      std::size_t focus_size = 0U;
      if (_dupenv_s(&focus, &focus_size, "JFG_PHASE9_FOCUS_UPDATES") != 0) {
        std::fputs("native boot setup failed: focus environment\n", stderr);
        return 3;
      }
      if (focus != nullptr && *focus != '\0') {
        const std::string_view specification(focus);
        const auto colon = specification.find(':');
        std::uint64_t first = 0U, last = 0U;
        bool valid = colon != std::string_view::npos && colon != 0U &&
                     colon + 1U < specification.size();
        if (valid) {
          const auto first_number = std::from_chars(
              specification.data(), specification.data() + colon, first);
          const auto last_number = std::from_chars(
              specification.data() + colon + 1U,
              specification.data() + specification.size(), last);
          valid = first_number.ec == std::errc{} &&
                  first_number.ptr == specification.data() + colon &&
                  last_number.ec == std::errc{} &&
                  last_number.ptr == specification.data() + specification.size();
        }
        std::free(focus);
        if (!valid || first < 1U || last < first || last - first > 15U) {
          std::fputs("native boot setup failed: focus update range\n", stderr);
          return 3;
        }
        state.focus_update_start = first;
        state.focus_update_end = last;
      } else {
        std::free(focus);
      }
    }
  }
  char *update_word = nullptr;
  std::size_t update_word_size = 0U;
  if (_dupenv_s(&update_word, &update_word_size,
                "JFG_PHASE9_UPDATE_WORD") != 0) {
    std::fputs("native boot setup failed: update word environment\n", stderr);
    return 3;
  }
  if (update_word != nullptr && *update_word != '\0') {
    const std::string_view specification(update_word);
    std::uint32_t address = 0U;
    bool valid = specification.size() == 10U &&
                 specification.substr(0U, 2U) == "0x";
    if (valid) {
      const auto parsed = std::from_chars(specification.data() + 2U,
                                          specification.data() + 10U,
                                          address, 16);
      valid = parsed.ec == std::errc{} &&
              parsed.ptr == specification.data() + 10U;
    }
    std::free(update_word);
    if (!valid || address < 0x80000000U || address > 0x803FFFFCU ||
        (address & 3U) != 0U || !state.update_hash_trace.is_open()) {
      std::fputs("native boot setup failed: update word range\n", stderr);
      return 3;
    }
    state.update_word_address = address;
    const auto update_word_path = std::filesystem::path(state.retrace_hash_path).parent_path() /
        "update-word.tsv";
    state.update_word_trace.open(update_word_path, std::ios::out | std::ios::trunc);
    if (!state.update_word_trace) {
      std::fputs("native boot setup failed: update word trace\n", stderr);
      return 3;
    }
    state.update_word_trace <<
        "update\tcontroller_polls\tvi\tframe\taddress\tvalue\n";
  } else {
    std::free(update_word);
  }
  char *watch = nullptr;
  std::size_t watch_size = 0U;
  if (_dupenv_s(&watch, &watch_size, "JFG_PHASE9_WATCH_WORD") != 0) {
    std::fputs("native boot setup failed: watch environment\n", stderr);
    return 3;
  }
  if (watch != nullptr && *watch != '\0') {
    const std::string_view specification(watch);
    std::uint32_t address = 0U;
    bool valid = specification.size() == 10U &&
                 specification.substr(0U, 2U) == "0x";
    if (valid) {
      const auto parsed = std::from_chars(specification.data() + 2U,
                                          specification.data() + 10U,
                                          address, 16);
      valid = parsed.ec == std::errc{} &&
              parsed.ptr == specification.data() + 10U;
    }
    std::free(watch);
    if (!valid || address < 0x80000000U || address > 0x803FFFFCU ||
        (address & 3U) != 0U || state.focus_update_start == 0U ||
        !state.update_hash_trace.is_open()) {
      std::fputs("native boot setup failed: watch word range\n", stderr);
      return 3;
    }
    state.watch_word_address = address;
    const auto watch_path = std::filesystem::path(state.retrace_hash_path).parent_path() /
        "watch-word.tsv";
    state.watch_word_trace.open(watch_path, std::ios::out | std::ios::trunc);
    if (!state.watch_word_trace) {
      std::fputs("native boot setup failed: watch word trace\n", stderr);
      return 3;
    }
    state.watch_word_trace <<
        "update_candidate\tvi_retraces\tcontroller_polls\tdepth\t"
        "dispatch_target\treturn_address\tbefore\tafter\n";
  } else {
    std::free(watch);
  }
  char *entry = nullptr;
  std::size_t entry_size = 0U;
  if (_dupenv_s(&entry, &entry_size, "JFG_PHASE9_ENTRY_TARGET") != 0) {
    std::fputs("native boot setup failed: entry environment\n", stderr);
    return 3;
  }
  if (entry != nullptr && *entry != '\0') {
    const std::string_view specification(entry);
    std::uint32_t target = 0U;
    bool valid = specification.size() == 10U &&
                 specification.substr(0U, 2U) == "0x";
    if (valid) {
      const auto parsed = std::from_chars(specification.data() + 2U,
                                          specification.data() + 10U,
                                          target, 16);
      valid = parsed.ec == std::errc{} &&
              parsed.ptr == specification.data() + 10U;
    }
    std::free(entry);
    if (!valid || target == 0U || (target & 3U) != 0U ||
        state.focus_update_start == 0U || !state.update_hash_trace.is_open()) {
      std::fputs("native boot setup failed: entry target range\n", stderr);
      return 3;
    }
    state.entry_probe_target = target;
    const auto entry_path =
        std::filesystem::path(state.retrace_hash_path).parent_path() /
        "entry-args.tsv";
    state.entry_probe_trace.open(entry_path, std::ios::out | std::ios::trunc);
    if (!state.entry_probe_trace) {
      std::fputs("native boot setup failed: entry trace\n", stderr);
      return 3;
    }
    state.entry_probe_trace <<
        "update_candidate\tvi_retraces\tcontroller_polls\ttarget\t"
        "a0\ta1\ta2\ta3\n";
  } else {
    std::free(entry);
  }
  char *entry_fpu = nullptr;
  std::size_t entry_fpu_size = 0U;
  if (_dupenv_s(&entry_fpu, &entry_fpu_size, "JFG_PHASE9_ENTRY_FPU") != 0) {
    std::fputs("native boot setup failed: entry FPU environment\n", stderr);
    return 3;
  }
  const bool trace_entry_fpu = entry_fpu != nullptr &&
      std::string_view(entry_fpu) == "1";
  const bool invalid_entry_fpu = entry_fpu != nullptr &&
      *entry_fpu != '\0' && !trace_entry_fpu;
  std::free(entry_fpu);
  if (invalid_entry_fpu || (trace_entry_fpu && state.entry_probe_target == 0U)) {
    std::fputs("native boot setup failed: entry FPU requires entry probe\n", stderr);
    return 3;
  }
  if (trace_entry_fpu) {
    const auto fpu_path =
        std::filesystem::path(state.retrace_hash_path).parent_path() /
        "entry-fpu.tsv";
    state.entry_fpu_trace.open(fpu_path, std::ios::out | std::ios::trunc);
    if (!state.entry_fpu_trace) {
      std::fputs("native boot setup failed: entry FPU trace\n", stderr);
      return 3;
    }
    state.entry_fpu_trace <<
        "update_candidate\tvi_retraces\tcontroller_polls\ttarget";
    for (std::size_t index = 0U; index < 32U; ++index)
      state.entry_fpu_trace << "\tf" << index << "_lo\tf" << index << "_hi";
    state.entry_fpu_trace << '\n';
  }
  char *entry_memory = nullptr;
  std::size_t entry_memory_size = 0U;
  if (_dupenv_s(&entry_memory, &entry_memory_size,
                "JFG_PHASE9_ENTRY_MEMORY") != 0) {
    std::fputs("native boot setup failed: entry memory environment\n", stderr);
    return 3;
  }
  const bool trace_entry_memory = entry_memory != nullptr &&
      std::string_view(entry_memory) == "1";
  const bool invalid_entry_memory = entry_memory != nullptr &&
      *entry_memory != '\0' && !trace_entry_memory;
  std::free(entry_memory);
  if (invalid_entry_memory ||
      (trace_entry_memory && state.entry_probe_target == 0U)) {
    std::fputs("native boot setup failed: entry memory requires entry probe\n",
               stderr);
    return 3;
  }
  if (trace_entry_memory) {
    const auto memory_path =
        std::filesystem::path(state.retrace_hash_path).parent_path() /
        "entry-memory.tsv";
    state.entry_memory_trace.open(memory_path, std::ios::out | std::ios::trunc);
    if (!state.entry_memory_trace) {
      std::fputs("native boot setup failed: entry memory trace\n", stderr);
      return 3;
    }
    state.entry_memory_trace <<
        "update_candidate\tvi_retraces\tcontroller_polls\ttarget\t"
        "a1_256\ta2_256\ta3_256\n";
  }
  char *entry_gpr = nullptr;
  std::size_t entry_gpr_size = 0U;
  if (_dupenv_s(&entry_gpr, &entry_gpr_size, "JFG_PHASE9_ENTRY_GPR") != 0) {
    std::fputs("native boot setup failed: entry GPR environment\n", stderr);
    return 3;
  }
  const bool trace_entry_gpr = entry_gpr != nullptr &&
      std::string_view(entry_gpr) == "1";
  const bool invalid_entry_gpr = entry_gpr != nullptr &&
      *entry_gpr != '\0' && !trace_entry_gpr;
  std::free(entry_gpr);
  if (invalid_entry_gpr || (trace_entry_gpr && state.entry_probe_target == 0U)) {
    std::fputs("native boot setup failed: entry GPR requires entry probe\n", stderr);
    return 3;
  }
  if (trace_entry_gpr) {
    const auto gpr_path =
        std::filesystem::path(state.retrace_hash_path).parent_path() /
        "entry-gpr.tsv";
    state.entry_gpr_trace.open(gpr_path, std::ios::out | std::ios::trunc);
    if (!state.entry_gpr_trace) {
      std::fputs("native boot setup failed: entry GPR trace\n", stderr);
      return 3;
    }
    state.entry_gpr_trace <<
        "update_candidate\tvi_retraces\tcontroller_polls\ttarget";
    for (std::size_t index = 0U; index < 32U; ++index)
      state.entry_gpr_trace << "\tr" << index << "_lo\tr" << index << "_hi";
    state.entry_gpr_trace << '\n';
  }
  char *instruction_probe = nullptr;
  const auto point_addresses = [](const char* name) {
    char* raw = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&raw, &size, name) != 0)
      return std::optional<std::vector<std::uint32_t>>{};
    const auto parsed = point_probe_addresses(raw == nullptr ? "" : raw);
    std::free(raw);
    return parsed;
  };
  const auto point_pcs = point_addresses("JFG_PHASE9_POINT_PCS");
  const auto point_words = point_addresses("JFG_PHASE9_POINT_WORDS");
  if (!point_pcs || !point_words ||
      (point_pcs->empty() && !point_words->empty()) ||
      (!point_pcs->empty() && (!state.guest_os_probe || state.focus_update_start == 0U ||
                              !state.update_hash_trace.is_open()))) {
    std::fputs("native boot setup failed: point probe requires original OS and focused updates\n", stderr);
    return 3;
  }
  state.point_probe_pcs = *point_pcs;
  state.point_probe_words = *point_words;
  char* device_events = nullptr;
  std::size_t device_events_size = 0U;
  if (_dupenv_s(&device_events, &device_events_size, "JFG_PHASE9_DEVICE_EVENTS") != 0) return 3;
  const bool trace_device_events = device_events != nullptr && std::string_view(device_events) == "1";
  const bool invalid_device_events = device_events != nullptr && !trace_device_events;
  std::free(device_events);
  if (invalid_device_events || (trace_device_events && point_pcs->empty())) {
    std::fputs("native boot setup failed: device events require focused original-OS point capture\n", stderr);
    return 3;
  }
  if (trace_device_events) {
    const auto device_event_path = std::filesystem::path(state.retrace_hash_path).parent_path() / "device-events.tsv";
    FILE* device_event_output = nullptr;
    if (_wfopen_s(&device_event_output, device_event_path.c_str(), L"wb") != 0 || device_event_output == nullptr) return 3;
    state.device_event_stream.reset(device_event_output);
    const auto event_first = static_cast<std::uint32_t>((std::max)(std::uint64_t{1}, state.focus_update_start - 1U));
    const auto event_last = static_cast<std::uint32_t>(state.focus_update_end + 1U);
    if (!jfg_device_event_open(&state.device_event_probe, device_event_output, event_first, event_last, JFG_EVENT_NATIVE_PRE_INSTRUCTION)) return 3;
  }
  char* eret_transfers = nullptr;
  std::size_t eret_transfers_size = 0U;
  if (_dupenv_s(&eret_transfers, &eret_transfers_size, "JFG_PHASE9_ERET_TRANSFERS") != 0) return 3;
  const bool trace_eret_transfers = eret_transfers != nullptr && std::string_view(eret_transfers) == "1";
  const bool invalid_eret_transfers = eret_transfers != nullptr && !trace_eret_transfers;
  std::free(eret_transfers);
  if (invalid_eret_transfers || (trace_eret_transfers && !trace_device_events)) return 3;
  if (trace_eret_transfers) {
    state.eret_transfer_stream.open(std::filesystem::path(state.retrace_hash_path).parent_path() / "eret-transfers.tsv");
    if (!state.eret_transfer_stream || !state.eret_transfer_probe.open(state.eret_transfer_stream,
        state.device_event_probe.first, state.device_event_probe.last)) return 3;
  }
  char* instruction_effects = nullptr;
  std::size_t instruction_effects_size = 0U;
  if (_dupenv_s(&instruction_effects, &instruction_effects_size, "JFG_PHASE9_INSTRUCTION_EFFECT_UPDATE") != 0) return 3;
  if (instruction_effects != nullptr) {
    const std::string_view value(instruction_effects);
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), state.instruction_effect_update);
    const bool valid = parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() &&
        state.guest_os_probe && trace_eret_transfers && trace_device_events &&
        state.instruction_effect_update >= state.device_event_probe.first &&
        state.instruction_effect_update <= state.device_event_probe.last;
    std::free(instruction_effects);
    if (!valid) return 3;
    state.instruction_effect_stream.open(std::filesystem::path(state.retrace_hash_path).parent_path() /
                                        "instruction-effects.bin", std::ios::binary);
    if (!state.instruction_effect_stream || !state.instruction_effect_trace.open(
          state.instruction_effect_stream, state.instruction_effect_update)) return 3;
  }
  if (!point_pcs->empty()) {
    state.point_probe_trace.open(std::filesystem::path(state.retrace_hash_path).parent_path() /
                                "point-probe.tsv", std::ios::out | std::ios::trunc);
    if (!state.point_probe_trace) return 3;
    state.point_probe_trace << "update_candidate\tvi_retraces\tcontroller_polls\tpc\topcode";
    for (unsigned index = 0; index < 32; ++index)
      state.point_probe_trace << "\tr" << index << "_lo\tr" << index << "_hi";
    for (const auto address : state.point_probe_words)
      state.point_probe_trace << "\tm" << std::hex << std::setw(8) << std::setfill('0') << address;
    state.point_probe_trace << std::dec << std::setfill(' ') << '\n';
  }
  std::size_t instruction_probe_size = 0U;
  if (_dupenv_s(&instruction_probe, &instruction_probe_size,
                "JFG_PHASE9_INSTRUCTION_PROBE") != 0) {
    std::fputs("native boot setup failed: instruction probe environment\n", stderr);
    return 3;
  }
  const bool trace_instruction = instruction_probe != nullptr &&
      std::string_view(instruction_probe) == "1";
  const bool invalid_instruction = instruction_probe != nullptr &&
      *instruction_probe != '\0' && !trace_instruction;
  std::free(instruction_probe);
  if (invalid_instruction ||
      (trace_instruction && (state.focus_update_start == 0U ||
                             !state.update_hash_trace.is_open()))) {
    std::fputs("native boot setup failed: instruction probe requires focused updates\n",
               stderr);
    return 3;
  }
  if (trace_instruction) {
    const auto instruction_path =
        std::filesystem::path(state.retrace_hash_path).parent_path() /
        "instruction-probe.tsv";
    state.instruction_probe_trace.open(instruction_path,
                                       std::ios::out | std::ios::trunc);
    if (!state.instruction_probe_trace) {
      std::fputs("native boot setup failed: instruction probe trace\n", stderr);
      return 3;
    }
    state.instruction_probe_trace <<
        "update_candidate\tvi_retraces\tcontroller_polls\tpc\t"
        "r4\tr5\tr6\tr7\tr10\tr15\tr19\tr20\tr21\tr23\tr25\t"
        "f4_lo\tf22_lo\tf23_odd_lo\thost_fe_round\n";
  }
#else
  if (play_mode)
    return 3;
#endif
  state.runtime = &runtime;
  state.rdram = rdram.data();
  state.rom = rom.data();
  state.rom_size = rom.size();
  if (state.navigation_mod.observing() && !state.navigation_mod.npc_rewards.load_rom(rom))
    std::fputs("navigation mod: NPC reward catalog unavailable; rewards remain unknown\n", stderr);
#if defined(JFG_PHASE8_LIVE_RUNTIME)
  char *poll_path = nullptr;
  std::size_t poll_path_size = 0U;
  if (_dupenv_s(&poll_path, &poll_path_size,
                "JFG_PHASE95_NATIVE_POLL_TRACE") != 0) {
    std::fputs("native boot setup failed: poll trace environment\n", stderr);
    return 3;
  }
  if (poll_path != nullptr && *poll_path != '\0') {
    if (poll_path_size > 32768U) {
      std::free(poll_path);
      std::fputs("native boot setup failed: poll trace path\n", stderr);
      return 3;
    }
    state.phase95_poll_trace.open(poll_path, std::ios::out | std::ios::trunc);
    if (!state.phase95_poll_trace) {
      std::free(poll_path);
      std::fputs("native boot setup failed: poll trace open\n", stderr);
      return 3;
    }
    state.phase95_poll_trace
        << "poll\tvi_retrace\tvi_frame\tfront_mode\tlevel_word"
           "\trng_seed\tbuttons\tx\ty\n";
  }
  std::free(poll_path);
  char *poll_hashes = nullptr;
  std::size_t poll_hashes_size = 0U;
  if (_dupenv_s(&poll_hashes, &poll_hashes_size,
                "JFG_PHASE9_POLL_HASHES") != 0) {
    std::fputs("native boot setup failed: poll hash environment\n", stderr);
    return 3;
  }
  const bool poll_hash_enabled = poll_hashes != nullptr &&
      std::strcmp(poll_hashes, "1") == 0;
  const bool poll_hash_invalid = poll_hashes != nullptr &&
      *poll_hashes != '\0' && !poll_hash_enabled;
  std::free(poll_hashes);
  if (poll_hash_invalid || (poll_hash_enabled &&
      (!state.input_replay_by_poll || state.retrace_hash_path.empty()))) {
    std::fputs("native boot setup failed: poll hash requires replay-by-poll\n", stderr);
    return 3;
  }
  if (poll_hash_enabled) {
    state.poll_hash_trace.open(state.retrace_hash_path + ".polls.jsonl",
                               std::ios::out | std::ios::trunc);
    if (!state.poll_hash_trace) {
      std::fputs("native boot setup failed: poll hash open\n", stderr);
      return 3;
    }
    state.poll_hash_trace <<
        "{\"kind\":\"jfg-phase9-poll-hash-header\",\"schema\":1,"
        "\"boundary\":\"pre-controller-input\"}\n";
  }
  char *event_range = nullptr;
  std::size_t event_range_size = 0U;
  if (_dupenv_s(&event_range, &event_range_size,
                "JFG_PHASE9_EVENT_TRACE_RANGE") != 0) {
    std::fputs("native boot setup failed: event trace environment\n", stderr);
    return 3;
  }
  if (event_range != nullptr && *event_range != '\0') {
    const std::string_view specification(event_range);
    bool valid = poll_hash_enabled && state.update_hash_trace.is_open() &&
                 specification.size() <= 64U;
    std::size_t cursor = 0U;
    while (valid && cursor < specification.size()) {
      if (state.event_range_count == state.event_ranges.size()) {
        valid = false;
        break;
      }
      const auto comma = specification.find(',', cursor);
      const auto end = comma == std::string_view::npos
                           ? specification.size() : comma;
      const auto colon = specification.find(':', cursor);
      std::uint64_t first = 0U, last = 0U;
      valid = colon != std::string_view::npos && colon > cursor &&
              colon + 1U < end && colon < end;
      if (valid) {
        const auto left = std::from_chars(
            specification.data() + cursor, specification.data() + colon,
            first);
        const auto right = std::from_chars(
            specification.data() + colon + 1U, specification.data() + end,
            last);
        valid = left.ec == std::errc{} &&
                left.ptr == specification.data() + colon &&
                right.ec == std::errc{} &&
                right.ptr == specification.data() + end &&
                last >= first && last - first <= 31U && last <= 1000000U &&
                (state.event_range_count == 0U ||
                 first > state.event_ranges[state.event_range_count - 1U].second);
      }
      if (valid)
        state.event_ranges[state.event_range_count++] = {first, last};
      if (comma == std::string_view::npos)
        break;
      cursor = comma + 1U;
      valid = valid && cursor < specification.size();
    }
    if (!valid || state.event_range_count == 0U) {
      std::free(event_range);
      std::fputs("native boot setup failed: event trace range\n", stderr);
      return 3;
    }
    state.event_trace.open(state.retrace_hash_path + ".events.tsv",
                           std::ios::out | std::ios::trunc);
    if (!state.event_trace) {
      std::free(event_range);
      std::fputs("native boot setup failed: event trace open\n", stderr);
      return 3;
    }
    state.event_trace << "sequence\tevent\tpoll\tcompleted_updates\tframe"
                         "\tvi\tstart_calls\tget_calls\tconnected\tbuttons"
                         "\tstick_x\tstick_y\n";
  }
  std::free(event_range);
  char *controller_return = nullptr;
  std::size_t controller_return_size = 0U;
  if (_dupenv_s(&controller_return, &controller_return_size,
                "JFG_PHASE9_CONTROLLER_RETURN") != 0) {
    std::fputs("native boot setup failed: controller return environment\n", stderr);
    return 3;
  }
  if (controller_return != nullptr && *controller_return != '\0') {
    const bool valid = std::strcmp(controller_return, "1") == 0 &&
                       state.event_range_count != 0U;
    std::free(controller_return);
    if (!valid) {
      std::fputs("native boot setup failed: controller return range\n", stderr);
      return 3;
    }
    state.controller_return_trace.open(
        state.retrace_hash_path + ".controller-return.tsv",
        std::ios::out | std::ios::trunc);
    if (!state.controller_return_trace) {
      std::fputs("native boot setup failed: controller return trace\n", stderr);
      return 3;
    }
    state.controller_return_trace <<
        "sequence\tpoll\tcompleted_updates\tvi\tframe\taddress\tdata\n";
  } else {
    std::free(controller_return);
  }
  std::uint32_t graphics_shadow_cursor =
      static_cast<std::uint32_t>(kRdramSize);
  for (std::uint32_t section = 0U;
       section < jfg_generated_section_count(); ++section) {
    JfgGeneratedSectionMetadata metadata{};
    if (jfg_generated_section_metadata(section, &metadata) == 0) {
      std::fputs("native boot setup failed: graphics overlay index\n",
                 stderr);
      return 3;
    }
    if (metadata.is_overlay == 0U)
      continue;
    const std::uint64_t extent = std::uint64_t{metadata.text_size} +
                                 metadata.data_size + metadata.bss_size;
    graphics_shadow_cursor = (graphics_shadow_cursor + 15U) & ~15U;
    if (extent == 0U || extent > UINT32_MAX - graphics_shadow_cursor ||
        graphics_shadow_cursor + extent > jfg::kRt64RequiredRdramBytes) {
      std::fputs("native boot setup failed: graphics overlay index\n",
                 stderr);
      return 3;
    }
    state.graphics_overlay_shadows.push_back(
        {section, metadata.linked_vram, static_cast<std::uint32_t>(extent),
         graphics_shadow_cursor});
    graphics_shadow_cursor += static_cast<std::uint32_t>(extent);
  }
#endif
  for (std::uint32_t source = 0U;
       source < jfg_generated_section_count(); ++source) {
    const JfgGeneratedR32Descriptor *descriptors = nullptr;
    std::size_t count = 0U;
    if (jfg_generated_relocation_descriptors(source, &descriptors, &count) ==
            0 ||
        (count != 0U && descriptors == nullptr)) {
      std::fputs("native boot setup failed: relocation target index\n",
                 stderr);
      return 3;
    }
    for (std::size_t index = 0U; index < count; ++index) {
      JfgGeneratedSectionMetadata target_metadata{};
      if (jfg_generated_section_metadata(descriptors[index].target_section,
                                         &target_metadata) == 0 ||
          descriptors[index].target_offset >
              UINT32_MAX - target_metadata.linked_vram) {
        std::fputs("native boot setup failed: relocation target index\n",
                   stderr);
        return 3;
      }
      state.generated_relocation_targets.insert(
          target_metadata.linked_vram + descriptors[index].target_offset);
    }
  }
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  char hidden_validation[2]{};
  const bool diagnostic_hidden_window =
      GetEnvironmentVariableA("JFG_DIAGNOSTIC_HIDDEN_WINDOW",hidden_validation,2U)==1U &&
      hidden_validation[0]=='1';
  LiveRt64Window live_window(state, play_mode && !diagnostic_hidden_window);
  if (!initialize_live_renderer(state, live_window)) {
    std::fputs("native boot setup failed: RT64 renderer\n", stderr);
    return 3;
  }
#endif
  jfg::support_event("native=renderer-ready");
  state.mmio_trace = &backing.mmio_trace();
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  state.mmio_trace->trace_audio_payloads = state.host_audio.capturing_pcm();
#endif
  state.io_revision = &state.mmio_trace->execution_io_revision;
  state.io_guest_count = &state.mmio_trace->guest_count;
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  if (state.guest_os_probe) {
    // Read-only, ROM-pinned corrected-Mupen IPL hardware observation. This
    // initializes platform registers only, not game actors, RNG or save RAM.
    // The reset interpreter contributes its actual executed instructions.
    // Startup phase and progressive scanlines passed the independent phase
    // microtest; no game-parity acceptance follows from this profile.
    state.cpu_count = 14421016U + 2U * handoff.instructions;
    state.mmio_trace->guest_execution = true;
    state.mmio_trace->device_rdram = {state.rdram, kRdramSize};
    state.mmio_trace->device_rom = {state.rom, state.rom_size};
    // IPL3 programs cartridge-domain timing from the ROM header before boot.
    // Recover that platform state because this private entry skips IPL3.
    if (state.rom_size < 4U) return 3;
    const auto cartridge_timing = (std::uint32_t(state.rom[0]) << 24U) |
        (std::uint32_t(state.rom[1]) << 16U) |
        (std::uint32_t(state.rom[2]) << 8U) | std::uint32_t(state.rom[3]);
    const std::array<std::uint32_t, 4> pi_boot_timing{
        cartridge_timing & 255U, (cartridge_timing >> 8U) & 255U,
        (cartridge_timing >> 16U) & 15U, (cartridge_timing >> 20U) & 3U};
    for (std::size_t index = 0U; index < pi_boot_timing.size(); ++index) {
      if (!state.mmio_trace->pi_dma.write(0x14U + static_cast<std::uint32_t>(index) * 4U,
              pi_boot_timing[index], state.cpu_count, state.mmio_trace->device_rdram,
              state.mmio_trace->device_rom)) return 3;
    }

    state.mmio_trace->flash_bus.bind(state.flashram);
    state.mmio_trace->guest_count = state.cpu_count;
    state.mmio_trace->mi_pending = 8U;
    state.mmio_trace->vi_next = 5000U + ((state.cpu_count - 5000U) / 500000U + 1U) * 500000U;
    char* guest_trace_path = nullptr;
    std::size_t guest_trace_path_size = 0U;
    if (_dupenv_s(&guest_trace_path, &guest_trace_path_size, "JFG_PHASE9_GUEST_OS_CLOCK_TRACE") != 0)
      return 3;
    if (guest_trace_path != nullptr && *guest_trace_path != '\0') {
      state.guest_clock_trace.open(guest_trace_path, std::ios::binary | std::ios::trunc);
      std::free(guest_trace_path);
      if (!state.guest_clock_trace) return 3;
      state.guest_clock_trace << "boot\t" << state.cpu_count << '\t' << handoff.instructions << '\n';
      state.guest_clock_trace << "pi-boot\t" << cartridge_timing;
      for (auto value : pi_boot_timing) state.guest_clock_trace << '\t' << value;
      state.guest_clock_trace << '\n';
    } else {
      std::free(guest_trace_path);
      if (!original_timing_mode) {
        std::fputs("native boot setup failed: guest OS diagnostic trace required\n", stderr);
        return 3;
      }
    }

  }
#endif
  if (state.guest_leaf_probe) {
    // Explicitly observed reference boot state, not a hardware-reset claim.
    if (!state.mmio_trace->mi_mask.initialize(0U)) return 3;
    state.mmio_trace->guest_mi_mask = true;
    // The HLE task engine is initially stopped, with no outstanding task.
    // This explicit owned-device state is not a general hardware reset model.
    if (!state.mmio_trace->sp_status.initialize(1U, false)) return 3;
    state.mmio_trace->guest_sp_status = true;
    state.mmio_trace->sp_rdram = {state.rdram, kRdramSize};
    state.mmio_trace->guest_pif_boot = true;
  }
  if (!backing.arm_mmio_monitor(state.journal)) {
    std::fputs("native boot setup failed: MMIO monitor\n", stderr);
    return 3;
  }
  state.scheduler = std::make_unique<ThreadScheduler>(hle::GuestMemory(
      {rdram.data(), rdram.size()},
      hle::GuestMemory::Layout::native_word_big_endian));
  if (state.guest_leaf_probe && !state.scheduler->set_baton_hook(
      [&state](int thread, bool entering) {
        const auto index = static_cast<std::size_t>(thread);
        if (thread < 0 || index >= state.saved_rcp_masks.size())
          fail_closed_dispatch(state, "guest-leaf", "thread-mask-owner", 0U);
        // The independent thread fixture qualifies the ordinary unrestricted
        // OS global policy. Restricted global policies need their own saved
        // versus effective-mask qualification; do not silently approximate.
        std::uint32_t global = 0U;
        if (!state.scheduler->memory().read_u32(0x800a9a6cU, global) ||
            global != 0x003fff01U)
          fail_closed_dispatch(state, "guest-leaf", "thread-mask-global-policy", global);
        if (entering) {
          if (!state.mmio_trace->mi_mask.initialize(state.saved_rcp_masks[index]))
            fail_closed_dispatch(state, "guest-leaf", "thread-mask-restore", 0U);
        } else {
          state.saved_rcp_masks[index] = state.mmio_trace->mi_mask.read();
        }
      })) return 3;
  if (jfg_minimal_runtime_initialize() == 0 ||
      !runtime.install_table(std::make_unique<Table>()).ok() ||
      jfg_minimal_runtime_bind_dispatch(dispatch, &state) == 0) {
    std::fputs("native boot setup failed: generated runtime\n", stderr);
    return 3;
  }
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
  // Clock policy is fixed for play. A bounded replay may request one measured
  // policy boundary so two clocks can begin from identical guest memory.
  // Avoid inspecting stream/vector internals at every guest instruction.
  char recorded_time_flag[2]{};
  const auto recorded_time_length = GetEnvironmentVariableA("JFG_PHASE9_REPLAY_RECORDED_TIME",recorded_time_flag,2U);
  if (recorded_time_length != 0U && (recorded_time_length != 1U || recorded_time_flag[0] != '1' ||
      !state.input_replay_loaded || !state.input_replay_by_poll)) return 3;
  state.input_replay_recorded_time = recorded_time_length == 1U;
  char gameplay_time_flag[2]{};
  const auto gameplay_time_length = GetEnvironmentVariableA("JFG_PHASE9_REPLAY_GAMEPLAY_TIME",gameplay_time_flag,2U);
  if (gameplay_time_length != 0U && (gameplay_time_length != 1U || gameplay_time_flag[0] != '1' ||
      !state.input_replay_loaded || !state.input_replay_by_poll || state.input_replay_recorded_time)) return 3;
  state.input_replay_gameplay_time = gameplay_time_length == 1U;
  if (state.input_replay_gameplay_time) {
    const auto& events = state.input_replay.events();
    const auto first = std::find_if(events.begin(),events.end(),[](const auto& event) { return event.sample.pc_mode == 3U; });
    if (first == events.end()) return 3;
    state.replay_gameplay_first_poll = static_cast<std::size_t>(first-events.begin());
  }
  char native_clock_flag[2]{};
  const auto native_clock_length = GetEnvironmentVariableA("JFG_DIAGNOSTIC_HOST_CLOCK",native_clock_flag,2U);
  if (native_clock_length != 0U && (native_clock_length != 1U || native_clock_flag[0] != '1')) return 3;
  bool native_preview_play = false;
#if defined(JFG_EXPERIMENTAL_NATIVE_PLAY_TIMING)
  native_preview_play = state.play_mode;
#endif
  state.diagnostic_host_clock_requested = native_clock_length == 1U || native_preview_play;
  char native_span_flag[2]{};
  const auto native_span_length = GetEnvironmentVariableA("JFG_DIAGNOSTIC_NATIVE_SPANS",native_span_flag,2U);
  if (native_span_length != 0U && (native_span_length != 1U || native_span_flag[0] != '1' ||
      !state.diagnostic_host_clock_requested || JFG_NATIVE_TIMING_SPAN_GENERATION != 1 ||
      state.instruction_effect_stream.is_open())) return 3;
  state.native_spans_enabled = native_span_length == 1U || native_preview_play;
  char pure_math[2]{};
  const auto pure_length=GetEnvironmentVariableA("JFG_DIAGNOSTIC_RESIDENT_MATH",pure_math,2U);
  if(pure_length && (pure_length!=1U || (pure_math[0]!='0' && pure_math[0]!='1') || (pure_math[0]=='1' && !state.native_spans_enabled)))return 3;
  state.resident_math_enabled=pure_length==1U ? pure_math[0]=='1' : native_preview_play;
  char bsp_flag[2]{};
  const auto bsp_length=GetEnvironmentVariableA("JFG_DIAGNOSTIC_BSP_NATIVE",bsp_flag,2U);
  if(bsp_length && (bsp_length!=1U || (bsp_flag[0]!='0' && bsp_flag[0]!='1')))return 3;
  if(bsp_length==1U && bsp_flag[0]=='1' && !jfg::boot::native_collision_available())return 3;
  state.diagnostic_bsp_enabled=jfg::boot::native_collision_available() &&
      (bsp_length==1U ? bsp_flag[0]=='1' : native_preview_play);


  if (native_preview_play) {
    jfg::support_event("timing=native-elapsed/experimental-preview");
    jfg::support_event("timing=native-spans-enabled");
  }
  char function_profile_flag[2]{};
  const auto function_profile_length=GetEnvironmentVariableA("JFG_DIAGNOSTIC_FUNCTION_PROFILE",function_profile_flag,2U);
  if(function_profile_length!=0U && (function_profile_length!=1U ||
      (function_profile_flag[0]!='1' && function_profile_flag[0]!='2') ||
      !state.gameplay_trace || memory_trace_length!=1U))return 3;
  state.diagnostic_function_profile=function_profile_length==1U?unsigned(function_profile_flag[0]-'0'):0U;
  for(const auto& item:std::array<std::pair<const char*,std::uint64_t*>,2>{{
      {"JFG_DIAGNOSTIC_PROFILE_FIRST_VI",&state.diagnostic_profile_first_vi},
      {"JFG_DIAGNOSTIC_PROFILE_LAST_VI",&state.diagnostic_profile_last_vi}}}) {
    char text[32]{};const auto length=GetEnvironmentVariableA(item.first,text,32U);
    if(length==0U)continue;
    if(length>=32U || !state.diagnostic_function_profile)return 3;
    const auto parsed=std::from_chars(text,text+length,*item.second);
    if(parsed.ec!=std::errc{} || parsed.ptr!=text+length)return 3;
  }
  if(state.diagnostic_profile_last_vi<=state.diagnostic_profile_first_vi ||
     state.diagnostic_profile_last_vi-state.diagnostic_profile_first_vi>3600U)return 3;
  char update_cycles_flag[2]{};
  const auto update_cycles_length = GetEnvironmentVariableA(
      "JFG_DIAGNOSTIC_UPDATE_CYCLES", update_cycles_flag, 2U);
  if (update_cycles_length != 0U && (update_cycles_length != 1U ||
      update_cycles_flag[0] != '1' || !state.gameplay_trace)) return 3;
  state.diagnostic_update_cycles = update_cycles_length == 1U;
  char cost_flag[2]{};
  const auto cost_length = GetEnvironmentVariableA("JFG_DIAGNOSTIC_CPU_COSTS", cost_flag, 2U);
  if (cost_length != 0U && (cost_length != 1U || cost_flag[0] != '1' ||
      state.play_mode || !state.gameplay_trace || !state.input_replay_loaded)) return 3;
  state.diagnostic_cpu_costs = cost_length == 1U;
  char gpu_cost_flag[2]{};
  const auto gpu_cost_length=GetEnvironmentVariableA("JFG_DIAGNOSTIC_GPU_COSTS",gpu_cost_flag,2U);
  if(gpu_cost_length!=0U && (gpu_cost_length!=1U || gpu_cost_flag[0]!='1' ||
      !state.diagnostic_function_profile))return 3;
  state.diagnostic_gpu_costs=gpu_cost_length==1U;
  if ((state.diagnostic_cpu_costs || state.diagnostic_gpu_costs) && (!state.rt64_shell ||
      !state.rt64_shell->enable_framebuffer_gpu_timing())) return 3;
  char boundary_text[32]{};
  const auto boundary_length = GetEnvironmentVariableA("JFG_DIAGNOSTIC_CLOCK_BOUNDARY_VI",boundary_text,32U);
  if (boundary_length != 0U) {
    if (boundary_length >= 32U || state.play_mode || !state.gameplay_trace ||
        !state.input_replay_loaded || !state.input_replay_by_poll) return 3;
    const auto parsed = std::from_chars(boundary_text,boundary_text+boundary_length,state.diagnostic_clock_boundary_vi);
    if (parsed.ec != std::errc{} || parsed.ptr != boundary_text+boundary_length ||
        state.diagnostic_clock_boundary_vi == 0U) return 3;
    if (state.input_replay_gameplay_time)
      state.replay_gameplay_first_poll = (std::numeric_limits<std::size_t>::max)();
  }
  char stall_text[16]{};
  auto stall_length = GetEnvironmentVariableA("JFG_DIAGNOSTIC_CLOCK_STALL_MS",stall_text,16U);
  if (stall_length == 0U)
    stall_length = GetEnvironmentVariableA("JFG_DIAGNOSTIC_BOUNDARY_STALL_MS",stall_text,16U);
  if (stall_length != 0U) {
    if (stall_length >= 16U || state.play_mode || !state.diagnostic_host_clock_requested ||
        state.diagnostic_clock_boundary_vi == 0U) return 3;
    const auto parsed = std::from_chars(stall_text,stall_text+stall_length,state.diagnostic_boundary_stall_ms);
    if (parsed.ec != std::errc{} || parsed.ptr != stall_text+stall_length ||
        state.diagnostic_boundary_stall_ms == 0U || state.diagnostic_boundary_stall_ms > 2000U) return 3;
  }
  state.diagnostic_host_clock = state.diagnostic_host_clock_requested && state.diagnostic_clock_boundary_vi == 0U;
  char audio_source_flag[2]{};
  const auto audio_source_length=GetEnvironmentVariableA("JFG_DIAGNOSTIC_NATIVE_AUDIO_SOURCE",audio_source_flag,2U);
  if(audio_source_length!=0U && (audio_source_length!=1U || audio_source_flag[0]!='1' ||
      !state.diagnostic_host_clock_requested)) return 3;
  // Chosen at startup, before even the instruction-clock prefix builds audio.
  // Switching providers over outstanding DMA descriptors is not supported.
  state.native_audio_source=audio_source_length==1U || native_preview_play;
  if(state.native_audio_source) jfg::support_event("audio=synchronous-rom-sample-cache");
  char audio_playback_flag[2]{};
  const auto audio_playback_length=GetEnvironmentVariableA("JFG_DIAGNOSTIC_NATIVE_AUDIO_PLAYBACK",audio_playback_flag,2U);
  if(audio_playback_length!=0U && (audio_playback_length!=1U ||
      (audio_playback_flag[0]!='0' && audio_playback_flag[0]!='1') ||
      !state.diagnostic_host_clock_requested)) return 3;
  state.native_audio_playback=audio_playback_length ? audio_playback_flag[0]=='1' : native_preview_play;
  if(state.native_audio_playback)jfg::support_event("audio=native-demand-playback");

  std::string clock_record_path,clock_replay_path;
  const auto clock_path = [](const char* name,std::string& destination) {
    char* value=nullptr;std::size_t size=0;
    if (_dupenv_s(&value,&size,name)!=0) return false;
    const bool valid=size<=32768U;
    if (valid && value) destination=value;
    std::free(value);return valid;
  };
  if (!clock_path("JFG_DIAGNOSTIC_CLOCK_RECORD",clock_record_path) ||
      !clock_path("JFG_DIAGNOSTIC_CLOCK_REPLAY",clock_replay_path)) return 3;
  char clock_limit_text[32]{};
  const auto clock_limit_length=GetEnvironmentVariableA(
      "JFG_DIAGNOSTIC_CLOCK_SAMPLE_LIMIT",clock_limit_text,32U);
  if (clock_limit_length!=0U) {
    if (clock_limit_length>=32U || state.play_mode ||
        (clock_record_path.empty() && clock_replay_path.empty())) return 3;
    std::uint64_t clock_sample_limit=0U;
    const auto parsed_limit=std::from_chars(clock_limit_text,
        clock_limit_text+clock_limit_length,clock_sample_limit);
    if (parsed_limit.ec!=std::errc{} ||
        parsed_limit.ptr!=clock_limit_text+clock_limit_length ||
        !state.native_clock_samples.set_sample_limit(clock_sample_limit)) return 3;
  }
  if (!clock_record_path.empty() || !clock_replay_path.empty()) {
    if (state.play_mode || !state.input_replay_loaded || !state.diagnostic_host_clock_requested ||
        state.diagnostic_clock_boundary_vi==0U || state.diagnostic_boundary_stall_ms!=0U ||
        (!clock_record_path.empty() && !clock_replay_path.empty())) return 3;
    state.native_clock_sample_buffer.resize(65536U);
    if (!clock_record_path.empty()) {
      state.native_clock_sample_output.rdbuf()->pubsetbuf(state.native_clock_sample_buffer.data(),65536);
      state.native_clock_sample_output.open(clock_record_path,std::ios::binary|std::ios::trunc);
      if (!state.native_clock_samples.record(state.native_clock_sample_output)) return 3;
    } else {
      state.native_clock_sample_input.rdbuf()->pubsetbuf(state.native_clock_sample_buffer.data(),65536);
      state.native_clock_sample_input.open(clock_replay_path,std::ios::binary);
      if (!state.native_clock_samples.replay(state.native_clock_sample_input)) return 3;
    }
  }
  char release_text[32]{};
  const auto release_length=GetEnvironmentVariableA("JFG_DIAGNOSTIC_CLOCK_RELEASE_VI",release_text,32U);
  if (release_length != 0U) {
    if (release_length>=32U || state.play_mode || !state.gameplay_trace ||
        !state.input_replay_by_poll || !state.native_clock_samples.replaying()) return 3;
    const auto parsed=std::from_chars(release_text,release_text+release_length,state.diagnostic_clock_release_vi);
    if (parsed.ec!=std::errc{} || parsed.ptr!=release_text+release_length ||
        state.diagnostic_clock_release_vi<=state.diagnostic_clock_boundary_vi ||
        state.diagnostic_clock_release_vi>=state.retrace_target) return 3;
  }
  char rom_completion_flag[2]{};
  const auto rom_completion_length=GetEnvironmentVariableA(
      "JFG_DIAGNOSTIC_NATIVE_ROM_COMPLETION",rom_completion_flag,2U);
  if(rom_completion_length && (rom_completion_length!=1U ||
      (rom_completion_flag[0]!='0' && rom_completion_flag[0]!='1') ||
      (rom_completion_flag[0]=='1' && !state.diagnostic_host_clock_requested)))return 3;
  const bool native_rom_requested=rom_completion_length ? rom_completion_flag[0]=='1' : native_preview_play;
  if (state.mmio_trace) {
    state.mmio_trace->native_elapsed_time = state.diagnostic_host_clock;
    // Recorded clock and instruction observers retain the reference bus model.
    // Ordinary input replay and the function-cost profiler remain eligible.
    state.mmio_trace->native_rom_completion=native_rom_requested &&
        !state.native_clock_sample_input.is_open() && !state.native_clock_sample_output.is_open() &&
        state.focus_update_start==0U && !state.guest_clock_trace.is_open() &&
        state.point_probe_pcs.empty() && !state.device_event_stream &&
        !state.eret_transfer_stream.is_open() && !state.instruction_effect_stream.is_open();
    if(state.mmio_trace->native_rom_completion)
      jfg::support_event("assets=native-rom-completion");
  }
  if(state.diagnostic_host_clock && state.native_audio_playback && state.mmio_trace &&
      !state.mmio_trace->ai_dma.native_queue_capacity(4U))
    fail_closed_dispatch(state,"audio","native-queue-capacity",0U);

  if (state.diagnostic_host_clock_requested && (!state.guest_os_probe || !state.original_os_host_pacing ||
      !state.point_probe_pcs.empty() || state.device_event_stream || state.eret_transfer_stream.is_open())) return 3;
  char* model_watch_value = nullptr;
  std::size_t model_watch_size = 0U;
  if (_dupenv_s(&model_watch_value,&model_watch_size,"JFG_DIAGNOSTIC_MODEL_POINTER_WATCH") != 0)
    return 3;
  const bool model_watch_requested = model_watch_value != nullptr;
  const bool model_watch_hardware = model_watch_requested && std::string_view(model_watch_value) == "hardware";
  const bool model_watch_pages = model_watch_requested && std::string_view(model_watch_value) == "pages";
  const bool model_watch_valid = !model_watch_requested || model_watch_hardware || model_watch_pages ||
      std::string_view(model_watch_value) == "1";
  std::free(model_watch_value);
  if (!model_watch_valid || (model_watch_requested &&
      (state.rdram_capture_path.empty() || !state.guest_os_probe))) return 3;
  if (model_watch_requested && std::signal(SIGABRT,private_abort_capture)==SIG_ERR)
    return 3;
  state.private_model_pointer_watch = model_watch_requested;
  state.private_model_hardware_watch = model_watch_hardware;
  state.private_model_page_watch = model_watch_pages;
  if ((model_watch_hardware || model_watch_pages) &&
      AddVectoredExceptionHandler(1U,model_watch_pages ? private_page_write_observer
                                                     : private_model_write_observer)==nullptr)
    return 3;

  state.execution_fast_profile_ready = !state.diagnostic_host_clock && state.guest_os_probe &&
      state.mmio_trace != nullptr && state.point_probe_pcs.empty() &&
      !state.device_event_stream && !state.eret_transfer_stream.is_open();
#endif
  jfg::support_event("native=running");
  DispatchBinding binding{state, true};
  if (state.guest_leaf_probe) {
    if (jfg_minimal_runtime_bind_cpu(native_cpu_operation, &state) == 0) {
      std::fputs("native boot setup failed: CPU operation owner\n", stderr);
      return 3;
    }
    binding.cpu_bound = true;
  }
  const auto function =
      runtime.lookup_function_by_guest_address(handoff.transfer_target);
  if (!function.ok())
    ledger(state, "generated", "reset-transfer-unresolved",
           handoff.transfer_target);
  else {
    recomp_context context{};
    constexpr std::array<gpr recomp_context::*, 32> kGprMembers = {
        &recomp_context::r0,  &recomp_context::r1,  &recomp_context::r2,
        &recomp_context::r3,  &recomp_context::r4,  &recomp_context::r5,
        &recomp_context::r6,  &recomp_context::r7,  &recomp_context::r8,
        &recomp_context::r9,  &recomp_context::r10, &recomp_context::r11,
        &recomp_context::r12, &recomp_context::r13, &recomp_context::r14,
        &recomp_context::r15, &recomp_context::r16, &recomp_context::r17,
        &recomp_context::r18, &recomp_context::r19, &recomp_context::r20,
        &recomp_context::r21, &recomp_context::r22, &recomp_context::r23,
        &recomp_context::r24, &recomp_context::r25, &recomp_context::r26,
        &recomp_context::r27, &recomp_context::r28, &recomp_context::r29,
        &recomp_context::r30, &recomp_context::r31,
    };
    for (std::size_t index = 0U; index < handoff.gpr.size(); ++index)
      context.*kGprMembers[index] =
          static_cast<gpr>(sign_extend_mips32(handoff.gpr[index]));
    context.f_odd = &context.f0.u32h;
    context.status_reg = 0U;
    context.mips3_float_mode = 0U;
    if (state.guest_leaf_probe)
      cop0_status_write(&context, 0x34000000U); // observed reference cold boot
    if (state.guest_os_probe) {
      state.guest_transport = std::make_unique<GuestThreadTransport<recomp_context>>(
          [&state](std::uint32_t entry, recomp_context& cpu) {
            const auto body = state.runtime->lookup_function_by_guest_address(entry);
            if (!body.ok()) fail_closed_dispatch(state, "guest-os", "thread-entry-unavailable", entry);
            invoke_generated(body.function, state.rdram, &cpu, state, entry);
          }, [](recomp_context& cpu) {
            cpu.f_odd = cpu.mips3_float_mode ? &cpu.f1.u32l : &cpu.f0.u32h;
            set_cop1_cs(cpu.fcr31);
          }, [&state](const GuestThreadTransport<recomp_context>::EretBoundary& edge,
                      const recomp_context& cpu) {
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
            if (!state.eret_transfer_stream.is_open() && !state.instruction_effect_stream.is_open()) return;
            if (!state.guest_eret_boundary || state.guest_epc != edge.target_pc)
              fail_closed_dispatch(state, "eret-transfer-probe", "uncommitted-return", state.guest_last_pc);
            emit_instruction_effect(state, cpu, 2U, state.instruction_effect_section,
                state.guest_last_pc, state.guest_last_word, edge.target_pc, edge.to_thread);
            if (!state.eret_transfer_stream.is_open()) return;
            const EretTransferRow row{
              static_cast<std::uint32_t>(state.completed_game_updates + 1U),
              state.guest_last_pc, state.guest_last_word, edge.from_thread, edge.to_thread,
              edge.target_pc, cpu.status_reg, static_cast<std::uint32_t>(state.cpu_count),
              {0, cpu.r1, cpu.r2, cpu.r3, cpu.r4, cpu.r5, cpu.r6, cpu.r7,
               cpu.r8, cpu.r9, cpu.r10, cpu.r11, cpu.r12, cpu.r13, cpu.r14, cpu.r15,
               cpu.r16, cpu.r17, cpu.r18, cpu.r19, cpu.r20, cpu.r21, cpu.r22, cpu.r23,
               cpu.r24, cpu.r25, cpu.r26, cpu.r27, cpu.r28, cpu.r29, cpu.r30, cpu.r31}};
            if (!state.eret_transfer_probe.emit(row))
              fail_closed_dispatch(state, "eret-transfer-probe", "output-or-boundary", state.guest_last_pc);
#else
            (void)state; (void)edge; (void)cpu;
#endif
          });
#if defined(_WIN32) && defined(JFG_PHASE8_LIVE_RUNTIME)
      state.guest_transport->enable_timing(state.diagnostic_cpu_costs || state.diagnostic_function_profile!=0U);
#endif
      try {
        state.guest_transport->run(handoff.transfer_target, context,
            state.play_mode ? UINT64_MAX : 1000000U);
      } catch (const std::exception& error) {
        std::fprintf(stderr, "guest OS transport failure: %s\n", error.what());
        fail_closed_dispatch(state, "guest-os", "transport-contract", state.guest_last_pc);
      }
    } else {
      invoke_generated(function.function, rdram.data(), &context, state,
                       handoff.transfer_target);
    }
  }
  if (!state.ledgered)
    ledger(state, "generated", "returned", handoff.transfer_target);
  return 4;
}

bool reset_transfer_target(const std::vector<std::uint8_t> &rom,
                           std::uint32_t &target) {
  std::vector<std::uint8_t> bytes(kRdramSize);
  if (!load_static_sections(rom, bytes))
    return false;
  const auto handoff = interpret_reset_handoff(
      bytes, 0x80000400U, kRdramSize / sizeof(std::uint32_t) + 4096U);
  if (!handoff.ok())
    return false;
  target = handoff.transfer_target;
  return true;
}

#if defined(_WIN32)
bool extract_canonical_ledger(const std::string &output,
                              std::string &ledger_line) {
  static const std::regex pattern(
      R"(\{"kind":"jfg-phase6-native-first-trap","category":"[a-z-]+","guest_target":"0x[0-9a-f]{8}","operation":"[A-Za-z_][A-Za-z0-9_-]*","disposition":"fail-closed-trap","count":1\}\r?\n)");
  std::sregex_iterator found(output.begin(), output.end(), pattern);
  if (found == std::sregex_iterator{} ||
      std::next(found) != std::sregex_iterator{}) {
    return false;
  }
  const std::smatch &match = *found;
  if (match.position() != 0U) {
    return false;
  }
  const std::string trailing = match.suffix().str();
  if (trailing.size() > 4096U ||
      trailing.find("jfg-phase6-native-first-trap") != std::string::npos) {
    return false;
  }
  if (!trailing.empty()) {
    return false;
  }
  ledger_line = match.str();
  if (ledger_line.size() >= 2U && ledger_line[ledger_line.size() - 2U] == '\r')
    ledger_line.erase(ledger_line.size() - 2U, 1U);
  return true;
}

bool extract_canonical_success(const std::string &output,
                               std::string &success_line) {
  static const std::regex pattern(
      R"jfg(\{"kind":"jfg-phase6-native-boot","status":"stable-vi","build_identity":"[A-Za-z0-9_.+-]+","sanitizer":"(?:none|address)","vi_retraces":3,"vi_frames":[1-9][0-9]*,"vi_interrupts":3,"vi_messages_delivered":3,"vi_queue":"0x[89a-f][0-9a-f]{7}","threads_created":[1-9][0-9]*,"rdram_bytes":4194304,"mmio_accesses":[1-9][0-9]*,"unsupported_accesses":0,"mmio_trace":"[rw]:0x[a-f0-9]{8}(?:,[rw]:0x[a-f0-9]{8})*","journal_entries":[1-9][0-9]*,"state_hash":"[0-9a-f]{64}","journal_hash":"[0-9a-f]{64}"\}\r?\n)jfg");
  std::smatch match;
  if (!std::regex_match(output, match, pattern))
    return false;
  success_line = match.str();
  if (success_line.size() >= 2U &&
      success_line[success_line.size() - 2U] == '\r')
    success_line.erase(success_line.size() - 2U, 1U);
  return true;
}

bool extract_canonical_probe(const std::string &output,
                             std::string &probe_line) {
  static const std::regex actor_trace_extension(
      R"jfg(\{"kind":"jfg-phase9-actor-trace","actors":\[.*\]\}\r?\n)jfg");
  static const std::regex king_bear_extension(
      R"jfg(,"hints_state":"0x[0-9a-f]{8}","hints_state_words":"(?:[0-9a-f]{8},){11}[0-9a-f]{8}","king_bear_control_calls":[0-9]+,"king_bear_callback":"0x[0-9a-f]{8}","king_bear_callback_section":[0-9]+,"king_bear_callback_offset":"0x[0-9a-f]{8}","king_bear_actor":"0x[0-9a-f]{8}","king_bear_yaw":"0x[0-9a-f]{4}","king_bear_position_bits":"[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8}","king_bear_state":"0x[0-9a-f]{8}","king_bear_state_words":"[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8}","hinttext_start_calls":[0-9]+,"hinttext_stop_calls":[0-9]+,"hinttext_active_calls":[0-9]+,"hinttext_active_zero_returns":[0-9]+,"hinttext_active_nonzero_returns":[0-9]+,"hinttext_active_last_return":[0-9]+,"hinttext_update_calls":[0-9]+,"hinttext_advance_calls":[0-9]+,"hinttext_accept_calls":[0-9]+,"hinttext_section_base":"0x[0-9a-f]{8}","hinttext_state_words":"(?:[0-9a-f]{8},){15}[0-9a-f]{8}","runlink_module7_words":"[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8}")jfg");
  static const std::regex pattern(
      R"jfg(\{"kind":"jfg-phase8-native-probe","status":"retrace-target","retrace_target":(?:[4-9]|[1-9][0-9]+),"vi_retraces":(?:[4-9]|[1-9][0-9]+),"vi_frames":[1-9][0-9]*,"vi_interrupts":(?:[4-9]|[1-9][0-9]+),"vi_messages_delivered":(?:[4-9]|[1-9][0-9]+),"mmio_accesses":[1-9][0-9]*,"unsupported_accesses":[0-9]+,"dispatches":[0-9]+,"pi_dma":[0-9]+,"overlay_publications":[0-9]+,"overlay_sequence":"[0-9,]*","phase9_route":\{"player_control_calls":[0-9]+,"weapon_update_calls":[0-9]+,"weapon_fire_held_updates":[0-9]+,"weapon_fire_pressed_updates":[0-9]+,"weapon_dummy_fire_calls":[0-9]+,"weapon_fire_calls":[0-9]+,"weapon_shot_calls":[0-9]+,"weapon_hit_calls":[0-9]+,"enemy_kill_calls":[0-9]+,"weapon_stat_shots":[0-9]+,"weapon_stat_hits":[0-9]+,"weapon_stat_kills":[0-9]+,"collision_calls":[0-9]+,"particle_update_calls":[0-9]+,"cutscene_active_calls":[0-9]+,"cutscene_camera_calls":[0-9]+,"level_change_calls":[0-9]+,"death_restart_calls":[0-9]+,"player_hit_check_calls":[0-9]+,"player_actor":"0x[0-9a-f]{8}","player_yaw":"0x[0-9a-f]{4}","player_position_bits":"[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8}","hints_control_calls":[0-9]+,"hints_talk_calls":[0-9]+,"hints_actor":"0x[0-9a-f]{8}","hints_yaw":"0x[0-9a-f]{4}","hints_position_bits":"[0-9a-f]{8},[0-9a-f]{8},[0-9a-f]{8}"\},"controller_events":[0-9]+,"generated_calls":[0-9]+,"mapped_calls":[0-9]+,"queue_create_calls":[0-9]+,"queue_send_calls":[0-9]+,"queue_recv_calls":[0-9]+,"sp_task_load_calls":[0-9]+,"sp_task_start_calls":[0-9]+,"controller_read_start_calls":[0-9]+,"controller_get_data_calls":[0-9]+,"vi_swap_calls":[0-9]+,"graphics_tasks":[0-9]+,"graphics_commands":[0-9]+,"graphics_triangles":[0-9]+,"presented_frames":[0-9]+,"decoded_audio_tasks":[0-9]+,"audio_device_initialized":(?:true|false),"audio_device_started":(?:true|false),"audio_device_frequency":[0-9]+,"audio_device_buffers":[0-9]+,"audio_device_queued_bytes":[0-9]+,"audio_device_consumed_bytes":[0-9]+,"audio_device_consumed_ms":[0-9]+,"audio_device_underruns":[0-9]+,"audio_device_overruns":[0-9]+,"ai_submission_overflows":[0-9]+,"frame_captured":(?:true|false),"input_replay_events":[0-9]+,"controller_samples":[0-9]+,"non_neutral_controller_samples":[0-9]+,"controller_disconnects":[0-9]+,"controller_reconnects":[0-9]+,"game_pressed_observations":[0-9]+,"game_stick_observations":[0-9]+,"last_game_stick_x":-?[0-9]+,"last_game_stick_y":-?[0-9]+,"menu_stick_observations":[0-9]+,"last_menu_stick_x":-?[0-9]+,"last_menu_stick_y":-?[0-9]+,"front_mode":[0-9]+,"flash_path_configured":(?:true|false),"flash_image_loaded":(?:true|false),"flash_image_created":(?:true|false),"flash_persist_count":[0-9]+,"threads_stopped":[0-9]+,"threads_runnable":[0-9]+,"threads_blocked":[0-9]+,"threads_finished":[0-9]+,"hle_counts":"[A-Za-z0-9_=,]+","thread_summary":"[0-9a-f:,]+","controller_read_thread":-?[0-9]+,"si_queue":"0x[0-9a-f]{8}","si_queue_valid":[0-9]+,"si_receive_calls":[0-9]+,"last_si_receive_return":"0x[0-9a-f]{8}","last_si_receive_block":[0-9]+,"last_si_dispatch_trace":"[0-9a-f,]+","journal_events":[0-9]+,"journal_entries":[0-9]+,"journal_overflow":(?:true|false),"state_hash":"[0-9a-f]{64}","journal_hash":"[0-9a-f]{64}"\}\r?\n)jfg");
  const std::string without_actor_trace =
      std::regex_replace(output, actor_trace_extension, "");
  const std::string canonical_output =
      std::regex_replace(without_actor_trace, king_bear_extension, "");
  if (canonical_output == without_actor_trace)
    return false;
  std::smatch match;
  if (!std::regex_match(canonical_output, match, pattern))
    return false;
  probe_line = output;
  if (probe_line.size() >= 2U &&
      probe_line[probe_line.size() - 2U] == '\r')
    probe_line.erase(probe_line.size() - 2U, 1U);
  return true;
}

bool extract_original_os_probe(const std::string& output, unsigned target, std::string& line) {
  char* enabled = nullptr;
  std::size_t size = 0;
  const bool opted_in = _dupenv_s(&enabled, &size, "JFG_PHASE9_GUEST_OS_PROBE") == 0 &&
      enabled && std::strcmp(enabled, "1") == 0;
  std::free(enabled);
  char original_timing[2]{};
  const auto timing_override = GetEnvironmentVariableA("JFG_ORIGINAL_TIMING",original_timing,2U);
  const bool runtime_opted_in = timing_override == 1U ? original_timing[0]=='1' :
      JFG_ORIGINAL_TIMING_GENERATION == 1;
  if (!opted_in && !runtime_opted_in) return false;
  static const std::regex pattern(
      R"jfg(\{"kind":"jfg-phase9-original-os-probe","acceptance":false,"status":"retrace-target","retrace_target":([1-9][0-9]*),"vi_retraces":([1-9][0-9]*),"controller_samples":[0-9]+,"completed_updates":[0-9]+,"cpu_count":[1-9][0-9]*,"instructions":[1-9][0-9]*,"graphics_tasks":[0-9]+,"decoded_audio_tasks":[0-9]+,"unsupported_accesses":0,"runtime_snapshot_complete":false,"state_hash":"[0-9a-f]{64}"\}\r?\n)jfg");
  std::smatch match;
  if (!std::regex_match(output, match, pattern) || match[1].str() != std::to_string(target) ||
      match[2].str() != std::to_string(target)) return false;
  line = output;
  return true;
}

bool extract_canonical_poll_probe(const std::string &output,
                                  const unsigned poll_target,
                                  std::string &probe_line) {
  // Reuse the strict retrace-probe suffix validator. Only the stop reason and
  // early VI counters differ; normalize those after checking their exact
  // order, decimal shape, requested target, and sampled-poll total.
  static const std::regex prefix(
      R"jfg(^\{"kind":"jfg-phase8-native-probe","status":"poll-target","poll_target":([1-9][0-9]*),"vi_retraces":([0-9]+),"vi_frames":([0-9]+),"vi_interrupts":([0-9]+),"vi_messages_delivered":([0-9]+),)jfg");
  static const std::regex samples(R"jfg(,"controller_samples":([0-9]+),)jfg");
  std::smatch prefix_match, sample_match;
  if (!std::regex_search(output, prefix_match, prefix) ||
      prefix_match[1].str() != std::to_string(poll_target) ||
      !std::regex_search(output, sample_match, samples) ||
      sample_match[1].str() != std::to_string(poll_target))
    return false;
  const std::string normalized =
      "{\"kind\":\"jfg-phase8-native-probe\",\"status\":\"retrace-target\","
      "\"retrace_target\":4,\"vi_retraces\":4,\"vi_frames\":4,"
      "\"vi_interrupts\":4,\"vi_messages_delivered\":4," +
      output.substr(prefix_match.length());
  std::string ignored;
  if (!extract_canonical_probe(normalized, ignored))
    return false;
  probe_line = output;
  if (probe_line.size() >= 2U &&
      probe_line[probe_line.size() - 2U] == '\r')
    probe_line.erase(probe_line.size() - 2U, 1U);
  return true;
}

bool extract_canonical_play_exit(const std::string &output,
                                 std::string &play_line) {
  static const std::regex pattern(
      R"jfg(\{"kind":"jfg-phase8-play","status":"window-closed","vi_retraces":[0-9]+,"graphics_tasks":[0-9]+,"presented_frames":[0-9]+,"decoded_audio_tasks":[0-9]+,"audio_device_initialized":(?:true|false),"audio_device_started":(?:true|false),"audio_device_frequency":[0-9]+,"audio_device_buffers":[0-9]+,"audio_device_consumed_bytes":[0-9]+,"audio_device_consumed_ms":[0-9]+,"audio_device_underruns":[0-9]+,"audio_device_overruns":[0-9]+,"frame_captured":(?:true|false),"controller_samples":[0-9]+,"non_neutral_controller_samples":[0-9]+,"non_neutral_controller_writes":[0-9]+,"controller_disconnects":[0-9]+,"controller_reconnects":[0-9]+,"game_pressed_observations":[0-9]+,"last_game_pressed_buttons":"0x[0-9a-f]{4}","game_stick_observations":[0-9]+,"last_game_stick_x":-?[0-9]+,"last_game_stick_y":-?[0-9]+,"menu_stick_observations":[0-9]+,"last_menu_stick_x":-?[0-9]+,"last_menu_stick_y":-?[0-9]+,"front_mode":[0-9]+,"unsupported_accesses":0,"journal_overflow":(?:true|false)\}\r?\n)jfg");
  std::smatch match;
  if (!std::regex_match(output, match, pattern))
    return false;
  play_line = match.str();
  if (play_line.size() >= 2U && play_line[play_line.size() - 2U] == '\r')
    play_line.erase(play_line.size() - 2U, 1U);
  return true;
}

std::wstring quote_argument(std::wstring_view value) {
  std::wstring quoted = L"\"";
  std::size_t slash_count = 0U;
  for (const wchar_t character : value) {
    if (character == L'\\') {
      ++slash_count;
      continue;
    }
    if (character == L'\"') {
      quoted.append(slash_count * 2U + 1U, L'\\');
      quoted += L'\"';
    } else {
      quoted.append(slash_count, L'\\');
      quoted += character;
    }
    slash_count = 0U;
  }
  quoted.append(slash_count * 2U, L'\\');
  quoted += L"\"";
  return quoted;
}

bool read_file(HANDLE file, std::string &output) {
  LARGE_INTEGER size{};
  if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > 65536)
    return false;
  output.resize(static_cast<std::size_t>(size.QuadPart));
  SetFilePointer(file, 0, nullptr, FILE_BEGIN);
  DWORD read = 0U;
  return output.empty() ||
         (ReadFile(file, output.data(), static_cast<DWORD>(output.size()),
                   &read, nullptr) != 0 &&
          read == output.size());
}

bool wide_from_utf8(const char *input, std::wstring &output) {
  if (input == nullptr) {
    return false;
  }
  constexpr int kMaximumWideCharacters = 32768;
  const int size =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1, nullptr, 0);
  if (size <= 1 || size > kMaximumWideCharacters) {
    return false;
  }
  output.resize(static_cast<std::size_t>(size));
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1,
                          output.data(), size) != size) {
    return false;
  }
  output.pop_back();
  return true;
}

bool parse_token(std::string_view text, std::array<std::uint8_t, 16U> &token) {
  if (text.size() != token.size() * 2U)
    return false;
  for (std::size_t index = 0; index < token.size(); ++index) {
    const auto digit = [](char value) -> int {
      if (value >= '0' && value <= '9')
        return value - '0';
      if (value >= 'a' && value <= 'f')
        return value - 'a' + 10;
      return -1;
    };
    const int high = digit(text[index * 2U]);
    const int low = digit(text[index * 2U + 1U]);
    if (high < 0 || low < 0)
      return false;
    token[index] = static_cast<std::uint8_t>((high << 4) | low);
  }
  return true;
}

bool validate_child_capability(const char *token_text,
                               const char *handle_text) {
  std::array<std::uint8_t, 16U> token{};
  if (token_text == nullptr || handle_text == nullptr ||
      !parse_token(token_text, token))
    return false;
  char *end = nullptr;
  const auto raw = std::strtoull(handle_text, &end, 16);
  if (*handle_text == '\0' || *end != '\0' || raw == 0U)
    return false;
  HANDLE marker = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(raw));
  DWORD flags = 0U;
  if (!GetHandleInformation(marker, &flags) ||
      (flags & HANDLE_FLAG_INHERIT) == 0U)
    return false;
  void *view = MapViewOfFile(marker, FILE_MAP_READ, 0, 0, token.size());
  if (view == nullptr)
    return false;
  const bool valid = std::memcmp(view, token.data(), token.size()) == 0;
  UnmapViewOfFile(view);
  return valid;
}

std::wstring token_text(const std::array<std::uint8_t, 16U> &token) {
  constexpr wchar_t digits[] = L"0123456789abcdef";
  std::wstring result(token.size() * 2U, L'0');
  for (std::size_t index = 0; index < token.size(); ++index) {
    result[index * 2U] = digits[token[index] >> 4U];
    result[index * 2U + 1U] = digits[token[index] & 15U];
  }
  return result;
}

bool module_filename(std::wstring &output) {
  std::vector<wchar_t> buffer(260U);
  for (;;) {
    const DWORD size = GetModuleFileNameW(nullptr, buffer.data(),
                                          static_cast<DWORD>(buffer.size()));
    if (size == 0U || size >= buffer.size() - 1U || size > 32767U) {
      if (size == 0U || buffer.size() >= 32768U) {
        return false;
      }
      buffer.resize(buffer.size() * 2U);
      continue;
    }
    output.assign(buffer.data(), size);
    return true;
  }
}

int run_parent(const char *executable_path, const char *rom_path,
               unsigned timeout_ms, const unsigned retrace_target,
               const unsigned poll_target, const bool play_mode) {
  (void)executable_path;
  std::vector<std::uint8_t> validated_rom;
  if (!read_supported_rom(rom_path, validated_rom))
    return 2;
  std::uint32_t entry_target = 0U;
  if (!reset_transfer_target(validated_rom, entry_target))
    return 3;
  wchar_t temp_path[MAX_PATH + 1U]{};
  wchar_t stdout_path[MAX_PATH + 1U]{};
  if (GetTempPathW(MAX_PATH, temp_path) == 0U ||
      GetTempFileNameW(temp_path, L"jfg", 0U, stdout_path) == 0U)
    return 3;
  SECURITY_ATTRIBUTES inheritable{};
  inheritable.nLength = sizeof(inheritable);
  inheritable.bInheritHandle = TRUE;
  HANDLE output =
      CreateFileW(stdout_path, GENERIC_READ | GENERIC_WRITE, 0, &inheritable,
                  CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
  if (output == INVALID_HANDLE_VALUE) {
    DeleteFileW(stdout_path);
    return 3;
  }
  if (SetHandleInformation(output, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT) ==
      0) {
    CloseHandle(output);
    DeleteFileW(stdout_path);
    return 3;
  }
  int result = 3;
  std::wstring executable;
  std::wstring rom;
  if (!module_filename(executable) || !wide_from_utf8(rom_path, rom)) {
    CloseHandle(output);
    DeleteFileW(stdout_path);
    return result;
  }
  std::wstring command = quote_argument(executable);
  command += L" --_jfg-phase6-child --rom ";
  command += quote_argument(rom);
  if (play_mode) {
    command += L" --play";
  } else if (poll_target != 0U) {
    command += L" --probe-polls ";
    command += std::to_wstring(poll_target);
  } else {
    command += L" --probe-retraces ";
    command += std::to_wstring(retrace_target);
  }
  HANDLE nul =
      CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                  &inheritable, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (nul == INVALID_HANDLE_VALUE) {
    CloseHandle(output);
    DeleteFileW(stdout_path);
    return result;
  }
  if (SetHandleInformation(nul, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT) ==
      0) {
    CloseHandle(nul);
    CloseHandle(output);
    DeleteFileW(stdout_path);
    return result;
  }
  std::array<std::uint8_t, 16U> token{};
  HANDLE marker =
      CreateFileMappingW(INVALID_HANDLE_VALUE, &inheritable, PAGE_READWRITE, 0U,
                         static_cast<DWORD>(token.size()), nullptr);
  void *marker_view = marker == nullptr ? nullptr
                                        : MapViewOfFile(marker, FILE_MAP_WRITE,
                                                        0U, 0U, token.size());
  if (marker == nullptr || marker_view == nullptr ||
      SystemFunction036(token.data(), static_cast<ULONG>(token.size())) ==
          FALSE) {
    if (marker_view != nullptr)
      UnmapViewOfFile(marker_view);
    if (marker != nullptr)
      CloseHandle(marker);
    CloseHandle(nul);
    CloseHandle(output);
    DeleteFileW(stdout_path);
    return result;
  }
  if (SetHandleInformation(marker, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT) ==
      0) {
    CloseHandle(marker);
    CloseHandle(nul);
    CloseHandle(output);
    DeleteFileW(stdout_path);
    return result;
  }
  std::memcpy(marker_view, token.data(), token.size());
  UnmapViewOfFile(marker_view);
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  startup.dwFlags = STARTF_USESTDHANDLES;
  startup.hStdOutput = output;
  startup.hStdError = output;
  startup.hStdInput = nul;
  PROCESS_INFORMATION child{};
  command += L" --_jfg-phase6-token ";
  command += quote_argument(token_text(token));
  command += L" --_jfg-phase6-marker ";
  wchar_t marker_text[2U + sizeof(std::uintptr_t) * 2U + 1U]{};
  std::swprintf(marker_text, std::size(marker_text), L"%llx",
                static_cast<unsigned long long>(
                    reinterpret_cast<std::uintptr_t>(marker)));
  command += marker_text;
  if (CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                     CREATE_NO_WINDOW, nullptr, nullptr, &startup,
                     &child) != 0) {
    const DWORD wait =
        WaitForSingleObject(child.hProcess, play_mode ? INFINITE : timeout_ms);
    DWORD exit_code = 0U;
    std::string child_output;
    if (wait == WAIT_TIMEOUT) {
      (void)TerminateProcess(child.hProcess, 4U);
      if (WaitForSingleObject(child.hProcess, INFINITE) == WAIT_OBJECT_0) {
        State state{};
        ledger(state, "watchdog", "wall-clock", entry_target);
        result = 4;
      }
    } else if (wait == WAIT_OBJECT_0 && FlushFileBuffers(output) != 0 &&
               read_file(output, child_output) &&
               GetExitCodeProcess(child.hProcess, &exit_code) != 0 &&
               (exit_code == 0U || exit_code == 4U) &&
               [&child_output, exit_code, retrace_target, poll_target,
                play_mode] {
                 std::string canonical_line;
                 const bool valid =
                     exit_code == 0U
                         ? (play_mode
                                ? extract_canonical_play_exit(
                                      child_output, canonical_line)
                            : poll_target != 0U
                                ? extract_canonical_poll_probe(
                                      child_output, poll_target, canonical_line)
                            : retrace_target == 3U
                                ? extract_canonical_success(child_output,
                                                            canonical_line)
                                : (extract_canonical_probe(child_output, canonical_line) ||
                                   extract_original_os_probe(child_output, retrace_target, canonical_line)))
                         : extract_canonical_ledger(child_output,
                                                    canonical_line);
                  if (!valid) {
                    return false;
                  }
                 child_output = std::move(canonical_line);
                 return true;
               }()) {
      std::fwrite(child_output.data(), 1U, child_output.size(), stdout);
      result = static_cast<int>(exit_code);
    } else {
#if defined(JFG_PHASE8_LIVE_RUNTIME)
      if (!child_output.empty())
        std::fwrite(child_output.data(), 1U, child_output.size(), stderr);
      std::fprintf(stderr, "native boot child rejected: exit=0x%08lx\n",
                   static_cast<unsigned long>(exit_code));
#endif
      if (wait == WAIT_FAILED ||
          GetExitCodeProcess(child.hProcess, &exit_code) == 0) {
        result = 3;
      } else {
        result = static_cast<int>(exit_code == 2U ? 2U : 3U);
      }
    }
    if (GetExitCodeProcess(child.hProcess, &exit_code)) {
      char support_code[32]{};
      std::snprintf(support_code, sizeof(support_code), "native_exit=0x%08lx", static_cast<unsigned long>(exit_code));
      jfg::support_event(support_code);
    }
    CloseHandle(child.hThread);
    CloseHandle(child.hProcess);
  }
  CloseHandle(marker);
  CloseHandle(nul);
  CloseHandle(output);
  DeleteFileW(stdout_path);
  return result;
}
#endif

int run(const char *executable_path, const char *rom_path, unsigned timeout_ms,
        bool child_mode, const char *child_token, const char *marker_handle,
        unsigned retrace_target, unsigned poll_target, bool play_mode) {
  jfg::support_event("native=boot");
  if (child_mode) {
#if defined(_WIN32)
    if (!validate_child_capability(child_token, marker_handle))
      return 3;
#else
    return 3;
#endif
    return run_child(rom_path, retrace_target, poll_target, play_mode);
  }
#if defined(_WIN32)
  return run_parent(executable_path, rom_path, timeout_ms, retrace_target,
                    poll_target, play_mode);
#else
  (void)executable_path;
  (void)timeout_ms;
  (void)child_token;
  (void)marker_handle;
  std::fputs(
      "native boot setup failed: process isolation unavailable on this host\n",
      stderr);
  return 3;
#endif
}
} // namespace jfg::boot::native
