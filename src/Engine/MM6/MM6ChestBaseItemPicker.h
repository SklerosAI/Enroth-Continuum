#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/Objects/ItemEnums.h"

namespace mm6 {

inline std::uint32_t nextChestRandom15(std::uint32_t &state) {
    state = state * 214013u + 2531011u;
    return (state >> 16) & 0x7fffu;
}

// Detached base-ID stage of the original MM6 ANY-item picker. It does not
// construct an Item, write live artifact state, or bypass the chest-load guard.
struct ChestBaseItemPick {
    int itemId = 0;
    std::uint32_t nextRngState = 0;
    int drawsConsumed = 0;
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
};

inline std::optional<ChestBaseItemPick> pickChestBaseItemId(
    GameTitle title, ItemTreasureLevel treasureLevel,
    const std::array<int, 399> &regularWeights,
    const std::array<std::uint8_t, 30> &generatedArtifactFlags,
    std::uint32_t rngState) {
    if (title != GameTitle::MM6)
        return std::nullopt;
    const int tier = std::to_underlying(treasureLevel);
    if (tier < 1 || tier > 6)
        return std::nullopt;

    std::int64_t weightSum = 0;
    for (int weight : regularWeights) {
        if (weight < 0)
            return std::nullopt;
        weightSum += weight;
    }
    if (weightSum <= 0 || weightSum > std::numeric_limits<std::int32_t>::max())
        return std::nullopt;

    int artifactsGenerated = 0;
    for (std::uint8_t flag : generatedArtifactFlags) {
        if (flag > 1)
            return std::nullopt;
        artifactsGenerated += flag;
    }

    ChestBaseItemPick result;
    result.generatedArtifactFlags = generatedArtifactFlags;
    auto draw = [&result, &rngState]() {
        ++result.drawsConsumed;
        return nextChestRandom15(rngState);
    };

    // The original ANY path draws this candidate at every treasure tier.
    const int artifactIndex = draw() % 30;
    if (tier == 6 && draw() % 100 < 5 &&
        !generatedArtifactFlags[artifactIndex] && artifactsGenerated < 13) {
        result.itemId = 400 + artifactIndex;
        result.generatedArtifactFlags[artifactIndex] = 1;
        result.nextRngState = rngState;
        return result;
    }

    const int roll = draw() % static_cast<int>(weightSum);
    result.nextRngState = rngState;
    if (roll == 0) {
        result.itemId = 1;  // Original zero-remainder branch, even if item 1 has zero weight.
        return result;
    }

    int cumulative = 0;
    for (int id = 1; id <= 399; ++id) {
        cumulative += regularWeights[id - 1];
        if (cumulative >= roll) {
            result.itemId = id;
            return result;
        }
    }
    return std::nullopt;
}

}  // namespace mm6
