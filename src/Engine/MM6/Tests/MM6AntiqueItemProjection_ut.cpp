#include <cstdint>
#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6AntiqueItemProjection.h"
#include "Engine/MM6/MM6BoundedInventoryProjection.h"
#include "Engine/MM6/MM6RawPartyInventoryStage.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    TableScope() {
        pItemTable = table.get();
        const ItemId armor = static_cast<ItemId>(100);
        table->items[armor].iconName = "armor";
        table->items[armor].name = "Armor";
        table->items[armor].baseValue = 123;
        table->items[armor].type = ITEM_TYPE_ARMOUR;
        table->itemSizes[armor] = Sizei(1, 1);
        auto &special = table->specialEnchantments[ITEM_ENCHANTMENT_ANTIQUE];
        special.itemSuffixOrPrefix = "Antique";
        special.valueMul = 10;
        special.chanceByItemType[ITEM_TYPE_ARMOUR] = 1;
    }

    ~TableScope() { pItemTable = previous; }
};

mm6::ItemRecord28 antiqueRecord() {
    mm6::ItemRecord28 raw;
    raw.setWord(0, 100);
    raw.setWord(12, 58);
    raw.setWord(16, 0xdeadbeefu);
    raw.setWord(20, 0x80000001u);
    raw[24] = 4;
    raw[25] = 9;
    raw[26] = 0x5a;
    raw[27] = 0xa5;
    return raw;
}

} // namespace

UNIT_TEST(MM6AntiqueItemProjection, ValueAndExactRawCarrier) {
    TableScope scope;
    const auto raw = antiqueRecord();
    auto projection = mm6::stageAntiqueItem(raw, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    EXPECT_EQ(projection->item.specialEnchantment, ITEM_ENCHANTMENT_ANTIQUE);
    EXPECT_EQ(projection->item.mm6RawRecord, raw);
    EXPECT_EQ(projection->item.GetValue(), 1230);
    EXPECT_EQ(projection->item.GetIdentifiedName(), "Antique Armor");
    EXPECT_EQ(mm6::captureAntiqueItem(*projection, *scope.table, GameTitle::MM6), raw);

    projection->item.SetBroken();
    auto changed = mm6::captureAntiqueItem(*projection, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(changed);
    EXPECT_EQ(changed->conditionRaw(), 0x80000003u);
    EXPECT_EQ(changed->secondBonusRaw(), 58);
    EXPECT_EQ((*changed)[24], 4);
    EXPECT_EQ((*changed)[26], 0x5a);
    EXPECT_EQ((*changed)[27], 0xa5);

    projection->item.mm6RawRecord.reset();
    EXPECT_FALSE(mm6::captureAntiqueItem(*projection, *scope.table, GameTitle::MM6));
}

UNIT_TEST(MM6AntiqueItemProjection, RejectsUnprovedSemanticsAndUnsafeValue) {
    TableScope scope;
    auto raw = antiqueRecord();
    EXPECT_FALSE(mm6::stageAntiqueItem(raw, *scope.table, GameTitle::MM7));
    raw.setWord(12, 1);
    EXPECT_FALSE(mm6::stageAntiqueItem(raw, *scope.table, GameTitle::MM6));
    raw = antiqueRecord();
    raw.setWord(4, 1);
    EXPECT_FALSE(mm6::stageAntiqueItem(raw, *scope.table, GameTitle::MM6));
    raw = antiqueRecord();
    scope.table->items[static_cast<ItemId>(100)].rarity = RARITY_ARTIFACT;
    EXPECT_FALSE(mm6::stageAntiqueItem(raw, *scope.table, GameTitle::MM6));
    scope.table->items[static_cast<ItemId>(100)].rarity = RARITY_COMMON;
    scope.table->items[static_cast<ItemId>(100)].baseValue = 214748365;
    EXPECT_FALSE(mm6::stageAntiqueItem(raw, *scope.table, GameTitle::MM6));
    scope.table->items[static_cast<ItemId>(100)].baseValue = 123;
    auto projection = mm6::stageAntiqueItem(raw, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    projection->item.potionPower = 1;
    EXPECT_FALSE(mm6::captureAntiqueItem(*projection, *scope.table, GameTitle::MM6));
    projection->item.potionPower = 0;
    projection->item.specialEnchantment = ITEM_ENCHANTMENT_OF_PROTECTION;
    EXPECT_FALSE(mm6::captureAntiqueItem(*projection, *scope.table, GameTitle::MM6));
    projection->item.specialEnchantment = ITEM_ENCHANTMENT_ANTIQUE;
    scope.table->specialEnchantments[ITEM_ENCHANTMENT_ANTIQUE].valueMul = 9;
    EXPECT_FALSE(mm6::captureAntiqueItem(*projection, *scope.table, GameTitle::MM6));
}

UNIT_TEST(MM6AntiqueItemProjection, FreshPartyRoundTripAndLateFailure) {
    TableScope scope;
    Party baseline;
    baseline.uNumGold = 17;
    mm6::RawInventoryFields raw;
    raw.ready = true;
    for (auto &character : raw.characters) {
        character.items[137] = antiqueRecord();
        character.gridWords[0] = 138;
    }
    EXPECT_TRUE(mm6::stageBoundedItem(raw.characters[0].items[137], *scope.table,
                                      GameTitle::MM6));
    EXPECT_TRUE(mm6::stageBoundedInventory(raw.characters[0], *scope.table,
                                           GameTitle::MM6));
    auto staged = mm6::stageRawPartyInventories(baseline, raw, *scope.table, GameTitle::MM6);
    EXPECT_EQ(mm6::captureRawPartyInventories(staged, *scope.table, GameTitle::MM6), raw);
    EXPECT_EQ(staged.party.pCharacters[2].inventory.entry(137)->GetValue(), 1230);
    EXPECT_EQ(baseline.uNumGold, 17);
    EXPECT_FALSE(baseline.pCharacters[3].inventory.entry(137));

    auto bad = raw;
    bad.characters[3].items[137].setWord(12, 59);
    EXPECT_THROW((void) mm6::stageRawPartyInventories(baseline, bad, *scope.table,
                                                      GameTitle::MM6), std::exception);
    EXPECT_EQ(baseline.uNumGold, 17);
    EXPECT_FALSE(baseline.pCharacters[3].inventory.entry(137));
}
