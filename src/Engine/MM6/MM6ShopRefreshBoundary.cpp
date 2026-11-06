#include "MM6ShopRefreshBoundary.h"

#include <bit>
#include <cstdint>
#include <stdexcept>
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

} // namespace

bool refreshShopStockIfExpired(ShopStockFields *stock, ShopGuildClockFields *clocks,
                               unsigned shopRow, std::uint64_t now, std::uint64_t nextDeadline,
                               const ShopStockGenerator &firstBank,
                               const ShopStockGenerator &secondBank, GameTitle title) {
    requireReady(stock, clocks, now, nextDeadline, title);
    const std::size_t clockSlot = shopClockSlot(shopRow);
    if (!firstBank || !secondBank)
        throw std::runtime_error("MM6 shop refresh needs both bank generators");
    if (!isExpired(clocks->deadlineWords[clockSlot], now))
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

bool refreshGuildStockIfExpired(ShopStockFields *stock, ShopGuildClockFields *clocks,
                                unsigned guildRow, std::uint64_t now, std::uint64_t nextDeadline,
                                const ShopStockGenerator &guild, GameTitle title) {
    requireReady(stock, clocks, now, nextDeadline, title);
    const std::size_t clockSlot = guildClockSlot(guildRow);
    if (!guild)
        throw std::runtime_error("MM6 guild refresh needs a generator");
    if (!isExpired(clocks->deadlineWords[clockSlot], now))
        return false;

    ShopStockFields stagedStock = *stock;
    guild(stagedStock);
    ShopGuildClockFields stagedClocks = *clocks;
    stagedClocks.deadlineWords[clockSlot] = nextDeadline;
    *stock = std::move(stagedStock);
    *clocks = std::move(stagedClocks);
    return true;
}

} // namespace mm6
