#include <array>
#include <cstdint>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestPlacement.h"
#include "Engine/MM6/MM6ChestTypeLayout.h"

namespace {

std::array<std::uint8_t, 292> retainedShape() {
    std::array<std::uint8_t, 292> data{};
    data[0] = 8;
    for (int index = 0; index < 8; ++index) {
        const int row = 4 + index * 36;
        data[row + 32] = 14;  // The retained metadata is not the runtime grid.
        data[row + 33] = 10;
        data[row + 34] = index + 1;
    }
    return data;
}

}  // namespace

UNIT_TEST(MM6ChestTypeLayout, UsesOriginalRuntimeDimensionsAndIconsForAllTypes) {
    const auto data = retainedShape();
    const std::array<int, 8> expectedX = {42, 18, 18, 42, 42, 42, 18, 42};
    const std::array<int, 8> expectedY = {34, 30, 30, 34, 34, 34, 30, 34};
    for (int type = 0; type < 8; ++type) {
        const auto layout = mm6::chestTypeLayout(GameTitle::MM6, type, data);
        ASSERT_TRUE(layout);
        EXPECT_EQ(layout->width, 9);
        EXPECT_EQ(layout->height, 9);
        EXPECT_EQ(layout->offsetX, expectedX[type]);
        EXPECT_EQ(layout->offsetY, expectedY[type]);
        EXPECT_EQ(layout->iconId, type + 1);
        EXPECT_EQ(layout->iconName, "chest0" + std::to_string(type + 1));
    }
}

UNIT_TEST(MM6ChestTypeLayout, RejectsForeignOrMalformedTableWithoutMutation) {
    auto data = retainedShape();
    const auto original = data;
    EXPECT_FALSE(mm6::chestTypeLayout(GameTitle::MM7, 0, data));
    EXPECT_FALSE(mm6::chestTypeLayout(GameTitle::MM6, -1, data));
    EXPECT_FALSE(mm6::chestTypeLayout(GameTitle::MM6, 8, data));
    EXPECT_FALSE(mm6::chestTypeLayout(GameTitle::MM6, 0, std::span(data).first(291)));
    data[0] = 7;
    EXPECT_FALSE(mm6::chestTypeLayout(GameTitle::MM6, 0, data));
    data = original;
    data[4 + 7 * 36 + 34] = 0;  // A late malformed row rejects type zero too.
    const auto malformed = data;
    EXPECT_FALSE(mm6::chestTypeLayout(GameTitle::MM6, 0, data));
    EXPECT_EQ(data, malformed);
}

UNIT_TEST(MM6ChestTypeLayout, SuppliesAreaBoundToDetachedPlacement) {
    const auto data = retainedShape();
    const auto layout = mm6::chestTypeLayout(GameTitle::MM6, 0, data);
    ASSERT_TRUE(layout);
    mm6::ChestPlacementInput input;
    input.title = GameTitle::MM6;
    input.width = layout->width;
    input.height = layout->height;
    input.items[81] = {1, 14, 14, 0};
    const auto result = mm6::placeChestItemsDetached(input);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->placedCount, 0);
    EXPECT_EQ(result->items[81].itemId, 1);
    EXPECT_EQ(result->grid[0], 0);
}
