#include <algorithm>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

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

static void confirmTransition(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void enterD02(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-21456, -286, 193), 90); // In front of the d02 door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
}

GAME_TEST(MM6Dungeons, TempleBatsBiteTheParty) {
    // The Abandoned Temple's monsters never noticed the party. The bats of the entrance hall fly up and bite.
    startNewGameThroughInput(game);
    enterD02(game);
    auto hpTape = tapes.totalHp();
    auto huntersTape = tapes.custom([] {
        return std::ranges::count_if(pActors, [](const Actor &actor) { return actor.monsterInfo.hostilityType == HOSTILITY_LONG; });
    });
    test.startTaping();

    game.tick(100); // The bats fly up and bite.

    EXPECT_LT(hpTape.delta(), 0);
    EXPECT_GT(huntersTape.back(), 0);
}
