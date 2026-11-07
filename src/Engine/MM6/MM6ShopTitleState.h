#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "Engine/GameTitle.h"

#include "MM6ShopGuildClockFields.h"
#include "MM6ShopStockFields.h"

namespace mm6 {

// A single MM6 process owner for the physical stock image and its keyed
// refresh deadlines. The inherited MM7 Party arrays are deliberately absent.
// SHOP is still outside KnownSaveFields and this owner has no house UI writer.
struct ShopTitleState {
    ShopStockFields stock;
    ShopGuildClockFields clocks;

    bool ready() const { return stock.ready && clocks.ready; }
    bool operator==(const ShopTitleState &) const = default;
};

inline ShopTitleState &shopTitleState() {
    static ShopTitleState owner;
    return owner;
}

inline void resetShopTitleState(ShopTitleState *owner) {
    if (owner)
        *owner = {};
}

// New Game has an empty physical image and zero deadlines. No default-ready
// global state is exposed before the caller explicitly publishes this seed.
inline ShopTitleState emptyNewGameShopTitleState(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 shop seed requires MM6 title");
    ShopTitleState result;
    result.stock.ready = true;
    result.clocks.ready = true;
    return result;
}

// Validate both independent sections on detached storage. A malformed late
// SCLK cannot publish an otherwise valid SHOP image, or vice versa.
inline ShopTitleState stageShopTitleState(std::span<const std::uint8_t> bytes,
                                          GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 shop restore requires MM6 title");
    const SaveContainer container = decodeSaveContainer(bytes);
    ShopTitleState staged;
    staged.stock = decodeShopStockSection(container);
    staged.clocks = decodeShopGuildClockSection(container);
    return staged;
}

inline void publishShopTitleState(ShopTitleState *owner, ShopTitleState staged,
                                  GameTitle title) {
    static_assert(std::is_nothrow_move_assignable_v<ShopTitleState>);
    if (title != GameTitle::MM6 || !owner || !staged.ready())
        throw std::runtime_error("MM6 shop publication requires a ready title-local owner");
    *owner = std::move(staged);
}

} // namespace mm6
