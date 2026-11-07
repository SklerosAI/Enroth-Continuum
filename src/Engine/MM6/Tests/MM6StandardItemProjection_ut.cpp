#include <cstdint>
#include <limits>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6StandardItemProjection.h"
#include "Engine/Objects/Inventory.h"

UNIT_TEST(MM6StandardItemProjection, ArmorBonusAndOpaqueTailRoundTrip) {
    ItemTable table;
    const ItemId armor = static_cast<ItemId>(10);
    table.items[armor].iconName = "armor";
    table.items[armor].type = ITEM_TYPE_ARMOUR;

    mm6::ItemRecord28 raw;
    raw.setWord(0, 10);
    raw.setWord(4, 12); // Original electricity, runtime air resistance.
    raw.setWord(8, 7);
    raw.setWord(20, 0x80000001u);
    raw[24] = 4;
    raw[26] = 0x5a;
    raw[27] = 0xa5;

    auto projection = mm6::stageStandardItem(raw, table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    ASSERT_TRUE(projection->item.standardEnchantment);
    EXPECT_EQ(*projection->item.standardEnchantment, ATTRIBUTE_RESIST_AIR);
    EXPECT_EQ(projection->item.standardEnchantmentStrength, 7);
    EXPECT_EQ(projection->item.mm6RawRecord, raw);
    EXPECT_EQ(mm6::captureStandardItem(*projection, table, GameTitle::MM6), raw);

    projection->item.standardEnchantment = ATTRIBUTE_RESIST_BODY;
    projection->item.standardEnchantmentStrength = 19;
    projection->item.SetBroken();
    auto changed = mm6::captureStandardItem(*projection, table, GameTitle::MM6);
    ASSERT_TRUE(changed);
    EXPECT_EQ(changed->bonusRaw(), 14);
    EXPECT_EQ(changed->bonusStrengthRaw(), 19);
    EXPECT_EQ(changed->conditionRaw(), 0x80000003u);
    EXPECT_EQ((*changed)[24], 4);
    EXPECT_EQ((*changed)[26], 0x5a);
    EXPECT_EQ((*changed)[27], 0xa5);

    CharacterInventory source = CharacterInventory::forMM6();
    CharacterInventory destination = CharacterInventory::forMM6();
    ASSERT_TRUE(source.tryRestoreAt(137, ITEM_SLOT_ARMOUR, projection->item));
    Item carried = source.take(source.entry(137));
    EXPECT_EQ(carried.mm6RawRecord, raw);
    auto equipped = destination.tryEquip(ITEM_SLOT_ARMOUR, carried);
    ASSERT_TRUE(equipped);
    EXPECT_EQ(equipped->mm6RawRecord, raw);

    projection->item.mm6RawRecord.reset();
    EXPECT_FALSE(mm6::captureStandardItem(*projection, table, GameTitle::MM6));
}

UNIT_TEST(MM6StandardItemProjection, RejectsUnsupportedCategoryAndFields) {
    ItemTable table;
    const ItemId armor = static_cast<ItemId>(10);
    table.items[armor].iconName = "armor";
    table.items[armor].type = ITEM_TYPE_ARMOUR;
    mm6::ItemRecord28 raw;
    raw.setWord(0, 10);
    raw.setWord(4, 1);
    raw.setWord(8, 5);
    EXPECT_FALSE(mm6::stageStandardItem(raw, table, GameTitle::MM7));
    auto projection = mm6::stageStandardItem(raw, table, GameTitle::MM6);
    ASSERT_TRUE(projection);

    projection->item.standardEnchantment = ATTRIBUTE_RESIST_EARTH;
    EXPECT_FALSE(mm6::captureStandardItem(*projection, table, GameTitle::MM6));
    projection->item.standardEnchantment = ATTRIBUTE_MIGHT;
    projection->item.specialEnchantment = static_cast<ItemEnchantment>(1);
    EXPECT_FALSE(mm6::captureStandardItem(*projection, table, GameTitle::MM6));
    projection->item.specialEnchantment = ITEM_ENCHANTMENT_NULL;
    table.items[armor].type = ITEM_TYPE_SPELL_SCROLL;
    EXPECT_FALSE(mm6::captureStandardItem(*projection, table, GameTitle::MM6));
    EXPECT_FALSE(mm6::stageStandardItem(raw, table, GameTitle::MM6));

    table.items[armor].type = ITEM_TYPE_ARMOUR;
    raw.setWord(4, 15);
    EXPECT_FALSE(mm6::stageStandardItem(raw, table, GameTitle::MM6));
    raw.setWord(4, 1);
    raw.setWord(8, 0);
    EXPECT_FALSE(mm6::stageStandardItem(raw, table, GameTitle::MM6));
    raw.setWord(8, 5);
    raw.setWord(12, 3);
    EXPECT_FALSE(mm6::stageStandardItem(raw, table, GameTitle::MM6));
}

UNIT_TEST(MM6StandardItemProjection, RejectsUnsafeStrengthAndNoncommonTemplate) {
    ItemTable table;
    const ItemId armor = static_cast<ItemId>(10);
    table.items[armor].iconName = "armor";
    table.items[armor].type = ITEM_TYPE_ARMOUR;
    table.items[armor].baseValue = 100;
    mm6::ItemRecord28 raw;
    raw.setWord(0, 10);
    raw.setWord(4, 1);
    const int maximum = (std::numeric_limits<int>::max() - 100) / 100;
    raw.setWord(8, maximum);

    auto projection = mm6::stageStandardItem(raw, table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    EXPECT_EQ(mm6::captureStandardItem(*projection, table, GameTitle::MM6), raw);
    ItemTable *previous = pItemTable;
    pItemTable = &table;
    EXPECT_EQ(projection->item.GetValue(), 100 + 100 * maximum);
    pItemTable = previous;
    projection->item.standardEnchantmentStrength = maximum + 1;
    EXPECT_FALSE(mm6::captureStandardItem(*projection, table, GameTitle::MM6));
    raw.setWord(8, maximum + 1);
    EXPECT_FALSE(mm6::stageStandardItem(raw, table, GameTitle::MM6));

    table.items[armor].baseValue = -100;
    raw.setWord(8, std::numeric_limits<int>::max() / 100 + 1);
    EXPECT_FALSE(mm6::stageStandardItem(raw, table, GameTitle::MM6));
    table.items[armor].baseValue = 100;

    raw.setWord(8, 7);
    table.items[armor].rarity = RARITY_ARTIFACT;
    EXPECT_FALSE(mm6::stageStandardItem(raw, table, GameTitle::MM6));
    table.items[armor].rarity = RARITY_RELIC;
    EXPECT_FALSE(mm6::stageStandardItem(raw, table, GameTitle::MM6));
    table.items[armor].rarity = RARITY_COMMON;
    projection = mm6::stageStandardItem(raw, table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    table.items[armor].rarity = RARITY_ARTIFACT;
    EXPECT_FALSE(mm6::captureStandardItem(*projection, table, GameTitle::MM6));
}
