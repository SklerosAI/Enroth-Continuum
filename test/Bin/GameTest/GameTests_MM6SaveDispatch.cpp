#include <stdexcept>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/SaveLoad.h"
#include "Engine/Timer.h"
#include "GUI/GUIWindow.h"

GAME_TEST(MM6SaveDispatch, RefusedManualAndQuickSaveLeaveLiveStateUntouched) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const auto quickCount = engine->config->gameplay.QuickSavesCount.value();
    const std::string quickName = getCurrentQuickSave();
    ASSERT_TRUE(quickName.ends_with(".ec6"));
    constexpr std::string_view manual = "saves/mm6-dispatch-refusal-probe.mm7";
    ASSERT_FALSE(ufs->exists(manual));
    const auto *menuBefore = pGUIWindow_CurrentMenu.get();
    const auto screenBefore = current_screen_type;
    const bool timerBefore = gameTimer->isPaused();

    game.runGameRoutine([] {
        quickSaveGame();
        doSavegame("mm6-dispatch-refusal-probe.mm7", "Refusal probe");
        quickLoadGame();
        EXPECT_THROW((void) saveGame(false, false,
                                     "saves/mm6-dispatch-refusal-probe.mm7",
                                     "Refusal probe"), std::runtime_error);
    });

    EXPECT_EQ(engine->config->gameplay.QuickSavesCount.value(), quickCount);
    EXPECT_EQ(getCurrentQuickSave(), quickName);
    EXPECT_FALSE(ufs->exists(manual));
    EXPECT_EQ(pGUIWindow_CurrentMenu.get(), menuBefore);
    EXPECT_EQ(current_screen_type, screenBefore);
    EXPECT_EQ(gameTimer->isPaused(), timerBefore);
}
