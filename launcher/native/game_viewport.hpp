#pragma once
#include <algorithm>
#include <cstdint>
namespace jfg::frontend {
struct GameViewport { int x, y, width, height; };
// Fit the game's selected display mode, independently of transient cinematic
// VI cropping. Widescreen is 16:9; the original display mode remains 4:3.
inline GameViewport game_viewport(int width, int height, int aspect_width=4, int aspect_height=3) noexcept {
    if(aspect_width<=0 || aspect_height<=0){aspect_width=4;aspect_height=3;}
    width = (std::max)(1, width); height = (std::max)(1, height);
    int w = width, h = height;
    if (std::int64_t(width) * aspect_height > std::int64_t(height) * aspect_width)
        w = static_cast<int>(std::int64_t(height) * aspect_width / aspect_height);
    else h = static_cast<int>(std::int64_t(width) * aspect_height / aspect_width);
    w = (std::max)(1, w); h = (std::max)(1, h);
    return {(width - w) / 2, (height - h) / 2, w, h};
}
}
