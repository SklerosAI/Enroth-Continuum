#include <memory>
#include <stdexcept>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

static void completeEditableMM6Party() {
    for (Character &character : pParty->pCharacters) {
        for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
            const Skill skill = character.GetSkillIdxByOrder(order);
            if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                character.pActiveSkills[skill] = CombinedSkillValue::novice();
        }
        if (character.GetSkillIdxByOrder(3) == SKILL_INVALID)
            throw std::runtime_error("MM6 creation party lacks four skills");
    }
    for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
        for (Character &character : pParty->pCharacters)
            for (Attribute stat : allStatAttributes())
                character.IncreaseAttribute(stat);
    if (CharacterCreation_GetUnspentAttributePointCount() != 0)
        throw std::runtime_error("MM6 creation party could not spend attribute points");
}

static void enterFirstMapFromNewGame(EngineController &game) {
    // Default party through the creation screen, then the first map in the real game loop. The test-only new-game
    // callback seeds the map cache from the loaded archives and writes no save.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([] {
        auto firstMap = mm6::prepareNewGameMM6FromLoadedMaps();
        pMapDeltas.swap(firstMap.session.staged.worldAndVisuals.world.mapDeltas);
        mm6::chestTitleState() = firstMap.session.staged.worldAndVisuals.chestTitle;
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    game.runGameRoutine(completeEditableMM6Party);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

GAME_TEST(MM6CalendarHeadless, DISABLED_NewGameCalendarShowsYear) {
    // A new MM6 game starts on the 1st of January 1165.
    // Run alone in a fresh MM6 headless process.
    auto statusTape = tapes.statusBar();
    auto textTape = tapes.allGUIWindowsText();
    auto screenTape = tapes.screen();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.moveMouse(517, 111); // The minimap.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_T);
    game.tick();

    EXPECT_CONTAINS(statusTape, "9:00am Monday 1 January 1165");
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_BOOKS));
    auto text = textTape.flatten();
    EXPECT_CONTAINS(text, "Day\t100:\t1101 - Monday");
    EXPECT_CONTAINS(text, "Month\t100:\t1101 - January");
    EXPECT_CONTAINS(text, "Year\t100:\t1101165");
}

GAME_TEST(MM6CalendarHeadless, DISABLED_NewPartyAges) {
    // A new MM6 party is 21 to 26 years old.
    // Run alone in a fresh MM6 headless process.
    auto textTape = tapes.allGUIWindowsText();
    auto screenTape = tapes.screen();
    auto activeTape = tapes.activeCharacterIndex();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressGuiButton("Game_Character1"); // The active character, opens the stats page.
    game.tick();
    for (PlatformKey key : {PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3, PlatformKey::KEY_DIGIT_4}) {
        game.pressAndReleaseKey(key);
        game.tick();
    }

    EXPECT_EQ(screenTape, tape(SCREEN_CHARACTERS));
    EXPECT_EQ(activeTape, tape(0, 1, 2, 3));
    auto ages = textTape.flatten().filter([](const std::string &line) { return line.starts_with("Age"); });
    EXPECT_EQ(ages, tape(std::string("Age\f65535\t11022\f00000 / 22\n"), std::string("Age\f65535\t11024\f00000 / 24\n"),
                         std::string("Age\f65535\t11022\f00000 / 22\n"), std::string("Age\f65535\t11026\f00000 / 26\n")));
}
