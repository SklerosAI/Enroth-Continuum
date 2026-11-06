#pragma once

#include <cstddef>
#include <string_view>

#include "Engine/Data/MapData.h"
#include "Engine/GameTitle.h"
#include "Engine/MapEnums.h"

#include "Utility/IndexedArray.h"

class Blob;
class LodReader;

struct MapTable {
    void Initialize(const Blob &mapStats, GameTitle gameTitle = GameTitle::MM7);
    MapId GetMapInfo(std::string_view fileName);
    /** Resolve the one-based Games LOD directory index stored in MM6 transport records. */
    MapId GetMM6TransportMapInfo(const LodReader &gamesLod, size_t oneBasedDirectoryIndex) const;
    IndexedArray<MapData, MAP_FIRST, MAP_LAST> pInfos;
};

extern MapTable *pMapTable;
