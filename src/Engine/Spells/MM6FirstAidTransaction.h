#pragma once

#include <array>

#include "Engine/Objects/Character.h"
#include "Engine/Spells/MM6FirstAidPreflight.h"

namespace mm6 {

struct FirstAidStateChange {
    int casterMana;
    int targetHealth;
    Duration casterRecovery;
};

// Applies a fully validated ordinary First Aid result to detached character
// state. The caller owns target selection, cues, queue cleanup and the final
// live commit; none of those are performed here.
FirstAidStateChange stageFirstAidOnCharacters(GameTitle title,
                                               const FirstAidPreflightPlan &plan,
                                               std::array<Character, 4> *characters,
                                               int targetMaxHealth);

}  // namespace mm6
