#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Tables/ChestTable.h"

#include "Library/Binary/BinaryTags.h"

UNIT_TEST(MM7ChestSnapshot, SkipsInvalidRawItemsAndGridIndices) {
    initializeChests();
    Chest_MM7 wire{};
    wire.items[80].itemId = std::to_underlying(ITEM_LAST_VALID) + 1;
    wire.items[136].itemId = std::to_underlying(ITEM_FIRST_RANDOM) - 1;
    wire.items[137].itemId = std::to_underlying(ITEM_FIRST_RANDOM);
    wire.items[138].itemId = std::to_underlying(ITEM_CRUDE_LONGSWORD);
    wire.items[139].itemId = std::to_underlying(ITEM_LAST_VALID) + 1;
    wire.inventoryMatrix[0] = 32767;
    wire.inventoryMatrix[1] = 81;

    ChestInventory inventory;
    reconstruct(wire, &inventory, tags::context(0));

    ASSERT_EQ(inventory.size(), 2);
    EXPECT_FALSE(static_cast<bool>(inventory.entry(80)));
    EXPECT_FALSE(static_cast<bool>(inventory.entry(136)));
    const InventoryConstEntry marker = inventory.entry(137);
    ASSERT_TRUE(static_cast<bool>(marker));
    EXPECT_EQ(marker->itemId, ITEM_FIRST_RANDOM);
    EXPECT_EQ(marker.zone(), INVENTORY_ZONE_STASH);
    const InventoryConstEntry valid = inventory.entry(138);
    ASSERT_TRUE(static_cast<bool>(valid));
    EXPECT_EQ(valid->itemId, ITEM_CRUDE_LONGSWORD);
    EXPECT_EQ(valid.zone(), INVENTORY_ZONE_STASH);
    EXPECT_FALSE(static_cast<bool>(inventory.entry(139)));
}
