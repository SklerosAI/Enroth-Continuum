#pragma once

#include "Engine/GameTitle.h"
#include "Engine/Party.h"
#include "Engine/Tables/ItemTable.h"

#include "MM6RawInventoryFields.h"

namespace mm6 {

// A detached Party copy carrying four source-indexed MM6 inventories. Raw
// physical records remain authoritative beside the runtime Item projections.
struct StagedRawPartyInventories {
    Party party;
    RawInventoryFields raw;
};

// Replace only the four inventories on a Party copy. Every referenced record
// must have a reversible, source-typed view; unsupported item semantics fail
// before the caller's Party or raw owner changes.
StagedRawPartyInventories stageRawPartyInventories(
    const Party &baseline, const RawInventoryFields &raw,
    const ItemTable &table, GameTitle title);

// Overlay edits from the detached Party into the original native records.
// Each item's 28-byte carrier is written at its current record index and
// placement, vacated records are cleared, and unsupported edits fail closed.
RawInventoryFields captureRawPartyInventories(
    const StagedRawPartyInventories &staged,
    const ItemTable &table, GameTitle title);

} // namespace mm6
