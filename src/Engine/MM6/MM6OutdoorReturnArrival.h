#pragma once

#include <cmath>
#include <memory>
#include <optional>
#include <string_view>

#include "Engine/Graphics/OutdoorTerrain.h"
#include "Engine/MM6/MM6OutdoorBorderEligibility.h"
#include "Engine/MM6/MM6OutdoorBorderTravel.h"
#include "Engine/Party.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Snapshots/CompositeSnapshots.h"

namespace mm6 {

enum class OutdoorReturnArrivalRefusal {
    None,
    WrongTitle,
    PartyState,
    InvalidRoute,
    WrongTarget,
    CornerUnverified,
    InvalidTerrain,
    ModelFloorUnverified,
};

struct OutdoorReturnArrival {
    std::unique_ptr<Party> detachedParty;
    std::optional<PartyPlacement> placement;
    OutdoorReturnArrivalRefusal refusal = OutdoorReturnArrivalRefusal::InvalidRoute;

    // An arrival copy does not publish a visited map or authorize travel.
    [[nodiscard]] bool canPublish() const { return false; }
    [[nodiscard]] bool canWriteCheckpoint() const { return false; }
};

// Match OutdoorTerrain::heightByPos on a decoded, validated ODM height map.
// The caller must have joined this base to the exact visited owner first.
[[nodiscard]] inline std::optional<float> outdoorReturnTerrainHeight(
    const OutdoorLocation_MM7 &base, float x, float y) {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        std::abs(x) > MM6_OUTDOOR_BORDER_EDGE ||
        std::abs(y) > MM6_OUTDOOR_BORDER_EDGE)
        return std::nullopt;
    const Pointi grid = worldToGrid(Vec3f(x, y, 0));
    if (grid.x < 0 || grid.x >= 127 || grid.y < 0 || grid.y >= 127)
        return std::nullopt;
    const Vec2i v0 = gridToWorld(grid);
    const Vec2i v1 = gridToWorld(grid + Pointi(1, 1));
    const auto height = [&](int gx, int gy) {
        return 32 * static_cast<int>(base.heightMap[gy * 128 + gx]);
    };
    const int z00 = height(grid.x, grid.y);
    const int z01 = height(grid.x, grid.y + 1);
    const int z10 = height(grid.x + 1, grid.y);
    const int z11 = height(grid.x + 1, grid.y + 1);
    if (z00 == z10 && z10 == z11 && z11 == z01)
        return static_cast<float>(z00);
    if (std::abs(v0.y - y) >= std::abs(x - v0.x))
        return z01 + (y - v1.y) * (z00 - z01) / 512.0f +
               (x - v0.x) * (z11 - z01) / 512.0f;
    return z10 + (v0.y - y) * (z11 - z10) / 512.0f +
           (v1.x - x) * (z00 - z10) / 512.0f;
}

// Test-only detached staging for an already source-validated target base.
// Refuse model XY coverage because ODM_GetFloorLevel may select a model face
// above the terrain there. The live Party is never modified, including errors.
[[nodiscard]] inline OutdoorReturnArrival stageDetachedOutdoorReturnArrival(
    GameTitle title, std::string_view activeMap, std::string_view targetMap,
    const OutdoorLocation_MM7 &validatedTargetBase, const Party &party) {
    if (title != GameTitle::MM6)
        return {.refusal = OutdoorReturnArrivalRefusal::WrongTitle};
    if (!borderOfferPartyFlagsPermit(title, party.uFlags) ||
        !borderConfirmFlyingStatePermits(title, party.bFlying) ||
        !std::isfinite(party.pos.z))
        return {.refusal = OutdoorReturnArrivalRefusal::PartyState};
    const BorderTravelAssessment route = assessOutdoorBorderTravel(
        title, activeMap, party.pos.x, party.pos.y, {});
    if (!route.candidate)
        return {.refusal = OutdoorReturnArrivalRefusal::InvalidRoute};
    if (route.candidate->destinationMap != targetMap)
        return {.refusal = OutdoorReturnArrivalRefusal::WrongTarget};
    if (route.candidate->crossedX && route.candidate->crossedY)
        return {.refusal = OutdoorReturnArrivalRefusal::CornerUnverified};
    const float x = route.candidate->arrivalX;
    const float y = route.candidate->arrivalY;
    const std::optional<float> z = outdoorReturnTerrainHeight(validatedTargetBase, x, y);
    if (!z || !std::isfinite(*z))
        return {.refusal = OutdoorReturnArrivalRefusal::InvalidTerrain};
    for (const BSPModelData_MM7 &model : validatedTargetBase.models) {
        if (model.minX > model.maxX || model.minY > model.maxY)
            return {.refusal = OutdoorReturnArrivalRefusal::ModelFloorUnverified};
        if (x + party.radius >= model.minX && x - party.radius <= model.maxX &&
            y + party.radius >= model.minY && y - party.radius <= model.maxY)
            return {.refusal = OutdoorReturnArrivalRefusal::ModelFloorUnverified};
    }
    auto staged = std::make_unique<Party>(party);
    staged->pos = Vec3f(x, y, *z);
    staged->velocity = Vec3f(0, 0, 0);
    staged->uFallStartZ = *z;
    const PartyPlacement placement(staged->pos, -1, party._viewPitch, 0);
    return {std::move(staged), placement,
            OutdoorReturnArrivalRefusal::None};
}

} // namespace mm6
