#include "Engine/Evt/MM6GiveItemEvent.h"

#include <array>
#include <stdexcept>
#include <utility>

#include "Engine/Evt/MM6ItemEventWrite.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6ItemGenerator.h"
#include "Engine/Party.h"
#include "Engine/Tables/ItemTable.h"

#include "Io/Mouse.h"

namespace mm6 {

void giveEventItem(ItemTreasureLevel treasureLevel, RandomItemType requestedKind, ItemId fixedId) {
    const int fixed = std::to_underlying(fixedId);
    if (fixed < 0 || fixed > 580)
        throw std::runtime_error("MM6 GiveItem fixed item is outside the title's item range");
    StagedTitleItem staged = stageTitleItem(chestTitleState(), *pItemTable, treasureLevel, requestedKind);
    if (fixedId != ITEM_NULL)
        staged.item.itemId = fixedId;

    if (pParty->pPickedItem.itemId == ITEM_NULL) {
        pParty->setHoldingItem(staged.item);
        chestTitleState() = staged.nextState;
        return;
    }

    std::array<Character *, 4> party;
    for (int i = 0; i < 4; ++i)
        party[i] = &pParty->pCharacters[i];
    Character *recipient = planEventItemRelocation(party, pParty->hasActiveCharacter() ? pParty->activeCharacterIndex() : -1,
                                                   pParty->pPickedItem);
    Item previous = pParty->pPickedItem;
    Pointi previousOffset = mouse->pickedItemOffset;
    InventoryEntry stored = relocateEventItemToInventory(*recipient, pParty->pPickedItem);
    try {
        pParty->setHoldingItem(staged.item);
    } catch (...) {
        pParty->pPickedItem = previous;
        recipient->inventory.take(stored);
        try {
            mouse->SetCursorBitmapFromItemID(previous.itemId);
        } catch (...) {
        }
        mouse->pickedItemOffset = previousOffset;
        throw;
    }
    chestTitleState() = staged.nextState;
}

} // namespace mm6
