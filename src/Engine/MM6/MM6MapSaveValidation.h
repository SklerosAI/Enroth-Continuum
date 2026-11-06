#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <variant>

#include "MM6KnownSaveFields.h"
#include "MM6OutdoorWire.h"

#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

#include "Utility/Memory/Blob.h"

namespace mm6 {

// The caller supplies MM6 base entries by basename from a trusted, title-
// specific content source. No filesystem or MM7 save path is chosen here.
using MapBaseReader = std::function<Blob(std::string_view)>;
// Check nonempty ground and fixed chest-slot item IDs against loaded MM6 item
// templates. Chest random-item markers -6..-1 are not templates. The caller
// owns the table and may reject a missing icon/footprint.
using GroundItemVerifier = std::function<bool(std::int32_t)>;

struct SavedActiveMapCounts {
    std::size_t actors = 0;
    std::size_t sprites = 0;
    std::size_t chests = 0;
};

struct StagedIndoorMap {
    std::string mapName;
    MM6IndoorSnapshot base;
    MM6IndoorRuntimeDelta delta;
};

struct StagedOutdoorMap {
    std::string mapName;
    OutdoorBaseWire base;
    MM6OutdoorDeltaSnapshot delta;
};

using StagedActiveMap = std::variant<StagedIndoorMap, StagedOutdoorMap>;

struct ValidatedKnownMapSave {
    KnownSaveFields known;
    SavedActiveMapCounts active;
    // The active map's decoded, owned candidate from the same 67-map pass.
    // This is not a reconstructed live IndoorLocation or OutdoorLocation.
    StagedActiveMap activeMap;
};

// Return the projected active delta's record counts from the same validated
// 67-map pass. These are saved-record bounds, not post-load runtime identity.
ValidatedKnownMapSave decodeKnownSaveWithActiveMapCounts(
    std::span<const std::uint8_t> bytes, std::span<const std::string> expectedNames,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyGroundItem);

// Decode every known EC6 section, check all 67 expected names, and validate
// every delta against its corresponding base before returning candidate state.
// The expected names must come from the locally verified private F3 manifest.
KnownSaveFields decodeKnownSaveWithMapBases(std::span<const std::uint8_t> bytes,
                                            std::span<const std::string> expectedNames,
                                            const MapBaseReader &readBase,
                                            const GroundItemVerifier &verifyGroundItem);

// Assign only after the entire known-save and map pass succeeds. This is not
// a live SaveLoad transaction and does not enable the save menu.
void loadKnownSaveWithMapBasesBeforeMutation(std::span<const std::uint8_t> bytes,
                                             std::span<const std::string> expectedNames,
                                             const MapBaseReader &readBase,
                                             const GroundItemVerifier &verifyGroundItem,
                                             KnownSaveFields *destination);

} // namespace mm6
