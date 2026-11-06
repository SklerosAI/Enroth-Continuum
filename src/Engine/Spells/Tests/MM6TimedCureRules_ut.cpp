#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6TimedCureRules.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace {

constexpr std::array<int, 6> kIds = {40, 49, 57, 60, 64, 67};
constexpr std::array<Condition, 6> kConditions = {CONDITION_PETRIFIED, CONDITION_CURSED, CONDITION_FEAR, CONDITION_PARALYZED, CONDITION_INSANE, CONDITION_WEAK};

Json syntheticManifest() {
    return {{"format", 1}, {"spell_report_sha256", std::string(64, 'a')},
            {"guide_report_sha256", std::string(64, 'b')},
            {"ids", {40, 49, 57, 60, 64, 67}},
            {"seconds_per_skill", {120, 480, 1440}}};
}

void load(const Json &document) {
    mm6::loadTimedCureRules(Blob::fromString(document.dump()));
}

}  // namespace

UNIT_TEST(MM6TimedCureRules, SixConditionsAndStrictRankWindows) {
    load(syntheticManifest());
    const Time now = Time::fromDays(10);
    for (size_t i = 0; i < kIds.size(); ++i) {
        const SpellId spell = static_cast<SpellId>(kIds[i]);
        EXPECT_TRUE(mm6::isTimedCureSpell(spell));
        for (int rank = 1; rank <= 3; ++rank) {
            constexpr int skill = 2;
            const Duration window = Duration::fromSeconds(
                syntheticManifest()["seconds_per_skill"][rank - 1].get<int>() * skill);
            CharacterConditions conditions;
            conditions.set(kConditions[i], now - window);
            conditions.set(CONDITION_SLEEP, now - Duration::fromSeconds(1));
            EXPECT_FALSE(mm6::applyTimedCure(spell, static_cast<Mastery>(rank), skill, now, conditions));
            EXPECT_TRUE(conditions.has(kConditions[i]));
            conditions.set(kConditions[i], now - window + Duration::fromTicks(1));
            EXPECT_TRUE(mm6::applyTimedCure(spell, static_cast<Mastery>(rank), skill, now, conditions));
            EXPECT_FALSE(conditions.has(kConditions[i]));
            EXPECT_TRUE(conditions.has(CONDITION_SLEEP));
            EXPECT_FALSE(mm6::applyTimedCure(spell, static_cast<Mastery>(rank), skill, now, conditions));
        }
    }
}

UNIT_TEST(MM6TimedCureRules, RejectsUnsupportedInputsBeforeMutation) {
    load(syntheticManifest());
    const Time now = Time::fromDays(10);
    CharacterConditions conditions;
    conditions.set(CONDITION_WEAK, now - Duration::fromSeconds(1));
    EXPECT_FALSE(mm6::isTimedCureSpell(SPELL_BODY_FIRST_AID));
    EXPECT_THROW(mm6::applyTimedCure(SPELL_BODY_FIRST_AID, MASTERY_NOVICE, 1, now, conditions),
                 std::runtime_error);
    EXPECT_THROW(mm6::applyTimedCure(static_cast<SpellId>(67), MASTERY_GRANDMASTER, 1, now, conditions),
                 std::runtime_error);
    EXPECT_THROW(mm6::applyTimedCure(static_cast<SpellId>(67), MASTERY_NOVICE, 0, now, conditions),
                 std::runtime_error);
    EXPECT_THROW(mm6::applyTimedCure(static_cast<SpellId>(67), MASTERY_NOVICE, 64, now, conditions),
                 std::runtime_error);
    EXPECT_TRUE(conditions.has(CONDITION_WEAK));
    conditions.set(CONDITION_WEAK, now + Duration::fromSeconds(1));
    EXPECT_THROW(mm6::applyTimedCure(static_cast<SpellId>(67), MASTERY_NOVICE, 1, now, conditions),
                 std::runtime_error);
    EXPECT_TRUE(conditions.has(CONDITION_WEAK));
}

UNIT_TEST(MM6TimedCureRules, MalformedReloadRetainsGoodWindows) {
    load(syntheticManifest());
    Json document = syntheticManifest();
    document["ids"][2] = 56;
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["seconds_per_skill"] = {480, 120, 1440};
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["seconds_per_skill"] = {120, 480, 604801};
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["guide_report_sha256"] = "invalid";
    EXPECT_THROW(load(document), std::runtime_error);
    CharacterConditions conditions;
    const Time now = Time::fromDays(10);
    conditions.set(CONDITION_WEAK, now - Duration::fromSeconds(119));
    EXPECT_TRUE(mm6::applyTimedCure(static_cast<SpellId>(67), MASTERY_NOVICE, 1, now, conditions));
}

UNIT_TEST(MM6TimedCureRules, PrivateF3MatchedManifest) {
    const char *path = std::getenv("ENROTH_MM6_TIMED_CURE_MANIFEST");
    if (!path)
        GTEST_SKIP() << "Set ignored MM6 rules/timed-cures.json manifest path";
    std::ifstream stream(path);
    ASSERT_TRUE(stream.good());
    std::string bytes(std::istreambuf_iterator<char>{stream}, {});
    Json document = Json::parse(bytes);
    mm6::loadTimedCureRules(Blob::fromString(bytes));
    const Time now = Time::fromDays(10);
    for (size_t i = 0; i < kIds.size(); ++i)
        for (int rank = 1; rank <= 3; ++rank) {
            const int seconds = document["seconds_per_skill"][rank - 1].get<int>();
            CharacterConditions conditions;
            const SpellId spell = static_cast<SpellId>(kIds[i]);
            conditions.set(kConditions[i], now - Duration::fromSeconds(seconds));
            EXPECT_FALSE(mm6::applyTimedCure(spell, static_cast<Mastery>(rank), 1, now, conditions));
            conditions.set(kConditions[i], now - Duration::fromSeconds(seconds) + Duration::fromTicks(1));
            EXPECT_TRUE(mm6::applyTimedCure(spell, static_cast<Mastery>(rank), 1, now, conditions));
        }
}
