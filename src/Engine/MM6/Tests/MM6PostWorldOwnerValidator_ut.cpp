#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestLiveOwnerAdapter.h"
#include "Engine/MM6/MM6PostWorldOwnerValidator.h"
#include "Engine/MM6/MM6RawSessionStage.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/MapTable.h"

namespace {

void put32(std::string *bytes, size_t offset, std::uint32_t value) {
    for (int i = 0; i < 4; ++i)
        (*bytes)[offset + i] = static_cast<char>(value >> (8 * i));
}

std::vector<std::uint8_t> sourceDelta(bool indoor) {
    const size_t chestOffset = indoor ? 8 + 875 + 3 * 4 : 8 + 2 * 88 * 11 + 3 * 4;
    const size_t doors = indoor ? 200 * 80 : 0;
    std::string bytes(chestOffset + 20 * mm6::CHEST_WIRE_SIZE + doors + 200 + 56, '\0');
    put32(&bytes, chestOffset - 4, 20);
    put32(&bytes, chestOffset + 4, static_cast<std::uint32_t>(-1));
    bytes[chestOffset + 20 * mm6::CHEST_WIRE_SIZE + doors] = static_cast<char>(0x35);
    return {bytes.begin(), bytes.end()};
}

MM6IndoorSnapshot indoorBaseShape() {
    MM6IndoorSnapshot base;
    base.sections.push_back({MM6IndoorSectionKind::DoorSlots, Blob(), 200});
    return base;
}

mm6::ChestRecordSet recordsFromDelta(const std::vector<std::uint8_t> &bytes,
                                    const MM6IndoorSnapshot *base) {
    mm6::ChestRecordSet records{};
    if (base) {
        const auto wire = deserializeMM6IndoorDelta(Blob::copy(bytes.data(), bytes.size()), *base);
        const Blob &section = wire.section(MM6IndoorSectionKind::Chests).bytes;
        EXPECT_TRUE(mm6::copyChestRecords(
            {static_cast<const std::uint8_t *>(section.data()), section.size()}, &records));
    } else {
        const auto wire = mm6::decodeOutdoorDelta(bytes);
        EXPECT_TRUE(mm6::copyChestRecords(wire.part(mm6::OutdoorDeltaPart::Chests), &records));
    }
    return records;
}

std::vector<Chest> baselineChests(const mm6::ChestRecordSet &records) {
    std::vector<Chest> chests(20);
    for (int index = 0; index < 20; ++index)
        reconstruct(projectMM6ChestRecord(records[index]), &chests[index],
                    tags::context<int>(index));
    return chests;
}

struct Fixture {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> items = std::make_unique<ItemTable>();
    std::unique_ptr<MapTable> maps = std::make_unique<MapTable>();
    std::unique_ptr<mm6::StagedRawSessionFields> staged =
        std::make_unique<mm6::StagedRawSessionFields>();
    MM6IndoorSnapshot indoorBase;
    std::vector<std::uint8_t> finalDelta;
    std::vector<Chest> finalChests;
    mm6::ChestTitleState finalTitle;
    PersistentVariables persistent{};
    mm6::PostWorldOwnerView live;

    explicit Fixture(bool indoor) {
        pItemTable = items.get();
        initializeChests();
        for (int id = 1; id <= 580; ++id)
            items->itemSizes[static_cast<ItemId>(id)] = Sizei(1, 1);
        auto &item = items->items[static_cast<ItemId>(200)];
        item.type = ITEM_TYPE_RING;
        for (int tier = 1; tier <= 6; ++tier)
            item.uChanceByTreasureLvl[static_cast<ItemTreasureLevel>(tier)] = 100;
        for (auto tier : items->standardEnchantmentRangeByTreasureLevel.indices())
            items->standardEnchantmentRangeByTreasureLevel[tier] = {1, 1};

        const std::string mapName = indoor ? "synthetic.blv" : "synthetic.odm";
        const auto original = sourceDelta(indoor);
        maps->pInfos[MAP_EMERALD_ISLAND].fileName = mapName;
        maps->pInfos[MAP_EMERALD_ISLAND].mapTreasureLevel = MAP_TREASURE_LEVEL_1;
        staged->raw.saved.metadata.header.mapFileName = mapName;
        if (indoor) {
            indoorBase = indoorBaseShape();
            mm6::StagedIndoorMap active;
            active.mapName = mapName;
            active.base = indoorBaseShape();
            active.delta = decodeMM6IndoorRuntimeDelta(
                Blob::copy(original.data(), original.size()), active.base);
            staged->indoorPlan.emplace();
            staged->indoorPlan->mapName = mapName;
            staged->indoorPlan->savedDelta = decodeMM6IndoorRuntimeDelta(
                Blob::copy(original.data(), original.size()), active.base);
            staged->indoorPlan->activeSidecar = decodeMM6IndoorRuntimeDelta(
                Blob::copy(original.data(), original.size()), active.base);
            staged->raw.activeMap = std::move(active);
        } else {
            mm6::StagedOutdoorMap active;
            active.mapName = mapName;
            active.delta = decodeMM6OutdoorDelta(Blob::copy(original.data(), original.size()));
            staged->outdoorPlan.emplace();
            staged->outdoorPlan->mapName = mapName;
            staged->outdoorPlan->savedDelta = active.delta;
            staged->outdoorPlan->activeSidecar = active.delta;
            staged->raw.activeMap = std::move(active);
        }
        staged->persistentEvents = mm6::stagePersistentEventBoundary(
            staged->raw.activeMap, GameTitle::MM6);
        staged->world.rawActiveDelta = original;
        if (!mm6::restoreChestTitleState(GameTitle::MM6,
                &staged->world.chestTitle, 8, {}))
            throw std::runtime_error("cannot seed synthetic MM6 chest title state");
        staged->raw.saved.chest = mm6::captureChestSaveFields(
            staged->world.chestTitle, indoor ? "synthetic.dlv" : "synthetic.ddm",
            original, GameTitle::MM6);

        finalDelta = original;
        finalChests = baselineChests(recordsFromDelta(finalDelta,
            indoor ? &indoorBase : nullptr));
        finalTitle = staged->world.chestTitle;
        mm6::ChestLiveBinding binding{GameTitle::MM6,
            indoor ? LEVEL_INDOOR : LEVEL_OUTDOOR, &finalChests,
            &finalDelta, &finalTitle, indoor ? &indoorBase : nullptr};
        if (!mm6::applyChestLiveOwnerHandoff(
                binding, MAP_TREASURE_LEVEL_1, *items, {}))
            throw std::runtime_error("cannot stage synthetic MM6 chest generation");
        for (Chest &chest : finalChests)
            chest.position = Vec3f(1, 2, 3);  // UpdateChestPositions runs before the hook.

        live.loadedMapId = MAP_EMERALD_ISLAND;
        live.levelType = indoor ? LEVEL_INDOOR : LEVEL_OUTDOOR;
        live.maps = maps.get();
        live.items = items.get();
        live.chests = finalChests;
        live.decodedDelta = finalDelta;
        live.persistent = &persistent;
        live.chestTitle = &finalTitle;
    }

    ~Fixture() { pItemTable = previous; }
};

} // namespace

UNIT_TEST(MM6PostWorldOwnerValidator, OutdoorFinalGenerationAndOwnersAgree) {
    Fixture fixture(false);
    EXPECT_NO_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM6));
    EXPECT_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM7), std::exception);
    fixture.live.loadedMapId = MAP_HARMONDALE;
    EXPECT_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM6), std::exception);
    fixture.live.loadedMapId = MAP_EMERALD_ISLAND;
    fixture.staged->raw.saved.chest.activeDeltaName = "another.ddm";
    EXPECT_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6PostWorldOwnerValidator, IndoorRejectsLateChestAndEventDivergence) {
    Fixture fixture(true);
    EXPECT_NO_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM6));
    fixture.persistent.decorVars[124] = 1;
    EXPECT_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM6), std::exception);
    fixture.persistent.decorVars[124] = 0;
    fixture.finalChests.back().flags |= CHEST_TRAPPED;
    EXPECT_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM6), std::exception);
    fixture.finalChests.back().flags &= ~CHEST_TRAPPED;
    fixture.finalTitle.lcgState++;
    EXPECT_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6PostWorldOwnerValidator, RejectsLateSidecarAndPidMismatchWithoutMutation) {
    Fixture fixture(false);
    const auto prior = fixture.finalDelta;
    fixture.finalDelta.back() ^= 1;
    EXPECT_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM6), std::exception);
    EXPECT_EQ(fixture.finalDelta.back(), prior.back() ^ 1);
    fixture.finalDelta = prior;
    std::array<Actor, 1> actors;
    actors[0].id = 0;
    fixture.live.actors = actors;
    EXPECT_THROW(mm6::requireRawV2PostWorldOwnerAgreement(
        *fixture.staged, fixture.live, GameTitle::MM6), std::exception);
}
