#include "MM6MapSaveValidation.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "MM6Archive.h"
#include "MM6OutdoorWire.h"

#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

namespace mm6 {
namespace {

void validateGroundItems(const std::vector<SpriteObject_MM7> &objects,
                         const GroundItemVerifier &verifyGroundItem) {
    for (const SpriteObject_MM7 &object : objects) {
        const std::int32_t itemId = object.containing_item.itemId;
        if (itemId == 0)
            continue;
        // The retained F3 item table has IDs 0..580. A higher value must not
        // reach the inherited item-table indexing path during reconstruction.
        if (itemId < 0 || itemId > 580 || !verifyGroundItem(itemId))
            throw std::runtime_error("MM6 map save has an invalid ground item template");
    }
}

void validateMapPair(const SavedMapDelta &delta, const MapBaseReader &readBase,
                     const GroundItemVerifier &verifyGroundItem) {
    const bool indoor = delta.name.ends_with(".dlv");
    const std::string baseName = delta.name.substr(0, delta.name.size() - 4) +
                                 (indoor ? ".blv" : ".odm");
    Blob baseEntry = readBase(baseName);
    if (baseEntry.empty())
        throw std::runtime_error("MM6 map save is missing a base map");
    Blob decodedBase = decodeMM6ArchiveEntry(baseEntry);
    Blob deltaEntry = Blob::view(delta.bytes.data(), delta.bytes.size());
    Blob decodedDelta = decodeMM6ArchiveEntry(deltaEntry);

    if (indoor) {
        MM6IndoorSnapshot base = deserializeMM6IndoorBase(decodedBase);
        MM6IndoorRuntimeDelta decoded = decodeMM6IndoorRuntimeDelta(decodedDelta, base);
        validateGroundItems(decoded.spriteObjects, verifyGroundItem);
    } else {
        (void) decodeOutdoorBase({static_cast<const std::uint8_t *>(decodedBase.data()), decodedBase.size()});
        MM6OutdoorDeltaSnapshot decoded = decodeMM6OutdoorDelta(decodedDelta);
        validateGroundItems(decoded.spriteObjects, verifyGroundItem);
    }
}

} // namespace

KnownSaveFields decodeKnownSaveWithMapBases(std::span<const std::uint8_t> bytes,
                                            std::span<const std::string> expectedNames,
                                            const MapBaseReader &readBase,
                                            const GroundItemVerifier &verifyGroundItem) {
    if (!readBase || !verifyGroundItem)
        throw std::runtime_error("MM6 map save lacks a base or item-template reader");
    KnownSaveFields candidate = decodeKnownSave(bytes);
    validateExpectedMapNames(candidate.maps, expectedNames);
    for (const SavedMapDelta &delta : candidate.maps.deltas)
        validateMapPair(delta, readBase, verifyGroundItem);
    return candidate;
}

void loadKnownSaveWithMapBasesBeforeMutation(std::span<const std::uint8_t> bytes,
                                             std::span<const std::string> expectedNames,
                                             const MapBaseReader &readBase,
                                             const GroundItemVerifier &verifyGroundItem,
                                             KnownSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 known save destination");
    KnownSaveFields candidate = decodeKnownSaveWithMapBases(bytes, expectedNames,
                                                            readBase, verifyGroundItem);
    *destination = std::move(candidate);
}

} // namespace mm6
