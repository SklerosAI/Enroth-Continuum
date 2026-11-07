#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

#include "Engine/GameTitle.h"

#include "MM6ShopGuildClockFields.h"
#include "MM6ShopStockFields.h"

namespace mm6 {

using ShopStockGenerator = std::function<void(ShopStockFields &)>;

enum class ShopRefreshPolicy {
    WhenExpired,
    OnEntry,
};

// Atlas stores the MM6 choice, but has no MM6 launch action yet. A future
// MM6 launch may pass this title-local flag; other values keep the original
// deadline schedule.
ShopRefreshPolicy shopRefreshPolicyFromModValue(std::string_view value, GameTitle title);
ShopRefreshPolicy shopRefreshPolicyFromEnvironment(GameTitle title);

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

// Call once per genuine vendor-entry transition. OnEntry runs generation even
// while the deadline is in the future, then writes the ordinary next deadline
// so switching the mod off resumes the original schedule. These detached
// functions do not open a live MM6 house or save route.
bool refreshShopStockForEntry(ShopStockFields *stock, ShopGuildClockFields *clocks,
                              unsigned shopRow, std::uint64_t now, std::uint64_t nextDeadline,
                              const ShopStockGenerator &firstBank,
                              const ShopStockGenerator &secondBank, GameTitle title,
                              ShopRefreshPolicy policy);

bool refreshGuildStockForEntry(ShopStockFields *stock, ShopGuildClockFields *clocks,
                               unsigned guildRow, std::uint64_t now, std::uint64_t nextDeadline,
                               const ShopStockGenerator &guild, GameTitle title,
                               ShopRefreshPolicy policy);

// Allocate one guard per genuine MM6 vendor window. The entry override runs
// only once for that visit; subpanel redraws cannot consume another draw.
// An absent mod leaves the normal deadline path to refresh*IfExpired. Failed
// generation does not consume the visit, allowing the caller to retry.
class ShopRefreshVisit {
 public:
    bool refreshShopOnEntry(ShopStockFields *stock, ShopGuildClockFields *clocks,
                            unsigned shopRow, std::uint64_t now, std::uint64_t nextDeadline,
                            const ShopStockGenerator &firstBank,
                            const ShopStockGenerator &secondBank, GameTitle title,
                            ShopRefreshPolicy policy);

    bool refreshGuildOnEntry(ShopStockFields *stock, ShopGuildClockFields *clocks,
                             unsigned guildRow, std::uint64_t now, std::uint64_t nextDeadline,
                             const ShopStockGenerator &guild, GameTitle title,
                             ShopRefreshPolicy policy);

 private:
    bool _entryHandled = false;
};

} // namespace mm6
