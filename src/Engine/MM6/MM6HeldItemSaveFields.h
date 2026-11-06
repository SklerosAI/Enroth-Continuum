#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

struct Party;

namespace mm6 {

// Item scalars stored without MM7 item-category dispatch. The cursor bitmap
// and click offset are UI state, reconstructed only after a full load commits.
struct HeldItemSaveFields {
    bool present = false;
    std::int32_t itemId = 0;
    std::int32_t potionPower = 0;
    std::int32_t goldAmount = 0;
    std::int32_t standardEnchantment = -1;
    std::int32_t standardStrength = 0;
    std::uint32_t specialEnchantment = 0;
    std::int32_t charges = 0;
    std::int32_t maxCharges = 0;
    std::uint32_t flags = 0;
    std::int32_t lichJarCharacterIndex = -1;
    std::int64_t expirationTicks = 0;

    bool operator==(const HeldItemSaveFields &) const = default;
};

/** Encode one required, versioned HELD section after bounded validation. */
SaveSection encodeHeldItemSection(const HeldItemSaveFields &fields);
HeldItemSaveFields decodeHeldItemSection(const SaveContainer &container);
void loadHeldItemBeforeMutation(std::span<const std::uint8_t> bytes,
                                HeldItemSaveFields *destination);

/** Capture from an active MM6 party; an empty cursor becomes canonical empty fields. */
HeldItemSaveFields captureHeldItem(const Party &party, GameTitle title);

/** Validate the item against loaded MM6 tables and assign only to a party copy. */
Party stageHeldItemParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
