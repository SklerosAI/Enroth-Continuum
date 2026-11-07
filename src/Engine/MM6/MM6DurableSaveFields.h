#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

struct Party;

namespace mm6 {

// The numeric bit IDs are preserved verbatim. The MM6 event/quest meaning of
// individual IDs is deliberately separate from this lossless save field.
struct DurableBits {
    std::array<std::uint8_t, 64> quest{};       // IDs 1..512.
    std::array<std::uint8_t, 26> autonotes{};   // IDs 1..208.
    std::array<std::array<std::uint8_t, 64>, 4> awards{};
    std::array<std::array<std::uint8_t, 64>, 4> characterEvents{};

    bool operator==(const DurableBits &) const = default;
};

struct DurableEffects {
    // Current runtime has nineteen condition clocks. MM6 condition meaning and
    // resistance channels still require title-specific semantic verification.
    std::array<std::array<std::int64_t, 19>, 4> conditionTicks{};

    bool operator==(const DurableEffects &) const = default;
};

struct DurableItem {
    std::uint8_t sourceIndex = 0; // MM6 CharacterInventory record index, 0..137.
    std::uint8_t zone = 0;        // 1 grid, 2 equipment.
    std::uint8_t x = 0;
    std::uint8_t y = 0;
    std::uint8_t equipmentSlot = 0; // 0 for grid, 1..16 for equipment.
    std::int32_t itemId = 0;
    std::int32_t potionPower = 0;
    std::int32_t goldAmount = 0;
    std::int32_t standardEnchantment = -1;
    std::int32_t standardStrength = 0;
    std::uint32_t specialEnchantment = 0;
    std::int32_t charges = 0;
    std::int32_t maxCharges = 0;
    std::uint32_t flags = 0;
    std::int64_t expirationTicks = 0;

    bool operator==(const DurableItem &) const = default;
};

struct DurableInventory {
    std::array<std::vector<DurableItem>, 4> characters;

    bool operator==(const DurableInventory &) const = default;
};

struct DurableSaveFields {
    DurableBits bits;
    DurableEffects effects;
    DurableInventory inventory;

    bool operator==(const DurableSaveFields &) const = default;
};

/** Encode three required, versioned QEST/EFFT/ITEM sections. */
std::array<SaveSection, 3> encodeDurableSections(const DurableSaveFields &fields);

/** Decode all three sections into a temporary value with strict bounds. */
DurableSaveFields decodeDurableSections(const SaveContainer &container);

/** The destination is assigned only after container and all fields validate. */
void loadDurableFieldsBeforeMutation(std::span<const std::uint8_t> bytes, DurableSaveFields *destination);

/** Capture durable fields from the current runtime without interpreting item IDs. */
DurableSaveFields captureDurableFields(const Party &party, GameTitle title);

/**
 * Stage verified bit/condition fields on a copy of the party. Inventory is
 * intentionally not applied until MM6 item footprints and record-index
 * reconstruction are proven. This function rejects any nonempty inventory.
 */
Party stageDurablePartyWithoutItems(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

/**
 * Stage bits, condition clocks and indexed MM6 items on a party copy. Every
 * item footprint and placement is checked against the loaded MM6 item table.
 * A changed mod footprint that no longer fits rejects the load without
 * changing the caller's party.
 */
Party stageDurableParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
