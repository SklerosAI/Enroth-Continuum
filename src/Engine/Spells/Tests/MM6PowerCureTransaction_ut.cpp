#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Executable.h"
#include "Engine/Spells/MM6PowerCureTransaction.h"
#include "Engine/Spells/MM6PowerCureRules.h"
#include "Engine/Spells/MM6SpellRules.h"

namespace {

void loadSyntheticRules() {
    mm6::SpellRules rules{};
    for (int id = 1; id <= 99; ++id)
        for (int rank = 0; rank < 3; ++rank)
            rules[id][rank] = {static_cast<std::uint16_t>(id + rank), static_cast<std::int16_t>(120 - 20 * rank)};
    mm6::loadSpellRules(rules);
}

mm6::PowerCureTransactionInput ordinaryCast() {
    mm6::PowerCureTransactionInput input;
    input.title = GameTitle::MM6;
    input.route = mm6::PowerCureCastRoute::Spellbook;
    input.spell = SPELL_BODY_POWER_CURE;
    input.mastery = MASTERY_NOVICE;
    input.skillLevel = 2;
    input.casterIndex = 0;
    input.expectedCasterMana = 100;
    input.spellKnown = true;
    input.partyInvisible = false;
    input.hasHeldItem = false;
    input.turnBasedMode = false;
    input.underwater = false;
    input.hasActorTarget = false;
    input.maxHealth = {20, 25, 50, 10};
    return input;
}

std::array<Character, 4> party() {
    std::array<Character, 4> characters;
    const std::array<int, 4> health{5, 19, 50, 1};
    for (int i = 0; i < 4; ++i)
        characters[i].health = health[i];
    characters[0].mana = 100;
    return characters;
}


}  // namespace

UNIT_TEST(MM6PowerCureTransaction, StagesAllFourCappedHealsWithManaAndRecovery) {
    loadSyntheticRules();
    auto characters = party();
    const auto change = mm6::stagePowerCureOnCharacters(ordinaryCast(), &characters);
    EXPECT_EQ(change.casterMana, 23);
    EXPECT_EQ(change.casterRecovery, Duration::fromTicks(120));
    EXPECT_EQ(change.health, (std::array<int, 4>{19, 25, 50, 10}));
    EXPECT_EQ(characters[0].mana, 23);
    EXPECT_EQ(characters[0].timeToRecovery, Duration::fromTicks(120));
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(characters[i].health, change.health[i]);
}

UNIT_TEST(MM6PowerCureTransaction, RejectsChangedTargetBeforeAnyMutation) {
    loadSyntheticRules();
    auto characters = party();
    characters[3].conditions.set(CONDITION_ZOMBIE, Time::fromDays(1));
    EXPECT_THROW(mm6::stagePowerCureOnCharacters(ordinaryCast(), &characters), std::runtime_error);
    EXPECT_EQ(characters[0].mana, 100);
    EXPECT_EQ(characters[0].timeToRecovery, 0_ticks);
    EXPECT_EQ(characters[0].health, 5);
    EXPECT_EQ(characters[1].health, 19);
    EXPECT_EQ(characters[2].health, 50);
    EXPECT_EQ(characters[3].health, 1);

    characters[3].conditions.reset(CONDITION_ZOMBIE);
    auto input = ordinaryCast();
    input.maxHealth[3] = 0;
    EXPECT_THROW(mm6::stagePowerCureOnCharacters(input, &characters), std::runtime_error);
    EXPECT_EQ(characters[0].mana, 100);
    EXPECT_EQ(characters[1].health, 19);
}

UNIT_TEST(MM6PowerCureTransaction, RejectsMM7AndChangedCasterBeforeAnyMutation) {
    loadSyntheticRules();
    auto characters = party();
    auto input = ordinaryCast();
    input.title = GameTitle::MM7;
    EXPECT_THROW(mm6::stagePowerCureOnCharacters(input, &characters), std::runtime_error);
    input = ordinaryCast();
    characters[0].mana = 99;
    EXPECT_THROW(mm6::stagePowerCureOnCharacters(input, &characters), std::runtime_error);
    characters[0].mana = 100;
    characters[0].timeToRecovery = Duration::fromTicks(1);
    EXPECT_THROW(mm6::stagePowerCureOnCharacters(input, &characters), std::runtime_error);
    EXPECT_EQ(characters[0].mana, 100);
    EXPECT_EQ(characters[0].health, 5);
    EXPECT_EQ(characters[1].health, 19);
    EXPECT_EQ(characters[0].timeToRecovery, Duration::fromTicks(1));
}

UNIT_TEST(MM6PowerCureTransaction, ExecutableRulesStageActualCharacterState) {
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    mm6::loadSpellRules(mm6::decodeSpellRules(mm6::Executable(data->read("mm6.exe"))));

    auto characters = party();
    characters[0].mana = 1000;
    for (Character &character : characters)
        character.health = 1;
    auto input = ordinaryCast();
    input.mastery = MASTERY_EXPERT;
    input.skillLevel = 7;
    input.expectedCasterMana = 1000;
    input.maxHealth = {1000, 1000, 1000, 1000};
    const auto rule = mm6::spellManaRecovery(SPELL_BODY_POWER_CURE, MASTERY_EXPERT);
    const int heal = mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_EXPERT, 7);
    const auto change = mm6::stagePowerCureOnCharacters(input, &characters);
    EXPECT_EQ(change.casterMana, 1000 - rule.mana);
    EXPECT_EQ(change.casterRecovery, Duration::fromTicks(rule.recoveryTicks));
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(characters[i].health, std::min(1000, 1 + heal));
}
