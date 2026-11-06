#pragma once

#include <array>

#include "Engine/GameTitle.h"

#include "GUI/GUIEnums.h"

#include "Io/InputEnums.h"

#include "Library/Geometry/Rect.h"

struct MM6HudBookControl {
    Recti rect;
    UIMessageType message;
    InputAction action;
};

inline constexpr bool useMm6HudBookControls(GameTitle title) {
    return title == GameTitle::MM6;
}

inline constexpr std::array<MM6HudBookControl, 4> mm6HudBookControls() {
    // The manual's fourth "Events" book manages time. Route its input to the
    // existing Calendar action; MM6 page content and art need separate work.
    return {{
        {{492, 260, 34, 81}, UIMSG_OpenQuestBook, INPUT_ACTION_OPEN_QUESTS},
        {{526, 260, 34, 81}, UIMSG_OpenAutonotes, INPUT_ACTION_OPEN_AUTONOTES},
        {{560, 260, 34, 81}, UIMSG_OpenMapBook, INPUT_ACTION_OPEN_MAP},
        {{594, 260, 34, 81}, UIMSG_OpenCalendar, INPUT_ACTION_OPEN_CALENDAR}
    }};
}
