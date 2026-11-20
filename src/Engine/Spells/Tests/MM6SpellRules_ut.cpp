#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Executable.h"
#include "Engine/Spells/MM6SpellRules.h"

UNIT_TEST(MM6SpellRules, AllIdsAndThreeRanks) {
    mm6::SpellRules rules{};
    mm6::SpellCosts costs{};
    for (int id = 1; id <= 99; ++id) {
        for (int rank = 0; rank < 3; ++rank) {
            rules[id][rank] = {static_cast<std::uint16_t>(id + rank), static_cast<std::int16_t>(120 - 20 * rank)};
            costs[id][rank] = static_cast<std::uint16_t>(200 + id + rank);
        }
    }
    mm6::loadSpellRules(rules);
    mm6::loadSpellCosts(costs);
    for (int id = 1; id <= 99; ++id) {
        const SpellId spell = static_cast<SpellId>(id);
        for (int rank = 1; rank <= 3; ++rank) {
            const mm6::SpellManaRecovery exeRule = mm6::spellManaRecovery(spell, static_cast<Mastery>(rank), mm6::SPELL_COST_FROM_EXECUTABLE);
            const mm6::SpellManaRecovery txtRule = mm6::spellManaRecovery(spell, static_cast<Mastery>(rank), mm6::SPELL_COST_FROM_SPELLS_TXT);
            EXPECT_EQ(exeRule.mana, id + rank - 1);
            EXPECT_EQ(txtRule.mana, 200 + id + rank - 1);
            EXPECT_EQ(exeRule.recoveryTicks, 140 - 20 * rank);
            EXPECT_EQ(txtRule.recoveryTicks, 140 - 20 * rank);
        }
    }
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_NONE, MASTERY_NOVICE, mm6::SPELL_COST_FROM_EXECUTABLE), std::runtime_error);
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_BOW_ARROW, MASTERY_NOVICE, mm6::SPELL_COST_FROM_EXECUTABLE), std::runtime_error);
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_FIRE_TORCH_LIGHT, MASTERY_NONE, mm6::SPELL_COST_FROM_EXECUTABLE), std::runtime_error);
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_FIRE_TORCH_LIGHT, MASTERY_GRANDMASTER, mm6::SPELL_COST_FROM_SPELLS_TXT), std::runtime_error);
}

UNIT_TEST(MM6SpellRules, ExecutableTable) {
    // Every MM6 spell costs something at Normal and recovers no faster as mastery rises.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    const mm6::SpellRules rules = mm6::decodeSpellRules(mm6::Executable(data->read("mm6.exe")));
    for (int id = 1; id <= 99; ++id) {
        SCOPED_TRACE(id);
        EXPECT_GT(rules[id][0].mana, 0);
        EXPECT_GT(rules[id][2].recoveryTicks, 0);
        EXPECT_GE(rules[id][0].recoveryTicks, rules[id][1].recoveryTicks);
        EXPECT_GE(rules[id][1].recoveryTicks, rules[id][2].recoveryTicks);
    }
    EXPECT_EQ(rules[1].front().mana, 1); // Torch Light.
    EXPECT_EQ(rules[1].front().recoveryTicks, 60);
}

UNIT_TEST(MM6SpellRules, SpellsTxtCosts) {
    // spells.txt and MM6.exe's table agree on every cost but Expert Static Charge.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    const mm6::SpellRules rules = mm6::decodeSpellRules(mm6::Executable(data->read("mm6.exe")));
    const mm6::SpellCosts costs = mm6::decodeSpellCosts(data->iconsData("spells.txt"));
    std::vector<std::tuple<int, int, int, int>> differences;
    for (int id = 1; id <= 99; ++id)
        for (int rank = 0; rank < 3; ++rank)
            if (rules[id][rank].mana != costs[id][rank])
                differences.emplace_back(id, rank, rules[id][rank].mana, costs[id][rank]);
    EXPECT_EQ(differences, (std::vector<std::tuple<int, int, int, int>>{{13, 1, 2, 1}}));
    EXPECT_EQ(costs[13], (std::array<std::uint16_t, 3>{2, 1, 0})); // Static Charge.
    EXPECT_EQ(costs[99], (std::array<std::uint16_t, 3>{200, 200, 200})); // Dark Containment.
}
