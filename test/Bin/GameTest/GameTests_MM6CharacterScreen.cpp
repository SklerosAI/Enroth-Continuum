#include <array>
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
#include "Engine/Resources/ResourceManager.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIButton.h"
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

static void clickCharacterScreenButton(EngineController &game, GUIButton *button) {
    game.pressAndReleaseButton(BUTTON_LEFT, button->rect.topLeft() + Pointi(56, 16)); // The right side of the proposed button rectangle.
    game.tick();
}

GAME_TEST(MM6CharacterScreenHeadless, ButtonArtExistsInMM6Archive) {
    for (std::string_view name : std::array<std::string_view, 14>{
             "buttsta1", "buttsta2", "stat_hot", "buttski1", "buttski2", "skil_hot",
             "buttinv1", "buttinv2", "equi_hot", "buttawa1", "buttawa2", "awar_hot",
             "buttexi1", "buttexi2"}) {
        SCOPED_TRACE(name);
        Sizei size = engine->resources()->iconSize(name);
        EXPECT_GT(size.w, 0);
        EXPECT_GT(size.h, 0);
    }
}

GAME_TEST(MM6CharacterScreenHeadless, TabsOpenByKeyAndButton) {
    // The character screen buttons had MM7 positions and placeholder-sized hit boxes, so clicking the right end of an MM6
    // button picture did nothing.
    // Run alone in a fresh MM6 headless process.
    auto screenTape = tapes.screen();
    auto tabTape = tapes.custom([] { return current_character_screen_window; });
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_I);
    game.tick(2); // The key queues the character screen for the next frame.
    for (PlatformKey key : {PlatformKey::KEY_C, PlatformKey::KEY_S, PlatformKey::KEY_A, PlatformKey::KEY_I}) {
        game.pressAndReleaseKey(key);
        game.tick();
    }
    for (GUIButton *button : {pCharacterScreen_StatsBtn, pCharacterScreen_SkillsBtn, pCharacterScreen_AwardsBtn, pCharacterScreen_InventoryBtn})
        clickCharacterScreenButton(game, button);
    clickCharacterScreenButton(game, pCharacterScreen_ExitBtn);
    game.tick(); // The exit button press draws its pressed picture, then closes the screen on the next frame.

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHARACTERS, SCREEN_GAME));
    EXPECT_EQ(tabTape, tape(WINDOW_CHARACTER_STATS, WINDOW_CHARACTER_INVENTORY,
                            WINDOW_CHARACTER_STATS, WINDOW_CHARACTER_SKILLS, WINDOW_CHARACTER_AWARDS, WINDOW_CHARACTER_INVENTORY,
                            WINDOW_CHARACTER_STATS, WINDOW_CHARACTER_SKILLS, WINDOW_CHARACTER_AWARDS, WINDOW_CHARACTER_INVENTORY));
}

GAME_TEST(MM6CharacterScreenHeadless, TabKeyRightAfterInventoryKey) {
    // Pressing C on the frame after I, before the character screen opened, crashed on a freed tab button.
    // Run alone in a fresh MM6 headless process.
    auto screenTape = tapes.screen();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_I);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_SPELL_BOOK));
}
