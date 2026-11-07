#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "MM6RawSaveRuntime.h"
#include "MM6WorldSaveRuntime.h"

namespace mm6 {

struct StagedRawSessionFields {
    StagedRawTitleOwners raw;
    StagedRawPartyInventories party;
    StagedRawWorldAndVisuals world;
    // Populated for outdoor slots from the validated MM6 base/delta pair.
    // Indoor world construction and all live resource publication remain open.
    std::optional<DetachedOutdoorWorldPlan> outdoorPlan;
    bool needsRecovery = false;
};

// Read-only v2 slot selection through every currently represented Party and
// world field adapter, and checks raw HELD/SHOP/SCLK owner equality. The
// supplied bases must come from a fresh MM6 session.
// This remains incomplete: BEAC/IFOV admission, final actor/PID identity,
// cursor/shop/UI publication and active-world construction are not staged.
std::optional<StagedRawSessionFields> preflightStagedRawSessionFields(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const Party &freshParty, const NPCStats &freshNpcs, const Timer &freshTimer,
    const ActiveOverlayList &freshOverlays,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, const ItemTable &itemTable,
    std::size_t overlayTableCount, std::size_t tileTableCount, GameTitle title);

} // namespace mm6
