#pragma once

#include <array>
#include <cstdint>

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

class Executable;

struct SpellManaRecovery {
    std::uint16_t mana = 0;
    std::int16_t recoveryTicks = 0;
};

/**
 * Spell point cost and recovery by spell ID, then by mastery from Normal to Master. Row 0 is unused.
 */
using SpellRules = std::array<std::array<SpellManaRecovery, 3>, 100>;

/**
 * @param exe                           `MM6.exe` to read the spell table from.
 * @return                              Cost and recovery of every spell.
 */
SpellRules decodeSpellRules(const Executable &exe);

void loadSpellRules(const SpellRules &rules);

// Valid only for MM6 spell IDs 1..99 and Normal/Expert/Master. It does not
// assert effect, scroll, event or monster cast behavior.
SpellManaRecovery spellManaRecovery(SpellId spellId, Mastery mastery);

} // namespace mm6
