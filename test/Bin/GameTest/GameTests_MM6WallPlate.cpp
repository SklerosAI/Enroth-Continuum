#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
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

static void enterAbandonedTemple(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-21456, -286, 193), 90); // In front of the temple door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

GAME_TEST(MM6WallPlate, WalkingIntoATempleDoorOpensIt) {
    // Walking into a door in the Abandoned Temple left it shut, the pressure plate on the door never ran its event.
    startNewGameThroughInput(game);
    enterAbandonedTemple(game);
    game.teleportTo(pMapTable->GetMapInfo("d02.blv"), Vec3f(15900, -16001, 219), 180); // East of door 2, facing it.
    auto doorTape = tapes.doorStates({2, 3});
    auto pastDoorTape = tapes.custom([] { return pParty->pos.x < 15739; }); // The door's west face.
    test.startTaping();

    game.pressKey(PlatformKey::KEY_UP);
    game.tick(12); // The party runs into the door, it slides aside and the party walks through.
    game.releaseKey(PlatformKey::KEY_UP);

    EXPECT_EQ(doorTape, tape({DOOR_OPEN, DOOR_OPEN}, {DOOR_CLOSING, DOOR_OPEN})); // A door's initial state is its shut one.
    EXPECT_EQ(pastDoorTape, tape(false, true));
}
