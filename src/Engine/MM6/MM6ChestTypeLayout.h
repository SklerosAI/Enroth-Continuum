#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "Engine/GameTitle.h"

namespace mm6 {

struct ChestTypeLayout {
    int width = 0;
    int height = 0;
    int offsetX = 0;
    int offsetY = 0;
    int iconId = 0;
    std::string iconName;
};

// The original MM6 placement and chest UI use these hardcoded 9x9 dimensions
// and pixel offsets, while dchest.bin carries separate unused 14x10 metadata.
// Decode the table only for its one-based chest icon ID. This detached check
// never changes a live chest, item grid, asset manager or RNG state.
inline std::optional<ChestTypeLayout> chestTypeLayout(GameTitle title, int type,
                                                       std::span<const std::uint8_t> dchest) {
    constexpr int count = 8;
    constexpr int rowSize = 36;
    constexpr int headerSize = 4;
    constexpr std::array<int, count> offsetsX = {42, 18, 18, 42, 42, 42, 18, 42};
    constexpr std::array<int, count> offsetsY = {34, 30, 30, 34, 34, 34, 30, 34};
    if (title != GameTitle::MM6 || type < 0 || type >= count ||
        dchest.size() != headerSize + count * rowSize || dchest[0] != count ||
        dchest[1] != 0 || dchest[2] != 0 || dchest[3] != 0)
        return std::nullopt;

    // Validate the entire icon mapping before returning a candidate; the
    // retained eight rows are sequential and every named icon is staged.
    for (int index = 0; index < count; ++index) {
        const int offset = headerSize + index * rowSize + 34;
        const int iconId = dchest[offset] | (dchest[offset + 1] << 8);
        if (iconId != index + 1)
            return std::nullopt;
    }
    const int iconId = type + 1;
    return ChestTypeLayout{9, 9, offsetsX[type], offsetsY[type], iconId,
                           "chest0" + std::to_string(iconId)};
}

}  // namespace mm6
