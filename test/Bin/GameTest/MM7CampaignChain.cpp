#include "MM7CampaignChain.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <numbers>
#include <span>
#include <string>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UISaveLoad.h"

std::pair<Vec3f, int> approachCampaignChainFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

void assertCampaignChainProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void loadCampaignChainSlot(EngineController &game, int chapter) {
    const std::string fileName = fmt::format("save{:03}.mm7", chapter);
    ASSERT_TRUE(ufs->exists("saves/" + fileName));
    game.pressGuiButton("MainMenu_LoadGame");
    auto loadWindowOpen = [] { return pGUIWindow_CurrentMenu && pGUIWindow_CurrentMenu->eWindowType == WINDOW_LOAD; };
    for (int i = 0; i < 10 && !loadWindowOpen(); i++)
        game.tick();
    ASSERT_TRUE(loadWindowOpen());
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find_if(slots, [&fileName](const SavegameSlot &entry) { return entry.fileName == fileName; });
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7); // The Load screen shows seven slots before it scrolls.
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick();
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, fileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, fileName);
}

void saveCampaignChainSlot(EngineController &game, int chapter) {
    const std::string path = fmt::format("saves/save{:03}.mm7", chapter);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.runGameRoutine([&] {
        saveGame(false, false, path, fmt::format("Campaign C{}", chapter));
    });
    ASSERT_TRUE(ufs->exists(path));
    const Blob saved = ufs->read(path);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}
