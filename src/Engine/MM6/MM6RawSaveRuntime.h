#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "MM6MapSaveValidation.h"
#include "MM6RawHeldCursorOwner.h"

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

} // namespace mm6
