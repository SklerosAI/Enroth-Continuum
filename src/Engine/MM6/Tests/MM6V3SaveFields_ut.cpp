#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V3SaveFields.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6SpawnedActorSection.h"
#include "Engine/Objects/Actor.h"

namespace {

mm6::V3SaveFields exampleFields() {
    mm6::V3SaveFields fields;
    fields.raw.core.party.playingTicks = 42;
    fields.raw.core.party.activeCharacter = 1;
    for (mm6::CoreCharacterFields &character : fields.raw.core.characters)
        character.name = "Hero";
    fields.raw.inventory.ready = true;
    fields.raw.metadata.header.displayName = "V3 candidate";
    fields.raw.metadata.header.mapFileName = "o0.odm";
    fields.raw.metadata.header.playingTicks = 42;
    fields.raw.metadata.ui.activeCharacter = 1;
    for (int index = 0; index < 52; ++index)
        fields.raw.maps.deltas.push_back({"d" + std::to_string(index) + ".dlv", {1}});
    for (int index = 0; index < 15; ++index)
        fields.raw.maps.deltas.push_back({"o" + std::to_string(index) + ".ddm", {2}});
    fields.raw.chest = {0x12345678, {}, "o0.ddm", {2}};
    fields.raw.npcs.activeCount = 399;
    fields.raw.recovery.playingTicks = 42;
    fields.raw.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    Actor actor(0);
    actor.monsterInfo.id = static_cast<MonsterId>(7);
    actor.monsterId = MONSTER_INVALID;
    actor.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    actor.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    const std::array<Actor, 1> actors{actor};
    fields.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    fields.generatedNpcs.mapName = "o0.odm";
    fields.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    fields.gameplayRng.sequentialState = 12345;
    return fields;
}

mm6::SaveSection &section(mm6::SaveContainer *container, const std::array<char, 4> &tag) {
    auto it = std::find_if(container->sections.begin(), container->sections.end(),
                           [&tag](const mm6::SaveSection &candidate) { return candidate.tag == tag; });
    if (it == container->sections.end())
        throw std::runtime_error("test section absent");
    return *it;
}

} // namespace

UNIT_TEST(MM6V3SaveFields, ExactTwentyTwoSectionRoundTripAndOldVersionRefusal) {
    const auto source = exampleFields();
    const auto bytes = mm6::encodeV3Save(source, GameTitle::MM6);
    const auto container = mm6::decodeSaveContainer(bytes);
    EXPECT_EQ(container.version, 3);
    ASSERT_EQ(container.sections.size(), 22);
    for (const auto &candidate : container.sections)
        EXPECT_TRUE(candidate.required);
    const auto decoded = mm6::decodeV3Save(bytes, GameTitle::MM6);
    EXPECT_EQ(decoded.generatedNpcs.mapName, source.generatedNpcs.mapName);
    EXPECT_EQ(decoded.activeActorPayload, source.activeActorPayload);
    EXPECT_EQ(decoded.gameplayRng, source.gameplayRng);
    EXPECT_EQ(mm6::encodeV3Save(decoded, GameTitle::MM6), bytes);

    const auto oldBytes = mm6::encodeRawSave(source.raw, GameTitle::MM6);
    EXPECT_NO_THROW((void) mm6::decodeRawSave(oldBytes, GameTitle::MM6));
    EXPECT_THROW((void) mm6::decodeV3Save(oldBytes, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeRawSave(bytes, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeV3Save(bytes, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::encodeV3Save(source, GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6V3SaveFields, CanonicalZeroActiveActorsRoundTrip) {
    auto source = exampleFields();
    const std::array<Actor, 0> none{};
    source.activeActorPayload = mm6::encodeActiveActorSection(none, GameTitle::MM6);
    const auto bytes = mm6::encodeV3Save(source, GameTitle::MM6);
    const auto decoded = mm6::decodeV3Save(bytes, GameTitle::MM6);
    EXPECT_EQ(decoded.activeActorPayload, source.activeActorPayload);
    EXPECT_EQ(mm6::encodeV3Save(decoded, GameTitle::MM6), bytes);
}

UNIT_TEST(MM6V3SaveFields, EverySectionRequiredAndExtrasReject) {
    auto base = mm6::decodeSaveContainer(mm6::encodeV3Save(exampleFields(), GameTitle::MM6));
    for (size_t index = 0; index < base.sections.size(); ++index) {
        auto missing = base;
        missing.sections.erase(missing.sections.begin() + index);
        EXPECT_THROW((void) mm6::decodeV3Save(mm6::encodeSaveContainer(missing), GameTitle::MM6),
                     std::exception);
        auto optional = base;
        optional.sections[index].required = false;
        EXPECT_THROW((void) mm6::decodeV3Save(mm6::encodeSaveContainer(optional), GameTitle::MM6),
                     std::exception);
    }
    auto extra = base;
    extra.sections.push_back({{'B', 'E', 'A', 'C'}, false, {1}});
    EXPECT_THROW((void) mm6::decodeV3Save(mm6::encodeSaveContainer(extra), GameTitle::MM6),
                 std::exception);
    auto oldVersion = base;
    oldVersion.version = 2;
    EXPECT_THROW((void) mm6::decodeV3Save(mm6::encodeSaveContainer(oldVersion), GameTitle::MM6),
                 std::exception);
}

UNIT_TEST(MM6V3SaveFields, ActiveActorAndSidecarBoundsReject) {
    auto base = mm6::decodeSaveContainer(mm6::encodeV3Save(exampleFields(), GameTitle::MM6));
    auto wrongVersion = section(&base, {'A', 'C', 'T', 'R'}).payload;
    wrongVersion[4] = 2;
    auto wrongCount = section(&base, {'A', 'C', 'T', 'R'}).payload;
    wrongCount[12] = 2;
    auto wrongFirstSlot = section(&base, {'A', 'C', 'T', 'R'}).payload;
    wrongFirstSlot[8] = 1;
    auto trailing = section(&base, {'A', 'C', 'T', 'R'}).payload;
    trailing.push_back(0);
    for (const std::vector<std::uint8_t> &invalid : {
             std::vector<std::uint8_t>{1, 0, 0}, wrongVersion, wrongCount,
             wrongFirstSlot, trailing}) {
        auto changed = base;
        section(&changed, {'A', 'C', 'T', 'R'}).payload = invalid;
        EXPECT_THROW((void) mm6::decodeV3Save(mm6::encodeSaveContainer(changed), GameTitle::MM6),
                     std::exception);
    }
    auto oldTag = base;
    section(&oldTag, {'A', 'C', 'T', 'R'}).tag = {'S', 'P', 'A', 'W'};
    EXPECT_THROW((void) mm6::decodeV3Save(mm6::encodeSaveContainer(oldTag), GameTitle::MM6),
                 std::exception);
    auto oldPayload = base;
    const auto actors = mm6::decodeActiveActorSection(
        section(&base, {'A', 'C', 'T', 'R'}).payload, GameTitle::MM6);
    section(&oldPayload, {'A', 'C', 'T', 'R'}).payload =
        mm6::encodeSpawnedActorSection(actors, 0, GameTitle::MM6);
    EXPECT_THROW((void) mm6::decodeV3Save(mm6::encodeSaveContainer(oldPayload), GameTitle::MM6),
                 std::exception);
    auto changed = base;
    section(&changed, {'G', 'N', 'P', 'C'}).payload.resize(100000);
    EXPECT_THROW((void) mm6::decodeV3Save(mm6::encodeSaveContainer(changed), GameTitle::MM6),
                 std::exception);
    changed = base;
    section(&changed, {'R', 'N', 'G', 'S'}).payload.resize(8200);
    EXPECT_THROW((void) mm6::decodeV3Save(mm6::encodeSaveContainer(changed), GameTitle::MM6),
                 std::exception);
    auto wrongMap = exampleFields();
    wrongMap.generatedNpcs.mapName = "o1.odm";
    EXPECT_THROW((void) mm6::encodeV3Save(wrongMap, GameTitle::MM6), std::exception);
}
