#pragma once

#include <span>
#include <vector>

#include "Engine/Objects/Character.h"
#include "Engine/Objects/Item.h"

namespace mm6 {

struct ItemRemovalStep {
    Character *character = nullptr;
    InventoryEntry inventoryEntry;
    bool held = false;
};

// MM6's fixed-item Add creates an identified, otherwise empty item in the cursor.
// Occupied-cursor relocation and the MM6 generated-artifact band stay guarded.
Item makeEventItem(int rawId, int value, const Item &heldItem);

// Plan every selected character before applying a removal, so an ambiguous
// duplicate grid match cannot leave a partly changed party.
std::vector<ItemRemovalStep> planEventItemRemoval(std::span<Character *> characters,
                                                   const Item &heldItem, int rawId, int value);

} // namespace mm6
