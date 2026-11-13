#include <algorithm>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/TileData.h"
#include "Engine/Engine.h"
#include "Engine/EngineCallObserver.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/TurnEngine/TurnEngine.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIRest.h"
#include "GUI/UI/UIStatusBar.h"

#include "Library/Image/Png.h"

#include "Utility/Math/TrigLut.h"
#include "Utility/Streams/FileOutputStream.h"
#include "Utility/System/Path.h"

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

static Vec3f nearestGround(int sourceTileset, float minDistance = 0) {
    // Center of the nearest tile of the given MM6 tileset at least minDistance away from the party.
    Vec3f result;
    float bestDistance = std::numeric_limits<float>::max();
    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            if (pOutdoor->pTerrain.tileDataByGrid(Pointi(x, y)).sourceTileset != sourceTileset)
                continue;
            const Vec3f center((x - 64) * 512 + 256, (63 - y) * 512 + 256, 0);
            const float distance = (center - pParty->pos).xy().length();
            if (distance >= minDistance && distance < bestDistance) {
                bestDistance = distance;
                result = center;
            }
        }
    }
    result.z = pOutdoor->pTerrain.heightByPos(result);
    return result;
}

GAME_TEST(MM6Melee, KillGoblinAndLootIt) {
    // A goblin dies to blows from the A key and from clicking it, gives experience and leaves gold on its body.
    startNewGameThroughInput(game);
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    const int goblin = game.spawnMonster(pParty->pos + ahead * 150, static_cast<MonsterId>(76), SPAWN_STATIONARY)->id; // Stationary, a wounded goblin runs.
    auto hpTape = actorTapes.hp(goblin);
    auto stateTape = actorTapes.aiState(goblin);
    auto experienceTape = tapes.totalExperience();
    auto statusTape = tapes.statusBar();
    auto goldTape = tapes.gold();
    test.startTaping();

    for (int i = 0; i < 200 && !pActors[goblin].IsNotAlive(); i++) { // Blows can miss.
        game.pointMouseAtActor(goblin);
        if (i % 2) {
            game.pressAndReleaseKey(PlatformKey::KEY_A);
        } else {
            game.pressAndReleaseButton(BUTTON_LEFT);
        }
        game.tick();
    }
    while (pActors[goblin].aiState != Dead)
        game.tick(); // The goblin falls.
    game.pointMouseAtActor(goblin);
    game.pressAndReleaseButton(BUTTON_LEFT); // Search the body.
    game.tick();

    EXPECT_EQ(hpTape.front(), 13);
    EXPECT_LE(hpTape.back(), 0);
    EXPECT_CONTAINS(stateTape, Dying);
    EXPECT_CONTAINS(stateTape, Dead);
    EXPECT_EQ(stateTape.back(), Removed); // The searched body is gone.
    EXPECT_TRUE(std::ranges::any_of(statusTape, [](const std::string &status) { return status.contains("hits Goblin for"); }));
    EXPECT_TRUE(std::ranges::any_of(statusTape, [](const std::string &status) { return status.contains("killing Goblin"); }));
    EXPECT_EQ(experienceTape.delta(), 56); // The goblin's experience, split between the four.
    EXPECT_CONTAINS(statusTape, "You found 8 gold!");
    EXPECT_EQ(goldTape.delta(), 8);
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

GAME_TEST(MM6TurnBased, PartyAndMonstersTakeTurns) {
    // MM6 drew no turn-based indicator, and the party had MM7's movement phase between the rounds.
    startNewGameThroughInput(game);
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    const int goblin = game.spawnMonster(pParty->pos + ahead * 150, static_cast<MonsterId>(76), SPAWN_DUMMY)->id;
    auto turnBasedTape = tapes.turnBasedMode();
    auto indicatorTape = tapes.hudQuads();
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto timeTape = tapes.custom([] { return pParty->GetPlayingTime().ticks(); });
    auto goblinHpTape = actorTapes.hp(goblin);
    auto soundsTape = tapes.sounds();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_RETURN);
    game.tick();
    while (!(pTurnEngine->turn_stage == TE_ATTACK && pTurnEngine->flags & TE_PLAYER_TURN))
        game.tick(); // The monsters finish their opening move.
    game.pressKey(PlatformKey::KEY_UP);
    game.tick();
    game.releaseKey(PlatformKey::KEY_UP);
    game.pointMouseAtActor(goblin);
    for (int i = 0; i < 4; i++) { // Two attacks, then the other two characters pass.
        game.pressAndReleaseKey(i < 2 ? PlatformKey::KEY_A : PlatformKey::KEY_B);
        game.tick();
    }
    while (pTurnEngine->turn_stage != TE_WAIT) { // The monsters strike, and the party passes until the round ends.
        game.pressAndReleaseKey(PlatformKey::KEY_B);
        game.tick();
    }
    while (!(pTurnEngine->turn_stage == TE_ATTACK && pTurnEngine->flags & TE_PLAYER_TURN))
        game.tick(); // The monsters move.
    game.pressAndReleaseKey(PlatformKey::KEY_RETURN);
    game.tick();

    auto indicator = indicatorTape.flatten().filter([](const DrawnQuad &quad) { return quad.texture.starts_with("new"); });
    auto hand = indicator.filter([](const DrawnQuad &quad) { return quad.texture.starts_with("newhand"); });
    auto hourglass = indicator.filter([](const DrawnQuad &quad) { return quad.texture.starts_with("newglas"); });
    EXPECT_EQ(turnBasedTape, tape(true, false));
    EXPECT_FALSE(hand.empty());
    EXPECT_FALSE(hourglass.empty());
    EXPECT_EQ(hand.size() + hourglass.size(), indicator.size());
    for (const DrawnQuad &quad : hand)
        EXPECT_EQ(quad.position, Pointi(432, 305)); // Centered on (444, 326).
    for (const DrawnQuad &quad : hourglass)
        EXPECT_EQ(quad.position, Pointi(433, 306));
    EXPECT_EQ(positionTape.size(), 1); // The party can't walk in turn-based mode.
    EXPECT_CONTAINS(timeTape.adjacentDeltas(), 426); // A round is 100 game seconds.
    EXPECT_MISSES(timeTape.adjacentDeltas(), 213); // MM7's movement phase.
    EXPECT_LT(goblinHpTape.min(), 13);
    EXPECT_CONTAINS(soundsTape.flatten(), static_cast<SoundId>(207)); // Entering turn-based mode.
    EXPECT_CONTAINS(soundsTape.flatten(), static_cast<SoundId>(206)); // Leaving it.
}

GAME_TEST(MM6Rest, RestAndHealOnGrass) {
    // Rest and heal by the New Sorpigal fountain, on grass, costs one food and brings everyone back to full.
    startNewGameThroughInput(game);
    for (Character &character : pParty->pCharacters) {
        character.health = 1;
        character.mana = 0;
    }
    auto hpTape = charTapes.hps();
    auto mpTape = charTapes.mps();
    auto foodTape = tapes.food();
    auto screenTape = tapes.screen();
    auto timeTape = tapes.time();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_RestAndHeal");
    game.tick();
    while (current_screen_type == SCREEN_REST)
        game.tick(); // The party sleeps eight hours.

    EXPECT_EQ(hpTape.frontBack(), tape({1, 1, 1, 1}, {42, 22, 20, 20}));
    EXPECT_EQ(mpTape.frontBack(), tape({0, 0, 0, 0}, {0, 19, 19, 13}));
    EXPECT_EQ(foodTape, tape(7, 6));
    EXPECT_EQ(screenTape, tape(SCREEN_REST, SCREEN_GAME));
    EXPECT_GE(timeTape.back() - timeTape.front(), Duration::fromHours(8));
}

GAME_TEST(MM6Rest, VolcanicGroundCostsFourFood) {
    // Resting on New Sorpigal's volcanic ground cost two food, MM6 asks four there.
    startNewGameThroughInput(game);
    game.teleportTo(engine->_currentLoadedMapId, nearestGround(3), 0);
    auto foodTape = tapes.food();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_RestAndHeal");
    game.tick();

    EXPECT_EQ(foodTape, tape(7, 3));
}

GAME_TEST(MM6Rest, NotEnoughFood) {
    // Without enough food the party doesn't rest, the original plays the error sound and no speech.
    startNewGameThroughInput(game);
    pParty->SetFood(0);
    pParty->pCharacters[0].health = 1;
    auto hpTape = charTapes.hp(0);
    auto statusTape = tapes.statusBar();
    auto soundsTape = tapes.sounds();
    auto screenTape = tapes.screen();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_RestAndHeal");
    game.tick();

    EXPECT_EQ(hpTape, tape(1));
    EXPECT_CONTAINS(statusTape, "You don't have enough food to rest");
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_error);
    EXPECT_EQ(screenTape, tape(SCREEN_REST));
}

GAME_TEST(MM6Rest, HostileMonsterNearby) {
    // A hostile monster in sight keeps the party from resting.
    startNewGameThroughInput(game);
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    game.spawnMonster(pParty->pos + ahead * 3000, static_cast<MonsterId>(76), SPAWN_STATIONARY);
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME));
    EXPECT_CONTAINS(statusTape, "There are hostile enemies near!");
}

GAME_TEST(MM6Rest, WaitAnHourNeitherEatsNorHeals) {
    // Waiting an hour passes the time without food and without healing.
    startNewGameThroughInput(game);
    pParty->pCharacters[0].health = 1;
    auto hpTape = charTapes.hp(0);
    auto foodTape = tapes.food();
    auto timeTape = tapes.time();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_Wait1Hour");
    game.tick();
    while (timeTape.back() < timeTape.front() + Duration::fromHours(1))
        game.tick(); // The hour passes.

    EXPECT_EQ(hpTape, tape(1));
    EXPECT_EQ(foodTape, tape(7));
}

GAME_TEST(MM6Rest, FirstNightWithoutRestWeakens) {
    // A new party has already gone a day without rest, so waiting until dawn on the first night weakens everyone and
    // eats a food.
    startNewGameThroughInput(game);
    const int actorCount = pActors.size();
    auto weakTape = charTapes.custom([](const Character &character) { return character.conditions.has(CONDITION_WEAK); });
    auto foodTape = tapes.food();
    auto clockTape = tapes.custom([] { return std::pair(pParty->uCurrentHour, pParty->uCurrentMinute); });
    auto restTape = tapes.custom([] { return currentRestType; });
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_WaitTillDawn");
    game.tick();
    while (currentRestType != REST_NONE)
        game.tick(); // The night passes.

    EXPECT_EQ(weakTape.frontBack(), tape({false, false, false, false}, {true, true, true, true}));
    EXPECT_EQ(foodTape, tape(7, 6));
    EXPECT_EQ(clockTape.front(), std::pair(9, 0));
    EXPECT_EQ(clockTape.back(), std::pair(5, 0));
    EXPECT_EQ(restTape, tape(REST_NONE, REST_WAIT, REST_NONE));
    EXPECT_EQ(pActors.size(), actorCount); // Waiting rolls no encounter.
}

GAME_TEST(MM6Rest, FinishedWaitWakesSleeper) {
    // A character a monster put to sleep slept on after a wait, MM6 wakes everyone when any wait ends.
    startNewGameThroughInput(game);
    for (Character &character : pParty->pCharacters)
        character.health = 200; // The beholder's bolts would knock a new character out.
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    Actor *beholder = game.spawnMonster(pParty->pos + ahead * 400, static_cast<MonsterId>(10), SPAWN_STATIONARY);
    beholder->monsterInfo.specialAttackLevel = 10; // Level 10 times 10 makes every bolt try.
    auto sleepTape = charTapes.custom([](const Character &character) { return character.conditions.has(CONDITION_SLEEP); });
    auto restTape = tapes.custom([] { return currentRestType; });
    auto screenTape = tapes.screen();
    test.startTaping();

    auto asleep = [] { return std::ranges::any_of(pParty->pCharacters, [](const Character &character) { return character.IsAsleep(); }); };
    for (int frame = 0; frame < 300 && !asleep(); frame++)
        game.tick(); // The beholder shoots, a bolt can miss and a character can resist.
    ASSERT_TRUE(asleep());
    game.teleportTo(engine->_currentLoadedMapId, nearestGround(0, 8000), 0); // Grass out of the beholder's sight.
    game.tick();
    while (pParty->uFlags & PARTY_FLAG_AIRBORNE)
        game.tick(); // The party lands.
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_Wait1Hour");
    game.tick();
    while (currentRestType != REST_NONE)
        game.tick(); // The hour passes.

    EXPECT_EQ(sleepTape.back(), std::vector({false, false, false, false}));
    EXPECT_EQ(restTape.back(), REST_NONE);
    EXPECT_EQ(screenTape.back(), SCREEN_REST); // The party woke before the rest screen closed.
}

GAME_TEST(MM6Conditions, SpiderPoisonsAndRatDiseases) {
    // A spider's bite poisons and a rat's bite diseases, and the condition lowers the victim's might.
    startNewGameThroughInput(game);
    for (Character &character : pParty->pCharacters)
        character.health = 200; // The bites would knock a new character out before the conditions show.
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    const Vec3f side(-ahead.y, ahead.x, 0);
    for (auto [monster, offset] : {std::pair(160, 100.0f), std::pair(146, -100.0f)}) { // Spider, Large Rat.
        Actor *biter = game.spawnMonster(pParty->pos + ahead * 200 + side * offset, static_cast<MonsterId>(monster), SPAWN_STATIONARY);
        biter->monsterInfo.specialAttackLevel = 10; // Level 10 times 10 makes every bite try.
    }
    auto conditionTape = charTapes.conditions();
    auto mightTape = charTapes.custom([](const Character &character) { return character.GetActualMight(); });
    auto poisonedTape = tapes.custom([] { return std::ranges::any_of(pParty->pCharacters, [](const Character &character) { return character.conditions.has(CONDITION_POISON_WEAK); }); });
    auto diseasedTape = tapes.custom([] { return std::ranges::any_of(pParty->pCharacters, [](const Character &character) { return character.conditions.has(CONDITION_DISEASE_WEAK); }); });
    test.startTaping();

    game.tick();
    for (int frame = 0; frame < 300 && !(poisonedTape.back() && diseasedTape.back()); frame++)
        game.tick(); // The monsters walk up and bite, a bite can miss and a character can resist.

    EXPECT_EQ(poisonedTape.frontBack(), tape(false, true));
    EXPECT_EQ(diseasedTape.frontBack(), tape(false, true));
    for (int i = 0; i < 4; i++) {
        const Condition condition = conditionTape.back()[i];
        if (condition == CONDITION_POISON_WEAK || condition == CONDITION_DISEASE_WEAK)
            EXPECT_LT(mightTape.back()[i], mightTape.front()[i]) << i;
        else
            EXPECT_EQ(mightTape.back()[i], mightTape.front()[i]) << i;
    }
}

GAME_TEST(MM6OffscreenFrame, DISABLED_TurnBasedCapture) {
    // Hidden capture of the party's turn in turn-based mode, for checking how the MM6 turn indicator looks.
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);
    const char *capturePath = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePath, nullptr);
    ASSERT_TRUE(std::string_view(capturePath).starts_with("/tmp/") || std::string_view(capturePath).starts_with("/private/tmp/"));
    ASSERT_EQ(Path(capturePath).extension(), ".png");

    startNewGameThroughInput(game);
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    game.spawnMonster(pParty->pos + ahead * 400, static_cast<MonsterId>(76), SPAWN_DUMMY);
    game.pressAndReleaseKey(PlatformKey::KEY_RETURN);
    game.tick();
    while (!(pTurnEngine->turn_stage == TE_ATTACK && pTurnEngine->flags & TE_PLAYER_TURN))
        game.tick(); // The monsters finish their opening move.

    RgbaImage frame;
    game.runGameRoutine([&frame] {
        render->ClearTarget(Color(0, 0, 0));
        engine->drawWorld();
        engine->drawHUD();
        render->flushAndScale();
        engine->drawOverlay();
        frame = render->MakeFullScreenshot();
    });
    FileOutputStream output{Path(capturePath)};
    output.write(png::encode(frame));
    output.close();
}
