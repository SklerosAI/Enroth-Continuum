#pragma once

#include <cstddef>
#include <stdexcept>

#include "Engine/GameTitle.h"

namespace mm6 {

// Physical record identity from the first shop bank's origin. These indices
// describe original MM6 storage aliasing, not a typed item or live owner.
// Bank menu labels and live ownership remain outside this index contract.
enum class ShopStockBank { First, Second };

inline std::size_t shopStockRecordIndex(ShopStockBank bank, unsigned row,
                                        unsigned position, GameTitle title) {
    if (title != GameTitle::MM6 || row < 1 || row > 47 || position >= 12 ||
        (bank != ShopStockBank::First && bank != ShopStockBank::Second))
        throw std::runtime_error("invalid MM6 shop stock record identity");
    const std::size_t bankRows = bank == ShopStockBank::First ? 0 : 47;
    return 12 * (bankRows + row) + position;
}

inline std::size_t guildStockRecordIndex(unsigned row, unsigned position,
                                         GameTitle title) {
    if (title != GameTitle::MM6 || row < 119 || row > 140 || position >= 12)
        throw std::runtime_error("invalid MM6 spell-guild stock record identity");
    // The first guild row begins at the last shop row's second-bank record.
    return 12 * (94 + row - 119) + position;
}

} // namespace mm6
