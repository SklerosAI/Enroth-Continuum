#include <algorithm>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/SaveLoad.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UISaveLoad.h"

#include "Library/FileSystem/Interface/FileSystem.h"

#include "Utility/String/Format.h"

namespace {
constexpr std::string_view slot = "saves/save000.mm7";
constexpr std::string_view fileName = "save000.mm7";
constexpr int savedGold = 12345;
}

// These two tests require --mm7-disposable-profile and exact filters. The
// profile is a marked owner-only temp directory, persisted between processes.
GAME_TEST(MM7DisposableSave, DISABLED_WriteFreshProfile) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_FALSE(ufs->exists(slot));

    game.startNewGame();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.runGameRoutine([] {
        pParty->uNumGold = savedGold;
        saveGame(false, false, slot, "Disposable MM7 save gate");
    });

    ASSERT_TRUE(ufs->exists(slot));
    EXPECT_GT(ufs->read(slot).size(), 1024u);
    EXPECT_EQ(pParty->uNumGold, savedGold);
}

GAME_TEST(MM7DisposableSave, DISABLED_ReloadFreshProcess) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_TRUE(ufs->exists(slot));
    const size_t savedSize = ufs->read(slot).size();
    ASSERT_GT(savedSize, 1024u);

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::find_if(slots.begin(), slots.end(), [](const SavegameSlot &entry) {
        return entry.fileName == fileName;
    });
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick(2);
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, fileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);

    EXPECT_EQ(current_screen_type, SCREEN_GAME);
    EXPECT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    EXPECT_EQ(engine->_lastLoadedSaveFileName, fileName);
    EXPECT_EQ(pParty->uNumGold, savedGold);
    EXPECT_EQ(ufs->read(slot).size(), savedSize);
}
