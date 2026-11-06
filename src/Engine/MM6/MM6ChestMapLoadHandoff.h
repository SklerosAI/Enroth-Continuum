#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "Engine/MM6/MM6ChestRuntimeProjection.h"

namespace mm6 {

// Explicit title-local owners for a future map-load adapter. The current
// global grng is Mersenne Twister and Party artifact flags use MM7 IDs, so
// neither can hold the original MM6 chest LCG/400..429 state implicitly.
struct ChestMapLoadOwners {
    GameTitle title = GameTitle::MM7;
    ChestRecordSet records = {};
    std::vector<Chest> chests;
    std::uint32_t rngState = 0;
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
};

struct ChestMapLoadCandidate {
    ChestRecordSet records = {};
    std::vector<Chest> chests;
    std::uint32_t nextRngState = 0;
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
    int markersVisited = 0;
    int rewardsGenerated = 0;
};

static_assert(noexcept(std::declval<ChestRecordSet &>().swap(std::declval<ChestRecordSet &>())));
static_assert(noexcept(std::declval<std::vector<Chest> &>().swap(std::declval<std::vector<Chest> &>())));

// Complete all generation and twenty Item/ChestInventory projections before
// returning a candidate. No owner is changed if a late chest fails.
inline std::optional<ChestMapLoadCandidate> stageChestMapLoadHandoff(
    const ChestMapLoadOwners &owners, MapTreasureLevel mapLevel, const ItemTable &table,
    const std::array<std::uint8_t, 30> &partyOwnedArtifactFlags) {
    if (owners.title != GameTitle::MM6 || pItemTable != &table)
        return std::nullopt;
    for (int index = 0; index < 30; ++index)
        if (owners.generatedArtifactFlags[index] > 1 || partyOwnedArtifactFlags[index] > 1)
            return std::nullopt;
    ChestRecordGenerationInput input;
    input.title = owners.title;
    input.mapTreasureLevel = mapLevel;
    input.rngState = owners.rngState;
    input.records = owners.records;
    input.partyOwnedArtifactFlags = partyOwnedArtifactFlags;
    input.generatedArtifactFlags = owners.generatedArtifactFlags;
    const auto generated = stageChestRecordGeneration(input, table);
    if (!generated)
        return std::nullopt;

    ChestMapLoadCandidate candidate;
    candidate.records = generated->records;
    candidate.nextRngState = generated->nextRngState;
    candidate.generatedArtifactFlags = generated->generatedArtifactFlags;
    candidate.markersVisited = generated->markersVisited;
    candidate.rewardsGenerated = generated->rewardsGenerated;
    candidate.chests.reserve(20);
    for (int index = 0; index < 20; ++index) {
        auto projected = stageChestRuntimeProjection(owners.title, candidate.records[index], index);
        if (!projected)
            return std::nullopt;
        candidate.chests.push_back(std::move(projected->chest));
    }
    return candidate;
}

// The only mutation is after the complete candidate exists. Vector/array
// swaps and scalar copies do not throw for these types, so a failed stage
// leaves records, live chests, LCG and artifact flags untouched together.
inline bool applyChestMapLoadHandoff(
    ChestMapLoadOwners *owners, MapTreasureLevel mapLevel, const ItemTable &table,
    const std::array<std::uint8_t, 30> &partyOwnedArtifactFlags) {
    if (!owners)
        return false;
    auto staged = stageChestMapLoadHandoff(*owners, mapLevel, table, partyOwnedArtifactFlags);
    if (!staged)
        return false;
    owners->records.swap(staged->records);
    owners->chests.swap(staged->chests);
    owners->rngState = staged->nextRngState;
    owners->generatedArtifactFlags = staged->generatedArtifactFlags;
    return true;
}

}  // namespace mm6
