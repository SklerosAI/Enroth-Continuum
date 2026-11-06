#pragma once

#include <array>
#include <stdexcept>

#include "Engine/GameTitle.h"
#include "Engine/Objects/CharacterEnums.h"

namespace mm6 {

// Original MM6 actual-stat evaluation chooses the first active condition in
// this order. It has no zombie entry; no active condition is neutral.
inline constexpr std::array<Condition, 17> kActualStatConditionPriority = {
    CONDITION_ERADICATED, CONDITION_PETRIFIED, CONDITION_DEAD,
    CONDITION_UNCONSCIOUS, CONDITION_SLEEP, CONDITION_PARALYZED,
    CONDITION_DISEASE_SEVERE, CONDITION_POISON_SEVERE,
    CONDITION_DISEASE_MEDIUM, CONDITION_POISON_MEDIUM,
    CONDITION_DISEASE_WEAK, CONDITION_POISON_WEAK,
    CONDITION_INSANE, CONDITION_DRUNK, CONDITION_FEAR,
    CONDITION_WEAK, CONDITION_CURSED
};

template <class HasCondition>
Condition actualStatCondition(GameTitle title, HasCondition &&hasCondition) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 actual-stat condition used for another title");
    for (Condition condition : kActualStatConditionPriority) {
        if (hasCondition(condition))
            return condition;
    }
    return CONDITION_GOOD;
}

}  // namespace mm6
