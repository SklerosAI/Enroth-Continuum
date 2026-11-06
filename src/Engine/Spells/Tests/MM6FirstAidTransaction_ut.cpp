#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6FirstAidTransaction.h"
#include "Engine/Spells/MM6FirstAidRules.h"
#include "Engine/Spells/MM6SpellRules.h"

#include "Utility/Memory/Blob.h"

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

UNIT_TEST(MM6FirstAidTransaction, PrivateRulesPlanAndStageActualCharacterState) {
    const char *spellPath = std::getenv("ENROTH_MM6_SPELL_RULES_MANIFEST");
    const char *firstAidPath = std::getenv("ENROTH_MM6_FIRST_AID_MANIFEST");
    if (!spellPath || !firstAidPath)
        GTEST_SKIP() << "Set ignored MM6 spell and First Aid manifests";

    auto loadFile = [](const char *path, void (*load)(const Blob &)) {
        std::ifstream stream(path);
        ASSERT_TRUE(stream.is_open());
        std::string bytes(std::istreambuf_iterator<char>{stream}, {});
        load(Blob::fromString(std::move(bytes)));
    };
    loadFile(spellPath, mm6::loadSpellRules);
    loadFile(firstAidPath, mm6::loadFirstAidRules);

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
