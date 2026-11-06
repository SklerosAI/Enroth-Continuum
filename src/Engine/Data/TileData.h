#pragma once

#include <cstdint>
#include <string>

#include "TileEnums.h"

struct TileData {
    std::string textureName;
    Tileset tileset = TILESET_INVALID;
    TileVariant variant = TILE_VARIANT_BASE1;
    TileFlags flags;
    // Keep the file's identity when the normalized MM7 enums merge terrain types.
    uint16_t sourceTileset = 255;
    uint16_t sourceVariant = 255;
};
