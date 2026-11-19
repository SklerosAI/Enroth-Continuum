#pragma once

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

// Only MM6 First Aid at Normal, Expert or Master has a reviewed HP effect.
int firstAidHealAmount(SpellId spellId, Mastery mastery);

}  // namespace mm6
