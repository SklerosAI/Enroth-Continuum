#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestRecordTransaction.h"

namespace {

mm6::ChestRecordGenerationInput inputWithMarker(std::uint32_t seed, int marker = -1) {
    mm6::ChestRecordGenerationInput input;
    input.title = GameTitle::MM6;
    input.mapTreasureLevel = MAP_TREASURE_LEVEL_1;
    input.rngState = seed;
    mm6::writeChest32(std::span(input.records[0]).subspan(4, 28), 0, marker);
    return input;
}

std::array<std::uint8_t, 292> chestTypes() {
    std::array<std::uint8_t, 292> data{};
    data[0] = 8;
    for (int i = 0; i < 8; ++i)
        data[4 + i * 36 + 34] = i + 1;
    return data;
}

std::unique_ptr<ItemTable> oneItemTable(ItemId id, ItemType type) {
    auto table = std::make_unique<ItemTable>();
    auto &item = table->items[id];
    item.type = type;
    table->items[static_cast<ItemId>(1)].type = type;  // Zero-remainder fallback may select item 1.
    for (int tier = 1; tier <= 6; ++tier)
        item.uChanceByTreasureLvl[static_cast<ItemTreasureLevel>(tier)] = 100;
    for (auto level : table->standardEnchantmentRangeByTreasureLevel.indices())
        table->standardEnchantmentRangeByTreasureLevel[level] = {1, 1};
    return table;
}

}  // namespace

UNIT_TEST(MM6ChestRecordTransaction, MarkerFreeStillDrawsAndPreservesEveryRecordByte) {
    auto input = inputWithMarker(0, 0);
    mm6::writeChest32(std::span(input.records[1]).subspan(4, 28), 0, 200);
    input.records[1][4 + 27] = 0x7f;  // Opaque fixed-item tail is preserved.
    const auto original = input.records;
    auto table = oneItemTable(static_cast<ItemId>(200), ITEM_TYPE_RING);
    const auto result = mm6::stageChestRecordGeneration(input, *table);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->records, original);
    EXPECT_EQ(result->nextRngState, 2531011u);
    EXPECT_EQ(result->markersVisited, 0);
    EXPECT_EQ(input.records, original);
}

UNIT_TEST(MM6ChestRecordTransaction, GoldStagesFullWireItemAndLeavesCallerUntouched) {
    const auto input = inputWithMarker(4);
    const auto original = input.records;
    auto table = oneItemTable(static_cast<ItemId>(200), ITEM_TYPE_RING);
    const auto result = mm6::stageChestRecordGeneration(input, *table);
    ASSERT_TRUE(result);
    const auto item = std::span(result->records[0]).subspan(4, 28);
    EXPECT_EQ(mm6::readChest32(item, 0), std::to_underlying(ITEM_GOLD_SMALL));
    EXPECT_EQ(mm6::readChest32(item, 12), 79);
    EXPECT_EQ(mm6::readChest32(item, 4), 0);
    EXPECT_EQ(mm6::readChest32(item, 16), 0);
    EXPECT_EQ(result->rewardsGenerated, 1);
    EXPECT_EQ(input.records, original);
}

UNIT_TEST(MM6ChestRecordTransaction, AnyUsesRetainedTableCallerAndKnownFieldOffsets) {
    const auto input = inputWithMarker(8);
    auto table = oneItemTable(static_cast<ItemId>(200), ITEM_TYPE_RING);
    const auto result = mm6::stageChestRecordGeneration(input, *table);
    ASSERT_TRUE(result);
    const auto item = std::span(result->records[0]).subspan(4, 28);
    EXPECT_EQ(mm6::readChest32(item, 0), 200);
    EXPECT_EQ(mm6::readChest32(item, 4), 0);
    EXPECT_EQ(mm6::readChest32(item, 8), 0);
    EXPECT_EQ(mm6::readChest32(item, 12), 0);
    EXPECT_EQ(mm6::readChest32(item, 16), 0);
    EXPECT_EQ(mm6::readChest32(item, 20), 1);
    EXPECT_EQ(item[26], 0);
    EXPECT_EQ(item[27], 0);
    EXPECT_EQ(result->rewardsGenerated, 1);
}

UNIT_TEST(MM6ChestRecordTransaction, DirectArtifactUsesExplicitSafeThirtyIdPolicy) {
    const auto input = inputWithMarker(0, -7);
    auto table = oneItemTable(static_cast<ItemId>(200), ITEM_TYPE_RING);
    const auto result = mm6::stageChestRecordGeneration(input, *table);
    ASSERT_TRUE(result);
    const int id = mm6::readChest32(std::span(result->records[0]).subspan(4, 28), 0);
    EXPECT_GE(id, 400);
    EXPECT_LE(id, 429);  // Original unsafe candidate ID 430 is deliberately excluded.
    EXPECT_EQ(result->generatedArtifactFlags[id - 400], 1);

    auto capped = input;
    for (int i = 0; i < 13; ++i)
        capped.partyOwnedArtifactFlags[i] = 1;
    const auto cleared = mm6::stageChestRecordGeneration(capped, *table);
    ASSERT_TRUE(cleared);
    EXPECT_EQ(mm6::readChest32(std::span(cleared->records[0]).subspan(4, 28), 0), 0);
    EXPECT_EQ(cleared->rewardsGenerated, 0);
    EXPECT_EQ(cleared->generatedArtifactFlags, capped.generatedArtifactFlags);
}

UNIT_TEST(MM6ChestRecordTransaction, AllSixMarkerLevelsStageAcrossSevenMapLevels) {
    auto table = oneItemTable(static_cast<ItemId>(200), ITEM_TYPE_RING);
    for (int marker = -1; marker >= -6; --marker) {
        for (int mapLevel = 0; mapLevel <= 6; ++mapLevel) {
            auto input = inputWithMarker(8, marker);
            input.mapTreasureLevel = static_cast<MapTreasureLevel>(mapLevel);
            const auto original = input.records;
            const auto result = mm6::stageChestRecordGeneration(input, *table);
            ASSERT_TRUE(result) << "marker=" << marker << " mapLevel=" << mapLevel;
            EXPECT_EQ(result->markersVisited, 1);
            for (int slot = 0; slot < 140; ++slot)
                EXPECT_GE(mm6::readChest32(std::span(result->records[0]).subspan(4 + slot * 28, 28), 0), 0);
            EXPECT_EQ(input.records, original);
        }
    }
}

UNIT_TEST(MM6ChestRecordTransaction, AdditionalAttemptsKeepGoldAndAnyFieldsInTheirSlots) {
    auto table = oneItemTable(static_cast<ItemId>(200), ITEM_TYPE_RING);
    bool foundMixed = false;
    for (std::uint32_t seed = 0; seed < 128 && !foundMixed; ++seed) {
        const auto input = inputWithMarker(seed);
        const auto result = mm6::stageChestRecordGeneration(input, *table);
        ASSERT_TRUE(result) << "seed=" << seed;
        int goldCount = 0;
        int anyCount = 0;
        for (int slot = 0; slot < 140; ++slot) {
            const auto item = std::span(result->records[0]).subspan(4 + slot * 28, 28);
            const int id = mm6::readChest32(item, 0);
            if (id == std::to_underlying(ITEM_GOLD_SMALL)) {
                EXPECT_GE(mm6::readChest32(item, 12), 50);
                EXPECT_LE(mm6::readChest32(item, 12), 100);
                EXPECT_EQ(mm6::readChest32(item, 20), 0);
                ++goldCount;
            } else if (id == 1 || id == 200) {
                EXPECT_EQ(mm6::readChest32(item, 12), 0);
                EXPECT_EQ(mm6::readChest32(item, 20), 1);
                ++anyCount;
            } else {
                EXPECT_EQ(id, 0);
            }
        }
        foundMixed = goldCount > 0 && anyCount > 0;
        EXPECT_EQ(input.records[0][4], static_cast<std::uint8_t>(-1));
    }
    EXPECT_TRUE(foundMixed);
}

UNIT_TEST(MM6ChestRecordTransaction, FirstOpenPlacementStagesGridFlagsAndWandCharges) {
    mm6::ChestRecord record{};
    mm6::writeChest32(std::span(record).subspan(4, 28), 0, 135);
    record[4 + 27] = 0x5a;  // Opaque tail must survive placement.
    const auto original = record;
    const auto types = chestTypes();
    const auto icon = [](int) -> std::optional<mm6::ChestIconSize> {
        return mm6::ChestIconSize{14, 14};
    };
    const auto placed = mm6::stageFirstChestRecordPlacement(GameTitle::MM6, record, types, 0, icon);
    ASSERT_TRUE(placed);
    EXPECT_EQ(placed->placedCount, 1);
    EXPECT_EQ(mm6::readChest16(placed->record, 2), 2);
    EXPECT_GE(mm6::readChest32(std::span(placed->record).subspan(4, 28), 16), 10);
    EXPECT_LE(mm6::readChest32(std::span(placed->record).subspan(4, 28), 16), 30);
    const auto grid = mm6::decodeChestGridWire(GameTitle::MM6, placed->record);
    ASSERT_TRUE(grid);
    EXPECT_EQ(std::count(grid->begin(), grid->end(), 1), 1);
    EXPECT_EQ(placed->record[4 + 27], 0x5a);
    EXPECT_EQ(record, original);
}

UNIT_TEST(MM6ChestRecordTransaction, RejectsMalformedAndForeignInputsAtomically) {
    auto input = inputWithMarker(8);
    auto table = oneItemTable(static_cast<ItemId>(200), ITEM_TYPE_RING);
    input.records[0][4 + 26] = 1;  // A generated target must start with blank non-ID bytes.
    const auto malformed = input.records;
    EXPECT_FALSE(mm6::stageChestRecordGeneration(input, *table));
    EXPECT_EQ(input.records, malformed);
    input.records[0][4 + 26] = 0;
    mm6::writeChest16(input.records[0], mm6::CHEST_GRID_OFFSET, 1);
    EXPECT_FALSE(mm6::stageChestRecordGeneration(input, *table));
    input = inputWithMarker(8);
    input.title = GameTitle::MM7;
    EXPECT_FALSE(mm6::stageChestRecordGeneration(input, *table));

    mm6::ChestRecord record{};
    mm6::writeChest32(std::span(record).subspan(4, 28), 0, -1);
    const auto types = chestTypes();
    auto icon = [](int) -> std::optional<mm6::ChestIconSize> { return mm6::ChestIconSize{14, 14}; };
    EXPECT_FALSE(mm6::stageFirstChestRecordPlacement(GameTitle::MM6, record, types, 0, icon));
    EXPECT_FALSE(mm6::stageFirstChestRecordPlacement(GameTitle::MM7, record, types, 0, icon));
}
