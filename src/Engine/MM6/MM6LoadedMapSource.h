#pragma once

#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Engine/GameTitle.h"
#include "Utility/Memory/Blob.h"

class LodReader;
struct MapTable;

namespace mm6 {

// Builds the complete MM6 map-pair set from the currently loaded Games LOD
// and map table. This is suitable for a local engine using the owner's game
// data; it does not depend on the F3 research reports used by offline checks.
class LoadedMapSource {
 public:
    LoadedMapSource(const LodReader &games, const MapTable &maps, GameTitle title);

    std::span<const std::string> expectedNames() const { return _deltaNames; }
    Blob readBase(std::string_view name) const;
    Blob readDefaultDelta(std::string_view name) const;

 private:
    const LodReader *_games;
    std::vector<std::string> _deltaNames;
    std::unordered_set<std::string> _baseNames;
    std::unordered_set<std::string> _deltaNameSet;
};

} // namespace mm6
