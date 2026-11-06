#pragma once

#include "Core/Time/Time.h"

#include "Engine/Objects/CharacterConditions.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

class Blob;

namespace mm6 {

// Six F3-matched single-character, single-condition cures share one private
// three-rank age-window manifest. This does not open the live cast gate.
void loadTimedCureRules(const Blob &source);
bool isTimedCureSpell(SpellId spellId);

// Returns true only when the target's matching condition was removed.
// All ID, rank, level and clock checks precede mutation.
bool applyTimedCure(SpellId spellId, Mastery mastery, int skillLevel,
                    Time now, CharacterConditions &conditions);

}  // namespace mm6
