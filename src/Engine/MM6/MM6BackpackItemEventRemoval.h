#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#include "MM6BoundedPartyInventoryProjection.h"

namespace mm6 {

// A bounded, detached version of the original MM6 item-event backpack removal.
// Only a positive one-based grid anchor is accepted. The event's matching and
// quantity rules remain the caller's responsibility; this helper does not
// authorize general UI pickup, equipment removal, or normal SaveLoad.
inline std::optional<ItemRecord28> tryRemovePhysicalBackpackItemForEvent(
    RawInventoryFields &raw, std::size_t characterIndex, std::size_t anchorCell,
    CharacterInventory &inventory, const ItemTable &table, GameTitle title) {
    static_assert(std::is_nothrow_move_assignable_v<RawInventoryFields>);
    static_assert(std::is_nothrow_move_assignable_v<CharacterInventory>);

    if (title != GameTitle::MM6 || !raw.ready ||
        characterIndex >= raw.characters.size() || anchorCell >= RAW_INVENTORY_GRID_CELLS)
        return std::nullopt;

    // Validate all four physical owners and prove the selected live view still
    // agrees with its raw owner before constructing a candidate transaction.
    auto before = stageBoundedPartyInventory(raw, table, title);
    if (!before)
        return std::nullopt;
    before->inventory(characterIndex) = inventory;
    auto recapturedBefore = captureBoundedPartyInventory(*before, table, title);
    if (!recapturedBefore || *recapturedBefore != raw)
        return std::nullopt;

    const RawCharacterInventory &source = raw.characters[characterIndex];
    const auto reference = std::bit_cast<std::int32_t>(source.gridWords[anchorCell]);
    if (reference <= 0 || reference > int(RAW_INVENTORY_ITEM_SLOTS))
        return std::nullopt;
    const int index = reference - 1;
    InventoryConstEntry entry = inventory.entry(index);
    if (!entry || entry.zone() != INVENTORY_ZONE_GRID ||
        source.items[index].itemNumberRaw() == 0)
        return std::nullopt;

    const Recti rect = entry.geometry();
    if (rect.w <= 0 || rect.h <= 0 || rect.x < 0 || rect.y < 0 ||
        rect.x + rect.w > 14 || rect.y + rect.h > 9 ||
        std::size_t(rect.y * 14 + rect.x) != anchorCell)
        return std::nullopt;
    for (int y = rect.y; y < rect.y + rect.h; ++y) {
        for (int x = rect.x; x < rect.x + rect.w; ++x) {
            const int cell = y * 14 + x;
            const auto expected = cell == int(anchorCell) ? std::uint32_t(index + 1) :
                std::bit_cast<std::uint32_t>(-int(anchorCell) - 1);
            if (source.gridWords[cell] != expected)
                return std::nullopt;
        }
    }

    RawInventoryFields stagedRaw = raw;
    CharacterInventory stagedInventory = inventory;
    ItemRecord28 removed = source.items[index];
    RawCharacterInventory &candidate = stagedRaw.characters[characterIndex];
    for (int byte = 0; byte < 25; ++byte)
        candidate.items[index][byte] = 0;
    for (int y = rect.y; y < rect.y + rect.h; ++y)
        for (int x = rect.x; x < rect.x + rect.w; ++x)
            candidate.gridWords[y * 14 + x] = 0;
    stagedInventory.take(stagedInventory.entry(index));

    auto after = stageBoundedPartyInventory(stagedRaw, table, title);
    if (!after)
        return std::nullopt;
    after->inventory(characterIndex) = stagedInventory;
    auto recapturedAfter = captureBoundedPartyInventory(*after, table, title);
    if (!recapturedAfter || *recapturedAfter != stagedRaw)
        return std::nullopt;

    raw = std::move(stagedRaw);
    inventory = std::move(stagedInventory);
    return removed;
}

} // namespace mm6
