#pragma once

#include <cstdint>

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

class Blob;

namespace mm6 {

struct SpellManaRecovery {
    std::uint16_t mana = 0;
    std::int16_t recoveryTicks = 0;
};

// The private profile manifest is generated from the F3-matched executable
// table. Loading is atomic; malformed rows cannot replace a valid table.
void loadSpellRules(const Blob &source);

// Valid only for MM6 spell IDs 1..99 and Normal/Expert/Master. It does not
// assert effect, scroll, event or monster cast behavior.
SpellManaRecovery spellManaRecovery(SpellId spellId, Mastery mastery);

} // namespace mm6
