#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Party.h"

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    for (PlatformKey key : characterKeys) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 3; skill++) {
            game.pressGuiButton(fmt::format("PartyCreation_Skill{}", skill));
            game.tick();
        }
        for (int attribute = 0; attribute < 7; attribute++) {
            for (int point = 0; point < 3; point++) {
                game.pressAndReleaseKey(PlatformKey::KEY_ADD);
                game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_DOWN);
            game.tick();
        }
    }
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
}

static Pointi giveToBackpack(int characterIndex, ItemId item) {
    return pParty->pCharacters[characterIndex].inventory.add(Item(item)).geometry().topLeft();
}

static void drinkFromBackpack(EngineController &game, PlatformKey characterKey, Pointi cell) {
    // The character picks the potion up in the backpack and drops it on the paperdoll.
    game.pressAndReleaseKey(characterKey);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_I);
    game.tick(2); // The key queues the character screen for the next frame.
    game.pressAndReleaseButton(BUTTON_LEFT, Pointi(14, 17) + cell * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {550, 200}); // The paperdoll.
    game.tick();
}

GAME_TEST(MM6Potion, DrinkCureWoundsAndMagicPotion) {
    // MM6 potions could not be drunk, every herb and potion was "can not be used that way".
    startNewGameThroughInput(game);
    pParty->pCharacters[1].health = 5;
    pParty->pCharacters[2].mana = 0;
    const Pointi cureWounds = giveToBackpack(1, static_cast<ItemId>(164));
    const Pointi magicPotion = giveToBackpack(2, static_cast<ItemId>(165));
    auto hpTape = charTapes.hps();
    auto mpTape = charTapes.mps();
    auto cureWoundsTape = tapes.totalItemCount(static_cast<ItemId>(164));
    auto magicPotionTape = tapes.totalItemCount(static_cast<ItemId>(165));
    auto bottleTape = tapes.totalItemCount(static_cast<ItemId>(163));
    auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
    auto soundsTape = tapes.sounds();
    auto screenTape = tapes.screen();
    test.startTaping();

    drinkFromBackpack(game, PlatformKey::KEY_DIGIT_2, cureWounds);
    drinkFromBackpack(game, PlatformKey::KEY_DIGIT_3, magicPotion);

    EXPECT_EQ(hpTape.frontBack(), tape({42, 5, 20, 20}, {42, 15, 20, 20})); // MM6 potions have no power, Cure Wounds heals 10.
    EXPECT_EQ(mpTape.frontBack(), tape({0, 19, 0, 13}, {0, 19, 10, 13}));
    EXPECT_EQ(cureWoundsTape, tape(1, 0));
    EXPECT_EQ(magicPotionTape, tape(1, 0));
    EXPECT_EQ(bottleTape, tape(0)); // No empty bottle comes back.
    EXPECT_EQ(heldTape.back(), ITEM_NULL);
    EXPECT_EQ(soundsTape.flatten().count(SOUND_drink), 2);
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHARACTERS, SCREEN_GAME, SCREEN_CHARACTERS, SCREEN_GAME)); // Drinking closes the screen.
}

GAME_TEST(MM6Potion, CurePoisonCuresPoison) {
    // Cure Poison clears every poison and nothing else, a diseased character stays diseased.
    startNewGameThroughInput(game);
    for (Condition condition : {CONDITION_POISON_WEAK, CONDITION_POISON_MEDIUM, CONDITION_POISON_SEVERE, CONDITION_DISEASE_WEAK})
        pParty->pCharacters[0].conditions.set(condition, pParty->GetPlayingTime());
    auto conditionsTape = charTapes.custom(0, [](const Character &character) {
        return std::vector<bool>{character.conditions.has(CONDITION_POISON_WEAK), character.conditions.has(CONDITION_POISON_MEDIUM),
                                 character.conditions.has(CONDITION_POISON_SEVERE), character.conditions.has(CONDITION_DISEASE_WEAK)};
    });
    const Pointi curePoison = giveToBackpack(0, static_cast<ItemId>(169));
    auto potionTape = tapes.totalItemCount(static_cast<ItemId>(169));
    test.startTaping();

    drinkFromBackpack(game, PlatformKey::KEY_DIGIT_1, curePoison);

    EXPECT_EQ(conditionsTape.frontBack(), tape(std::vector{true, true, true, true}, std::vector{false, false, false, true}));
    EXPECT_EQ(potionTape, tape(1, 0));
}

GAME_TEST(MM6Potion, EmptyBottleStaysInHand) {
    // An empty MM6 bottle can't be drunk, it stays in the hand with the "can not be used that way" message.
    startNewGameThroughInput(game);
    const Pointi bottle = giveToBackpack(0, static_cast<ItemId>(163));
    auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
    auto statusTape = tapes.statusBar();
    auto soundsTape = tapes.sounds();
    test.startTaping();

    drinkFromBackpack(game, PlatformKey::KEY_DIGIT_1, bottle);

    EXPECT_EQ(heldTape, tape(ITEM_NULL, static_cast<ItemId>(163)));
    EXPECT_CONTAINS(statusTape, "Potion Bottle can not be used that way");
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_error);
    EXPECT_MISSES(soundsTape.flatten(), SOUND_drink);
}
