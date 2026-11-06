#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

namespace mm6 {

// Original MM6 shop rows 1..47 and spell-guild rows 119..140 use 64-bit
// refresh deadlines. Shop 47 and guild 119 address the same physical slot.
// These are raw clock words; stock, ban and bounty semantics are not inferred.
struct ShopGuildClockFields {
    bool ready = false;
    std::array<std::uint64_t, 68> deadlineWords = {};

    bool operator==(const ShopGuildClockFields &) const = default;
};

std::size_t shopClockSlot(unsigned row);
std::size_t guildClockSlot(unsigned row);

ShopGuildClockFields captureShopGuildClocks(const ShopGuildClockFields &owner, GameTitle title);
SaveSection encodeShopGuildClockSection(const ShopGuildClockFields &fields);
ShopGuildClockFields decodeShopGuildClockSection(const SaveContainer &container);
void loadShopGuildClocksBeforeMutation(std::span<const std::uint8_t> bytes,
                                      ShopGuildClockFields *destination, GameTitle title);

} // namespace mm6
