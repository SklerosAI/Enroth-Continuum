#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

#include "MM6MapSaveValidation.h"
#include "MM6RawHeldCursorOwner.h"
#include "MM6RawPartyInventoryStage.h"

namespace mm6 {

// Owned, detached v2 candidate. The inventory stays in physical MM6 form;
// projection into CharacterInventory and publication to live globals are later
// transactions. The cursor and shop are fresh title-local process owners.
struct StagedRawTitleOwners {
    RawSaveFields saved;
    RawInventoryFields inventory;
    RawHeldCursorOwner cursor;
    ShopTitleState shops;
    StagedActiveMap activeMap;
};

// Decode the complete v2 manifest, validate all 67 deltas against their MM6
// bases, then build fresh raw owners. Referenced item IDs must pass the caller's
// MM6 template verifier. No globals, profile files or live owners are touched.
StagedRawTitleOwners stageRawTitleOwners(
    std::span<const std::uint8_t> bytes, std::span<const std::string> expectedNames,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem,
    GameTitle title);

// Recompose a v2 candidate from detached owners after edits, including a
// fresh 67-map/base and item-template pass. This does not write a file.
RawSaveFields captureRawTitleOwners(const StagedRawTitleOwners &staged,
                                    std::span<const std::string> expectedNames,
                                    const MapBaseReader &readBase,
                                    const GroundItemVerifier &verifyItem,
                                    GameTitle title);

struct StagedRawSaveSlot {
    StagedRawTitleOwners owners;
    bool needsRecovery = false;
};

// Select a native v2 primary or backup and stage its map, item, cursor and shop
// owners before any disk repair or live mutation. The selected candidate is
// still incomplete as a playable session; callers must validate and stage all
// remaining owners before deciding whether to restore a backup.
std::optional<StagedRawSaveSlot> preflightStagedRawSaveSlot(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, GameTitle title);

// Join decoded core/progress fields and source-backed physical inventory on
// one detached Party. Other v2 Party fields and all live owners remain outside
// this bounded candidate. The saved/raw owner images must agree.
StagedRawPartyInventories stageRawPartyCoreProgressInventory(
    const Party &baseline, const StagedRawTitleOwners &owners,
    const ItemTable &itemTable, GameTitle title);

} // namespace mm6
