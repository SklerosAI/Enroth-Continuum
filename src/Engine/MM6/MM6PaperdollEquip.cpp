#include "MM6PaperdollEquip.h"

#include <cassert>

#include "Engine/Data/SoundEnums.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/ItemEnumFunctions.h"
#include "Engine/Party.h"
#include "Io/Mouse.h"
#include "Media/Audio/AudioPlayer.h"

#include "MM6BoundedInventoryProjection.h"

namespace mm6 {

/**
 * Puts the held item into an equipment slot the MM6 way. An occupied slot is swapped at the same record index, and
 * the item coming back onto the cursor gets its body location byte cleared. With no free record nothing happens.
 *
 * @param character                     Character whose paperdoll was clicked.
 * @param slot                          Slot to equip into.
 */
static void equipHeldItem(Character *character, ItemSlot slot) {
    CharacterInventory &inventory = character->inventory;
    if (InventoryEntry entry = inventory.entry(slot)) {
        const int index = entry.index();
        Item outgoing = inventory.take(entry);
        Item incoming = pParty->takeHoldingItem();
        setRawBodyLocation(&incoming, slot);
        InventoryEntry equipped = inventory.tryRestoreAt(index, slot, incoming);
        assert(equipped);
        setRawBodyLocation(&outgoing, ITEM_SLOT_INVALID);
        pParty->setHoldingItem(outgoing);
    } else if (inventory.canEquip(slot)) {
        Item incoming = pParty->takeHoldingItem();
        setRawBodyLocation(&incoming, slot);
        inventory.equip(slot, incoming);
    }
}

void onPaperdollLeftClickWithHeldItem() {
    Character &character = pParty->activeCharacter();
    CharacterInventory &inventory = character.inventory;
    const ItemType type = pParty->pPickedItem.type();
    const Skill skill = pParty->pPickedItem.skill();
    InventoryEntry mainHand = inventory.entry(ITEM_SLOT_MAIN_HAND);
    InventoryEntry offHand = inventory.entry(ITEM_SLOT_OFF_HAND);
    const bool twoHanded = mainHand && mainHand->type() == ITEM_TYPE_TWO_HANDED;

    if (type < ITEM_TYPE_SINGLE_HANDED || type > ITEM_TYPE_WAND) {
        character.useItem(pParty->activeCharacterIndex(), false);
        return;
    }

    // MM6 rings, amulets and other worn items are misc items. Misc items and clubs need no skill.
    if (type != ITEM_TYPE_RING && skill != SKILL_CLUB && skill != SKILL_MISC && !character.HasSkill(skill)) {
        character.playReaction(SPEECH_CANT_EQUIP);
        return;
    }

    switch (type) {
    case ITEM_TYPE_RING:
        for (ItemSlot slot : allRingSlots()) {
            if (inventory.canEquip(slot)) {
                equipHeldItem(&character, slot);
                return;
            }
        }
        if (inventory.entry(ITEM_SLOT_RING6)) // The original swaps an empty ring 6 through record index -1.
            equipHeldItem(&character, ITEM_SLOT_RING6);
        return;
    case ITEM_TYPE_SHIELD:
        if (!offHand && twoHanded) {
            pAudioPlayer->playUISound(SOUND_error); // The original swaps the two-hander out but leaves its record numbered.
            return;
        }
        equipHeldItem(&character, ITEM_SLOT_OFF_HAND);
        return;
    case ITEM_TYPE_TWO_HANDED:
        if (offHand) {
            // With the main hand empty the original swaps the off hand out but leaves its record numbered.
            pAudioPlayer->playUISound(SOUND_error);
            return;
        }
        equipHeldItem(&character, ITEM_SLOT_MAIN_HAND);
        return;
    case ITEM_TYPE_SINGLE_HANDED:
    case ITEM_TYPE_WAND: {
        const Mastery mastery = character.getSkillValue(skill).mastery();
        const bool offHandSkill = (skill == SKILL_DAGGER && mastery >= MASTERY_EXPERT) ||
                                  (skill == SKILL_SWORD && mastery >= MASTERY_MASTER);
        equipHeldItem(&character, offHandSkill && !twoHanded && mouse->position().x >= 560 ? ITEM_SLOT_OFF_HAND : ITEM_SLOT_MAIN_HAND);
        return;
    }
    default:
        equipHeldItem(&character, itemSlotsForItemType(type)[0]);
        return;
    }
}

} // namespace mm6
