#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

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

static int weakCharacters() {
    int result = 0;
    for (const Character &character : pParty->pCharacters)
        result += character.conditions.has(CONDITION_WEAK);
    return result;
}

static void confirmTransition(EngineController &game) {
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

GAME_TEST(MM6MapTravel, WalkOffEdgeToNeighbour) {
    // Walking off an MM6 outdoor map only pushed the party back, no neighbouring map was ever offered.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto timeTape = tapes.custom([] { return pParty->GetPlayingTime().ticks(); });
    auto foodTape = tapes.food();
    auto weakTape = tapes.custom([] { return weakCharacters(); });
    game.teleportTo(oute3, Vec3f(-22450, -10000, 100), 180); // Next to the west edge, facing it.
    test.startTaping();

    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    ASSERT_EQ(screenTape.back(), SCREEN_CHANGE_LOCATION);
    confirmTransition(game);

    EXPECT_EQ(mapTape, tape(oute3, outd3));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_CONTAINS(positionTape, Vec3f(-22606, -10000, 110)); // Past the edge when the dialogue opened.
    EXPECT_CONTAINS(positionTape, Vec3f(22450, -10000, 164)); // One map width east, on the terrain of the new map.
    EXPECT_CONTAINS(timeTape.adjacentDeltas(), Duration::fromDays(5).ticks()); // The trip, in one frame.
    EXPECT_EQ(foodTape, tape(7, 2));
    EXPECT_EQ(weakTape, tape(0));
}
