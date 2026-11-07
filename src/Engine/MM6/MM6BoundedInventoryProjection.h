#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

#include "Engine/Objects/Inventory.h"
#include "Engine/Objects/ItemEnumFunctions.h"

#include "MM6BoundedItemProjection.h"
#include "MM6RawInventoryFields.h"

namespace mm6 {

// An opt-in stage for source records whose referenced items all satisfy the
// bounded plain or standard-item gates. Unreferenced records stay in raw.
class BoundedInventoryProjection {
 public:
    BoundedInventoryProjection(const BoundedInventoryProjection &) = default;
    BoundedInventoryProjection(BoundedInventoryProjection &&) = default;
    BoundedInventoryProjection &operator=(const BoundedInventoryProjection &) = default;
    BoundedInventoryProjection &operator=(BoundedInventoryProjection &&) = default;

    CharacterInventory &inventory() { return _inventory; }
    const CharacterInventory &inventory() const { return _inventory; }

 private:
    BoundedInventoryProjection() = default;
    friend std::optional<RawCharacterInventory> captureBoundedInventory(
        const BoundedInventoryProjection &, const ItemTable &, GameTitle);
    friend std::optional<BoundedInventoryProjection> stageBoundedInventory(
        const RawCharacterInventory &, const ItemTable &, GameTitle);

    CharacterInventory _inventory = CharacterInventory::forMM6();
    RawCharacterInventory _raw;
    std::array<InventoryZone, RAW_INVENTORY_ITEM_SLOTS> _originalZones = {};
    std::array<ItemSlot, RAW_INVENTORY_ITEM_SLOTS> _originalSlots = {};
    std::array<ItemType, RAW_INVENTORY_ITEM_SLOTS> _originalTypes = {};
};

/**
 * Writes the body location byte of an item's raw record. The original paperdoll stores the one-based equipment slot
 * there on equip, and zero for the item it swaps back onto the cursor. Unequipping reads the byte to find the slot.
 *
 * @param item                          Item to update. An item without a raw record is left as is.
 * @param slot                          Equipment slot, or `ITEM_SLOT_INVALID` for an item that isn't equipped.
 */
inline void setRawBodyLocation(Item *item, ItemSlot slot) {
    if (item->mm6RawRecord)
        (*item->mm6RawRecord)[24] = static_cast<std::uint8_t>(std::to_underlying(slot));
}

/**
 * Writes the projection's runtime inventory back into its physical records. Every item supplies its own 28 bytes
 * through `Item::mm6RawRecord`, so an item may move between the grid, the paperdoll and other record indices, or
 * arrive from another owner. A record whose item left is cleared the way the original clears it.
 *
 * @param projection                    Projection whose inventory was edited after staging.
 * @param table                         The loaded MM6 item table.
 * @param title                         Must be `GameTitle::MM6`.
 * @return                              Physical inventory, or `std::nullopt` if an item lacks its raw record, has an
 *                                      unsupported field, lands on a record the original would not pick, or sits in
 *                                      an equipment slot its body location byte doesn't name.
 */
inline std::optional<RawCharacterInventory> captureBoundedInventory(
    const BoundedInventoryProjection &projection, const ItemTable &table, GameTitle title) {
    if (title != GameTitle::MM6 || pItemTable != &table || projection._inventory.capacity() != 138)
        return std::nullopt;

    // The original clears bytes 0..24 of a vacated record and keeps 25..27.
    RawCharacterInventory result = projection._raw;
    for (int index = 0; index < 138; ++index)
        if (projection._originalZones[index] != INVENTORY_ZONE_INVALID)
            std::fill(result.items[index].begin(), result.items[index].begin() + 25, 0);
    result.gridWords.fill(0);
    result.equipmentWords.fill(0);
    for (InventoryConstEntry entry : projection._inventory.entries()) {
        const int index = entry.index();
        if (index < 0 || index >= 138 || !entry->mm6RawRecord)
            return std::nullopt;
        const ItemRecord28 &carried = *entry->mm6RawRecord;
        const bool unchanged = carried == projection._raw.items[index] &&
            projection._originalZones[index] == entry.zone() &&
            (entry.zone() != INVENTORY_ZONE_EQUIPMENT || projection._originalSlots[index] == entry.slot());

        auto item = stageBoundedItem(carried, table, title);
        if (!item || (unchanged && boundedItemSourceType(*item) != projection._originalTypes[index]))
            return std::nullopt;
        if (!unchanged) {
            // The original picks the first record whose item number is zero.
            if (projection._originalZones[index] == INVENTORY_ZONE_INVALID &&
                projection._raw.items[index].itemNumberRaw())
                return std::nullopt;
            if (entry.zone() == INVENTORY_ZONE_EQUIPMENT && carried.bodyLocationRaw() != std::to_underlying(entry.slot()))
                return std::nullopt;
        }
        setBoundedItem(*item, *entry);
        auto encoded = captureBoundedItem(*item, table, title);
        if (!encoded)
            return std::nullopt;
        result.items[index] = *encoded;

        if (entry.zone() == INVENTORY_ZONE_EQUIPMENT) {
            const int slot = std::to_underlying(entry.slot()) - 1;
            if (slot < 0 || slot >= 16 || result.equipmentWords[slot])
                return std::nullopt;
            result.equipmentWords[slot] = index + 1;
        } else if (entry.zone() == INVENTORY_ZONE_GRID) {
            const Recti rect = entry.geometry();
            if (rect.w <= 0 || rect.h <= 0 || rect.x < 0 || rect.y < 0 ||
                rect.x + rect.w > 14 || rect.y + rect.h > 9)
                return std::nullopt;
            const int anchor = rect.y * 14 + rect.x;
            for (int y = rect.y; y < rect.y + rect.h; ++y) {
                for (int x = rect.x; x < rect.x + rect.w; ++x) {
                    const int cell = y * 14 + x;
                    if (result.gridWords[cell])
                        return std::nullopt;
                    result.gridWords[cell] = cell == anchor ? index + 1 :
                        std::bit_cast<std::uint32_t>(-anchor - 1);
                }
            }
        } else {
            return std::nullopt;
        }
    }
    return result;
}

inline std::optional<BoundedInventoryProjection> stageBoundedInventory(
    const RawCharacterInventory &raw, const ItemTable &table, GameTitle title) {
    if (title != GameTitle::MM6 || pItemTable != &table)
        return std::nullopt;

    BoundedInventoryProjection result;
    result._raw = raw;
    for (int cell = 0; cell < 126; ++cell) {
        const int value = std::bit_cast<std::int32_t>(raw.gridWords[cell]);
        if (value <= 0)
            continue;
        const int index = value - 1;
        if (index < 0 || index >= 138 || result._originalZones[index] != INVENTORY_ZONE_INVALID)
            return std::nullopt;
        auto item = stageBoundedItem(raw.items[index], table, title);
        if (!item || !result._inventory.tryRestoreAt(index, Pointi(cell % 14, cell / 14), boundedItem(*item)))
            return std::nullopt;
        result._originalZones[index] = INVENTORY_ZONE_GRID;
        result._originalTypes[index] = boundedItemSourceType(*item);
    }
    for (int slot = 0; slot < 16; ++slot) {
        const int value = std::bit_cast<std::int32_t>(raw.equipmentWords[slot]);
        if (value < 0)
            return std::nullopt;
        if (!value)
            continue;
        const int index = value - 1;
        if (index < 0 || index >= 138 || result._originalZones[index] != INVENTORY_ZONE_INVALID)
            return std::nullopt;
        auto item = stageBoundedItem(raw.items[index], table, title);
        const ItemSlot bodySlot = static_cast<ItemSlot>(slot + 1);
        if (!item || !result._inventory.tryRestoreAt(index, bodySlot, boundedItem(*item)))
            return std::nullopt;
        result._originalZones[index] = INVENTORY_ZONE_EQUIPMENT;
        result._originalSlots[index] = bodySlot;
        result._originalTypes[index] = boundedItemSourceType(*item);
    }

    auto replay = captureBoundedInventory(result, table, title);
    if (!replay || replay->gridWords != raw.gridWords ||
        replay->equipmentWords != raw.equipmentWords || replay->items != raw.items)
        return std::nullopt;
    return result;
}

} // namespace mm6
