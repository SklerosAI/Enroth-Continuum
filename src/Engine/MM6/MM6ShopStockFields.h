#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"
#include "MM6ShopStockPurchase.h"

namespace mm6 {

// The original new-game initializer covers 120 contiguous physical rows of
// twelve 28-byte records. Logical shop/guild row keys may alias a record.
constexpr std::size_t SHOP_STOCK_RECORD_COUNT = 120 * 12;

struct ShopStockFields {
    bool ready = false;
    std::array<ShopStockRecord, SHOP_STOCK_RECORD_COUNT> records = {};

    bool operator==(const ShopStockFields &) const = default;
};

ShopStockFields captureShopStock(const ShopStockFields &owner, GameTitle title);
SaveSection encodeShopStockSection(const ShopStockFields &fields);
ShopStockFields decodeShopStockSection(const SaveContainer &container);
void loadShopStockBeforeMutation(std::span<const std::uint8_t> bytes,
                                 ShopStockFields *destination, GameTitle title);

} // namespace mm6
