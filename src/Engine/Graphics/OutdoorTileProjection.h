#pragma once

#include <array>

// The ODM tile map stores 128 cells per row. Local tile IDs 90..233 select
// one of four 36-tile ranges; lower IDs are already global. The last range
// is reserved and resolves to the default tile. Keep this mapping shared by
// the live terrain reconstruction and detached MM6 save preflight.
inline int outdoorGlobalTileId(const std::array<int, 4> &baseIds, int localTileId) {
    if (localTileId < 90)
        return localTileId;
    if (localTileId >= 234)
        return 0;
    const int tilesetIndex = (localTileId - 90) / 36;
    const int tilesetOffset = (localTileId - 90) % 36;
    return baseIds[tilesetIndex] + tilesetOffset;
}
