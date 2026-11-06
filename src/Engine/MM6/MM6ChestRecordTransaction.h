#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "Engine/MM6/MM6ChestDirectArtifactPicker.h"
#include "Engine/MM6/MM6ChestGridWire.h"
#include "Engine/MM6/MM6ChestOuterPlan.h"
#include "Engine/MM6/MM6ChestPlacement.h"
#include "Engine/MM6/MM6ChestTypeLayout.h"

namespace mm6 {

using ChestRecord = std::array<std::uint8_t, CHEST_WIRE_SIZE>;
using ChestRecordSet = std::array<ChestRecord, 20>;

struct ChestRecordGenerationInput {
    GameTitle title = GameTitle::MM7;
    MapTreasureLevel mapTreasureLevel = static_cast<MapTreasureLevel>(-1);
    std::uint32_t rngState = 0;
    ChestRecordSet records = {};
    std::array<std::uint8_t, 30> partyOwnedArtifactFlags = {};
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
};

struct ChestRecordGenerationResult {
    ChestRecordSet records = {};
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
    std::uint32_t nextRngState = 0;
    int markersVisited = 0;
    int rewardsGenerated = 0;
};

inline std::uint16_t readChest16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return std::uint16_t(bytes[offset]) | (std::uint16_t(bytes[offset + 1]) << 8);
}

inline std::int32_t readChest32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    const std::uint32_t bits = std::uint32_t(bytes[offset]) |
                               (std::uint32_t(bytes[offset + 1]) << 8) |
                               (std::uint32_t(bytes[offset + 2]) << 16) |
                               (std::uint32_t(bytes[offset + 3]) << 24);
    return std::bit_cast<std::int32_t>(bits);
}

inline void writeChest16(std::span<std::uint8_t> bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

inline void writeChest32(std::span<std::uint8_t> bytes, std::size_t offset, std::int32_t value) {
    const auto bits = std::bit_cast<std::uint32_t>(value);
    for (int i = 0; i < 4; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(bits >> (i * 8));
}

// Stages all twenty original-size chest records, preserving fixed-item bytes
// and every opaque byte. A generated target must be a blank or marker slot;
// this is true of all retained baselines, but changed-save semantics remain
// a separate live gate. The explicit direct-artifact policy excludes ID 430,
// applies ownership/generated uniqueness and caps their union at thirteen.
inline std::optional<ChestRecordGenerationResult> stageChestRecordGeneration(
    const ChestRecordGenerationInput &input, const ItemTable &table) {
    if (input.title != GameTitle::MM6)
        return std::nullopt;
    ChestOuterInput outerInput;
    outerInput.title = input.title;
    outerInput.mapTreasureLevel = input.mapTreasureLevel;
    outerInput.rngState = input.rngState;
    for (int chestIndex = 0; chestIndex < 20; ++chestIndex) {
        const ChestRecord &record = input.records[chestIndex];
        if (readChest16(record, 0) >= 8)
            return std::nullopt;
        const auto grid = decodeChestGridWire(input.title, record);
        if (!grid)
            return std::nullopt;
        bool hasMarker = false;
        for (int slot = 0; slot < 140; ++slot) {
            const auto item = std::span(record).subspan(4 + slot * 28, 28);
            const int id = readChest32(item, 0);
            outerInput.itemIds[chestIndex][slot] = id;
            if (id <= 0 && std::any_of(item.begin() + 4, item.end(), [](std::uint8_t byte) {
                    return byte != 0;
                }))
                return std::nullopt;
            hasMarker |= id < 0;
        }
        if (hasMarker && std::any_of(grid->begin(), grid->end(), [](std::int16_t cell) {
                return cell != 0;
            }))
            return std::nullopt;
    }

    struct PickedFields {
        ChestRewardSource source = ChestRewardSource::Empty;
        ChestFinalizedItem item;
        int directId = 0;
    };
    std::vector<PickedFields> picks;
    auto generated = input.generatedArtifactFlags;
    auto any = [&](ItemTreasureLevel tier, std::uint32_t state) -> std::optional<ChestOuterReward> {
        const auto item = stageChestAnyFromItemTable(input.title, tier, table, generated, state);
        if (!item)
            return std::nullopt;
        generated = item->generatedArtifactFlags;
        picks.push_back({ChestRewardSource::AnyItem, *item, 0});
        return ChestOuterReward{item->itemId, item->nextRngState};
    };
    auto direct = [&](ItemTreasureLevel, std::uint32_t state) -> std::optional<ChestOuterReward> {
        const auto item = pickDirectChestArtifactSafePolicy(input.title, input.partyOwnedArtifactFlags,
                                                            generated, state);
        if (!item)
            return std::nullopt;
        generated = item->generatedArtifactFlags;
        picks.push_back({ChestRewardSource::DirectArtifact, {}, item->itemId});
        return ChestOuterReward{item->itemId, item->nextRngState};
    };
    const auto outer = stageChestOuterIds(outerInput, any, direct);
    if (!outer)
        return std::nullopt;

    ChestRecordGenerationResult result;
    result.records = input.records;
    result.generatedArtifactFlags = generated;
    result.nextRngState = outer->nextRngState;
    result.markersVisited = outer->markersVisited;
    result.rewardsGenerated = outer->rewardsGenerated;
    std::size_t nextPick = 0;
    for (const ChestOuterSelection &selection : outer->selections) {
        auto item = std::span(result.records[selection.chestIndex]).subspan(4 + selection.slotIndex * 28, 28);
        std::fill(item.begin(), item.end(), 0);
        if (selection.source == ChestRewardSource::Empty)
            continue;
        writeChest32(item, 0, selection.itemId);
        if (selection.source == ChestRewardSource::Gold) {
            writeChest32(item, 12, selection.goldAmount);
            continue;
        }
        if (nextPick >= picks.size() || picks[nextPick].source != selection.source)
            return std::nullopt;
        const PickedFields &picked = picks[nextPick++];
        if (selection.source == ChestRewardSource::DirectArtifact) {
            if (selection.itemId != picked.directId)
                return std::nullopt;
            continue;
        }
        if (selection.itemId != picked.item.itemId)
            return std::nullopt;
        writeChest32(item, 4, picked.item.standardAttributeIndex);
        writeChest32(item, 8, picked.item.standardStrength);
        writeChest32(item, 12, picked.item.specialEnchantmentId);
        writeChest32(item, 16, picked.item.numCharges);
        writeChest32(item, 20, std::bit_cast<std::int32_t>(picked.item.flags));
    }
    if (nextPick != picks.size())
        return std::nullopt;
    return result;
}

struct ChestIconSize {
    int width = 0;
    int height = 0;
};
using ChestIconLookup = std::function<std::optional<ChestIconSize>(int itemId)>;

struct ChestRecordPlacementResult {
    ChestRecord record = {};
    std::uint32_t nextRngState = 0;
    int placedCount = 0;
    int unplacedCount = 0;
};

// Original placement runs at first chest-open, after map-level generation.
// It is deliberately a separate transaction and does not mutate a live chest.
inline std::optional<ChestRecordPlacementResult> stageFirstChestRecordPlacement(
    GameTitle title, const ChestRecord &record, std::span<const std::uint8_t> dchest,
    std::uint32_t rngState, const ChestIconLookup &iconLookup) {
    if (title != GameTitle::MM6 || !iconLookup)
        return std::nullopt;
    const auto layout = chestTypeLayout(title, readChest16(record, 0), dchest);
    const auto grid = decodeChestGridWire(title, record);
    if (!layout || !grid)
        return std::nullopt;
    ChestPlacementInput input;
    input.title = title;
    input.width = layout->width;
    input.height = layout->height;
    input.flags = readChest16(record, 2);
    input.rngState = rngState;
    input.grid = *grid;
    for (int slot = 0; slot < 140; ++slot) {
        const auto item = std::span(record).subspan(4 + slot * 28, 28);
        const int id = readChest32(item, 0);
        if (id < 0 || id > 580)
            return std::nullopt;
        input.items[slot].itemId = id;
        input.items[slot].currentCharges = readChest32(item, 16);
        if (id != 0) {
            const auto icon = iconLookup(id);
            if (!icon)
                return std::nullopt;
            input.items[slot].iconWidth = icon->width;
            input.items[slot].iconHeight = icon->height;
        }
    }
    const auto placed = placeChestItemsDetached(input);
    if (!placed)
        return std::nullopt;
    const auto encoded = encodeChestGridWire(title, record, placed->grid);
    if (!encoded)
        return std::nullopt;
    ChestRecordPlacementResult result;
    result.record = *encoded;
    writeChest16(result.record, 2, placed->flags);
    for (int slot = 0; slot < 140; ++slot)
        if (placed->items[slot].currentCharges != input.items[slot].currentCharges)
            writeChest32(std::span(result.record).subspan(4 + slot * 28, 28), 16,
                         placed->items[slot].currentCharges);
    result.nextRngState = placed->nextRngState;
    result.placedCount = placed->placedCount;
    result.unplacedCount = placed->unplacedCount;
    return result;
}

}  // namespace mm6
