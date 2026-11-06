#pragma once

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

class Blob;

namespace mm6 {

// The private F3-matched profile manifest holds the base and per-skill heal.
// The live cast gate remains closed until its surrounding transaction is proven.
void loadPowerCureRules(const Blob &source);

// Valid only for MM6 Power Cure, three MM6 masteries and stored skill levels.
int powerCureHealAmount(SpellId spellId, Mastery mastery, int skillLevel);

}  // namespace mm6
