#include <array>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6ItemEventWrite.h"
#include "Engine/Tables/ItemTable.h"

UNIT_TEST(MM6ItemEventWrite, BoundedEmptyCursorRewardHasOnlyOriginalFields) {
    Item held;
    Item created = mm6::makeEventItem(17, 1, held);
    EXPECT_EQ(created.itemId, static_cast<ItemId>(1));
    EXPECT_EQ(created.flags, ITEM_IDENTIFIED);
    EXPECT_EQ(created.potionPower, 0);
    EXPECT_EQ(created.numCharges, 0);
    EXPECT_EQ(created.maxCharges, 0);
    EXPECT_EQ(held.itemId, ITEM_NULL);

    held.itemId = static_cast<ItemId>(2);
    EXPECT_THROW((void) mm6::makeEventItem(17, 1, held), std::runtime_error);
    held.Reset();
    EXPECT_THROW((void) mm6::makeEventItem(17, 400, held), std::runtime_error);
    EXPECT_THROW((void) mm6::makeEventItem(17, 429, held), std::runtime_error);
    EXPECT_THROW((void) mm6::makeEventItem(17, 0, held), std::runtime_error);
    EXPECT_THROW((void) mm6::makeEventItem(17, 581, held), std::runtime_error);
    EXPECT_THROW((void) mm6::makeEventItem(16, 1, held), std::runtime_error);
}

UNIT_TEST(MM6ItemEventWrite, PlansGridBeforeHeldAndPreservesEquipment) {
    ItemTable localTable;
    ItemTable *previous = pItemTable;
    struct RestoreTable {
        ItemTable *previous;
        ~RestoreTable() { pItemTable = previous; }
    } restore{previous};
    pItemTable = &localTable;
    localTable.itemSizes[static_cast<ItemId>(1)] = Sizei(1, 1);

    Character first;
    Character second;
    Item item;
    item.itemId = static_cast<ItemId>(1);
    ASSERT_TRUE(first.inventory.tryAdd(Pointi(0, 0), item));
    ASSERT_TRUE(second.inventory.equip(ITEM_SLOT_RING1, item));
    std::array<Character *, 2> selected{&first, &second};
    std::vector<mm6::ItemRemovalStep> plan = mm6::planEventItemRemoval(selected, item, 17, 1);
    ASSERT_EQ(plan.size(), 2);
    EXPECT_TRUE(plan[0].inventoryEntry);
    EXPECT_FALSE(plan[0].held);
    EXPECT_FALSE(plan[1].inventoryEntry);
    EXPECT_TRUE(plan[1].held);

    first.inventory.take(plan[0].inventoryEntry);
    EXPECT_FALSE(first.inventory.find(static_cast<ItemId>(1)));
    EXPECT_TRUE(second.inventory.entry(ITEM_SLOT_RING1));
    EXPECT_EQ(item.itemId, static_cast<ItemId>(1));
}

UNIT_TEST(MM6ItemEventWrite, RejectsDuplicateBackpackBeforeAnyRemoval) {
    ItemTable localTable;
    ItemTable *previous = pItemTable;
    struct RestoreTable {
        ItemTable *previous;
        ~RestoreTable() { pItemTable = previous; }
    } restore{previous};
    pItemTable = &localTable;
    localTable.itemSizes[static_cast<ItemId>(1)] = Sizei(1, 1);

    Character first;
    Character second;
    Item item;
    item.itemId = static_cast<ItemId>(1);
    ASSERT_TRUE(first.inventory.tryAdd(Pointi(0, 0), item));
    ASSERT_TRUE(second.inventory.tryAdd(Pointi(0, 0), item));
    ASSERT_TRUE(second.inventory.tryAdd(Pointi(1, 0), item));
    std::array<Character *, 2> selected{&first, &second};
    EXPECT_THROW((void) mm6::planEventItemRemoval(selected, Item(), 17, 1), std::runtime_error);
    EXPECT_EQ(first.inventory.size(), 1);
    EXPECT_EQ(second.inventory.size(), 2);
    EXPECT_THROW((void) mm6::planEventItemRemoval(selected, Item(), 17, 0), std::runtime_error);
}

UNIT_TEST(MM6ItemEventWrite, HeldMatchIsConsumedOnceAndMissingItemIsNoOp) {
    Character first;
    Character second;
    std::array<Character *, 2> selected{&first, &second};
    Item held;
    held.itemId = static_cast<ItemId>(1);
    std::vector<mm6::ItemRemovalStep> plan = mm6::planEventItemRemoval(selected, held, 17, 1);
    ASSERT_EQ(plan.size(), 2);
    EXPECT_TRUE(plan[0].held);
    EXPECT_FALSE(plan[1].held);
    EXPECT_FALSE(plan[0].inventoryEntry);
    EXPECT_FALSE(plan[1].inventoryEntry);

    plan = mm6::planEventItemRemoval(selected, held, 17, 2);
    ASSERT_EQ(plan.size(), 2);
    EXPECT_FALSE(plan[0].held);
    EXPECT_FALSE(plan[1].held);
    EXPECT_EQ(held.itemId, static_cast<ItemId>(1));
}
