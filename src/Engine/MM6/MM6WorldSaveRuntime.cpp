#include "MM6WorldSaveRuntime.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Engine.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

namespace mm6 {
namespace {

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 world save fields cannot apply to another title");
}

std::string deltaCompanion(std::string_view mapName) {
    if (!mapName.ends_with(".blv") && !mapName.ends_with(".odm"))
        throw std::runtime_error("MM6 active map has no delta companion");
    return std::string(mapName.substr(0, mapName.size() - 4)) +
        (mapName.ends_with(".blv") ? ".dlv" : ".ddm");
}

std::vector<std::uint8_t> copiedBytes(const Blob &blob) {
    if (blob.empty())
        return {};
    const auto *start = static_cast<const std::uint8_t *>(blob.data());
    return {start, start + blob.size()};
}

StagedWorldFields stageWorldFromKnown(const NPCStats &liveNpcs,
                                     const KnownSaveFields &candidate,
                                     std::span<const std::string> expectedNames) {
    validateExpectedMapNames(candidate.maps, expectedNames);
    StagedWorldFields staged;
    staged.npcs = liveNpcs;
    staged.npcs.pNPCData = candidate.npcs.npcData;
    staged.npcs.pGroups = candidate.npcs.groups;
    staged.npcs.uNumNewNPCs = candidate.npcs.activeCount;
    for (const SavedMapDelta &delta : candidate.maps.deltas)
        staged.mapDeltas.emplace(delta.name, Blob::copy(delta.bytes.data(), delta.bytes.size()));
    return staged;
}

} // namespace

StagedKnownWorldAndVisuals stageKnownWorldAndVisuals(
    const NPCStats &liveNpcs, const Timer &liveTimer, const ActiveOverlayList &liveOverlays,
    std::span<const std::uint8_t> bytes, std::span<const std::string> expectedNames,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem,
    std::size_t overlayTableCount, GameTitle title) {
    requireMM6(title);
    KnownSaveFields known = decodeKnownSaveWithMapBases(bytes, expectedNames, readBase, verifyItem);
    StagedWorldFields world = stageWorldFromKnown(liveNpcs, known, expectedNames);
    StagedSessionVisuals visuals = stageSessionVisuals(
        liveTimer, liveOverlays, {known.metadata.timer, known.metadata.overlays},
        overlayTableCount, title);
    return {std::move(known), std::move(world), std::move(visuals)};
}

void validateOutdoorMapIdentity(MapId currentMapId, const MapTable &mapTable,
                                std::string_view savedMapName,
                                std::span<const std::string> expectedNames, GameTitle title) {
    requireMM6(title);
    if (currentMapId < MAP_FIRST || currentMapId > MAP_LAST ||
        !savedMapName.ends_with(".odm") || expectedNames.size() != 67)
        throw std::runtime_error("MM6 outdoor capture has no bounded current map identity");

    const std::string &currentName = mapTable.pInfos[currentMapId].fileName;
    if (currentName != savedMapName ||
        std::count(expectedNames.begin(), expectedNames.end(), deltaCompanion(savedMapName)) != 1)
        throw std::runtime_error("MM6 outdoor map disagrees with HEAD or the 67-map manifest");
    for (MapId id : mapTable.pInfos.indices())
        if (id != currentMapId && mapTable.pInfos[id].fileName == currentName)
            throw std::runtime_error("ambiguous MM6 outdoor map-table filename");
}

KnownSaveFields captureKnownWorldFields(const KnownSaveFields &seed, const NPCStats &npcs,
                                        const std::unordered_map<std::string, Blob> &priorDeltas,
                                        std::string_view currentDeltaName,
                                        std::span<const std::uint8_t> currentDelta,
                                        std::span<const std::string> expectedNames, GameTitle title) {
    requireMM6(title);
    if (currentDeltaName != deltaCompanion(seed.metadata.header.mapFileName) ||
        currentDelta.empty() || priorDeltas.size() != expectedNames.size())
        throw std::runtime_error("MM6 world capture lacks a complete current-map delta");

    KnownSaveFields candidate = seed;
    candidate.npcs = captureNpcFields(npcs, title);
    candidate.maps.deltas.clear();
    candidate.maps.deltas.reserve(expectedNames.size());
    for (const std::string &name : expectedNames) {
        auto prior = priorDeltas.find(name);
        if (prior == priorDeltas.end())
            throw std::runtime_error("MM6 world capture is missing a saved map delta");
        candidate.maps.deltas.push_back({name, name == currentDeltaName
            ? std::vector<std::uint8_t>(currentDelta.begin(), currentDelta.end())
            : copiedBytes(prior->second)});
    }
    validateExpectedMapNames(candidate.maps, expectedNames);
    (void) encodeKnownSave(candidate);
    return candidate;
}

StagedWorldFields stageKnownWorldFields(const NPCStats &liveNpcs,
                                       std::span<const std::uint8_t> bytes,
                                       std::span<const std::string> expectedNames, GameTitle title) {
    requireMM6(title);
    KnownSaveFields candidate = decodeKnownSave(bytes);
    return stageWorldFromKnown(liveNpcs, candidate, expectedNames);
}

KnownSaveFields captureKnownWorldFromActiveMap(
    const KnownSaveFields &seed, const NPCStats &npcs,
    const std::unordered_map<std::string, Blob> &priorDeltas,
    std::span<const std::string> expectedNames, GameTitle title) {
    requireMM6(title);
    const std::string &mapName = seed.metadata.header.mapFileName;
    Blob changed;
    if (uCurrentlyLoadedLevelType == LEVEL_INDOOR && mapName.ends_with(".blv") &&
        pIndoor && pIndoor->filename == mapName) {
        changed = encodeMM6IndoorChangedState(*pIndoor);
    } else if (uCurrentlyLoadedLevelType == LEVEL_OUTDOOR && mapName.ends_with(".odm") && pOutdoor) {
        if (!engine || engine->gameTitle() != GameTitle::MM6 || !pMapTable)
            throw std::runtime_error("MM6 outdoor capture requires its loaded engine map table");
        validateOutdoorMapIdentity(engine->_currentLoadedMapId, *pMapTable, mapName, expectedNames, title);
        changed = encodeMM6OutdoorChangedState(*pOutdoor);
    } else {
        throw std::runtime_error("MM6 active level and save map do not match");
    }
    if (changed.empty())
        throw std::runtime_error("MM6 changed-state writer returned an empty delta");
    return captureKnownWorldFields(seed, npcs, priorDeltas, deltaCompanion(mapName),
                                   {static_cast<const std::uint8_t *>(changed.data()), changed.size()},
                                   expectedNames, title);
}

} // namespace mm6
