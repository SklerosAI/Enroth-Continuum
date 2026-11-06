#include "MM6NewGameSaveRuntime.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "MM6AtomicSaveFile.h"
#include "MM6Archive.h"

namespace mm6 {

std::vector<std::uint8_t> encodeOfflineNewGameAutosave(
    const KnownSaveFields &seed, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 new-game autosave cannot encode another title");
    if (seed.metadata.header.mapFileName != NEW_GAME_FIRST_MAP ||
        expectedNames.size() != 67 || !readDefaultDelta || !readBase || !verifyItem)
        throw std::runtime_error("MM6 new-game autosave lacks first-map or verified archive inputs");
    if (seed.core.party.position[2] != 340 || seed.core.party.fallStartZ != 340)
        throw std::runtime_error("MM6 new-game autosave lacks its first-map fall start");

    KnownSaveFields candidate = seed;
    candidate.maps.deltas.clear();
    candidate.maps.deltas.reserve(expectedNames.size());
    for (const std::string &name : expectedNames) {
        Blob entry = readDefaultDelta(name);
        if (entry.empty())
            throw std::runtime_error("MM6 new-game archive has an empty default map delta");
        const auto *start = static_cast<const std::uint8_t *>(entry.data());
        candidate.maps.deltas.push_back({name, std::vector<std::uint8_t>(start, start + entry.size())});
    }

    // The first-map sidecar is the decoded default DDM. The caller supplies
    // the title-local RNG/ledger seed; no inherited MM7 state is consulted.
    if (candidate.chest.activeDeltaName != "oute3.ddm")
        throw std::runtime_error("MM6 new-game autosave lacks its CHST first-map identity");
    const auto active = std::find_if(candidate.maps.deltas.begin(), candidate.maps.deltas.end(),
        [](const SavedMapDelta &delta) { return delta.name == "oute3.ddm"; });
    if (active == candidate.maps.deltas.end())
        throw std::runtime_error("MM6 new-game autosave lacks its first-map delta");
    Blob decoded = decodeMM6ArchiveEntry(Blob::view(active->bytes.data(), active->bytes.size()));
    const auto *start = static_cast<const std::uint8_t *>(decoded.data());
    candidate.chest.rawActiveDelta.assign(start, start + decoded.size());

    std::vector<std::uint8_t> encoded = encodeKnownSave(candidate);
    (void) decodeKnownSaveWithActiveMapCounts(encoded, expectedNames, readBase, verifyItem);
    return encoded;
}

std::vector<std::uint8_t> writeOfflineNewGameAutosave(
    const std::filesystem::path &profileRoot, const KnownSaveFields &seed,
    std::span<const std::string> expectedNames, const DefaultDeltaReader &readDefaultDelta,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem, GameTitle title) {
    std::vector<std::uint8_t> encoded = encodeOfflineNewGameAutosave(
        seed, expectedNames, readDefaultDelta, readBase, verifyItem, title);
    writeAtomicSaveFile(profileRoot, NEW_GAME_AUTOSAVE_SLOT, encoded);
    return encoded;
}

} // namespace mm6
