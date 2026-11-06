#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6SpellRules.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace {

Json syntheticManifest() {
    Json document;
    document["format"] = 1;
    document["source_report_sha256"] = std::string(64, 'a');
    document["spells"] = Json::array();
    for (int id = 1; id <= 99; ++id)
        document["spells"].push_back({{"id", id}, {"mana", {id, id + 1, id + 2}},
                                       {"recovery", {120, 100, 80}}});
    return document;
}

void load(const Json &document) {
    mm6::loadSpellRules(Blob::fromString(document.dump()));
}

} // namespace

UNIT_TEST(MM6SpellRules, AllIdsAndThreeRanks) {
    load(syntheticManifest());
    for (int id = 1; id <= 99; ++id) {
        const SpellId spell = static_cast<SpellId>(id);
        for (int rank = 1; rank <= 3; ++rank) {
            const mm6::SpellManaRecovery rule = mm6::spellManaRecovery(spell, static_cast<Mastery>(rank));
            EXPECT_EQ(rule.mana, id + rank - 1);
            EXPECT_EQ(rule.recoveryTicks, 140 - 20 * rank);
        }
    }
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_NONE, MASTERY_NOVICE), std::runtime_error);
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_BOW_ARROW, MASTERY_NOVICE), std::runtime_error);
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_FIRE_TORCH_LIGHT, MASTERY_NONE), std::runtime_error);
    EXPECT_THROW(mm6::spellManaRecovery(SPELL_FIRE_TORCH_LIGHT, MASTERY_GRANDMASTER), std::runtime_error);
}

UNIT_TEST(MM6SpellRules, InvalidManifestCannotReplaceLoadedRules) {
    Json manifest = syntheticManifest();
    load(manifest);
    const auto before = mm6::spellManaRecovery(SPELL_DARK_SOULDRINKER, MASTERY_MASTER);

    manifest["spells"][98]["id"] = 98;
    EXPECT_THROW(load(manifest), std::runtime_error);
    manifest = syntheticManifest();
    manifest["spells"][0]["recovery"] = {80, 100, 120};
    EXPECT_THROW(load(manifest), std::runtime_error);
    manifest = syntheticManifest();
    manifest["spells"][0]["mana"] = {-1, 1, 1};
    EXPECT_THROW(load(manifest), std::runtime_error);
    manifest = syntheticManifest();
    manifest["spells"][0]["mana"] = {32768, 1, 1};
    EXPECT_THROW(load(manifest), std::runtime_error);
    EXPECT_EQ(mm6::spellManaRecovery(SPELL_DARK_SOULDRINKER, MASTERY_MASTER).mana, before.mana);
    EXPECT_EQ(mm6::spellManaRecovery(SPELL_DARK_SOULDRINKER, MASTERY_MASTER).recoveryTicks,
              before.recoveryTicks);
}

UNIT_TEST(MM6SpellRules, PrivateF3MatchedManifest) {
    const char *path = std::getenv("ENROTH_MM6_SPELL_RULES_MANIFEST");
    if (!path)
        GTEST_SKIP() << "Set ignored MM6 rules/spells.json manifest path";
    std::ifstream stream(path);
    ASSERT_TRUE(stream.good());
    std::string bytes(std::istreambuf_iterator<char>{stream}, {});
    Json document = Json::parse(bytes);
    mm6::loadSpellRules(Blob::fromString(bytes));
    ASSERT_EQ(document["spells"].size(), 99);
    for (int id = 1; id <= 99; ++id)
        for (int rank = 1; rank <= 3; ++rank) {
            auto actual = mm6::spellManaRecovery(static_cast<SpellId>(id), static_cast<Mastery>(rank));
            EXPECT_EQ(actual.mana, document["spells"][id - 1]["mana"][rank - 1].get<int>());
            EXPECT_EQ(actual.recoveryTicks, document["spells"][id - 1]["recovery"][rank - 1].get<int>());
        }
}
