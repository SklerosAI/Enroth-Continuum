#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6GroundItemProjection.h"

namespace {

std::array<std::uint8_t, 100> record(std::uint32_t itemId, bool active = true) {
    std::array<std::uint8_t, 100> result{};
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<std::uint8_t>(i + 17);
    result[2] = active ? 3 : 0;
    result[3] = 0;
    mm6::ItemRecord28 item;
    item.setWord(0, itemId);
    item.setWord(20, 1);
    item[25] = 9;
    item[26] = 0xa6;
    item[27] = 0xb7;
    std::copy(item.begin(), item.end(), result.begin() + 36);
    return result;
}

void define(ItemTable *table, ItemId id, ItemType type) {
    table->items[id].iconName = "item";
    table->items[id].type = type;
    table->itemSizes[id] = Sizei(1, 1);
}

} // namespace

UNIT_TEST(MM6GroundItemProjection, PlainAndStandardExactRoundTrip) {
    ItemTable table;
    define(&table, static_cast<ItemId>(221), ITEM_TYPE_SPELL_SCROLL);
    define(&table, static_cast<ItemId>(10), ITEM_TYPE_ARMOUR);

    const auto plain = record(221);
    auto stagedPlain = mm6::stageGroundItemRecord(plain, table, GameTitle::MM6);
    ASSERT_TRUE(stagedPlain);
    ASSERT_TRUE(stagedPlain->item);
    EXPECT_EQ(mm6::boundedItem(*stagedPlain->item).mm6RawRecord, mm6::groundItemRaw(plain));
    EXPECT_EQ(mm6::captureGroundItemRecord(*stagedPlain, table, GameTitle::MM6), plain);

    auto standard = record(10);
    mm6::ItemRecord28 enchanted = mm6::groundItemRaw(standard);
    enchanted.setWord(4, 12);
    enchanted.setWord(8, 7);
    std::copy(enchanted.begin(), enchanted.end(), standard.begin() + 36);
    auto stagedStandard = mm6::stageGroundItemRecord(standard, table, GameTitle::MM6);
    ASSERT_TRUE(stagedStandard);
    ASSERT_TRUE(stagedStandard->item);
    EXPECT_EQ(mm6::captureGroundItemRecord(*stagedStandard, table, GameTitle::MM6), standard);
}

UNIT_TEST(MM6GroundItemProjection, ChangesOnlySupportedItemBytes) {
    ItemTable table;
    define(&table, static_cast<ItemId>(221), ITEM_TYPE_SPELL_SCROLL);
    const auto original = record(221);
    auto staged = mm6::stageGroundItemRecord(original, table, GameTitle::MM6);
    ASSERT_TRUE(staged);
    ASSERT_TRUE(staged->item);
    mm6::ItemRecord28 updated = mm6::groundItemRaw(original);
    Item changed = mm6::boundedItem(*staged->item);
    changed.flags = ItemFlags(3);
    mm6::setBoundedItem(*staged->item, changed);
    updated.setWord(20, 3);
    auto expected = original;
    std::copy(updated.begin(), updated.end(), expected.begin() + 36);
    EXPECT_EQ(mm6::captureGroundItemRecord(*staged, table, GameTitle::MM6), expected);

    Item replaced = mm6::boundedItem(*staged->item);
    replaced.mm6RawRecord = mm6::groundItemRaw(record(10));
    mm6::setBoundedItem(*staged->item, replaced);
    EXPECT_FALSE(mm6::captureGroundItemRecord(*staged, table, GameTitle::MM6));
}

UNIT_TEST(MM6GroundItemProjection, EmptyAndRemovedRecordsKeepOpaqueBytes) {
    ItemTable table;
    define(&table, static_cast<ItemId>(221), ITEM_TYPE_SPELL_SCROLL);
    auto empty = record(0);
    auto stagedEmpty = mm6::stageGroundItemRecord(empty, table, GameTitle::MM6);
    ASSERT_TRUE(stagedEmpty);
    EXPECT_FALSE(stagedEmpty->item);
    EXPECT_EQ(mm6::captureGroundItemRecord(*stagedEmpty, table, GameTitle::MM6), empty);

    auto removed = record(221, false);
    auto stagedRemoved = mm6::stageGroundItemRecord(removed, table, GameTitle::MM6);
    ASSERT_TRUE(stagedRemoved);
    EXPECT_FALSE(stagedRemoved->item);
    EXPECT_EQ(mm6::captureGroundItemRecord(*stagedRemoved, table, GameTitle::MM6), removed);
    stagedRemoved->item = mm6::stageBoundedItem(mm6::groundItemRaw(removed), table, GameTitle::MM6);
    EXPECT_FALSE(mm6::captureGroundItemRecord(*stagedRemoved, table, GameTitle::MM6));
}

UNIT_TEST(MM6GroundItemProjection, UnsupportedSourceRejects) {
    ItemTable table;
    define(&table, static_cast<ItemId>(221), ITEM_TYPE_SPELL_SCROLL);
    auto plain = record(221);
    EXPECT_FALSE(mm6::stageGroundItemRecord(std::span(plain).first(99), table, GameTitle::MM6));
    EXPECT_FALSE(mm6::stageGroundItemRecord(plain, table, GameTitle::MM7));
    auto badId = record(581);
    EXPECT_FALSE(mm6::stageGroundItemRecord(badId, table, GameTitle::MM6));
    auto equipped = plain;
    equipped[36 + 24] = 1;
    EXPECT_FALSE(mm6::stageGroundItemRecord(equipped, table, GameTitle::MM6));
    auto bonus = plain;
    bonus[36 + 12] = 1;
    EXPECT_FALSE(mm6::stageGroundItemRecord(bonus, table, GameTitle::MM6));
    table.items[static_cast<ItemId>(221)].type = ITEM_TYPE_POTION;
    EXPECT_FALSE(mm6::stageGroundItemRecord(plain, table, GameTitle::MM6));
    table.items[static_cast<ItemId>(221)].type = ITEM_TYPE_SPELL_SCROLL;
    table.items[static_cast<ItemId>(221)].rarity = RARITY_ARTIFACT;
    EXPECT_FALSE(mm6::stageGroundItemRecord(plain, table, GameTitle::MM6));
}
