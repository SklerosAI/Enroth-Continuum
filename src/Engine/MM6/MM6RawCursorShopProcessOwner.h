#pragma once

#include <cstdint>

#include "Engine/GameTitle.h"

#include "MM6RawCursorShopStage.h"
#include "MM6ShopStockIdentity.h"

namespace mm6 {

// MM6 child-process cursor authority. It starts unready and must be reset on
// child startup and New Game before either owner is seeded or restored.
RawHeldCursorOwner &rawHeldCursorState(GameTitle title);
void resetRawCursorShopOwners(RawHeldCursorOwner *cursor, ShopTitleState *shops,
                              GameTitle title);
void resetRawCursorShopProcessOwners(GameTitle title);

// Preflight the exact decoded v2 source against both candidates, then publish
// with nonthrowing assignments. No Party, mouse image or house UI is touched.
void publishRawCursorShopOwners(RawHeldCursorOwner *cursor, ShopTitleState *shops,
                                const RawSaveFields &saved, StagedRawCursorShop staged,
                                GameTitle title);

// Title-keyed service read surface. Records remain native 28-byte values;
// callers must not reinterpret them as inherited MM7 Item objects.
ShopStockRecord readShopStock(const ShopTitleState &shops, ShopStockBank bank,
                              unsigned row, unsigned position, GameTitle title);
ShopStockRecord readGuildStock(const ShopTitleState &shops, unsigned row,
                               unsigned position, GameTitle title);
std::uint64_t readShopDeadline(const ShopTitleState &shops, unsigned row,
                               GameTitle title);
std::uint64_t readGuildDeadline(const ShopTitleState &shops, unsigned row,
                                GameTitle title);

} // namespace mm6
