#include "MM6ShopRefreshBoundary.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace mm6 {
namespace {

void requireReady(const ShopStockFields *stock, const ShopGuildClockFields *clocks,
                  std::uint64_t now, std::uint64_t nextDeadline, GameTitle title) {
    if (title != GameTitle::MM6 || !stock || !clocks || !stock->ready || !clocks->ready ||
        std::bit_cast<std::int64_t>(nextDeadline) <= std::bit_cast<std::int64_t>(now))
        throw std::runtime_error("MM6 shop refresh requires ready title-local state and a future deadline");
}

bool isExpired(std::uint64_t deadline, std::uint64_t now) {
    // The original compares the high dwords signed, then the low dwords unsigned.
    return std::bit_cast<std::int64_t>(deadline) < std::bit_cast<std::int64_t>(now);
}

void requirePolicy(ShopRefreshPolicy policy) {
    if (policy != ShopRefreshPolicy::WhenExpired && policy != ShopRefreshPolicy::OnEntry)
        throw std::runtime_error("invalid MM6 shop refresh policy");
}

} // namespace

ShopRefreshPolicy shopRefreshPolicyFromModValue(std::string_view value, GameTitle title) {
    return title == GameTitle::MM6 && value == "1" ?
        ShopRefreshPolicy::OnEntry : ShopRefreshPolicy::WhenExpired;
}

ShopRefreshPolicy shopRefreshPolicyFromEnvironment(GameTitle title) {
    const char *value = std::getenv("ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY");
    return shopRefreshPolicyFromModValue(value ? value : "", title);
}

bool refreshShopStockForEntry(ShopStockFields *stock, ShopGuildClockFields *clocks,
                              unsigned shopRow, std::uint64_t now, std::uint64_t nextDeadline,
                              const ShopStockGenerator &firstBank,
                              const ShopStockGenerator &secondBank, GameTitle title,
                              ShopRefreshPolicy policy) {
    static_assert(std::is_nothrow_move_assignable_v<ShopStockFields>);
    static_assert(std::is_nothrow_move_assignable_v<ShopGuildClockFields>);
    requireReady(stock, clocks, now, nextDeadline, title);
    requirePolicy(policy);
    const std::size_t clockSlot = shopClockSlot(shopRow);
    if (!firstBank || !secondBank)
        throw std::runtime_error("MM6 shop refresh needs both bank generators");
    if (policy == ShopRefreshPolicy::WhenExpired &&
        !isExpired(clocks->deadlineWords[clockSlot], now))
        return false;

    ShopStockFields stagedStock = *stock;
    firstBank(stagedStock);
    secondBank(stagedStock);
    ShopGuildClockFields stagedClocks = *clocks;
    stagedClocks.deadlineWords[clockSlot] = nextDeadline;
    *stock = std::move(stagedStock);
    *clocks = std::move(stagedClocks);
    return true;
}

bool refreshGuildStockForEntry(ShopStockFields *stock, ShopGuildClockFields *clocks,
                               unsigned guildRow, std::uint64_t now, std::uint64_t nextDeadline,
                               const ShopStockGenerator &guild, GameTitle title,
                               ShopRefreshPolicy policy) {
    requireReady(stock, clocks, now, nextDeadline, title);
    requirePolicy(policy);
    const std::size_t clockSlot = guildClockSlot(guildRow);
    if (!guild)
        throw std::runtime_error("MM6 guild refresh needs a generator");
    if (policy == ShopRefreshPolicy::WhenExpired &&
        !isExpired(clocks->deadlineWords[clockSlot], now))
        return false;

    ShopStockFields stagedStock = *stock;
    guild(stagedStock);
    ShopGuildClockFields stagedClocks = *clocks;
    stagedClocks.deadlineWords[clockSlot] = nextDeadline;
    *stock = std::move(stagedStock);
    *clocks = std::move(stagedClocks);
    return true;
}

bool refreshShopStockIfExpired(ShopStockFields *stock, ShopGuildClockFields *clocks,
                               unsigned shopRow, std::uint64_t now, std::uint64_t nextDeadline,
                               const ShopStockGenerator &firstBank,
                               const ShopStockGenerator &secondBank, GameTitle title) {
    return refreshShopStockForEntry(stock, clocks, shopRow, now, nextDeadline,
                                    firstBank, secondBank, title, ShopRefreshPolicy::WhenExpired);
}

bool refreshGuildStockIfExpired(ShopStockFields *stock, ShopGuildClockFields *clocks,
                                unsigned guildRow, std::uint64_t now, std::uint64_t nextDeadline,
                                const ShopStockGenerator &guild, GameTitle title) {
    return refreshGuildStockForEntry(stock, clocks, guildRow, now, nextDeadline,
                                     guild, title, ShopRefreshPolicy::WhenExpired);
}

bool ShopRefreshVisit::refreshShopOnEntry(ShopStockFields *stock, ShopGuildClockFields *clocks,
                                          unsigned shopRow, std::uint64_t now,
                                          std::uint64_t nextDeadline,
                                          const ShopStockGenerator &firstBank,
                                          const ShopStockGenerator &secondBank, GameTitle title,
                                          ShopRefreshPolicy policy) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 shop visit requires MM6 title");
    requirePolicy(policy);
    if (_entryHandled || policy == ShopRefreshPolicy::WhenExpired)
        return false;
    const bool changed = refreshShopStockForEntry(stock, clocks, shopRow, now, nextDeadline,
                                                   firstBank, secondBank, title, policy);
    if (changed)
        _entryHandled = true;
    return changed;
}

bool ShopRefreshVisit::refreshGuildOnEntry(ShopStockFields *stock, ShopGuildClockFields *clocks,
                                           unsigned guildRow, std::uint64_t now,
                                           std::uint64_t nextDeadline,
                                           const ShopStockGenerator &guild, GameTitle title,
                                           ShopRefreshPolicy policy) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 guild visit requires MM6 title");
    requirePolicy(policy);
    if (_entryHandled || policy == ShopRefreshPolicy::WhenExpired)
        return false;
    const bool changed = refreshGuildStockForEntry(stock, clocks, guildRow, now, nextDeadline,
                                                    guild, title, policy);
    if (changed)
        _entryHandled = true;
    return changed;
}

} // namespace mm6
