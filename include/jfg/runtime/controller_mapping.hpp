#pragma once

#include "jfg/runtime/input_stick.hpp"
#include "jfg/runtime/controller_device_key.hpp"
#include <array>
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

namespace jfg {
// SDL's standardized controller layout, independent of device/vendor names.
// Bindings 0..14 are buttons; 15..26 are +/- directions for six axes.
struct ControllerMapping {
    int device = -1;
    std::string device_key; // device=-4: stable SDL identity, never an enumeration index.
    int stick = 0;
    int deadzone = 7849;
    int threshold = 8689;
    int trigger = 3855;
    int invert_x = 0;
    int invert_y = 0;
    std::array<int, 14> bindings{0, 1, 23, 6, 11, 12, 13, 14, 9, 10, 22, 21, 20, 19};
};

inline bool parse_controller_mapping(std::string_view text, ControllerMapping& result) {
    if (text.empty() || text.size() > 4096U) return false;
    ControllerMapping candidate;
    std::array<bool, 23> seen{};
    int version=0;
    while (!text.empty()) {
        const auto newline = text.find('\n');
        auto line = text.substr(0, newline);
        text = newline == std::string_view::npos ? std::string_view{} : text.substr(newline + 1U);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.empty() || line.size() > 128U) return false;
        const auto equal = line.find('=');
        if (equal == std::string_view::npos) return false;
        const auto key = line.substr(0, equal), value = line.substr(equal + 1U);
        constexpr std::array<std::string_view, 8> keys{
            "version", "device", "stick", "deadzone", "threshold", "trigger", "invert_x", "invert_y"};
        std::size_t slot = seen.size();
        for (std::size_t i = 0; i < keys.size(); ++i) if (key == keys[i]) slot = i;
        for (std::size_t i = 0; i < 14U; ++i) if (key == "map" + std::to_string(i)) slot = i + 8U;
        if(key=="device_key")slot=22;
        if (slot == seen.size() || seen[slot]) return false;
        seen[slot] = true;
        if(slot==22) {
            if(!valid_controller_device_key(value))return false;
            candidate.device_key=value;continue;
        }
        int number = 0;
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() ||
            value != std::to_string(number)) return false;
        switch (slot) {
        case 0: if (number != 1 && number != 2) return false; version=number; break;
        case 1: candidate.device = number; break;
        case 2: candidate.stick = number; break;
        case 3: candidate.deadzone = number; break;
        case 4: candidate.threshold = number; break;
        case 5: candidate.trigger = number; break;
        case 6: candidate.invert_x = number; break;
        case 7: candidate.invert_y = number; break;
        default: candidate.bindings[slot - 8U] = number; break;
        }
    }
    for(std::size_t i=0;i<22;++i)if(!seen[i])return false;
    if(version==1 && (seen[22] || candidate.device==-4))return false;
    if(version==2 && (candidate.device!=-4 || !seen[22]))return false;
    if (candidate.device < -4 || candidate.device > 3 || candidate.stick < 0 || candidate.stick > 1 || candidate.deadzone < 0 || candidate.deadzone > 30000 ||
        candidate.threshold < 1000 || candidate.threshold > 32000 || candidate.trigger < 1000 || candidate.trigger > 32000 ||
        candidate.invert_x < 0 || candidate.invert_x > 1 || candidate.invert_y < 0 || candidate.invert_y > 1) return false;
    for (const int binding : candidate.bindings) if (binding < -1 || binding > 26) return false;
    result = candidate; // Invalid files never partially replace the active mapping.
    return true;
}

inline std::string serialize_controller_mapping(const ControllerMapping& mapping) {
    std::string text=mapping.device==-4?"version=2\n":"version=1\n";
    const std::array<std::string_view,7> keys{"device","stick","deadzone","threshold","trigger","invert_x","invert_y"};
    const std::array<int,7> values{mapping.device,mapping.stick,mapping.deadzone,mapping.threshold,mapping.trigger,mapping.invert_x,mapping.invert_y};
    for(std::size_t i=0;i<keys.size();++i)text+=std::string(keys[i])+"="+std::to_string(values[i])+"\n";
    for(std::size_t i=0;i<mapping.bindings.size();++i)text+="map"+std::to_string(i)+"="+std::to_string(mapping.bindings[i])+"\n";
    if(mapping.device==-4)text+="device_key="+mapping.device_key+"\n";
    return text;
}

struct StandardControllerSample {
    std::array<bool, 15> buttons{};
    std::array<int, 6> axes{};
};
struct MappedControllerSample {
    std::uint16_t buttons = 0;
    N64StickSample stick{};
};
inline MappedControllerSample map_controller(const ControllerMapping& mapping,
                                             const StandardControllerSample& sample) noexcept {
    constexpr std::array<std::uint16_t, 14> masks{
        0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
        0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001};
    MappedControllerSample result;
    for (std::size_t i = 0; i < mapping.bindings.size(); ++i) {
        const int binding = mapping.bindings[i];
        bool down = false;
        if (binding >= 0 && binding < 15) down = sample.buttons[static_cast<std::size_t>(binding)];
        else if (binding >= 15 && binding <= 26) {
            const auto axis = static_cast<std::size_t>((binding - 15) / 2);
            const int threshold = axis >= 4U ? mapping.trigger : mapping.threshold;
            down = (binding - 15) % 2 == 0 ? sample.axes[axis] > threshold : sample.axes[axis] < -threshold;
        }
        if (down) result.buttons |= masks[i];
    }
    const auto axis = static_cast<std::size_t>(mapping.stick == 1 ? 2 : 0);
    // SDL Y grows downward; N64 Y grows upward. Normalize before optional inversion.
    const int x = sample.axes[axis] * (mapping.invert_x ? -1 : 1);
    const int y = sample.axes[axis + 1U] * (mapping.invert_y ? 1 : -1);
    result.stick = scale_xinput_left_stick(x, y, mapping.deadzone);
    return result;
}
} // namespace jfg
