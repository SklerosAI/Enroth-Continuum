#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <span>

#include "Engine/MM6/MM6ChestRuntimeProjection.h"

namespace mm6 {

enum class ChestInteractionAction {
    OpenOnly,
    TakeGridItem
};

struct ChestFirstOpenInput {
    GameTitle title = GameTitle::MM7;
    ChestRecord record = {};
    int chestIndex = -1;
    std::span<const std::uint8_t> dchest;
    std::uint32_t lcgState = 0;
    ChestIconLookup iconLookup;
    ChestInteractionAction action = ChestInteractionAction::OpenOnly;
    Pointi gridPoint;
    Item heldItem;
    int partyGold = 0;
    // The original MM6 gold-share/hireling result is not yet proved. The
    // caller must provide the exact nonnegative net credit for gold pickup.
    std::optional<int> netGoldCredit;
};

struct ChestFirstOpenResult {
    ChestRecord record = {};
    std::uint32_t nextLcgState = 0;
    Item heldItem;
    int partyGold = 0;
    int grossGold = 0;
    bool placed = false;
    bool itemTaken = false;
};

// Detached first-open plus one grid pickup. All chest, RNG, held-item and gold
// updates are staged in a returned value. The caller must atomically publish
// it with its active-map sidecar and title state; this function does not touch
// live owners. Traps and repeat-placement debug options remain unproved.
inline std::optional<ChestFirstOpenResult> stageChestFirstOpenInteraction(
    const ChestFirstOpenInput &input) {
    if (input.title != GameTitle::MM6 || input.chestIndex < 0 || input.chestIndex >= 20 ||
        input.partyGold < 0 || !chestTypeLayout(input.title, readChest16(input.record, 0), input.dchest) ||
        (readChest16(input.record, 2) & 1u))
        return std::nullopt;

    ChestFirstOpenResult result;
    result.record = input.record;
    result.nextLcgState = input.lcgState;
    result.heldItem = input.heldItem;
    result.partyGold = input.partyGold;
    if (!(readChest16(result.record, 2) & 2u)) {
        const auto placed = stageFirstChestRecordPlacement(input.title, result.record, input.dchest,
                                                           result.nextLcgState, input.iconLookup);
        if (!placed)
            return std::nullopt;
        result.record = placed->record;
        result.nextLcgState = placed->nextRngState;
        result.placed = true;
    }

    auto projection = stageChestRuntimeProjection(input.title, result.record, input.chestIndex);
    if (!projection)
        return std::nullopt;
    if (input.action == ChestInteractionAction::OpenOnly)
        return result;
    if (input.action != ChestInteractionAction::TakeGridItem ||
        input.gridPoint.x < 0 || input.gridPoint.x >= 9 ||
        input.gridPoint.y < 0 || input.gridPoint.y >= 9 ||
        input.heldItem.itemId != ITEM_NULL)
        return std::nullopt;

    InventoryEntry entry = projection->chest.inventory.entry(input.gridPoint);
    if (!entry)
        return std::nullopt;
    const Item taken = projection->chest.inventory.take(entry);
    const int id = std::to_underlying(taken.itemId);
    if (id >= 197 && id <= 199) {
        if (taken.goldAmount <= 0 || !input.netGoldCredit || *input.netGoldCredit < 0 ||
            *input.netGoldCredit > std::numeric_limits<int>::max() - result.partyGold)
            return std::nullopt;
        result.grossGold = taken.goldAmount;
        result.partyGold += *input.netGoldCredit;
    } else {
        if (input.netGoldCredit)
            return std::nullopt;
        result.heldItem = taken;
    }
    const auto changed = stageChestRuntimeRecord(input.title, *projection);
    if (!changed)
        return std::nullopt;
    result.record = *changed;
    result.itemTaken = true;
    return result;
}

}  // namespace mm6
