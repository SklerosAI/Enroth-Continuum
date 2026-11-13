#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"

#include "GUI/GUIWindow.h"

static bool isMainMenuButtonArt(const DrawnQuad &quad) {
    return quad.texture.starts_with("start") || quad.texture.starts_with("mm");
}

GAME_TEST(MM6MainMenu, ButtonArtAndMusic) {
    // The title screen played MM7's track, showed a still hover image with hit boxes as big as the art and drew the
    // hover image as the pressed one.
    auto quadsTape = tapes.hudQuads();
    auto musicTape = tapes.music();
    test.startTaping();

    game.pressAndReleaseButton(BUTTON_LEFT, 500, 90); // Load Game.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2); // The load screen closes on the first frame, the main menu opens on the second.
    ASSERT_EQ(GetCurrentMenuID(), MENU_MAIN);

    game.moveMouse(614, 30); // Under the right edge of the New Game art, outside its hit box.
    game.tick();
    game.moveMouse(500, 30);
    game.tick(10); // The hovered button animates to its last frame and turns back.
    game.pressAndReleaseButton(BUTTON_LEFT, 500, 30);
    game.tick();

    auto art = quadsTape.flatten().filter(isMainMenuButtonArt);
    EXPECT_EQ(musicTape.flatten(), tape(MusicId(13)));
    EXPECT_EQ(art, tape(DrawnQuad{"start00b", {482, 71}}, DrawnQuad{"mmloa1", {482, 71}}, DrawnQuad{"start00b", {482, 71}},
                        DrawnQuad{"start00a", {482, 9}}, DrawnQuad{"start01a", {482, 9}}, DrawnQuad{"start02a", {482, 9}},
                        DrawnQuad{"start03a", {482, 9}}, DrawnQuad{"start04a", {482, 9}}, DrawnQuad{"start05a", {482, 9}},
                        DrawnQuad{"start06a", {482, 9}}, DrawnQuad{"start05a", {482, 9}}, DrawnQuad{"mmnew1", {482, 9}}));
}
