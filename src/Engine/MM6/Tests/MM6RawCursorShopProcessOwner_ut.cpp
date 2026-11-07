#include <exception>
#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"

namespace {

std::unique_ptr<mm6::RawSaveFields> savedPair() {
    auto saved = std::make_unique<mm6::RawSaveFields>();
    saved->heldItem.record[25] = 0xa1;
    saved->heldItem.record[26] = 0xb2;
    saved->heldItem.record[27] = 0xc3;
    saved->shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    const auto shared = mm6::guildStockRecordIndex(119, 0, GameTitle::MM6);
    saved->shops.stock.records[shared].setWord(0, 17);
    saved->shops.stock.records[shared][27] = 0xd4;
    saved->shops.clocks.deadlineWords[mm6::guildClockSlot(119)] = 123456;
    return saved;
}

struct ProcessOwnerRestore {
    mm6::RawHeldCursorOwner &cursor = mm6::rawHeldCursorState(GameTitle::MM6);
    mm6::ShopTitleState &shops = mm6::shopTitleState();
    mm6::RawHeldCursorOwner oldCursor = cursor;
    mm6::ShopTitleState oldShops = shops;

    ~ProcessOwnerRestore() {
        cursor = oldCursor;
        shops = oldShops;
    }
};

} // namespace

UNIT_TEST(MM6RawCursorShopProcessOwner, CheckedPublicationAndExplicitReset) {
    auto saved = savedPair();
    mm6::RawHeldCursorOwner cursor;
    mm6::ShopTitleState shops;
    mm6::publishRawCursorShopOwners(
        &cursor, &shops, *saved, mm6::stageRawCursorShop(*saved, GameTitle::MM6),
        GameTitle::MM6);
    EXPECT_TRUE(cursor.ready);
    EXPECT_EQ(cursor.record[27], 0xc3);
    EXPECT_EQ(shops, saved->shops);
    mm6::resetRawCursorShopOwners(&cursor, &shops, GameTitle::MM6);
    EXPECT_FALSE(cursor.ready);
    EXPECT_FALSE(shops.ready());
    EXPECT_THROW(mm6::resetRawCursorShopOwners(&cursor, &shops, GameTitle::MM7),
                 std::exception);
    EXPECT_FALSE(cursor.ready);
    EXPECT_FALSE(shops.ready());
}

UNIT_TEST(MM6RawCursorShopProcessOwner, ProcessResetLeavesBothOwnersUnready) {
    ProcessOwnerRestore restore;
    restore.cursor.ready = true;
    restore.cursor.record.setWord(0, 17);
    restore.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    EXPECT_THROW((void) mm6::rawHeldCursorState(GameTitle::MM7), std::exception);
    EXPECT_THROW(mm6::resetRawCursorShopProcessOwners(GameTitle::MM7), std::exception);
    EXPECT_TRUE(restore.cursor.ready);
    EXPECT_TRUE(restore.shops.ready());
    mm6::resetRawCursorShopProcessOwners(GameTitle::MM6);
    EXPECT_FALSE(restore.cursor.ready);
    EXPECT_FALSE(restore.shops.ready());
}

UNIT_TEST(MM6RawCursorShopProcessOwner, LateFailureLeavesBothDestinationsUntouched) {
    auto saved = savedPair();
    mm6::RawHeldCursorOwner cursor;
    cursor.ready = true;
    cursor.record.setWord(0, 19);
    mm6::ShopTitleState shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    shops.stock.records.back()[27] = 0xe1;
    const auto beforeCursor = cursor;
    const auto beforeShops = shops;

    auto staged = mm6::stageRawCursorShop(*saved, GameTitle::MM6);
    staged.shops.stock.records.back()[27] ^= 1;
    EXPECT_THROW(mm6::publishRawCursorShopOwners(&cursor, &shops, *saved,
                                                 staged, GameTitle::MM6), std::exception);
    EXPECT_EQ(cursor, beforeCursor);
    EXPECT_EQ(shops, beforeShops);

    staged = mm6::stageRawCursorShop(*saved, GameTitle::MM6);
    staged.cursor.record[27] ^= 1;
    EXPECT_THROW(mm6::publishRawCursorShopOwners(&cursor, &shops, *saved,
                                                 staged, GameTitle::MM6), std::exception);
    EXPECT_EQ(cursor, beforeCursor);
    EXPECT_EQ(shops, beforeShops);
    EXPECT_THROW(mm6::publishRawCursorShopOwners(nullptr, &shops, *saved,
                                                 staged, GameTitle::MM6), std::exception);
    EXPECT_EQ(shops, beforeShops);
}

UNIT_TEST(MM6RawCursorShopProcessOwner, ServiceReadersPreserveSharedRawRecordAndClock) {
    auto saved = savedPair();
    const auto &shops = saved->shops;
    const auto shopRecord = mm6::readShopStock(
        shops, mm6::ShopStockBank::Second, 47, 0, GameTitle::MM6);
    const auto guildRecord = mm6::readGuildStock(shops, 119, 0, GameTitle::MM6);
    EXPECT_EQ(shopRecord, guildRecord);
    EXPECT_EQ(shopRecord.itemNumberRaw(), 17);
    EXPECT_EQ(shopRecord[27], 0xd4);
    EXPECT_EQ(mm6::readShopDeadline(shops, 47, GameTitle::MM6), 123456);
    EXPECT_EQ(mm6::readGuildDeadline(shops, 119, GameTitle::MM6), 123456);
    EXPECT_THROW(mm6::readGuildStock(shops, 118, 0, GameTitle::MM6), std::exception);
    EXPECT_THROW(mm6::readShopStock(shops, mm6::ShopStockBank::First, 1, 0,
                                   GameTitle::MM7), std::exception);
    auto malformed = shops;
    malformed.stock.records[mm6::guildStockRecordIndex(119, 0, GameTitle::MM6)].setWord(0, 581);
    EXPECT_THROW(mm6::readGuildStock(malformed, 119, 0, GameTitle::MM6), std::exception);
    malformed.clocks.ready = false;
    EXPECT_THROW(mm6::readShopDeadline(malformed, 47, GameTitle::MM6), std::exception);
}
