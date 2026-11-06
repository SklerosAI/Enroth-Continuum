#include <cstddef>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Objects/Chest.h"
#include "Engine/MM6/MM6ChestBaseItemPicker.h"

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

UNIT_TEST(MM6ChestGenerationGuard, BasePickerKeepsOriginalZeroRemainder) {
    std::array<int, 399> weights{};
    weights[4] = 3;  // Item 5.
    weights[9] = 2;  // Item 10.
    std::array<std::uint8_t, 30> artifacts{};

    auto zero = mm6::pickChestBaseItemId(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, weights, artifacts, 4);
    ASSERT_TRUE(zero.has_value());
    EXPECT_EQ(zero->itemId, 1);  // The original remainder-zero branch ignores item 1's zero weight.
    EXPECT_EQ(zero->drawsConsumed, 2);  // Artifact candidate is drawn even below tier 6.
    EXPECT_EQ(zero->nextRngState, 3323539102u);
    EXPECT_EQ(zero->generatedArtifactFlags, artifacts);

    auto weighted = mm6::pickChestBaseItemId(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, weights, artifacts, 0);
    ASSERT_TRUE(weighted.has_value());
    EXPECT_EQ(weighted->itemId, 10);
    EXPECT_EQ(weighted->drawsConsumed, 2);
    EXPECT_EQ(weighted->nextRngState, 505908858u);
}

UNIT_TEST(MM6ChestGenerationGuard, BasePickerStagesTierSixArtifactAndFallback) {
    std::array<int, 399> weights{};
    weights[4] = 3;
    weights[9] = 2;
    std::array<std::uint8_t, 30> artifacts{};

    auto granted = mm6::pickChestBaseItemId(GameTitle::MM6, ITEM_TREASURE_LEVEL_6, weights, artifacts, 17);
    ASSERT_TRUE(granted.has_value());
    EXPECT_EQ(granted->itemId, 404);
    EXPECT_EQ(granted->drawsConsumed, 2);
    EXPECT_EQ(granted->nextRngState, 1743419155u);
    EXPECT_EQ(granted->generatedArtifactFlags[4], 1);
    EXPECT_EQ(artifacts[4], 0);  // A detached result never mutates the caller's state.

    artifacts[4] = 1;
    auto duplicate = mm6::pickChestBaseItemId(GameTitle::MM6, ITEM_TREASURE_LEVEL_6, weights, artifacts, 17);
    ASSERT_TRUE(duplicate.has_value());
    EXPECT_EQ(duplicate->itemId, 5);
    EXPECT_EQ(duplicate->drawsConsumed, 3);
    EXPECT_EQ(duplicate->nextRngState, 1967211914u);
    EXPECT_EQ(duplicate->generatedArtifactFlags, artifacts);

    artifacts.fill(0);
    for (int i = 5; i < 18; ++i)
        artifacts[i] = 1;
    auto limited = mm6::pickChestBaseItemId(GameTitle::MM6, ITEM_TREASURE_LEVEL_6, weights, artifacts, 17);
    ASSERT_TRUE(limited.has_value());
    EXPECT_EQ(limited->itemId, 5);
    EXPECT_EQ(limited->generatedArtifactFlags, artifacts);
}

UNIT_TEST(MM6ChestGenerationGuard, BasePickerFailsClosedForInvalidInputsAndMM7) {
    std::array<int, 399> weights{};
    weights[4] = 3;
    std::array<std::uint8_t, 30> artifacts{};
    EXPECT_FALSE(mm6::pickChestBaseItemId(GameTitle::MM7, ITEM_TREASURE_LEVEL_4, weights, artifacts, 17));
    EXPECT_FALSE(mm6::pickChestBaseItemId(GameTitle::MM6, ITEM_TREASURE_LEVEL_7, weights, artifacts, 17));
    weights[4] = -1;
    EXPECT_FALSE(mm6::pickChestBaseItemId(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, weights, artifacts, 17));
    weights[4] = 0;
    EXPECT_FALSE(mm6::pickChestBaseItemId(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, weights, artifacts, 17));
    weights[4] = 3;
    artifacts[0] = 2;
    EXPECT_FALSE(mm6::pickChestBaseItemId(GameTitle::MM6, ITEM_TREASURE_LEVEL_4, weights, artifacts, 17));
}
