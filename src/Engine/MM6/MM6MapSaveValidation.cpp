#include "MM6MapSaveValidation.h"

#include <stdexcept>
#include <string>
#include <utility>

#include "MM6Archive.h"
#include "MM6OutdoorWire.h"

#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

namespace mm6 {
namespace {

void validateMapPair(const SavedMapDelta &delta, const MapBaseReader &readBase) {
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
        (void) decodeMM6IndoorRuntimeDelta(decodedDelta, base);
    } else {
        (void) decodeOutdoorBase({static_cast<const std::uint8_t *>(decodedBase.data()), decodedBase.size()});
        (void) decodeMM6OutdoorDelta(decodedDelta);
    }
}

} // namespace

KnownSaveFields decodeKnownSaveWithMapBases(std::span<const std::uint8_t> bytes,
                                            std::span<const std::string> expectedNames,
                                            const MapBaseReader &readBase) {
    if (!readBase)
        throw std::runtime_error("MM6 map save has no base-map reader");
    KnownSaveFields candidate = decodeKnownSave(bytes);
    validateExpectedMapNames(candidate.maps, expectedNames);
    for (const SavedMapDelta &delta : candidate.maps.deltas)
        validateMapPair(delta, readBase);
    return candidate;
}

void loadKnownSaveWithMapBasesBeforeMutation(std::span<const std::uint8_t> bytes,
                                             std::span<const std::string> expectedNames,
                                             const MapBaseReader &readBase,
                                             KnownSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 known save destination");
    KnownSaveFields candidate = decodeKnownSaveWithMapBases(bytes, expectedNames, readBase);
    *destination = std::move(candidate);
}

} // namespace mm6
