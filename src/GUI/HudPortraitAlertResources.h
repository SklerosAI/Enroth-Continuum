#pragma once

#include <array>
#include <string_view>

#include "Engine/GameTitle.h"
#include "Library/Geometry/Point.h"

struct HudPortraitAlertResources {
    std::string_view green;
    std::string_view yellow;
    std::string_view red;
};

inline constexpr HudPortraitAlertResources hudPortraitAlertResources(GameTitle title,
        std::string_view mm7Green, std::string_view mm7Yellow, std::string_view mm7Red) {
    if (title == GameTitle::MM6)
        return {"BUTTGEM", "BUTTyel", "BUTTred"};
    return {mm7Green, mm7Yellow, mm7Red};
}

template<class T>
inline constexpr T hudPortraitAlertForFlags(T green, T yellow, T red, bool redAlert, bool yellowAlert) {
    return redAlert ? red : yellowAlert ? yellow : green;
}

// The original MM6 framebuffer blit uses a fixed four-entry x table plus a
// destination offset of (56, 378). MM7 retains its existing coordinates.
inline constexpr Pointi hudPortraitAlertPosition(GameTitle title, int index, Pointi mm7Position) {
    if (title != GameTitle::MM6 || index < 0 || index >= 4)
        return mm7Position;
    constexpr std::array<int, 4> x = {78, 191, 304, 416};
    return {x[index], 378};
}
