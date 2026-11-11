#include <cstdlib>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

#include "Utility/String/Format.h"

#include "OffscreenCaptureForTest.h"

// A fresh-process, explicit macOS diagnostic. --offscreen-gl installs the
// hidden SDL OpenGL input/focus guard before GameStarter configures its window.
GAME_TEST(MM7OffscreenFrame, DISABLED_FirstOutdoorCapture) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    ASSERT_NE(window->nativeHandle(), nullptr);
    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);

    const char *capturePath = std::getenv("MM7_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePath, nullptr);
    ASSERT_TRUE(isOffscreenCapturePath(capturePath));

    game.startNewGame();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());

    const RgbaImage frame = drawOffscreenGameFrame(game);

    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());
    ASSERT_EQ(frame.size(), Sizei(640, 480));
    const Color first = frame[0][0];
    size_t changed = 0;
    size_t nonblackWorld = 0;
    size_t nonblackHud = 0;
    for (int y = 0; y < frame.height(); ++y) {
        for (int x = 0; x < frame.width(); ++x) {
            const Color pixel = frame[y][x];
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

    writeOffscreenCapture(capturePath, frame);
}

GAME_TEST(MM7OffscreenFrame, DISABLED_MapCapture) {
    // Progress capture of any MM7 map from its party start point, for enroth-capture. MM7_OFFSCREEN_CAPTURE_MAP is a
    // map file name or an index into the map table that wraps around, MM7_OFFSCREEN_CAPTURE_PATH the PNG to write.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    ASSERT_NE(window->nativeHandle(), nullptr);
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);

    const char *capturePath = std::getenv("MM7_OFFSCREEN_CAPTURE_PATH");
    const char *mapValue = std::getenv("MM7_OFFSCREEN_CAPTURE_MAP");
    ASSERT_NE(capturePath, nullptr);
    ASSERT_NE(mapValue, nullptr);
    ASSERT_TRUE(isOffscreenCapturePath(capturePath));

    std::vector<MapId> maps;
    for (MapId id : pMapTable->pInfos.indices())
        if (!pMapTable->pInfos[id].fileName.empty())
            maps.push_back(id);
    const MapId map = offscreenCaptureMap(mapValue, maps);
    ASSERT_NE(map, MAP_INVALID) << mapValue;
    fmt::print("offscreen capture map: {}\n", pMapTable->pInfos[map].fileName);

    game.startNewGame();
    if (map != engine->_currentLoadedMapId) {
        game.runGameRoutine([map] {
            engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_SKIP_NEXT_WORLD_UPDATE;
            uGameState = GAME_STATE_CHANGE_LOCATION;
            onMapLeave();
        });
        game.tick();
        game.skipLoadingScreen();
    }
    ASSERT_EQ(engine->_currentLoadedMapId, map);
    ASSERT_FALSE(window->isVisible());

    const RgbaImage frame = drawOffscreenGameFrame(game);
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(frame.size(), Sizei(640, 480));
    writeOffscreenCapture(capturePath, frame);
}
