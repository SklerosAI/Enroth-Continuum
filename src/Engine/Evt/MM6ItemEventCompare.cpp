#include "Engine/Evt/MM6ItemEventCompare.h"

#include <stdexcept>

namespace mm6 {

bool hasEventItem(const Character &character, const Item &heldItem, int rawId, int value) {
    if (rawId != 17 || value < 1 || value > 580)
        throw std::runtime_error("MM6 item event variable or operand is outside title range");
    const ItemId itemId = static_cast<ItemId>(value);
    return static_cast<bool>(character.inventory.find(itemId)) || heldItem.itemId == itemId;
}

} // namespace mm6
