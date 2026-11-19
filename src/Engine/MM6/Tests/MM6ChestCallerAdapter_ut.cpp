#include <array>
#include <memory>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestCallerAdapter.h"

UNIT_TEST(MM6ChestCallerAdapter, ClassifiesSourcesWithoutDrawing) {
    EXPECT_EQ(mm6::chestRewardSource(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, 19),
              mm6::ChestRewardSource::Empty);
    EXPECT_EQ(mm6::chestRewardSource(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, 20),
              mm6::ChestRewardSource::Gold);
    EXPECT_EQ(mm6::chestRewardSource(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, 59),
              mm6::ChestRewardSource::Gold);
    EXPECT_EQ(mm6::chestRewardSource(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, 60),
              mm6::ChestRewardSource::AnyItem);
    EXPECT_EQ(mm6::chestRewardSource(GameTitle::MM6, ITEM_TREASURE_LEVEL_7, 0),
              mm6::ChestRewardSource::DirectArtifact);
    EXPECT_TRUE(mm6::needsDirectChestArtifactPicker(GameTitle::MM6, ITEM_TREASURE_LEVEL_7));
    EXPECT_FALSE(mm6::needsDirectChestArtifactPicker(GameTitle::MM7, ITEM_TREASURE_LEVEL_7));
    EXPECT_FALSE(mm6::chestRewardSource(GameTitle::MM7, ITEM_TREASURE_LEVEL_4, 60));
    EXPECT_FALSE(mm6::chestRewardSource(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, 100));
    EXPECT_FALSE(mm6::chestRewardSource(GameTitle::MM6, ITEM_TREASURE_LEVEL_INVALID, 60));
}

UNIT_TEST(MM6ChestCallerAdapter, StagesSixGoldBandsAndMapsIds) {
    const std::array<ItemId, 6> expectedIds = {
        ITEM_GOLD_SMALL, ITEM_GOLD_SMALL, ITEM_GOLD_MEDIUM,
        ITEM_GOLD_MEDIUM, ITEM_GOLD_LARGE, ITEM_GOLD_LARGE};
    const std::array<int, 6> bases = {50, 100, 200, 500, 1000, 2000};
    const std::array<int, 6> spans = {51, 101, 301, 501, 1001, 3001};
    for (int i = 0; i < 6; ++i) {
        auto result = mm6::pickChestGold(GameTitle::MM6, static_cast<ItemTreasureLevel>(i + 1), 0);
        ASSERT_TRUE(result);
        std::uint32_t state = 0;
        EXPECT_EQ(result->itemId, expectedIds[i]);
        EXPECT_EQ(result->amount, bases[i] + static_cast<int>(mm6::nextChestRandom15(state) % spans[i]));
        EXPECT_EQ(result->nextRngState, state);
    }
    EXPECT_FALSE(mm6::pickChestGold(GameTitle::MM6, ITEM_TREASURE_LEVEL_7, 0));
    EXPECT_FALSE(mm6::pickChestGold(GameTitle::MM7, ITEM_TREASURE_LEVEL_4, 0));
    EXPECT_EQ(mm6::mm6ChestItemId(GameTitle::MM6, 400), static_cast<ItemId>(400));
    EXPECT_FALSE(mm6::mm6ChestItemId(GameTitle::MM6, 0));
    EXPECT_FALSE(mm6::mm6ChestItemId(GameTitle::MM6, 581));
    EXPECT_FALSE(mm6::mm6ChestItemId(GameTitle::MM7, 400));
}

UNIT_TEST(MM6ChestCallerAdapter, AdaptsTemplateAndStandardRows) {
    auto table = std::make_unique<ItemTable>();
    auto &data = table->items[static_cast<ItemId>(200)];
    data.type = ITEM_TYPE_RING;
    data.identifyAndRepairDifficulty = 5;
    data.uChanceByTreasureLvl[ITEM_TREASURE_LEVEL_4] = 100;
    table->standardEnchantmentChanceForEquipment[ITEM_TREASURE_LEVEL_4] = 100;
    table->standardEnchantments[ATTRIBUTE_INTELLIGENCE].chanceByItemType[ITEM_TYPE_RING] = 3;
    table->standardEnchantments[ATTRIBUTE_PERSONALITY].chanceByItemType[ITEM_TYPE_RING] = 2;
    for (auto level : table->standardEnchantmentRangeByTreasureLevel.indices())
        table->standardEnchantmentRangeByTreasureLevel[level] = {4, 6};

    std::array<std::uint8_t, 30> flags{};
    auto staged = mm6::stageChestAnyFromItemTable(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, *table, flags, 0);
    ASSERT_TRUE(staged);
    EXPECT_EQ(staged->itemId, 200);
    EXPECT_EQ(staged->flags, 0u);
    EXPECT_GT(staged->standardAttributeIndex, 0);
    EXPECT_GE(staged->standardStrength, 4);
    EXPECT_LE(staged->standardStrength, 6);
    EXPECT_EQ(staged->specialEnchantmentId, 0);
    EXPECT_EQ(flags[0], 0);
    EXPECT_EQ(data.uChanceByTreasureLvl[ITEM_TREASURE_LEVEL_4], 100);

    data.uChanceByTreasureLvl[ITEM_TREASURE_LEVEL_4] = -1;
    EXPECT_FALSE(mm6::stageChestAnyFromItemTable(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, *table, flags, 0));
    data.uChanceByTreasureLvl[ITEM_TREASURE_LEVEL_4] = 100;
    data.type = ITEM_TYPE_INVALID;
    EXPECT_FALSE(mm6::stageChestAnyFromItemTable(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, *table, flags, 0));
    data.type = ITEM_TYPE_RING;
    EXPECT_FALSE(mm6::stageChestAnyFromItemTable(GameTitle::MM7, ITEM_TREASURE_LEVEL_4, *table, flags, 0));
}

UNIT_TEST(MM6ChestCallerAdapter, AdaptsSpecialAndWandRows) {
    auto table = std::make_unique<ItemTable>();
    auto &data = table->items[static_cast<ItemId>(200)];
    data.type = ITEM_TYPE_RING;
    data.uChanceByTreasureLvl[ITEM_TREASURE_LEVEL_4] = 100;
    table->standardEnchantmentChanceForEquipment[ITEM_TREASURE_LEVEL_4] = 1;
    table->specialEnchantmentChanceForEquipment[ITEM_TREASURE_LEVEL_4] = 99;
    table->specialEnchantments[static_cast<ItemEnchantment>(5)].enchantmentLevel = 2;
    table->specialEnchantments[static_cast<ItemEnchantment>(5)].chanceByItemType[ITEM_TYPE_RING] = 4;
    for (auto level : table->standardEnchantmentRangeByTreasureLevel.indices())
        table->standardEnchantmentRangeByTreasureLevel[level] = {1, 1};
    std::array<std::uint8_t, 30> flags{};
    auto special = mm6::stageChestAnyFromItemTable(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, *table, flags, 0);
    ASSERT_TRUE(special);
    EXPECT_EQ(special->itemId, 200);
    EXPECT_EQ(special->specialEnchantmentId, 5);

    auto &wand = table->items[static_cast<ItemId>(135)];
    data.uChanceByTreasureLvl[ITEM_TREASURE_LEVEL_4] = 0;
    wand.uChanceByTreasureLvl[ITEM_TREASURE_LEVEL_4] = 100;
    wand.type = ITEM_TYPE_WAND;
    wand.damageMod = 50;
    auto charged = mm6::stageChestAnyFromItemTable(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, *table, flags, 0);
    ASSERT_TRUE(charged);
    EXPECT_EQ(charged->itemId, 135);
    EXPECT_GE(charged->numCharges, 50);
    EXPECT_LE(charged->numCharges, 55);
    EXPECT_EQ(wand.damageMod, 50);
    wand.damageMod = 256;
    EXPECT_FALSE(mm6::stageChestAnyFromItemTable(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, *table, flags, 0));
}

UNIT_TEST(MM6ChestCallerAdapter, StagesShippedItemTables) {
    const auto data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    auto table = std::make_unique<ItemTable>();
    table->loadMm6Tables(data->iconsData("stditems.txt"), data->iconsData("spcitems.txt"),
                         data->iconsData("items.txt"), data->iconsData("rnditems.txt"));
    std::array<int, 6> weightSums{};
    for (int id = 1; id <= 399; ++id)
        for (int tier = 0; tier < 6; ++tier)
            weightSums[tier] += table->items[static_cast<ItemId>(id)].uChanceByTreasureLvl[static_cast<ItemTreasureLevel>(tier + 1)];
    EXPECT_EQ(weightSums, (std::array<int, 6>{437, 827, 959, 970, 928, 805}));

    std::array<std::uint8_t, 30> flags{};
    for (int tier = 1; tier <= 6; ++tier) {
        for (std::uint32_t seed = 0; seed < 32; ++seed) {
            auto result = mm6::stageChestAnyFromItemTable(
                GameTitle::MM6, static_cast<ItemTreasureLevel>(tier), *table, flags, seed);
            ASSERT_TRUE(result) << "tier=" << tier << " seed=" << seed;
            EXPECT_TRUE(mm6::mm6ChestItemId(GameTitle::MM6, result->itemId));
            EXPECT_GE(result->drawsConsumed, 2);
            EXPECT_EQ(flags[0], 0);
        }
    }
}
