#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "MM6NewGameSaveRuntime.h"
#include "MM6IndoorFaceOverrides.h"
#include "MM6RawSaveRuntime.h"

namespace mm6 {

struct PreparedRawNewGameAutosave {
    std::vector<std::uint8_t> bytes;
    StagedRawTitleOwners staged;
};

// Build a native EC6 v2 first-map candidate entirely on detached owners.
// The raw inventory and cursor must correspond to sourceParty; neither can be
// inferred from an item lacking MM6 raw-record provenance. Shops must be the
// fresh empty title owner. This performs the 67-map and item-table preflight,
// but is not an all-owner live save or permission to open SaveLoad.
PreparedRawNewGameAutosave prepareOfflineRawNewGameAutosave(
    const KnownSaveFields &seed, const Party &sourceParty,
    const RawInventoryFields &rawInventory, const RawHeldCursorOwner &cursor,
    const ShopTitleState &shops, const IndoorFaceOverrides &observedIndoorFaces,
    std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, const ItemTable &itemTable,
    GameTitle title);

// Test/offline disk seam: only a previously prepared candidate may be written
// to the native MM6 profile's fixed autosave.ec6 slot. No live owner is assigned.
void writeOfflineRawNewGameAutosave(const std::filesystem::path &profileRoot,
                                     const PreparedRawNewGameAutosave &prepared,
                                     GameTitle title);

} // namespace mm6
