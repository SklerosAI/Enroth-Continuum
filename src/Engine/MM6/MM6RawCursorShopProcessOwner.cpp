#include "MM6RawCursorShopProcessOwner.h"

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace mm6 {
namespace {

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 raw cursor/shop owner requires MM6 title");
}

void requireReadyShops(const ShopTitleState &shops, GameTitle title) {
    requireMM6(title);
    if (!shops.ready())
        throw std::runtime_error("MM6 service read requires ready SHOP and SCLK owners");
}

ShopStockRecord checkedRecord(const ShopTitleState &shops, std::size_t index,
                              GameTitle title) {
    requireReadyShops(shops, title);
    const ShopStockRecord record = shops.stock.records.at(index);
    if (record.itemNumberRaw() > 580)
        throw std::runtime_error("MM6 service stock has an invalid item number");
    return record;
}

} // namespace

RawHeldCursorOwner &rawHeldCursorState(GameTitle title) {
    requireMM6(title);
    static RawHeldCursorOwner owner;
    return owner;
}

void resetRawCursorShopOwners(RawHeldCursorOwner *cursor, ShopTitleState *shops,
                              GameTitle title) {
    static_assert(std::is_nothrow_move_assignable_v<RawHeldCursorOwner>);
    static_assert(std::is_nothrow_move_assignable_v<ShopTitleState>);
    requireMM6(title);
    if (!cursor || !shops)
        throw std::runtime_error("MM6 raw cursor/shop reset needs both destinations");
    *cursor = RawHeldCursorOwner{};
    *shops = ShopTitleState{};
}

void resetRawCursorShopProcessOwners(GameTitle title) {
    requireMM6(title);
    resetRawCursorShopOwners(&rawHeldCursorState(title), &shopTitleState(), title);
}

void publishRawCursorShopOwners(RawHeldCursorOwner *cursor, ShopTitleState *shops,
                                const RawSaveFields &saved, StagedRawCursorShop staged,
                                GameTitle title) {
    static_assert(std::is_nothrow_move_assignable_v<RawHeldCursorOwner>);
    static_assert(std::is_nothrow_move_assignable_v<ShopTitleState>);
    requireMM6(title);
    if (!cursor || !shops)
        throw std::runtime_error("MM6 raw cursor/shop publication needs both destinations");
    validateRawCursorShopJoin(saved, staged.cursor, staged.shops, title);
    *cursor = std::move(staged.cursor);
    *shops = std::move(staged.shops);
}

ShopStockRecord readShopStock(const ShopTitleState &shops, ShopStockBank bank,
                              unsigned row, unsigned position, GameTitle title) {
    requireReadyShops(shops, title);
    return checkedRecord(shops, shopStockRecordIndex(bank, row, position, title), title);
}

ShopStockRecord readGuildStock(const ShopTitleState &shops, unsigned row,
                               unsigned position, GameTitle title) {
    requireReadyShops(shops, title);
    return checkedRecord(shops, guildStockRecordIndex(row, position, title), title);
}

std::uint64_t readShopDeadline(const ShopTitleState &shops, unsigned row,
                               GameTitle title) {
    requireReadyShops(shops, title);
    return shops.clocks.deadlineWords.at(shopClockSlot(row));
}

std::uint64_t readGuildDeadline(const ShopTitleState &shops, unsigned row,
                                GameTitle title) {
    requireReadyShops(shops, title);
    return shops.clocks.deadlineWords.at(guildClockSlot(row));
}

} // namespace mm6
