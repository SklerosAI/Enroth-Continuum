#pragma once

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

class Blob;

namespace mm6 {

// The private, F3-matched manifest holds the three fixed heal amounts.
// Loading is atomic; no regular MM6 cast is enabled by this rule alone.
void loadFirstAidRules(const Blob &source);

// Only MM6 First Aid at Normal, Expert or Master has a reviewed HP effect.
int firstAidHealAmount(SpellId spellId, Mastery mastery);

}  // namespace mm6
