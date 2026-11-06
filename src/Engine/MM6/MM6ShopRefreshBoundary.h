#pragma once

#include <cstdint>
#include <functional>

#include "Engine/GameTitle.h"

#include "MM6ShopGuildClockFields.h"
#include "MM6ShopStockFields.h"

namespace mm6 {

using ShopStockGenerator = std::function<void(ShopStockFields &)>;

// Detached model of the original observed menu refresh branches. The caller
// supplies the next deadline calculated from its MM6 house interval. A shop
// refresh invokes both bank generators; a guild refresh invokes only its own.
// Neither function enters a live house or changes the normal SaveLoad guard.
bool refreshShopStockIfExpired(ShopStockFields *stock, ShopGuildClockFields *clocks,
                               unsigned shopRow, std::uint64_t now, std::uint64_t nextDeadline,
                               const ShopStockGenerator &firstBank,
                               const ShopStockGenerator &secondBank, GameTitle title);

bool refreshGuildStockIfExpired(ShopStockFields *stock, ShopGuildClockFields *clocks,
                                unsigned guildRow, std::uint64_t now, std::uint64_t nextDeadline,
                                const ShopStockGenerator &guild, GameTitle title);

} // namespace mm6
