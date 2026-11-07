#include "MM6RawCursorShopStage.h"

#include <stdexcept>
#include <utility>

namespace mm6 {

void validateRawCursorShopJoin(const RawSaveFields &saved,
                               const RawHeldCursorOwner &cursor,
                               const ShopTitleState &shops, GameTitle title) {
    if (title != GameTitle::MM6 || !saved.shops.ready() || !shops.ready())
        throw std::runtime_error("MM6 cursor/shop join requires ready title-local owners");

    const auto held = captureRawHeldCursor(cursor, title);
    if (!held || *held != saved.heldItem || shops != saved.shops)
        throw std::runtime_error("MM6 cursor/shop owners diverge from decoded save fields");

    // Reuse each field contract before a later caller may publish either owner.
    (void) encodeRawHeldItemSection(*held, title);
    (void) encodeShopStockSection(captureShopStock(shops.stock, title));
    (void) encodeShopGuildClockSection(captureShopGuildClocks(shops.clocks, title));
    for (const ShopStockRecord &record : shops.stock.records)
        if (record.itemNumberRaw() > 580)
            throw std::runtime_error("MM6 shop owner has an invalid item number");
}

StagedRawCursorShop stageRawCursorShop(const RawSaveFields &saved, GameTitle title) {
    if (title != GameTitle::MM6 || !saved.shops.ready())
        throw std::runtime_error("MM6 cursor/shop stage requires ready decoded fields");
    auto cursor = stageRawHeldCursorFromFields(saved.heldItem, title);
    if (!cursor)
        throw std::runtime_error("MM6 decoded cursor field is invalid");
    StagedRawCursorShop staged{std::move(*cursor), saved.shops};
    validateRawCursorShopJoin(saved, staged.cursor, staged.shops, title);
    return staged;
}

} // namespace mm6
