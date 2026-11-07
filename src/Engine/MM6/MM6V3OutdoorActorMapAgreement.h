#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "MM6MapSaveValidation.h"
#include "MM6V3SaveFields.h"

#include "Engine/Objects/Actor.h"

namespace mm6 {

// Detached, owned result of the exact v3 manifest and 67-map validation pass.
// No OutdoorLocation, process actor list, or save slot is mutated.
struct V3OutdoorActorMapAgreement {
    V3SaveFields saved;
    StagedOutdoorMap activeMap;
    MM6OutdoorDeltaSnapshot activeSidecar;
    std::vector<Actor> activeActors;
};

// Bind ACTR's complete PID-ordered list to the selected native MM6 DDM and
// CHST sidecar. Native DDM actors supply only stable identity anchors; mutable
// gameplay fields are owned by ACTR. The cap comes from the caller's runtime.
V3OutdoorActorMapAgreement stageV3OutdoorActorMapAgreement(
    std::span<const std::uint8_t> bytes,
    std::span<const std::string> expectedNames,
    const MapBaseReader &readBase,
    const GroundItemVerifier &verifyGroundItem,
    std::size_t runtimeActorCap,
    GameTitle title);

} // namespace mm6
