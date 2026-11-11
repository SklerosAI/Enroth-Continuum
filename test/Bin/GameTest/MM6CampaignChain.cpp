#include "MM6CampaignChain.h"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/Tables/MapTable.h"
#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UISaveLoad.h"
#include "Library/Platform/Interface/PlatformEnumFunctions.h"

static std::filesystem::path profileRoot;

static std::string slotFileName(int chapter) {
    return fmt::format("save{:03}.ec6", chapter);
}

static void tickUntil(EngineController &game, const std::function<bool()> &condition) {
    for (int i = 0; i < 10 && !condition(); i++)
        game.tick();
}

static void clickArrow(EngineController &game, UIMessageType arrow, int times) {
    auto button = std::ranges::find(pGUIWindow_CurrentMenu->vButtons, arrow, &GUIButton::msg);
    ASSERT_NE(button, pGUIWindow_CurrentMenu->vButtons.end());
    for (int i = 0; i < times; i++) {
        game.pressAndReleaseButton(BUTTON_LEFT, (*button)->rect.center());
        game.tick();
    }
}

void setMM6CampaignChainProfile(const std::filesystem::path &root) {
    mm6::testing::setNativeProfileRootForTest(root);
    profileRoot = root;
}

const std::filesystem::path &mm6CampaignChainProfile() {
    return profileRoot;
}

void assertMM6CampaignChainProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_FALSE(profileRoot.empty());
    ASSERT_EQ(profileRoot, std::filesystem::canonical(profileRoot));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void loadMM6CampaignChainSlot(EngineController &game, int chapter) {
    const std::string fileName = slotFileName(chapter);
    ASSERT_TRUE(std::filesystem::is_regular_file(profileRoot / "saves" / fileName));
    game.pressGuiButton("MainMenu_LoadGame");
    tickUntil(game, [] { return pGUIWindow_CurrentMenu && pGUIWindow_CurrentMenu->eWindowType == WINDOW_LOAD; });
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find(slots, fileName, &SavegameSlot::fileName);
    ASSERT_NE(found, slots.end());
    const int index = found - slots.begin();
    const int scroll = std::max(0, index - 6); // The Load screen shows seven slots.
    clickArrow(game, UIMSG_DownArrow, scroll);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index - scroll));
    game.tick();
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, fileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, fileName);
    ASSERT_TRUE(mm6::isManualSaveArmed());
}

void saveMM6CampaignChainSlot(EngineController &game, int chapter) {
    const std::string fileName = slotFileName(chapter);
    const std::string name = fmt::format("c{}", chapter);
    for (int earlier = 0; earlier < chapter; earlier++)
        ASSERT_TRUE(std::filesystem::is_regular_file(profileRoot / "saves" / slotFileName(earlier)));
    ASSERT_FALSE(std::filesystem::exists(profileRoot / "saves" / fileName));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    tickUntil(game, [] { return current_screen_type == SCREEN_MENU; });
    ASSERT_EQ(current_screen_type, SCREEN_MENU);
    game.pressGuiButton("GameMenu_SaveGame");
    tickUntil(game, [] { return current_screen_type == SCREEN_SAVEGAME; });
    ASSERT_EQ(current_screen_type, SCREEN_SAVEGAME);
    const auto &slots = saveLoadMenu()->slots();
    ASSERT_EQ(std::ssize(slots), chapter + 1); // The new save slot, then the slots of the earlier chapters.
    ASSERT_TRUE(slots.front().fileName.empty());
    const int selected = std::ranges::find(slots, saveLoadMenu()->selectedSlot().fileName, &SavegameSlot::fileName) - slots.begin();
    clickArrow(game, UIMSG_ArrowUp, std::min(selected, std::max(0, static_cast<int>(slots.size()) - 7))); // The menu scrolled to the loaded slot.
    game.doubleClickGuiButton("SaveMenu_Slot0");
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();
    game.pressAndReleaseKey(platformKeyForDigit(chapter));
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    tickUntil(game, [] { return current_screen_type == SCREEN_GAME; });
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    ASSERT_TRUE(std::filesystem::is_regular_file(profileRoot / "saves" / fileName));
    const std::string mapFileName = pMapTable->pInfos[engine->_currentLoadedMapId].fileName;
    game.runGameRoutine([&] {
        const std::vector<mm6::SaveSlotInfo> listed = mm6::listFirstMapManualSlots();
        const auto slot = std::ranges::find(listed, fileName, &mm6::SaveSlotInfo::fileName);
        ASSERT_NE(slot, listed.end());
        EXPECT_EQ(slot->header.displayName, name);
        EXPECT_EQ(slot->header.mapFileName, mapFileName);
        EXPECT_NO_THROW((void) mm6::readFirstMapManualSlot(fileName));
    });
}
