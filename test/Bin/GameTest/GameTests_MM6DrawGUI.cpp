#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Objects/Chest.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIGame.h"

#include "Utility/Memory/Blob.h"

// Explicit, fresh-process probe. DoPrepareWorld and DrawGUI mutate global
// owners that a later GameTest cannot safely reuse in the same process.
GAME_TEST(MM6DrawGUIHeadless, DISABLED_ScopedFirstOutdoorFrame) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);

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
        // Game::gameLoop loads portraits before DoPrepareWorld; this scoped
        // callback enters below that outer loop and supplies the same owner.
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

    bool drawn = false;
    game.runGameRoutine([&drawn] {
        render->BeginScene2D();
        engine->DrawGUI();
        drawn = true;
    });
    EXPECT_TRUE(drawn);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
