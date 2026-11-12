#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Viewport.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/AutonoteTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/QuestTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/Books/AutonotesBook.h"

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

static int firstAutonote(AutonoteType type) {
    for (int i = 0; i < pAutonoteTxt.size(); ++i)
        if (pAutonoteTxt[i].eType == type && !pAutonoteTxt[i].pText.empty())
            return i;
    return -1;
}

static std::vector<bool> framesShowing(const TestMultiTape<std::string> &textTape, const std::string &text) {
    std::vector<bool> result;
    for (const auto &frame : textTape) {
        bool shown = frame.contains(text);
        if (result.empty() || result.back() != shown)
            result.push_back(shown);
    }
    return result;
}

GAME_TEST(MM6BooksHeadless, BookFrameHasBorders) {
    // MM6 books were drawn without the side borders of the book frame.
    auto texturesTape = tapes.hudTextures();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_Q);
    game.tick();

    EXPECT_TRUE(texturesTape.back().containsAll("book", "tabexit", "border5", "border6"));
}

GAME_TEST(MM6BooksHeadless, QuestPageTabsTurnTheRightWay) {
    // The top quest book tab turned back a page and the bottom one forward, MM6 has them the other way round.
    auto textTape = tapes.allGUIWindowsText();
    auto texturesTape = tapes.hudTextures();
    enterFirstMapFromNewGame(game);
    std::string firstQuest;
    game.runGameRoutine([&] {
        for (QuestBit bit : pQuestTable.indices()) {
            if (pQuestTable[bit].empty())
                continue;
            if (firstQuest.empty())
                firstQuest = pQuestTable[bit];
            pParty->_questBits.set(bit); // Every quest, so that the book has more than one page.
        }
    });
    ASSERT_FALSE(firstQuest.empty());
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_Q);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {440, 30}); // The top tab.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {440, 65}); // The bottom tab.
    game.tick();

    EXPECT_EQ(framesShowing(textTape, firstQuest), std::vector<bool>({true, false, true}));
    EXPECT_TRUE(texturesTape.front().containsAll("quest_bg", "tab+off", "tab--on")); // Only the top tab can turn the first page.
}

GAME_TEST(MM6BooksHeadless, AutonotesTabsPickTheirCategory) {
    // The MM6 autonotes book drew MM7 tabs that MM6 doesn't have.
    auto textTape = tapes.allGUIWindowsText();
    auto texturesTape = tapes.hudTextures();
    enterFirstMapFromNewGame(game);
    const int potionNote = firstAutonote(AUTONOTE_POTION_RECIPE);
    const int fountainNote = firstAutonote(AUTONOTE_STAT_HINT);
    ASSERT_NE(potionNote, -1);
    ASSERT_NE(fountainNote, -1);
    game.runGameRoutine([&] {
        pParty->_autonoteBits.set(potionNote);
        pParty->_autonoteBits.set(fountainNote);
        autonoteBookDisplayType = AUTONOTE_POTION_RECIPE;
    });
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_N);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {440, 170}); // The second category tab.
    game.tick();

    EXPECT_EQ(framesShowing(textTape, pAutonoteTxt[potionNote].pText), std::vector<bool>({true, false}));
    EXPECT_EQ(framesShowing(textTape, pAutonoteTxt[fountainNote].pText), std::vector<bool>({false, true}));
    EXPECT_TRUE(texturesTape.front().containsAll("note_bg", "anot1on", "anot2off", "anot3off", "anot4off", "anot5off"));
    EXPECT_CONTAINS(texturesTape.back(), "anot2on");
}

GAME_TEST(MM6BooksHeadless, MapWestAndEastTabsScroll) {
    // The MM6 map book had its West and East tabs swapped.
    auto xTape = tapes.custom([] { return viewparams->sViewCenterX; });
    auto yTape = tapes.custom([] { return viewparams->sViewCenterY; });
    auto texturesTape = tapes.hudTextures();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_M);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {440, 30}); // Zoom in, the whole map fits the view before.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {440, 205}); // The fifth tab.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {440, 240}); // The sixth tab.
    game.tick();

    ASSERT_EQ(xTape.size(), 4);
    EXPECT_LT(xTape[2], xTape[1]);
    EXPECT_EQ(xTape[3], xTape[1]);
    EXPECT_EQ(yTape.size(), 2); // Only the zoom recenters vertically.
    EXPECT_TRUE(texturesTape.front().containsAll("mapbordr", "zoom+off", "zoom-on", "tabnoff", "tabsoff", "tabwoff", "tabeoff"));
    EXPECT_CONTAINS(texturesTape[2], "tabwon");
    EXPECT_CONTAINS(texturesTape[3], "tabeon");
}

GAME_TEST(MM6BooksHeadless, CalendarShowsTimeInEnrothAndMoon) {
    // The MM6 calendar was titled "Running" and drew no moon.
    auto textTape = tapes.allGUIWindowsText();
    auto texturesTape = tapes.hudTextures();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_T);
    game.tick();

    EXPECT_CONTAINS(textTape.back(), "Time in Enroth");
    EXPECT_MISSES(textTape.back(), "Running");
    EXPECT_TRUE(texturesTape.back().containsAll("time_bg", "moon_new")); // A new game starts on the 1st, a new moon.
}
