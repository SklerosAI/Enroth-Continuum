#include <algorithm>
#include <cstdlib>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Graphics/Sprites.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Objects/Chest.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIGame.h"

#include "Library/Image/Png.h"

#include "Utility/Memory/Blob.h"
#include "Utility/Streams/FileOutputStream.h"
#include "Utility/System/Path.h"

GAME_TEST(MM6SpriteOctants, SevenCharacterDirectionalPrefix) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const int frameId = pSpriteFrameTable->FastFindSprite("pfemsta");
    ASSERT_GT(frameId, 0);
    SpriteFrame *frame = pSpriteFrameTable->GetFrame(frameId, 0_ticks);
    ASSERT_NE(frame, nullptr);
    for (int octant = 0; octant < 8; ++octant)
        EXPECT_NE(frame->sprites[octant], nullptr) << "octant " << octant;
}

// Explicit, fresh-process, macOS-only capture. --offscreen-gl installs the
// hidden SDL OpenGL input/focus guard before GameStarter configures its window.
GAME_TEST(MM6OffscreenFrame, DISABLED_FirstOutdoorCapture) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(window->nativeHandle(), nullptr);
    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);

    const char *capturePath = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePath, nullptr);
    const std::string_view destination(capturePath);
    ASSERT_TRUE(destination.starts_with("/tmp/") || destination.starts_with("/private/tmp/"));
    ASSERT_TRUE(Path(capturePath).isNormalized());
    ASSERT_EQ(Path(capturePath).extension(), ".png");

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    ASSERT_EQ(lWindowList.size(), 2u);

    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls()) {
        if (std::string_view(name).ends_with(".dlv") || std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    }
    ASSERT_TRUE(pMapDeltas.contains("oute3.ddm"));
    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(),
                                            0x12345678u, {}));

    bool loaded = false;
    game.runGameRoutine([firstMap, &loaded] {
        GameUI_LoadPlayerPortraitsAndVoices();
        engine->_pendingTransition = MapDestination(firstMap, MAP_START_POINT_PARTY);
        {
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
        }
        loaded = !mm6::chestMapLoadOptedIn();
    });
    for (int tick = 0; tick < 256 && !loaded; ++tick)
        game.tick();
    ASSERT_TRUE(loaded);
    ASSERT_EQ(engine->_currentLoadedMapId, firstMap);
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    ASSERT_EQ(vChests.size(), 20u);
    ASSERT_EQ(lWindowList.size(), 1u);
    ASSERT_FALSE(window->isVisible());

    RgbaImage frame;
    game.runGameRoutine([&frame] {
        uGameState = GAME_STATE_PLAYING;
        current_screen_type = SCREEN_GAME;
        render->ClearTarget(Color(0, 0, 0));
        engine->drawWorld();
        engine->drawHUD();
        render->flushAndScale();
        engine->drawOverlay();
        frame = render->MakeFullScreenshot(); // Read before swap; no display presentation.
    });

    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());
    ASSERT_EQ(frame.size(), Sizei(640, 480));
    const Color first = frame[0][0];
    size_t changed = 0;
    size_t nonblackWorld = 0;
    size_t nonblackHud = 0;
    for (int y = 0; y < frame.height(); ++y) {
        for (int x = 0; x < frame.width(); ++x) {
            Color pixel = frame[y][x];
            changed += pixel != first;
            if (pixel.r || pixel.g || pixel.b) {
                if (x < 460 && y < 350)
                    ++nonblackWorld;
                if (y >= 350)
                    ++nonblackHud;
            }
        }
    }
    EXPECT_GT(changed, 10000u);
    EXPECT_GT(nonblackWorld, 10000u);
    EXPECT_GT(nonblackHud, 10000u);
    // Before the normal 2D flush, these four inner face regions contain only
    // the monochrome frame backing even though portrait quads were queued.
    for (int center : {50, 163, 276, 388}) {
        size_t chromaticPixels = 0;
        for (int y = 405; y < 445; ++y) {
            for (int x = center - 12; x < center + 13; ++x) {
                const Color pixel = frame[y][x];
                const int spread = std::max({pixel.r, pixel.g, pixel.b}) -
                                   std::min({pixel.r, pixel.g, pixel.b});
                chromaticPixels += spread > 30;
            }
        }
        EXPECT_GT(chromaticPixels, 250u) << "portrait center " << center;
    }

    FileOutputStream output{Path(capturePath)};
    output.write(png::encode(frame));
    output.close();
}
