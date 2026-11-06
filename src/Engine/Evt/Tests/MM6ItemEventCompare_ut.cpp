#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6ItemEventCompare.h"

UNIT_TEST(MM6ItemEventCompare, FindsSelectedInventoryOrHeldItem) {
    Character character;
    Item held;
    held.itemId = ITEM_NULL;
    EXPECT_FALSE(mm6::hasEventItem(character, held, 17, 1));

    Item carried;
    carried.itemId = static_cast<ItemId>(1);
    ASSERT_TRUE(character.inventory.equip(ITEM_SLOT_RING1, carried));
    EXPECT_TRUE(mm6::hasEventItem(character, held, 17, 1));
    EXPECT_FALSE(mm6::hasEventItem(character, held, 17, 580));

    held.itemId = static_cast<ItemId>(580);
    EXPECT_TRUE(mm6::hasEventItem(character, held, 17, 580));
    EXPECT_FALSE(mm6::hasEventItem(character, held, 17, 2));
}

UNIT_TEST(MM6ItemEventCompare, InvalidIdsRejectWithoutChangingItems) {
    Character character;
    Item held;
    held.itemId = static_cast<ItemId>(500);
    EXPECT_THROW((void) mm6::hasEventItem(character, held, 16, 500), std::runtime_error);
    EXPECT_THROW((void) mm6::hasEventItem(character, held, 17, 0), std::runtime_error);
    EXPECT_THROW((void) mm6::hasEventItem(character, held, 17, 581), std::runtime_error);
    EXPECT_EQ(held.itemId, static_cast<ItemId>(500));
    EXPECT_FALSE(character.inventory.find(static_cast<ItemId>(500)));
}
