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
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Pid.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/ItemTable.h"

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
    fields.chest = {17, {}, "d0.dlv", std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0)};
    fields.shopGuildClocks.ready = true;
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

std::vector<std::uint8_t> outdoorBaseWithTriangle() {
    constexpr std::size_t header = 160 + 16 + 3 * 128 * 128;
    constexpr std::size_t modelExtra = header + 4 + 188;
    constexpr std::size_t face = modelExtra + 3 * 12;
    std::vector<std::uint8_t> bytes(header + 4 + 188 + 3 * 12 + 308 + 2 + 10 +
                                    4 + 4 + 128 * 128 * 4 + 4 + 20, 0);
    bytes[header] = 1; // One model.
    bytes[header + 4 + 68] = 3; // Three local vertices.
    bytes[header + 4 + 76] = 1; // One face.
    bytes[face + 302] = 3; // Face uses all three vertices.
    bytes[face + 32 + 2] = 1;
    bytes[face + 32 + 4] = 2;
    bytes[face + 308 + 2] = 's'; // Opaque texture key, never resolved here.
    bytes[modelExtra] = 10; // First vertex X.
    bytes[modelExtra + 12] = 20; // Second vertex X.
    bytes[modelExtra + 24] = 30; // Third vertex X.
    bytes[bytes.size() - 24] = 1; // One spawn.
    bytes[bytes.size() - 20] = 77; // Spawn X.
    bytes[bytes.size() - 20 + 12] = 8; // Radius.
    bytes[bytes.size() - 20 + 14] = std::to_underlying(OBJECT_Actor);
    bytes[bytes.size() - 20 + 16] = 5; // Monster index.
    return bytes;
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

UNIT_TEST(MM6WorldSaveRuntime, PreparesDetachedOutdoorGeometryWithoutLiveWorld) {
    mm6::KnownSaveFields source = seedFields();
    source.metadata.header.mapFileName = "o0.odm";
    source.chest.activeDeltaName = "o0.ddm";
    source.chest.rawActiveDelta = minimalOutdoorDelta();
    const auto names = expectedNames();
    for (const std::string &name : names)
        source.maps.deltas.push_back({name, name.ends_with(".dlv") ?
            minimalIndoorDelta() : minimalOutdoorDelta()});
    const std::vector<std::uint8_t> encoded = mm6::encodeKnownSave(source);
    const auto readBase = [](std::string_view name) {
        const auto bytes = name == "o0.odm" ? outdoorBaseWithTriangle() :
            (name.ends_with(".odm") ? minimalOutdoorBase() : minimalIndoorBase());
        return Blob::copy(bytes.data(), bytes.size());
    };
    NPCStats npcs{};
    Timer timer;
    ActiveOverlayList overlays;
    auto staged = mm6::stageKnownWorldAndVisuals(
        npcs, timer, overlays, encoded, names, readBase,
        [](std::int32_t) { return true; }, 5, GameTitle::MM6);
    auto plan = mm6::prepareDetachedOutdoorWorldPlan(staged, GameTitle::MM6);
    EXPECT_EQ(plan.mapName, "o0.odm");
    EXPECT_EQ(plan.base.models.size(), 1);
    EXPECT_EQ(plan.base.modelExtras[0].vertices.size(), 3);
    EXPECT_EQ(plan.vertices, std::vector<Vec3f>({Vec3f(10, 0, 0), Vec3f(20, 0, 0), Vec3f(30, 0, 0)}));
    EXPECT_EQ(plan.faceVertexIds.size(), 1);
    EXPECT_EQ(plan.faceVertexIds[0], std::vector<std::size_t>({0, 1, 2}));
    ASSERT_EQ(plan.spawnPoints.size(), 1);
    EXPECT_EQ(plan.spawnPoints[0].position, Vec3f(77, 0, 0));
    EXPECT_EQ(plan.spawnPoints[0].radius, 8);
    EXPECT_EQ(plan.spawnPoints[0].type, OBJECT_Actor);
    EXPECT_EQ(plan.spawnPoints[0].monsterIndex, 5);
    EXPECT_EQ(plan.activeSidecar.wire.bytes, source.chest.rawActiveDelta);
    EXPECT_EQ(plan.savedDelta.wire.bytes, source.chest.rawActiveDelta);
    EXPECT_EQ(npcs.uNumNewNPCs, 0);

    EXPECT_THROW((void) mm6::prepareDetachedOutdoorWorldPlan(staged, GameTitle::MM7), std::exception);
    auto &active = std::get<mm6::StagedOutdoorMap>(staged.activeMap);
    active.base.faceBytes(0, 0)[32] = 3; // Late topology fault after base and sidecar decoding.
    EXPECT_THROW((void) mm6::prepareDetachedOutdoorWorldPlan(staged, GameTitle::MM6), std::exception);
    EXPECT_EQ(plan.faceVertexIds[0], std::vector<std::size_t>({0, 1, 2}));
    EXPECT_EQ(staged.rawActiveDelta, source.chest.rawActiveDelta);
    active.base.faceBytes(0, 0)[32] = 0;
    active.base.bytes[active.base.bytes.size() - 20 + 14] = 9;
    EXPECT_THROW((void) mm6::prepareDetachedOutdoorWorldPlan(staged, GameTitle::MM6), std::exception);
    active.base.bytes[active.base.bytes.size() - 20 + 14] = std::to_underlying(OBJECT_Actor);
    staged.rawActiveDelta[0] = 9;
    EXPECT_THROW((void) mm6::prepareDetachedOutdoorWorldPlan(staged, GameTitle::MM6), std::exception);
}

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
    source.chest.rawActiveDelta = currentDelta;
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
    EXPECT_TRUE(staged.chestTitle.ready);
    EXPECT_EQ(staged.chestTitle.lcgState, 17);
    EXPECT_EQ(staged.rawActiveDelta, currentDelta);
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

UNIT_TEST(MM6WorldSaveRuntime, StagesRepresentedPartyFamiliesWithWorldAndRejectsLateFailure) {
    const std::vector<std::string> names = expectedNames();
    std::unordered_map<std::string, Blob> prior = validPriorDeltas(names);
    NPCStats liveNpcs{};
    liveNpcs.pNPCData[70].name = "Prior NPC";
    NPCStats savedNpcs = liveNpcs;
    savedNpcs.pNPCData[70].name = "Saved NPC";
    mm6::KnownSaveFields source = mm6::captureKnownWorldFields(
        seedFields(), savedNpcs, prior, "d0.dlv", indoorDeltaWithActors(1), names, GameTitle::MM6);
    source.chest.rawActiveDelta = indoorDeltaWithActors(1);
    mm6::ShopGuildClockFields clocks;
    clocks.ready = true;
    clocks.deadlineWords[mm6::shopClockSlot(47)] = 0x12345678;
    clocks.deadlineWords[mm6::guildClockSlot(140)] = 90001;
    source = mm6::captureKnownShopGuildClockFields(source, clocks, GameTitle::MM6);
    source.core.party.gold = 731;
    source.core.characters[0].name = "Saved Hero";
    source.core.characters[0].level = 5;
    source.core.characters[0].modifiers.statBonuses[0] = 9;
    source.core.characters[0].runtimeBonuses.bytes = {0x80, 2, 3, 4, 5, 6, 7, 0xff};
    source.durable.bits.quest[0] = 1;
    source.durable.effects.conditionTicks[0][0] = 1234;
    source.resistances.characters[0].base[0] = 17;
    source.buffs.party[0].expireTicks = 500;
    source.buffs.party[0].power = 3;
    source.hirelings.displayNames[0] = "Mira";
    mm6::DurableItem sword;
    sword.sourceIndex = 17;
    sword.zone = 1;
    sword.x = 2;
    sword.y = 3;
    sword.itemId = std::to_underlying(ITEM_CRUDE_LONGSWORD);
    source.durable.inventory.characters[0].push_back(sword);
    source.heldItem.present = true;
    source.heldItem.itemId = std::to_underlying(ITEM_BRASS_RING);
    source.recovery.characterRecoveryTicks[0] = 250;
    source.recovery.eventClocks.counters[0] = 901;
    std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(source);

    ItemTable localTable;
    ItemTable *previousTable = pItemTable;
    struct RestoreTable {
        ItemTable *previous;
        ~RestoreTable() { pItemTable = previous; }
    } restore{previousTable};
    pItemTable = &localTable;
    localTable.items[ITEM_CRUDE_LONGSWORD].iconName = "sword";
    localTable.itemSizes[ITEM_CRUDE_LONGSWORD] = Sizei(2, 1);
    localTable.items[ITEM_BRASS_RING].iconName = "ring";
    localTable.itemSizes[ITEM_BRASS_RING] = Sizei(1, 1);

    Party liveParty;
    liveParty.uNumGold = 19;
    liveParty.pCharacters[0].name = "Prior Hero";
    liveParty._roundingDt = Duration::fromTicks(1);
    liveParty._delayedReactionTimer = Duration::fromTicks(50);
    liveParty._delayedReactionSpeech = SPEECH_SHOP_RUDE;
    liveParty._delayedReactionCharacterId = 2;
    Timer liveTimer;
    ActiveOverlayList liveOverlays;
    auto stage = [&] {
        return mm6::stageKnownSession(liveParty, liveNpcs, liveTimer, liveOverlays,
                                      bytes, names, syntheticBase,
                                      [](std::int32_t id) { return id >= 1 && id <= 580; },
                                      5, GameTitle::MM6);
    };

    mm6::StagedKnownSession staged = stage();
    EXPECT_EQ(staged.party.uNumGold, 731);
    EXPECT_EQ(staged.party.pCharacters[0].name, "Saved Hero");
    EXPECT_EQ(staged.party.pCharacters[0].uLevel, 5);
    EXPECT_EQ(staged.party.pCharacters[0]._statBonuses[ATTRIBUTE_MIGHT], 9);
    EXPECT_EQ(mm6::captureCharacterRuntimeBonuses(staged.party.pCharacters[0], GameTitle::MM6).bytes,
              source.core.characters[0].runtimeBonuses.bytes);
    EXPECT_TRUE(staged.party._questBits.test(static_cast<QuestBit>(1)));
    EXPECT_EQ(staged.party.pCharacters[0].conditions.get(static_cast<Condition>(0)).ticks(), 1234);
    EXPECT_EQ(staged.party.pCharacters[0].sResFireBase, 17);
    EXPECT_EQ(staged.party.pPartyBuffs[static_cast<PartyBuff>(0)].power, 3);
    EXPECT_EQ(staged.party.pHireling1Name, "Mira");
    EXPECT_EQ(staged.party.pCharacters[0].inventory.entry(Pointi(2, 3)).index(), 17);
    EXPECT_EQ(staged.party.pPickedItem.itemId, ITEM_BRASS_RING);
    EXPECT_EQ(staged.party.pCharacters[0].timeToRecovery.ticks(), 250);
    EXPECT_EQ(staged.party.PartyTimes.CounterEventValues[0].ticks(), 901);
    EXPECT_EQ(staged.party._roundingDt.ticks(), 0);
    EXPECT_EQ(staged.party._delayedReactionTimer.ticks(), 0);
    EXPECT_EQ(staged.party._delayedReactionSpeech, SPEECH_NONE);
    EXPECT_EQ(staged.party._delayedReactionCharacterId, -1);
    EXPECT_EQ(staged.worldAndVisuals.world.npcs.pNPCData[70].name, "Saved NPC");
    EXPECT_EQ(staged.shopGuildClocks, clocks);
    EXPECT_TRUE(staged.worldAndVisuals.chestTitle.ready);
    EXPECT_EQ(staged.worldAndVisuals.rawActiveDelta, source.chest.rawActiveDelta);
    EXPECT_TRUE(std::holds_alternative<mm6::StagedIndoorMap>(staged.worldAndVisuals.activeMap));
    EXPECT_EQ(liveParty.uNumGold, 19);
    EXPECT_EQ(liveParty.pCharacters[0].name, "Prior Hero");
    EXPECT_EQ(liveParty._roundingDt.ticks(), 1);
    EXPECT_EQ(liveParty._delayedReactionTimer.ticks(), 50);
    EXPECT_EQ(liveParty._delayedReactionSpeech, SPEECH_SHOP_RUDE);
    EXPECT_EQ(liveParty._delayedReactionCharacterId, 2);
    EXPECT_EQ(liveNpcs.pNPCData[70].name, "Prior NPC");

    const std::vector<std::uint8_t> validBytes = bytes;
    mm6::SaveContainer truncatedClock = mm6::decodeSaveContainer(bytes);
    truncatedClock.sections.back().payload.pop_back();
    bytes = mm6::encodeSaveContainer(truncatedClock);
    EXPECT_THROW((void) stage(), std::exception);
    EXPECT_EQ(liveParty.uNumGold, 19);
    EXPECT_EQ(liveNpcs.pNPCData[70].name, "Prior NPC");
    bytes = validBytes;

    // HELD stages after ITEM. A missing loaded held-item template rejects the
    // whole candidate without exposing the already restored inventory.
    localTable.items[ITEM_BRASS_RING].iconName.clear();
    EXPECT_THROW((void) stage(), std::exception);
    EXPECT_FALSE(liveParty.pCharacters[0].inventory.entry(Pointi(2, 3)));
    EXPECT_EQ(liveParty.pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(liveNpcs.pNPCData[70].name, "Prior NPC");
    localTable.items[ITEM_BRASS_RING].iconName = "ring";

    // Recovery is the final Party adapter; rejecting active turn state here
    // must not expose any earlier staged owner to the caller.
    liveParty.bTurnBasedModeOn = true;
    EXPECT_THROW((void) stage(), std::exception);
    EXPECT_EQ(liveParty.uNumGold, 19);
    EXPECT_EQ(liveParty.pCharacters[0].name, "Prior Hero");
    EXPECT_EQ(liveNpcs.pNPCData[70].name, "Prior NPC");
    EXPECT_EQ(liveTimer.time().ticks(), 0);

    liveParty.bTurnBasedModeOn = false;
    liveParty.pCharacters[0].vBeacons[0] = LloydBeacon{};
    EXPECT_THROW((void) stage(), std::exception);
    EXPECT_THROW((void) mm6::captureKnownPartyFields(source, liveParty, GameTitle::MM6), std::exception);
    EXPECT_EQ(liveParty.pCharacters[0].name, "Prior Hero");
    EXPECT_TRUE(liveParty.pCharacters[0].vBeacons[0].has_value());
    EXPECT_EQ(liveNpcs.pNPCData[70].name, "Prior NPC");

    liveParty.pCharacters[0].vBeacons[0].reset();
    liveParty.PartyTimes.shopNextRefreshTime[HOUSE_FIRST_SHOP] = Time::fromTicks(700);
    EXPECT_THROW((void) stage(), std::exception);
    EXPECT_THROW((void) mm6::captureKnownPartyFields(source, liveParty, GameTitle::MM6), std::exception);
    liveParty.PartyTimes.shopNextRefreshTime[HOUSE_FIRST_SHOP] = Time();

    liveParty.standartItemsInShops[HOUSE_FIRST_SHOP][0].itemId = ITEM_CRUDE_LONGSWORD;
    EXPECT_THROW((void) stage(), std::exception);
    liveParty.standartItemsInShops[HOUSE_FIRST_SHOP][0].Reset();
    liveParty.standartItemsInShops[HOUSE_FIRST_SHOP][0].goldAmount = 8;
    EXPECT_THROW((void) stage(), std::exception); // Zero ID must not erase unknown slot data.
    liveParty.standartItemsInShops[HOUSE_FIRST_SHOP][0].Reset();

    liveParty.PartyTimes.guildNextRefreshTime[HOUSE_FIRST_MAGIC_GUILD] = Time::fromTicks(701);
    EXPECT_THROW((void) stage(), std::exception);
    liveParty.PartyTimes.guildNextRefreshTime[HOUSE_FIRST_MAGIC_GUILD] = Time();

    liveParty.PartyTimes.bountyHuntNextGenTime[HOUSE_FIRST_TOWN_HALL] = Time::fromTicks(702);
    EXPECT_THROW((void) stage(), std::exception);
    EXPECT_EQ(liveParty.PartyTimes.bountyHuntNextGenTime[HOUSE_FIRST_TOWN_HALL].ticks(), 702);
    EXPECT_EQ(liveNpcs.pNPCData[70].name, "Prior NPC");
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
    indoor.chest.rawActiveDelta = indoorDeltaWithActors(1);
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
    outdoorSeed.chest.activeDeltaName = "o0.ddm";
    outdoorSeed.chest.rawActiveDelta = outdoorDeltaWithActors(2);
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
    lateInvalid.sections.back().payload[0] = 3; // Unsupported SCLK version with valid outer CRCs.
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
    const mm6::ChestTitleState priorChest = mm6::chestTitleState();
    struct RestoreChest {
        mm6::ChestTitleState prior;
        ~RestoreChest() { mm6::chestTitleState() = prior; }
    } restoreChest{priorChest};
    mm6::ChestTitleState titleChest;
    titleChest.ready = true;
    titleChest.lcgState = 0x12345678;
    titleChest.generatedArtifactFlags[7] = 1;
    mm6::chestTitleState() = titleChest;
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
    EXPECT_EQ(decoded.chest.rawActiveDelta, sidecarBefore);
    EXPECT_EQ(decoded.chest.lcgState, 0x12345678);
    EXPECT_EQ(location.mm6DeltaBytes, sidecarBefore);
    EXPECT_EQ(prior.at("d0.dlv").str(), "prior:d0.dlv");
    EXPECT_EQ(npcs.pNPCData[0].name, "Runtime NPC");

    mm6::resetChestTitleState(&mm6::chestTitleState());
    EXPECT_THROW((void) mm6::captureKnownWorldFromActiveMap(
        seedFields(), npcs, prior, names, GameTitle::MM6), std::exception);
    mm6::chestTitleState() = titleChest;

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
