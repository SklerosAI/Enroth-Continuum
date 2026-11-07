#include <cstdint>
#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PlainInventoryProjection.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    TableScope() {
        pItemTable = table.get();
        const ItemId scroll = static_cast<ItemId>(221);
        table->items[scroll].iconName = "scroll";
        table->items[scroll].type = ITEM_TYPE_SPELL_SCROLL;
        table->itemSizes[scroll] = Sizei(1, 1);
        const ItemId wand = static_cast<ItemId>(135);
        table->items[wand].iconName = "wand";
        table->items[wand].type = ITEM_TYPE_WAND;
        table->itemSizes[wand] = Sizei(1, 1);
    }

    ~TableScope() { pItemTable = previous; }
};

mm6::RawCharacterInventory twoItems() {
    mm6::RawCharacterInventory raw;
    raw.items[137].setWord(0, 221);
    raw.items[137].setWord(20, 1);
    raw.items[137][26] = 0x5a;
    raw.gridWords[0] = 138;
    raw.items[0].setWord(0, 135);
    raw.items[0].setWord(16, 3);
    raw.items[0][25] = 5;
    raw.equipmentWords[1] = 1;
    raw.items[50][27] = 0xa5; // Unreferenced physical bytes remain owned by the sidecar.
    return raw;
}

} // namespace

UNIT_TEST(MM6PlainInventoryProjection, FinalSlotGridAndEquipmentRoundTrip) {
    TableScope scope;
    const auto source = twoItems();
    auto projection = mm6::stagePlainInventory(source, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    EXPECT_EQ(projection->inventory().capacity(), 138);
    EXPECT_EQ(projection->inventory().entry(137)->itemId, static_cast<ItemId>(221));
    EXPECT_EQ(projection->inventory().entry(ITEM_SLOT_MAIN_HAND).index(), 0);
    EXPECT_EQ(mm6::capturePlainInventory(*projection, *scope.table, GameTitle::MM6), source);

    projection->inventory().entry(137)->SetBroken();
    Item moved = projection->inventory().take(projection->inventory().entry(137));
    ASSERT_TRUE(projection->inventory().tryRestoreAt(137, Pointi(1, 0), moved));
    auto changed = mm6::capturePlainInventory(*projection, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(changed);
    EXPECT_EQ(changed->gridWords[0], 0);
    EXPECT_EQ(changed->gridWords[1], 138);
    EXPECT_EQ(changed->items[137].conditionRaw(), 3);
    EXPECT_EQ(changed->items[137][26], 0x5a);
    EXPECT_EQ(changed->items[50][27], 0xa5);

    projection->inventory().take(projection->inventory().entry(137));
    EXPECT_FALSE(mm6::capturePlainInventory(*projection, *scope.table, GameTitle::MM6));
}

UNIT_TEST(MM6PlainInventoryProjection, RejectsMalformedReferencesAndUnmappedItems) {
    TableScope scope;
    auto raw = twoItems();
    raw.gridWords[0] = 139;
    EXPECT_FALSE(mm6::stagePlainInventory(raw, *scope.table, GameTitle::MM6));
    raw = twoItems();
    raw.gridWords[1] = 0xffffffffu;
    EXPECT_FALSE(mm6::stagePlainInventory(raw, *scope.table, GameTitle::MM6));
    raw = twoItems();
    raw.equipmentWords[0] = 138;
    EXPECT_FALSE(mm6::stagePlainInventory(raw, *scope.table, GameTitle::MM6));
    raw = twoItems();
    raw.items[137].setWord(4, 2);
    EXPECT_FALSE(mm6::stagePlainInventory(raw, *scope.table, GameTitle::MM6));
    raw = twoItems();
    EXPECT_FALSE(mm6::stagePlainInventory(raw, *scope.table, GameTitle::MM7));

    raw = twoItems();
    scope.table->itemSizes[static_cast<ItemId>(221)] = Sizei(2, 1);
    raw.gridWords[1] = 0xffffffffu; // Canonical continuation of the first grid cell.
    auto wide = mm6::stagePlainInventory(raw, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(wide);
    EXPECT_EQ(mm6::capturePlainInventory(*wide, *scope.table, GameTitle::MM6), raw);

    Item wand = wide->inventory().take(wide->inventory().entry(ITEM_SLOT_MAIN_HAND));
    ASSERT_TRUE(wide->inventory().tryRestoreAt(0, ITEM_SLOT_OFF_HAND, wand));
    EXPECT_FALSE(mm6::capturePlainInventory(*wide, *scope.table, GameTitle::MM6));

    auto unchanged = mm6::stagePlainInventory(raw, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(unchanged);
    scope.table->items[static_cast<ItemId>(221)].type = ITEM_TYPE_POTION;
    EXPECT_FALSE(mm6::capturePlainInventory(*unchanged, *scope.table, GameTitle::MM6));
}
