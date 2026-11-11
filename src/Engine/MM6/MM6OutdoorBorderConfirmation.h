#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "Core/Time/Duration.h"
#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6OutdoorBorderEligibility.h"
#include "Engine/MM6/MM6OutdoorBorderTravel.h"
#include "Engine/Party.h"

namespace mm6 {

/**
 * What confirming a border crossing does to the party in MM6, given its food before the trip.
 */
struct BorderConfirmationEffects {
    int travelDays = 0;
    bool extraRest = false; // The trip itself always rests the party, it rests once more when it has any food.
    bool weak = false; // Not enough food for every travel day.
    int foodAfter = 0;

    bool operator==(const BorderConfirmationEffects &) const = default;
};

/**
 * @param food                          Food the party has when the trip is confirmed.
 * @param travelDays                    Hireling-adjusted trip length, see `borderTravelDays`.
 * @return                              Effects of the original MM6 confirmation.
 */
[[nodiscard]] inline BorderConfirmationEffects borderConfirmationEffects(int food, int travelDays) {
    BorderConfirmationEffects effects;
    effects.travelDays = travelDays;
    effects.extraRest = food > 0;
    effects.weak = food < travelDays;
    effects.foodAfter = std::max(0, food - travelDays);
    return effects;
}

enum class BorderConfirmationRefusal {
    None,
    WrongTitle,
    Flying,
    RouteChanged,
};

/**
 * A confirmed border crossing applied to a copy of the party. On refusal, `party` is null and nothing was copied.
 */
struct BorderConfirmation {
    std::unique_ptr<Party> party;
    std::optional<BorderTravelCandidate> destination;
    BorderConfirmationEffects effects;
    std::vector<int> weakReactions; // Characters that weren't Weak before, they voice the new condition.
    BorderConfirmationRefusal refusal = BorderConfirmationRefusal::RouteChanged;
};

/**
 * Applies the MM6 border confirmation to a copy of the party: the trip's days pass with a full rest even without food,
 * the party rests again if it has any food, gets Weak if it doesn't have enough, and arrives at the wrapped position.
 * The live party, the map and the world are never touched. The arrival Z is left to the target map's placement.
 *
 * @param title                         Game title, only MM6 is accepted.
 * @param sourceMap                     Outdoor map the party is leaving.
 * @param offered                       Crossing that the travel dialogue offered.
 * @param party                         Live party, only read.
 * @param hirelings                     Hireling professions that shorten the trip.
 * @param fullRest                      Full rest and heal of the given party, `restAndHealDetachedParty` in the game.
 *                                      It must not touch any other party.
 * @return                              The party copy with every effect applied, or a refusal.
 */
[[nodiscard]] inline BorderConfirmation stageBorderConfirmation(
    GameTitle title, std::string_view sourceMap, const BorderTravelCandidate &offered, const Party &party,
    const BorderTravelHirelings &hirelings, const std::function<void(Party &)> &fullRest) {
    if (title != GameTitle::MM6)
        return {.refusal = BorderConfirmationRefusal::WrongTitle};
    if (!borderConfirmFlyingStatePermits(title, party.bFlying))
        return {.refusal = BorderConfirmationRefusal::Flying};
    const BorderTravelAssessment route = assessOutdoorBorderTravel(title, sourceMap, party.pos.x, party.pos.y,
                                                                   hirelings);
    if (!route.candidate || route.candidate->destinationMap != offered.destinationMap)
        return {.refusal = BorderConfirmationRefusal::RouteChanged};

    BorderConfirmation result;
    result.destination = route.candidate;
    result.effects = borderConfirmationEffects(party.uNumFoodRations, route.candidate->travelDays);
    result.party = std::make_unique<Party>(party);
    Party &copy = *result.party;

    copy.GetPlayingTime() += Duration::fromDays(result.effects.travelDays);
    const CivilTime time = copy.GetPlayingTime().toCivilTime();
    copy.uCurrentTimeSecond = time.second;
    copy.uCurrentMinute = time.minute;
    copy.uCurrentHour = time.hour;
    copy.uCurrentMonthWeek = time.week - 1;
    copy.uCurrentDayOfMonth = time.day - 1;
    copy.uCurrentMonth = time.month - 1;
    copy.uCurrentYear = time.year;
    copy.pHirelings[0].hasUsedAbility = false;
    copy.pHirelings[1].hasUsedAbility = false;
    fullRest(copy);

    if (result.effects.extraRest)
        fullRest(copy);
    if (result.effects.weak) {
        for (std::size_t i = 0; i < copy.pCharacters.size(); ++i) {
            if (!copy.pCharacters[i].conditions.has(CONDITION_WEAK))
                result.weakReactions.push_back(static_cast<int>(i));
            copy.pCharacters[i].conditions.set(CONDITION_WEAK, copy.GetPlayingTime()); // Refreshed even if already Weak.
        }
        ++copy.days_played_without_rest;
    }
    copy.SetFood(result.effects.foodAfter);

    copy.pos.x = route.candidate->arrivalX;
    copy.pos.y = route.candidate->arrivalY;
    result.refusal = BorderConfirmationRefusal::None;
    return result;
}

} // namespace mm6
