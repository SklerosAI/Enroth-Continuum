#include <cstdlib>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ChestSeedAndOwnership.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"
#include "GUI/GUIWindow.h"

// Isolated lifecycle probe only. It starts from an unsaved creation Party and
// asks the normal world preparation path to enter a non-start outdoor map.
// The result does not admit a durable save or a campaign transition.
GAME_TEST(MM6V5LaterOutdoorHeadless, DISABLED_DirectSecondOutdoorLoadProbe) {
    const char *enable = std::getenv("ENROTH_MM6_V5_LATER_MAP_ENABLE");
    if (!enable || std::string_view(enable) != "strict-later-map-probe")
        GTEST_SKIP() << "Run only as an isolated headless MM6 map-load probe";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    constexpr std::string_view target = "oute2.odm";
    const MapId mapId = pMapTable->GetMapInfo(target);
    ASSERT_NE(mapId, MAP_INVALID);
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    pMapDeltas.clear();
    for (const std::string &name : maps.expectedNames())
        pMapDeltas.emplace(name, maps.readDefaultDelta(name));
    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(
        GameTitle::MM6, &mm6::chestTitleState(), 0x12345678u, {}));
    bool prepared = false;
    game.runGameRoutine([&] {
        engine->_pendingTransition = MapDestination(mapId, MAP_START_POINT_PARTY);
        mm6::ScopedChestMapLoadOptIn chestOptIn;
        DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
        prepared = true;
    });
    for (int tick = 0; tick < 256 && !prepared; ++tick)
        game.tick();
    EXPECT_TRUE(prepared);
    EXPECT_EQ(engine->_currentLoadedMapId, mapId);
    EXPECT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    EXPECT_FALSE(engine->_pendingTransition);
    EXPECT_FALSE(pOutdoor->mm6DeltaBytes.empty());
    EXPECT_EQ(vChests.size(), 20);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
