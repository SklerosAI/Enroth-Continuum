#pragma once

#include <string_view>

#include "Engine/GameTitle.h"

#include "Library/Geometry/Point.h"

struct HudFrameAssets {
    std::string_view right;
    std::string_view bottom;
    std::string_view top;
    std::string_view left;
};

struct HudFramePositions {
    Pointi right;
    Pointi bottom;
    Pointi top;
    Pointi left;
};

inline constexpr HudFrameAssets hudFrameAssets(GameTitle title,
        std::string_view mm7Right, std::string_view mm7Bottom,
        std::string_view mm7Top, std::string_view mm7Left) {
    if (title == GameTitle::MM6)
        return {"border1.pcx", "border2.pcx", "border3", "border4"};
    return {mm7Right, mm7Bottom, mm7Top, mm7Left};
}

inline constexpr HudFramePositions hudFramePositions(GameTitle title) {
    if (title == GameTitle::MM6)
        return {{468, 141}, {0, 371}, {0, 0}, {0, 8}};
    return {{468, 0}, {0, 352}, {0, 0}, {0, 8}};
}
