#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "Engine/MM6/MM6ChestBaseItemPicker.h"

namespace mm6 {

/**
 * What happens to chest items that don't fit the grid.
 */
enum class ChestOverflow {
    Hidden, // MM6.exe places the items once, in slot order, and never shows an item that didn't fit.
    Placed  // GrayFace's FixChestsByReorder, FixChestsByCompacting and FixChests.
};

// A detached view of the original 4,204-byte chest: four header bytes,
// 140 28-byte item slots, then 140 signed 16-bit grid cells. Only the fields
// needed by the original placement pass are represented here.
struct ChestPlacementItem {
    int itemId = 0;
    int iconWidth = 0;
    int iconHeight = 0;
    int currentCharges = 0;
    int value = 0; // Gold value, orders the items for `ChestOverflow::Placed`.
};

struct ChestPlacementInput {
    GameTitle title = GameTitle::MM7;
    int width = 0;
    int height = 0;
    std::uint16_t flags = 0;
    std::uint32_t rngState = 0;
    std::array<ChestPlacementItem, 140> items = {};
    std::array<std::int16_t, 140> grid = {};
    ChestOverflow overflow = ChestOverflow::Hidden;
};

struct ChestPlacementResult {
    std::uint16_t flags = 0;
    std::uint32_t nextRngState = 0;
    int drawsConsumed = 0;
    int placedCount = 0;
    int unplacedCount = 0;
    std::array<ChestPlacementItem, 140> items = {};
    std::array<std::int16_t, 140> grid = {};
};

namespace detail {

inline bool validChestPlacementInput(const ChestPlacementInput &input) {
    if (input.title != GameTitle::MM6 || input.width <= 0 || input.height <= 0 ||
        input.width > 140 || input.height > 140 || input.width * input.height > 140)
        return false;
    for (const ChestPlacementItem &item : input.items) {
        if (item.itemId < 0 || item.itemId > 580 || item.currentCharges < 0)
            return false;
        if (item.itemId != 0 && (item.iconWidth <= 0 || item.iconHeight <= 0 ||
                                 item.iconWidth > 4096 || item.iconHeight > 4096))
            return false;
    }
    return true;
}

struct ChestLayout {
    std::array<std::int16_t, 140> grid = {};
    std::vector<int> placed; // Item slots in placement order.
};

/**
 * Puts each item into the first cell of `cells` where it fits, as MM6.exe's fit and place helpers at 0x41de90 and
 * 0x41e210 do. Wand charges are left to the caller.
 *
 * @param input                         Chest dimensions and items.
 * @param grid                          Grid to start from.
 * @param order                         Item slots, in the order to place them.
 * @param cells                         Grid cells, in the order to try them.
 * @return                              The grid with the items, and the slots that got a cell.
 */
inline ChestLayout layOutChestItems(const ChestPlacementInput &input, const std::array<std::int16_t, 140> &grid,
                                    const std::vector<int> &order, const std::vector<int> &cells) {
    ChestLayout result;
    result.grid = grid;
    for (int itemIndex : order) {
        const ChestPlacementItem &item = input.items[itemIndex];
        // The original clamps the icon dimensions to 14 pixels, then uses
        // (pixels - 14) / 32 + 1 for each footprint axis.
        const int itemWidth = (std::max(item.iconWidth, 14) - 14) / 32 + 1;
        const int itemHeight = (std::max(item.iconHeight, 14) - 14) / 32 + 1;
        auto fits = [&](int cell) {
            const int x = cell % input.width;
            const int y = cell / input.width;
            if (x + itemWidth > input.width || y + itemHeight > input.height)
                return false;
            for (int dy = 0; dy < itemHeight; ++dy)
                for (int dx = 0; dx < itemWidth; ++dx)
                    if (result.grid[cell + dy * input.width + dx] != 0)
                        return false;
            return true;
        };
        const auto anchor = std::ranges::find_if(cells, fits);
        if (anchor == cells.end())
            continue;  // The original leaves this item slot occupied but ungridded.
        for (int dy = 0; dy < itemHeight; ++dy)
            for (int dx = 0; dx < itemWidth; ++dx)
                result.grid[*anchor + dy * input.width + dx] = static_cast<std::int16_t>(-1 - *anchor);
        result.grid[*anchor] = static_cast<std::int16_t>(itemIndex + 1);
        result.placed.push_back(itemIndex);
    }
    return result;
}

/**
 * @param input                         Chest items.
 * @param slots                         Occupied item slots.
 * @return                              The slots with artifacts and quest items first, then by value, most valuable
 *                                      first. Equal items keep their slot order.
 */
inline std::vector<int> mostValuableChestItemsFirst(const ChestPlacementInput &input, std::vector<int> slots) {
    std::ranges::stable_sort(slots, std::greater(), [&](int slot) {
        return std::pair(input.items[slot].itemId >= 400, input.items[slot].value);
    });
    return slots;
}

inline std::vector<int> chestCellsInRowOrder(int area) {
    std::vector<int> result(area);
    for (int cell = 0; cell < area; ++cell)
        result[cell] = cell;
    return result;
}

/**
 * Gives each placed wand that has no charges 10 to 30, as MM6.exe's place helper at 0x41e210 does.
 *
 * @param layout                        Placed item slots, in placement order.
 * @param[out] result                   Items, chest random state and draw count to update.
 */
inline void chargeChestWands(const ChestLayout &layout, ChestPlacementResult *result) {
    for (int itemIndex : layout.placed) {
        ChestPlacementItem &item = result->items[itemIndex];
        if (item.itemId >= 135 && item.itemId <= 159 && item.currentCharges == 0) {
            ++result->drawsConsumed;
            item.currentCharges = 10 + nextChestRandom15(result->nextRngState) % 21;
        }
    }
}

}  // namespace detail

// Model only first-time placement into an empty grid. Existing-grid replay,
// icon lookup, chest-type dimensions and the live load/save transaction remain
// deliberately outside this contract. With `ChestOverflow::Placed`, a chest
// whose items don't all fit the original pass is laid out again, most valuable
// items first, and compacted into rows when that shows more of the valuable
// items. The original pass's random draws stay the same.
inline std::optional<ChestPlacementResult> placeChestItemsDetached(const ChestPlacementInput &input) {
    if (!detail::validChestPlacementInput(input) || (input.flags & 2u))
        return std::nullopt;
    const int area = input.width * input.height;
    for (std::int16_t cell : input.grid)
        if (cell != 0)
            return std::nullopt;

    ChestPlacementResult result;
    result.items = input.items;
    result.flags = input.flags;
    result.nextRngState = input.rngState;

    std::array<std::uint8_t, 144> shuffledCells = {};
    for (int index = 0; index < area; ++index) {
        int position;
        do {
            if (result.drawsConsumed >= 1'000'000)
                return std::nullopt;
            ++result.drawsConsumed;
            position = nextChestRandom15(result.nextRngState) & 255;
        } while (position >= area);
        // The original stores index zero into its zero-initialized buffer, so
        // that first entry still appears vacant to this occupancy check.
        while (shuffledCells[position])
            position = (position + 1) % area;
        shuffledCells[position] = static_cast<std::uint8_t>(index);
    }
    const std::vector<int> shuffledOrder(shuffledCells.begin(), shuffledCells.begin() + area);

    std::vector<int> slots;
    for (int itemIndex = 0; itemIndex < area; ++itemIndex)
        if (input.items[itemIndex].itemId != 0)
            slots.push_back(itemIndex);
    detail::ChestLayout layout = detail::layOutChestItems(input, input.grid, slots, shuffledOrder);
    if (input.overflow == ChestOverflow::Placed && layout.placed.size() < slots.size()) {
        const std::vector<int> order = detail::mostValuableChestItemsFirst(input, slots);
        layout = detail::layOutChestItems(input, input.grid, order, shuffledOrder);
        const detail::ChestLayout compact = detail::layOutChestItems(input, input.grid, order, detail::chestCellsInRowOrder(area));
        auto placedMask = [&](const detail::ChestLayout &candidate) {
            std::vector<bool> result;
            for (int itemIndex : order)
                result.push_back(std::ranges::contains(candidate.placed, itemIndex));
            return result;
        };
        if (placedMask(compact) > placedMask(layout))
            layout = compact;
    }

    detail::chargeChestWands(layout, &result);
    result.grid = layout.grid;
    result.placedCount = layout.placed.size();
    result.unplacedCount = slots.size() - layout.placed.size();
    result.flags |= 2u;
    return result;
}

/**
 * Places the items of an already opened chest that have no cell, most valuable first, each into the first cells in row
 * order where it fits. This is GrayFace's FixChests, run each time the chest is opened again.
 *
 * @param input                         Opened chest, with its grid.
 * @return                              The chest with the items that fit now, or empty for a chest that was never
 *                                      opened or malformed input.
 */
inline std::optional<ChestPlacementResult> placeChestLeftoversDetached(const ChestPlacementInput &input) {
    if (!detail::validChestPlacementInput(input) || !(input.flags & 2u))
        return std::nullopt;
    const int area = input.width * input.height;
    std::array<bool, 140> gridded = {};
    for (int cell = 0; cell < 140; ++cell) {
        const int value = input.grid[cell];
        if ((cell >= area && value != 0) || value > area || (value > 0 && input.items[value - 1].itemId == 0))
            return std::nullopt;
        if (value > 0)
            gridded[value - 1] = true;
    }

    std::vector<int> slots;
    for (int itemIndex = 0; itemIndex < area; ++itemIndex)
        if (input.items[itemIndex].itemId != 0 && !gridded[itemIndex])
            slots.push_back(itemIndex);
    const detail::ChestLayout layout = detail::layOutChestItems(input, input.grid, detail::mostValuableChestItemsFirst(input, slots),
                                                                detail::chestCellsInRowOrder(area));

    ChestPlacementResult result;
    result.items = input.items;
    result.flags = input.flags;
    result.nextRngState = input.rngState;
    detail::chargeChestWands(layout, &result);
    result.grid = layout.grid;
    result.placedCount = layout.placed.size();
    result.unplacedCount = slots.size() - layout.placed.size();
    return result;
}

}  // namespace mm6
