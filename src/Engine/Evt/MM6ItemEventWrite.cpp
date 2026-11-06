#include "Engine/Evt/MM6ItemEventWrite.h"

#include <stdexcept>

namespace mm6 {
namespace {

ItemId checkedItemId(int rawId, int value) {
    if (rawId != 17 || value < 1 || value > 580)
        throw std::runtime_error("MM6 item event variable or operand is outside title range");
    return static_cast<ItemId>(value);
}

InventoryEntry uniqueGridItem(Character &character, ItemId itemId) {
    InventoryEntry found;
    Sizei grid = character.inventory.gridSize();
    for (int y = 0; y < grid.h; ++y) {
        for (int x = 0; x < grid.w; ++x) {
            InventoryEntry entry = character.inventory.entry(Pointi(x, y));
            if (!entry || entry->itemId != itemId || (found && found.index() == entry.index()))
                continue;
            if (found)
                throw std::runtime_error("MM6 item removal has duplicate backpack matches");
            found = entry;
        }
    }
    return found;
}

} // namespace

Item makeEventItem(int rawId, int value, const Item &heldItem) {
    ItemId itemId = checkedItemId(rawId, value);
    if (heldItem.itemId != ITEM_NULL)
        throw std::runtime_error("MM6 item reward with occupied cursor needs relocation preflight");
    if (value >= 400 && value <= 429)
        throw std::runtime_error("MM6 generated-artifact state is not yet durable");
    Item item;
    item.itemId = itemId;
    item.flags = ITEM_IDENTIFIED;
    return item;
}

std::vector<ItemRemovalStep> planEventItemRemoval(std::span<Character *> characters,
                                                   const Item &heldItem, int rawId, int value) {
    ItemId itemId = checkedItemId(rawId, value);
    bool heldAvailable = heldItem.itemId == itemId;
    std::vector<ItemRemovalStep> result;
    result.reserve(characters.size());
    for (Character *character : characters) {
        if (!character)
            throw std::runtime_error("MM6 item removal selected a null character");
        ItemRemovalStep step;
        step.character = character;
        step.inventoryEntry = uniqueGridItem(*character, itemId);
        step.held = !step.inventoryEntry && heldAvailable;
        if (step.held)
            heldAvailable = false;
        result.push_back(step);
    }
    return result;
}

} // namespace mm6
