#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace jfg {

inline constexpr char kInputReplayHeaderV8[] = "jfg-phase8-input-v8";
inline constexpr char kInputReplayHeaderV7[] = "jfg-phase8-input-v7";
inline constexpr char kInputReplayHeaderV6[] = "jfg-phase8-input-v6";
inline constexpr char kInputReplayHeaderV5[] = "jfg-phase8-input-v5";
inline constexpr char kInputReplayHeaderV4[] = "jfg-phase8-input-v4";
inline constexpr char kInputReplayHeaderV3[] = "jfg-phase8-input-v3";
inline constexpr char kInputReplayHeaderV1[] = "jfg-phase8-input-v1";
inline constexpr char kInputReplayHeader[] = "jfg-phase8-input-v2";
// Manual play recordings sample input throughout long routes. The byte-size
// limit in the loader remains the primary bound; allow enough individual
// samples for roughly two hours of 30 Hz controller polling without rejecting a
// recorder-produced file.
inline constexpr std::size_t kMaximumInputReplayEvents = 262144U;

struct ControllerReplaySample {
    bool connected = true;
    std::uint16_t buttons = 0U;
    std::int8_t stick_x = 0;
    std::int8_t stick_y = 0;

    // 0: stock, 1/2: legacy aim, 3: mouse angles, 4: legacy 8-bit stick rate,
    // 5 (v4 only): precise modern stick rate, 2048 units at full travel/100%.
    std::uint8_t pc_mode = 0;
    std::int16_t look_x = 0, look_y = 0;
    // v5: bit 7 selects modern action semantics; bits 0..2 are held scope actions.
    std::uint8_t modern_flags = 0;

    // v6: semantic native-menu selection, separate from emulated pad input.
    // kind 2=start, 3=saves, 4=save actions; target is one-based (0=no event).
    std::uint8_t menu_kind = 0, menu_target = 0;

    bool operator==(const ControllerReplaySample&) const = default;

    [[nodiscard]] bool neutral() const noexcept {
        return connected && buttons == 0U && stick_x == 0 && stick_y == 0 && pc_mode == 0 && look_x == 0 && look_y == 0 && modern_flags == 0 && menu_kind == 0 && menu_target == 0;
    }
};

struct InputReplayEvent {
    std::uint64_t first_retrace = 0U;
    std::uint64_t end_retrace = 0U;
    ControllerReplaySample sample;

    bool operator==(const InputReplayEvent&) const = default;
};

enum class InputReplayError {
    none = 0,
    io_error,
    too_large,
    invalid_header,
    invalid_record,
    invalid_order,
};

// Poll recordings may contain distinct samples taken during the same retrace.
enum class InputReplayOrder { retrace, recorded_poll };

class DeterministicInputReplay final {
public:
    [[nodiscard]] static InputReplayError load(
        const std::filesystem::path& path,
        DeterministicInputReplay& output,
        InputReplayOrder order = InputReplayOrder::retrace);

    [[nodiscard]] ControllerReplaySample sample_at(
        std::uint64_t retrace) const noexcept;
    [[nodiscard]] ControllerReplaySample sample_by_poll(
        std::uint64_t poll) const noexcept;
    [[nodiscard]] const std::vector<InputReplayEvent>& events() const noexcept;

private:
    std::vector<InputReplayEvent> events_;
};

}  // namespace jfg
