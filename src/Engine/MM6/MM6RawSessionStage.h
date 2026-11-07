#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "MM6RawSaveRuntime.h"
#include "MM6IndoorWorldPlan.h"
#include "MM6PersistentEventBoundary.h"
#include "MM6WorldSaveRuntime.h"

namespace mm6 {

struct StagedRawSessionFields {
    StagedRawTitleOwners raw;
    StagedRawPartyInventories party;
    StagedRawWorldAndVisuals world;
    DetachedPersistentEventBoundary persistentEvents;
    // One plan is populated from the validated MM6 base/delta pair. These
    // are CPU-only projections, not live world or resource publication.
    std::optional<DetachedIndoorWorldPlan> indoorPlan;
    std::optional<DetachedOutdoorWorldPlan> outdoorPlan;
    bool needsRecovery = false;
};

// Read-only v2 slot selection through every currently represented Party and
// world field adapter, raw HELD/SHOP/SCLK owner equality, a bounded map-event
// projection and the fail-closed saved actor-overlay policy. The
// NPC baseline is a freshly parsed MM6 table image. The Party baseline is
// constructed locally without resetting the live mouse cursor; timer and
// overlay bases are also constructed locally.
// This remains incomplete: live UI/world publication, final actor/PID
// identity and full BEAC/IFOV support are not staged.
std::optional<StagedRawSessionFields> preflightStagedRawSessionFields(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const NpcStaticBaseline &freshNpcs,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, const ItemTable &itemTable,
    std::size_t overlayTableCount, std::size_t tileTableCount, GameTitle title);

} // namespace mm6
