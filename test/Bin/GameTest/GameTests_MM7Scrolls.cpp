#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Party.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Item.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/MonsterEnums.h"

#include "GUI/GUIEnums.h"
#include "GUI/GUIWindow.h"

#include "GameTestCommon.h"

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

GAME_TEST(MM7Scrolls, CancelActorTargetScroll) {
    // Cancelling the targeting after reading a Fire Bolt scroll used the scroll up.
    for (bool cancel : {true, false}) {
        SCOPED_TRACE(fmt::format("cancel={}", cancel));
        test.prepareForNextTest();
        auto scrollTape = tapes.totalItemCount(ITEM_SCROLL_FIRE_BOLT);
        auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
        auto targetingTape = tapes.custom([] { return pGUIWindow_CastTargetedSpell != nullptr; });
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        test.startTaping();
        prepareForBattleTest();
        engine->config->debug.NoActors.setValue(false);

        game.spawnMonster(pParty->pos + Vec3f(0, 300, 0), MONSTER_RAT_A, SPAWN_DUMMY);
        auto hpTape = actorTapes.hp(0);
        pParty->setHoldingItem(Item(ITEM_SCROLL_FIRE_BOLT));
        game.tick();
        game.pressGuiButton("Game_Character1", BUTTON_RIGHT);
        game.tick(2); // The scroll is read, and on the next frame the targeting cursor comes up.
        if (cancel) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        } else {
            game.pointMouseAtActor(0);
            game.pressAndReleaseButton(BUTTON_LEFT);
        }
        game.tick(2); // The bolt flies to the rat.

        EXPECT_EQ(targetingTape, tape(false, true, false));
        if (cancel) {
            EXPECT_EQ(scrollTape, tape(0, 1, 0, 1));
            EXPECT_EQ(heldTape, tape(ITEM_NULL, ITEM_SCROLL_FIRE_BOLT, ITEM_NULL, ITEM_SCROLL_FIRE_BOLT));
            EXPECT_EQ(hpTape.delta(), 0);
        } else {
            EXPECT_EQ(scrollTape, tape(0, 1, 0));
            EXPECT_EQ(heldTape, tape(ITEM_NULL, ITEM_SCROLL_FIRE_BOLT, ITEM_NULL));
            EXPECT_LT(hpTape.delta(), 0);
        }
    }
}

GAME_TEST(MM7Scrolls, CancelCharacterTargetScroll) {
    // Cancelling the targeting after reading a Cure Weakness scroll used the scroll up.
    for (bool cancel : {true, false}) {
        SCOPED_TRACE(fmt::format("cancel={}", cancel));
        test.prepareForNextTest();
        auto scrollTape = tapes.totalItemCount(ITEM_SCROLL_CURE_WEAKNESS);
        auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
        auto targetingTape = tapes.custom([] { return pGUIWindow_CastTargetedSpell != nullptr; });
        auto conditionTape = charTapes.condition(1);
        game.startNewGame();
        test.startTaping();

        pParty->pCharacters[1].SetCondition(CONDITION_WEAK, false);
        pParty->setHoldingItem(Item(ITEM_SCROLL_CURE_WEAKNESS));
        game.tick();
        game.pressGuiButton("Game_Character1", BUTTON_RIGHT);
        game.tick(2); // The scroll is read, and on the next frame the targeting cursor comes up.
        if (cancel) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        } else {
            game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
        }
        game.tick();

        EXPECT_EQ(targetingTape, tape(false, true, false));
        if (cancel) {
            EXPECT_EQ(scrollTape, tape(1, 0, 1));
            EXPECT_EQ(heldTape, tape(ITEM_SCROLL_CURE_WEAKNESS, ITEM_NULL, ITEM_SCROLL_CURE_WEAKNESS));
            EXPECT_EQ(conditionTape, tape(CONDITION_WEAK));
        } else {
            EXPECT_EQ(scrollTape, tape(1, 0));
            EXPECT_EQ(heldTape, tape(ITEM_SCROLL_CURE_WEAKNESS, ITEM_NULL));
            EXPECT_EQ(conditionTape, tape(CONDITION_WEAK, CONDITION_GOOD));
        }
    }
}

GAME_TEST(MM7Scrolls, CancelActorTargetScrollFromInventory) {
    // Cancelling the targeting after reading a Fire Bolt scroll on a portrait from the inventory used the scroll up.
    for (bool cancel : {true, false}) {
        SCOPED_TRACE(fmt::format("cancel={}", cancel));
        test.prepareForNextTest();
        auto scrollTape = tapes.totalItemCount(ITEM_SCROLL_FIRE_BOLT);
        auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
        auto screenTape = tapes.screen();
        auto targetingTape = tapes.custom([] { return pGUIWindow_CastTargetedSpell != nullptr; });
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        test.startTaping();
        prepareForBattleTest();
        engine->config->debug.NoActors.setValue(false);

        game.spawnMonster(pParty->pos + Vec3f(0, 300, 0), MONSTER_RAT_A, SPAWN_DUMMY);
        auto hpTape = actorTapes.hp(0);
        game.goToInventory(0);
        pParty->setHoldingItem(Item(ITEM_SCROLL_FIRE_BOLT));
        game.tick();
        game.pressGuiButton("Game_Character1", BUTTON_RIGHT);
        game.tick(2); // The scroll is read and the inventory closes, on the next frame the targeting cursor comes up.
        if (cancel) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        } else {
            game.pointMouseAtActor(0);
            game.pressAndReleaseButton(BUTTON_LEFT);
        }
        game.tick(2); // The bolt flies to the rat.

        EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHARACTERS, SCREEN_GAME));
        EXPECT_EQ(targetingTape, tape(false, true, false));
        if (cancel) {
            EXPECT_EQ(scrollTape, tape(0, 1, 0, 1));
            EXPECT_EQ(heldTape, tape(ITEM_NULL, ITEM_SCROLL_FIRE_BOLT, ITEM_NULL, ITEM_SCROLL_FIRE_BOLT));
            EXPECT_EQ(hpTape.delta(), 0);
        } else {
            EXPECT_EQ(scrollTape, tape(0, 1, 0));
            EXPECT_EQ(heldTape, tape(ITEM_NULL, ITEM_SCROLL_FIRE_BOLT, ITEM_NULL));
            EXPECT_LT(hpTape.delta(), 0);
        }
    }
}
