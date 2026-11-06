#include "Engine/Evt/MM6ItemEventWrite.h"

#include <stdexcept>

#include "Engine/Tables/ItemTable.h"

namespace mm6 {
namespace {

ItemId checkedItemId(int rawId, int value) {
    if (rawId != 17 || value < 1 || value > 580)
        throw std::runtime_error("MM6 item event variable or operand is outside title range");
    return static_cast<ItemId>(value);
}

InventoryEntry firstGridItem(Character &character, ItemId itemId) {
    Sizei grid = character.inventory.gridSize();
    for (int y = 0; y < grid.h; ++y) {
        for (int x = 0; x < grid.w; ++x) {
            InventoryEntry entry = character.inventory.entry(Pointi(x, y));
            if (entry && entry->itemId == itemId)
                return entry;
        }
    }
    return {};
}

} // namespace

Item makeEventItem(int rawId, int value) {
    ItemId itemId = checkedItemId(rawId, value);
    if (value >= 400 && value <= 429)
        throw std::runtime_error("MM6 generated-artifact state is not yet durable");
    Item item;
    item.itemId = itemId;
    item.flags = ITEM_IDENTIFIED;
    return item;
}

Character *planEventItemRelocation(std::span<Character *> party, int activeIndex,
                                   const Item &heldItem) {
    if (party.size() != 4 || activeIndex < -1 || activeIndex >= 4 ||
        heldItem.itemId < ITEM_FIRST_VALID || heldItem.itemId > static_cast<ItemId>(580) || !pItemTable)
        throw std::runtime_error("MM6 item relocation has invalid party or held item");
    for (Character *character : party)
        if (!character)
            throw std::runtime_error("MM6 item relocation has a null character");

    Sizei size = heldItem.inventorySize();
    if (size.w <= 0 || size.h <= 0)
        throw std::runtime_error("MM6 held item has no valid inventory size");
    if (activeIndex >= 0 && party[activeIndex]->inventory.canAdd(size))
        return party[activeIndex];
    for (Character *character : party)
        if (character->inventory.canAdd(size))
            return character;
    throw std::runtime_error("MM6 item reward requires unproved ground-drop fallback");
}

InventoryEntry relocateEventItemToInventory(Character &recipient, Item &heldItem) {
    if (heldItem.itemId == ITEM_NULL)
        throw std::runtime_error("MM6 item relocation found an empty cursor");
    InventoryEntry stored = recipient.inventory.tryAdd(heldItem);
    if (!stored)
        throw std::runtime_error("MM6 item relocation lost reserved inventory space");
    heldItem.Reset();
    return stored;
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
        step.inventoryEntry = firstGridItem(*character, itemId);
        step.held = !step.inventoryEntry && heldAvailable;
        if (step.held)
            heldAvailable = false;
        result.push_back(step);
    }
    return result;
}

} // namespace mm6
