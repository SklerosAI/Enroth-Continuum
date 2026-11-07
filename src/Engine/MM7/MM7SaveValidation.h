#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "Engine/GameTitle.h"

#include "Utility/Memory/Blob.h"

class LodReader;
struct Character_MM7;
struct Chest_MM7;
struct IndoorDelta_MM7;
struct OutdoorDelta_MM7;
struct Party_MM7;

namespace mm7 {

// Detached structural preflight for the MM7 native save format. The supplied
// Games LOD must be the already selected MM7 game-data archive. This checks
// file framing, required image payloads and map-delta decoding, but not every
// semantic reference used by later live reconstruction. It deliberately
// rejects a missing or corrupt thumbnail even though the current menu can
// display a save with one; legacy-save compatibility needs a separate check.
// It cannot alone be
// used as the atomic writer's complete SaveBytesValidator callback.
bool preflightNativeSaveStructure(std::span<const std::uint8_t> bytes,
                                  GameTitle title, const LodReader &gamesLod);

namespace detail {

constexpr std::size_t maxDecodedSaveEntryBytes = 64 * 1024 * 1024;

// Reject a large declared decompressed size before the LOD decoder allocates.
Blob decodeBoundedSaveEntry(const Blob &source);

// Check fixed and counted MM7 delta records before the regular deserializer
// resizes vectors. Counts are derived from a decoded, trusted base map.
bool hasBoundedDeltaRecords(const Blob &decoded, bool indoor,
                            std::size_t faceCount, std::size_t decorationCount,
                            std::size_t doorCount = 0, std::size_t doorDataBytes = 0);

// Check selected references in inventories, the active chest grid, sprites
// and indoor doors. Other semantic owners remain outside this preflight.
// The current chest reconstruction still stashes stale raw item IDs outside
// the active grid; this detached preflight is not a production validator.
bool hasSafeCharacterInventoryReferences(const Character_MM7 &character);
bool hasSafeChestReferences(const Chest_MM7 &chest);
bool hasSafePartyInventoryReferences(const Party_MM7 &party);
bool hasSafeIndoorDeltaReferences(const IndoorDelta_MM7 &delta,
                                   std::size_t vertexCount, std::size_t faceCount,
                                   std::size_t sectorCount);
bool hasSafeOutdoorDeltaReferences(const OutdoorDelta_MM7 &delta);

} // namespace detail
} // namespace mm7
