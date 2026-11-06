#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6FirstAidPreflight.h"
#include "Engine/Spells/MM6FirstAidRules.h"
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

    const Json firstAidRules = {{"format", 1}, {"id", std::to_underlying(SPELL_BODY_FIRST_AID)},
                                {"spell_report_sha256", std::string(64, 'a')},
                                {"guide_report_sha256", std::string(64, 'b')},
                                {"heal", {4, 6, 9}}};
    mm6::loadFirstAidRules(Blob::fromString(firstAidRules.dump()));
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

void loadFile(const char *path, void (*load)(const Blob &)) {
    std::ifstream stream(path);
    if (!stream.good())
        throw std::runtime_error("Cannot read ignored MM6 rule manifest");
    std::string bytes(std::istreambuf_iterator<char>{stream}, {});
    load(Blob::fromString(bytes));
}

} // namespace

UNIT_TEST(MM6FirstAidPreflight, ThreeRanksProduceReadOnlyPlans) {
    loadSyntheticRules();
    const int healAmounts[] = {4, 6, 9};
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

UNIT_TEST(MM6FirstAidPreflight, PrivateF3MatchedRuleComposition) {
    const char *spellPath = std::getenv("ENROTH_MM6_SPELL_RULES_MANIFEST");
    const char *firstAidPath = std::getenv("ENROTH_MM6_FIRST_AID_MANIFEST");
    if (!spellPath || !firstAidPath)
        GTEST_SKIP() << "Set ignored MM6 spell and First Aid manifests";
    loadFile(spellPath, mm6::loadSpellRules);
    loadFile(firstAidPath, mm6::loadFirstAidRules);
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
