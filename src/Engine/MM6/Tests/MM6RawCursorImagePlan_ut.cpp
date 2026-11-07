#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawCursorImagePlan.h"
#include "Engine/MM6/MM6RawHeldPartyStage.h"

namespace {

struct TestItems {
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    TestItems() {
        const ItemId id = static_cast<ItemId>(100);
        table->items[id].iconName = "synthetic-cursor-icon";
        table->items[id].type = ITEM_TYPE_ARMOUR;
        table->items[id].rarity = RARITY_COMMON;
        table->itemSizes[id] = Sizei(1, 1);
    }
};

mm6::RawHeldCursorOwner cursorWithFlags(std::uint32_t flags) {
    auto cursor = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    cursor.record.setWord(0, 100);
    cursor.record.setWord(20, flags);
    cursor.record[27] = 0xa5;
    return cursor;
}

} // namespace

UNIT_TEST(MM6RawCursorImagePlan, DerivesHeldIconAndDrawTintWithoutInventingOffset) {
    TestItems items;
    for (const auto [flags, tint] : {
             std::pair{std::uint32_t(0), mm6::HeldCursorTint::Unidentified},
             std::pair{std::uint32_t(ITEM_IDENTIFIED), mm6::HeldCursorTint::Normal},
             std::pair{std::uint32_t(ITEM_BROKEN | ITEM_IDENTIFIED), mm6::HeldCursorTint::Broken}}) {
        const auto cursor = cursorWithFlags(flags);
        const Party party = mm6::stageRawHeldParty(Party{}, cursor, *items.table, GameTitle::MM6);
        const auto plan = mm6::stageRawCursorImagePlan(party, cursor, *items.table, GameTitle::MM6);
        EXPECT_TRUE(plan.occupied);
        EXPECT_EQ(plan.itemId, static_cast<ItemId>(100));
        EXPECT_EQ(plan.iconName, "synthetic-cursor-icon");
        EXPECT_EQ(plan.tint, tint);
        EXPECT_FALSE(plan.pickedItemOffset.has_value());
        EXPECT_EQ(party.pPickedItem.mm6RawRecord, cursor.record);
    }
}

UNIT_TEST(MM6RawCursorImagePlan, EmptyOwnerRequestsArrowAndZeroOffset) {
    TestItems items;
    const auto cursor = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    const Party party;
    const auto plan = mm6::stageRawCursorImagePlan(party, cursor, *items.table, GameTitle::MM6);
    EXPECT_FALSE(plan.occupied);
    EXPECT_EQ(plan.itemId, ITEM_NULL);
    EXPECT_EQ(plan.iconName, "MICON1");
    ASSERT_TRUE(plan.pickedItemOffset);
    EXPECT_EQ(plan.pickedItemOffset->x, 0);
    EXPECT_EQ(plan.pickedItemOffset->y, 0);
}

UNIT_TEST(MM6RawCursorImagePlan, RejectsMismatchAndUnsupportedTemplateWithoutMutation) {
    TestItems items;
    const auto cursor = cursorWithFlags(std::uint32_t(ITEM_IDENTIFIED));
    Party party = mm6::stageRawHeldParty(Party{}, cursor, *items.table, GameTitle::MM6);
    EXPECT_THROW((void) mm6::stageRawCursorImagePlan(party, cursor, *items.table, GameTitle::MM7),
                 std::exception);
    auto unready = cursor;
    unready.ready = false;
    EXPECT_THROW((void) mm6::stageRawCursorImagePlan(party, unready, *items.table, GameTitle::MM6),
                 std::exception);
    party.pPickedItem.flags &= ~ITEM_IDENTIFIED;
    EXPECT_THROW((void) mm6::stageRawCursorImagePlan(party, cursor, *items.table, GameTitle::MM6),
                 std::exception);
    party.pPickedItem.flags |= ITEM_IDENTIFIED;
    party.pPickedItem.mm6RawRecord.reset();
    EXPECT_THROW((void) mm6::stageRawCursorImagePlan(party, cursor, *items.table, GameTitle::MM6),
                 std::exception);
    party.pPickedItem.mm6RawRecord = cursor.record;
    items.table->items[static_cast<ItemId>(100)].iconName.clear();
    EXPECT_THROW((void) mm6::stageRawCursorImagePlan(party, cursor, *items.table, GameTitle::MM6),
                 std::exception);
    EXPECT_EQ(party.pPickedItem.itemId, static_cast<ItemId>(100));
    EXPECT_EQ(cursor.record[27], 0xa5);
}

UNIT_TEST(MM6RawCursorImagePlan, RejectsStalePartyPayloadForEmptyRawOwner) {
    TestItems items;
    const auto cursor = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    Party party;
    party.pPickedItem.flags = ITEM_BROKEN;
    EXPECT_THROW((void) mm6::stageRawCursorImagePlan(party, cursor, *items.table, GameTitle::MM6),
                 std::exception);
    EXPECT_EQ(party.pPickedItem.flags, ITEM_BROKEN);
}
