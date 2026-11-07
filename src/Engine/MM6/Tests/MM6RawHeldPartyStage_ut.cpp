#include <memory>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6BoundedItemProjection.h"
#include "Engine/MM6/MM6RawHeldPartyStage.h"

namespace {

struct TestItems {
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    TestItems() {
        const ItemId id = static_cast<ItemId>(100);
        table->items[id].iconName = "synthetic";
        table->items[id].type = ITEM_TYPE_ARMOUR;
        table->items[id].rarity = RARITY_COMMON;
        table->itemSizes[id] = Sizei(1, 1);
    }
};

mm6::RawHeldCursorOwner plainCursor() {
    auto owner = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    owner.record.setWord(0, 100);
    owner.record.setWord(20, 3);
    owner.record[25] = 4;
    owner.record[26] = 0xa5;
    owner.record[27] = 0x5a;
    return owner;
}

} // namespace

UNIT_TEST(MM6RawHeldPartyStage, ProjectsAndReplaysPhysicalHeldItemOnDetachedParty) {
    TestItems items;
    Party baseline;
    baseline.uNumGold = 123;
    const auto cursor = plainCursor();
    const Party staged = mm6::stageRawHeldParty(baseline, cursor, *items.table, GameTitle::MM6);
    EXPECT_EQ(staged.uNumGold, 123);
    EXPECT_EQ(staged.pPickedItem.itemId, static_cast<ItemId>(100));
    EXPECT_EQ(staged.pPickedItem.mm6RawRecord, cursor.record);
    EXPECT_EQ(baseline.pPickedItem.itemId, ITEM_NULL);
    auto projection = mm6::stageBoundedItem(cursor.record, *items.table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    mm6::setBoundedItem(*projection, staged.pPickedItem);
    EXPECT_EQ(mm6::captureBoundedItem(*projection, *items.table, GameTitle::MM6), cursor.record);
}

UNIT_TEST(MM6RawHeldPartyStage, ClearsOnlyDetachedPartyForEmptyPhysicalCursor) {
    TestItems items;
    Party baseline;
    baseline.pPickedItem.itemId = static_cast<ItemId>(100);
    auto cursor = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    cursor.record[25] = 0xa1;
    cursor.record[26] = 0xb2;
    cursor.record[27] = 0xc3;
    const Party staged = mm6::stageRawHeldParty(baseline, cursor, *items.table, GameTitle::MM6);
    EXPECT_EQ(staged.pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(staged.pPickedItem.mm6RawRecord.has_value());
    EXPECT_EQ(baseline.pPickedItem.itemId, static_cast<ItemId>(100));
    EXPECT_EQ(cursor.record[27], 0xc3);
}

UNIT_TEST(MM6RawHeldPartyStage, RejectsForeignUnreadyAndUnsupportedCursorWithoutMutation) {
    TestItems items;
    Party baseline;
    baseline.uNumGold = 321;
    auto cursor = plainCursor();
    EXPECT_THROW((void) mm6::stageRawHeldParty(baseline, cursor, *items.table, GameTitle::MM7),
                 std::exception);
    cursor.ready = false;
    EXPECT_THROW((void) mm6::stageRawHeldParty(baseline, cursor, *items.table, GameTitle::MM6),
                 std::exception);
    cursor = plainCursor();
    items.table->items[static_cast<ItemId>(100)].rarity = RARITY_ARTIFACT;
    EXPECT_THROW((void) mm6::stageRawHeldParty(baseline, cursor, *items.table, GameTitle::MM6),
                 std::exception);
    EXPECT_EQ(baseline.uNumGold, 321);
    EXPECT_EQ(baseline.pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(cursor.record[27], 0x5a);
}
