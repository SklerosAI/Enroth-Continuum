#include <cstdint>
#include <memory>
#include <stdexcept>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Executable.h"
#include "Engine/Spells/MM6SpellRules.h"

UNIT_TEST(MM6SpellRules, AllIdsAndThreeRanks) {
    mm6::SpellRules rules{};
    for (int id = 1; id <= 99; ++id)
        for (int rank = 0; rank < 3; ++rank)
            rules[id][rank] = {static_cast<std::uint16_t>(id + rank), static_cast<std::int16_t>(120 - 20 * rank)};
    mm6::loadSpellRules(rules);
    for (int id = 1; id <= 99; ++id) {
        const SpellId spell = static_cast<SpellId>(id);
        for (int rank = 1; rank <= 3; ++rank) {
            const mm6::SpellManaRecovery rule = mm6::spellManaRecovery(spell, static_cast<Mastery>(rank));
            EXPECT_EQ(rule.mana, id + rank - 1);
            EXPECT_EQ(rule.recoveryTicks, 140 - 20 * rank);
        }
    }
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_NONE, MASTERY_NOVICE), std::runtime_error);
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_BOW_ARROW, MASTERY_NOVICE), std::runtime_error);
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_FIRE_TORCH_LIGHT, MASTERY_NONE), std::runtime_error);
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_FIRE_TORCH_LIGHT, MASTERY_GRANDMASTER), std::runtime_error);
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
