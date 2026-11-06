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

// MM6's fixed-item Add creates an identified, otherwise empty item.
// The generated-artifact band stays guarded until its state is durable.
Item makeEventItem(int rawId, int value);

// The original cursor setter tries the active character first, then characters
// 0..3 in order. Ground-drop fallback stays guarded until it is safe to persist.
Character *planEventItemRelocation(std::span<Character *> party, int activeIndex,
                                   const Item &heldItem);

// Insert the exact held item before clearing the cursor. A failed insertion
// leaves the cursor unchanged and is reported to the event interpreter.
InventoryEntry relocateEventItemToInventory(Character &recipient, Item &heldItem);

// Scan each selected character's 126 grid cells in original row-major order,
// taking the first matching item before considering the cursor.
std::vector<ItemRemovalStep> planEventItemRemoval(std::span<Character *> characters,
                                                   const Item &heldItem, int rawId, int value);

} // namespace mm6
