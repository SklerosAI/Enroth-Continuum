#pragma once

#include <string_view>

#include "Engine/GameTitle.h"

inline constexpr std::string_view calendarBackgroundIcon(GameTitle title) {
    return title == GameTitle::MM6 ? "time_bg" : "sbdate-time";
}
