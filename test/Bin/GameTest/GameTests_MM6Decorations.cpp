#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

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

GAME_TEST(MM6Decorations, HoverTreeShowsItsHint) {
    // Hovering a tree on oute3 crashed. Trees were taken for MM7's interactive decorations, and this one came after
    // the 124 that get an event variable.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_EQ(engine->_currentLoadedMapId, oute3);
    const LevelDecoration &tree = pLevelDecorations[458];
    const std::string &hint = pDecorationTable->decoration(tree.uDecorationDescID)->hint;
    ASSERT_FALSE(hint.empty());
    auto statusTape = tapes.statusBar();
    test.startTaping();

    game.teleportTo(oute3, tree.vPosition + Vec3f(600, 0, 0), 180);
    game.pointMouseAtDecoration(458);

    EXPECT_EQ(statusTape.back(), hint);
}

GAME_TEST(MM6Decorations, BarrelNamesAndRunsItsGlobalEvent) {
    // MM6 barrels showed their plain name on hover and did nothing on click.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const LevelDecoration &barrel = pLevelDecorations[400];
    auto statusTape = tapes.statusBar();
    auto mightTape = charTapes.custom([](const Character &character) { return character._stats[ATTRIBUTE_MIGHT]; });
    test.startTaping();

    game.teleportTo(oute3, barrel.vPosition - Vec3f(300, 0, 0), 0);
    game.pointMouseAtDecoration(400);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick(22); // The barrel's message shows for two seconds, then the hover text comes back a frame later.

    EXPECT_EQ(mightTape.delta(), tape(1, 0, 0, 0));
    EXPECT_CONTAINS(statusTape, pNPCTopics[411].pTopic);
    EXPECT_EQ(statusTape.back(), pNPCTopics[410].pTopic);
}
