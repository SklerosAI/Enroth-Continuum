#include <cstdint>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestOuterPlan.h"

namespace {

mm6::ChestOuterInput inputWithMarker(std::uint32_t seed) {
    mm6::ChestOuterInput input;
    input.title = GameTitle::MM6;
    input.mapTreasureLevel = MAP_TREASURE_LEVEL_1;
    input.rngState = seed;
    input.itemIds[0][0] = -1;
    return input;
}

}  // namespace

UNIT_TEST(MM6ChestOuterPlan, MarkerFreeEntryStillConsumesOneDraw) {
    auto input = inputWithMarker(0);
    input.itemIds[0][0] = 0;
    auto result = mm6::stageChestOuterIds(input, {}, {});
    ASSERT_TRUE(result);
    EXPECT_EQ(result->nextRngState, 2531011u);
    EXPECT_EQ(result->markersVisited, 0);
    EXPECT_EQ(result->itemIds, input.itemIds);
    EXPECT_EQ(input.rngState, 0u);
}

UNIT_TEST(MM6ChestOuterPlan, FirstProbabilityCarriesAcrossMarkers) {
    auto input = inputWithMarker(51);
    input.itemIds[0][1] = -1;
    auto result = mm6::stageChestOuterIds(input, {}, {});
    ASSERT_TRUE(result);
    EXPECT_EQ(result->markersVisited, 2);
    EXPECT_EQ(result->emptyAttempts, 2);
    EXPECT_EQ(result->rewardsGenerated, 0);
    EXPECT_EQ(result->itemIds[0][0], 0);
    EXPECT_EQ(result->itemIds[0][1], 0);
    EXPECT_EQ(result->nextRngState, 2842859534u);
    EXPECT_EQ(input.itemIds[0][0], -1);
    EXPECT_EQ(input.itemIds[0][1], -1);
}

UNIT_TEST(MM6ChestOuterPlan, GoldFirstAttemptStagesRngAndSlot) {
    auto input = inputWithMarker(4);
    auto result = mm6::stageChestOuterIds(input, {}, {});
    ASSERT_TRUE(result);
    EXPECT_EQ(result->itemIds[0][0], std::to_underlying(ITEM_GOLD_SMALL));
    EXPECT_EQ(result->generatedGoldAmounts[0][0], 79);
    EXPECT_EQ(result->rewardsGenerated, 1);
    EXPECT_EQ(result->nextRngState, 2172805640u);
}

UNIT_TEST(MM6ChestOuterPlan, FullChestStopsBeforeAnotherProbabilityDraw) {
    auto input = inputWithMarker(7);
    input.itemIds[0].fill(1);
    input.itemIds[0][0] = -1;
    int calls = 0;
    auto any = [&calls](ItemTreasureLevel tier, std::uint32_t state)
        -> std::optional<mm6::ChestOuterReward> {
        ++calls;
        EXPECT_EQ(tier, ITEM_TREASURE_LEVEL_1);
        EXPECT_EQ(state, 3144650392u);
        return mm6::ChestOuterReward{200, state};
    };
    auto result = mm6::stageChestOuterIds(input, any, {});
    ASSERT_TRUE(result);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(result->itemIds[0][0], 200);
    EXPECT_EQ(result->rewardsGenerated, 1);
    EXPECT_EQ(result->nextRngState, 3144650392u);
}

UNIT_TEST(MM6ChestOuterPlan, InvalidOrUnprovedSourcesRejectWithoutCallerMutation) {
    auto input = inputWithMarker(8);  // First probability enters ANY.
    EXPECT_FALSE(mm6::stageChestOuterIds(input, {}, {}));
    EXPECT_EQ(input.itemIds[0][0], -1);
    input.itemIds[19][139] = -8;
    int calls = 0;
    auto any = [&calls](ItemTreasureLevel, std::uint32_t state)
        -> std::optional<mm6::ChestOuterReward> {
        ++calls;
        return mm6::ChestOuterReward{200, state};
    };
    EXPECT_FALSE(mm6::stageChestOuterIds(input, any, {}));
    EXPECT_EQ(calls, 0);  // Whole input is validated before the entry draw or callback.
    EXPECT_EQ(input.itemIds[0][0], -1);
    input.itemIds[19][139] = 0;
    input.title = GameTitle::MM7;
    EXPECT_FALSE(mm6::stageChestOuterIds(input, any, {}));
    EXPECT_EQ(calls, 0);
}
