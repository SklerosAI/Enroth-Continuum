#include "Engine/Spells/MM6SpellRules.h"

#include <optional>
#include <stdexcept>
#include <utility>

#include "Engine/MM6/MM6Executable.h"

namespace mm6 {

static std::optional<SpellRules> loadedSpellRules;

SpellRules decodeSpellRules(const Executable &exe) {
    SpellRules result{};
    for (int spellId = 1; spellId <= 99; spellId++) {
        const std::uint32_t row = 0x4BDD70 + 14 * spellId; // Cost by mastery, recovery by mastery, flags.
        for (int rank = 0; rank < 3; rank++)
            result[spellId][rank] = {static_cast<std::uint16_t>(exe.i16(row + 2 * rank)), exe.i16(row + 6 + 2 * rank)};
    }
    return result;
}

void loadSpellRules(const SpellRules &rules) {
    loadedSpellRules = rules;
}

SpellManaRecovery spellManaRecovery(SpellId spellId, Mastery mastery) {
    const int id = std::to_underlying(spellId);
    const int rank = std::to_underlying(mastery);
    if (id < 1 || id > 99 || rank < 1 || rank > 3)
        throw std::runtime_error("MM6 spell rule lookup has invalid spell ID or mastery");
    if (!loadedSpellRules)
        throw std::runtime_error("MM6 spell rules have not been loaded");
    return (*loadedSpellRules)[id][rank - 1];
}

} // namespace mm6
