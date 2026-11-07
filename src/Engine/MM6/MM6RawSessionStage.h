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
    bool needsRecovery = false;
};

// Read-only v2 slot selection through every currently represented Party and
// world field adapter. The supplied bases must come from a fresh MM6 session.
// This remains incomplete: BEAC/IFOV admission, final actor/PID identity,
// cursor/shop/UI publication and active-world construction are not staged.
std::optional<StagedRawSessionFields> preflightStagedRawSessionFields(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const Party &freshParty, const NPCStats &freshNpcs, const Timer &freshTimer,
    const ActiveOverlayList &freshOverlays,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, const ItemTable &itemTable,
    std::size_t overlayTableCount, GameTitle title);

} // namespace mm6
