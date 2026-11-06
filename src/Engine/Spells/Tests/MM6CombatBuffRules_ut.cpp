#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6CombatBuffRules.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace {

Json syntheticManifest() {
    return {{"format", 1}, {"spell_report_sha256", std::string(64, 'a')},
            {"guide_report_sha256", std::string(64, 'b')}, {"ids", {17, 51}},
            {"base_seconds", 600}, {"per_skill_seconds", {10, 10, 30}},
            {"heroism_power", {7, 2}}};
}

void load(const Json &document) {
    mm6::loadCombatBuffRules(Blob::fromString(document.dump()));
}

}  // namespace

UNIT_TEST(MM6CombatBuffRules, TwoBuffsThreeRanksAndPartyScope) {
    load(syntheticManifest());
    const Time now = Time::fromDays(10);
    for (SpellId spell : {SPELL_AIR_SHIELD, SPELL_SPIRIT_HEROISM}) {
        EXPECT_TRUE(mm6::isCombatBuffSpell(spell));
        for (int rank = 1; rank <= 3; ++rank) {
            const auto plan = mm6::planCombatBuff(spell, static_cast<Mastery>(rank), 3, now);
            EXPECT_EQ(plan.partyWide, rank >= 2);
            EXPECT_EQ(plan.expires, now + Duration::fromSeconds(600 + (rank == 3 ? 30 : 10) * 3));
            EXPECT_EQ(plan.buff, spell == SPELL_AIR_SHIELD ? CHARACTER_BUFF_SHIELD : CHARACTER_BUFF_HEROISM);
            EXPECT_EQ(plan.power, spell == SPELL_AIR_SHIELD ? 0 : 13);
            SpellBuff slot;
            EXPECT_TRUE(mm6::applyCombatBuff(slot, plan, 2));
            EXPECT_EQ(slot.expireTime, plan.expires);
            EXPECT_EQ(slot.power, plan.power);
            EXPECT_EQ(slot.skillMastery, plan.mastery);
            EXPECT_EQ(slot.caster, 2);
            auto shorter = plan;
            shorter.expires -= Duration::fromTicks(1);
            EXPECT_FALSE(mm6::applyCombatBuff(slot, shorter, 2));
        }
    }
}

UNIT_TEST(MM6CombatBuffRules, RejectsUnsupportedInputsBeforeBuffMutation) {
    load(syntheticManifest());
    const Time now = Time::fromDays(10);
    EXPECT_FALSE(mm6::isCombatBuffSpell(SPELL_SPIRIT_BLESS));
    EXPECT_THROW(mm6::planCombatBuff(SPELL_SPIRIT_BLESS, MASTERY_NOVICE, 1, now), std::runtime_error);
    EXPECT_THROW(mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NONE, 1, now), std::runtime_error);
    EXPECT_THROW(mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_GRANDMASTER, 1, now), std::runtime_error);
    EXPECT_THROW(mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NOVICE, 0, now), std::runtime_error);
    EXPECT_THROW(mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NOVICE, 64, now), std::runtime_error);
    SpellBuff slot;
    auto plan = mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NOVICE, 1, now);
    plan.power = 1;
    EXPECT_THROW(mm6::applyCombatBuff(slot, plan, 0), std::runtime_error);
    EXPECT_FALSE(slot.Active());
    plan = mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NOVICE, 1, now);
    EXPECT_THROW(mm6::applyCombatBuff(slot, plan, 4), std::runtime_error);
    EXPECT_FALSE(slot.Active());
}

UNIT_TEST(MM6CombatBuffRules, InvalidReloadPreservesGoodRule) {
    load(syntheticManifest());
    Json document = syntheticManifest();
    document["ids"][1] = 50;
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["per_skill_seconds"] = {30, 10, 30};
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["heroism_power"] = {7, 0};
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["guide_report_sha256"] = "invalid";
    EXPECT_THROW(load(document), std::runtime_error);
    EXPECT_EQ(mm6::planCombatBuff(SPELL_SPIRIT_HEROISM, MASTERY_MASTER, 3, Time::fromDays(10)).power, 13);
}

UNIT_TEST(MM6CombatBuffRules, PrivateF3MatchedManifest) {
    const char *path = std::getenv("ENROTH_MM6_COMBAT_BUFF_MANIFEST");
    if (!path)
        GTEST_SKIP() << "Set ignored MM6 rules/combat-buffs.json manifest path";
    std::ifstream stream(path);
    ASSERT_TRUE(stream.good());
    std::string bytes(std::istreambuf_iterator<char>{stream}, {});
    Json document = Json::parse(bytes);
    mm6::loadCombatBuffRules(Blob::fromString(bytes));
    const Time now = Time::fromDays(10);
    for (SpellId spell : {SPELL_AIR_SHIELD, SPELL_SPIRIT_HEROISM})
        for (int rank = 1; rank <= 3; ++rank) {
            const auto plan = mm6::planCombatBuff(spell, static_cast<Mastery>(rank), 7, now);
            EXPECT_EQ(plan.expires, now + Duration::fromSeconds(document["base_seconds"].get<int>() +
                document["per_skill_seconds"][rank - 1].get<int>() * 7));
            EXPECT_EQ(plan.power, spell == SPELL_AIR_SHIELD ? 0 :
                document["heroism_power"][0].get<int>() + document["heroism_power"][1].get<int>() * 7);
        }
}
