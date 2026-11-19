#pragma once

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

// Valid only for MM6 Power Cure, three MM6 masteries and stored skill levels.
int powerCureHealAmount(SpellId spellId, Mastery mastery, int skillLevel);

}  // namespace mm6
