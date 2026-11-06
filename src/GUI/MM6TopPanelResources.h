#pragma once

#include <cstdint>
#include <string_view>

#include "Engine/GameTitle.h"

// The original MM6 top-panel blit selects by the party's 24-hour clock.
// Its unsigned comparisons route an out-of-range value to the night panel.
inline constexpr std::string_view mm6TopPanelIconForHour(GameTitle title, int hour) {
    if (title != GameTitle::MM6)
        return {};
    const std::uint32_t sourceHour = static_cast<std::uint32_t>(hour);
    if (sourceHour < 5)
        return "tap4";
    if (sourceHour < 6)
        return "tap3";
    if (sourceHour < 20)
        return "tap2";
    if (sourceHour < 21)
        return "tap1";
    return "tap4";
}
