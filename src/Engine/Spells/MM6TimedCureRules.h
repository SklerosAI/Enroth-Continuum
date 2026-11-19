#pragma once

#include "Core/Time/Time.h"

#include "Engine/Objects/CharacterConditions.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

/**
 * How old a condition may be for a condition cure spell to remove it.
 */
enum class TimedCureWindow {
    TIMED_CURE_WINDOW_ORIGINAL, // MM6.exe. Remove Fear, Cure Paralysis, Cure Insanity and Cure Weakness take 3 hours per skill point at Expert, 3 days at Master.
    TIMED_CURE_WINDOW_DESCRIBED, // The spell descriptions and the GrayFace patch, 1 hour per skill point at Expert and 1 day at Master.
};
using enum TimedCureWindow;

bool isTimedCureSpell(SpellId spellId);

// Returns true only when the target's matching condition was removed.
// All ID, rank, level and clock checks precede mutation.
bool applyTimedCure(SpellId spellId, Mastery mastery, int skillLevel, Time now, CharacterConditions &conditions,
                    TimedCureWindow window);

}  // namespace mm6
