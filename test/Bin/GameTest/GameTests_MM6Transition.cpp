#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtInstruction.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Party.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Timer.h"

#include "GUI/GUIWindow.h"
#include "GUI/GUIButton.h"
#include "GUI/UI/UIGame.h"
#include "GUI/UI/UITransition.h"

#include "Utility/Memory/Blob.h"

extern GraphicsImage *transition_ui_icon;

// This test enters the first map through the existing scoped headless route, then runs
// an actual first-map event. It does not claim keyboard walking or Space input.
// DoPrepareWorld mutates global owners, so run this alone in a fresh process.
GAME_TEST(MM6Transition, DISABLED_FirstMapEntranceDialogAndGuard) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);

    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls()) {
        if (std::string_view(name).ends_with(".dlv") || std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    }
    ASSERT_TRUE(pMapDeltas.contains("oute3.ddm"));
    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(), 0x12345678u, {}));

    bool prepared = false;
    game.runGameRoutine([firstMap, &prepared] {
        GameUI_LoadPlayerPortraitsAndVoices();
        engine->_pendingTransition = MapDestination(firstMap, MAP_START_POINT_PARTY);
        {
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
        }
        prepared = !mm6::chestMapLoadOptedIn();
    });
    for (int tick = 0; tick < 256 && !prepared; ++tick)
        game.tick();
    ASSERT_TRUE(prepared);
    ASSERT_EQ(engine->_currentLoadedMapId, firstMap);
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);

    // A retained first-map entrance event must still point to its native
    // MM6 dialog picture and to a real destination map.
    constexpr int entranceEvent = 101;
    ASSERT_TRUE(engine->_localEventMap.hasEvent(entranceEvent));
    const auto &instructions = engine->_localEventMap.function(entranceEvent);
    int entranceMoveStep = -1;
    for (const EvtInstruction &instruction : instructions) {
        if (instruction.opcode == EVENT_MoveToMap && instruction.data.move_map_descr.exit_pic_id == 5 &&
            instruction.data.move_map_descr.house_id != HOUSE_INVALID)
            entranceMoveStep = instruction.step;
    }
    ASSERT_GE(entranceMoveStep, 0);

    const Vec3f originalPosition = pParty->pos;
    const int originalGold = pParty->uNumGold;
    const auto originalTime = pParty->GetPlayingTime();
    bool opened = false;
    game.runGameRoutine([&opened, entranceMoveStep] {
        current_screen_type = SCREEN_GAME;
        // The scoped harness has no walking/input loop. Begin at the native
        // MoveToMap instruction to isolate its transition-dialog dispatch.
        runEvent(EvtContext(EVT_SOURCE_MAP, entranceEvent), entranceMoveStep);
        opened = pDialogueWindow && pDialogueWindow->eWindowType == WINDOW_INDOOR_ENTRY_EXIT;
        if (opened) {
            auto *dialog = static_cast<GUIWindow_IndoorEntryExit *>(pDialogueWindow.get());
            dialog->Update();
        }
    });
    ASSERT_TRUE(opened);
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    ASSERT_NE(transition_ui_icon, nullptr);
    EXPECT_EQ(transition_ui_icon->name(), "istairdn");
    EXPECT_GT(transition_ui_icon->width(), 0);
    EXPECT_GT(transition_ui_icon->height(), 0);
    const auto hasButton = [](std::string_view name) {
        return std::ranges::any_of(pDialogueWindow->vButtons, [name](const GUIButton *button) {
            return button->id == name;
        });
    };
    EXPECT_TRUE(hasButton("Transition_No"));
    EXPECT_FALSE(hasButton("Transition_Yes"));
    ASSERT_NE(pBtn_ExitCancel, nullptr);
    ASSERT_FALSE(pBtn_ExitCancel->vTextures.empty());
    EXPECT_EQ(pBtn_ExitCancel->vTextures[0]->name(), "buttesc2");

    const MapDestination destination = static_cast<GUIWindow_IndoorEntryExit *>(pDialogueWindow.get())->destination();
    ASSERT_NE(destination.map(), MAP_INVALID);
    ASSERT_NE(destination.map(), firstMap);

    // The parent loop is still in party creation, so its normal message
    // dispatcher is unavailable. Close the event dialog with the same public
    // cleanup functions as the Cancel handler and assert no world mutation.
    game.runGameRoutine([] {
        DialogueEnding();
        back_to_game();
        current_screen_type = SCREEN_GAME;
    });
    EXPECT_EQ(pDialogueWindow, nullptr);
    EXPECT_EQ(engine->_currentLoadedMapId, firstMap);
    EXPECT_EQ(pParty->pos, originalPosition);
    EXPECT_EQ(pParty->uNumGold, originalGold);
    EXPECT_EQ(pParty->GetPlayingTime(), originalTime);
    EXPECT_FALSE(engine->_pendingTransition.has_value());

    // This checks the lower-level transition guard only. The live Accept
    // handler calls onMapLeave() first, so its refusal is not state-atomic.
    bool refused = false;
    game.runGameRoutine([&] {
        try {
            startMapTransition(destination);
        } catch (const std::runtime_error &) {
            refused = true;
        }
    });
    EXPECT_TRUE(refused);
    EXPECT_EQ(engine->_currentLoadedMapId, firstMap);
    EXPECT_FALSE(engine->_pendingTransition.has_value());
    EXPECT_EQ(pParty->pos, originalPosition);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
