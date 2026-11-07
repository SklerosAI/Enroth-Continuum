#include <algorithm>
#include <array>
#include <exception>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6V4SaveFields.h"
#include "Engine/Objects/Actor.h"

namespace {

mm6::V4SaveFields exampleFields() {
    mm6::V4SaveFields fields;
    auto &v3 = fields.v3;
    v3.raw.core.party.playingTicks = 42;
    v3.raw.core.party.activeCharacter = 1;
    for (mm6::CoreCharacterFields &character : v3.raw.core.characters)
        character.name = "Hero";
    v3.raw.inventory.ready = true;
    v3.raw.metadata.header.displayName = "V4 candidate";
    v3.raw.metadata.header.mapFileName = "o0.odm";
    v3.raw.metadata.header.playingTicks = 42;
    v3.raw.metadata.ui.activeCharacter = 1;
    for (int index = 0; index < 52; ++index)
        v3.raw.maps.deltas.push_back({"d" + std::to_string(index) + ".dlv", {1}});
    for (int index = 0; index < 15; ++index)
        v3.raw.maps.deltas.push_back({"o" + std::to_string(index) + ".ddm", {2}});
    v3.raw.chest = {0x12345678, {}, "o0.ddm", {2}};
    v3.raw.npcs.activeCount = 399;
    v3.raw.recovery.playingTicks = 42;
    v3.raw.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    const std::array<Actor, 0> actors{};
    v3.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    v3.generatedNpcs.mapName = "o0.odm";
    v3.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    v3.gameplayRng.sequentialState = 12345;
    mm6::EventRuntimeMapEntry events;
    events.mapName = "o0.odm";
    events.respawnCount = 1;
    events.lastRespawnDay = 5;
    events.decorVars[0] = 32;
    fields.eventRuntime.maps.push_back(events);
    return fields;
}

} // namespace

UNIT_TEST(MM6V4SaveFields, ExactTwentyThreeSectionRoundTripAndV3Isolation) {
    const auto source = exampleFields();
    const auto bytes = mm6::encodeV4Save(source, GameTitle::MM6);
    const auto container = mm6::decodeSaveContainer(bytes);
    EXPECT_EQ(container.version, 4);
    ASSERT_EQ(container.sections.size(), 23);
    for (const auto &section : container.sections)
        EXPECT_TRUE(section.required);
    const auto decoded = mm6::decodeV4Save(bytes, GameTitle::MM6);
    EXPECT_EQ(decoded.eventRuntime, source.eventRuntime);
    EXPECT_EQ(mm6::encodeV4Save(decoded, GameTitle::MM6), bytes);
    EXPECT_THROW((void) mm6::decodeV3Save(bytes, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeV4Save(
        mm6::encodeV3Save(source.v3, GameTitle::MM6), GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeV4Save(bytes, GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6V4SaveFields, RejectsMissingExtraOrForeignEventMap) {
    auto source = exampleFields();
    source.eventRuntime.maps.clear();
    EXPECT_THROW((void) mm6::encodeV4Save(source, GameTitle::MM6), std::exception);
    source = exampleFields();
    source.eventRuntime.maps[0].mapName = "o1.odm";
    EXPECT_THROW((void) mm6::encodeV4Save(source, GameTitle::MM6), std::exception);
    source = exampleFields();
    source.eventRuntime.maps.push_back({.mapName = "o99.odm", .lastRespawnDay = 5});
    EXPECT_THROW((void) mm6::encodeV4Save(source, GameTitle::MM6), std::exception);
    source = exampleFields();
    auto container = mm6::decodeSaveContainer(mm6::encodeV4Save(source, GameTitle::MM6));
    container.sections.back().required = false;
    EXPECT_THROW((void) mm6::decodeV4Save(mm6::encodeSaveContainer(container), GameTitle::MM6),
                 std::exception);
    container = mm6::decodeSaveContainer(mm6::encodeV4Save(source, GameTitle::MM6));
    container.sections.push_back({{'B', 'E', 'A', 'C'}, false, {1}});
    EXPECT_THROW((void) mm6::decodeV4Save(mm6::encodeSaveContainer(container), GameTitle::MM6),
                 std::exception);
}
