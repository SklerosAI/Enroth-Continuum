#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "Engine/GameTitle.h"

namespace mm6 {

inline constexpr std::size_t CHEST_WIRE_SIZE = 4204;
inline constexpr std::size_t CHEST_GRID_CELLS = 140;
inline constexpr std::size_t CHEST_GRID_OFFSET = 4 + 140 * 28;
static_assert(CHEST_GRID_OFFSET + CHEST_GRID_CELLS * 2 == CHEST_WIRE_SIZE);

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

}  // namespace mm6
