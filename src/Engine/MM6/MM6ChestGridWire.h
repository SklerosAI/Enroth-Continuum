#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

namespace mm6 {

inline constexpr std::size_t CHEST_WIRE_SIZE = 4204;
inline constexpr std::size_t CHEST_GRID_CELLS = 140;
inline constexpr std::size_t CHEST_GRID_OFFSET = 4 + 140 * 28;
static_assert(CHEST_GRID_OFFSET + CHEST_GRID_CELLS * 2 == CHEST_WIRE_SIZE);

/**
 * Cells of one chest page. Every MM6 chest type is 9 x 9, and MM6.exe never reads the record's grid cells past them.
 */
inline constexpr int CHEST_PAGE_CELLS = 81;

/**
 * An item on a chest page after the first.
 */
struct ChestPageItem {
    int page = 0; // 1 for the second page.
    int slot = 0; // Item slot in the chest record.
    int cell = 0; // Grid cell of the item's top left corner.

    bool operator==(const ChestPageItem &) const = default;
};

// The original MM6 map writer copies the entire in-memory chest record into
// the delta buffer. These helpers only read or replace its signed LE grid;
// they do not validate live placement, mutate a Chest, or enable MM6 saves.
inline std::optional<std::array<std::int16_t, CHEST_GRID_CELLS>> decodeChestGridWire(
    GameTitle title, std::span<const std::uint8_t> record) {
    if (title != GameTitle::MM6 || record.size() != CHEST_WIRE_SIZE)
        return std::nullopt;
    std::array<std::int16_t, CHEST_GRID_CELLS> grid{};
    for (std::size_t i = 0; i < grid.size(); ++i) {
        const std::size_t offset = CHEST_GRID_OFFSET + 2 * i;
        const std::uint16_t bits = std::uint16_t(record[offset]) |
                                   (std::uint16_t(record[offset + 1]) << 8);
        grid[i] = std::bit_cast<std::int16_t>(bits);
    }
    return grid;
}

inline std::optional<std::array<std::uint8_t, CHEST_WIRE_SIZE>> encodeChestGridWire(
    GameTitle title, std::span<const std::uint8_t> originalRecord,
    const std::array<std::int16_t, CHEST_GRID_CELLS> &grid) {
    if (title != GameTitle::MM6 || originalRecord.size() != CHEST_WIRE_SIZE)
        return std::nullopt;
    std::array<std::uint8_t, CHEST_WIRE_SIZE> result{};
    std::copy(originalRecord.begin(), originalRecord.end(), result.begin());
    for (std::size_t i = 0; i < grid.size(); ++i) {
        const std::size_t offset = CHEST_GRID_OFFSET + 2 * i;
        const auto bits = std::bit_cast<std::uint16_t>(grid[i]);
        result[offset] = static_cast<std::uint8_t>(bits);
        result[offset + 1] = static_cast<std::uint8_t>(bits >> 8);
    }
    return result;
}

/**
 * Reads the items on later chest pages from the grid cells past the first page. Each of those cells holds one item as
 * `1 + slot + 140 * cell`, in page order, and 0 after the last. The first item is on the second page, and an item whose
 * value is negated starts the next page.
 *
 * @param grid                          Chest grid.
 * @param area                          Cells of one page.
 * @return                              Items on later pages, or empty for cells that don't hold a list of items.
 */
inline std::optional<std::vector<ChestPageItem>> decodeChestPages(const std::array<std::int16_t, CHEST_GRID_CELLS> &grid, int area) {
    constexpr int slots = CHEST_GRID_CELLS;
    if (area <= 0 || area > slots)
        return std::nullopt;
    std::vector<ChestPageItem> result;
    int cell = area;
    for (; cell < slots && grid[cell] != 0; ++cell) {
        const int value = std::abs(grid[cell]) - 1;
        const int page = result.empty() ? 1 : result.back().page + (grid[cell] < 0);
        if ((result.empty() && grid[cell] < 0) || value / slots >= area)
            return std::nullopt;
        result.push_back({page, value % slots, value / slots});
    }
    if (std::any_of(grid.begin() + cell, grid.end(), [](std::int16_t value) { return value != 0; }))
        return std::nullopt;
    return result;
}

/**
 * @param grid                          Chest grid with its first page.
 * @param area                          Cells of one page.
 * @param items                         Items on later pages, in page order, starting on the second page without a
 *                                      page left empty.
 * @return                              The grid with the items in the cells past the first page, or empty if they
 *                                      don't fit there.
 */
inline std::optional<std::array<std::int16_t, CHEST_GRID_CELLS>> encodeChestPages(std::array<std::int16_t, CHEST_GRID_CELLS> grid, int area,
                                                                                  std::span<const ChestPageItem> items) {
    constexpr int slots = CHEST_GRID_CELLS;
    if (area <= 0 || area + items.size() > CHEST_GRID_CELLS)
        return std::nullopt;
    std::fill(grid.begin() + area, grid.end(), 0);
    for (std::size_t index = 0; index < items.size(); ++index) {
        const ChestPageItem &item = items[index];
        const int previousPage = index == 0 ? 1 : items[index - 1].page;
        if (item.page < previousPage || item.page > previousPage + (index > 0) || item.slot < 0 || item.slot >= slots ||
            item.cell < 0 || item.cell >= area)
            return std::nullopt;
        const int value = 1 + item.slot + slots * item.cell;
        if (value > std::numeric_limits<std::int16_t>::max())
            return std::nullopt;
        grid[area + index] = static_cast<std::int16_t>(item.page > previousPage ? -value : value);
    }
    return grid;
}

}  // namespace mm6
