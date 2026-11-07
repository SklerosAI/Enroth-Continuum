#pragma once

#include "Engine/GameTitle.h"
#include "Engine/Party.h"
#include "Engine/Tables/ItemTable.h"

#include "MM6RawHeldCursorOwner.h"

namespace mm6 {

// Project the saved physical cursor record onto a detached Party copy. The
// raw cursor remains the authority; no mouse image, live Party or process
// owner is changed. Unsupported item unions fail before the copy is returned.
Party stageRawHeldParty(const Party &baseline, const RawHeldCursorOwner &cursor,
                        const ItemTable &itemTable, GameTitle title);

} // namespace mm6
