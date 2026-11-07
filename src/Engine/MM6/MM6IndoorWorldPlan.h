#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"

namespace mm6 {

struct StagedRawTitleOwners;

struct DetachedIndoorDoorReferences {
    std::vector<std::size_t> vertices;
    std::vector<std::size_t> faces;
    std::vector<std::size_t> sectors;
};

struct DetachedIndoorWorldPlan {
    std::string mapName;
    // Owned CPU projections only. No IndoorLocation, texture, renderer, actor,
    // chest or global process owner is constructed or assigned here.
    IndoorLocation_MM7 base;
    MM6IndoorRuntimeDelta savedDelta;
    MM6IndoorRuntimeDelta activeSidecar;
    std::vector<DetachedIndoorDoorReferences> savedDoors;
    std::vector<DetachedIndoorDoorReferences> sidecarDoors;
};

// Start from the already validated v2 active BLV/DLV and CHST candidate.
// Recheck base, delta and door topology against owned projections, including
// any in-memory changes since raw preflight, before returning a detached plan.
DetachedIndoorWorldPlan prepareDetachedRawIndoorWorldPlan(
    const StagedRawTitleOwners &raw, GameTitle title);

} // namespace mm6
