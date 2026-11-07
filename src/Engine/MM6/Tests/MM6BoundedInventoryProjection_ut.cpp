#include <cstdint>
#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6BoundedInventoryProjection.h"

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
        const ItemId armor = static_cast<ItemId>(10);
        table->items[armor].iconName = "armor";
        table->items[armor].type = ITEM_TYPE_ARMOUR;
        table->itemSizes[armor] = Sizei(1, 1);
    }

    ~TableScope() { pItemTable = previous; }
};

mm6::RawCharacterInventory threeItems() {
    mm6::RawCharacterInventory raw;
    raw.items[137].setWord(0, 221);
    raw.items[137].setWord(20, 1);
    raw.items[137][26] = 0x5a;
    raw.gridWords[0] = 138;
    raw.items[0].setWord(0, 135);
    raw.items[0].setWord(16, 3);
    raw.items[0][25] = 5;
    raw.equipmentWords[1] = 1;
    raw.items[136].setWord(0, 10);
    raw.items[136].setWord(4, 12);
    raw.items[136].setWord(8, 7);
    raw.items[136].setWord(20, 1);
    raw.items[136][26] = 0x6a;
    raw.equipmentWords[3] = 137;
    raw.items[50][27] = 0xa5; // Unreferenced physical bytes remain owned by the sidecar.
    return raw;
}

} // namespace

UNIT_TEST(MM6BoundedInventoryProjection, FinalSlotGridAndEquipmentRoundTrip) {
    TableScope scope;
    const auto source = threeItems();
    auto projection = mm6::stageBoundedInventory(source, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    EXPECT_EQ(projection->inventory().capacity(), 138);
    EXPECT_EQ(projection->inventory().entry(137)->itemId, static_cast<ItemId>(221));
    EXPECT_EQ(projection->inventory().entry(ITEM_SLOT_MAIN_HAND).index(), 0);
    EXPECT_EQ(projection->inventory().entry(ITEM_SLOT_ARMOUR).index(), 136);
    EXPECT_EQ(mm6::captureBoundedInventory(*projection, *scope.table, GameTitle::MM6), source);

    projection->inventory().entry(137)->SetBroken();
    Item moved = projection->inventory().take(projection->inventory().entry(137));
    ASSERT_TRUE(projection->inventory().tryRestoreAt(137, Pointi(1, 0), moved));
    projection->inventory().entry(136)->standardEnchantment = ATTRIBUTE_RESIST_BODY;
    projection->inventory().entry(136)->standardEnchantmentStrength = 19;
    auto changed = mm6::captureBoundedInventory(*projection, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(changed);
    EXPECT_EQ(changed->gridWords[0], 0);
    EXPECT_EQ(changed->gridWords[1], 138);
    EXPECT_EQ(changed->items[137].conditionRaw(), 3);
    EXPECT_EQ(changed->items[137][26], 0x5a);
    EXPECT_EQ(changed->items[50][27], 0xa5);
    EXPECT_EQ(changed->items[136].bonusRaw(), 14);
    EXPECT_EQ(changed->items[136].bonusStrengthRaw(), 19);
    EXPECT_EQ(changed->items[136][26], 0x6a);

    projection->inventory().take(projection->inventory().entry(137));
    auto removed = mm6::captureBoundedInventory(*projection, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(removed);
    EXPECT_EQ(removed->gridWords[1], 0);
    EXPECT_EQ(removed->items[137].itemNumberRaw(), 0);
    EXPECT_EQ(removed->items[137].conditionRaw(), 0);
    EXPECT_EQ(removed->items[137][26], 0x5a);
}

UNIT_TEST(MM6BoundedInventoryProjection, GridToPaperdollMoveKeepsRawBytes) {
    // Equipping a backpack item used to fail capture because its zone changed.
    TableScope scope;
    auto source = threeItems();
    source.items[5].setWord(0, 10);
    source.items[5].setWord(20, 1);
    source.items[5][25] = 0x11;
    source.items[5][27] = 0x77;
    source.gridWords[20] = 6;
    source.equipmentWords[3] = 0;
    source.items[136] = {};
    auto projection = mm6::stageBoundedInventory(source, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(projection);

    CharacterInventory &inventory = projection->inventory();
    Item armor = inventory.take(inventory.entry(5));
    mm6::setRawBodyLocation(&armor, ITEM_SLOT_ARMOUR);
    InventoryEntry equipped = inventory.equip(ITEM_SLOT_ARMOUR, armor);
    ASSERT_EQ(equipped.index(), 1);
    auto captured = mm6::captureBoundedInventory(*projection, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(captured);
    EXPECT_EQ(captured->gridWords[20], 0);
    EXPECT_EQ(captured->equipmentWords[3], 2);
    mm6::ItemRecord28 expected = source.items[5];
    expected[24] = 4;
    EXPECT_EQ(captured->items[1], expected);
    EXPECT_EQ(captured->items[5].itemNumberRaw(), 0);
    EXPECT_EQ(captured->items[5][24], 0);
    EXPECT_EQ(captured->items[5][25], 0x11);
    EXPECT_EQ(captured->items[5][27], 0x77);
    EXPECT_EQ(captured->items[50][27], 0xa5);

    auto restaged = mm6::stageBoundedInventory(*captured, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(restaged);
    EXPECT_EQ(restaged->inventory().entry(ITEM_SLOT_ARMOUR).index(), 1);

    // Unequipping and placing in the backpack keeps the body location byte, the original copies all 28 bytes.
    Item unequipped = restaged->inventory().take(restaged->inventory().entry(ITEM_SLOT_ARMOUR));
    ASSERT_TRUE(restaged->inventory().tryAdd(Pointi(3, 3), unequipped));
    auto backpack = mm6::captureBoundedInventory(*restaged, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(backpack);
    EXPECT_EQ(backpack->equipmentWords[3], 0);
    EXPECT_EQ(backpack->gridWords[3 * 14 + 3], 2);
    EXPECT_EQ(backpack->items[1], expected);
}

UNIT_TEST(MM6BoundedInventoryProjection, ArrivingItemsNeedRawRecordAndFreeRecord) {
    // An item arriving from another owner brings its own 28 bytes.
    TableScope scope;
    auto source = threeItems();
    auto projection = mm6::stageBoundedInventory(source, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    mm6::ItemRecord28 bought;
    bought.setWord(0, 221);
    bought[26] = 0x3c;
    Item scroll;
    scroll.itemId = static_cast<ItemId>(221);
    scroll.mm6RawRecord = bought;
    InventoryEntry added = projection->inventory().tryAdd(Pointi(5, 5), scroll);
    ASSERT_EQ(added.index(), 1);
    auto captured = mm6::captureBoundedInventory(*projection, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(captured);
    EXPECT_EQ(captured->items[1], bought);
    EXPECT_EQ(captured->gridWords[5 * 14 + 5], 2);

    added->mm6RawRecord.reset();
    EXPECT_FALSE(mm6::captureBoundedInventory(*projection, *scope.table, GameTitle::MM6));

    // An unreferenced record with an item number is not free to the original allocator.
    source.items[1].setWord(0, 221);
    auto occupied = mm6::stageBoundedInventory(source, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(occupied);
    ASSERT_EQ(occupied->inventory().tryAdd(Pointi(5, 5), scroll).index(), 1);
    EXPECT_FALSE(mm6::captureBoundedInventory(*occupied, *scope.table, GameTitle::MM6));
}

UNIT_TEST(MM6BoundedInventoryProjection, RejectsMalformedReferencesAndUnmappedItems) {
    TableScope scope;
    auto raw = threeItems();
    raw.gridWords[0] = 139;
    EXPECT_FALSE(mm6::stageBoundedInventory(raw, *scope.table, GameTitle::MM6));
    raw = threeItems();
    raw.gridWords[1] = 0xffffffffu;
    EXPECT_FALSE(mm6::stageBoundedInventory(raw, *scope.table, GameTitle::MM6));
    raw = threeItems();
    raw.equipmentWords[0] = 138;
    EXPECT_FALSE(mm6::stageBoundedInventory(raw, *scope.table, GameTitle::MM6));
    raw = threeItems();
    raw.equipmentWords[0] = 0x80000000u;
    EXPECT_FALSE(mm6::stageBoundedInventory(raw, *scope.table, GameTitle::MM6));
    raw = threeItems();
    raw.items[137].setWord(4, 2);
    EXPECT_FALSE(mm6::stageBoundedInventory(raw, *scope.table, GameTitle::MM6));
    raw = threeItems();
    EXPECT_FALSE(mm6::stageBoundedInventory(raw, *scope.table, GameTitle::MM7));

    raw = threeItems();
    scope.table->itemSizes[static_cast<ItemId>(221)] = Sizei(2, 1);
    raw.gridWords[1] = 0xffffffffu; // Canonical continuation of the first grid cell.
    auto wide = mm6::stageBoundedInventory(raw, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(wide);
    EXPECT_EQ(mm6::captureBoundedInventory(*wide, *scope.table, GameTitle::MM6), raw);

    Item wand = wide->inventory().take(wide->inventory().entry(ITEM_SLOT_MAIN_HAND));
    ASSERT_TRUE(wide->inventory().tryRestoreAt(0, ITEM_SLOT_OFF_HAND, wand));
    EXPECT_FALSE(mm6::captureBoundedInventory(*wide, *scope.table, GameTitle::MM6)); // Body location still names the main hand.
    mm6::setRawBodyLocation(wide->inventory().entry(ITEM_SLOT_OFF_HAND).get(), ITEM_SLOT_OFF_HAND);
    auto offHand = mm6::captureBoundedInventory(*wide, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(offHand);
    EXPECT_EQ(offHand->equipmentWords[0], 1);
    EXPECT_EQ(offHand->equipmentWords[1], 0);
    EXPECT_EQ(offHand->items[0][24], 1);

    auto unchanged = mm6::stageBoundedInventory(raw, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(unchanged);
    scope.table->items[static_cast<ItemId>(221)].type = ITEM_TYPE_POTION;
    EXPECT_FALSE(mm6::captureBoundedInventory(*unchanged, *scope.table, GameTitle::MM6));
}
