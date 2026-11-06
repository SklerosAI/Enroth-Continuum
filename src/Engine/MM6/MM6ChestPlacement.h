#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

#include "Engine/MM6/MM6ChestBaseItemPicker.h"

namespace mm6 {

// A detached view of the original 4,204-byte chest: four header bytes,
// 140 28-byte item slots, then 140 signed 16-bit grid cells. Only the fields
// needed by the original placement pass are represented here.
struct ChestPlacementItem {
    int itemId = 0;
    int iconWidth = 0;
    int iconHeight = 0;
    int currentCharges = 0;
};

struct ChestPlacementInput {
    GameTitle title = GameTitle::MM7;
    int width = 0;
    int height = 0;
    std::uint16_t flags = 0;
    std::uint32_t rngState = 0;
    std::array<ChestPlacementItem, 140> items = {};
    std::array<std::int16_t, 140> grid = {};
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

// Model only first-time placement into an empty grid. Existing-grid replay,
// icon lookup, chest-type dimensions and the live load/save transaction remain
// deliberately outside this contract.
inline std::optional<ChestPlacementResult> placeChestItemsDetached(const ChestPlacementInput &input) {
    if (input.title != GameTitle::MM6 || input.width <= 0 || input.height <= 0 ||
        input.width > 140 || input.height > 140 || input.width * input.height > 140 ||
        (input.flags & 2u))
        return std::nullopt;
    const int area = input.width * input.height;
    for (std::int16_t cell : input.grid)
        if (cell != 0)
            return std::nullopt;
    for (const ChestPlacementItem &item : input.items) {
        if (item.itemId < 0 || item.itemId > 580 || item.currentCharges < 0)
            return std::nullopt;
        if (item.itemId != 0 && (item.iconWidth <= 0 || item.iconHeight <= 0 ||
                                 item.iconWidth > 4096 || item.iconHeight > 4096))
            return std::nullopt;
    }

    ChestPlacementResult result;
    result.items = input.items;
    result.grid = input.grid;
    result.flags = input.flags;
    result.nextRngState = input.rngState;
    auto draw = [&result]() {
        ++result.drawsConsumed;
        return nextChestRandom15(result.nextRngState);
    };

    std::array<std::uint8_t, 144> shuffledCells = {};
    for (int index = 0; index < area; ++index) {
        int position;
        do {
            if (result.drawsConsumed >= 1'000'000)
                return std::nullopt;
            position = draw() & 255;
        } while (position >= area);
        // The original stores index zero into its zero-initialized buffer, so
        // that first entry still appears vacant to this occupancy check.
        while (shuffledCells[position])
            position = (position + 1) % area;
        shuffledCells[position] = static_cast<std::uint8_t>(index);
    }

    for (int itemIndex = 0; itemIndex < area; ++itemIndex) {
        ChestPlacementItem &item = result.items[itemIndex];
        if (item.itemId == 0)
            continue;
        // The original clamps the icon dimensions to 14 pixels, then uses
        // (pixels - 14) / 32 + 1 for each footprint axis.
        const int itemWidth = (std::max(item.iconWidth, 14) - 14) / 32 + 1;
        const int itemHeight = (std::max(item.iconHeight, 14) - 14) / 32 + 1;
        int anchor = -1;
        for (int scan = 0; scan < area; ++scan) {
            const int cell = shuffledCells[scan];
            const int x = cell % input.width;
            const int y = cell / input.width;
            if (x + itemWidth > input.width || y + itemHeight > input.height)
                continue;
            bool fits = true;
            for (int dy = 0; dy < itemHeight && fits; ++dy)
                for (int dx = 0; dx < itemWidth; ++dx)
                    if (result.grid[cell + dy * input.width + dx] != 0) {
                        fits = false;
                        break;
                    }
            if (fits) {
                anchor = cell;
                break;
            }
        }
        if (anchor < 0) {
            ++result.unplacedCount;
            continue;  // The original leaves this item slot occupied but ungridded.
        }
        if (item.itemId >= 135 && item.itemId <= 159 && item.currentCharges == 0)
            item.currentCharges = 10 + draw() % 21;
        for (int dy = 0; dy < itemHeight; ++dy)
            for (int dx = 0; dx < itemWidth; ++dx)
                result.grid[anchor + dy * input.width + dx] = static_cast<std::int16_t>(-1 - anchor);
        result.grid[anchor] = static_cast<std::int16_t>(itemIndex + 1);
        ++result.placedCount;
    }
    result.flags |= 2u;
    return result;
}

}  // namespace mm6
