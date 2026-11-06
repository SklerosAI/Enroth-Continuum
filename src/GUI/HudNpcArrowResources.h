#pragma once

#include <string_view>

#include "Engine/GameTitle.h"

struct HudNpcArrowResources {
    std::string_view left;
    std::string_view right;
};

// MM6's 14x25 up-state arrows are retained as distinct left/right icons.
// Their placement and click bounds remain owned by the UI layout.
inline constexpr HudNpcArrowResources hudNpcArrowResources(
    GameTitle title, std::string_view mm7Left, std::string_view mm7Right) {
    if (title == GameTitle::MM6)
        return {"npc_lup", "npc_rup"};
    return {mm7Left, mm7Right};
}
