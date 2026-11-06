#include <cstdint>
#include <deque>
#include <exception>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6WorldSaveRuntime.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Tables/MapTable.h"

namespace {

std::vector<std::string> expectedNames() {
    std::vector<std::string> names;
    for (int i = 0; i < 52; ++i)
        names.push_back("d" + std::to_string(i) + ".dlv");
    for (int i = 0; i < 15; ++i)
        names.push_back("o" + std::to_string(i) + ".ddm");
    return names;
}

mm6::KnownSaveFields seedFields() {
    mm6::KnownSaveFields fields;
    fields.core.party.playingTicks = 42;
    fields.core.party.activeCharacter = 1;
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.metadata.header.displayName = "World capture";
    fields.metadata.header.mapFileName = "d0.blv";
    fields.metadata.header.playingTicks = 42;
    fields.metadata.ui.activeCharacter = 1;
    fields.recovery.playingTicks = 42;
    return fields;
}

std::unordered_map<std::string, Blob> priorDeltas(const std::vector<std::string> &names) {
    std::unordered_map<std::string, Blob> deltas;
    for (const std::string &name : names)
        deltas.emplace(name, Blob::fromString("prior:" + name));
    return deltas;
}

struct ScopedActiveMap {
    IndoorLocation *indoor = pIndoor;
    OutdoorLocation *outdoor = pOutdoor;
    LevelType levelType = uCurrentlyLoadedLevelType;
    std::deque<Actor> actors;
    std::vector<SpriteObject> sprites;
    std::vector<Chest> chests;

    ScopedActiveMap() {
        actors.swap(pActors);
        sprites.swap(pSpriteObjects);
        chests.swap(vChests);
    }

    ~ScopedActiveMap() {
        pIndoor = indoor;
        pOutdoor = outdoor;
        uCurrentlyLoadedLevelType = levelType;
        pActors.clear();
        pSpriteObjects.clear();
        vChests.clear();
        actors.swap(pActors);
        sprites.swap(pSpriteObjects);
        chests.swap(vChests);
    }
};

std::vector<std::uint8_t> minimalIndoorDelta() {
    return std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
}

} // namespace

UNIT_TEST(MM6WorldSaveRuntime, CapturesAndStagesNonzeroNpcAndCurrentMapOnCopies) {
    std::vector<std::string> names = expectedNames();
    std::unordered_map<std::string, Blob> prior = priorDeltas(names);
    NPCStats liveNpcs{};
    liveNpcs.uNumNewNPCs = 71;
    liveNpcs.pNPCData[70].name = "Mira";
    liveNpcs.pNPCData[70].fame = -17;
    liveNpcs.pGroups[50] = 42;
    liveNpcs.pOriginalNPCData[1].name = "Original source";
    const std::vector<std::uint8_t> current = {9, 8, 7, 6};

    mm6::KnownSaveFields captured = mm6::captureKnownWorldFields(
        seedFields(), liveNpcs, prior, "d0.dlv", current, names, GameTitle::MM6);
    std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(captured);
    mm6::KnownSaveFields decoded = mm6::decodeKnownSave(bytes);
    EXPECT_EQ(decoded.npcs.npcData[70].name, "Mira");
    EXPECT_EQ(decoded.npcs.groups[50], 42);
    EXPECT_EQ(decoded.maps.deltas[0].bytes, current);
    EXPECT_EQ(prior.at("d0.dlv").str(), "prior:d0.dlv");

    NPCStats destination{};
    destination.uNumNewNPCs = 3;
    destination.pNPCData[70].name = "Old runtime";
    destination.pOriginalNPCData[1].name = "Original source";
    mm6::StagedWorldFields staged = mm6::stageKnownWorldFields(destination, bytes, names, GameTitle::MM6);
    EXPECT_EQ(staged.npcs.uNumNewNPCs, 71);
    EXPECT_EQ(staged.npcs.pNPCData[70].name, "Mira");
    EXPECT_EQ(staged.npcs.pNPCData[70].fame, -17);
    EXPECT_EQ(staged.npcs.pGroups[50], 42);
    EXPECT_EQ(staged.npcs.pOriginalNPCData[1].name, "Original source");
    EXPECT_EQ(staged.mapDeltas.at("d0.dlv").str(), std::string_view("\x09\x08\x07\x06", 4));
    EXPECT_EQ(staged.mapDeltas.at("o14.ddm").str(), "prior:o14.ddm");
    EXPECT_EQ(destination.pNPCData[70].name, "Old runtime");
    EXPECT_EQ(prior.at("d0.dlv").str(), "prior:d0.dlv");
}

UNIT_TEST(MM6WorldSaveRuntime, OutdoorIdentityJoinsCurrentMapTableHeadAndManifest) {
    MapTable maps;
    maps.pInfos[MAP_EMERALD_ISLAND].fileName = "o0.odm";
    maps.pInfos[MAP_HARMONDALE].fileName = "o1.odm";
    std::vector<std::string> names = expectedNames();

    EXPECT_NO_THROW(mm6::validateOutdoorMapIdentity(
        MAP_EMERALD_ISLAND, maps, "o0.odm", names, GameTitle::MM6));
    EXPECT_THROW(mm6::validateOutdoorMapIdentity(
        MAP_HARMONDALE, maps, "o0.odm", names, GameTitle::MM6), std::exception);
    EXPECT_THROW(mm6::validateOutdoorMapIdentity(
        MAP_INVALID, maps, "o0.odm", names, GameTitle::MM6), std::exception);
    EXPECT_THROW(mm6::validateOutdoorMapIdentity(
        MAP_EMERALD_ISLAND, maps, "d0.blv", names, GameTitle::MM6), std::exception);
    EXPECT_THROW(mm6::validateOutdoorMapIdentity(
        MAP_EMERALD_ISLAND, maps, "o0.odm", names, GameTitle::MM7), std::exception);

    names[52] = "other.ddm"; // Replace the active companion, preserving count 67.
    EXPECT_THROW(mm6::validateOutdoorMapIdentity(
        MAP_EMERALD_ISLAND, maps, "o0.odm", names, GameTitle::MM6), std::exception);
    names = expectedNames();
    maps.pInfos[MAP_HARMONDALE].fileName = "o0.odm";
    EXPECT_THROW(mm6::validateOutdoorMapIdentity(
        MAP_EMERALD_ISLAND, maps, "o0.odm", names, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6WorldSaveRuntime, RejectsIncompleteAndLateInvalidStateWithoutInputMutation) {
    std::vector<std::string> names = expectedNames();
    std::unordered_map<std::string, Blob> prior = priorDeltas(names);
    NPCStats liveNpcs{};
    liveNpcs.uNumNewNPCs = 1;
    liveNpcs.pNPCData[0].name = "Before";
    const std::vector<std::uint8_t> current = {4, 3, 2, 1};

    EXPECT_THROW((void) mm6::captureKnownWorldFields(
        seedFields(), liveNpcs, prior, "o0.ddm", current, names, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::captureKnownWorldFields(
        seedFields(), liveNpcs, prior, "d0.dlv", {}, names, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::captureKnownWorldFields(
        seedFields(), liveNpcs, prior, "d0.dlv", current, names, GameTitle::MM7), std::exception);
    prior.erase("d1.dlv");
    EXPECT_THROW((void) mm6::captureKnownWorldFields(
        seedFields(), liveNpcs, prior, "d0.dlv", current, names, GameTitle::MM6), std::exception);
    prior.emplace("d1.dlv", Blob::fromString("restored"));
    prior.emplace("foreign.ddm", Blob::fromString("foreign"));
    EXPECT_THROW((void) mm6::captureKnownWorldFields(
        seedFields(), liveNpcs, prior, "d0.dlv", current, names, GameTitle::MM6), std::exception);
    prior.erase("foreign.ddm");

    mm6::KnownSaveFields captured = mm6::captureKnownWorldFields(
        seedFields(), liveNpcs, prior, "d0.dlv", current, names, GameTitle::MM6);
    mm6::SaveContainer lateInvalid = mm6::decodeSaveContainer(mm6::encodeKnownSave(captured));
    lateInvalid.sections.back().payload[0] = 3; // Unsupported RCVR version with valid outer CRCs.
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(lateInvalid);
    EXPECT_THROW((void) mm6::stageKnownWorldFields(liveNpcs, bytes, names, GameTitle::MM6),
                 std::exception);
    EXPECT_THROW((void) mm6::stageKnownWorldFields(
        liveNpcs, mm6::encodeKnownSave(captured), names, GameTitle::MM7), std::exception);
    EXPECT_EQ(liveNpcs.pNPCData[0].name, "Before");
    EXPECT_EQ(prior.at("d0.dlv").str(), "prior:d0.dlv");
}

UNIT_TEST(MM6WorldSaveRuntime, CapturesBoundedActiveIndoorStateWithoutChangingWorld) {
    ScopedActiveMap globalState;
    IndoorLocation location;
    location.filename = "d0.blv";
    location.mm6DeltaBytes = minimalIndoorDelta();
    location._visible_outlines.fill(0);
    location.doors.assign(200, BLVDoor{});
    location.dlv.respawnCount = 3;
    location.dlv.lastRespawnDay = 5;
    pIndoor = &location;
    pOutdoor = nullptr;
    uCurrentlyLoadedLevelType = LEVEL_INDOOR;

    std::vector<std::string> names = expectedNames();
    std::unordered_map<std::string, Blob> prior = priorDeltas(names);
    const std::vector<std::uint8_t> sidecarBefore = location.mm6DeltaBytes;
    NPCStats npcs{};
    npcs.uNumNewNPCs = 1;
    npcs.pNPCData[0].name = "Runtime NPC";
    mm6::KnownSaveFields captured = mm6::captureKnownWorldFromActiveMap(
        seedFields(), npcs, prior, names, GameTitle::MM6);
    ASSERT_EQ(captured.maps.deltas.size(), 67);
    EXPECT_GT(captured.maps.deltas[0].bytes.size(), prior.at("d0.dlv").size());
    EXPECT_EQ(captured.maps.deltas[0].bytes[0], 3);
    EXPECT_EQ(captured.maps.deltas[0].bytes[4], 5);
    mm6::KnownSaveFields decoded = mm6::decodeKnownSave(mm6::encodeKnownSave(captured));
    EXPECT_EQ(decoded.maps.deltas[0].bytes, captured.maps.deltas[0].bytes);
    EXPECT_EQ(location.mm6DeltaBytes, sidecarBefore);
    EXPECT_EQ(prior.at("d0.dlv").str(), "prior:d0.dlv");
    EXPECT_EQ(npcs.pNPCData[0].name, "Runtime NPC");

    location.filename = "other.blv";
    EXPECT_THROW((void) mm6::captureKnownWorldFromActiveMap(
        seedFields(), npcs, prior, names, GameTitle::MM6), std::exception);
    location.filename = "d0.blv";
    location.mm6DeltaBytes = {1};
    EXPECT_THROW((void) mm6::captureKnownWorldFromActiveMap(
        seedFields(), npcs, prior, names, GameTitle::MM6), std::exception);
    EXPECT_EQ(prior.at("d0.dlv").str(), "prior:d0.dlv");
    EXPECT_THROW((void) mm6::captureKnownWorldFromActiveMap(
        seedFields(), npcs, prior, names, GameTitle::MM7), std::exception);
    uCurrentlyLoadedLevelType = LEVEL_OUTDOOR;
    EXPECT_THROW((void) mm6::captureKnownWorldFromActiveMap(
        seedFields(), npcs, prior, names, GameTitle::MM6), std::exception);
}
