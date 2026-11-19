#include <cstdint>
#include <memory>
#include <stdexcept>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Executable.h"
#include "Engine/Spells/MM6FirstAidPreflight.h"
#include "Engine/Spells/MM6FirstAidRules.h"
#include "Engine/Spells/MM6SpellRules.h"

namespace {

void loadSyntheticRules() {
    mm6::SpellRules rules{};
    for (int id = 1; id <= 99; ++id)
        for (int rank = 0; rank < 3; ++rank)
            rules[id][rank] = {static_cast<std::uint16_t>(id + rank), static_cast<std::int16_t>(120 - 20 * rank)};
    mm6::loadSpellRules(rules);
}

mm6::FirstAidPreflightInput ordinaryCast() {
    mm6::FirstAidPreflightInput input;
    input.title = GameTitle::MM6;
    input.route = mm6::FirstAidCastRoute::Spellbook;
    input.spell = SPELL_BODY_FIRST_AID;
    input.mastery = MASTERY_NOVICE;
    input.skillLevel = 1;
    input.casterIndex = 0;
    input.targetCharacterIndex = 1;
    input.availableMana = 500;
    input.spellKnown = true;
    input.casterCanAct = true;
    input.casterInRecovery = false;
    input.casterCursed = false;
    input.partyInvisible = false;
    input.hasHeldItem = false;
    input.turnBasedMode = false;
    input.underwater = false;
    input.targetCanReceiveHealing = true;
    return input;
}


} // namespace

UNIT_TEST(MM6FirstAidPreflight, ThreeRanksProduceReadOnlyPlans) {
    loadSyntheticRules();
    const int healAmounts[] = {5, 7, 10};
    for (int rank = 1; rank <= 3; ++rank) {
        const auto input = [&] {
            auto value = ordinaryCast();
            value.mastery = static_cast<Mastery>(rank);
            return value;
        }();
        const auto plan = mm6::planFirstAidSpellbookCast(input);
        EXPECT_EQ(plan.state, mm6::FirstAidPreflightState::InputsReady);
        EXPECT_EQ(plan.casterIndex, 0);
        EXPECT_EQ(plan.targetCharacterIndex, 1);
        EXPECT_EQ(plan.manaCost, std::to_underlying(SPELL_BODY_FIRST_AID) + rank - 1);
        EXPECT_EQ(plan.recoveryTicks, 140 - 20 * rank);
        EXPECT_EQ(plan.healAmount, healAmounts[rank - 1]);
        EXPECT_EQ(input.availableMana, 500);
        EXPECT_EQ(input.targetCharacterIndex, 1);
    }
}

UNIT_TEST(MM6FirstAidPreflight, MissingTargetAndManaDoNotMutate) {
    loadSyntheticRules();
    auto input = ordinaryCast();
    input.targetCharacterIndex = -1;
    input.targetCanReceiveHealing = false;
    auto plan = mm6::planFirstAidSpellbookCast(input);
    EXPECT_EQ(plan.state, mm6::FirstAidPreflightState::NeedsCharacterTarget);
    EXPECT_EQ(plan.targetCharacterIndex, -1);
    EXPECT_EQ(input.targetCharacterIndex, -1);

    input.availableMana = plan.manaCost - 1;
    plan = mm6::planFirstAidSpellbookCast(input);
    EXPECT_EQ(plan.state, mm6::FirstAidPreflightState::InsufficientMana);
    EXPECT_EQ(input.availableMana, plan.manaCost - 1);
}

UNIT_TEST(MM6FirstAidPreflight, RejectsUnsupportedRoutesAndState) {
    loadSyntheticRules();
    auto input = ordinaryCast();
    input.title = GameTitle::MM7;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.spell = SPELL_BODY_CURE_WEAKNESS;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.route = mm6::FirstAidCastRoute::Quickspell;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.route = mm6::FirstAidCastRoute::Scroll;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.casterIndex = 4;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.targetCharacterIndex = 4;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.mastery = MASTERY_GRANDMASTER;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.hasHeldItem = true;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.casterCursed = true;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.partyInvisible = true;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.turnBasedMode = true;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.spellKnown = false;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
    input = ordinaryCast();
    input.targetCanReceiveHealing = false;
    EXPECT_THROW(mm6::planFirstAidSpellbookCast(input), std::runtime_error);
}

UNIT_TEST(MM6FirstAidPreflight, ExecutableRuleComposition) {
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    mm6::loadSpellRules(mm6::decodeSpellRules(mm6::Executable(data->read("mm6.exe"))));
    for (int rank = 1; rank <= 3; ++rank) {
        auto input = ordinaryCast();
        input.mastery = static_cast<Mastery>(rank);
        const auto plan = mm6::planFirstAidSpellbookCast(input);
        const auto cost = mm6::spellManaRecovery(input.spell, input.mastery);
        EXPECT_EQ(plan.manaCost, cost.mana);
        EXPECT_EQ(plan.recoveryTicks, cost.recoveryTicks);
        EXPECT_EQ(plan.healAmount, mm6::firstAidHealAmount(input.spell, input.mastery));
    }
}
