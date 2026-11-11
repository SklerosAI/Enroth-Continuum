#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestFirstOpenTransaction.h"
#include "Engine/Tables/ChestTable.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();
    TableScope() {
        pItemTable = table.get();
        initializeChests();
        table->itemSizes[static_cast<ItemId>(200)] = Sizei(1, 1);
        table->itemSizes[ITEM_GOLD_SMALL] = Sizei(1, 1);
        table->items[static_cast<ItemId>(200)].type = ITEM_TYPE_RING;
        table->items[ITEM_GOLD_SMALL].type = ITEM_TYPE_GOLD;
    }
    ~TableScope() { pItemTable = previous; }
};

std::array<std::uint8_t, 292> chestTypes() {
    std::array<std::uint8_t, 292> types{};
    types[0] = 8;
    for (int index = 0; index < 8; ++index)
        types[4 + index * 36 + 34] = index + 1;
    return types;
}

mm6::ChestFirstOpenInput inputFor(int itemId, int gold = 0) {
    mm6::ChestFirstOpenInput input;
    input.title = GameTitle::MM6;
    input.chestIndex = 0;
    input.lcgState = 8;
    mm6::writeChest32(std::span(input.record).subspan(4, 28), 0, itemId);
    mm6::writeChest32(std::span(input.record).subspan(4, 28), 12, gold);
    input.record[4 + 26] = 0x5a;
    input.iconLookup = [](int) { return mm6::ChestIconSize{32, 32}; };
    return input;
}

Pointi itemPoint(const mm6::ChestRecord &record) {
    const auto grid = mm6::decodeChestGridWire(GameTitle::MM6, record);
    EXPECT_TRUE(grid);
    for (int cell = 0; cell < 81; ++cell)
        if ((*grid)[cell] > 0)
            return Pointi(cell % 9, cell / 9);
    return Pointi(-1, -1);
}

}  // namespace

UNIT_TEST(MM6ChestFirstOpenTransaction, FirstOpenAndItemPickupAreDetached) {
    TableScope table;
    const auto types = chestTypes();
    auto input = inputFor(200);
    input.dchest = types;
    const auto original = input.record;
    auto opened = mm6::stageChestFirstOpenInteraction(input);
    ASSERT_TRUE(opened);
    EXPECT_TRUE(opened->placed);
    EXPECT_FALSE(opened->itemTaken);
    EXPECT_EQ(mm6::readChest16(opened->record, 2) & 2u, 2u);
    EXPECT_NE(opened->nextLcgState, input.lcgState);

    input.action = mm6::ChestInteractionAction::TakeGridItem;
    input.gridPoint = itemPoint(opened->record);
    auto taken = mm6::stageChestFirstOpenInteraction(input);
    ASSERT_TRUE(taken);
    EXPECT_TRUE(taken->placed);
    EXPECT_TRUE(taken->itemTaken);
    EXPECT_EQ(taken->heldItem.itemId, static_cast<ItemId>(200));
    ASSERT_TRUE(taken->heldItem.mm6RawRecord);
    EXPECT_EQ((*taken->heldItem.mm6RawRecord)[26], 0x5a);
    EXPECT_EQ(mm6::readChest32(std::span(taken->record).subspan(4, 28), 0), 0);
    EXPECT_EQ(taken->record[4 + 26], 0x5a);
    EXPECT_EQ(input.record, original);

    auto again = input;
    again.record = opened->record;
    again.lcgState = opened->nextLcgState;
    again.action = mm6::ChestInteractionAction::OpenOnly;
    auto reopened = mm6::stageChestFirstOpenInteraction(again);
    ASSERT_TRUE(reopened);
    EXPECT_FALSE(reopened->placed);
    EXPECT_EQ(reopened->record, opened->record);
    EXPECT_EQ(reopened->nextLcgState, opened->nextLcgState);
}

UNIT_TEST(MM6ChestFirstOpenTransaction, GoldRequiresExplicitNetCreditAndBounds) {
    TableScope table;
    const auto types = chestTypes();
    auto input = inputFor(std::to_underlying(ITEM_GOLD_SMALL), 79);
    input.dchest = types;
    input.partyGold = 100;
    auto opened = mm6::stageChestFirstOpenInteraction(input);
    ASSERT_TRUE(opened);
    input.action = mm6::ChestInteractionAction::TakeGridItem;
    input.gridPoint = itemPoint(opened->record);
    EXPECT_FALSE(mm6::stageChestFirstOpenInteraction(input));
    input.netGoldCredit = -1;
    EXPECT_FALSE(mm6::stageChestFirstOpenInteraction(input));
    input.netGoldCredit = std::numeric_limits<int>::max();
    EXPECT_FALSE(mm6::stageChestFirstOpenInteraction(input));
    input.netGoldCredit = 79;
    auto taken = mm6::stageChestFirstOpenInteraction(input);
    ASSERT_TRUE(taken);
    EXPECT_TRUE(taken->itemTaken);
    EXPECT_EQ(taken->grossGold, 79);
    EXPECT_EQ(taken->partyGold, 179);
    EXPECT_EQ(taken->heldItem.itemId, ITEM_NULL);
    EXPECT_EQ(input.partyGold, 100);
}

UNIT_TEST(MM6ChestFirstOpenTransaction, RejectsForeignTitleAndBadActionWithoutMutation) {
    TableScope table;
    const auto types = chestTypes();
    auto input = inputFor(200);
    input.dchest = types;
    const auto original = input.record;
    input.title = GameTitle::MM7;
    EXPECT_FALSE(mm6::stageChestFirstOpenInteraction(input));
    input.title = GameTitle::MM6;
    input.action = mm6::ChestInteractionAction::TakeGridItem;
    input.gridPoint = Pointi(9, 0);
    EXPECT_FALSE(mm6::stageChestFirstOpenInteraction(input));
    input.heldItem.itemId = static_cast<ItemId>(200);
    EXPECT_FALSE(mm6::stageChestFirstOpenInteraction(input));
    EXPECT_EQ(input.record, original);
    EXPECT_EQ(input.lcgState, 8u);
}

UNIT_TEST(MM6ChestFirstOpenTransaction, NonGoldPickupCarriesExactRawCursorAndChestTail) {
    TableScope table;
    const auto types = chestTypes();
    auto input = inputFor(200);
    input.dchest = types;
    auto opened = mm6::stageChestFirstOpenInteraction(input);
    ASSERT_TRUE(opened);
    input.record = opened->record;
    input.lcgState = opened->nextLcgState;
    input.gridPoint = itemPoint(input.record);
    const auto beforeChest = input.record;
    auto empty = mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    ASSERT_TRUE(empty);
    auto picked = mm6::stageChestCursorPickup(input, *empty);
    ASSERT_TRUE(picked);
    EXPECT_TRUE(picked->chest.itemTaken);
    EXPECT_FALSE(picked->chest.placed);
    EXPECT_EQ(picked->chest.nextLcgState, opened->nextLcgState);
    ASSERT_TRUE(picked->chest.heldItem.mm6RawRecord);
    EXPECT_EQ(picked->cursor.record, *picked->chest.heldItem.mm6RawRecord);
    EXPECT_EQ(picked->cursor.record[26], 0x5a);
    EXPECT_EQ(mm6::readChest32(std::span(picked->chest.record).subspan(4, 28), 0), 0);
    EXPECT_EQ(picked->chest.record[4 + 26], 0x5a);
    EXPECT_EQ(input.record, beforeChest);
    EXPECT_EQ(empty->record.itemNumberRaw(), 0u);
}

UNIT_TEST(MM6ChestFirstOpenTransaction, CursorJoinRejectsGoldOccupiedAndForeignOwners) {
    TableScope table;
    const auto types = chestTypes();
    auto input = inputFor(200);
    input.dchest = types;
    auto opened = mm6::stageChestFirstOpenInteraction(input);
    ASSERT_TRUE(opened);
    input.record = opened->record;
    input.gridPoint = itemPoint(input.record);
    auto empty = mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    ASSERT_TRUE(empty);
    const auto beforeChest = input.record;
    const auto beforeCursor = *empty;

    mm6::ItemRecord28 held;
    held.setWord(0, 200);
    auto occupied = mm6::stageRawHeldCursorSet(*empty, held, GameTitle::MM6);
    ASSERT_TRUE(occupied);
    EXPECT_FALSE(mm6::stageChestCursorPickup(input, *occupied));

    auto foreign = *empty;
    foreign.title = GameTitle::MM7;
    EXPECT_FALSE(mm6::stageChestCursorPickup(input, foreign));
    input.gridPoint = Pointi(9, 0);
    EXPECT_FALSE(mm6::stageChestCursorPickup(input, *empty));
    input.gridPoint = itemPoint(input.record);
    input.record = inputFor(std::to_underlying(ITEM_GOLD_SMALL), 79).record;
    auto goldOpen = mm6::stageChestFirstOpenInteraction(input);
    ASSERT_TRUE(goldOpen);
    input.record = goldOpen->record;
    input.gridPoint = itemPoint(input.record);
    EXPECT_FALSE(mm6::stageChestCursorPickup(input, *empty));
    EXPECT_EQ(beforeCursor, *empty);
    EXPECT_EQ(beforeChest, opened->record);
}
