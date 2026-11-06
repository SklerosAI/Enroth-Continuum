#include <cstddef>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Objects/Chest.h"

UNIT_TEST(MM6ChestGenerationGuard, BlocksMM6BeforeInheritedChestMutation) {
    EXPECT_EQ(mm6::chestGenerationRoute(GameTitle::MM6), mm6::ChestGenerationRoute::Blocked);
    const size_t chestCount = vChests.size();
    EXPECT_THROW(mm6::generateChestItemsForTitle(GameTitle::MM6), std::runtime_error);
    EXPECT_EQ(vChests.size(), chestCount);
}

UNIT_TEST(MM6ChestGenerationGuard, RoutesMM7AndBlocksUnknownTitles) {
    EXPECT_EQ(mm6::chestGenerationRoute(GameTitle::MM7), mm6::ChestGenerationRoute::InheritedMM7);
    EXPECT_EQ(mm6::chestGenerationRoute(static_cast<GameTitle>(2)), mm6::ChestGenerationRoute::Blocked);
}

UNIT_TEST(MM6ChestGenerationGuard, UsesMM6MarkerRemap) {
    // These cells differ from inherited MM7 remapping.
    auto tier4 = mm6::chestTreasureLevelRangeForMarker(-4, MAP_TREASURE_LEVEL_2);
    ASSERT_TRUE(tier4.has_value());
    EXPECT_EQ(tier4->front(), ITEM_TREASURE_LEVEL_2);
    EXPECT_EQ(tier4->back(), ITEM_TREASURE_LEVEL_3);

    auto tier5 = mm6::chestTreasureLevelRangeForMarker(-5, MAP_TREASURE_LEVEL_1);
    ASSERT_TRUE(tier5.has_value());
    EXPECT_EQ(tier5->front(), ITEM_TREASURE_LEVEL_2);
    EXPECT_EQ(tier5->back(), ITEM_TREASURE_LEVEL_3);

    auto tier6 = mm6::chestTreasureLevelRangeForMarker(-6, MAP_TREASURE_LEVEL_1);
    ASSERT_TRUE(tier6.has_value());
    EXPECT_EQ(tier6->front(), ITEM_TREASURE_LEVEL_3);
    EXPECT_EQ(tier6->back(), ITEM_TREASURE_LEVEL_3);

    auto artifact = mm6::chestTreasureLevelRangeForMarker(-7, MAP_TREASURE_LEVEL_7);
    ASSERT_TRUE(artifact.has_value());
    EXPECT_EQ(artifact->front(), ITEM_TREASURE_LEVEL_7);
    EXPECT_EQ(artifact->back(), ITEM_TREASURE_LEVEL_7);
}

UNIT_TEST(MM6ChestGenerationGuard, RejectsUnprovedMarkerAndMapLevels) {
    EXPECT_FALSE(mm6::chestTreasureLevelRangeForMarker(0, MAP_TREASURE_LEVEL_1));
    EXPECT_FALSE(mm6::chestTreasureLevelRangeForMarker(-8, MAP_TREASURE_LEVEL_1));
    EXPECT_FALSE(mm6::chestTreasureLevelRangeForMarker(-1, static_cast<MapTreasureLevel>(-1)));
    EXPECT_FALSE(mm6::chestTreasureLevelRangeForMarker(-1, static_cast<MapTreasureLevel>(7)));
}
