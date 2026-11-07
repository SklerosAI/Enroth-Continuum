#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "MM6EventRuntimeSaveFields.h"
#include "MM6V3FullSessionPreflight.h"
#include "MM6V4SaveFields.h"

namespace mm6 {

// Detached v4 OUTDOOR candidate. The caller must supply the rebuilt active
// decorations from the same validated MM6 base/delta as readBase; this stage
// checks their count and EVRT binding, but does not independently reconstruct
// them from the base bytes. Indoor maps, live publication and save slots are
// outside this contract.
struct V4FullSessionPreflight {
    V4SaveFields saved;
    V3FullSessionPreflight v3;
    EventRuntimeMapEntry activeEventRuntime;
};

V4FullSessionPreflight stageV4FullSessionPreflight(
    std::span<const std::uint8_t> bytes,
    const NpcStaticBaseline &freshNpcs,
    std::span<const std::string> expectedNames,
    const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem,
    const ItemTable &itemTable,
    std::size_t overlayTableCount,
    std::size_t tileTableCount,
    std::size_t runtimeActorCap,
    RandomEngineType selectedRngType,
    std::span<LevelDecoration> rebuiltDecorations,
    GameTitle title);

} // namespace mm6
