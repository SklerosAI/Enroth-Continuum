#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "MM6PreEventGeneratedNpcReplay.h"
#include "MM6RawCursorImagePlan.h"
#include "MM6RawSessionStage.h"
#include "MM6RecoverySaveRuntime.h"
#include "MM6V3OutdoorActorMapAgreement.h"

namespace mm6 {

// Owned, detached v3 candidate. This is a semantic preflight, not a live
// publisher or a primary/backup selector. No profile file is read or repaired.
struct V3FullSessionPreflight {
    V3OutdoorActorMapAgreement actorMap;
    StagedRawSessionFields represented;
    PreEventGeneratedNpcReplay preEvent;
    RawCursorImagePlan cursorImage;
    PostWorldArmageddonResumePlan armageddon;
};

V3FullSessionPreflight stageV3FullSessionPreflight(
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
    GameTitle title);

} // namespace mm6
