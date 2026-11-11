#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6V5OutdoorMapIdentity.h"
#include "Engine/Objects/NPCEnums.h"

namespace mm6 {

inline constexpr float MM6_OUTDOOR_BORDER_EDGE = 22528.0f;

/**
 * Neighbouring outdoor map that walking off the edge of an MM6 outdoor map leads to, and where the party arrives.
 */
struct BorderTravelCandidate {
    std::string destinationMap;
    float arrivalX = 0;
    float arrivalY = 0;
    int travelDays = 5;
    bool crossedX = false;
    bool crossedY = false;
};

struct BorderTravelHirelings {
    bool guide = false;
    bool tracker = false;
    bool pathfinder = false;
    bool explorer = false;
};

/**
 * @param isHired                       Whether a hireling with the given profession is in the party, as
 *                                      `CheckHiredNPCSpeciality` answers it.
 * @return                              The hirelings that shorten MM6 border travel. They are the MM6 Guide, Tracker,
 *                                      Pathfinder and Explorer, the same profession IDs as in MM7.
 */
[[nodiscard]] inline BorderTravelHirelings borderTravelHirelings(const std::function<bool(NpcProfession)> &isHired) {
    return {.guide = isHired(NPC_PROFESSION_GUIDE), .tracker = isHired(NPC_PROFESSION_TRACKER),
            .pathfinder = isHired(NPC_PROFESSION_PATHFINDER), .explorer = isHired(NPC_PROFESSION_EXPLORER)};
}

enum class BorderTravelRefusal {
    None,
    WrongTitle,
    InvalidSourceMap,
    InvalidCoordinates,
    NoBorderCrossing,
    OutsideGrid,
    ArrivalOutsideBounds,
};

struct BorderTravelAssessment {
    std::optional<BorderTravelCandidate> candidate;
    BorderTravelRefusal refusal = BorderTravelRefusal::InvalidSourceMap;
};

// Original MM6 has a five-day base and these four reductions, with a one-day
// floor. Keep the hired-profession lookup outside this pure function.
[[nodiscard]] inline int borderTravelDays(const BorderTravelHirelings &hirelings) {
    int days = 5;
    days -= hirelings.guide ? 1 : 0;
    days -= hirelings.tracker ? 2 : 0;
    days -= hirelings.pathfinder ? 3 : 0;
    days -= hirelings.explorer ? 1 : 0;
    return std::max(1, days);
}

/**
 * Finds the outdoor map that walking off an edge leads to. The neighbour's `outXY.odm` name has the column letter or
 * the row digit moved, both at a corner, and each crossed axis wraps by one map width. No map has an exceptional edge.
 *
 * @param title                         Game title, only MM6 has this route.
 * @param sourceMap                     Outdoor map the party is on.
 * @param partyX                        Party X.
 * @param partyY                        Party Y.
 * @param hirelings                     Hireling professions that shorten the trip.
 * @return                              The neighbouring map and arrival, or a refusal. Party state that blocks the
 *                                      offer is the caller's to check, see `borderOfferPartyFlagsPermit`.
 */
[[nodiscard]] inline BorderTravelAssessment assessOutdoorBorderTravel(
    GameTitle title, std::string_view sourceMap, float partyX, float partyY,
    const BorderTravelHirelings &hirelings) {
    if (title != GameTitle::MM6)
        return {.refusal = BorderTravelRefusal::WrongTitle};
    if (!isMM6OutdoorMapName(sourceMap))
        return {.refusal = BorderTravelRefusal::InvalidSourceMap};
    if (!std::isfinite(partyX) || !std::isfinite(partyY))
        return {.refusal = BorderTravelRefusal::InvalidCoordinates};

    constexpr float edge = MM6_OUTDOOR_BORDER_EDGE;
    constexpr float width = edge * 2;
    const int columnChange = partyX < -edge ? -1 : (partyX > edge ? 1 : 0);
    const int rowChange = partyY < -edge ? 1 : (partyY > edge ? -1 : 0);
    if (columnChange == 0 && rowChange == 0)
        return {.refusal = BorderTravelRefusal::NoBorderCrossing};

    const int column = static_cast<int>(sourceMap[3]) + columnChange;
    const int row = static_cast<int>(sourceMap[4]) + rowChange;
    if (column < 'a' || column > 'e' || row < '1' || row > '3')
        return {.refusal = BorderTravelRefusal::OutsideGrid};

    const float arrivalX = partyX - columnChange * width;
    const float arrivalY = partyY + rowChange * width;
    if (!std::isfinite(arrivalX) || !std::isfinite(arrivalY) ||
        arrivalX < -edge || arrivalX > edge || arrivalY < -edge || arrivalY > edge)
        return {.refusal = BorderTravelRefusal::ArrivalOutsideBounds};

    BorderTravelCandidate candidate;
    candidate.destinationMap = sourceMap;
    candidate.destinationMap[3] = static_cast<char>(column);
    candidate.destinationMap[4] = static_cast<char>(row);
    candidate.arrivalX = arrivalX;
    candidate.arrivalY = arrivalY;
    candidate.travelDays = borderTravelDays(hirelings);
    candidate.crossedX = columnChange != 0;
    candidate.crossedY = rowChange != 0;
    return {.candidate = std::move(candidate), .refusal = BorderTravelRefusal::None};
}

} // namespace mm6
