#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

#include "Engine/MM6/MM6ChestRecordTransaction.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/ItemTable.h"

namespace mm6 {

static_assert(std::endian::native == std::endian::little);

// A bounded MM6 chest grid in the MM7-shaped intermediate. The active 9x9
// area may reference only slots the current ChestInventory reconstructor can
// restore; the remaining 59 wire cells are carried by the raw sidecar.
inline bool validChestRuntimeGrid(GameTitle title, const Chest_MM7 &chest) {
    if (title != GameTitle::MM6 || chest.chestTypeId >= 8)
        return false;
    for (const Item_MM7 &item : chest.items)
        if (item.itemId < -7 || item.itemId > 580)
            return false;
    bool hasGrid = false;
    for (int cell = 0; cell < 140; ++cell)
        hasGrid |= chest.inventoryMatrix[cell] != 0;
    if (hasGrid && !(chest.flags & 2u))
        return false;
    for (int cell = 0; cell < 81; ++cell) {
        const int value = chest.inventoryMatrix[cell];
        if (value > 0 && (value > 81 || chest.items[value - 1].itemId <= 0))
            return false;
        if (value < 0) {
            const int anchor = -value - 1;
            if (anchor < 0 || anchor >= cell || chest.inventoryMatrix[anchor] <= 0)
                return false;
        }
    }
    return true;
}

struct ChestRuntimeProjection {
    Chest chest;
    ChestRecord wireSidecar = {};
};

// This is an opt-in stage for a resolved MM6 chest, not a map-load hook. The
// existing ChestInventory reconstructor is allowed only when it round-trips
// the active grid and the supported first 24 item bytes exactly. Each occupied
// Item also carries its complete source record for later item-value moves.
inline std::optional<ChestRuntimeProjection> stageChestRuntimeProjection(
    GameTitle title, const ChestRecord &record, int chestIndex) {
    if (title != GameTitle::MM6 || chestIndex < 0 || chestIndex >= 20 || !pItemTable ||
        readChest16(record, 0) >= 8)
        return std::nullopt;
    const int type = readChest16(record, 0);
    if (chestTable[type].size != Sizei(9, 9))
        return std::nullopt;

    Chest_MM7 source{};
    source.chestTypeId = type;
    source.flags = readChest16(record, 2);
    for (int slot = 0; slot < 140; ++slot) {
        const auto bytes = std::span(record).subspan(4 + slot * 28, 28);
        std::memcpy(&source.items[slot], bytes.data(), 28);
        const int id = source.items[slot].itemId;
        if (id < 0 || id > 580)
            return std::nullopt;
        if (id > 0) {
            const Sizei size = pItemTable->itemSizes[static_cast<ItemId>(id)];
            if (size.w <= 0 || size.h <= 0 || size.w > 9 || size.h > 9)
                return std::nullopt;
        }
    }
    const auto grid = decodeChestGridWire(title, record);
    if (!grid || std::any_of(grid->begin() + 81, grid->end(), [](std::int16_t cell) {
            return cell != 0;
        }))
        return std::nullopt;
    source.inventoryMatrix = *grid;
    if (!validChestRuntimeGrid(title, source))
        return std::nullopt;

    ChestRuntimeProjection result;
    result.wireSidecar = record;
    reconstruct(source, &result.chest, tags::context<int>(chestIndex));
    Chest_MM7 replay{};
    snapshot(result.chest, &replay);
    if (replay.chestTypeId != source.chestTypeId || replay.flags != source.flags ||
        replay.inventoryMatrix != source.inventoryMatrix)
        return std::nullopt;
    for (int slot = 0; slot < 140; ++slot)
        if (std::memcmp(&replay.items[slot], &source.items[slot], 24) != 0)
            return std::nullopt;
    for (int slot = 0; slot < 140; ++slot) {
        if (source.items[slot].itemId <= 0)
            continue;
        InventoryEntry entry = result.chest.inventory.entry(slot);
        if (!entry)
            return std::nullopt;
        ItemRecord28 raw;
        const auto bytes = std::span(record).subspan(4 + slot * 28, 28);
        std::copy(bytes.begin(), bytes.end(), raw.begin());
        entry->mm6RawRecord = raw;
    }
    return result;
}

// Copy live supported fields onto the original 28-byte records and all 140
// grid cells. Occupied slots must still own their exact source record and
// unchanged supported fields. Bytes 24..27 stay in the sidecar: equipped-slot
// and lich-jar fields in the MM7-shaped Item cannot be assumed equivalent for
// MM6. New or modified chest items require a separate proved writer.
inline std::optional<ChestRecord> stageChestRuntimeRecord(
    GameTitle title, const ChestRuntimeProjection &projection) {
    if (title != GameTitle::MM6)
        return std::nullopt;
    Chest_MM7 live{};
    snapshot(projection.chest, &live);
    if (!validChestRuntimeGrid(title, live) || live.chestTypeId != readChest16(projection.wireSidecar, 0))
        return std::nullopt;
    if (std::any_of(live.inventoryMatrix.begin() + 81, live.inventoryMatrix.end(),
                    [](std::int16_t cell) { return cell != 0; }))
        return std::nullopt;
    for (const Item_MM7 &item : live.items)
        if (item.itemId < 0)
            return std::nullopt;
    ChestRecord result = projection.wireSidecar;
    writeChest16(result, 2, live.flags);
    for (int slot = 0; slot < 140; ++slot) {
        auto bytes = std::span(result).subspan(4 + slot * 28, 28);
        if (live.items[slot].itemId > 0) {
            InventoryConstEntry entry = projection.chest.inventory.entry(slot);
            if (!entry || !entry->mm6RawRecord ||
                !std::equal(bytes.begin(), bytes.end(), entry->mm6RawRecord->begin()) ||
                std::memcmp(&live.items[slot], bytes.data(), 24) != 0)
                return std::nullopt;
        }
        std::memcpy(bytes.data(), &live.items[slot], 24);
    }
    const auto encoded = encodeChestGridWire(title, result, live.inventoryMatrix);
    if (!encoded)
        return std::nullopt;
    return *encoded;
}

}  // namespace mm6
