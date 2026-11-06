#pragma once

#include <string_view>

#include "Engine/GameTitle.h"

struct HudNavigationResources {
    std::string_view compass;
    std::string_view zoomIn;
    std::string_view zoomOut;
};

inline constexpr HudNavigationResources hudNavigationResources(GameTitle title,
        std::string_view mm7Compass, std::string_view mm7ZoomIn, std::string_view mm7ZoomOut) {
    if (title == GameTitle::MM6)
        return {"compass", "zoom+on", "zoom-on"};
    return {mm7Compass, mm7ZoomIn, mm7ZoomOut};
}
