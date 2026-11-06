#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

#include "Engine/GameTitle.h"

#include "MM6ItemRecord28.h"

namespace mm6 {

using ShopStockRecord = ItemRecord28;

// Apply only after a caller has accepted a purchase and secured inventory space.
// The original MM6 success paths copy all 28 bytes, then clear bytes 0..24 of
// the source record. The last three bytes remain in the physical stock slot.
inline ShopStockRecord takePurchasedStockRecord(ShopStockRecord &source, GameTitle title) {
    if (title != GameTitle::MM6 ||
        std::all_of(source.begin(), source.begin() + 4, [](std::uint8_t byte) { return byte == 0; }))
        throw std::runtime_error("invalid MM6 purchased stock record");

    ShopStockRecord purchased = source;
    std::fill(source.begin(), source.begin() + 25, 0);
    return purchased;
}

} // namespace mm6
