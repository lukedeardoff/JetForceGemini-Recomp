#include "rt64_f3ddkr.hpp"
#include "jfg/renderer/frame_diagnostics_state.hpp"
#include "jfg/renderer/interpolation_bridge.hpp"
#include "jfg/renderer/presentation_identity.hpp"
#include "jfg/renderer/semantic_presentation.hpp"
#include <bit>
#include "jfg/renderer/rt64_f3ddkr_address.hpp"

#include "gbi/rt64_gbi_f3d.h"
#include "gbi/rt64_gbi_rdp.h"
#include "hle/rt64_application.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <future>
#include "jfg/renderer/floating_environment.hpp"
#include <unordered_map>
#include <unordered_set>
#include <cstdio>
#include <share.h>
#include <direct.h>
#include <cstdlib>
#include <string>

namespace jfg {
namespace {

// F3DDKR texture-offset table (opcode 0x02), as JFG's own RSP microcode does
// it (disassembled 2026-10-11, IMEM 0x1328-0x135C and 0x14D4-0x1500):
// - 0x02 DMAs the table and copies its first 32 u16 entries (64 bytes) to DMEM
//   0x320, sets the index to 0 and enables the table (DMEM 0x1E6/0x1E7).
// - Every G_SETTIMG in the display list then adds the next entry (unsigned) to
//   its address and advances the index, whatever the image format.
// - BF (DMA offsets) disables the table. Commands inside a 0x07 DMA list are
//   sent to the RDP untouched and do not use the table.
// The game's model renderer (objPrintModelObject) gives every model a 40-entry
// table that it rewrites each frame for animated textures (frame offsets);
// unanimated models get the all-zero table at 0x800A0838. GLideN64's heuristic
// (RGBA images only, undo on some LOADBLOCKs, index per LOADBLOCK), which this
// file used until 2026-10-11, froze animated IA/I textures on frame 0: Juno's
// water splash never played its widen-and-fade animation.
constexpr std::size_t kTextureTableEntries = 32U;

// Asteroid investigation (2026-10-07): texture-image log + texture-shift switch.
//   JFG_TEXLOG=1          write diag\texlog.tsv (between JFG_TEXLOG_FROM_S and
//                         JFG_TEXLOG_TO_S seconds after the first graphics task)
//   JFG_TEXSHIFT_MODE=0   microcode behaviour above (default)
//   JFG_TEXSHIFT_MODE=1   ignore DKR/JFG texture-offset shifts (test only)
//   JFG_TEXSHIFT_MODE=2   the old GLideN64 heuristic (for comparison)
// Lines: T task ms | O offset_addr | I fmt siz width base final offset count shift
//        | B lrs block_bytes shift action(0=count++,1=undo)
// JFG's texture-offset tables are 40 u16 entries (0x50 bytes), double
// buffered back to back (two tables 0x50 apart per scene). A texture image
// past entry 39 reads the other buffer's entry 0 (an animated frame offset
// such as 4096) and is loaded from the wrong address: the intro asteroids
// turned "psychedelic" every other frame. Stop applying the table past its
// last entry. JFG_TEXTABLE_ENTRIES overrides the size (0 = no limit).
// Upstream 500b2a2 resets the table on BF (the original RSP behaviour), which
// supersedes this cap, so it is off by default (0 = no limit).
constexpr std::size_t kDefaultTextureOffsetEntries = 0U;

struct TexLog {
    bool ready = false;
    std::size_t table_entries = kDefaultTextureOffsetEntries;
    bool enabled = false;
    int shift_mode = 0;
    long long from_ms = 20000, to_ms = 75000;
    std::FILE* file = nullptr;
    std::uint64_t task = 0U;
    long long now_ms = 0;
    std::uint64_t lines = 0U;
    std::chrono::steady_clock::time_point start{};
    bool started = false;
};
TexLog texlog;

long long texlog_env(const char* name, long long fallback) {
    char* value = nullptr;
    std::size_t length = 0U;
    long long result = fallback;
    if (_dupenv_s(&value, &length, name) == 0 && value != nullptr) {
        if (*value != '\0') result = std::atoll(value);
    }
    std::free(value);
    return result;
}

void texlog_init() {
    if (texlog.ready) return;
    texlog.ready = true;
    texlog.enabled = texlog_env("JFG_TEXLOG", 0) != 0;
    texlog.shift_mode = static_cast<int>(texlog_env("JFG_TEXSHIFT_MODE", 0));
    texlog.table_entries = static_cast<std::size_t>(
        texlog_env("JFG_TEXTABLE_ENTRIES", static_cast<long long>(kDefaultTextureOffsetEntries)));
    texlog.from_ms = texlog_env("JFG_TEXLOG_FROM_S", 20) * 1000;
    texlog.to_ms = texlog_env("JFG_TEXLOG_TO_S", 75) * 1000;
    if (!texlog.enabled) return;
    wchar_t* base = nullptr;
    std::size_t length = 0U;
    if (_wdupenv_s(&base, &length, L"LOCALAPPDATA") == 0 && base != nullptr) {
        std::wstring path = std::wstring(base) + L"\\JFGRecomp\\diag\\texlog.tsv";
        if (_wfopen_s(&texlog.file, path.c_str(), L"w") != 0) texlog.file = nullptr;
    }
    std::free(base);
    if (texlog.file != nullptr)
        std::fprintf(texlog.file, "# texlog v2 shift_mode=%d table_entries=%zu\n", texlog.shift_mode, texlog.table_entries);
    else
        texlog.enabled = false;
}

bool texlog_on() {
    return texlog.enabled && texlog.file != nullptr &&
           texlog.now_ms >= texlog.from_ms && texlog.now_ms <= texlog.to_ms &&
           texlog.lines < 3000000U;
}

// JFG_RIPPLE_LOG=1: diag\ripple.tsv (water splash/wake investigation).
// Translucent textured triangles (ZMODE_XLU with FORCE_BL, the wake family)
// are aggregated per DMA-triangle command and render state:
//   G ms task wid omL omH ccL ccH geom tri_src vtx_src
//     texsrc fmt/siz/width bytes load(1 block, 2 tile)/rt64_diff (bytes where
//     RT64's RDRAM copy of the texture differs from the guest's)/overlapping
//     RT64 framebuffer address/width/last write type
//     tile(uls ult lrs lrt cms cmt masks maskt shifts shiftt fmt siz line tmem)
//     prim env | n screen-bbox object-bbox u0,u1,v0,v1 a0,a1 r0,r1
//     then up to 6 corners: x,y,rgba,u,v (u/v 10.5)
//   TASK task ms        one per graphics task (frame marker)
//   TEX addr fmt/siz/width bytes hex   first use of each texture source
// texsrc is the RDRAM address the tile-0 TMEM area was last loaded from,
// so CPU-chosen flipbook frames show up as changing addresses.
struct RippleLog {
    bool ready = false;
    std::FILE* file = nullptr;
    std::uint64_t lines = 0U;
    std::chrono::steady_clock::time_point start{};
};
RippleLog ripple_log;

std::FILE* ripple_log_file() {
    if (!ripple_log.ready) {
        ripple_log.ready = true;
        ripple_log.start = std::chrono::steady_clock::now();
        if (texlog_env("JFG_RIPPLE_LOG", 0) != 0) {
            wchar_t* base = nullptr;
            std::size_t length = 0U;
            if (_wdupenv_s(&base, &length, L"LOCALAPPDATA") == 0 && base != nullptr) {
                std::wstring path = std::wstring(base) + L"\\JFGRecomp\\diag\\ripple.tsv";
                // Shared read so the log can be inspected while the game runs.
                ripple_log.file = _wfsopen(path.c_str(), L"w", _SH_DENYNO);
            }
            std::free(base);
        }
    }
    if (ripple_log.file == nullptr || ripple_log.lines >= 400000U) return nullptr;
    return ripple_log.file;
}

struct RippleCorner {
    float x = 0.0F, y = 0.0F;
    unsigned rgba = 0U;
    int u = 0, v = 0;
};

struct RippleGroup {
    bool active = false;
    std::string key;
    std::string head;
    unsigned triangles = 0U;
    float sx0 = 0, sy0 = 0, sx1 = 0, sy1 = 0;
    float ox0 = 0, oy0 = 0, oz0 = 0, ox1 = 0, oy1 = 0, oz1 = 0;
    int u0 = 0, u1 = 0, v0 = 0, v1 = 0;
    unsigned a0 = 0, a1 = 0, c0 = 0, c1 = 0;
    std::array<RippleCorner, 6U> corners{};
    std::size_t corner_count = 0U;
};
RippleGroup ripple_group;

// RDRAM source of the most recent load into each TMEM word (ripple log).
struct TmemSource {
    std::uint32_t address = 0U;
    std::uint32_t bytes = 0U;
    unsigned fmt = 0U, siz = 0U, width = 0U, load = 0U;
};
std::array<TmemSource, 512U> tmem_sources{};
std::unordered_set<std::uint32_t> ripple_dumped;
std::uint32_t ripple_vertex_source = 0U;

// Community geometry dump (2026-10-19, title-screen hallway). When the game is
// started with JFG_GEOMDUMP=N, pressing F9 (recording start) asks for the next
// N graphics tasks to be written to diag\geometry\task-<task>-vi<tick>.tsv:
//   T  task vi_tick
//   CI address fmt siz width                       set color image
//   X  seq fbpair proj calls flags cull skipped clip omL omH geom ccL ccH
//      texsrc fmt/siz/width, then per corner: x y z w (clip) sx sy sz (screen)
//   R  w0 w1 w2/w3 w4/w5 omL omH ccL ccH texsrc      texture rectangle
//   F  w0 w1 omL omH                                 fill rectangle
//   B  first last clip                               0xBE (ignored)
// fbpair/proj/calls are RT64's counters when the triangle is decoded; the
// inspector's "Framebuffers #f - <type> #p - Call #c" numbering follows them.
std::atomic<unsigned> geomdump_pending{0U};
struct GeomDump {
    std::FILE* file = nullptr;
    std::uint64_t task = 0U;
    std::uint64_t seq = 0U;
};
GeomDump geomdump;

void geomdump_begin_task(const std::uint64_t task, const std::uint64_t vi_tick) {
    if (geomdump.file != nullptr) {
        std::fclose(geomdump.file);
        geomdump.file = nullptr;
    }
    unsigned pending = geomdump_pending.load();
    while (pending != 0U &&
           !geomdump_pending.compare_exchange_weak(pending, pending - 1U)) {
    }
    if (pending == 0U)
        return;
    wchar_t* base = nullptr;
    std::size_t length = 0U;
    if (_wdupenv_s(&base, &length, L"LOCALAPPDATA") == 0 && base != nullptr) {
        const std::wstring folder = std::wstring(base) + L"\\JFGRecomp\\diag\\geometry";
        (void)_wmkdir(folder.c_str());
        const std::wstring path = folder + L"\\task-" + std::to_wstring(task) +
            L"-vi" + std::to_wstring(vi_tick) + L".tsv";
        geomdump.file = _wfsopen(path.c_str(), L"w", _SH_DENYNO);
    }
    std::free(base);
    geomdump.task = task;
    geomdump.seq = 0U;
    if (geomdump.file != nullptr)
        std::fprintf(geomdump.file, "T\t%llu\t%llu\n",
            static_cast<unsigned long long>(task),
            static_cast<unsigned long long>(vi_tick));
}

struct GeomDumpCounters {
    unsigned fbpair = 0U, proj = 0U, calls = 0U;
};

GeomDumpCounters geomdump_counters(RT64::State& state) {
    GeomDumpCounters counters{};
    const RT64::Workload& workload =
        state.ext.workloadQueue->workloads[state.ext.workloadQueue->writeCursor];
    if (workload.fbPairCount == 0U || workload.fbPairCount > workload.fbPairs.size())
        return counters;
    counters.fbpair = workload.fbPairCount - 1U;
    const RT64::FramebufferPair& pair = workload.fbPairs[counters.fbpair];
    if (pair.projectionCount == 0U || pair.projectionCount > pair.projections.size())
        return counters;
    counters.proj = pair.projectionCount - 1U;
    counters.calls = pair.projections[counters.proj].gameCallCount;
    return counters;
}

void ripple_group_flush() {
    if (!ripple_group.active) return;
    std::FILE* log = ripple_log_file();
    if (log != nullptr) {
        const RippleGroup& g = ripple_group;
        std::fprintf(log, "%s\t%u\t%.1f,%.1f,%.1f,%.1f\t%.0f,%.0f,%.0f,%.0f,%.0f,%.0f\t%d,%d,%d,%d\t%u,%u\t%u,%u",
            g.head.c_str(), g.triangles,
            static_cast<double>(g.sx0), static_cast<double>(g.sy0), static_cast<double>(g.sx1), static_cast<double>(g.sy1),
            static_cast<double>(g.ox0), static_cast<double>(g.oy0), static_cast<double>(g.oz0),
            static_cast<double>(g.ox1), static_cast<double>(g.oy1), static_cast<double>(g.oz1),
            g.u0, g.u1, g.v0, g.v1, g.a0, g.a1, g.c0, g.c1);
        for (std::size_t index = 0U; index < g.corner_count; ++index) {
            const RippleCorner& corner = g.corners[index];
            std::fprintf(log, "\t%.1f,%.1f,%08x,%d,%d",
                static_cast<double>(corner.x), static_cast<double>(corner.y),
                corner.rgba, corner.u, corner.v);
        }
        std::fputc('\n', log);
        if ((++ripple_log.lines % 32U) == 0U) std::fflush(log);
    }
    ripple_group.active = false;
}

constexpr std::uint8_t kOpDmaMatrix = 0x01U;
constexpr std::uint8_t kOpDmaTextureOffset = 0x02U;
constexpr std::uint8_t kOpDmaVertex = 0x04U;
constexpr std::uint8_t kOpDmaTriangles = 0x05U;
constexpr std::uint8_t kOpDisplayList = 0x06U;
constexpr std::uint8_t kOpDmaDisplayList = 0x07U;
constexpr std::uint8_t kOpSetPerspNorm = 0xB5U;
constexpr std::uint8_t kOpCullDisplayList = 0xBEU;
constexpr std::uint8_t kOpPopMatrix = 0xBDU;
constexpr std::uint8_t kOpMoveWord = 0xBCU;
constexpr std::uint8_t kOpDmaOffsets = 0xBFU;
constexpr std::uint8_t kOpSetTextureImage = 0xFDU;
constexpr std::uint8_t kOpSetDepthImage = 0xFEU;
constexpr std::uint8_t kOpSetColorImage = 0xFFU;
constexpr std::uint8_t kMoveWordBillboard = 0x02U;
constexpr std::uint8_t kMoveWordModelMatrix = 0x0AU;
constexpr std::size_t kMatrixCount = 32U;
constexpr std::size_t kVertexCount = 80U;
constexpr std::uint32_t kRdramBytes = 8U * 1024U * 1024U;
constexpr std::uint32_t kScratchAddress = kRdramBytes - 4096U;
// A guest list that branches to itself would otherwise spin forever on the
// game thread (jump form) or grow RT64's unbounded return stack (call form).
// The largest observed Goldwood task issues a few hundred G_DL edges; the
// budget is generous so only a cycle or a wild pointer can exhaust it.
constexpr std::size_t kMaximumDisplayListBranches = 65536U;
// The RSP microcode's own display-list stack is far shallower than this.
constexpr std::size_t kMaximumReturnStackDepth = 64U;
// F3D G_MOVEMEM payloads are at most 64 bytes (a full light set).
constexpr std::uint32_t kMaximumMoveMemBytes = 64U;

struct SemanticVertexState {
    SemanticPresentationSample sample;
    PresentationMatrix world{};
    std::uint32_t source = 0;
    std::array<std::uint64_t, 4> billboard_key{}, rigid_key{};
    // Points into this task's immutable identities; consumed before begin()
    // replaces that shared snapshot or clears the per-task vertex metadata.
    const RigidPresentationPose* rigid_pose = nullptr;
};
struct Context {
    RT64::Application* application = nullptr;
    RT64::GBI gbi{};
    std::array<hlslpp::float4x4, kMatrixCount> matrices{};
    std::array<std::uint32_t, kMatrixCount> matrix_segmented_addresses{};
    std::array<std::uint32_t, kMatrixCount> matrix_physical_addresses{};
    std::bitset<kMatrixCount> loaded_matrices{};
    std::uint32_t matrix_offset = 0U;
    std::uint32_t vertex_offset = 0U;
    std::uint32_t texture_offset = 0U;
    std::uint32_t texture_shift = 0U;
    std::size_t texture_count = 0U;
    std::array<std::uint16_t, kTextureTableEntries> texture_table{};
    std::size_t texture_index = 0U;
    bool texture_table_enabled = false;
    bool in_dma_list = false;
    // The game's G_CULL_FRONT bit (see dma_triangles): set by
    // G_SETGEOMETRYMODE, cleared by G_CLEARGEOMETRYMODE.
    bool cull_front_switch = false;
    std::size_t vertex_cursor = 0U;
    std::size_t selected_matrix = 0U;
    std::size_t commands = 0U;
    Rt64F3ddkrStats stats{};
    std::bitset<kVertexCount> loaded_vertices{};
    std::span<const std::uint8_t> simulation_rdram{};
    bool interpolate = false;
    std::uint64_t vi_tick = 0U;
    PresentationHistory presentation_history;
    PresentationHistory rigid_baseline_history; // Used only by explicit diagnostics.
    PresentationHistory shared_matrix_baseline_history; // Same-input old-policy diagnostic.
    PresentationHistory camera_shape_baseline_history; // Explicit comparison only.
    SemanticPresentationHistory semantic_history;
    std::shared_ptr<const PresentationSceneIdentity> identities;
    std::array<hlslpp::float4x4,kMatrixCount> model_matrices{}, camera_matrices{};
    std::array<std::uint64_t,kMatrixCount> camera_ids{};
    std::array<bool,kMatrixCount> model_finite{};
    bool semantic_target = false;
    std::vector<SemanticVertexState> semantic_vertices;
    // Title-layer fade (see title_fade_mode): whether it runs in this task,
    // the global vertices of drawn layer triangles (paired with their
    // texture image as a strip key) and those of every other drawn triangle.
    bool title_fade = false;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> title_fade_layer;
    std::vector<std::uint32_t> title_fade_other;
    bool billboard = false;
    bool failed = false;
};

std::mutex contexts_mutex;
std::unordered_map<RT64::State*, Context*> contexts;
// Bound only during synchronous interpretation by the owning adapter. This
// avoids a global mutex/hash lookup for every command without retaining a
// pointer after the call, across destruction, or on another renderer thread.
thread_local Context* dispatch_context = nullptr;


[[nodiscard]] Context* find_context(RT64::State* state) noexcept {
    if (dispatch_context && dispatch_context->application->state.get() == state)
        return dispatch_context;
    const std::scoped_lock lock(contexts_mutex);
    const auto iterator = contexts.find(state);
    return iterator == contexts.end() ? nullptr : iterator->second;
}

void reject(RT64::State* state, RT64::DisplayList** dl) noexcept {
    if (Context* context = find_context(state); context != nullptr) {
        context->failed = true;
        // A handler that already recorded a specific cause keeps it. Generic
        // rejections (an unsupported opcode reached by running past the end
        // of a list, for example) fall back to the most recent G_DL edge so
        // triage can distinguish a bad pointer from a list that ran on.
        if (context->stats.rejection_reason == 0U &&
            context->stats.rejection_source_address == 0U) {
            context->stats.rejection_source_address =
                context->stats.last_display_list_address;
            context->stats.rejection_detail =
                context->stats.last_display_list_target;
        }
        if (dl != nullptr && *dl != nullptr) {
            context->stats.rejected_command_word0 = (**dl).w0;
            context->stats.rejected_command_word1 = (**dl).w1;
            const std::uintptr_t cursor =
                reinterpret_cast<std::uintptr_t>(*dl);
            const std::array<std::uintptr_t, 2U> bases{
                state == nullptr
                    ? 0U
                    : reinterpret_cast<std::uintptr_t>(state->RDRAM),
                reinterpret_cast<std::uintptr_t>(
                    context->simulation_rdram.data()),
            };
            for (const std::uintptr_t base : bases) {
                if (base != 0U && cursor >= base &&
                    cursor - base <= kRdramBytes - sizeof(**dl)) {
                    context->stats.rejected_command_address =
                        static_cast<std::uint32_t>(cursor - base);
                    break;
                }
            }
        }
    }
    if (dl != nullptr) {
        *dl = nullptr;
    }
}

[[nodiscard]] bool physical_range(
    const std::uint32_t address,
    const std::size_t length) noexcept {
    return address <= kRdramBytes && length <= kRdramBytes - address;
}

[[nodiscard]] std::uint8_t guest_byte(
    const Context& context,
    const RT64::State& state,
    const std::uint32_t address) noexcept {
    if (context.simulation_rdram.size() == kRdramBytes) {
        return context.simulation_rdram[address ^ 3U];
    }
    return state.RDRAM[address ^ 3U];
}

[[nodiscard]] std::int16_t guest_s16(
    const Context& context,
    const RT64::State& state,
    const std::uint32_t address) noexcept {
    const std::uint16_t value =
        (static_cast<std::uint16_t>(guest_byte(context, state, address)) << 8U) |
        guest_byte(context, state, address + 1U);
    return static_cast<std::int16_t>(value);
}

[[nodiscard]] std::uint32_t segmented_physical(
    RT64::State& state,
    const std::uint32_t address) noexcept {
    return detail::resolve_f3ddkr_byte_source(
        state.rsp->fromSegmented(address));
}

void select_matrix(Context& context) noexcept {
    RT64::RSP& rsp = *context.application->state->rsp;
    const std::size_t index = context.selected_matrix;
    rsp.modelMatrixStackSize = 1;
    rsp.modelMatrixStack[0] = context.matrices[index];
    rsp.modelMatrixSegmentedAddressStack[0] =
        context.matrix_segmented_addresses[index];
    rsp.modelMatrixPhysicalAddressStack[0] =
        context.matrix_physical_addresses[index];
    RT64::TransformGroup& group = rsp.extended.modelMatrixIdStack[0];
    group.matrixId = G_EX_ID_IGNORE;
    group.decompose = false;
    group.positionInterpolation = G_EX_COMPONENT_SKIP;
    group.rotationInterpolation = G_EX_COMPONENT_SKIP;
    group.scaleInterpolation = G_EX_COMPONENT_SKIP;
    group.skewInterpolation = G_EX_COMPONENT_SKIP;
    group.perspectiveInterpolation = G_EX_COMPONENT_SKIP;
    group.tileInterpolation = G_EX_COMPONENT_SKIP;
    group.ordering = G_EX_ORDER_LINEAR;
    rsp.extended.modelMatrixIdStackChanged = true;

    const int projection = rsp.projectionMatrixStackSize - 1;
    const hlslpp::float4x4 identity = hlslpp::float4x4::identity();
    rsp.viewMatrixStack[projection] = identity;
    rsp.projMatrixStack[projection] = identity;
    rsp.viewProjMatrixStack[projection] = identity;
    rsp.invViewProjMatrixStack[projection] = identity;
    rsp.projectionMatrixInversed = true;
    rsp.modelViewProjInserted = false;
    rsp.modelViewProjChanged = true;
}

void unsupported(RT64::State* state, RT64::DisplayList** dl) {
    reject(state, dl);
}

void no_op(RT64::State*, RT64::DisplayList**) {
}

void dma_matrix(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    ++context->commands;
    ++context->stats.matrix_commands;
    const RT64::DisplayList command = **dl;
    const detail::F3ddkrMatrixCommand decoded_command =
        detail::decode_f3ddkr_matrix_command(command.w0);
    const std::size_t index = decoded_command.index;
    const bool multiply = decoded_command.multiply;
    const std::uint32_t segmented = state->rsp->fromSegmented(command.w1);
    if ((command.w0 & 0xFFFFU) != 64U ||
        index >= context->matrices.size()) {
        reject(state, dl);
        return;
    }
    const std::uint32_t address = detail::resolve_f3ddkr_dma_base(
        segmented, context->matrix_offset);
    if (!physical_range(address, sizeof(RT64::FixedMatrix))) {
        reject(state, dl);
        return;
    }
    const auto* fixed = reinterpret_cast<const RT64::FixedMatrix*>(
        context->simulation_rdram.size() == kRdramBytes
            ? context->simulation_rdram.data() + address
            : state->fromRDRAM(address));
    hlslpp::float4x4 decoded = fixed->toMatrix4x4();
    const auto model_matrix = decoded;
    if (multiply) {
        if (!context->loaded_matrices.test(0U)) {
            reject(state, dl);
            return;
        }
        // JFG's multiply form composes the object matrix with slot zero's
        // already-combined camera MVP. RT64 uses row-vector matrices here.
        decoded = hlslpp::mul(decoded, context->matrices[0U]);
    }
    context->camera_ids[index] = 0U;
    if (multiply && index != 0U && context->interpolate && context->identities) {
        context->model_matrices[index] = model_matrix;
        context->camera_matrices[index] = context->matrices[0U];
        context->camera_ids[index] = context->camera_ids[0U];
        const auto inverse = hlslpp::inverse(model_matrix);
        bool finite = true;
        for(int r=0;r<4;++r) for(int c=0;c<4;++c)
            finite = finite && std::isfinite(float(model_matrix[r][c])) && std::isfinite(float(inverse[r][c]));
        context->model_finite[index] = finite;
    }
    else if (!multiply && index == 0U && context->identities) {
        for (const auto& camera : context->identities->cameras)
            if (camera.matrix == address) context->camera_ids[index] = camera.identity;
    }
    context->matrices[index] = decoded;
    context->loaded_matrices.set(index);
    context->matrix_segmented_addresses[index] = command.w1;
    context->matrix_physical_addresses[index] = address;
    context->selected_matrix = index;
    select_matrix(*context);
}

void dma_texture_offset(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    ++context->commands;
    context->texture_offset = segmented_physical(*state, (*dl)->w1) &
        0x00FF'FFF8U;
    context->texture_shift = 0U;
    context->texture_count = 0U;
    context->texture_table.fill(0U);
    if (physical_range(context->texture_offset, kTextureTableEntries * 2U)) {
        for (std::size_t entry = 0U; entry < kTextureTableEntries; ++entry) {
            context->texture_table[entry] = static_cast<std::uint16_t>(guest_s16(
                *context, *state,
                context->texture_offset + static_cast<std::uint32_t>(entry * 2U)));
        }
    }
    context->texture_index = 0U;
    context->texture_table_enabled = texlog.shift_mode == 0;
    if (texlog_on()) {
        ++texlog.lines;
        std::fprintf(texlog.file, "O\t%x\t%x\t%x\n", context->texture_offset, (*dl)->w0, (*dl)->w1);
    }
    if (texlog.shift_mode != 2) {
        // The legacy path below reads texture_offset; only mode 2 uses it.
        context->texture_offset = 0U;
    }
}

void set_color_image(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    const std::uint8_t format = static_cast<std::uint8_t>(
        (*dl)->p0(21U, 3U));
    const std::uint8_t size = static_cast<std::uint8_t>(
        (*dl)->p0(19U, 2U));
    const std::uint16_t width = static_cast<std::uint16_t>(
        (*dl)->p0(0U, 12U) + 1U);
    context->semantic_target = format == G_IM_FMT_RGBA && size == G_IM_SIZ_16b && width >= 320U;
    context->stats.color_image_address =
        segmented_physical(*state, (*dl)->w1);
    state->rdp->setColorImage(
        format, size, width, context->stats.color_image_address);
    if (geomdump.file != nullptr)
        std::fprintf(geomdump.file, "CI\t%06x\t%u\t%u\t%u\n",
            static_cast<unsigned>(context->stats.color_image_address),
            static_cast<unsigned>(format), static_cast<unsigned>(size),
            static_cast<unsigned>(width));
}

void set_depth_image(RT64::State* state, RT64::DisplayList** dl) {
    state->rdp->setDepthImage(segmented_physical(*state, (*dl)->w1));
}

void set_texture_image(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    const std::uint8_t format = static_cast<std::uint8_t>(
        (*dl)->p0(21U, 3U));
    const std::uint8_t size = static_cast<std::uint8_t>(
        (*dl)->p0(19U, 2U));
    const std::uint16_t width = static_cast<std::uint16_t>(
        (*dl)->p0(0U, 12U) + 1U);
    std::uint32_t address = segmented_physical(*state, (*dl)->w1);
    const std::uint32_t texlog_base = address;
    const std::uint32_t texlog_offset = context->texture_offset;
    const std::size_t texlog_count = context->texture_count;
    if (context->texture_offset != 0U && texlog.table_entries != 0U &&
        context->texture_count >= texlog.table_entries) {
        // Past the end of the table: stop applying it (see note above).
        if (texlog_on()) {
            ++texlog.lines;
            std::fprintf(texlog.file, "X\t%x\t%zu\n", context->texture_offset,
                context->texture_count);
        }
        context->texture_offset = 0U;
        context->texture_shift = 0U;
        context->texture_count = 0U;
    }
    if (context->texture_table_enabled && !context->in_dma_list) {
        const std::uint16_t entry = context->texture_index < kTextureTableEntries
            ? context->texture_table[context->texture_index]
            : std::uint16_t{0U};
        ++context->texture_index;
        context->texture_shift = entry;
        address = detail::resolve_f3ddkr_byte_source(address + entry);
    }
    else if (context->texture_offset != 0U) {
        if (format == G_IM_FMT_RGBA) {
            const std::uint32_t shift_address = context->texture_offset +
                static_cast<std::uint32_t>(context->texture_count * 2U);
            if (!physical_range(shift_address, 2U)) {
                reject(state, dl);
                return;
            }
            context->texture_shift = static_cast<std::uint16_t>(
                guest_s16(*context, *state, shift_address));
            address = detail::resolve_f3ddkr_byte_source(
                address + context->texture_shift);
        }
        else {
            context->texture_offset = 0U;
            context->texture_shift = 0U;
            context->texture_count = 0U;
        }
    }
    if (texlog_on()) {
        ++texlog.lines;
        std::fprintf(texlog.file, "I\t%u\t%u\t%u\t%x\t%x\t%x\t%zu\t%u\t%x\n",
            format, size, width, texlog_base, address, texlog_offset,
            context->texture_table_enabled ? context->texture_index : texlog_count,
            static_cast<unsigned>(context->texture_shift), (*dl)->w0);
    }
    state->rdp->setTextureImage(format, size, width, address);
}

// Menu icons in widescreen (community fix, 2026-10-08). In 4:3 the game
// draws the START/PLAY/MULTI/OPTIONS icons as unscaled copy-mode texrects,
// which skip alpha-0 texels. Widescreen scales them by 4/3, so it uses a
// 1-cycle texrect (OtherMode L 0x00552048: AA, IM_RD, ALPHA_CVG_SEL, no alpha
// compare). Each 44x44 icon has a 1-px ring of alpha-0 grey texels that then
// draws as an opaque grey box. Give exactly this texrect mode the alpha
// threshold (blend alpha 100/255) so the ring is discarded as in copy mode.
// JFG_MENU_ICON_ALPHA=0 restores the plain behaviour.
constexpr std::uint32_t kMenuIconOtherModeL = 0x00552048U;

bool menu_icon_alpha_enabled() noexcept {
    static const bool enabled = texlog_env("JFG_MENU_ICON_ALPHA", 1) != 0;
    return enabled;
}

void texture_rectangle_with_icon_alpha(
    RT64::State* state, RT64::DisplayList** dl, const bool flip) {
    RT64::RDP& rdp = *state->rdp;
    const std::uint32_t high = rdp.otherMode.H;
    const std::uint32_t low = rdp.otherMode.L;
    const bool one_cycle = ((high >> 20U) & 0x3U) == 0U;
    const bool patch = menu_icon_alpha_enabled() && one_cycle &&
        low == kMenuIconOtherModeL;
    if (patch) {
        rdp.setOtherMode(high, low | 0x1U);  // G_AC_THRESHOLD
    }
    if (geomdump.file != nullptr) {
        const auto& cc = rdp.colorCombinerStack[rdp.colorCombinerStackSize - 1];
        const TmemSource& source = tmem_sources[rdp.tiles[((*dl)->w1 >> 24U) & 7U].tmem & 511U];
        std::fprintf(geomdump.file,
            "R\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%06x\t%u/%u/%u\t%u\n",
            static_cast<unsigned>((*dl)->w0), static_cast<unsigned>((*dl)->w1),
            static_cast<unsigned>((*dl)[1].w1), static_cast<unsigned>((*dl)[2].w1),
            static_cast<unsigned>(low), static_cast<unsigned>(high),
            static_cast<unsigned>(cc.L), static_cast<unsigned>(cc.H),
            static_cast<unsigned>(rdp.colorImage.address),
            static_cast<unsigned>(source.address), source.fmt, source.siz, source.width,
            flip ? 1U : 0U);
    }
    if (flip) {
        RT64::GBI_RDP::texrectFlip(state, dl);
    }
    else {
        RT64::GBI_RDP::texrect(state, dl);
    }
    if (patch) {
        rdp.setOtherMode(high, low);
    }
}

void texture_rectangle(RT64::State* state, RT64::DisplayList** dl) {
    texture_rectangle_with_icon_alpha(state, dl, false);
}

void fill_rectangle(RT64::State* state, RT64::DisplayList** dl) {
    if (geomdump.file != nullptr)
        std::fprintf(geomdump.file, "F\t%08x\t%08x\t%08x\t%08x\t%06x\n",
            static_cast<unsigned>((*dl)->w0), static_cast<unsigned>((*dl)->w1),
            static_cast<unsigned>(state->rdp->otherMode.L),
            static_cast<unsigned>(state->rdp->otherMode.H),
            static_cast<unsigned>(state->rdp->colorImage.address));
    RT64::GBI_RDP::fillRect(state, dl);
}

void texture_rectangle_flip(RT64::State* state, RT64::DisplayList** dl) {
    texture_rectangle_with_icon_alpha(state, dl, true);
}

void load_block(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    const std::uint16_t lrs = static_cast<std::uint16_t>(
        (*dl)->p1(12U, 12U));
    if (context->texture_offset != 0U) {
        const std::uint32_t block_bytes = ((lrs >> 2U) + 1U) << 3U;
        if (block_bytes != 0U &&
            (context->texture_shift % block_bytes) != 0U) {
            // The shift is a raw guest s16. Undoing a shift larger than the
            // current texture address would wrap to a ~4 GiB address that
            // RT64's loadBlock dereferences unchecked.
            if (context->texture_shift > state->rdp->texture.address) {
                context->stats.rejection_reason = kF3ddkrRejectTextureShiftWrap;
                context->stats.rejection_detail = context->texture_shift;
                reject(state, dl);
                return;
            }
            if (texlog_on()) {
                ++texlog.lines;
                std::fprintf(texlog.file, "B\t%u\t%u\t%u\t1\n", lrs,
                    block_bytes, static_cast<unsigned>(context->texture_shift));
            }
            state->rdp->texture.address -= context->texture_shift;
            context->texture_offset = 0U;
            context->texture_shift = 0U;
            context->texture_count = 0U;
        }
        else {
            if (texlog_on()) {
                ++texlog.lines;
                std::fprintf(texlog.file, "B\t%u\t%u\t%u\t0\n", lrs,
                    block_bytes, static_cast<unsigned>(context->texture_shift));
            }
            ++context->texture_count;
        }
    }
    if (ripple_log_file() != nullptr || geomdump.file != nullptr) {
        const RT64::RDP& rdp = *state->rdp;
        const unsigned tile_index = ((*dl)->w1 >> 24U) & 7U;
        const std::uint32_t texels = static_cast<std::uint32_t>(lrs) + 1U;
        TmemSource& source = tmem_sources[rdp.tiles[tile_index].tmem & 511U];
        source.address = rdp.texture.address;
        source.bytes = (texels << rdp.texture.siz) >> 1U;
        source.fmt = rdp.texture.fmt;
        source.siz = rdp.texture.siz;
        source.width = rdp.texture.width;
        source.load = 1U;
    }
    RT64::GBI_RDP::loadBlock(state, dl);
}

void load_tile(RT64::State* state, RT64::DisplayList** dl) {
    if (ripple_log_file() != nullptr || geomdump.file != nullptr) {
        const RT64::RDP& rdp = *state->rdp;
        const std::uint32_t w0 = (*dl)->w0;
        const std::uint32_t w1 = (*dl)->w1;
        const unsigned tile_index = (w1 >> 24U) & 7U;
        const std::uint32_t uls = ((w0 >> 12U) & 0xFFFU) >> 2U;
        const std::uint32_t ult = (w0 & 0xFFFU) >> 2U;
        const std::uint32_t lrt = (w1 & 0xFFFU) >> 2U;
        const std::uint32_t row_bytes =
            (static_cast<std::uint32_t>(rdp.texture.width) << rdp.texture.siz) >> 1U;
        const std::uint32_t rows = lrt >= ult ? lrt - ult + 1U : 0U;
        TmemSource& source = tmem_sources[rdp.tiles[tile_index].tmem & 511U];
        source.address = rdp.texture.address + ult * row_bytes +
            ((uls << rdp.texture.siz) >> 1U);
        source.bytes = rows * row_bytes;
        source.fmt = rdp.texture.fmt;
        source.siz = rdp.texture.siz;
        source.width = rdp.texture.width;
        source.load = 2U;
    }
    RT64::GBI_RDP::loadTile(state, dl);
}

PresentationMatrix presentation_matrix(const hlslpp::float4x4& value) {
    PresentationMatrix result{};
    for(int r=0;r<4;++r) for(int c=0;c<4;++c) result[r*4+c]=value[r][c];
    return result;
}
hlslpp::float4x4 presentation_matrix(const PresentationMatrix& m) {
    return hlslpp::float4x4(m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],
            m[8],m[9],m[10],m[11],m[12],m[13],m[14],m[15]);
}
SemanticVertexState semantic_vertex(const Context& context,std::uint32_t source,
                                   const std::array<float,3>& local,
                                   const PresentationRigidIdentity* rigid,
                                   const PresentationBeamIdentity* beam,
                                   const PresentationModelIdentity* bridge) {
    SemanticVertexState result;
    result.source = source;
    if(!context.identities || context.billboard) return result;
    const auto slot=context.selected_matrix;
    if (beam && rigid->camera && rigid_pose_matches_matrix(rigid->pose,
            presentation_matrix(context.matrices[slot]))) {
        result.sample.key={beam->lifetime,
            (std::uint64_t(beam->actor)<<32U)|beam->control,
            (std::uint64_t(beam->header)<<16U)|beam->flags,rigid->camera,
            0x4245414dU,(source-beam->vertices)/10U};
        result.world=rigid->pose.world;
        result.sample.camera=presentation_matrix(hlslpp::mul(
            presentation_matrix(rigid->pose.view),presentation_matrix(rigid->pose.projection)));
        const auto world=hlslpp::mul(hlslpp::float4(local[0],local[1],local[2],1.0f),
            presentation_matrix(result.world));
        result.sample.world={world.x,world.y,world.z};
        result.sample.valid=std::isfinite(float(world.w)) &&
            std::abs(float(world.w)-1.0f)<0.001f;
        return result;
    }
    if (bridge && rigid->camera && rigid_pose_matches_matrix(rigid->pose,
            presentation_matrix(context.matrices[slot]))) {
        result.sample.key={bridge->lifetime,
            (std::uint64_t(bridge->actor)<<32U)|bridge->instance,
            bridge->model,rigid->camera,0x42524944U,(source-bridge->vertices)/10U};
        result.world=rigid->pose.world;
        result.sample.camera=presentation_matrix(hlslpp::mul(
            presentation_matrix(rigid->pose.view),presentation_matrix(rigid->pose.projection)));
        const auto world=hlslpp::mul(hlslpp::float4(local[0],local[1],local[2],1.0f),
            presentation_matrix(result.world));
        result.sample.world={world.x,world.y,world.z};
        result.sample.valid=std::isfinite(float(world.w)) &&
            std::abs(float(world.w)-1.0f)<0.001f;
        return result;
    }
    if(context.camera_ids[slot]==0U) return result;
    const auto identity = presentation_vertex_identity(
        *context.identities, context.matrix_physical_addresses[slot], source);
    if (!identity.owner)
        return result;
    const auto &owner = *identity.owner;
    const auto &geometry = identity.geometry ? *identity.geometry : owner;
    result.sample.key = {
        owner.lifetime, (std::uint64_t(owner.actor) << 32U) | geometry.instance,
        geometry.model, context.camera_ids[slot],
        identity.joint, identity.vertex};
    result.world = presentation_matrix(context.model_matrices[slot]);
    result.sample.camera = presentation_matrix(context.camera_matrices[slot]);
    if (!identity.geometry)
        return result;
    const auto world =
        hlslpp::mul(hlslpp::float4(local[0], local[1], local[2], 1.0f),
                    context.model_matrices[slot]);
    result.sample.world = {world.x, world.y, world.z};
    result.sample.valid = context.model_finite[slot] &&
                          std::isfinite(float(world.w)) &&
                          std::abs(float(world.w) - 1.0f) < 0.001f;
    return result;
}

void dma_vertex(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    ++context->commands;
    ++context->stats.vertex_batches;
    const RT64::DisplayList command = **dl;
    const std::size_t count = (command.w0 >> 19U) & 0x1FU;
    const std::size_t requested_start = (command.w0 >> 9U) & 0x1FU;
    const bool append = (command.w0 & 0x0001'0000U) != 0U;
    if (append) {
        if (context->billboard) {
            context->vertex_cursor = 1U;
        }
    }
    else {
        context->vertex_cursor = 0U;
    }
    if (requested_start > kVertexCount - context->vertex_cursor) {
        reject(state, dl);
        return;
    }
    const std::size_t first = context->vertex_cursor + requested_start;
    const std::uint32_t segmented = state->rsp->fromSegmented(command.w1);
    if (count == 0U || first > kVertexCount || count > kVertexCount - first) {
        reject(state, dl);
        return;
    }
    const std::uint32_t address = detail::resolve_f3ddkr_vertex_dma_source(
        segmented, context->vertex_offset, command.w0);
    const std::size_t source_bytes = count * 10U;
    if (!physical_range(address, source_bytes)) {
        reject(state, dl);
        return;
    }
    ripple_vertex_source = address;

    hlslpp::float4x4 unanchored_matrix{};
    bool anchored_billboard = false;
    std::array<std::uint64_t, 4> billboard_key{};
    const auto rigid_key = context->interpolate && context->identities &&
            context->semantic_target && !context->billboard
        ? presentation_rigid_key(*context->identities,
            context->matrix_physical_addresses[context->selected_matrix], address,
            static_cast<std::uint32_t>(count))
        : std::array<std::uint64_t, 4>{};
    const auto* rigid_owner = context->interpolate && context->identities &&
        (rigid_key[0] || !context->identities->beams.empty())
        ? presentation_rigid_owner(*context->identities,
            context->matrix_physical_addresses[context->selected_matrix]) : nullptr;
    const auto* beam = rigid_owner ? presentation_beam_vertex(
        *context->identities,*rigid_owner,address) : nullptr;
    if (beam && count > (80U-(address-beam->vertices))/10U) beam=nullptr;
    static const bool bridge_vertices_enabled=[] {
#ifdef _MSC_VER
        char* value=nullptr;std::size_t length=0U;
        if(_dupenv_s(&value,&length,"JFG_DIAGNOSTIC_BRIDGE_VERTICES")!=0)return false;
        const bool enabled=!value || (length==2U && value[0]=='1');
        std::free(value);return enabled;
#else
        const char* value=std::getenv("JFG_DIAGNOSTIC_BRIDGE_VERTICES");
        return !value || std::strcmp(value,"1")==0;
#endif
    }();
    const auto* bridge=bridge_vertices_enabled && rigid_owner
        ? presentation_bridge_vertices(*context->identities,*rigid_owner,address,
            static_cast<std::uint32_t>(count)) : nullptr;

    if (context->billboard) {
        const int workload_cursor = state->ext.workloadQueue->writeCursor;
        RT64::Workload& workload =
            state->ext.workloadQueue->workloads[workload_cursor];
        const std::uint32_t anchor_index = state->rsp->indices[0U];
        if (anchor_index >= workload.drawData.posTransformed.size()) {
            reject(state, dl);
            return;
        }
        if (context->identities &&
            anchor_index < context->semantic_vertices.size()) {
            const auto source = context->semantic_vertices[anchor_index].source;
            for (const auto &sprite : context->identities->sprites)
                if (sprite.anchor == source) {
                    billboard_key = {sprite.lifetime, sprite.actor,
                                     sprite.sprite, context->camera_ids[0]};
                    break;
                }
        }
        unanchored_matrix = context->matrices[context->selected_matrix];
        context->matrices[context->selected_matrix][3] +=
            workload.drawData.posTransformed[anchor_index];
        anchored_billboard = true;
    }
    select_matrix(*context);
    std::array<std::byte, sizeof(RT64::RSP::Vertex) * 32U> saved{};
    auto* scratch = state->fromRDRAM(kScratchAddress);
    const std::size_t scratch_bytes = count * sizeof(RT64::RSP::Vertex);
    for (std::size_t index = 0U; index < scratch_bytes; ++index) {
        saved[index] = static_cast<std::byte>(scratch[index]);
    }
    auto* vertices = reinterpret_cast<RT64::RSP::Vertex*>(scratch);
    for (std::size_t index = 0U; index < count; ++index) {
        const std::uint32_t source =
            address + static_cast<std::uint32_t>(index * 10U);
        RT64::RSP::Vertex& vertex = vertices[index];
        vertex.x = guest_s16(*context, *state, source);
        vertex.y = guest_s16(*context, *state, source + 2U);
        vertex.z = guest_s16(*context, *state, source + 4U);
        vertex.flag = 0U;
        vertex.s = 0;
        vertex.t = 0;
        vertex.color.r = guest_byte(*context, *state, source + 6U);
        vertex.color.g = guest_byte(*context, *state, source + 7U);
        vertex.color.b = guest_byte(*context, *state, source + 8U);
        vertex.color.a = guest_byte(*context, *state, source + 9U);
    }
    state->rsp->setVertex(
        kScratchAddress,
        static_cast<std::uint32_t>(count),
        static_cast<std::uint32_t>(first));
    {
        const int workload_cursor = state->ext.workloadQueue->writeCursor;
        RT64::Workload& workload =
            state->ext.workloadQueue->workloads[workload_cursor];
        for (std::size_t index = first; index < first + count; ++index) {
            const std::uint32_t global = state->rsp->indices[index];
            if (global >= workload.drawData.posTransformed.size()) {
                continue;
            }
            if(context->interpolate && context->identities && context->semantic_target) {
                context->semantic_vertices.resize(workload.drawData.worldIndices.size());
                const auto& positions=workload.drawData.posFloats;
                context->semantic_vertices[global]=semantic_vertex(*context,
                    address+static_cast<std::uint32_t>((index-first)*10U),
                    {positions[global*3U],positions[global*3U+1U],positions[global*3U+2U]},rigid_owner,beam,bridge);
                context->semantic_vertices[global].billboard_key =
                    billboard_key;
                context->semantic_vertices[global].rigid_key = rigid_key;
                context->semantic_vertices[global].rigid_pose = rigid_owner ? &rigid_owner->pose : nullptr;
            }
            const hlslpp::float4 position =
                workload.drawData.posTransformed[global];
            if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
                !std::isfinite(position.z) || !std::isfinite(position.w)) {
                continue;
            }
            ++context->stats.vertices_finite;
            const float limit = std::abs(position.w) * 2.0F;
            if (position.w != 0.0F && std::abs(position.x) <= limit &&
                std::abs(position.y) <= limit &&
                std::abs(position.z) <= limit) {
                ++context->stats.vertices_in_clip;
            }
        }
    }
    for (std::size_t index = 0U; index < scratch_bytes; ++index) {
        scratch[index] = std::to_integer<std::uint8_t>(saved[index]);
    }
    if (anchored_billboard) {
        context->matrices[context->selected_matrix] = unanchored_matrix;
        select_matrix(*context);
    }
    for (std::size_t index = first; index < first + count; ++index) {
        context->loaded_vertices.set(index);
    }
    context->stats.vertices_loaded += count;
    context->vertex_cursor += count;
}

[[nodiscard]] bool set_vertex_texture_coordinates(
    Context& context, RT64::State& state,
    RT64::RSP& rsp,
    const std::uint8_t vertex,
    const std::uint32_t coordinate) {
    const int workload_cursor = state.ext.workloadQueue->writeCursor;
    RT64::Workload& workload =
        state.ext.workloadQueue->workloads[workload_cursor];
    auto& draw = workload.drawData;
    std::uint32_t global = rsp.indices[vertex];
    const auto has_components = [global](const std::size_t size,
                                         const std::size_t stride) {
        return stride != 0U && global <= (SIZE_MAX - (stride - 1U)) / stride &&
            static_cast<std::size_t>(global) * stride + (stride - 1U) < size;
    };
    if (!has_components(draw.posFloats.size(), 3U) ||
        !has_components(draw.velFloats.size(), 3U) ||
        !has_components(draw.normColBytes.size(), 4U) ||
        !has_components(draw.tcFloats.size(), 2U) ||
        !has_components(draw.tcVelFloats.size(), 2U) ||
        global >= draw.viewProjIndices.size() ||
        global >= draw.worldIndices.size() || global >= draw.fogIndices.size() ||
        global >= draw.lightIndices.size() || global >= draw.lightCounts.size() ||
        global >= draw.lookAtIndices.size() ||
        global >= draw.posTransformed.size() || global >= draw.posScreen.size()) {
        return false;
    }

    // RT64's pinned modifyVertex implementation appends directly from its
    // own vectors. A growth reallocation can invalidate the source reference,
    // corrupting positions when F3DDKR clones a shared vertex for per-triangle
    // texture coordinates. Clone through local values so this adapter remains
    // correct without modifying the pinned dependency checkout.
    if (rsp.used[vertex]) {
        const std::uint32_t replacement = draw.vertexCount();
        const float px = draw.posFloats[global * 3U];
        const float py = draw.posFloats[global * 3U + 1U];
        const float pz = draw.posFloats[global * 3U + 2U];
        const float vx = draw.velFloats[global * 3U];
        const float vy = draw.velFloats[global * 3U + 1U];
        const float vz = draw.velFloats[global * 3U + 2U];
        const std::uint8_t red = draw.normColBytes[global * 4U];
        const std::uint8_t green = draw.normColBytes[global * 4U + 1U];
        const std::uint8_t blue = draw.normColBytes[global * 4U + 2U];
        const std::uint8_t alpha = draw.normColBytes[global * 4U + 3U];
        const float texture_s = draw.tcFloats[global * 2U];
        const float texture_t = draw.tcFloats[global * 2U + 1U];
        const float velocity_s = draw.tcVelFloats[global * 2U];
        const float velocity_t = draw.tcVelFloats[global * 2U + 1U];
        const auto view_projection = draw.viewProjIndices[global];
        const auto world = draw.worldIndices[global];
        const auto fog = draw.fogIndices[global];
        const auto light = draw.lightIndices[global];
        const auto light_count = draw.lightCounts[global];
        const auto look_at = draw.lookAtIndices[global];
        const auto transformed = draw.posTransformed[global];
        const auto screen = draw.posScreen[global];

        draw.posFloats.insert(draw.posFloats.end(), {px, py, pz});
        draw.velFloats.insert(draw.velFloats.end(), {vx, vy, vz});
        draw.normColBytes.insert(
            draw.normColBytes.end(), {red, green, blue, alpha});
        draw.tcFloats.insert(draw.tcFloats.end(), {texture_s, texture_t});
        draw.tcVelFloats.insert(
            draw.tcVelFloats.end(), {velocity_s, velocity_t});
        draw.viewProjIndices.emplace_back(view_projection);
        draw.worldIndices.emplace_back(world);
        draw.fogIndices.emplace_back(fog);
        draw.lightIndices.emplace_back(light);
        draw.lightCounts.emplace_back(light_count);
        draw.lookAtIndices.emplace_back(look_at);
        draw.posTransformed.emplace_back(transformed);
        draw.posScreen.emplace_back(screen);
        if(context.interpolate && context.identities && context.semantic_target) {
            const auto semantic=global<context.semantic_vertices.size() ? context.semantic_vertices[global] : SemanticVertexState{};
            context.semantic_vertices.resize(draw.worldIndices.size());
            context.semantic_vertices[replacement]=semantic;
        }
        rsp.indices[vertex] = replacement;
        rsp.used[vertex] = false;
        global = replacement;
    }

    draw.tcFloats[global * 2U] =
        static_cast<float>(static_cast<std::int16_t>(coordinate >> 16U)) /
        32.0F;
    draw.tcFloats[global * 2U + 1U] =
        static_cast<float>(static_cast<std::int16_t>(coordinate)) / 32.0F;
    draw.lookAtIndices[global] = 0U;
    return true;
}

[[nodiscard]] std::uint8_t vertex_clip_code(
    const hlslpp::float4& position) noexcept {
    std::uint8_t clip = 0U;
    if (position.x > position.w)
        clip |= 0x01U;
    if (position.x < -position.w)
        clip |= 0x02U;
    if (position.y > position.w)
        clip |= 0x04U;
    if (position.y < -position.w)
        clip |= 0x08U;
    if (position.w < 0.01F)
        clip |= 0x10U;
    return clip;
}

void capture_loaded_vertices(Context& context) noexcept {
    context.stats.loaded_vertices_low = 0U;
    context.stats.loaded_vertices_high = 0U;
    for (std::size_t index = 0U; index < 64U; ++index) {
        if (context.loaded_vertices.test(index))
            context.stats.loaded_vertices_low |= std::uint64_t{1U} << index;
    }
    for (std::size_t index = 64U;
         index < context.loaded_vertices.size(); ++index) {
        if (context.loaded_vertices.test(index))
            context.stats.loaded_vertices_high |=
                static_cast<std::uint16_t>(1U << (index - 64U));
    }
}

void geomdump_triangle(RT64::State& state, const std::uint8_t flags,
    const std::array<std::uint32_t, 3U>& globals, const bool skipped,
    const std::uint8_t clip) {
    if (geomdump.file == nullptr)
        return;
    const GeomDumpCounters counters = geomdump_counters(state);
    const RT64::RDP& rdp = *state.rdp;
    const auto& cc = rdp.colorCombinerStack[rdp.colorCombinerStackSize - 1];
    const std::uint32_t geometry =
        state.rsp->geometryModeStack[state.rsp->geometryModeStackSize - 1];
    const TmemSource& source = tmem_sources[rdp.tiles[0].tmem & 511U];
    std::fprintf(geomdump.file,
        "X\t%llu\t%u\t%u\t%u\t%02x\t%x\t%u\t%02x\t%08x\t%08x\t%08x\t%08x\t%08x\t%06x\t%u/%u/%u",
        static_cast<unsigned long long>(geomdump.seq++), counters.fbpair,
        counters.proj, counters.calls, static_cast<unsigned>(flags),
        static_cast<unsigned>(geometry & state.rsp->cullBothMask),
        skipped ? 1U : 0U, static_cast<unsigned>(clip),
        static_cast<unsigned>(rdp.otherMode.L), static_cast<unsigned>(rdp.otherMode.H),
        static_cast<unsigned>(geometry), static_cast<unsigned>(cc.L),
        static_cast<unsigned>(cc.H), static_cast<unsigned>(source.address),
        source.fmt, source.siz, source.width);
    const RT64::Workload& workload =
        state.ext.workloadQueue->workloads[state.ext.workloadQueue->writeCursor];
    for (const std::uint32_t global : globals) {
        if (global >= workload.drawData.posTransformed.size() ||
            global >= workload.drawData.posScreen.size()) {
            std::fputs("\t?", geomdump.file);
            continue;
        }
        const hlslpp::float4 p = workload.drawData.posTransformed[global];
        const hlslpp::float3 q = workload.drawData.posScreen[global];
        std::fprintf(geomdump.file, "\t%.3f %.3f %.3f %.3f %.3f %.3f %.5f",
            static_cast<double>(float(p.x)), static_cast<double>(float(p.y)),
            static_cast<double>(float(p.z)), static_cast<double>(float(p.w)),
            static_cast<double>(float(q.x)), static_cast<double>(float(q.y)),
            static_cast<double>(float(q.z)));
    }
    std::fputc('\n', geomdump.file);
}

// Title hallway "outside" layer fade (a port choice, not console behaviour).
// The title hallway draws an outside layer early in the frame: two textured
// strips, 2-cycle mip lerp, no Z, opaque. It is seen through the translucent
// glass walls. The first strip curves away from the camera, so its far part
// is squeezed into a narrow bright slab between Juno and Vela whose left
// side (the strip's fold and far end) is a straight edge cutting into the
// building. Here the layer is drawn with blending and its vertex alpha
// falls to zero towards each strip's far end (largest w), so the slab melts
// into the panorama drawn before it instead of ending on an edge.
// JFG_TITLE_FADE: 0 off, 1 title and menus (default), 2 wherever the
// layer's exact render state appears. JFG_TITLE_FADE_PERCENT: fade length
// as a percentage of each strip's depth range (default 75, 1-100). On the
// title's first strip, 75 leaves the near half almost opaque and the
// straight edge at about a quarter of its opacity.
constexpr std::uint32_t kTitleLayerOtherModeL = 0x0C192048U;
constexpr std::uint32_t kTitleLayerOtherModeH = 0x00192CCFU;
constexpr std::uint32_t kTitleLayerCombineW0 = 0xFC26A004U;
constexpr std::uint32_t kTitleLayerCombineW1 = 0x1FFC93F8U;
constexpr std::uint32_t kFrontModeAddress = 0x000A51B0U;
constexpr std::uint8_t kFrontModeTitle = 0x03U;
constexpr std::uint8_t kFrontModeMenus = 0x18U;

[[nodiscard]] unsigned title_fade_mode() noexcept {
    static const unsigned mode = static_cast<unsigned>(
        std::clamp<long long>(texlog_env("JFG_TITLE_FADE", 1), 0, 2));
    return mode;
}

[[nodiscard]] float title_fade_fraction() noexcept {
    static const float fraction = static_cast<float>(
        std::clamp<long long>(texlog_env("JFG_TITLE_FADE_PERCENT", 75), 1, 100)) / 100.0F;
    return fraction;
}

[[nodiscard]] bool title_fade_task(const Context& context, const RT64::State& state) noexcept {
    switch (title_fade_mode()) {
    case 0U:
        return false;
    case 2U:
        return true;
    default: {
        const std::uint8_t front = guest_byte(context, state, kFrontModeAddress);
        return front == kFrontModeTitle || front == kFrontModeMenus;
    }
    }
}

// Switches the RDP state of a title-layer batch to a blended variant and
// restores the game's state when the batch ends, on every return path.
// RT64 captures a draw call's state at its first triangle, so the change
// covers exactly this batch; only the RDP copies are touched, so the RSP's
// other-mode stack (the base for partial G_SETOTHERMODE updates) keeps the
// game's values.
class TitleLayerBlend {
public:
    TitleLayerBlend(RT64::State& state, const bool enabled) noexcept {
        if (!enabled)
            return;
        RT64::RDP& rdp = *state.rdp;
        const auto& cc = rdp.colorCombinerStack[rdp.colorCombinerStackSize - 1];
        if (rdp.otherMode.L != kTitleLayerOtherModeL ||
            rdp.otherMode.H != kTitleLayerOtherModeH ||
            cc.L != kTitleLayerCombineW0 || cc.H != kTitleLayerCombineW1)
            return;
        state_ = &state;
        mode_h_ = rdp.otherMode.H;
        mode_l_ = rdp.otherMode.L;
        combine_w0_ = cc.L;
        combine_w1_ = cc.H;
        group_ = rdp.texture.address;
        // Cycle-2 alpha (1 - 0) * SHADE + 0: the vertex alpha alone, so the
        // texels' own 1-bit alpha stays without effect, as before.
        const std::uint32_t alpha_fields = (7U << 21U) | (7U << 18U) | (7U << 3U) | 7U;
        const std::uint32_t combine_w1 =
            (combine_w1_ & ~alpha_fields) | (6U << 21U) | (4U << 18U) | (7U << 3U) | 7U;
        rdp.setCombine((static_cast<std::uint64_t>(combine_w1) << 32U) | combine_w0_);
        // FORCE_BL, alpha from the combiner instead of coverage, and the
        // cycle-2 blender PIXEL * A + MEM * (1 - A).
        rdp.setOtherMode(mode_h_, (mode_l_ & ~(0x2000U | (3U << 16U))) | 0x4000U);
    }
    ~TitleLayerBlend() {
        if (state_ == nullptr)
            return;
        state_->rdp->setCombine(
            (static_cast<std::uint64_t>(combine_w1_) << 32U) | combine_w0_);
        state_->rdp->setOtherMode(mode_h_, mode_l_);
    }
    TitleLayerBlend(const TitleLayerBlend&) = delete;
    TitleLayerBlend& operator=(const TitleLayerBlend&) = delete;
    [[nodiscard]] bool active() const noexcept { return state_ != nullptr; }
    [[nodiscard]] std::uint32_t group() const noexcept { return group_; }

private:
    RT64::State* state_ = nullptr;
    std::uint32_t mode_h_ = 0U, mode_l_ = 0U;
    std::uint32_t combine_w0_ = 0U, combine_w1_ = 0U;
    std::uint32_t group_ = 0U;
};

// Runs before full sync, while the workload's vertex data can still change.
void apply_title_fade(Context& context, RT64::State& state) {
    auto& layer = context.title_fade_layer;
    auto& other = context.title_fade_other;
    if (!layer.empty()) {
        auto& draw = state.ext.workloadQueue->workloads[
            state.ext.workloadQueue->writeCursor].drawData;
        std::sort(other.begin(), other.end());
        std::sort(layer.begin(), layer.end());
        layer.erase(std::unique(layer.begin(), layer.end()), layer.end());
        // A vertex also used by other geometry keeps its alpha.
        const auto fadeable = [&](const std::uint32_t global) {
            return global < draw.posTransformed.size() &&
                static_cast<std::size_t>(global) * 4U + 3U < draw.normColBytes.size() &&
                !std::binary_search(other.begin(), other.end(), global);
        };
        const float fraction = title_fade_fraction();
        for (std::size_t first = 0U; first < layer.size();) {
            std::size_t last = first;
            float w_min = INFINITY, w_max = -INFINITY;
            for (; last < layer.size() && layer[last].first == layer[first].first; ++last) {
                if (!fadeable(layer[last].second))
                    continue;
                const float w = float(draw.posTransformed[layer[last].second].w);
                w_min = std::min(w_min, w);
                w_max = std::max(w_max, w);
            }
            if (std::isfinite(w_min) && std::isfinite(w_max) && w_max > w_min) {
                const float span = std::max((w_max - w_min) * fraction, 1.0F);
                for (std::size_t index = first; index < last; ++index) {
                    const std::uint32_t global = layer[index].second;
                    if (!fadeable(global))
                        continue;
                    const float w = float(draw.posTransformed[global].w);
                    float alpha = std::clamp((w_max - w) / span, 0.0F, 1.0F);
                    alpha = alpha * alpha * (3.0F - 2.0F * alpha);
                    std::uint8_t& byte = draw.normColBytes[static_cast<std::size_t>(global) * 4U + 3U];
                    byte = static_cast<std::uint8_t>(std::lround(alpha * static_cast<float>(byte)));
                }
            }
            first = last;
        }
    }
    layer.clear();
    other.clear();
}

void dma_triangles(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    ++context->commands;
    ++context->stats.triangle_batches;
    const RT64::DisplayList command = **dl;
    const std::size_t count = ((command.w0 >> 20U) & 0x0FU) + 1U;
    const std::uint8_t texture =
        static_cast<std::uint8_t>((command.w0 >> 16U) & 0x0FU);
    const std::uint32_t address = detail::resolve_f3ddkr_direct_dma_source(
        state->rsp->fromSegmented(command.w1));
    if (!physical_range(address, count * 16U)) {
        context->stats.rejection_reason = 1U;
        context->stats.rejection_detail = address;
        reject(state, dl);
        return;
    }
    // The F3DDKR command changes only gSP.texture.on. RT64 stores the mip
    // count as level + 1, so feeding textureState.levels back through
    // setTexture() would incorrectly increment an unrelated field on every
    // DMA triangle batch.
    state->rsp->textureState.on = texture;
    const TitleLayerBlend title_layer(*state, context->title_fade);
    for (std::size_t triangle = 0U; triangle < count; ++triangle) {
        const std::uint32_t source =
            address + static_cast<std::uint32_t>(triangle * 16U);
        const std::uint8_t flags = guest_byte(*context, *state, source);
        const std::array<std::uint8_t, 3U> vertices{
            guest_byte(*context, *state, source + 1U),
            guest_byte(*context, *state, source + 2U),
            guest_byte(*context, *state, source + 3U),
        };
        for (const std::uint8_t vertex : vertices) {
            if (vertex >= context->loaded_vertices.size() ||
                !context->loaded_vertices.test(vertex)) {
                context->stats.rejection_reason = 2U;
                context->stats.rejection_detail = vertex;
                context->stats.rejection_source_address = source;
                context->stats.rejection_triangle =
                    static_cast<std::uint32_t>(triangle);
                capture_loaded_vertices(*context);
                reject(state, dl);
                return;
            }
        }
        const int workload_cursor = state->ext.workloadQueue->writeCursor;
        const RT64::Workload& workload =
            state->ext.workloadQueue->workloads[workload_cursor];
        std::uint8_t common_clip = 0x1FU;
        for (const std::uint8_t vertex : vertices) {
            const std::uint32_t global = state->rsp->indices[vertex];
            if (global >= workload.drawData.posTransformed.size()) {
                context->stats.rejection_reason = 3U;
                context->stats.rejection_detail = global;
                reject(state, dl);
                return;
            }
            common_clip &=
                vertex_clip_code(workload.drawData.posTransformed[global]);
        }
        // The current endpoint may be outside while an intermediate camera
        // still sees this triangle. Clip interpolated scene geometry on the
        // GPU at its presentation pose; keep original/offscreen behavior.
        if (common_clip != 0U &&
            !(context->interpolate && context->semantic_target)) {
            if (geomdump.file != nullptr)
                geomdump_triangle(*state, flags,
                    {state->rsp->indices[vertices[0]], state->rsp->indices[vertices[1]],
                     state->rsp->indices[vertices[2]]}, true, common_clip);
            continue;
        }
        // JFG's microcode (DMA-triangle handler IMEM 0x1754-0x1770, triangle
        // setup 0x1B18-0x1BF8): a triangle without flag 0x40 gets G_CULL_BACK.
        // If the game has set G_CULL_FRONT, that bit turns the test round, so
        // single-sided triangles lose their front faces instead. The game sets
        // it around mirrored reflection draws (overlay 137,
        // realReflectPrint). GLideN64's rule (front only when the viewport X
        // scale is negative) ignored the game's bit, so single-sided parts
        // such as faces vanished from floor reflections. A negative viewport
        // scale needs no rule here: RT64 culls by screen winding like the RDP.
        const std::uint32_t cull_mode = (flags & 0x40U) != 0U
            ? 0U
            : (context->cull_front_switch
                ? state->rsp->cullFrontMask
                : state->rsp->cullBothMask ^ state->rsp->cullFrontMask);
        state->rsp->modifyGeometryMode(
            ~state->rsp->cullBothMask, cull_mode);
        if (geomdump.file != nullptr)
            geomdump_triangle(*state, flags,
                {state->rsp->indices[vertices[0]], state->rsp->indices[vertices[1]],
                 state->rsp->indices[vertices[2]]}, false, common_clip);
        for (std::size_t corner = 0U; corner < vertices.size(); ++corner) {
            const std::uint32_t coordinate =
                (static_cast<std::uint32_t>(static_cast<std::uint16_t>(
                    guest_s16(*context, *state, source + 4U +
                        static_cast<std::uint32_t>(corner * 4U)))) << 16U) |
                static_cast<std::uint16_t>(guest_s16(
                    *context, *state, source + 6U +
                        static_cast<std::uint32_t>(corner * 4U)));
            if (!set_vertex_texture_coordinates(
                    *context, *state, *state->rsp, vertices[corner], coordinate)) {
                context->stats.rejection_reason = 4U;
                context->stats.rejection_detail = vertices[corner];
                reject(state, dl);
                return;
            }
        }
        if (((state->rdp->otherMode.L & 0x0C00U) == 0x0800U) &&
            ((state->rdp->otherMode.L & 0x4000U) != 0U) && ripple_log_file() != nullptr) {
            const RT64::Workload& logged =
                state->ext.workloadQueue->workloads[workload_cursor];
            const RT64::RDP& rdp = *state->rdp;
            const auto& cc = rdp.colorCombinerStack[rdp.colorCombinerStackSize - 1];
            const auto& pc = rdp.primColorStack[rdp.primColorStackSize - 1];
            const auto& ec = rdp.envColorStack[rdp.envColorStackSize - 1];
            const auto& tile = rdp.tiles[0];
            const TmemSource& texsrc = tmem_sources[tile.tmem & 511U];
            const std::uint32_t geometry = state->rsp->geometryModeStack[state->rsp->geometryModeStackSize - 1];
            if (texsrc.bytes != 0U && texsrc.bytes <= 4096U &&
                physical_range(texsrc.address, texsrc.bytes) &&
                ripple_dumped.size() < 600U &&
                ripple_dumped.insert(texsrc.address).second) {
                if (std::FILE* log = ripple_log_file(); log != nullptr) {
                    std::fprintf(log, "TEX\t%06x\t%u/%u/%u\t%u\t", texsrc.address,
                        texsrc.fmt, texsrc.siz, texsrc.width, texsrc.bytes);
                    for (std::uint32_t index = 0U; index < texsrc.bytes; ++index)
                        std::fprintf(log, "%02x", static_cast<unsigned>(
                            guest_byte(*context, *state, texsrc.address + index)));
                    std::fputc('\n', log);
                    ++ripple_log.lines;
                }
            }
            // Bytes of the texture source where RT64's RDRAM copy differs
            // from the guest's (stale framebuffer data, for example).
            unsigned rt64_diff = 0U;
            if (texsrc.bytes != 0U && texsrc.bytes <= 4096U &&
                physical_range(texsrc.address, texsrc.bytes) &&
                context->simulation_rdram.size() == kRdramBytes) {
                for (std::uint32_t index = 0U; index < texsrc.bytes; ++index) {
                    const std::uint32_t byte_address = (texsrc.address + index) ^ 3U;
                    if (state->RDRAM[byte_address] != context->simulation_rdram[byte_address])
                        ++rt64_diff;
                }
            }
            // An RT64 framebuffer overlapping the texture source makes RT64
            // sample its render target instead of RDRAM.
            std::uint32_t overlap_address = 0U;
            unsigned overlap_width = 0U, overlap_type = 0U;
            if (texsrc.bytes != 0U) {
                for (const auto& [fb_key, framebuffer] : state->framebufferManager.framebuffers) {
                    (void)fb_key;
                    if (framebuffer.overlaps(texsrc.address, texsrc.address + texsrc.bytes)) {
                        overlap_address = framebuffer.addressStart;
                        overlap_width = framebuffer.width;
                        overlap_type = static_cast<unsigned>(framebuffer.lastWriteType);
                        break;
                    }
                }
            }
            char key[640];
            std::snprintf(key, sizeof(key),
                "%llu\t%llu\t%08x\t%08x\t%08x\t%08x\t%08x\t%06x\t%06x\t%06x\t%u/%u/%u\t%u\t%u/%u/%06x/%u/%u\t%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\t%.3f,%.3f,%.3f,%.3f\t%.3f,%.3f,%.3f,%.3f",
                static_cast<unsigned long long>(texlog.task),
                static_cast<unsigned long long>(state->workloadId), rdp.otherMode.L, rdp.otherMode.H,
                cc.L, cc.H, geometry, address & 0x00FF'FFFFU, ripple_vertex_source & 0x00FF'FFFFU,
                texsrc.address & 0x00FF'FFFFU, texsrc.fmt, texsrc.siz, texsrc.width, texsrc.bytes, texsrc.load, rt64_diff,
                overlap_address & 0x00FF'FFFFU, overlap_width, overlap_type,
                unsigned(tile.uls), unsigned(tile.ult), unsigned(tile.lrs), unsigned(tile.lrt),
                unsigned(tile.cms), unsigned(tile.cmt), unsigned(tile.masks), unsigned(tile.maskt),
                unsigned(tile.shifts), unsigned(tile.shiftt), unsigned(tile.fmt), unsigned(tile.siz),
                unsigned(tile.line), unsigned(tile.tmem),
                static_cast<double>(float(pc.x)), static_cast<double>(float(pc.y)),
                static_cast<double>(float(pc.z)), static_cast<double>(float(pc.w)),
                static_cast<double>(float(ec.x)), static_cast<double>(float(ec.y)),
                static_cast<double>(float(ec.z)), static_cast<double>(float(ec.w)));
            if (!ripple_group.active || ripple_group.key != key) {
                ripple_group_flush();
                const double ms = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - ripple_log.start).count();
                char head[720];
                std::snprintf(head, sizeof(head), "G\t%.0f\t%s", ms, key);
                ripple_group = RippleGroup{};
                ripple_group.active = true;
                ripple_group.key = key;
                ripple_group.head = head;
                ripple_group.sx0 = ripple_group.sy0 = ripple_group.ox0 = ripple_group.oy0 = ripple_group.oz0 = 1e9F;
                ripple_group.sx1 = ripple_group.sy1 = ripple_group.ox1 = ripple_group.oy1 = ripple_group.oz1 = -1e9F;
                ripple_group.u0 = ripple_group.v0 = 1 << 30;
                ripple_group.u1 = ripple_group.v1 = -(1 << 30);
                ripple_group.a0 = ripple_group.c0 = 255U;
            }
            RippleGroup& g = ripple_group;
            ++g.triangles;
            for (std::size_t corner = 0U; corner < vertices.size(); ++corner) {
                const std::uint32_t global = state->rsp->indices[vertices[corner]];
                RippleCorner sample{};
                if (global < logged.drawData.posScreen.size()) {
                    const float x = float(logged.drawData.posScreen[global].x);
                    const float y = float(logged.drawData.posScreen[global].y);
                    g.sx0 = std::min(g.sx0, x); g.sx1 = std::max(g.sx1, x);
                    g.sy0 = std::min(g.sy0, y); g.sy1 = std::max(g.sy1, y);
                    sample.x = x;
                    sample.y = y;
                }
                if ((static_cast<std::size_t>(global) * 3U + 2U) < logged.drawData.posFloats.size()) {
                    const float ox = logged.drawData.posFloats[global * 3U];
                    const float oy = logged.drawData.posFloats[global * 3U + 1U];
                    const float oz = logged.drawData.posFloats[global * 3U + 2U];
                    g.ox0 = std::min(g.ox0, ox); g.ox1 = std::max(g.ox1, ox);
                    g.oy0 = std::min(g.oy0, oy); g.oy1 = std::max(g.oy1, oy);
                    g.oz0 = std::min(g.oz0, oz); g.oz1 = std::max(g.oz1, oz);
                }
                if ((static_cast<std::size_t>(global) * 4U + 3U) < logged.drawData.normColBytes.size()) {
                    const unsigned r = logged.drawData.normColBytes[global * 4U];
                    const unsigned gr = logged.drawData.normColBytes[global * 4U + 1U];
                    const unsigned b = logged.drawData.normColBytes[global * 4U + 2U];
                    const unsigned a = logged.drawData.normColBytes[global * 4U + 3U];
                    g.a0 = std::min(g.a0, a); g.a1 = std::max(g.a1, a);
                    g.c0 = std::min(g.c0, r); g.c1 = std::max(g.c1, r);
                    sample.rgba = (r << 24U) | (gr << 16U) | (b << 8U) | a;
                }
                const int u = guest_s16(*context, *state, source + 4U + static_cast<std::uint32_t>(corner * 4U));
                const int v = guest_s16(*context, *state, source + 6U + static_cast<std::uint32_t>(corner * 4U));
                g.u0 = std::min(g.u0, u); g.u1 = std::max(g.u1, u);
                g.v0 = std::min(g.v0, v); g.v1 = std::max(g.v1, v);
                sample.u = u;
                sample.v = v;
                if (g.corner_count < g.corners.size())
                    g.corners[g.corner_count++] = sample;
            }
        }
        if (context->title_fade) {
            for (const std::uint8_t vertex : vertices) {
                const std::uint32_t global = state->rsp->indices[vertex];
                if (title_layer.active())
                    context->title_fade_layer.emplace_back(title_layer.group(), global);
                else
                    context->title_fade_other.push_back(global);
            }
        }
        state->rsp->drawIndexedTri(vertices[0U], vertices[1U], vertices[2U]);
        ++context->stats.triangles_drawn;
    }
    context->vertex_cursor = 0U;
}

void set_perspective_normalization(
    RT64::State* state,
    RT64::DisplayList** dl) {
    if (Context* context = find_context(state); context != nullptr) {
        ++context->commands;
    }
    state->rsp->setPerspNorm((*dl)->p1(16U, 16U));
}

// Community (2026-10-19, title-screen hallway "cutting into the building"):
// JFG's own RSP microcode treats 0xBE as a no-op. Its immediate-command table
// (DMEM 0x76 + opcode*2) sends 0xB0, 0xB5, 0xBD and 0xBE straight back to the
// command loop (IMEM 0x10CC); see diag\ucode_f3d_text.bin/ucode_f3d_data.bin.
// The console therefore never skips a display list here, and the vertex
// ranges the game passes need not bound anything. Culling on them can drop
// geometry the console draws; the title hallway draws without a depth test,
// so such a hole keeps the previous frames' pixels. That is the suspected
// cause of the straight-edged panel Luke saw (it vanishes when the RT64
// inspector re-renders the frame). JFG_CULLDL=1 restores the F3D-style cull
// below for comparison.
bool display_list_cull_enabled() noexcept {
    static const bool enabled = texlog_env("JFG_CULLDL", 0) != 0;
    return enabled;
}

void cull_display_list(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr || dl == nullptr || *dl == nullptr) {
        reject(state, dl);
        return;
    }
    ++context->commands;
    if (!display_list_cull_enabled()) {
        if (geomdump.file != nullptr) {
            // Would-be decision of the old cull, for comparison only.
            const std::size_t from = ((*dl)->w0 & 0x00FF'FFFFU) / 40U;
            const std::size_t to = (*dl)->w1 >= 40U ? ((*dl)->w1 / 40U) - 1U : from;
            std::uint8_t clip = 0x1FU;
            const RT64::Workload& workload =
                state->ext.workloadQueue->workloads[state->ext.workloadQueue->writeCursor];
            for (std::size_t vertex = (std::min)(from, to);
                 vertex <= (std::max)(from, to) && vertex < context->loaded_vertices.size(); ++vertex) {
                const std::uint32_t global = state->rsp->indices[vertex];
                if (!context->loaded_vertices.test(vertex) ||
                    global >= workload.drawData.posTransformed.size()) {
                    clip = 0xFFU;
                    break;
                }
                clip &= vertex_clip_code(workload.drawData.posTransformed[global]);
            }
            std::fprintf(geomdump.file, "B\t%zu\t%zu\t%02x\n", from, to,
                static_cast<unsigned>(clip));
        }
        return;
    }
    std::size_t first = ((*dl)->w0 & 0x00FF'FFFFU) / 40U;
    if ((*dl)->w1 < 40U) {
        reject(state, dl);
        return;
    }
    std::size_t last = ((*dl)->w1 / 40U) - 1U;
    if (last < first)
        std::swap(first, last);
    if (last >= context->loaded_vertices.size()) {
        reject(state, dl);
        return;
    }

    const int workload_cursor = state->ext.workloadQueue->writeCursor;
    const RT64::Workload& workload =
        state->ext.workloadQueue->workloads[workload_cursor];
    std::uint8_t common_clip = 0x1FU;
    for (std::size_t vertex = first; vertex <= last; ++vertex) {
        if (!context->loaded_vertices.test(vertex)) {
            reject(state, dl);
            return;
        }
        const std::uint32_t global = state->rsp->indices[vertex];
        if (global >= workload.drawData.posTransformed.size()) {
            reject(state, dl);
            return;
        }
        const hlslpp::float4 position =
            workload.drawData.posTransformed[global];
        common_clip &= vertex_clip_code(position);
        if (common_clip == 0U)
            return;
    }
    // As with individual triangles, the endpoint is insufficient to cull
    // an interpolated display list. Loaded-vertex validation still applies.
    if (context->interpolate && context->semantic_target)
        return;
    // A cull at the top level has nothing to return to. RT64 would report
    // that as a clean EndDL and silently terminate the whole task.
    if (state->returnAddressStack.empty()) {
        context->stats.rejection_reason = kF3ddkrRejectReturnStackUnderflow;
        reject(state, dl);
        return;
    }
    *dl = state->popReturnAddress();
}

// JFG uses full G_RDPSETOTHERMODE (0xEF) for its render modes. RT64's stock
// handler only updates the RDP copy, but RT64's RSP keeps its own other-mode
// stack and uses it to decide which framebuffer pairs write depth. With the
// stale RSP copy no draw counted as Z_UPD, so the native render-to-RAM pass
// never copied depth back and the game's lens-flare z-buffer test always saw
// open sky. Keep both copies in sync.
void set_geometry_mode(RT64::State* state, RT64::DisplayList** dl) {
    if (Context* context = find_context(state); context != nullptr &&
        ((*dl)->w1 & state->rsp->cullFrontMask) != 0U) {
        context->cull_front_switch = true;
    }
    RT64::GBI_F3D::setGeometryMode(state, dl);
}

void clear_geometry_mode(RT64::State* state, RT64::DisplayList** dl) {
    if (Context* context = find_context(state); context != nullptr &&
        ((*dl)->w1 & state->rsp->cullFrontMask) != 0U) {
        context->cull_front_switch = false;
    }
    RT64::GBI_F3D::clearGeometryMode(state, dl);
}

void set_other_mode_full(RT64::State* state, RT64::DisplayList** dl) {
    const std::uint32_t high = (*dl)->p0(0U, 24U);
    const std::uint32_t low = (*dl)->w1;
    state->rsp->setOtherMode(high, low);
}

void move_mem(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    // Stock F3D moveMem dereferences a 24-bit masked address into RT64's
    // 8 MiB snapshot without a range check. Bound it here before delegating.
    const std::uint32_t address = state->rsp->fromSegmentedMasked((*dl)->w1);
    if (!physical_range(address, kMaximumMoveMemBytes)) {
        context->stats.rejection_reason = kF3ddkrRejectMoveMemRange;
        context->stats.rejection_detail = address;
        reject(state, dl);
        return;
    }
    RT64::GBI_F3D::moveMem(state, dl);
}

void move_word(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    ++context->commands;
    const std::uint8_t subtype = static_cast<std::uint8_t>((*dl)->w0);
    if (subtype == kMoveWordBillboard) {
        if (((*dl)->w1 & ~1U) != 0U) {
            reject(state, dl);
            return;
        }
        context->billboard = ((*dl)->w1 & 1U) != 0U;
        return;
    }
    if (subtype == kMoveWordModelMatrix) {
        context->selected_matrix = ((*dl)->w1 >> 6U) & 0x03U;
        select_matrix(*context);
        return;
    }
    RT64::GBI_F3D::moveWord(state, dl);
}

void dma_offsets(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    ++context->commands;
    context->matrix_offset = (*dl)->w0 & 0x00FF'FFFFU;
    context->vertex_offset = (*dl)->w1 & 0x00FF'FFFFU;
    // The original BF handler clears the texture-offset enable byte (DMEM
    // 0x1E7). A new model's DMA bases must not retain the prior texture table.
    context->texture_offset = 0U;
    context->texture_shift = 0U;
    context->texture_count = 0U;
    context->texture_table_enabled = false;
    if (texlog_on()) {
        ++texlog.lines;
        std::fprintf(texlog.file, "D\t%x\t%x\n", context->matrix_offset,
            context->vertex_offset);
    }
}

void run_display_list(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr || context->simulation_rdram.size() != kRdramBytes) {
        reject(state, dl);
        return;
    }
    ++context->stats.display_list_branches;
    if (context->stats.display_list_branches > kMaximumDisplayListBranches) {
        context->stats.rejection_reason = kF3ddkrRejectBranchBudget;
        context->stats.rejection_detail = static_cast<std::uint32_t>(
            context->stats.display_list_branches);
        reject(state, dl);
        return;
    }
    if ((*dl)->p0(16U, 1U) == 0U) {
        if (state->returnAddressStack.size() >= kMaximumReturnStackDepth) {
            context->stats.rejection_reason = kF3ddkrRejectReturnStackDepth;
            context->stats.rejection_detail = static_cast<std::uint32_t>(
                state->returnAddressStack.size());
            reject(state, dl);
            return;
        }
        state->pushReturnAddress(*dl);
    }
    const std::uint32_t address = detail::resolve_f3ddkr_direct_dma_source(
        state->rsp->fromSegmented((*dl)->w1));
    const std::uintptr_t command_cursor =
        reinterpret_cast<std::uintptr_t>(*dl);
    const std::uintptr_t simulation_base = reinterpret_cast<std::uintptr_t>(
        context->simulation_rdram.data());
    if (command_cursor >= simulation_base &&
        command_cursor - simulation_base <=
            kRdramBytes - sizeof(RT64::DisplayList)) {
        // Remember the most recent direct display-list edge. reject() copies
        // it into the rejection fields only when no handler recorded a more
        // specific cause, so a later successful edge cannot clobber an
        // earlier triangle diagnostic.
        context->stats.last_display_list_address =
            static_cast<std::uint32_t>(command_cursor - simulation_base);
        context->stats.last_display_list_target = address;
    }
    if (!physical_range(address, sizeof(RT64::DisplayList))) {
        reject(state, dl);
        return;
    }
    *dl = reinterpret_cast<RT64::DisplayList*>(
        const_cast<std::uint8_t*>(context->simulation_rdram.data()) +
        address) - 1;
}

bool detailed_function_profile_enabled() {
    static const bool enabled=[] {
#ifdef _MSC_VER
        char* value=nullptr;std::size_t length=0U;
        if(_dupenv_s(&value,&length,"JFG_DIAGNOSTIC_FUNCTION_PROFILE")!=0)return false;
        const bool selected=value && length==2U && (value[0]=='1' || value[0]=='2');
        std::free(value);
        return selected;
#else
        const char* value=std::getenv("JFG_DIAGNOSTIC_FUNCTION_PROFILE");
        return value && (value[0]=='1' || value[0]=='2') && value[1]=='\0';
#endif
    }();
    return enabled;
}

void prepare_interpolation(Context& context, RT64::State& state) {
    if (!context.interpolate) return;
    state.flush();
    auto& workload = state.ext.workloadQueue->workloads[state.ext.workloadQueue->writeCursor];
    auto& data = workload.drawData;
    bool scene = false;
    for (std::uint32_t f = 0; f < workload.fbPairCount; ++f) {
        const auto& pair = workload.fbPairs[f];
        scene = scene || (pair.colorImage.width >= 320U &&
            pair.colorImage.siz == G_IM_SIZ_16b && !pair.drawColorRect.isEmpty());
    }
    const bool history_profile=interpolation_trace_enabled() || detailed_function_profile_enabled();
    auto history_stage_started=history_profile ? std::chrono::steady_clock::now()
        : std::chrono::steady_clock::time_point{};
    const auto history_stage=[&](unsigned stage) {
        if (!history_profile) return;
        const auto now=std::chrono::steady_clock::now();
        const auto elapsed=static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(now-history_stage_started).count());
        if(stage>=1U && stage<=context.stats.history_stage_microseconds.size())
            context.stats.history_stage_microseconds[stage-1U]+=elapsed;
        if(interpolation_trace_enabled())
            trace_interpolation("history-stage",context.vi_tick,stage,scene?1U:0U,0U,
                static_cast<unsigned>(data.worldIndices.size()),0.0f,elapsed,
                data.worldTransforms.size());
        history_stage_started=std::chrono::steady_clock::now();
    };
    context.semantic_vertices.resize(data.worldIndices.size());
    std::vector<SemanticPresentationSample> semantic_samples;
    semantic_samples.reserve(context.semantic_vertices.size());
    for (const auto& vertex : context.semantic_vertices) {
        auto sample = vertex.sample;
        if (!data.modifyPosUints.empty()) sample.valid = false;
        semantic_samples.push_back(sample);
    }
    SemanticRigidPoses semantic_rigid_poses;
    std::set<std::uint64_t> semantic_pose_conflicts;
    for(const auto& vertex:context.semantic_vertices) {
        if(!vertex.sample.valid || vertex.sample.key[4]!=0x42524944U || !vertex.rigid_pose)continue;
        const auto lifetime=vertex.sample.key[0];
        const auto [it,inserted]=semantic_rigid_poses.emplace(lifetime,*vertex.rigid_pose);
        if(!inserted && it->second!=*vertex.rigid_pose)semantic_pose_conflicts.insert(lifetime);
    }
    for(const auto lifetime:semantic_pose_conflicts)semantic_rigid_poses.erase(lifetime);
    const bool semantic_cadence = context.presentation_history.has_continuous_cadence(context.vi_tick, scene);
    std::fenv_t semantic_environment{};
    const bool semantic_parallel = scene && semantic_samples.size() >= 128 &&
        capture_floating_environment(semantic_environment);
    struct SemanticResult {
        std::vector<SemanticPresentationSample> samples;
        std::vector<SemanticPresentationPrevious> history;
        int flags = 0;
    };
    auto semantic_job = std::async(semantic_parallel ? std::launch::async : std::launch::deferred,
        [&context, values = std::move(semantic_samples), tick = context.vi_tick, scene,
         semantic_cadence, semantic_parallel, semantic_environment,
         poses=std::move(semantic_rigid_poses)]() mutable {
            const ScopedFloatingEnvironment environment(
                semantic_parallel ? &semantic_environment : nullptr);
            SemanticPresentationDiagnostics reasons;
            auto history = context.semantic_history.record(values, tick, scene, semantic_cadence,
                interpolation_trace_enabled()?&reasons:nullptr,&poses);
            for(const auto& [lifetime,counts]:reasons) {
                const auto sample=std::find_if(values.begin(),values.end(),[&](const auto& s){
                    return s.key[0]==lifetime;
                });
                const auto actor=sample==values.end()?0U:static_cast<unsigned>(sample->key[1]>>32U);
                trace_interpolation("semantic-reject",tick,actor,
                    static_cast<unsigned>(counts[0]),static_cast<unsigned>(counts[1]),
                    static_cast<unsigned>(counts[2]),0.0f,counts[3],counts[4]);
            }
            return SemanticResult{std::move(values), std::move(history), floating_exception_flags()};
        });
    std::vector<PresentationMesh> meshes(data.worldTransforms.size());
    std::vector<std::uint32_t> local_indices(data.worldIndices.size());
    std::vector<std::size_t> vertex_counts(meshes.size());
    for (std::size_t v=0;v<data.worldIndices.size();++v)
        if (data.worldIndices[v]<meshes.size() && v*3U+2U<data.posFloats.size())
            ++vertex_counts[data.worldIndices[v]];
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        // UV clones normally contribute one triangle index per position.
        // Reserve headroom once; unusual topology still grows normally.
        meshes[i].positions.reserve(vertex_counts[i]);
        meshes[i].key.reserve(vertex_counts[i]*6U+11U);
        for (std::size_t r = 0; r < 4; ++r)
            for (std::size_t c = 0; c < 4; ++c)
                meshes[i].matrix[r*4+c] = data.worldTransforms[i].m[r][c];
        meshes[i].eligible = data.modifyPosUints.empty();
    }
    for (std::size_t v = 0; v < data.worldIndices.size(); ++v) {
        const auto m = data.worldIndices[v];
        if (m >= meshes.size() || v*3+2 >= data.posFloats.size()) continue;
        auto& mesh = meshes[m];
        local_indices[v] = static_cast<std::uint32_t>(mesh.positions.size());
        std::array<float, 3> point{};
        for (std::size_t c = 0; c < 3; ++c) {
            point[c] = data.posFloats[v*3+c];
            mesh.key.push_back(std::bit_cast<std::uint32_t>(point[c]));
        }
        mesh.positions.push_back(point);
    }
    for (auto& mesh : meshes) {
        mesh.key.push_back(0xFFFFFFFFU);
        mesh.key.push_back(static_cast<std::uint32_t>(mesh.positions.size()));
    }
    std::size_t cross_matrix_triangles=0, equal_matrix_triangles=0;
    for (std::size_t t = 0; t + 2 < data.faceIndices.size(); t += 3) {
        const auto a = data.faceIndices[t], b = data.faceIndices[t+1], c = data.faceIndices[t+2];
        if (a >= data.worldIndices.size() || b >= data.worldIndices.size() ||
            c >= data.worldIndices.size()) continue;
        const auto ma = data.worldIndices[a], mb = data.worldIndices[b], mc = data.worldIndices[c];
        if (ma >= meshes.size() || mb >= meshes.size() || mc >= meshes.size()) continue;
        if (ma != mb || ma != mc) {
            ++cross_matrix_triangles;
            if(allow_shared_matrix_triangle(meshes[ma],meshes[mb],meshes[mc]))
                ++equal_matrix_triangles;
            continue;
        }
        meshes[ma].key.insert(meshes[ma].key.end(),
            {local_indices[a], local_indices[b], local_indices[c]});
    }
    history_stage(1U); // Geometry keys and local indices.
    // Identical sprite quads must not match a different pickup just because
    // their geometry is equal. Stable actor lifetime + asset + camera owns
    // the billboard transform; texture animation keeps its authored cadence.
    std::map<unsigned, std::array<std::uint64_t, 4>> billboard_owners;
    for (std::size_t v = 0;
         v < context.semantic_vertices.size() && v < data.worldIndices.size();
         ++v) {
        const auto &key = context.semantic_vertices[v].billboard_key;
        if (key[0] != 0 && key[3] != 0)
            billboard_owners[data.worldIndices[v]] = key;
    }
    for (const auto &[matrix, key] : billboard_owners)
        if (matrix < meshes.size())
            bind_presentation_billboard(meshes[matrix], key);
    const bool rigid_trace = scene && interpolation_trace_enabled();
    const bool rigid_compare=rigid_trace && interpolation_compare_enabled();
    PresentationHistoryFrame rigid_baseline;
    if (rigid_compare)
        rigid_baseline = context.rigid_baseline_history.record(meshes, context.vi_tick, scene);
    std::map<unsigned, std::array<std::uint64_t, 4>> rigid_owners;
    std::set<unsigned> rigid_conflicts;
    std::map<unsigned, const RigidPresentationPose*> rigid_poses;
    for (std::size_t v = 0; v < context.semantic_vertices.size() && v < data.worldIndices.size(); ++v) {
        const auto &key = context.semantic_vertices[v].rigid_key;
        if (!key[0] || !key[3]) continue;
        const auto matrix = data.worldIndices[v];
        const auto [it, inserted] = rigid_owners.emplace(matrix, key);
        if (!inserted && it->second != key) rigid_conflicts.insert(matrix);
        const auto* pose=context.semantic_vertices[v].rigid_pose;
        if (pose) {
            const auto [previous, fresh]=rigid_poses.emplace(matrix,pose);
            if (!fresh && *previous->second != *pose) rigid_conflicts.insert(matrix);
        }
    }
    for (const auto &[matrix, key] : rigid_owners) if (matrix < meshes.size()) {
        if (rigid_conflicts.contains(matrix)) meshes[matrix].eligible = false;
        else {
            bind_presentation_rigid(meshes[matrix], key);
            const auto pose=rigid_poses.find(matrix);
            if (pose!=rigid_poses.end() && rigid_pose_matches_matrix(*pose->second, meshes[matrix].matrix))
                meshes[matrix].rigid_pose=*pose->second;
            else if (pose!=rigid_poses.end() && rigid_compare && context.vi_tick>=3300U) {
                static unsigned captured=0;
                if(captured<512U) {
                    ++captured;
                    const std::array<float,16>* matrices[]{&pose->second->world,&pose->second->view,
                        &pose->second->projection,&meshes[matrix].matrix};
                    for(unsigned part=0;part<4;++part)for(unsigned row=0;row<4;++row) {
                        const auto& m=*matrices[part];
                        const auto pair=[&](unsigned c) { return
                            (std::uint64_t(std::bit_cast<std::uint32_t>(m[row*4+c]))<<32U)|
                            std::bit_cast<std::uint32_t>(m[row*4+c+1]); };
                        trace_interpolation("rigid-reject",context.vi_tick,static_cast<unsigned>(key[1]>>32U),
                            part,row,captured,0.0f,pair(0),pair(2));
                    }
                }
            }
        }
    }
    PresentationHistoryFrame shared_matrix_baseline;
    if(rigid_compare) {
        auto old_policy=meshes;
        for(auto& mesh:old_policy) if(!mesh.complete_local_topology) mesh.eligible=false;
        shared_matrix_baseline=context.shared_matrix_baseline_history.record(
            old_policy,context.vi_tick,scene);
    }
    PresentationHistoryFrame camera_shape_baseline;
    if(rigid_compare) camera_shape_baseline=context.camera_shape_baseline_history.record(
        meshes,context.vi_tick,scene);
    if(context.identities)for(std::size_t m=0;m<meshes.size() && m<data.worldTransformPhysicalAddresses.size();++m) {
        const PresentationCameraIdentity* owner=nullptr;
        bool ambiguous=false;
        for(const auto& camera:context.identities->cameras)
            if(camera.matrix==data.worldTransformPhysicalAddresses[m] && camera.pose_valid) {
                if(owner)ambiguous=true;
                owner=&camera;
            }
        if(owner && !ambiguous) {
            meshes[m].camera_owner=std::array<std::uint64_t,2>{owner->generation,owner->identity};
            meshes[m].camera_pose=owner->pose;
        }
    }
    auto frame = std::make_shared<PresentationHistoryFrame>(
        context.presentation_history.record(meshes, context.vi_tick, scene, rigid_compare));
    if(rigid_compare) {
        std::size_t gained=0,lost=0,changed=0,owned=0;
        for(std::size_t m=0;m<meshes.size();++m) {
            const auto& old=camera_shape_baseline.transforms[m];
            const auto& current=frame->transforms[m];
            owned+=meshes[m].camera_owner.has_value();
            if(current.matched && !old.matched)gained+=meshes[m].positions.size();
            if(old.matched && !current.matched)lost+=meshes[m].positions.size();
            if(old.matched && current.matched && old.previous!=current.previous)++changed;
        }
        trace_interpolation("camera-identity",context.vi_tick,frame->source_rate,
            static_cast<unsigned>(changed),static_cast<unsigned>(owned),
            static_cast<unsigned>(data.worldIndices.size()),0.0f,gained,lost);
        const auto& camera_debug=frame->debug_cameras;
        trace_interpolation("camera-history",context.vi_tick,frame->source_rate,
            static_cast<unsigned>(camera_debug[0]),static_cast<unsigned>(camera_debug[1]),
            static_cast<unsigned>(camera_debug[2]),frame->debug_projection_delta,
            camera_debug[3],camera_debug[4]);
        for(unsigned owner=0;owner<frame->debug_camera_rejections.size();++owner)
            for(unsigned endpoint=0;endpoint<2;++endpoint)for(unsigned part=0;part<2;++part)
                for(unsigned row=0;row<4;++row) {
                    const auto& pose=frame->debug_camera_rejections[owner][endpoint];
                    const auto& matrix=part?pose.projection:pose.view;
                    const auto pair=[&](unsigned c) {return
                        (std::uint64_t(std::bit_cast<std::uint32_t>(matrix[row*4+c]))<<32U)|
                        std::bit_cast<std::uint32_t>(matrix[row*4+c+1]);};
                    trace_interpolation("camera-operand",context.vi_tick,owner,
                        endpoint,part,row,0.0f,pair(0),pair(2));
                }
    }
    if(rigid_compare) {
        std::size_t gained=0,lost=0,changed=0;
        for(std::size_t m=0;m<meshes.size();++m) {
            const auto& old=shared_matrix_baseline.transforms[m];
            const auto& current=frame->transforms[m];
            if(current.matched && !old.matched)gained+=meshes[m].positions.size();
            if(old.matched && !current.matched)lost+=meshes[m].positions.size();
            if(old.matched && current.matched && old.previous!=current.previous)++changed;
        }
        trace_interpolation("matrix-sharing",context.vi_tick,frame->source_rate,
            static_cast<unsigned>(changed),0U,static_cast<unsigned>(data.worldIndices.size()),
            0.0f,gained,lost);
    }
    history_stage(2U); // Matrix ownership and rigid/shape history.
    if (rigid_trace) {
        std::size_t valid=0;
        for (const auto& mesh:meshes) valid+=mesh.rigid_pose.has_value();
        trace_interpolation("rigid-pose", context.vi_tick, frame->source_rate, 0U, 0U,
            static_cast<unsigned>(rigid_owners.size()), 0.0f, valid, 0U);
    }
    if (rigid_compare) for (const auto &[matrix,key] : rigid_owners)
        if (matrix < frame->transforms.size())
            trace_interpolation("rigid",context.vi_tick,static_cast<unsigned>(key[1]>>32U),
                static_cast<unsigned>(key[1]),static_cast<unsigned>(key[2]),
                static_cast<unsigned>(meshes[matrix].positions.size()),0.0f,
                rigid_baseline.transforms[matrix].matched,frame->transforms[matrix].matched);
    if (rigid_trace) for (const auto &[matrix,key] : rigid_owners)
        if (matrix < frame->transforms.size())
            trace_interpolation("rigid-owned",context.vi_tick,static_cast<unsigned>(key[1]>>32U),
                static_cast<unsigned>(key[1]),static_cast<unsigned>(key[2]),
                static_cast<unsigned>(meshes[matrix].positions.size()),0.0f,
                frame->transforms[matrix].matched,0U);
    auto semantic_result = semantic_job.get();
    if (semantic_parallel) merge_floating_exceptions(semantic_result.flags);
    const auto& samples = semantic_result.samples;
    const auto& history = semantic_result.history;
    history_stage(3U); // Semantic history and ownership diagnostics.
    std::size_t semantic_count=0,matched_count=0,moving_count=0;
    if (scene && interpolation_trace_enabled()) {
        std::size_t matched = 0;
        for (const auto &[matrix, key] : billboard_owners)
            matched += matrix < frame->transforms.size() &&
                       frame->transforms[matrix].matched;
        trace_interpolation("billboard", context.vi_tick, frame->source_rate, 0,
                            0, static_cast<unsigned>(billboard_owners.size()),
                            0, matched, 0);
    }
    std::vector<hlslpp::float4x4> inverse_worlds(frame->transforms.size());
    std::vector<bool> inverse_ready(frame->transforms.size(),false);
    for(std::size_t v=0;v<samples.size();++v) {
        if(samples[v].key[0]==0U) continue;
        ++semantic_count;
        const auto m=data.worldIndices[v];
        if(m>=frame->transforms.size()) continue;
        auto& transform=frame->transforms[m];
        transform.matched=false; // Hold all parts together on an invalid sample.
        if(!history[v].matched) continue;
        const auto& vertex=context.semantic_vertices[v];
        if(!inverse_ready[m]) {
            inverse_worlds[m]=hlslpp::inverse(presentation_matrix(vertex.world));
            inverse_ready[m]=true;
        }
        const auto& inverse=inverse_worlds[m];
        const auto& p=history[v].world;
        const auto old_local=hlslpp::mul(hlslpp::float4(p[0],p[1],p[2],1.0f),inverse);
        const float old[3]={old_local.x,old_local.y,old_local.z};
        for(std::size_t c=0;c<3;++c) data.velFloats[v*3U+c]=data.posFloats[v*3U+c]-old[c];
        transform.matched=true;transform.semantic_vertices=true;
        transform.current_world=vertex.world;
        transform.previous_camera=history[v].camera;
        transform.current_camera=samples[v].camera;
        ++matched_count;
        if(samples[v].world!=history[v].world) ++moving_count;
    }
    history_stage(4U); // Previous-world endpoint conversion.
    if(scene) trace_interpolation("semantic",context.vi_tick,frame->source_rate,0U,0U,
        static_cast<unsigned>(semantic_count),0.0f,matched_count,moving_count);
    if (scene && interpolation_trace_enabled()) {
        std::map<std::uint32_t,std::array<unsigned,3>> bridges;
        for (std::size_t v=0;v<samples.size();++v) if(samples[v].key[4]==0x42524944U) {
            auto& count=bridges[static_cast<std::uint32_t>(samples[v].key[1]>>32U)];
            ++count[0];count[1]+=history[v].matched;
            count[2]+=history[v].matched && samples[v].world!=history[v].world;
        }
        for (const auto& [actor,count]:bridges)
            trace_interpolation("bridge-owned",context.vi_tick,actor,0U,0U,
                count[0],0.0f,count[1],count[2]);
        std::map<std::uint32_t,std::array<unsigned,2>> beams;
        for (std::size_t v=0;v<samples.size();++v) if(samples[v].key[4]==0x4245414dU) {
            auto& count=beams[static_cast<std::uint32_t>(samples[v].key[1]>>32U)];
            ++count[0]; count[1]+=history[v].matched;
        }
        for (const auto& [actor,count]:beams)
            trace_interpolation("beam-owned",context.vi_tick,actor,0U,0U,count[0],0.0f,count[1],0U);
    }
    if(scene && interpolation_trace_enabled()) {
        std::set<PresentationMatrix> cameras;
        for(const auto& sample:samples) if(sample.valid) cameras.insert(sample.camera);
        std::size_t camera_vertices=0, camera_matched=0, all_matched=0;
        std::size_t invalid_camera_vertices=0;
        for(std::size_t v=0;v<data.worldIndices.size();++v) {
            const auto m=data.worldIndices[v];
            if(m>=meshes.size()) continue;
            const bool matched=frame->transforms[m].matched;
            all_matched+=matched;
            if(cameras.contains(meshes[m].matrix)) {
                ++camera_vertices;camera_matched+=matched;
                invalid_camera_vertices+=!meshes[m].eligible;
            }
        }
        trace_interpolation("geometry-conflict",context.vi_tick,frame->source_rate,0U,0U,
            static_cast<unsigned>(cross_matrix_triangles),0.0f,equal_matrix_triangles,invalid_camera_vertices);
        if(rigid_compare && context.vi_tick>=3300U) {
            static unsigned captured_camera_holds=0;
            for(std::size_t m=0;m<meshes.size();++m) {
                if(!cameras.contains(meshes[m].matrix) || frame->transforms[m].matched ||
                    meshes[m].positions.empty() || captured_camera_holds>=2048U) continue;
                ++captured_camera_holds;
                trace_interpolation("camera-hold",context.vi_tick,frame->source_rate,
                    static_cast<unsigned>(m),frame->debug_reasons[m],
                    static_cast<unsigned>(meshes[m].positions.size()),0.0f,
                    meshes[m].key.size(),captured_camera_holds);
                for(unsigned part=0;part<2;++part) for(unsigned row=0;row<4;++row) {
                    const auto& matrix=part?meshes[m].matrix:frame->debug_previous[m];
                    const auto pair=[&](unsigned c) {return
                        (std::uint64_t(std::bit_cast<std::uint32_t>(matrix[row*4+c]))<<32U)|
                        std::bit_cast<std::uint32_t>(matrix[row*4+c+1]);};
                    trace_interpolation("camera-matrix",context.vi_tick,static_cast<unsigned>(m),
                        part,row,captured_camera_holds,0.0f,pair(0),pair(2));
                }
            }
        }
        trace_interpolation("coverage",context.vi_tick,frame->source_rate,0U,
            static_cast<unsigned>(camera_vertices),static_cast<unsigned>(data.worldIndices.size()),
            0.0f,camera_matched,all_matched);
    }
    history_stage(5U); // Coverage diagnostics.
    state.setRefreshRate(static_cast<std::uint16_t>(frame->source_rate));
    publish_presentation_snapshot(&workload, std::move(frame));
    context.semantic_vertices.clear();
}

void full_sync(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    const auto start = std::chrono::steady_clock::now();
    const bool detailed=detailed_function_profile_enabled();
    auto rdp_started=std::chrono::steady_clock::time_point{};
    if (context != nullptr) {
        const auto& workload = state->ext.workloadQueue->workloads[state->ext.workloadQueue->writeCursor];
        bool scene = false;
        for (std::uint32_t f = 0; f < workload.fbPairCount; ++f) {
            const auto& pair = workload.fbPairs[f];
            scene = scene || (pair.colorImage.width >= 320U && pair.colorImage.siz == G_IM_SIZ_16b);
        }
        if (scene) frame_diagnostic_state().scene(context->vi_tick);
        apply_title_fade(*context, *state);
        const auto history_start=std::chrono::steady_clock::now();
        prepare_interpolation(*context, *state);
        if(scene && interpolation_trace_enabled()) trace_interpolation("history-cost",context->vi_tick,
            0U,0U,0U,0U,0.0f,std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now()-history_start).count(),0U);
        if(detailed) {
            rdp_started=std::chrono::steady_clock::now();
            context->stats.history_prepare_microseconds+=static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(rdp_started-history_start).count());
        }
    }
    RT64::GBI_RDP::fullSync(state, dl);
    if (context != nullptr) {
        const auto end=std::chrono::steady_clock::now();
        context->stats.full_sync_microseconds+=static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());
        if(detailed)context->stats.rdp_sync_microseconds+=static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(end-rdp_started).count());
    }
}

void dispatch_nested(RT64::State* state, RT64::DisplayList*& command) {
    Context* context = find_context(state);
    if (context == nullptr || command == nullptr) {
        return;
    }
    const std::uint8_t opcode = static_cast<std::uint8_t>(command->w0 >> 24U);
    if ((opcode & 0xC0U) != 0xC0U) {
        // F3DDKR's counted DMA-list handler dispatches only RDP commands;
        // low opcodes inside the fixed-size block are data words, not
        // commands. Count them so a block that draws nothing is visible in
        // diagnostics without rejecting valid guest data.
        ++context->stats.nested_data_words;
        return;
    }
    const RT64::GBIFunction function = context->gbi.map[opcode];
    if (function == nullptr) {
        reject(state, &command);
        return;
    }
    function(state, &command);
}

void dma_display_list(RT64::State* state, RT64::DisplayList** dl) {
    Context* context = find_context(state);
    if (context == nullptr) {
        reject(state, dl);
        return;
    }
    ++context->commands;
    ++context->stats.dma_display_lists;
    const RT64::DisplayList outer = **dl;
    const std::size_t count = (outer.w0 >> 16U) & 0xFFU;
    const std::size_t encoded_bytes = outer.w0 & 0xFFFFU;
    const std::uint32_t address = detail::resolve_f3ddkr_direct_dma_source(
        state->rsp->fromSegmented(outer.w1));
    if (count == 0U || encoded_bytes != count * sizeof(RT64::DisplayList) ||
        !physical_range(address, encoded_bytes)) {
        reject(state, dl);
        return;
    }
    RT64::DisplayList* nested = reinterpret_cast<RT64::DisplayList*>(
        context->simulation_rdram.size() == kRdramBytes
            ? const_cast<std::uint8_t*>(context->simulation_rdram.data()) +
                address
            : state->fromRDRAM(address));
    RT64::DisplayList* const end = nested + count;
    // The microcode sends these words straight to the RDP: no texture-offset
    // table (see set_texture_image).
    context->in_dma_list = true;
    while (nested != nullptr && nested < end && !context->failed) {
        RT64::DisplayList* current = nested;
        dispatch_nested(state, nested);
        if (nested == nullptr) {
            // EndDL is valid only as the final encoded word of a counted
            // list. It must not silently truncate the block.
            if (current + 1 != end) {
                reject(state, dl);
            }
            break;
        }
        RT64::DisplayList* const next = nested + 1;
        if (next <= current || next > end) {
            // Handlers such as TextureRectangle advance over continuation
            // words themselves. Count those words, and reject control flow
            // that escapes the explicitly bounded block.
            reject(state, dl);
            break;
        }
        nested = next;
    }
    context->in_dma_list = false;
}

void reset(RT64::State* state) {
    RT64::GBI_F3D::reset(state);
    if (Context* context = find_context(state); context != nullptr) {
        context->matrix_offset = 0U;
        context->vertex_offset = 0U;
        context->texture_offset = 0U;
        context->texture_shift = 0U;
        context->texture_count = 0U;
        context->texture_table.fill(0U);
        context->texture_index = 0U;
        context->texture_table_enabled = false;
        context->in_dma_list = false;
        context->cull_front_switch = false;
        context->vertex_cursor = 0U;
        context->loaded_vertices.reset();
        context->selected_matrix = 0U;
        context->matrix_segmented_addresses.fill(0U);
        context->matrix_physical_addresses.fill(0U);
        context->loaded_matrices.reset();
        context->camera_ids.fill(0U);
        context->model_finite.fill(false);
        context->semantic_target=false;
        context->billboard = false;
    }
}

}  // namespace

struct Rt64F3ddkr::Impl {
    explicit Impl(RT64::Application& app) {
        context.application = &app;
    }
    Context context;
};

Rt64F3ddkr::Rt64F3ddkr(RT64::Application& application, const bool interpolate)
    : impl_(std::make_unique<Impl>(application)) {
    impl_->context.interpolate = interpolate;
    const std::scoped_lock lock(contexts_mutex);
    contexts.emplace(application.state.get(), &impl_->context);
}

Rt64F3ddkr::~Rt64F3ddkr() {
    if (impl_ != nullptr && impl_->context.application != nullptr &&
        impl_->context.application->state != nullptr) {
        const std::scoped_lock lock(contexts_mutex);
        contexts.erase(impl_->context.application->state.get());
        if (impl_->context.application->workloadQueue != nullptr)
            for (auto& workload : impl_->context.application->workloadQueue->workloads)
                publish_presentation_snapshot(&workload, nullptr);
    }
}

void Rt64F3ddkr::begin(
    const std::span<const std::uint8_t> simulation_rdram, const std::uint64_t vi_tick,
    std::shared_ptr<const PresentationSceneIdentity> identities) noexcept {
    impl_->context.identities=std::move(identities);
    impl_->context.semantic_vertices.clear();
    impl_->context.vi_tick = vi_tick;
    impl_->context.simulation_rdram = simulation_rdram;
    impl_->context.commands = 0U;
    impl_->context.stats = {};
    impl_->context.failed = false;
    impl_->context.title_fade_layer.clear();
    impl_->context.title_fade_other.clear();
    impl_->context.title_fade =
        title_fade_task(impl_->context, *impl_->context.application->state);
    texlog_init();
    if (!texlog.started) { texlog.started = true; texlog.start = std::chrono::steady_clock::now(); }
    texlog.now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - texlog.start).count();
    ++texlog.task;
    geomdump_begin_task(texlog.task, vi_tick);
    if (std::FILE* ripple = ripple_log_file(); ripple != nullptr) {
        ripple_group_flush();
        const double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - ripple_log.start).count();
        std::fprintf(ripple, "TASK\t%llu\t%.0f\n",
            static_cast<unsigned long long>(texlog.task), ms);
        ++ripple_log.lines;
        std::fflush(ripple);
    }
    if (texlog_on()) {
        ++texlog.lines;
        std::fprintf(texlog.file, "T\t%llu\t%lld\n",
            static_cast<unsigned long long>(texlog.task), texlog.now_ms);
        std::fflush(texlog.file);
    }
    reset(impl_->context.application->state.get());
}

void Rt64F3ddkr::process_display_list(std::uint32_t address) {
    auto& context=impl_->context;
    struct RestoreBinding {
        Context* previous;
        ~RestoreBinding() { dispatch_context=previous; }
    } restore{dispatch_context};
    dispatch_context=&context;
    context.application->processDisplayLists(
        const_cast<std::uint8_t*>(context.simulation_rdram.data()),address,0U,true);
}

void Rt64F3ddkr::install() noexcept {
    Context& context = impl_->context;
    context.gbi = {};
    context.gbi.ucode = RT64::GBIUCode::F3D;
    RT64::GBI_RDP::setup(&context.gbi, true);
    RT64::GBI_F3D::setup(&context.gbi);
    for (RT64::GBIFunction& function : context.gbi.map) {
        if (function == nullptr) {
            function = &unsupported;
        }
    }
    context.gbi.map[kOpDmaMatrix] = &dma_matrix;
    context.gbi.map[kOpDmaTextureOffset] = &dma_texture_offset;
    context.gbi.map[kOpDmaVertex] = &dma_vertex;
    context.gbi.map[kOpDmaTriangles] = &dma_triangles;
    context.gbi.map[kOpDisplayList] = &run_display_list;
    context.gbi.map[kOpDmaDisplayList] = &dma_display_list;
    context.gbi.map[G_RDPFULLSYNC] = &full_sync;
    context.gbi.map[kOpSetPerspNorm] = &set_perspective_normalization;
    context.gbi.map[kOpCullDisplayList] = &cull_display_list;
    context.gbi.map[kOpPopMatrix] = &no_op;
    context.gbi.map[kOpMoveWord] = &move_word;
    context.gbi.map[kOpDmaOffsets] = &dma_offsets;
    context.gbi.map[kOpSetTextureImage] = &set_texture_image;
    context.gbi.map[kOpSetDepthImage] = &set_depth_image;
    context.gbi.map[kOpSetColorImage] = &set_color_image;
    context.gbi.map[G_LOADBLOCK] = &load_block;
    context.gbi.map[G_LOADTILE] = &load_tile;
    context.gbi.map[G_TEXRECT] = &texture_rectangle;
    context.gbi.map[G_FILLRECT] = &fill_rectangle;
    context.gbi.map[G_TEXRECTFLIP] = &texture_rectangle_flip;
    context.gbi.map[F3D_G_MOVEMEM] = &move_mem;
    context.gbi.map[G_RDPSETOTHERMODE] = &set_other_mode_full;
    context.gbi.map[F3D_G_SETGEOMETRYMODE] = &set_geometry_mode;
    context.gbi.map[F3D_G_CLEARGEOMETRYMODE] = &clear_geometry_mode;
    // F3DDKR has no sprite microcode; stock F3D would interpret 0x09 as a
    // Sprite2D base pointer and dereference it unchecked.
    context.gbi.map[F3D_G_SPRITE2D_BASE] = &unsupported;
    context.gbi.resetFromTask = &reset;
    RT64::Application& application = *context.application;
    application.interpreter->hleGBI = &context.gbi;
    application.state->rsp->setGBI(&context.gbi);
}

bool Rt64F3ddkr::complete() const noexcept {
    // A valid setup list may contain only stock RDP/GBI commands (for
    // example FullSync followed by EndDL), so custom-command count is a
    // diagnostic rather than an acceptance requirement.
    return impl_ != nullptr && !impl_->context.failed;
}

std::size_t Rt64F3ddkr::command_count() const noexcept {
    return impl_ == nullptr ? 0U : impl_->context.commands;
}

Rt64F3ddkrStats Rt64F3ddkr::stats() const noexcept {
    return impl_ == nullptr ? Rt64F3ddkrStats{} : impl_->context.stats;
}

// Community geometry dump: called when F9 starts a recording (native_boot).
void request_geometry_dump(const unsigned tasks) noexcept {
    geomdump_pending.store(tasks);
}

}  // namespace jfg
