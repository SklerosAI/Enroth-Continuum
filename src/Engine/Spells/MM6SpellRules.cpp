#include "Engine/Spells/MM6SpellRules.h"

#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/MM6/MM6Executable.h"

#include "Library/Serialization/Serialization.h"

#include "Utility/Memory/Blob.h"
#include "Utility/String/Split.h"
#include "Utility/String/Transformations.h"

namespace mm6 {

static std::optional<SpellRules> loadedSpellRules;
static std::optional<SpellCosts> loadedSpellCosts;

SpellRules decodeSpellRules(const Executable &exe) {
    SpellRules result{};
    for (int spellId = 1; spellId <= 99; spellId++) {
        const std::uint32_t row = 0x4BDD70 + 14 * spellId; // Cost by mastery, recovery by mastery, flags.
        for (int rank = 0; rank < 3; rank++)
            result[spellId][rank] = {static_cast<std::uint16_t>(exe.i16(row + 2 * rank)), exe.i16(row + 6 + 2 * rank)};
    }
    return result;
}

SpellCosts decodeSpellCosts(const Blob &spellsTxt) {
    SpellCosts result{};
    std::array<bool, 100> seen{};
    for (std::string_view line : split(spellsTxt.str()).by("\r\n")) {
        const std::vector<std::string_view> tokens = split(line).by('\t');
        int spellId = 0;
        if (!tryDeserialize(trim(tokens[0]), &spellId))
            continue; // Blank lines and school headers.
        if (spellId < 1 || spellId > 99 || tokens.size() < 8 || seen[spellId])
            throw std::runtime_error("MM6 spells.txt has an unexpected spell row");
        seen[spellId] = true;
        for (int rank = 0; rank < 3; rank++)
            if (!tryDeserialize(trim(tokens[5 + rank]), &result[spellId][rank]))
                throw std::runtime_error("MM6 spells.txt has a spell cost that isn't a number");
    }
    for (int spellId = 1; spellId <= 99; spellId++)
        if (!seen[spellId])
            throw std::runtime_error("MM6 spells.txt misses a spell");
    return result;
}

void loadSpellRules(const SpellRules &rules) {
    loadedSpellRules = rules;
}

void loadSpellCosts(const SpellCosts &costs) {
    loadedSpellCosts = costs;
}

SpellManaRecovery spellManaRecovery(SpellId spellId, Mastery mastery, SpellCostSource costSource) {
    const int id = std::to_underlying(spellId);
    const int rank = std::to_underlying(mastery);
    if (id < 1 || id > 99 || rank < 1 || rank > 3)
        throw std::runtime_error("MM6 spell rule lookup has invalid spell ID or mastery");
    if (!loadedSpellRules)
        throw std::runtime_error("MM6 spell rules have not been loaded");
    SpellManaRecovery result = (*loadedSpellRules)[id][rank - 1];
    if (costSource == SPELL_COST_FROM_SPELLS_TXT) {
        if (!loadedSpellCosts)
            throw std::runtime_error("MM6 spell costs from spells.txt have not been loaded");
        result.mana = (*loadedSpellCosts)[id][rank - 1];
    }
    return result;
}

SpellManaRecovery spellManaRecovery(SpellId spellId, Mastery mastery) {
    return spellManaRecovery(spellId, mastery, engine->config->gameplay.FixSpellsCost.value() ? SPELL_COST_FROM_SPELLS_TXT : SPELL_COST_FROM_EXECUTABLE);
}

} // namespace mm6
