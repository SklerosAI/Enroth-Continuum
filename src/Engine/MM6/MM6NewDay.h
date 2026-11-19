#pragma once

#include <algorithm>

#include "Engine/Objects/Character.h"
#include "Engine/Party.h"

#include "Library/Random/RandomEngine.h"

namespace mm6 {

/**
 * What a jump in time across a month end does to the day change.
 */
enum class MonthStartRule {
    MONTH_START_VANILLA, // A jump across a month end with both hours past 3 am starts no new day.
    MONTH_START_GRAYFACE // It does, the GrayFace patch compares the days for inequality.
};
using enum MonthStartRule;

/**
 * @offset 0x4881C4
 *
 * @param oldHour                       Hour of the party clock before time moved on.
 * @param oldDayOfMonth                 Day of the month before time moved on, 0-based.
 * @param newHour                       Hour after time moved on.
 * @param newDayOfMonth                 Day of the month after time moved on, 0-based.
 * @param rule                          What a jump across a month end does.
 * @return                              Whether MM6 starts a new day. A jump that lands before 3 am starts none.
 */
inline bool isNewDay(int oldHour, int oldDayOfMonth, int newHour, int newDayOfMonth, MonthStartRule rule) {
    const bool dayChanged = rule == MONTH_START_GRAYFACE ? newDayOfMonth != oldDayOfMonth : newDayOfMonth > oldDayOfMonth;
    return newHour >= 3 && (oldHour < 3 || dayChanged);
}

/**
 * @offset 0x4881E4
 *
 * MM6's party rules for a new day: reputation decays, and the days without rest count up, with weakness, hunger,
 * starvation, death and insanity for a party that keeps going without rest. Conditions are written as they are, with
 * no speech and no protection from items.
 *
 * @param[in,out] party                 Party whose day begins.
 * @param rng                           Game RNG.
 */
inline void startNewDay(Party &party, RandomEngine &rng) {
    party.mm6Reputation = std::clamp(static_cast<int>(party.mm6Reputation * 0.99), -1500, 1500);

    const Time now = party.GetPlayingTime();
    if (++party.days_played_without_rest <= 1)
        return;

    for (Character &character : party.pCharacters)
        if (!character.conditions.has(CONDITION_WEAK))
            character.conditions.set(CONDITION_WEAK, now);

    if (party.GetFood() > 0) {
        party.TakeFood(1);
    } else {
        for (Character &character : party.pCharacters)
            character.health = character.health / (party.days_played_without_rest + 1) + 1;
    }

    if (party.days_played_without_rest <= 3)
        return;

    for (Character &character : party.pCharacters) {
        character.resetTempBonuses();
        if (character.conditions.hasAny({CONDITION_DEAD, CONDITION_PETRIFIED, CONDITION_ERADICATED}))
            continue;
        if (rng.random(100) < 5 * party.days_played_without_rest) {
            character.conditions.set(CONDITION_DEAD, now);
            character.health = 0;
            character.mana = 0;
        } else if (rng.random(100) < 10 * party.days_played_without_rest) {
            if (!character.conditions.has(CONDITION_INSANE))
                character.conditions.set(CONDITION_INSANE, now);
        }
    }
}

} // namespace mm6
