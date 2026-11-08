#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "GUI/UI/Houses/MM6ShopPrices.h"

#include "Engine/Data/ItemData.h"
#include "Engine/Objects/Item.h"
#include "Engine/Tables/ItemTable.h"

static mm6::ItemRecord28 record(int item, int bonus, int strength, int special) {
    mm6::ItemRecord28 result;
    result.setWord(0, item);
    result.setWord(4, bonus);
    result.setWord(8, strength);
    result.setWord(12, special);
    return result;
}

UNIT_TEST(MM6ShopPrices, MerchantPercent) {
    // MM6 merchant is 3, 4 or 5 per point by mastery, plus 7, with hireling points counted before the multiplier.
    EXPECT_EQ(mm6::merchantPercent(CombinedSkillValue(), 0), 0);
    EXPECT_EQ(mm6::merchantPercent(CombinedSkillValue(), 4), 19);
    EXPECT_EQ(mm6::merchantPercent(CombinedSkillValue(2, MASTERY_NOVICE), 0), 13);
    EXPECT_EQ(mm6::merchantPercent(CombinedSkillValue(2, MASTERY_EXPERT), 6), 39);
    EXPECT_EQ(mm6::merchantPercent(CombinedSkillValue(10, MASTERY_MASTER), 0), 57);
    EXPECT_EQ(mm6::merchantPercent(CombinedSkillValue(30, MASTERY_MASTER), 0), 157); // No cap at 100.
}

UNIT_TEST(MM6ShopPrices, BuyPrice) {
    // Buying truncates value times the shop multiplier, takes the merchant percentage off, and never goes below the value.
    EXPECT_EQ(mm6::itemBuyPrice(100, 1.5f, 0), 150);
    EXPECT_EQ(mm6::itemBuyPrice(33, 1.5f, 0), 49);
    EXPECT_EQ(mm6::itemBuyPrice(100, 1.5f, 13), 130);
    EXPECT_EQ(mm6::itemBuyPrice(100, 1.5f, 57), 100);
    EXPECT_EQ(mm6::itemBuyPrice(100, 1.5f, 157), 100);
    EXPECT_EQ(mm6::itemBuyPrice(0, 1.5f, 0), 1);
}

UNIT_TEST(MM6ShopPrices, SellPrice) {
    // Selling pays value over multiplier plus 2, plus the merchant percentage of the value, capped at the value.
    EXPECT_EQ(mm6::itemSellPrice(100, 1.5f, 0), 28);
    EXPECT_EQ(mm6::itemSellPrice(100, 1.5f, 13), 41);
    EXPECT_EQ(mm6::itemSellPrice(100, 1.5f, 157), 100);
    EXPECT_EQ(mm6::itemSellPrice(1, 1.5f, 0), 1);
    EXPECT_EQ(mm6::itemSellPrice(0, 1.5f, 0), 1);
}

UNIT_TEST(MM6ShopPrices, SalePayment) {
    // A broken item fetches 1 gold, and a general store pays half, but never less than 1.
    EXPECT_EQ(mm6::itemSalePayment(100, 1.5f, 0, false, false), 28);
    EXPECT_EQ(mm6::itemSalePayment(100, 1.5f, 0, true, false), 1);
    EXPECT_EQ(mm6::itemSalePayment(100, 1.5f, 0, false, true), 14);
    EXPECT_EQ(mm6::itemSalePayment(100, 1.5f, 0, true, true), 1);
}

UNIT_TEST(MM6ShopPrices, ServicePrices) {
    // Identify costs 50 times the multiplier and repair the value over 6 minus the multiplier, both discounted to no
    // less than a third.
    EXPECT_EQ(mm6::itemIdentifyPrice(1.5f, 0), 75);
    EXPECT_EQ(mm6::itemIdentifyPrice(1.5f, 13), 65);
    EXPECT_EQ(mm6::itemIdentifyPrice(1.5f, 157), 25);
    EXPECT_EQ(mm6::itemIdentifyPrice(0.01f, 0), 1);
    EXPECT_EQ(mm6::itemRepairPrice(900, 1.5f, 0), 200);
    EXPECT_EQ(mm6::itemRepairPrice(900, 1.5f, 13), 174);
    EXPECT_EQ(mm6::itemRepairPrice(900, 1.5f, 157), 66);
    EXPECT_EQ(mm6::itemRepairPrice(1, 1.5f, 0), 1);
}

UNIT_TEST(MM6ShopPrices, ItemValue) {
    // A standard bonus adds 100 gold a point, and a special bonus adds its value when above 10 and multiplies otherwise.
    ItemTable table;
    table.items[static_cast<ItemId>(5)].baseValue = 40;
    table.specialEnchantments[static_cast<ItemEnchantment>(3)].valueAdd = 500;
    table.specialEnchantments[static_cast<ItemEnchantment>(4)].valueMul = 2;
    table.specialEnchantments[static_cast<ItemEnchantment>(5)].valueMul = 10;
    EXPECT_EQ(mm6::itemValue(record(5, 0, 0, 0), table), 40);
    EXPECT_EQ(mm6::itemValue(record(5, 1, 3, 0), table), 340);
    EXPECT_EQ(mm6::itemValue(record(5, 0, 0, 3), table), 540);
    EXPECT_EQ(mm6::itemValue(record(5, 0, 0, 4), table), 80);
    EXPECT_EQ(mm6::itemValue(record(5, 0, 0, 5), table), 400);
    EXPECT_THROW(mm6::itemValue(record(0, 0, 0, 0), table), std::runtime_error);
    EXPECT_THROW(mm6::itemValue(record(581, 0, 0, 0), table), std::runtime_error);
    EXPECT_THROW(mm6::itemValue(record(5, 0, 0, 60), table), std::runtime_error);
}

UNIT_TEST(MM6ShopPrices, LiveItemValue) {
    // A live item is valued from its original record when it has one, otherwise from its bonus fields.
    ItemTable table;
    table.items[static_cast<ItemId>(5)].baseValue = 40;
    table.items[static_cast<ItemId>(6)].type = ITEM_TYPE_POTION;
    table.specialEnchantments[static_cast<ItemEnchantment>(3)].valueAdd = 500;
    Item item;
    item.itemId = static_cast<ItemId>(5);
    EXPECT_EQ(mm6::itemValue(item, table), 40);
    item.standardEnchantment = ATTRIBUTE_MIGHT;
    item.standardEnchantmentStrength = 3;
    EXPECT_EQ(mm6::itemValue(item, table), 340);
    item.standardEnchantment.reset();
    item.specialEnchantment = static_cast<ItemEnchantment>(3);
    EXPECT_EQ(mm6::itemValue(item, table), 540);
    item.mm6RawRecord = record(5, 0, 0, 0);
    EXPECT_EQ(mm6::itemValue(item, table), 40);

    Item potion;
    potion.itemId = static_cast<ItemId>(6);
    EXPECT_THROW(mm6::itemValue(potion, table), std::runtime_error);
}

UNIT_TEST(MM6ShopPrices, ShopTradesItem) {
    // Shops deal in items by house id band, an untyped item goes only to general stores, and a few item ids are never taken.
    ItemData sword;
    sword.type = ITEM_TYPE_SINGLE_HANDED;
    sword.skill = SKILL_SWORD;
    ItemData helm;
    helm.type = ITEM_TYPE_HELMET;
    helm.skill = SKILL_MISC;
    ItemData ring;
    ring.type = ITEM_TYPE_RING;
    ring.skill = SKILL_MISC;
    ItemData untyped;
    untyped.type = ITEM_TYPE_MISC;
    untyped.skill = SKILL_MISC;

    const ItemId id = static_cast<ItemId>(10);
    EXPECT_TRUE(mm6::shopTradesItem(1, id, sword));
    EXPECT_FALSE(mm6::shopTradesItem(1, id, helm));
    EXPECT_FALSE(mm6::shopTradesItem(14, id, untyped));
    EXPECT_FALSE(mm6::shopTradesItem(28, id, untyped));
    EXPECT_TRUE(mm6::shopTradesItem(42, id, untyped));
    EXPECT_FALSE(mm6::shopTradesItem(15, id, sword));
    EXPECT_TRUE(mm6::shopTradesItem(28, id, helm));
    EXPECT_FALSE(mm6::shopTradesItem(29, id, sword));
    EXPECT_TRUE(mm6::shopTradesItem(29, id, helm));
    EXPECT_TRUE(mm6::shopTradesItem(41, id, ring));
    EXPECT_TRUE(mm6::shopTradesItem(42, id, sword));
    EXPECT_TRUE(mm6::shopTradesItem(47, id, ring));
    EXPECT_FALSE(mm6::shopTradesItem(48, id, ring));

    EXPECT_FALSE(mm6::shopTradesItem(42, static_cast<ItemId>(430), ring));
    EXPECT_FALSE(mm6::shopTradesItem(42, static_cast<ItemId>(435), ring));
    EXPECT_TRUE(mm6::shopTradesItem(42, static_cast<ItemId>(436), ring));
    EXPECT_TRUE(mm6::shopTradesItem(42, static_cast<ItemId>(445), ring));
    EXPECT_FALSE(mm6::shopTradesItem(42, static_cast<ItemId>(446), ring));
}
