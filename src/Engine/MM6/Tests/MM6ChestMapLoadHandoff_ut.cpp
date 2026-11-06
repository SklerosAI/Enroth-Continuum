#include <array>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestMapLoadHandoff.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Tables/ChestTable.h"

#include "Utility/Memory/Blob.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    TableScope() {
        pItemTable = table.get();
        initializeChests();
        for (int id = 1; id <= 580; ++id)
            table->itemSizes[static_cast<ItemId>(id)] = Sizei(1, 1);
        table->items[static_cast<ItemId>(1)].type = ITEM_TYPE_RING;
        auto &regular = table->items[static_cast<ItemId>(200)];
        regular.type = ITEM_TYPE_RING;
        for (int tier = 1; tier <= 6; ++tier)
            regular.uChanceByTreasureLvl[static_cast<ItemTreasureLevel>(tier)] = 100;
        for (auto level : table->standardEnchantmentRangeByTreasureLevel.indices())
            table->standardEnchantmentRangeByTreasureLevel[level] = {1, 1};
    }
    ~TableScope() { pItemTable = previous; }
};

mm6::ChestMapLoadOwners ownersWithMarker(int marker = -1) {
    mm6::ChestMapLoadOwners owners;
    owners.title = GameTitle::MM6;
    owners.rngState = 8;
    mm6::writeChest32(std::span(owners.records[0]).subspan(4, 28), 0, marker);
    owners.chests.emplace_back();  // Existing live owner must survive a failed stage.
    return owners;
}

Blob readFile(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    return Blob::fromString(std::string(std::istreambuf_iterator<char>(stream), {}));
}

void copyRecords(std::span<const std::uint8_t> section, mm6::ChestRecordSet *records) {
    ASSERT_EQ(section.size(), 4 + 20 * mm6::CHEST_WIRE_SIZE);
    for (int index = 0; index < 20; ++index)
        std::memcpy((*records)[index].data(), section.data() + 4 + index * mm6::CHEST_WIRE_SIZE,
                    mm6::CHEST_WIRE_SIZE);
}

}  // namespace

UNIT_TEST(MM6ChestMapLoadHandoff, CommitsAllTwentyChestsAndLcgTogether) {
    TableScope table;
    auto owners = ownersWithMarker();
    const auto original = owners.records;
    const auto staged = mm6::stageChestMapLoadHandoff(owners, MAP_TREASURE_LEVEL_1, *table.table, {});
    ASSERT_TRUE(staged);
    EXPECT_EQ(staged->chests.size(), 20);
    EXPECT_EQ(staged->markersVisited, 1);
    EXPECT_EQ(owners.records, original);
    EXPECT_EQ(owners.rngState, 8u);
    EXPECT_EQ(owners.chests.size(), 1);

    ASSERT_TRUE(mm6::applyChestMapLoadHandoff(&owners, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(owners.chests.size(), 20);
    EXPECT_EQ(owners.records, staged->records);
    EXPECT_EQ(owners.rngState, staged->nextRngState);
    EXPECT_EQ(owners.generatedArtifactFlags, staged->generatedArtifactFlags);
    EXPECT_GE(mm6::readChest32(std::span(owners.records[0]).subspan(4, 28), 0), 0);
}

UNIT_TEST(MM6ChestMapLoadHandoff, DirectArtifactCommitsOnlySafeMm6Flag) {
    TableScope table;
    auto owners = ownersWithMarker(-7);
    ASSERT_TRUE(mm6::applyChestMapLoadHandoff(&owners, MAP_TREASURE_LEVEL_1, *table.table, {}));
    const int id = mm6::readChest32(std::span(owners.records[0]).subspan(4, 28), 0);
    EXPECT_GE(id, 400);
    EXPECT_LE(id, 429);
    EXPECT_EQ(owners.generatedArtifactFlags[id - 400], 1);
    EXPECT_EQ(owners.chests.size(), 20);
}

UNIT_TEST(MM6ChestMapLoadHandoff, LateProjectionAndForeignTitleFailWithoutCommit) {
    TableScope table;
    auto owners = ownersWithMarker();
    mm6::writeChest32(std::span(owners.records[19]).subspan(4, 28), 0, 250);
    table.table->itemSizes[static_cast<ItemId>(250)] = Sizei();
    const auto original = owners.records;
    EXPECT_FALSE(mm6::applyChestMapLoadHandoff(&owners, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(owners.records, original);
    EXPECT_EQ(owners.rngState, 8u);
    EXPECT_EQ(owners.generatedArtifactFlags, (std::array<std::uint8_t, 30>{}));
    EXPECT_EQ(owners.chests.size(), 1);

    owners.title = GameTitle::MM7;
    EXPECT_FALSE(mm6::applyChestMapLoadHandoff(&owners, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(owners.records, original);
    owners.title = GameTitle::MM6;
    mm6::writeChest32(std::span(owners.records[19]).subspan(4, 28), 0, 581);
    const auto malformed = owners.records;
    EXPECT_FALSE(mm6::applyChestMapLoadHandoff(&owners, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(owners.records, malformed);
}

UNIT_TEST(MM6ChestMapLoadHandoff, LateFailureRollsBackGeneratedArtifactAndRng) {
    TableScope table;
    auto owners = ownersWithMarker(-7);
    mm6::writeChest32(std::span(owners.records[19]).subspan(4, 28), 0, 250);
    table.table->itemSizes[static_cast<ItemId>(250)] = Sizei();
    const auto original = owners.records;
    EXPECT_FALSE(mm6::applyChestMapLoadHandoff(&owners, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(owners.records, original);
    EXPECT_EQ(owners.rngState, 8u);
    EXPECT_EQ(owners.generatedArtifactFlags, (std::array<std::uint8_t, 30>{}));
    EXPECT_EQ(owners.chests.size(), 1);
}

UNIT_TEST(MM6ChestMapLoadHandoff, MarkerFreeMapRejectsMalformedArtifactFlags) {
    TableScope table;
    auto owners = ownersWithMarker(0);
    owners.generatedArtifactFlags[0] = 2;
    const auto original = owners.records;
    EXPECT_FALSE(mm6::applyChestMapLoadHandoff(&owners, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(owners.generatedArtifactFlags[0], 2);
    EXPECT_EQ(owners.records, original);
    EXPECT_EQ(owners.rngState, 8u);
    owners.generatedArtifactFlags[0] = 0;
    std::array<std::uint8_t, 30> malformedOwned{};
    malformedOwned[0] = 2;
    EXPECT_FALSE(mm6::applyChestMapLoadHandoff(&owners, MAP_TREASURE_LEVEL_1, *table.table,
                                                malformedOwned));
    EXPECT_EQ(owners.records, original);
}

UNIT_TEST(MM6ChestMapLoadHandoff, RetainedBaselinesStageWithSyntheticTable) {
    const char *indoorDirectory = std::getenv("ENROTH_MM6_INDOOR_DECODED_DIR");
    const char *outdoorDirectory = std::getenv("ENROTH_MM6_OUTDOOR_DECODED_DIR");
    if (!indoorDirectory || !outdoorDirectory)
        GTEST_SKIP() << "Set retained decoded MM6 map directories";
    TableScope table;
    int indoorCount = 0;
    int outdoorCount = 0;
    for (const auto &entry : std::filesystem::directory_iterator(indoorDirectory)) {
        if (entry.path().extension() != ".dlv")
            continue;
        auto basePath = entry.path();
        basePath.replace_extension(".blv");
        const auto base = deserializeMM6IndoorBase(readFile(basePath));
        const auto delta = deserializeMM6IndoorDelta(readFile(entry.path()), base);
        auto owners = ownersWithMarker(0);
        const Blob &chests = delta.section(MM6IndoorSectionKind::Chests).bytes;
        copyRecords({static_cast<const std::uint8_t *>(chests.data()), chests.size()}, &owners.records);
        ASSERT_TRUE(mm6::stageChestMapLoadHandoff(owners, MAP_TREASURE_LEVEL_6, *table.table, {}))
            << "indoor ordinal=" << indoorCount;
        ++indoorCount;
    }
    for (const auto &entry : std::filesystem::directory_iterator(outdoorDirectory)) {
        if (entry.path().extension() != ".ddm")
            continue;
        const auto delta = decodeMM6OutdoorDelta(readFile(entry.path()));
        auto owners = ownersWithMarker(0);
        copyRecords(delta.wire.part(mm6::OutdoorDeltaPart::Chests), &owners.records);
        ASSERT_TRUE(mm6::stageChestMapLoadHandoff(owners, MAP_TREASURE_LEVEL_6, *table.table, {}))
            << "outdoor ordinal=" << outdoorCount;
        ++outdoorCount;
    }
    EXPECT_EQ(indoorCount, 52);
    EXPECT_EQ(outdoorCount, 15);
}
