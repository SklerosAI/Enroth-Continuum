#pragma once

#include <utility>

#include "Engine/Data/DecorationEnums.h"

#include "Library/Random/RandomEngine.h"

namespace mm6 {

/**
 * MM6 runs the global event `decorVars[eventVarId] + MM6_DECORATION_EVENT_BASE` for an interactive decoration, and
 * names it with that event's topic on hover.
 */
inline constexpr int MM6_DECORATION_EVENT_BASE = 400;

/**
 * Ports the decoration switch that MM6.exe repeats for hover at 0x41F70C, click at 0x420AA8, respawn at 0x455220 and
 * map load at 0x455651.
 *
 * @param id                            MM6 decoration list row.
 * @return                              Whether a decoration with this row and no map event runs a global event.
 */
inline bool isInteractiveDecoration(DecorationId id) {
    switch (std::to_underlying(id)) {
    case 118:
    case 119:
    case 120:
    case 121:
    case 146:
    case 154:
    case 155:
    case 158:
    case 162:
    case 163:
    case 164:
    case 166:
    case 167:
    case 182:
        return true;
    default:
        return false;
    }
}

/**
 * Ports MM6.exe 0x455050, which picks the first global event of an interactive decoration on respawn.
 *
 * @param id                            MM6 decoration list row.
 * @param rng                           Random engine. One roll out of 100 is always drawn, the 118-121 rows draw a
 *                                      second one out of 10 when the first is 50 or more.
 * @return                              Global event minus `MM6_DECORATION_EVENT_BASE`, or 0 for a row that isn't
 *                                      interactive.
 */
inline int decorationGlobalEvent(DecorationId id, RandomEngine &rng) {
    const int roll = rng.random(100);
    switch (std::to_underlying(id)) {
    case 118:
    case 119:
    case 120:
    case 121:
        return roll < 50 ? 47 : 48 + rng.random(10);
    case 146:
        return roll < 20 ? 37 : 36;
    case 154:
        return roll < 40 ? 31 : roll < 70 ? 32 : roll < 90 ? 33 : 34;
    case 155:
        return roll < 25 ? 43 : roll < 50 ? 44 : roll < 75 ? 45 : 46;
    case 158:
        return roll < 50 ? 29 : 30;
    case 162:
        return 35;
    case 163:
    case 164:
        return roll < 30 ? 10 : roll < 90 ? 11 + (roll - 30) / 10 : 17;
    case 166:
        return roll < 40 ? 39 : roll < 70 ? 40 : roll < 90 ? 41 : 42;
    case 167:
        return roll < 80 ? 25 : roll < 90 ? 26 : roll < 97 ? 27 : 28;
    case 182:
        return roll < 20 ? 19 : 23;
    default:
        return 0;
    }
}

} // namespace mm6
