#include "MM6WorldSaveRuntime.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

} // namespace

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
    validateExpectedMapNames(candidate.maps, expectedNames);

    StagedWorldFields staged;
    staged.npcs = liveNpcs;
    staged.npcs.pNPCData = std::move(candidate.npcs.npcData);
    staged.npcs.pGroups = candidate.npcs.groups;
    staged.npcs.uNumNewNPCs = candidate.npcs.activeCount;
    for (const SavedMapDelta &delta : candidate.maps.deltas)
        staged.mapDeltas.emplace(delta.name, Blob::copy(delta.bytes.data(), delta.bytes.size()));
    return staged;
}

} // namespace mm6
