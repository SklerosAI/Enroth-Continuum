#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6PowerCureRules.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace {

Json syntheticManifest() {
    return {{"format", 1}, {"id", std::to_underlying(SPELL_BODY_POWER_CURE)},
            {"spell_report_sha256", std::string(64, 'a')},
            {"guide_report_sha256", std::string(64, 'b')},
            {"base", 11}, {"per_skill", 3}};
}

void load(const Json &document) {
    mm6::loadPowerCureRules(Blob::fromString(document.dump()));
}

}  // namespace

UNIT_TEST(MM6PowerCureRules, ThreeRanksAndSkillBounds) {
    load(syntheticManifest());
    for (Mastery rank : {MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER}) {
        EXPECT_EQ(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, rank, 1), 14);
        EXPECT_EQ(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, rank, 63), 200);
    }
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_NONE, 1), std::runtime_error);
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_GRANDMASTER, 1), std::runtime_error);
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_NOVICE, 0), std::runtime_error);
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_NOVICE, 64), std::runtime_error);
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_FIRST_AID, MASTERY_NOVICE, 1), std::runtime_error);
}

UNIT_TEST(MM6PowerCureRules, MalformedReloadRetainsGoodRule) {
    load(syntheticManifest());
    Json document = syntheticManifest();
    document["id"] = std::to_underlying(SPELL_BODY_FIRST_AID);
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["base"] = 0;
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["per_skill"] = 101;
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["guide_report_sha256"] = "invalid";
    EXPECT_THROW(load(document), std::runtime_error);
    EXPECT_EQ(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_MASTER, 7), 32);
}

UNIT_TEST(MM6PowerCureRules, PrivateF3MatchedManifest) {
    const char *path = std::getenv("ENROTH_MM6_POWER_CURE_MANIFEST");
    if (!path)
        GTEST_SKIP() << "Set ignored MM6 rules/power-cure.json manifest path";
    std::ifstream stream(path);
    ASSERT_TRUE(stream.good());
    std::string bytes(std::istreambuf_iterator<char>{stream}, {});
    Json document = Json::parse(bytes);
    mm6::loadPowerCureRules(Blob::fromString(bytes));
    for (int rank = 1; rank <= 3; ++rank)
        for (int skill : {1, 7, 63})
            EXPECT_EQ(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, static_cast<Mastery>(rank), skill),
                      document["base"].get<int>() + document["per_skill"].get<int>() * skill);
}
