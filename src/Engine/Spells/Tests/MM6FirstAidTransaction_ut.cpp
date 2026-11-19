#include <array>
#include <memory>
#include <stdexcept>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Executable.h"
#include "Engine/Spells/MM6FirstAidTransaction.h"
#include "Engine/Spells/MM6FirstAidRules.h"
#include "Engine/Spells/MM6SpellRules.h"

UNIT_TEST(MM6FirstAidTransaction, StagesManaHealingAndRecoveryTogether) {
    std::array<Character, 4> characters;
    characters[0].mana = 20;
    characters[1].health = 10;
    const mm6::FirstAidPreflightPlan plan{
        mm6::FirstAidPreflightState::InputsReady, 0, 1, 2, 80, 9};

    const auto change = mm6::stageFirstAidOnCharacters(GameTitle::MM6, plan, &characters, 15);
    EXPECT_EQ(change.casterMana, 18);
    EXPECT_EQ(change.targetHealth, 15);
    EXPECT_EQ(change.casterRecovery, Duration::fromTicks(80));
    EXPECT_EQ(characters[0].mana, 18);
    EXPECT_EQ(characters[1].health, 15);
    EXPECT_EQ(characters[0].timeToRecovery, Duration::fromTicks(80));

    characters[0].timeToRecovery = 0_ticks;
    characters[0].health = 3;
    const mm6::FirstAidPreflightPlan selfPlan{
        mm6::FirstAidPreflightState::InputsReady, 0, 0, 2, 60, 4};
    mm6::stageFirstAidOnCharacters(GameTitle::MM6, selfPlan, &characters, 10);
    EXPECT_EQ(characters[0].mana, 16);
    EXPECT_EQ(characters[0].health, 7);
}

UNIT_TEST(MM6FirstAidTransaction, RejectsChangedStateWithoutMutation) {
    std::array<Character, 4> characters;
    characters[0].mana = 2;
    characters[1].health = 10;
    const mm6::FirstAidPreflightPlan plan{
        mm6::FirstAidPreflightState::InputsReady, 0, 1, 3, 80, 9};
    EXPECT_THROW(mm6::stageFirstAidOnCharacters(GameTitle::MM6, plan, &characters, 20), std::runtime_error);
    EXPECT_EQ(characters[0].mana, 2);
    EXPECT_EQ(characters[1].health, 10);
    EXPECT_EQ(characters[0].timeToRecovery, 0_ticks);

    characters[0].mana = 20;
    EXPECT_THROW(mm6::stageFirstAidOnCharacters(GameTitle::MM7, plan, &characters, 20), std::runtime_error);
    EXPECT_EQ(characters[0].mana, 20);
    EXPECT_EQ(characters[1].health, 10);

    characters[1].conditions.set(CONDITION_DEAD, Time::fromDays(1));
    EXPECT_THROW(mm6::stageFirstAidOnCharacters(GameTitle::MM6, plan, &characters, 20), std::runtime_error);
    EXPECT_EQ(characters[0].mana, 20);
    EXPECT_EQ(characters[1].health, 10);
    EXPECT_EQ(characters[0].timeToRecovery, 0_ticks);
}

UNIT_TEST(MM6FirstAidTransaction, ExecutableRulesPlanAndStageActualCharacterState) {
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    mm6::loadSpellRules(mm6::decodeSpellRules(mm6::Executable(data->read("mm6.exe"))));

    std::array<Character, 4> characters;
    characters[0].mana = 1000;
    characters[1].health = 1;
    mm6::FirstAidPreflightInput input;
    input.title = GameTitle::MM6;
    input.route = mm6::FirstAidCastRoute::Spellbook;
    input.spell = SPELL_BODY_FIRST_AID;
    input.mastery = MASTERY_NOVICE;
    input.skillLevel = 1;
    input.casterIndex = 0;
    input.targetCharacterIndex = 1;
    input.availableMana = characters[0].mana;
    input.spellKnown = true;
    input.casterCanAct = true;
    input.casterInRecovery = false;
    input.casterCursed = false;
    input.partyInvisible = false;
    input.hasHeldItem = false;
    input.turnBasedMode = false;
    input.underwater = false;
    input.targetCanReceiveHealing = true;

    const auto plan = mm6::planFirstAidSpellbookCast(input);
    ASSERT_EQ(plan.state, mm6::FirstAidPreflightState::InputsReady);
    const auto change = mm6::stageFirstAidOnCharacters(GameTitle::MM6, plan, &characters, 1000);
    EXPECT_EQ(change.casterMana, 1000 - plan.manaCost);
    EXPECT_EQ(change.targetHealth, 1 + plan.healAmount);
    EXPECT_EQ(change.casterRecovery, Duration::fromTicks(plan.recoveryTicks));
}
