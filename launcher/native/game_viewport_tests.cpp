#include "game_viewport.hpp"
#include <cstdlib>
#include <iostream>
static void check(bool value) { if (!value) std::abort(); }
int main() {
    using jfg::frontend::game_viewport;
    auto wide = game_viewport(1920, 1080);
    check(wide.x == 240 && wide.y == 0 && wide.width == 1440 && wide.height == 1080);
    auto classic = game_viewport(640, 480);
    check(classic.x == 0 && classic.y == 0 && classic.width == 640 && classic.height == 480);
    auto tall = game_viewport(600, 900);
    check(tall.x == 0 && tall.y == 225 && tall.width == 600 && tall.height == 450);
    auto ultrawide = game_viewport(3440, 1440);
    check(ultrawide.x == 760 && ultrawide.width == 1920 && ultrawide.height == 1440);
    for (int w = 1; w < 2048; w += 17) for (int h = 1; h < 1600; h += 19) {
        auto v = game_viewport(w, h);
        check(v.x >= 0 && v.y >= 0 && v.width > 0 && v.height > 0);
        check(v.x + v.width <= w && v.y + v.height <= h);
        check(std::abs(w - v.width - 2*v.x) <= 1 && std::abs(h - v.height - 2*v.y) <= 1);
        check(std::abs(3*v.width - 4*v.height) <= 4);
    }
    const auto wideMode=game_viewport(1920,1080,320,180);
    check(wideMode.x==0 && wideMode.y==0 && wideMode.width==1920 && wideMode.height==1080);
    const auto wideInClassic=game_viewport(640,480,320,180);
    check(wideInClassic.x==0 && wideInClassic.y==60 && wideInClassic.width==640 && wideInClassic.height==360);
    const auto wideUltra=game_viewport(3440,1440,16,9);
    check(wideUltra.x==440 && wideUltra.width==2560 && wideUltra.height==1440);
    const auto invalid=game_viewport(1920,1080,0,0);
    check(invalid.x==wide.x && invalid.width==wide.width);
    for(int w=1;w<2048;w+=17)for(int h=1;h<1600;h+=19) {
      const auto v=game_viewport(w,h,16,9);
      check(v.x>=0 && v.y>=0 && v.width>0 && v.height>0);
      check(v.x+v.width<=w && v.y+v.height<=h);
      check(std::abs(9*v.width-16*v.height)<=16);
    }
    auto minimized = game_viewport(0, 0);
    check(minimized.width == 1 && minimized.height == 1);
    std::cout << "Game viewport checks passed (windowed, fullscreen, ultrawide, portrait, odd sizes).\n";
}
