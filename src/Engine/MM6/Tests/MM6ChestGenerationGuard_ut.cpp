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
