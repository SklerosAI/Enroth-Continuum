#pragma once

#include <array>
#include <cstdint>

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

class Blob;

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
 * Spell point cost by spell ID, then by mastery from Normal to Master. Row 0 is unused.
 */
using SpellCosts = std::array<std::array<std::uint16_t, 3>, 100>;

/**
 * Table a spell's cost comes from. MM6.exe's table and `spells.txt` disagree on Static Charge at Expert.
 */
enum class SpellCostSource {
    SPELL_COST_FROM_EXECUTABLE,
    SPELL_COST_FROM_SPELLS_TXT,
};
using enum SpellCostSource;

/**
 * @param exe                           `MM6.exe` to read the spell table from.
 * @return                              Cost and recovery of every spell.
 */
SpellRules decodeSpellRules(const Executable &exe);

/**
 * @param spellsTxt                     MM6 `spells.txt`.
 * @return                              Normal, Expert and Master cost of every spell.
 */
SpellCosts decodeSpellCosts(const Blob &spellsTxt);

void loadSpellRules(const SpellRules &rules);

void loadSpellCosts(const SpellCosts &costs);

/**
 * @param spellId                       MM6 spell, 1 to 99.
 * @param mastery                       Normal, Expert or Master.
 * @param costSource                    Table to take the cost from. Recovery always comes from the executable.
 * @return                              The spell's cost and recovery, for a player's own cast. Scroll, event and
 *                                      monster casts have no verified rule.
 */
SpellManaRecovery spellManaRecovery(SpellId spellId, Mastery mastery, SpellCostSource costSource);

/**
 * @param spellId                       MM6 spell, 1 to 99.
 * @param mastery                       Normal, Expert or Master.
 * @return                              The spell's cost and recovery, the cost from `spells.txt` while
 *                                      `gameplay.fix_spells_cost` is on and from the executable otherwise.
 */
SpellManaRecovery spellManaRecovery(SpellId spellId, Mastery mastery);

} // namespace mm6
