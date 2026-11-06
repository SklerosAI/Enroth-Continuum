#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6PowerCureTransaction.h"
#include "Engine/Spells/MM6PowerCureRules.h"
#include "Engine/Spells/MM6SpellRules.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace {

void loadSyntheticRules() {
    Json spellRules;
    spellRules["format"] = 1;
    spellRules["source_report_sha256"] = std::string(64, 'a');
    spellRules["spells"] = Json::array();
    for (int id = 1; id <= 99; ++id)
        spellRules["spells"].push_back({{"id", id}, {"mana", {id, id + 1, id + 2}},
                                         {"recovery", {120, 100, 80}}});
    mm6::loadSpellRules(Blob::fromString(spellRules.dump()));
    const Json powerCureRules = {{"format", 1}, {"id", std::to_underlying(SPELL_BODY_POWER_CURE)},
                                 {"spell_report_sha256", std::string(64, 'a')},
                                 {"guide_report_sha256", std::string(64, 'b')},
                                 {"base", 11}, {"per_skill", 3}};
    mm6::loadPowerCureRules(Blob::fromString(powerCureRules.dump()));
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

void loadFile(const char *path, void (*load)(const Blob &)) {
    std::ifstream stream(path);
    if (!stream.good())
        throw std::runtime_error("Cannot read ignored MM6 spell rule manifest");
    std::string bytes(std::istreambuf_iterator<char>{stream}, {});
    load(Blob::fromString(std::move(bytes)));
}

}  // namespace

UNIT_TEST(MM6PowerCureTransaction, StagesAllFourCappedHealsWithManaAndRecovery) {
    loadSyntheticRules();
    auto characters = party();
    const auto change = mm6::stagePowerCureOnCharacters(ordinaryCast(), &characters);
    EXPECT_EQ(change.casterMana, 23);
    EXPECT_EQ(change.casterRecovery, Duration::fromTicks(120));
    EXPECT_EQ(change.health, (std::array<int, 4>{20, 25, 50, 10}));
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

UNIT_TEST(MM6PowerCureTransaction, PrivateRulesStageActualCharacterState) {
    const char *spellPath = std::getenv("ENROTH_MM6_SPELL_RULES_MANIFEST");
    const char *powerCurePath = std::getenv("ENROTH_MM6_POWER_CURE_MANIFEST");
    if (!spellPath || !powerCurePath)
        GTEST_SKIP() << "Set ignored MM6 spell and Power Cure manifests";
    loadFile(spellPath, mm6::loadSpellRules);
    loadFile(powerCurePath, mm6::loadPowerCureRules);

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
