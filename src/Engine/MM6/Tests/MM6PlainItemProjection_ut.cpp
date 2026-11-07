#include <cstdint>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PlainItemProjection.h"
#include "Engine/Objects/Inventory.h"
#include "Engine/Snapshots/EntitySnapshots.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    ItemTable table;

    TableScope() { pItemTable = &table; }
    ~TableScope() { pItemTable = previous; }
};

} // namespace

UNIT_TEST(MM6PlainItemProjection, ScrollInInheritedPotionRangeKeepsRawIdentity) {
    ItemTable table;
    const ItemId scroll = static_cast<ItemId>(221); // An MM6 scroll, but an MM7 potion ID.
    table.items[scroll].iconName = "scroll";
    table.items[scroll].type = ITEM_TYPE_SPELL_SCROLL;

    mm6::ItemRecord28 raw;
    raw.setWord(0, 221);
    raw.setWord(16, 7);
    raw.setWord(20, 0x80000001u);
    raw[24] = 5;
    raw[25] = 9;
    raw[26] = 0x5a;
    raw[27] = 0xa5;

    auto projection = mm6::stagePlainItem(raw, table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    EXPECT_EQ(projection->sourceType, ITEM_TYPE_SPELL_SCROLL);
    EXPECT_EQ(projection->item.potionPower, 0);
    EXPECT_EQ(projection->item.numCharges, 7);
    EXPECT_EQ(projection->item.maxCharges, 9);
    EXPECT_EQ(projection->item.mm6RawRecord, raw);
    EXPECT_EQ(mm6::capturePlainItem(*projection, table, GameTitle::MM6), raw);

    projection->item.flags = ItemFlags(0x80000003u);
    auto changed = mm6::capturePlainItem(*projection, table, GameTitle::MM6);
    ASSERT_TRUE(changed);
    EXPECT_EQ(changed->word(16), 7);
    EXPECT_EQ(changed->word(20), 0x80000003u);
    EXPECT_EQ((*changed)[24], 5);
    EXPECT_EQ((*changed)[26], 0x5a);
    EXPECT_EQ((*changed)[27], 0xa5);
    projection->item.numCharges = 8;
    EXPECT_FALSE(mm6::capturePlainItem(*projection, table, GameTitle::MM6));
    projection->item.numCharges = 7;
    projection->item.flags = ItemFlags(0x80000403u);
    EXPECT_FALSE(mm6::capturePlainItem(*projection, table, GameTitle::MM6));
    projection->item.flags = ItemFlags(0x80000003u);
    projection->item.mm6RawRecord.reset();
    EXPECT_FALSE(mm6::capturePlainItem(*projection, table, GameTitle::MM6));
}

UNIT_TEST(MM6PlainItemProjection, GoldValueRoundTripAndUnsupportedUnionsReject) {
    ItemTable table;
    const ItemId gold = static_cast<ItemId>(197);
    const ItemId bottle = static_cast<ItemId>(164);
    const ItemId wand = static_cast<ItemId>(135);
    table.items[gold].iconName = "gold";
    table.items[gold].type = ITEM_TYPE_GOLD;
    table.items[bottle].iconName = "bottle";
    table.items[bottle].type = ITEM_TYPE_POTION;
    table.items[wand].iconName = "wand";
    table.items[wand].type = ITEM_TYPE_WAND;

    mm6::ItemRecord28 raw;
    raw.setWord(0, 197);
    raw.setWord(12, 79);
    auto projection = mm6::stagePlainItem(raw, table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    EXPECT_EQ(projection->item.goldAmount, 79);
    projection->item.goldAmount = 81;
    auto changed = mm6::capturePlainItem(*projection, table, GameTitle::MM6);
    ASSERT_TRUE(changed);
    EXPECT_EQ(changed->word(12), 81);

    raw.setWord(0, 164);
    EXPECT_FALSE(mm6::stagePlainItem(raw, table, GameTitle::MM6)); // A bottle cannot inherit gold's value union.
    raw.setWord(12, 0);
    raw.setWord(4, 12);
    EXPECT_FALSE(mm6::stagePlainItem(raw, table, GameTitle::MM6)); // Potion power is not mapped yet.
    raw.setWord(4, 0);
    ASSERT_TRUE(mm6::stagePlainItem(raw, table, GameTitle::MM6));
    EXPECT_FALSE(mm6::stagePlainItem(raw, table, GameTitle::MM7));
    projection->item.standardEnchantment = ATTRIBUTE_MIGHT;
    EXPECT_FALSE(mm6::capturePlainItem(*projection, table, GameTitle::MM6));
    projection->item.standardEnchantment.reset();
    projection->raw.setWord(4, 1);
    EXPECT_FALSE(mm6::capturePlainItem(*projection, table, GameTitle::MM6));

    raw = {};
    raw.setWord(0, 135);
    raw.setWord(16, 3);
    raw[25] = 5;
    auto wandProjection = mm6::stagePlainItem(raw, table, GameTitle::MM6);
    ASSERT_TRUE(wandProjection);
    wandProjection->item.numCharges = 4;
    wandProjection->item.maxCharges = 6;
    auto updatedWand = mm6::capturePlainItem(*wandProjection, table, GameTitle::MM6);
    ASSERT_TRUE(updatedWand);
    EXPECT_EQ(updatedWand->word(16), 4);
    EXPECT_EQ((*updatedWand)[25], 6);
}

UNIT_TEST(MM6PlainItemProjection, RawRecordFollowsValueCopyAcrossInventories) {
    TableScope scope;
    const ItemId scroll = static_cast<ItemId>(221);
    scope.table.items[scroll].iconName = "scroll";
    scope.table.items[scroll].type = ITEM_TYPE_SPELL_SCROLL;
    scope.table.itemSizes[scroll] = Sizei(1, 1);

    mm6::ItemRecord28 raw;
    raw.setWord(0, 221);
    raw[26] = 0x5a;
    auto staged = mm6::stagePlainItem(raw, scope.table, GameTitle::MM6);
    ASSERT_TRUE(staged);

    CharacterInventory source = CharacterInventory::forMM6();
    CharacterInventory destination = CharacterInventory::forMM6();
    ASSERT_TRUE(source.tryRestoreAt(137, Pointi(0, 0), staged->item));
    Item carried = source.take(source.entry(137));
    EXPECT_EQ(carried.mm6RawRecord, raw);
    auto added = destination.tryAdd(Pointi(1, 0), carried);
    ASSERT_TRUE(added);
    EXPECT_EQ(added->mm6RawRecord, raw);

    carried.Reset();
    EXPECT_FALSE(carried.mm6RawRecord);
    carried = *added;
    Item_MM7 mm7 = {};
    mm7.itemId = 1;
    reconstruct(mm7, &carried);
    EXPECT_FALSE(carried.mm6RawRecord);
}
