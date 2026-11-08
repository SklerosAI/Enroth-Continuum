#pragma once

#include "Engine/Objects/ItemEnums.h"

namespace mm6 {

/**
 * MM6 GiveItem, evt opcode 41. Rolls a random item on the MM6 gameplay LCG even when a fixed ID is given, then the
 * fixed ID replaces only the item number and the rolled bonus, charges and identified flag stay. The item goes on
 * the cursor. A previously held item moves to the active character, then to characters 0..3.
 *
 * Throws before changing anything when the roll falls outside the original generator's tables, the LCG isn't
 * restored, or no character has room for the held item. MM6 drops the held item on the ground then, which has no
 * port yet.
 *
 * @param treasureLevel                 Treasure level 1..6.
 * @param requestedKind                 Requested item kind.
 * @param fixedId                       Item ID that replaces the rolled one, or `ITEM_NULL`.
 */
void giveEventItem(ItemTreasureLevel treasureLevel, RandomItemType requestedKind, ItemId fixedId);

} // namespace mm6
