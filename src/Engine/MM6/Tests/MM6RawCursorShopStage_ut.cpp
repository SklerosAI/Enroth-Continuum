#include <cstdint>
#include <exception>
#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawCursorShopStage.h"
#include "Engine/MM6/MM6ShopStockIdentity.h"

namespace {

std::unique_ptr<mm6::RawSaveFields> decodedCandidate() {
    auto saved = std::make_unique<mm6::RawSaveFields>();
    saved->heldItem.record[25] = 0xa1;
    saved->heldItem.record[26] = 0xb2;
    saved->heldItem.record[27] = 0xc3;
    saved->shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    const auto shared = mm6::guildStockRecordIndex(119, 0, GameTitle::MM6);
    saved->shops.stock.records[shared].setWord(0, 17);
    saved->shops.stock.records[shared][27] = 0xd4;
    saved->shops.clocks.deadlineWords[mm6::shopClockSlot(47)] = 123456;
    return saved;
}

} // namespace

UNIT_TEST(MM6RawCursorShopStage, StagesNativeCursorAndSharedShopClockOnCopies) {
    auto saved = decodedCandidate();
    const auto baselineShop = saved->shops;
    const auto baselineHeld = saved->heldItem;
    auto staged = mm6::stageRawCursorShop(*saved, GameTitle::MM6);
    EXPECT_TRUE(staged.cursor.ready);
    EXPECT_EQ(staged.cursor.record[25], 0xa1);
    EXPECT_EQ(staged.cursor.record[27], 0xc3);
    const auto sharedShop = mm6::shopStockRecordIndex(
        mm6::ShopStockBank::Second, 47, 0, GameTitle::MM6);
    const auto sharedGuild = mm6::guildStockRecordIndex(119, 0, GameTitle::MM6);
    EXPECT_EQ(sharedShop, sharedGuild);
    EXPECT_EQ(staged.shops.stock.records[sharedShop][27], 0xd4);
    EXPECT_EQ(mm6::shopClockSlot(47), mm6::guildClockSlot(119));
    EXPECT_EQ(staged.shops.clocks.deadlineWords[mm6::guildClockSlot(119)], 123456);
    EXPECT_NO_THROW(mm6::validateRawCursorShopJoin(
        *saved, staged.cursor, staged.shops, GameTitle::MM6));
    EXPECT_EQ(saved->shops, baselineShop);
    EXPECT_EQ(saved->heldItem, baselineHeld);
}

UNIT_TEST(MM6RawCursorShopStage, StagesPresentCursorWithoutLosingRawFields) {
    auto saved = decodedCandidate();
    saved->heldItem.present = true;
    saved->heldItem.record.setWord(0, 99);
    saved->heldItem.record.setWord(12, 0x12345678);
    const auto source = saved->heldItem;
    auto staged = mm6::stageRawCursorShop(*saved, GameTitle::MM6);
    EXPECT_EQ(staged.cursor.record, source.record);
    EXPECT_EQ(mm6::captureRawHeldCursor(staged.cursor, GameTitle::MM6), source);
    EXPECT_EQ(saved->heldItem, source);
}

UNIT_TEST(MM6RawCursorShopStage, RejectsCrossOwnerDivergenceBeforePublication) {
    auto saved = decodedCandidate();
    auto staged = mm6::stageRawCursorShop(*saved, GameTitle::MM6);
    const auto original = staged;

    staged.cursor.record[27] ^= 1;
    EXPECT_THROW(mm6::validateRawCursorShopJoin(*saved, staged.cursor, staged.shops,
                                                GameTitle::MM6), std::exception);
    staged = original;
    staged.shops.stock.records.back()[27] ^= 1;
    EXPECT_THROW(mm6::validateRawCursorShopJoin(*saved, staged.cursor, staged.shops,
                                                GameTitle::MM6), std::exception);
    staged = original;
    staged.shops.clocks.deadlineWords[mm6::guildClockSlot(119)]++;
    EXPECT_THROW(mm6::validateRawCursorShopJoin(*saved, staged.cursor, staged.shops,
                                                GameTitle::MM6), std::exception);
    staged = original;
    staged.shops.clocks.ready = false;
    EXPECT_THROW(mm6::validateRawCursorShopJoin(*saved, staged.cursor, staged.shops,
                                                GameTitle::MM6), std::exception);
    EXPECT_EQ(saved->shops, original.shops);
    EXPECT_EQ(saved->heldItem.record, original.cursor.record);
}

UNIT_TEST(MM6RawCursorShopStage, RejectsLateInvalidFieldAndForeignTitle) {
    auto saved = decodedCandidate();
    const auto original = saved->shops;
    saved->shops.stock.records.back().setWord(0, 581);
    EXPECT_THROW((void) mm6::stageRawCursorShop(*saved, GameTitle::MM6), std::exception);
    EXPECT_EQ(saved->shops.stock.records.back().itemNumberRaw(), 581);
    saved->shops = original;
    saved->shops.stock.ready = false;
    EXPECT_THROW((void) mm6::stageRawCursorShop(*saved, GameTitle::MM6), std::exception);
    saved->shops = original;
    saved->heldItem.record[24] = 1; // Empty cursor requires bytes 0..24 clear.
    EXPECT_THROW((void) mm6::stageRawCursorShop(*saved, GameTitle::MM6), std::exception);
    saved->heldItem.record[24] = 0;
    EXPECT_THROW((void) mm6::stageRawCursorShop(*saved, GameTitle::MM7), std::exception);
    EXPECT_EQ(saved->shops, original);
}
