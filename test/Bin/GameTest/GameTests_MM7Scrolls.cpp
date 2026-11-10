#include "Testing/Game/GameTest.h"

#include "Engine/Party.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/Item.h"
#include "Engine/Objects/ItemEnums.h"

#include "GUI/GUIEnums.h"

GAME_TEST(MM7Scrolls, CancelItemTargetScroll) {
    // Cancelling the item pick after reading a Fire Aura scroll used the scroll up.
    for (bool cancel : {true, false}) {
        SCOPED_TRACE(fmt::format("cancel={}", cancel));
        test.prepareForNextTest();
        auto scrollTape = tapes.totalItemCount(ITEM_SCROLL_FIRE_AURA);
        auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
        auto screenTape = tapes.screen();
        auto enchantmentTape = tapes.custom([] {
            InventoryEntry sword = pParty->pCharacters[0].inventory.entry(ITEM_SLOT_MAIN_HAND);
            return sword ? sword->specialEnchantment : ITEM_ENCHANTMENT_NULL;
        });
        game.startNewGame();
        test.startTaping();

        pParty->pCharacters[0].inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_BROADSWORD));
        pParty->setHoldingItem(Item(ITEM_SCROLL_FIRE_AURA));
        game.tick();
        game.pressGuiButton("Game_Character1", BUTTON_RIGHT); // Read the scroll, the inventory opens to pick an item.
        game.tick();
        if (cancel) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        } else {
            game.pressAndReleaseButton(BUTTON_LEFT, 521, 95); // The sword on the paperdoll.
        }
        game.tick();

        if (cancel) {
            EXPECT_EQ(scrollTape, tape(1, 0, 1));
            EXPECT_EQ(heldTape, tape(ITEM_SCROLL_FIRE_AURA, ITEM_NULL, ITEM_SCROLL_FIRE_AURA));
            EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CASTING, SCREEN_GAME));
            EXPECT_EQ(enchantmentTape, tape(ITEM_ENCHANTMENT_NULL));
        } else {
            EXPECT_EQ(scrollTape, tape(1, 0));
            EXPECT_EQ(heldTape, tape(ITEM_SCROLL_FIRE_AURA, ITEM_NULL));
            EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CASTING));
            EXPECT_EQ(enchantmentTape, tape(ITEM_ENCHANTMENT_NULL, ITEM_ENCHANTMENT_OF_INFERNOS)); // A scroll casts at master.
        }
    }
}
