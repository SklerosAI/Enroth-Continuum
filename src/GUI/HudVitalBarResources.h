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
    return {{{94, 385, 6, 78}, {209, 385, 6, 78},
             {323, 385, 6, 78}, {439, 385, 6, 78}}};
}

inline constexpr std::array<Recti, 4> mm6HudManaBarRects() {
    return {{{102, 385, 6, 78}, {217, 385, 6, 78},
             {331, 385, 6, 78}, {447, 385, 6, 78}}};
}
