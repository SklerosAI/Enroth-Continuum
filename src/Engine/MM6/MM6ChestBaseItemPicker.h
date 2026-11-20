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

/**
 * The base item stage of MM6.exe's any-item picker, with its tier-6 artifact roll.
 *
 * @param title                         Must be `GameTitle::MM6`.
 * @param treasureLevel                 Treasure level 1..6.
 * @param regularWeights                Weights of items 1..399 at that treasure level.
 * @param generatedArtifactFlags        Generated flags of artifacts 400..429.
 * @param rngState                      MM6 gameplay LCG state.
 * @param collectedArtifactFlags        Collected flags of artifacts 400..429 under the All Artifacts mod. The roll
 *                                      then skips these instead of the generated ones, with no cap of 13. `nullptr`
 *                                      keeps MM6.exe's rule.
 * @return                              Picked id with the next LCG state and generated flags, or `std::nullopt` for
 *                                      weights or flags the original would misread.
 */
inline std::optional<ChestBaseItemPick> pickChestBaseItemId(
    GameTitle title, ItemTreasureLevel treasureLevel,
    const std::array<int, 399> &regularWeights,
    const std::array<std::uint8_t, 30> &generatedArtifactFlags,
    std::uint32_t rngState, const std::array<std::uint8_t, 30> *collectedArtifactFlags = nullptr) {
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
    if (collectedArtifactFlags)
        for (std::uint8_t flag : *collectedArtifactFlags)
            if (flag > 1)
                return std::nullopt;

    ChestBaseItemPick result;
    result.generatedArtifactFlags = generatedArtifactFlags;
    auto draw = [&result, &rngState]() {
        ++result.drawsConsumed;
        return nextChestRandom15(rngState);
    };

    // The original ANY path draws this candidate at every treasure tier.
    const int artifactIndex = draw() % 30;
    const bool artifactAvailable = collectedArtifactFlags ? !(*collectedArtifactFlags)[artifactIndex]
                                                          : !generatedArtifactFlags[artifactIndex] && artifactsGenerated < 13;
    if (tier == 6 && draw() % 100 < 5 && artifactAvailable) {
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
