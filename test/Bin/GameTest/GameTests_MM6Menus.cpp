#include <cstdlib>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"

#include "GUI/GUIWindow.h"

#include "Library/Image/Png.h"

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

GAME_TEST(MM6GameMenu, ButtonsMatchArt) {
    // The escape menu had MM7's button layout over MM6's art, so Resume Game started a new game and Load Game quit.
    startNewGameThroughInput(game);
    auto screenTape = tapes.screen();
    test.startTaping();

    const Pointi resume(128, 180), controls(350, 180), load(350, 233), save(128, 286);
    for (Pointi button : {resume, controls, load, save}) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick(2); // The game stops on the first frame, the menu opens on the second.
        game.pressAndReleaseButton(BUTTON_LEFT, button);
        game.tick(2); // The menu takes the click on the second frame.
        if (button != resume) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick(2);
        }
    }

    EXPECT_EQ(screenTape, tape(SCREEN_MENU, SCREEN_GAME, SCREEN_MENU, SCREEN_OPTIONS, SCREEN_GAME, SCREEN_MENU, SCREEN_LOADGAME, SCREEN_GAME,
                               SCREEN_MENU, SCREEN_SAVEGAME, SCREEN_GAME));
}

GAME_TEST(MM6GameMenu, Controls) {
    // The Controls screen had MM7's options layout and its key binding and video buttons, which MM6 doesn't have.
    startNewGameThroughInput(game);
    auto screenTape = tapes.screen();
    auto quadsTape = tapes.hudQuads();
    auto soundTape = tapes.config(engine->config->settings.SoundLevel);
    auto musicTape = tapes.config(engine->config->settings.MusicLevel);
    auto turnTape = tapes.config(engine->config->settings.TurnSpeed);
    auto walkTape = tapes.config(engine->config->settings.WalkSound);
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2); // The game stops on the first frame, the menu opens on the second.
    game.pressAndReleaseButton(BUTTON_LEFT, 350, 180); // Controls.
    game.tick(2); // The menu takes the click on the second frame.
    game.pressAndReleaseButton(BUTTON_LEFT, 446, 169); // Sound volume right arrow.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, 278 + 16 * 3 + 8, 220); // Fourth notch of the music volume.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, 57, 270); // 16x turn rate.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, 133, 301); // Walksound.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, 358, 310); // Resume Game.
    game.tick(2);

    auto art = quadsTape.flatten();
    EXPECT_EQ(screenTape, tape(SCREEN_MENU, SCREEN_OPTIONS, SCREEN_GAME));
    EXPECT_EQ(soundTape, tape(1, 2));
    EXPECT_EQ(musicTape, tape(1, 3));
    EXPECT_EQ(turnTape, tape(0, 128));
    EXPECT_EQ(walkTape, tape(true, false));
    EXPECT_CONTAINS(art, (DrawnQuad{"controlbg", {8, 132}}));
    EXPECT_CONTAINS(art, (DrawnQuad{"con_high", {171, 212}}));
    EXPECT_CONTAINS(art, (DrawnQuad{"con_smoo", {171, 262}}));
    EXPECT_CONTAINS(art, (DrawnQuad{"con_16x", {21, 262}}));
    EXPECT_CONTAINS(art, (DrawnQuad{"con_x", {21, 293}})); // Walksound, before the click.
    EXPECT_CONTAINS(art, (DrawnQuad{"con_x", {21, 312}})); // Show Damage.
    EXPECT_CONTAINS(art, (DrawnQuad{"convol20", {294, 161}}));
    EXPECT_CONTAINS(art, (DrawnQuad{"con_arrr", {438, 161}}));
    EXPECT_CONTAINS(art, (DrawnQuad{"convol30", {310, 161}}));
    EXPECT_CONTAINS(art, (DrawnQuad{"convol40", {326, 212}}));
    EXPECT_CONTAINS(art, (DrawnQuad{"convol20", {294, 262}}));
    EXPECT_MISSES(art, (DrawnQuad{"con_16x", {22, 270}}));
}

GAME_TEST(MM6OffscreenFrame, DISABLED_MenusCapture) {
    // Hidden captures of the title screen with New Game hovered, the escape menu and its Controls screen. Each one goes
    // next to MM6_OFFSCREEN_CAPTURE_PATH with a suffix.
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);
    const char *capturePath = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePath, nullptr);
    const std::string_view destination(capturePath);
    ASSERT_TRUE(destination.starts_with("/tmp/") || destination.starts_with("/private/tmp/"));
    ASSERT_TRUE(destination.ends_with(".png"));

    auto capture = [&](std::string_view suffix, bool world) {
        RgbaImage frame;
        game.runGameRoutine([&frame, world] {
            render->ClearTarget(Color(0, 0, 0));
            if (world) {
                engine->drawWorld();
                engine->drawHUD();
            }
            GUI_UpdateWindows();
            render->flushAndScale();
            frame = render->MakeFullScreenshot();
        });
        FileOutputStream output{Path(fmt::format("{}-{}.png", destination.substr(0, destination.size() - 4), suffix))};
        output.write(png::encode(frame));
        output.close();
    };

    game.moveMouse(500, 30);
    game.tick(8);
    capture("title", false);
    startNewGameThroughInput(game);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    capture("escape", true);
    game.pressAndReleaseButton(BUTTON_LEFT, 350, 180); // Controls.
    game.tick(2);
    capture("controls", true);
}
