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

void validateItemId(std::int32_t itemId, const GroundItemVerifier &verifyGroundItem) {
    if (itemId == 0)
        return;
    // The retained F3 item table has IDs 0..580. A higher value must not
    // reach the inherited item-table indexing path during reconstruction.
    if (itemId < 0 || itemId > 580 || !verifyGroundItem(itemId))
        throw std::runtime_error("MM6 map save has an invalid item template");
}

void validateMapItems(const std::vector<SpriteObject_MM7> &objects,
                      const std::vector<Chest_MM7> &chests,
                      const GroundItemVerifier &verifyGroundItem) {
    for (const SpriteObject_MM7 &object : objects) {
        validateItemId(object.containing_item.itemId, verifyGroundItem);
    }
    // Only the 140 fixed 28-byte item slots have a proved MM6 layout. The
    // following 280-byte inventory grid/tail remains opaque.
    for (const Chest_MM7 &chest : chests) {
        for (const Item_MM7 &item : chest.items) {
            // Retained MM6 chests also carry the six random-item markers
            // handled by GenerateItemsInChest. They are not table templates.
            if (item.itemId >= -6 && item.itemId <= -1)
                continue;
            validateItemId(item.itemId, verifyGroundItem);
        }
    }
}

SavedActiveMapCounts validateMapPair(const SavedMapDelta &delta, const MapBaseReader &readBase,
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
        validateMapItems(decoded.spriteObjects, decoded.chests, verifyGroundItem);
        return {decoded.actors.size(), decoded.spriteObjects.size(), decoded.chests.size()};
    } else {
        (void) decodeOutdoorBase({static_cast<const std::uint8_t *>(decodedBase.data()), decodedBase.size()});
        MM6OutdoorDeltaSnapshot decoded = decodeMM6OutdoorDelta(decodedDelta);
        validateMapItems(decoded.spriteObjects, decoded.chests, verifyGroundItem);
        return {decoded.actors.size(), decoded.spriteObjects.size(), decoded.chests.size()};
    }
}

} // namespace

ValidatedKnownMapSave decodeKnownSaveWithActiveMapCounts(
    std::span<const std::uint8_t> bytes, std::span<const std::string> expectedNames,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyGroundItem) {
    if (!readBase || !verifyGroundItem)
        throw std::runtime_error("MM6 map save lacks a base or item-template reader");
    KnownSaveFields candidate = decodeKnownSave(bytes);
    validateExpectedMapNames(candidate.maps, expectedNames);
    const std::string &mapName = candidate.metadata.header.mapFileName;
    const std::string activeDelta = mapName.substr(0, mapName.size() - 4) +
                                    (mapName.ends_with(".blv") ? ".dlv" : ".ddm");
    SavedActiveMapCounts active;
    bool foundActive = false;
    for (const SavedMapDelta &delta : candidate.maps.deltas) {
        SavedActiveMapCounts counts = validateMapPair(delta, readBase, verifyGroundItem);
        if (delta.name == activeDelta) {
            active = counts;
            foundActive = true;
        }
    }
    if (!foundActive)
        throw std::runtime_error("MM6 active map was not in the validated delta set");
    return {std::move(candidate), active};
}

KnownSaveFields decodeKnownSaveWithMapBases(std::span<const std::uint8_t> bytes,
                                            std::span<const std::string> expectedNames,
                                            const MapBaseReader &readBase,
                                            const GroundItemVerifier &verifyGroundItem) {
    return decodeKnownSaveWithActiveMapCounts(bytes, expectedNames, readBase, verifyGroundItem).known;
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
