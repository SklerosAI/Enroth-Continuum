#include <cstdint>
#include <deque>
#include <exception>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6WorldSaveRuntime.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Pid.h"
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

std::vector<std::uint8_t> indoorDeltaWithActors(std::uint8_t count) {
    std::vector<std::uint8_t> bytes(8 + 875 + 3 * 4 + count * 548 + 200 * 80 + 200 + 56, 0);
    bytes[8 + 875] = count;
    return bytes;
}

std::vector<std::uint8_t> minimalIndoorBase() {
    std::vector<std::uint8_t> bytes(136 + 10 * 4, 0);
    bytes[136 + 4 * 4] = 200; // Fixed MM6 door slots.
    return bytes;
}

std::vector<std::uint8_t> minimalOutdoorBase() {
    return std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
}

std::vector<std::uint8_t> minimalOutdoorDelta() {
    return std::vector<std::uint8_t>(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
}

std::vector<std::uint8_t> outdoorDeltaWithActors(std::uint8_t count) {
    std::vector<std::uint8_t> bytes(8 + 2 * 88 * 11 + 3 * 4 + count * 548 + 200 + 56, 0);
    bytes[8 + 2 * 88 * 11] = count;
    return bytes;
}

std::unordered_map<std::string, Blob> validPriorDeltas(const std::vector<std::string> &names) {
    std::unordered_map<std::string, Blob> deltas;
    for (const std::string &name : names) {
        std::vector<std::uint8_t> bytes = name.ends_with(".dlv") ? minimalIndoorDelta() : minimalOutdoorDelta();
        deltas.emplace(name, Blob::copy(bytes.data(), bytes.size()));
    }
    return deltas;
}

Blob syntheticBase(std::string_view name) {
    std::vector<std::uint8_t> bytes = name.ends_with(".blv") ? minimalIndoorBase() : minimalOutdoorBase();
    return Blob::copy(bytes.data(), bytes.size());
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

UNIT_TEST(MM6WorldSaveRuntime, ValidatesAndStagesWorldAndVisualsAsOneDetachedCandidate) {
    const std::vector<std::string> names = expectedNames();
    std::unordered_map<std::string, Blob> prior = validPriorDeltas(names);
    NPCStats liveNpcs{};
    liveNpcs.uNumNewNPCs = 71;
    liveNpcs.pNPCData[70].name = "Prior NPC";
    NPCStats savedNpcs = liveNpcs;
    savedNpcs.pNPCData[70].name = "Saved NPC";
    savedNpcs.pGroups[50] = 42;
    std::vector<std::uint8_t> currentDelta = indoorDeltaWithActors(1);
    currentDelta[0] = 3;
    currentDelta[4] = 5;
    mm6::KnownSaveFields source = mm6::captureKnownWorldFields(
        seedFields(), savedNpcs, prior, "d0.dlv",
        currentDelta, names, GameTitle::MM6);
    source.metadata.timer.elapsedTicks = 123456;
    source.metadata.overlays[4].tableIndex = 3;
    source.metadata.overlays[4].animationLength = 10;
    source.metadata.overlays[4].x = -17;
    source.metadata.overlays[4].packedPid = Pid::actor(0).packed();
    std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(source);
    std::vector<std::uint8_t> indoorBase = minimalIndoorBase();
    std::vector<std::uint8_t> outdoorBase = minimalOutdoorBase();

    Timer liveTimer;
    ActiveOverlayList liveOverlays;
    liveOverlays.pOverlays[4].screenSpaceX = 99;
    mm6::StagedKnownWorldAndVisuals staged = mm6::stageKnownWorldAndVisuals(
        liveNpcs, liveTimer, liveOverlays, bytes, names,
        [&](std::string_view name) {
            const auto &base = name.ends_with(".blv") ? indoorBase : outdoorBase;
            return Blob::view(base.data(), base.size());
        },
        [](std::int32_t id) { return id >= 1 && id <= 580; }, 5, GameTitle::MM6);
    EXPECT_EQ(staged.known.core.party.playingTicks, 42);
    EXPECT_EQ(staged.world.npcs.pNPCData[70].name, "Saved NPC");
    EXPECT_EQ(staged.world.npcs.pGroups[50], 42);
    EXPECT_EQ(staged.world.mapDeltas.at("d0.dlv").size(), currentDelta.size());
    EXPECT_EQ(static_cast<const std::uint8_t *>(staged.world.mapDeltas.at("d0.dlv").data())[0], 3);
    EXPECT_EQ(static_cast<const std::uint8_t *>(staged.world.mapDeltas.at("d0.dlv").data())[4], 5);
    EXPECT_EQ(staged.world.mapDeltas.size(), 67);
    EXPECT_EQ(staged.visuals.timer.time().ticks(), 123456);
    EXPECT_TRUE(staged.visuals.timer.isPaused());
    EXPECT_EQ(staged.visuals.overlays.pOverlays[4].screenSpaceX, -17);
    ASSERT_TRUE(std::holds_alternative<mm6::StagedIndoorMap>(staged.activeMap));
    const mm6::StagedIndoorMap &active = std::get<mm6::StagedIndoorMap>(staged.activeMap);
    EXPECT_EQ(active.mapName, "d0.blv");
    EXPECT_EQ(active.delta.respawnCount, 3);
    EXPECT_EQ(active.delta.lastRespawnDay, 5);
    EXPECT_EQ(active.delta.actors.size(), 1);
    EXPECT_EQ(liveNpcs.pNPCData[70].name, "Prior NPC");
    EXPECT_EQ(liveTimer.time().ticks(), 0);
    EXPECT_EQ(liveOverlays.pOverlays[4].screenSpaceX, 99);

    // No active candidate may borrow the encoded input or the base reader's
    // backing memory: both are allowed to disappear before a later commit.
    bytes.clear();
    indoorBase[136 + 4 * 4] = 0;
    staged.known.maps.deltas[0].bytes[0] = 0;
    EXPECT_EQ(active.delta.respawnCount, 3);
    EXPECT_EQ(active.base.section(MM6IndoorSectionKind::DoorSlots).count, 200);
    EXPECT_EQ(static_cast<std::uint8_t>(active.base.section(MM6IndoorSectionKind::DoorSlots).bytes.str()[0]),
              200);
    EXPECT_EQ(static_cast<std::uint8_t>(active.delta.wire.section(MM6IndoorSectionKind::ResetHeader).bytes.str()[0]),
              3);

    source.metadata.overlays[49].tableIndex = 5;
    source.metadata.overlays[49].animationLength = 1;
    source.metadata.overlays[49].packedPid = Pid::actor(0).packed();
    bytes = mm6::encodeKnownSave(source);
    EXPECT_THROW((void) mm6::stageKnownWorldAndVisuals(
        liveNpcs, liveTimer, liveOverlays, bytes, names, syntheticBase,
        [](std::int32_t) { return true; }, 5, GameTitle::MM6), std::exception);
    EXPECT_EQ(liveNpcs.pNPCData[70].name, "Prior NPC");
    EXPECT_EQ(liveOverlays.pOverlays[4].screenSpaceX, 99);

    source.metadata.overlays[49] = {};
    source.maps.deltas.back().bytes.pop_back();
    bytes = mm6::encodeKnownSave(source);
    EXPECT_THROW((void) mm6::stageKnownWorldAndVisuals(
        liveNpcs, liveTimer, liveOverlays, bytes, names, syntheticBase,
        [](std::int32_t) { return true; }, 5, GameTitle::MM6), std::exception);
    EXPECT_EQ(liveNpcs.pNPCData[70].name, "Prior NPC");
    EXPECT_EQ(prior.at("d0.dlv").size(), minimalIndoorDelta().size());
    EXPECT_THROW((void) mm6::stageKnownWorldAndVisuals(
        liveNpcs, liveTimer, liveOverlays, mm6::encodeKnownSave(source), names, syntheticBase,
        [](std::int32_t) { return true; }, 5, GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6WorldSaveRuntime, RejectsSavedOverlayActorPidsOutsideActiveIndoorAndOutdoorDelta) {
    const std::vector<std::string> names = expectedNames();
    const auto prior = validPriorDeltas(names);
    NPCStats npcs{};
    Timer timer;
    ActiveOverlayList overlays;
    overlays.pOverlays[0].screenSpaceX = 91;
    int baseReads = 0;
    const auto check = [&](const mm6::KnownSaveFields &source) {
        std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(source);
        baseReads = 0;
        return mm6::stageKnownWorldAndVisuals(npcs, timer, overlays,
            bytes, names, [&](std::string_view name) {
                ++baseReads;
                return syntheticBase(name);
            },
            [](std::int32_t id) { return id >= 1 && id <= 580; }, 5, GameTitle::MM6);
    };

    mm6::KnownSaveFields indoor = mm6::captureKnownWorldFields(
        seedFields(), npcs, prior, "d0.dlv", indoorDeltaWithActors(1), names, GameTitle::MM6);
    indoor.metadata.overlays[0].animationLength = 10;
    indoor.metadata.overlays[0].packedPid = Pid::actor(0).packed();
    EXPECT_NO_THROW((void) check(indoor));
    indoor.metadata.overlays[0].packedPid = Pid::actor(1).packed();
    EXPECT_THROW((void) check(indoor), std::exception);
    EXPECT_EQ(baseReads, 67);
    indoor.metadata.overlays[0].packedPid = Pid::sprite(0).packed();
    EXPECT_THROW((void) check(indoor), std::exception);
    EXPECT_EQ(overlays.pOverlays[0].screenSpaceX, 91);

    mm6::KnownSaveFields outdoorSeed = seedFields();
    outdoorSeed.metadata.header.mapFileName = "o0.odm";
    mm6::KnownSaveFields outdoor = mm6::captureKnownWorldFields(
        outdoorSeed, npcs, prior, "o0.ddm", outdoorDeltaWithActors(2), names, GameTitle::MM6);
    outdoor.metadata.overlays[0].animationLength = 10;
    outdoor.metadata.overlays[0].packedPid = Pid::actor(1).packed();
    mm6::StagedKnownWorldAndVisuals stagedOutdoor = check(outdoor);
    ASSERT_TRUE(std::holds_alternative<mm6::StagedOutdoorMap>(stagedOutdoor.activeMap));
    const mm6::StagedOutdoorMap &activeOutdoor = std::get<mm6::StagedOutdoorMap>(stagedOutdoor.activeMap);
    EXPECT_EQ(activeOutdoor.mapName, "o0.odm");
    EXPECT_EQ(activeOutdoor.delta.actors.size(), 2);
    EXPECT_EQ(activeOutdoor.delta.wire.actorCount, 2);
    outdoor.metadata.overlays[0].packedPid = Pid::actor(2).packed();
    EXPECT_THROW((void) check(outdoor), std::exception);
    EXPECT_EQ(baseReads, 67);
    EXPECT_EQ(overlays.pOverlays[0].screenSpaceX, 91);
    EXPECT_EQ(npcs.uNumNewNPCs, 0);
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
