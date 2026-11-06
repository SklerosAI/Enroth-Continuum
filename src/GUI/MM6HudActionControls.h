#pragma once

#include <array>

#include "Engine/GameTitle.h"

#include "GUI/GUIEnums.h"

#include "Io/InputEnums.h"

#include "Library/Geometry/Rect.h"

struct MM6HudActionControl {
    Recti rect;
    UIMessageType message;
    InputAction action;
};

inline constexpr bool useMm6HudActionControls(GameTitle title) {
    return title == GameTitle::MM6;
}

inline constexpr std::array<MM6HudActionControl, 4> mm6HudActionControls() {
    // The four controls are baked into the lower right frame. Their roles
    // follow the original manual; the rectangles bound the visible ovals.
    return {{
        {{490, 398, 29, 61}, UIMSG_SpellBookWindow, INPUT_ACTION_OPEN_SPELLBOOK},
        {{524, 398, 29, 61}, UIMSG_RestWindow, INPUT_ACTION_REST},
        {{559, 398, 29, 61}, UIMSG_QuickReference, INPUT_ACTION_OPEN_QUICK_REFERENCE},
        {{594, 398, 29, 61}, UIMSG_GameMenuButton, INPUT_ACTION_INVALID}
    }};
}
