#include <array>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawSaveFields.h"
#include "Engine/MM6/MM6V6IndoorSaveFields.h"
#include "Library/Random/SequentialRandomEngine.h"
#include "Utility/Memory/Blob.h"

namespace {

mm6::V6IndoorSaveFields candidate() {
    mm6::V6IndoorSaveFields result;
    auto &raw = result.raw;
    raw.core.party.playingTicks = 42;
    raw.core.party.activeCharacter = 2;
    for (auto &character : raw.core.characters)
        character.name = "Hero";
    raw.inventory.ready = true;
    raw.metadata.header.displayName = "Indoor candidate";
    raw.metadata.header.mapFileName = "d0.blv";
    raw.metadata.header.playingTicks = 42;
    raw.metadata.ui.activeCharacter = 2;
    for (int index = 0; index < 52; ++index)
        raw.maps.deltas.push_back({"d" + std::to_string(index) + ".dlv", {1}});
    for (int index = 0; index < 15; ++index)
        raw.maps.deltas.push_back({"o" + std::to_string(index) + ".ddm", {2}});
    raw.chest = {0x12345678, {}, "d0.dlv", {1}};
    raw.npcs.activeCount = 399;
    raw.recovery.playingTicks = 42;
    raw.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);

    result.generatedNpcs.mapName = "d0.blv";
    SequentialRandomEngine rng;
    rng.seed(22);
    result.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    result.gameplayRng.sequentialState = rng.state();
    mm6::EventRuntimeMapEntry event;
    event.mapName = "d0.blv";
    event.lastRespawnDay = 5;
    result.eventRuntime.maps.push_back(event);
    result.actors = {"d0.blv", 1, std::vector<std::uint8_t>(548, 0x11)};
    result.sprites = {"d0.blv", 2, std::vector<std::uint8_t>(200, 0x22)};
    result.decorations = {"d0.blv", 3, std::vector<std::uint8_t>(180, 0x33)};
    return result;
}

mm6::SaveSection &section(mm6::SaveContainer *container,
                          const std::array<char, 4> &tag) {
    for (auto &item : container->sections)
        if (item.tag == tag)
            return item;
    throw std::runtime_error("MM6 v6 fixture section is absent");
}

} // namespace

UNIT_TEST(MM6V6IndoorSaveFields, StrictTwentySixSectionRoundTrip) {
    const auto source = candidate();
    const auto bytes = mm6::encodeV6IndoorSave(source, GameTitle::MM6);
    const auto container = mm6::decodeSaveContainer(bytes);
    EXPECT_EQ(container.version, 6);
    EXPECT_EQ(container.sections.size(), 26);
    const auto decoded = mm6::decodeV6IndoorSave(bytes, GameTitle::MM6);
    EXPECT_EQ(decoded.raw.metadata.header.mapFileName, "d0.blv");
    EXPECT_EQ(decoded.actors, source.actors);
    EXPECT_EQ(decoded.sprites, source.sprites);
    EXPECT_EQ(decoded.decorations, source.decorations);
    EXPECT_EQ(mm6::encodeV6IndoorSave(decoded, GameTitle::MM6), bytes);
    EXPECT_THROW((void) mm6::decodeRawSave(bytes, GameTitle::MM6), std::exception);
    EXPECT_FALSE(mm6::StagedV6IndoorSaveFields{}.canEnterGameLoop());
}

UNIT_TEST(MM6V6IndoorSaveFields, RejectsMissingMismatchedAndMalformedOwners) {
    auto source = candidate();
    source.sprites.mapName = "oute3.odm";
    EXPECT_THROW((void) mm6::encodeV6IndoorSave(source, GameTitle::MM6), std::exception);
    source = candidate();
    source.actors.records.pop_back();
    EXPECT_THROW((void) mm6::encodeV6IndoorSave(source, GameTitle::MM6), std::exception);
    source = candidate();
    EXPECT_THROW((void) mm6::encodeV6IndoorSave(source, GameTitle::MM7), std::exception);

    auto container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    section(&container, {'I', 'A', 'C', 'R'}).payload[2] = 'x';
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
    container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    section(&container, {'I', 'S', 'C', 'R'}).payload.pop_back();
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
    container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    section(&container, {'I', 'F', 'O', 'V'}).required = true;
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V6IndoorSaveFields, NativeAnchorCaptureRequiresMatchingIndoorPlan) {
    mm6::StagedRawSessionFields represented;
    represented.indoorPlan.emplace();
    represented.indoorPlan->mapName = "d0.blv";
    represented.indoorPlan->savedDelta.wire.sections.push_back(
        {MM6IndoorSectionKind::Actors, Blob::fromString(std::string(4, '\0')), 0});
    represented.indoorPlan->savedDelta.wire.sections.push_back(
        {MM6IndoorSectionKind::GroundItems, Blob::fromString(std::string(4, '\0')), 0});
    MM6IndoorSnapshot base;
    base.sections.push_back(
        {MM6IndoorSectionKind::Sprites, Blob::fromString(std::string(4, '\0')), 0});
    represented.raw.activeMap = mm6::StagedIndoorMap{"d0.blv", std::move(base), {}};
    mm6::IndoorNativeSlotAnchor actors, sprites, decorations;
    mm6::captureV6IndoorNativeAnchors(represented, &actors, &sprites, &decorations);
    EXPECT_EQ(actors, (mm6::IndoorNativeSlotAnchor{"d0.blv", 0, {}}));
    EXPECT_EQ(sprites, (mm6::IndoorNativeSlotAnchor{"d0.blv", 0, {}}));
    EXPECT_EQ(decorations, (mm6::IndoorNativeSlotAnchor{"d0.blv", 0, {}}));

    represented.indoorPlan->mapName = "other.blv";
    EXPECT_THROW(mm6::captureV6IndoorNativeAnchors(
        represented, &actors, &sprites, &decorations), std::exception);
    represented.indoorPlan->mapName = "d0.blv";
    represented.indoorPlan->savedDelta.wire.sections[0].count = 1;
    EXPECT_THROW(mm6::captureV6IndoorNativeAnchors(
        represented, &actors, &sprites, &decorations), std::exception);
}
