#pragma once

#include <optional>
#include <string>

#include "Engine/GameTitle.h"
#include "Engine/Party.h"
#include "Engine/Tables/ItemTable.h"
#include "Library/Geometry/Point.h"

#include "MM6RawHeldCursorOwner.h"

namespace Io { class Mouse; }

namespace mm6 {

enum class HeldCursorTint { Normal, Broken, Unidentified };

// Data-only request for a later mouse/asset transaction. An occupied item's
// pickup offset is absent from EC6 v2, so no visual offset is claimed here.
struct RawCursorImagePlan {
    bool occupied = false;
    ItemId itemId = ITEM_NULL;
    std::string iconName;
    HeldCursorTint tint = HeldCursorTint::Normal;
    std::optional<Pointi> pickedItemOffset;
};

// Check the detached Party item against the authoritative physical HELD
// record, then derive only the icon name and draw tint used by Io::Mouse.
// Does not resolve GraphicsImage or change live mouse, Party or globals.
RawCursorImagePlan stageRawCursorImagePlan(const Party &party,
                                           const RawHeldCursorOwner &cursor,
                                           const ItemTable &itemTable,
                                           GameTitle title);

// After world preparation resets the mouse, check the empty arrow or the
// decoded custom arrow bitmap. A held item still needs an offset transaction.
void requirePostWorldEmptyCursorReady(const RawCursorImagePlan &plan,
                                      const Party &party, const Io::Mouse &pointer,
                                      bool alwaysCustomCursor, GameTitle title);

} // namespace mm6
