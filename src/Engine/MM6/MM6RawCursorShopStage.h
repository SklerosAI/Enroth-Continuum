#pragma once

#include "Engine/GameTitle.h"

#include "MM6RawHeldCursorOwner.h"
#include "MM6RawSaveFields.h"
#include "MM6ShopTitleState.h"

namespace mm6 {

// A detached load candidate for HELD v2 and the physical SHOP/SCLK pair.
// The input must come from a fully decoded EC6 v2 RawSaveFields candidate.
struct StagedRawCursorShop {
    RawHeldCursorOwner cursor;
    ShopTitleState shops;
};

// Check that separate owners still represent the same decoded save image.
// Use before later live publication; edited owners need a new save capture.
void validateRawCursorShopJoin(const RawSaveFields &saved,
                               const RawHeldCursorOwner &cursor,
                               const ShopTitleState &shops, GameTitle title);

StagedRawCursorShop stageRawCursorShop(const RawSaveFields &saved, GameTitle title);

} // namespace mm6
