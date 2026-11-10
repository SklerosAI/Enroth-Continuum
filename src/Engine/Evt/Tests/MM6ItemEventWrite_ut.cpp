#include <array>
#include <stdexcept>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6ItemEventWrite.h"
#include "Engine/Tables/ItemTable.h"

UNIT_TEST(MM6ItemEventWrite, BoundedEmptyCursorRewardHasOnlyOriginalFields) {
    Item held;
    Item created = mm6::makeEventItem(17, 1);
    EXPECT_EQ(created.itemId, static_cast<ItemId>(1));
    EXPECT_EQ(created.flags, ITEM_IDENTIFIED);
    EXPECT_EQ(created.potionPower, 0);
    EXPECT_EQ(created.numCharges, 0);
    EXPECT_EQ(created.maxCharges, 0);
    EXPECT_EQ(held.itemId, ITEM_NULL);

    EXPECT_THROW((void) mm6::makeEventItem(17, 400), std::runtime_error);
    EXPECT_THROW((void) mm6::makeEventItem(17, 429), std::runtime_error);
    EXPECT_THROW((void) mm6::makeEventItem(17, 0), std::runtime_error);
    EXPECT_THROW((void) mm6::makeEventItem(17, 581), std::runtime_error);
    EXPECT_THROW((void) mm6::makeEventItem(16, 1), std::runtime_error);
}

UNIT_TEST(MM6ItemEventWrite, RelocatesExactHeldItemActiveFirstThenOriginalFallbackOrder) {
    ItemTable localTable;
    ItemTable *previous = pItemTable;
    struct RestoreTable {
        ItemTable *previous;
        ~RestoreTable() { pItemTable = previous; }
    } restore{previous};
    pItemTable = &localTable;
    localTable.itemSizes[static_cast<ItemId>(1)] = Sizei(1, 1);

    std::array<Character, 4> characters;
    std::array<Character *, 4> party{&characters[0], &characters[1], &characters[2], &characters[3]};
    Item held;
    held.itemId = static_cast<ItemId>(1);
    held.numCharges = 7;
    held.flags = ITEM_BROKEN;
    EXPECT_EQ(mm6::planEventItemRelocation(party, 2, held), party[2]);

    Sizei grid = characters[2].inventory.gridSize();
    for (int y = 0; y < grid.h; ++y)
        for (int x = 0; x < grid.w; ++x)
            ASSERT_TRUE(characters[2].inventory.tryAdd(Pointi(x, y), held));
    EXPECT_EQ(mm6::planEventItemRelocation(party, 2, held), party[0]);

    InventoryEntry stored = mm6::relocateEventItemToInventory(characters[0], held);
    ASSERT_TRUE(stored);
    EXPECT_EQ(stored->itemId, static_cast<ItemId>(1));
    EXPECT_EQ(stored->numCharges, 7);
    EXPECT_EQ(stored->flags, ITEM_BROKEN);
    EXPECT_EQ(held.itemId, ITEM_NULL);
    EXPECT_EQ(characters[2].inventory.size(), grid.w * grid.h);
}

UNIT_TEST(MM6ItemEventWrite, FullPartyPreflightLeavesCursorAndInventoryUnchanged) {
    ItemTable localTable;
    ItemTable *previous = pItemTable;
    struct RestoreTable {
        ItemTable *previous;
        ~RestoreTable() { pItemTable = previous; }
    } restore{previous};
    pItemTable = &localTable;
    localTable.itemSizes[static_cast<ItemId>(1)] = Sizei(1, 1);

    std::array<Character, 4> characters;
    std::array<Character *, 4> party{&characters[0], &characters[1], &characters[2], &characters[3]};
    Item held;
    held.itemId = static_cast<ItemId>(1);
    held.numCharges = 9;
    for (Character &character : characters) {
        Sizei grid = character.inventory.gridSize();
        for (int y = 0; y < grid.h; ++y)
            for (int x = 0; x < grid.w; ++x)
                ASSERT_TRUE(character.inventory.tryAdd(Pointi(x, y), held));
    }
    EXPECT_THROW((void) mm6::planEventItemRelocation(party, 3, held), std::runtime_error);
    EXPECT_THROW((void) mm6::relocateEventItemToInventory(characters[0], held), std::runtime_error);
    EXPECT_EQ(held.itemId, static_cast<ItemId>(1));
    EXPECT_EQ(held.numCharges, 9);
    for (Character &character : characters)
        EXPECT_EQ(character.inventory.size(), character.inventory.gridSize().w * character.inventory.gridSize().h);
    EXPECT_THROW((void) mm6::makeEventItem(17, 400), std::runtime_error);
    EXPECT_EQ(held.itemId, static_cast<ItemId>(1));
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

UNIT_TEST(MM6ItemEventWrite, DuplicateBackpackRemovalUsesFirstGridCellNotRecordOrder) {
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
    item.numCharges = 7;
    ASSERT_TRUE(first.inventory.tryAdd(Pointi(0, 0), item));
    item.numCharges = 10;
    ASSERT_TRUE(second.inventory.tryAdd(Pointi(0, 1), item)); // Lower record index, later grid cell.
    item.numCharges = 20;
    ASSERT_TRUE(second.inventory.tryAdd(Pointi(13, 0), item)); // Higher record index, earlier grid cell.
    Item held;
    held.itemId = static_cast<ItemId>(1);
    held.numCharges = 99;
    std::array<Character *, 2> selected{&first, &second};
    std::vector<mm6::ItemRemovalStep> plan = mm6::planEventItemRemoval(selected, held, 17, 1);
    ASSERT_EQ(plan.size(), 2);
    ASSERT_TRUE(plan[1].inventoryEntry);
    EXPECT_EQ(plan[1].inventoryEntry->numCharges, 20);
    EXPECT_FALSE(plan[1].held);
    EXPECT_THROW((void) mm6::planEventItemRemoval(selected, held, 17, 0), std::runtime_error);
    EXPECT_EQ(first.inventory.size(), 1);
    EXPECT_EQ(second.inventory.size(), 2);
    EXPECT_EQ(held.numCharges, 99);

    Item removed = second.inventory.take(plan[1].inventoryEntry);
    EXPECT_EQ(removed.numCharges, 20);
    ASSERT_TRUE(second.inventory.entry(Pointi(0, 1)));
    EXPECT_EQ(second.inventory.entry(Pointi(0, 1))->numCharges, 10);
    EXPECT_FALSE(second.inventory.entry(Pointi(13, 0)));
    EXPECT_EQ(held.numCharges, 99);
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
