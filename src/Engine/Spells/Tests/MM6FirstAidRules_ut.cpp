#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6FirstAidRules.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace {

Json syntheticManifest() {
    return {{"format", 1}, {"id", std::to_underlying(SPELL_BODY_FIRST_AID)},
            {"spell_report_sha256", std::string(64, 'a')},
            {"guide_report_sha256", std::string(64, 'b')},
            {"heal", {4, 6, 9}}};
}

void load(const Json &document) {
    mm6::loadFirstAidRules(Blob::fromString(document.dump()));
}

}  // namespace

UNIT_TEST(MM6FirstAidRules, ThreeRanksAndOnlyReviewedSpell) {
    load(syntheticManifest());
    EXPECT_EQ(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_NOVICE), 4);
    EXPECT_EQ(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_EXPERT), 6);
    EXPECT_EQ(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_MASTER), 9);
    EXPECT_THROW(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_NONE), std::runtime_error);
    EXPECT_THROW(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_GRANDMASTER), std::runtime_error);
    EXPECT_THROW(mm6::firstAidHealAmount(SPELL_BODY_CURE_WEAKNESS, MASTERY_NOVICE), std::runtime_error);
}

UNIT_TEST(MM6FirstAidRules, RejectsMalformedReloadWithoutReplacingValidAmounts) {
    load(syntheticManifest());
    Json document = syntheticManifest();
    document["id"] = std::to_underlying(SPELL_BODY_CURE_WEAKNESS);
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["heal"] = {1, 2};
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["heal"] = {10, 9, 8};
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["heal"] = {0, 6, 9};
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["heal"] = {4, 6, 1001};
    EXPECT_THROW(load(document), std::runtime_error);
    document = syntheticManifest();
    document["guide_report_sha256"] = "invalid";
    EXPECT_THROW(load(document), std::runtime_error);
    EXPECT_EQ(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_MASTER), 9);
}

UNIT_TEST(MM6FirstAidRules, PrivateF3MatchedManifest) {
    const char *path = std::getenv("ENROTH_MM6_FIRST_AID_MANIFEST");
    if (!path)
        GTEST_SKIP() << "Set ignored MM6 rules/first-aid.json manifest path";
    std::ifstream stream(path);
    ASSERT_TRUE(stream.good());
    std::string bytes(std::istreambuf_iterator<char>{stream}, {});
    Json document = Json::parse(bytes);
    mm6::loadFirstAidRules(Blob::fromString(bytes));
    for (int rank = 1; rank <= 3; ++rank)
        EXPECT_EQ(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, static_cast<Mastery>(rank)),
                  document["heal"][rank - 1].get<int>());
}
