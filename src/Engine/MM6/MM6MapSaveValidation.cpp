#include "MM6MapSaveValidation.h"

#include <optional>
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

struct ValidatedMapPair {
    SavedActiveMapCounts counts;
    std::optional<StagedActiveMap> activeMap;
};

ValidatedMapPair validateMapPair(const SavedMapDelta &delta, const MapBaseReader &readBase,
                                 const GroundItemVerifier &verifyGroundItem, bool retainActive,
                                 std::span<const std::uint8_t> rawActiveDelta) {
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
        // Both indoor decoders make owned wire copies, so the retained active
        // result cannot borrow the caller's save span or a base-reader view.
        MM6IndoorSnapshot base = deserializeMM6IndoorBase(decodedBase);
        MM6IndoorRuntimeDelta decoded = decodeMM6IndoorRuntimeDelta(decodedDelta, base);
        validateMapItems(decoded.spriteObjects, decoded.chests, verifyGroundItem);
        if (retainActive) {
            MM6IndoorRuntimeDelta raw = decodeMM6IndoorRuntimeDelta(
                Blob::view(rawActiveDelta.data(), rawActiveDelta.size()), base);
            validateMapItems(raw.spriteObjects, raw.chests, verifyGroundItem);
            if (raw.actors.size() != decoded.actors.size() ||
                raw.spriteObjects.size() != decoded.spriteObjects.size() ||
                raw.chests.size() != decoded.chests.size() || raw.doors.size() != decoded.doors.size())
                throw std::runtime_error("MM6 CHST sidecar and active indoor delta disagree on record shape");
        }
        ValidatedMapPair result;
        result.counts = {decoded.actors.size(), decoded.spriteObjects.size(), decoded.chests.size()};
        if (retainActive)
            result.activeMap.emplace(StagedIndoorMap{std::move(baseName), std::move(base), std::move(decoded)});
        return result;
    } else {
        // The outdoor wire decoders likewise copy their source bytes.
        OutdoorBaseWire base = decodeOutdoorBase(
            {static_cast<const std::uint8_t *>(decodedBase.data()), decodedBase.size()});
        MM6OutdoorDeltaSnapshot decoded = decodeMM6OutdoorDelta(decodedDelta);
        validateMapItems(decoded.spriteObjects, decoded.chests, verifyGroundItem);
        if (retainActive) {
            MM6OutdoorDeltaSnapshot raw = decodeMM6OutdoorDelta(
                Blob::view(rawActiveDelta.data(), rawActiveDelta.size()));
            validateMapItems(raw.spriteObjects, raw.chests, verifyGroundItem);
            if (raw.actors.size() != decoded.actors.size() ||
                raw.spriteObjects.size() != decoded.spriteObjects.size() ||
                raw.chests.size() != decoded.chests.size())
                throw std::runtime_error("MM6 CHST sidecar and active outdoor delta disagree on record shape");
        }
        ValidatedMapPair result;
        result.counts = {decoded.actors.size(), decoded.spriteObjects.size(), decoded.chests.size()};
        if (retainActive)
            result.activeMap.emplace(StagedOutdoorMap{std::move(baseName), std::move(base), std::move(decoded)});
        return result;
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
    std::optional<StagedActiveMap> activeMap;
    for (const SavedMapDelta &delta : candidate.maps.deltas) {
        const bool isActive = delta.name == activeDelta;
        ValidatedMapPair validated = validateMapPair(delta, readBase, verifyGroundItem, isActive,
                                                     candidate.chest.rawActiveDelta);
        if (isActive) {
            active = validated.counts;
            activeMap = std::move(validated.activeMap);
        }
    }
    if (!activeMap)
        throw std::runtime_error("MM6 active map was not in the validated delta set");
    return {std::move(candidate), active, std::move(*activeMap)};
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
