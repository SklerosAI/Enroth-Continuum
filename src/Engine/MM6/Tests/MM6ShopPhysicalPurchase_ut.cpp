#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ShopPhysicalPurchase.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    ItemTable table;

    TableScope() {
        pItemTable = &table;
        const ItemId scroll = static_cast<ItemId>(221);
        table.items[scroll].iconName = "scroll";
        table.items[scroll].type = ITEM_TYPE_SPELL_SCROLL;
        table.itemSizes[scroll] = Sizei(1, 1);
        const ItemId armor = static_cast<ItemId>(10);
        table.items[armor].iconName = "armor";
        table.items[armor].type = ITEM_TYPE_ARMOUR;
        table.itemSizes[armor] = Sizei(2, 2);
    }
    ~TableScope() { pItemTable = previous; }
};

mm6::ItemRecord28 record(std::uint32_t id, std::uint8_t tail = 0x5a) {
    mm6::ItemRecord28 result;
    result.setWord(0, id);
    result.setWord(20, 1);
    result[26] = tail;
    result[27] = 0xa5;
    return result;
}

CharacterInventory runtimeFrom(const mm6::RawCharacterInventory &raw, const ItemTable &table) {
    auto projection = mm6::stageBoundedInventory(raw, table, GameTitle::MM6);
    if (!projection)
        throw std::runtime_error("invalid synthetic raw inventory");
    return projection->inventory();
}

} // namespace

UNIT_TEST(MM6ShopPhysicalPurchase, CopiesFullRecordAndClearsOnlySourcePrefix) {
    TableScope scope;
    mm6::ShopStockFields stock;
    stock.ready = true;
    const std::size_t physicalRow = 94 * 12; // One shared-edge physical index.
    stock.records[physicalRow] = record(221);
    const auto beforeStock = stock.records[physicalRow];
    mm6::RawInventoryFields raw;
    raw.ready = true;
    raw.characters[2].items[0][25] = 0xcc; // Free ID with retained bytes.
    raw.characters[2].items[0][26] = 0xdd;
    raw.characters[2].items[0][27] = 0xee;
    raw.characters[3].items[0][26] = 0x77;
    const auto otherCharacter = raw.characters[3];
    CharacterInventory inventory = runtimeFrom(raw.characters[2], scope.table);

    auto index = mm6::tryPurchasePhysicalStockItem(
        stock, physicalRow, raw, 2, inventory, scope.table, GameTitle::MM6);
    ASSERT_TRUE(index);
    EXPECT_EQ(*index, 0);
    EXPECT_EQ(raw.characters[2].items[0], beforeStock);
    EXPECT_EQ(raw.characters[3], otherCharacter);
    ASSERT_TRUE(inventory.entry(0));
    EXPECT_EQ(inventory.entry(0)->mm6RawRecord, beforeStock);
    EXPECT_EQ(raw.characters[2].gridWords[0], 1u);
    for (int i = 0; i < 25; ++i)
        EXPECT_EQ(stock.records[physicalRow][i], 0) << i;
    for (int i = 25; i < 28; ++i)
        EXPECT_EQ(stock.records[physicalRow][i], beforeStock[i]) << i;
}

UNIT_TEST(MM6ShopPhysicalPurchase, MultiCellPlacementAndLateFullBackpackReject) {
    TableScope scope;
    mm6::ShopStockFields stock;
    stock.ready = true;
    stock.records[11] = record(10);
    mm6::RawInventoryFields raw;
    raw.ready = true;
    CharacterInventory inventory = runtimeFrom(raw.characters[0], scope.table);

    ASSERT_TRUE(mm6::tryPurchasePhysicalStockItem(
        stock, 11, raw, 0, inventory, scope.table, GameTitle::MM6));
    EXPECT_EQ(raw.characters[0].gridWords[0], 1u);
    EXPECT_EQ(raw.characters[0].gridWords[1], std::uint32_t(-1));
    EXPECT_EQ(raw.characters[0].gridWords[14], std::uint32_t(-1));
    EXPECT_EQ(raw.characters[0].gridWords[15], std::uint32_t(-1));

    mm6::ShopStockFields fullStock;
    fullStock.ready = true;
    fullStock.records[11] = record(221);
    mm6::RawInventoryFields fullRaw;
    fullRaw.ready = true;
    for (int cell = 0; cell < 126; ++cell) {
        fullRaw.characters[0].items[cell] = record(221);
        fullRaw.characters[0].gridWords[cell] = cell + 1;
    }
    CharacterInventory fullInventory = runtimeFrom(fullRaw.characters[0], scope.table);
    const auto beforeRaw = fullRaw;
    const auto beforeStock = fullStock;
    EXPECT_FALSE(mm6::tryPurchasePhysicalStockItem(
        fullStock, 11, fullRaw, 0, fullInventory, scope.table, GameTitle::MM6));
    EXPECT_EQ(fullRaw, beforeRaw);
    EXPECT_EQ(fullStock, beforeStock);
    EXPECT_EQ(fullInventory.size(), 126);
}

UNIT_TEST(MM6ShopPhysicalPurchase, UsesLastPhysicalItemSlotAndReplacesItsOldTail) {
    TableScope scope;
    mm6::ShopStockFields stock;
    stock.ready = true;
    stock.records[1] = record(221);
    mm6::RawInventoryFields raw;
    raw.ready = true;
    for (int cell = 0; cell < 125; ++cell) {
        raw.characters[0].items[cell] = record(221);
        raw.characters[0].gridWords[cell] = cell + 1;
    }
    for (int slot = 0; slot < 12; ++slot) {
        const int index = 125 + slot;
        raw.characters[0].items[index] = record(10);
        raw.characters[0].items[index][24] = slot + 1;
        raw.characters[0].equipmentWords[slot] = index + 1;
    }
    raw.characters[0].items[137][26] = 0xee;
    CharacterInventory inventory = runtimeFrom(raw.characters[0], scope.table);
    ASSERT_EQ(inventory.size(), 137);

    auto added = mm6::tryPurchasePhysicalStockItem(
        stock, 1, raw, 0, inventory, scope.table, GameTitle::MM6);
    ASSERT_TRUE(added);
    EXPECT_EQ(*added, 137);
    EXPECT_EQ(raw.characters[0].items[137][26], 0x5a);
    EXPECT_EQ(raw.characters[0].gridWords[125], 138u);
    EXPECT_EQ(inventory.size(), 138);
}

UNIT_TEST(MM6ShopPhysicalPurchase, RejectsWrongOwnerAndRuntimeDriftWithoutMutation) {
    TableScope scope;
    mm6::ShopStockFields stock;
    stock.ready = true;
    stock.records[0] = record(221);
    mm6::RawInventoryFields raw;
    raw.ready = true;
    CharacterInventory inventory = runtimeFrom(raw.characters[0], scope.table);
    const auto beforeStock = stock;
    const auto beforeRaw = raw;

    EXPECT_FALSE(mm6::tryPurchasePhysicalStockItem(
        stock, 0, raw, 0, inventory, scope.table, GameTitle::MM7));
    EXPECT_FALSE(mm6::tryPurchasePhysicalStockItem(
        stock, stock.records.size(), raw, 0, inventory, scope.table, GameTitle::MM6));
    EXPECT_FALSE(mm6::tryPurchasePhysicalStockItem(
        stock, 0, raw, 4, inventory, scope.table, GameTitle::MM6));
    EXPECT_EQ(stock, beforeStock);
    EXPECT_EQ(raw, beforeRaw);

    auto carried = mm6::stageBoundedItem(record(221), scope.table, GameTitle::MM6);
    ASSERT_TRUE(carried);
    ASSERT_TRUE(inventory.tryAdd(mm6::boundedItem(*carried)));
    EXPECT_FALSE(mm6::tryPurchasePhysicalStockItem(
        stock, 0, raw, 0, inventory, scope.table, GameTitle::MM6));
    EXPECT_EQ(stock, beforeStock);
    EXPECT_EQ(raw, beforeRaw);
    EXPECT_EQ(inventory.size(), 1);

    CharacterInventory empty = runtimeFrom(beforeRaw.characters[0], scope.table);
    raw.characters[0].items[0] = record(221, 0x99); // Unreferenced but not physically free.
    const auto occupiedRaw = raw;
    EXPECT_FALSE(mm6::tryPurchasePhysicalStockItem(
        stock, 0, raw, 0, empty, scope.table, GameTitle::MM6));
    EXPECT_EQ(stock, beforeStock);
    EXPECT_EQ(raw, occupiedRaw);
    EXPECT_EQ(empty.size(), 0);
}
