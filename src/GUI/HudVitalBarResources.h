#pragma once

#include <array>
#include <string_view>

#include "Engine/GameTitle.h"

#include "Library/Geometry/Rect.h"

struct HudVitalBarResources {
    std::string_view blue;
    std::string_view green;
    std::string_view yellow;
    std::string_view red;
};

inline constexpr HudVitalBarResources hudVitalBarResources(GameTitle title,
        std::string_view mm7Blue, std::string_view mm7Green,
        std::string_view mm7Yellow, std::string_view mm7Red) {
    if (title == GameTitle::MM6)
        return {"manafull", "hitsfull", {}, {}};
    return {mm7Blue, mm7Green, mm7Yellow, mm7Red};
}

inline constexpr std::array<Recti, 4> mm6HudHealthBarRects() {
    // Original MM6 HUD: x = 93 + {0,113,226,338}, row 383, 6x78.
    return {{{93, 383, 6, 78}, {206, 383, 6, 78},
             {319, 383, 6, 78}, {431, 383, 6, 78}}};
}

inline constexpr std::array<Recti, 4> mm6HudManaBarRects() {
    // The paired mana slot starts nine pixels to the right of health.
    return {{{102, 383, 6, 78}, {215, 383, 6, 78},
             {328, 383, 6, 78}, {440, 383, 6, 78}}};
}
