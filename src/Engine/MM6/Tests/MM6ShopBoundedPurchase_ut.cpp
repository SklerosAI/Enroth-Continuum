#include <cstdint>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ShopBoundedPurchase.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    ItemTable table;

    TableScope() { pItemTable = &table; }
    ~TableScope() { pItemTable = previous; }
};

mm6::ShopStockRecord stockRecord(std::uint32_t id) {
    mm6::ShopStockRecord record;
    record.setWord(0, id);
    record.setWord(20, 1);
    record[25] = 7;
    record[26] = 0x5a;
    record[27] = 0xa5;
    return record;
}

} // namespace

UNIT_TEST(MM6ShopBoundedPurchase, PlainAndStandardItemsCarrySourceThroughPurchase) {
    TableScope scope;
    const ItemId scroll = static_cast<ItemId>(221);
    const ItemId armor = static_cast<ItemId>(10);
    scope.table.items[scroll].iconName = "scroll";
    scope.table.items[scroll].type = ITEM_TYPE_SPELL_SCROLL;
    scope.table.itemSizes[scroll] = Sizei(1, 1);
    scope.table.items[armor].iconName = "armor";
    scope.table.items[armor].type = ITEM_TYPE_ARMOUR;
    scope.table.itemSizes[armor] = Sizei(2, 2);

    CharacterInventory inventory = CharacterInventory::forMM6();
    auto plain = stockRecord(221);
    const auto originalPlain = plain;
    auto plainIndex = mm6::tryPurchaseBoundedStockItem(plain, inventory, scope.table, GameTitle::MM6);
    ASSERT_TRUE(plainIndex);
    ASSERT_TRUE(inventory.entry(*plainIndex));
    EXPECT_EQ(inventory.entry(*plainIndex)->mm6RawRecord, originalPlain);
    for (int i = 0; i < 25; ++i)
        EXPECT_EQ(plain[i], 0) << i;
    for (int i = 25; i < 28; ++i)
        EXPECT_EQ(plain[i], originalPlain[i]) << i;

    auto standard = stockRecord(10);
    standard.setWord(4, 12);
    standard.setWord(8, 7);
    const auto originalStandard = standard;
    auto standardIndex = mm6::tryPurchaseBoundedStockItem(standard, inventory, scope.table, GameTitle::MM6);
    ASSERT_TRUE(standardIndex);
    ASSERT_TRUE(inventory.entry(*standardIndex));
    EXPECT_EQ(inventory.entry(*standardIndex)->mm6RawRecord, originalStandard);
    EXPECT_EQ(inventory.size(), 2);
}

UNIT_TEST(MM6ShopBoundedPurchase, RejectsUnsupportedRecordsWithoutMutatingEitherOwner) {
    TableScope scope;
    const ItemId scroll = static_cast<ItemId>(221);
    scope.table.items[scroll].iconName = "scroll";
    scope.table.items[scroll].type = ITEM_TYPE_SPELL_SCROLL;
    scope.table.itemSizes[scroll] = Sizei(1, 1);
    CharacterInventory inventory = CharacterInventory::forMM6();
    const auto original = stockRecord(221);

    auto rejected = [&](mm6::ShopStockRecord candidate, GameTitle title = GameTitle::MM6) {
        const auto before = candidate;
        EXPECT_FALSE(mm6::tryPurchaseBoundedStockItem(candidate, inventory, scope.table, title));
        EXPECT_EQ(candidate, before);
        EXPECT_EQ(inventory.size(), 0);
    };
    rejected(original, GameTitle::MM7);
    auto sold = original;
    sold.setWord(0, 0);
    rejected(sold);
    auto equipped = original;
    equipped[24] = 1;
    rejected(equipped);
    auto unsupportedBonus = original;
    unsupportedBonus.setWord(12, 1);
    rejected(unsupportedBonus);

    scope.table.items[scroll].rarity = RARITY_ARTIFACT;
    rejected(original);
    scope.table.items[scroll].rarity = RARITY_COMMON;
    scope.table.items[scroll].type = ITEM_TYPE_POTION;
    rejected(original);
    scope.table.items[scroll].type = ITEM_TYPE_GOLD;
    rejected(original);
}

UNIT_TEST(MM6ShopBoundedPurchase, FullBackpackRejectsAfterItemStageWithoutStockClear) {
    TableScope scope;
    const ItemId scroll = static_cast<ItemId>(221);
    scope.table.items[scroll].iconName = "scroll";
    scope.table.items[scroll].type = ITEM_TYPE_SPELL_SCROLL;
    scope.table.itemSizes[scroll] = Sizei(1, 1);
    const auto source = stockRecord(221);
    auto staged = mm6::stageBoundedItem(source, scope.table, GameTitle::MM6);
    ASSERT_TRUE(staged);

    CharacterInventory inventory = CharacterInventory::forMM6();
    for (int y = 0; y < 9; ++y)
        for (int x = 0; x < 14; ++x)
            ASSERT_TRUE(inventory.tryAdd(Pointi(x, y), mm6::boundedItem(*staged)));
    ASSERT_EQ(inventory.size(), 126);

    auto candidate = source;
    EXPECT_FALSE(mm6::tryPurchaseBoundedStockItem(candidate, inventory, scope.table, GameTitle::MM6));
    EXPECT_EQ(candidate, source);
    EXPECT_EQ(inventory.size(), 126);
    EXPECT_FALSE(inventory.findSpace(mm6::boundedItem(*staged)));
}

UNIT_TEST(MM6ShopBoundedPurchase, InvalidFootprintAndWrongTableRejectBeforeMutation) {
    TableScope scope;
    const ItemId scroll = static_cast<ItemId>(221);
    scope.table.items[scroll].iconName = "scroll";
    scope.table.items[scroll].type = ITEM_TYPE_SPELL_SCROLL;
    CharacterInventory inventory = CharacterInventory::forMM6();
    const auto source = stockRecord(221);
    auto candidate = source;
    EXPECT_FALSE(mm6::tryPurchaseBoundedStockItem(candidate, inventory, scope.table, GameTitle::MM6));
    EXPECT_EQ(candidate, source);
    EXPECT_EQ(inventory.size(), 0);

    scope.table.itemSizes[scroll] = Sizei(1, 1);
    ItemTable otherTable = scope.table;
    EXPECT_FALSE(mm6::tryPurchaseBoundedStockItem(candidate, inventory, otherTable, GameTitle::MM6));
    EXPECT_EQ(candidate, source);
    EXPECT_EQ(inventory.size(), 0);
}
