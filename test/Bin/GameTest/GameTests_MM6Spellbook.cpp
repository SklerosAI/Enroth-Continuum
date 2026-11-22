#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6PlayerSpell.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Spells/SpellEnumFunctions.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/mm7_data.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UISpellbook.h"

#include "Library/Color/ColorTable.h"
#include "Library/Image/Png.h"

#include "Utility/Streams/FileOutputStream.h"
#include "Utility/System/Path.h"

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

static constexpr Pointi MM6_CURE_WEAKNESS_PICTURE = {221, 56};
static constexpr Pointi MM6_FLAME_ARROW_PICTURE = {350, 60};
static constexpr Pointi MM6_FIRE_TAB = {430, 30};
static constexpr Pointi MM6_STATIC_CHARGE_PICTURE = {360, 60};
static constexpr Pointi MM6_AIR_TAB = {430, 65};

static PlatformKey characterKey(int characterIndex) {
    return static_cast<PlatformKey>(std::to_underlying(PlatformKey::KEY_DIGIT_1) + characterIndex);
}

static void writeOffscreenFrame(EngineController &game, std::string_view name) {
    RgbaImage frame;
    game.runGameRoutine([&frame] {
        render->ClearTarget(Color(0, 0, 0));
        engine->drawWorld();
        engine->drawHUD();
        render->flushAndScale();
        engine->drawOverlay();
        frame = render->MakeFullScreenshot();
    });
    FileOutputStream output{Path(fmt::format("{}-{}.png", std::getenv("MM6_OFFSCREEN_CAPTURE_PATH"), name))};
    output.write(png::encode(frame));
    output.close();
}

static void requireOffscreenCapture() {
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);
    const char *capturePrefix = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePrefix, nullptr);
    ASSERT_TRUE(std::string_view(capturePrefix).starts_with("/private/tmp/"));
}

static std::string popupCost(const AccessibleVector<DrawnLine> &frame) {
    auto line = std::ranges::find(frame, std::string("SP Cost"), &DrawnLine::text);
    return line != frame.end() && std::next(line) != frame.end() ? std::next(line)->text : std::string();
}

GAME_TEST(MM6Spellbook, SpellPictureSelectsThenCasts) {
    // The MM6 spellbook put its spells in MM7 cells, so clicking the Cure Weakness picture missed it.
    auto screenTape = tapes.screen();
    auto selectedTape = tapes.custom([] { return spellbookSelectedSpell; });
    auto mpTape = charTapes.mp(1);
    auto conditionTape = charTapes.condition(2);
    enterFirstMapFromNewGame(game);
    pParty->pCharacters[2].SetCondition(CONDITION_WEAK, false);
    test.startTaping();

    game.pressAndReleaseKey(characterKey(1)); // The Cleric, the only Body caster.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, MM6_CURE_WEAKNESS_PICTURE);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, MM6_CURE_WEAKNESS_PICTURE);
    game.tick(2); // The book closes, the target picker opens on the next frame.
    game.pressAndReleaseKey(characterKey(2));
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_SPELL_BOOK, SCREEN_GAME));
    EXPECT_EQ(selectedTape, tape(SPELL_NONE, SPELL_BODY_CURE_WEAKNESS));
    EXPECT_EQ(mpTape.size(), 2);
    EXPECT_LT(mpTape.back(), mpTape.front());
    EXPECT_EQ(conditionTape, tape(CONDITION_WEAK, CONDITION_GOOD));
}

GAME_TEST(MM6Spellbook, SchoolTabPicture) {
    // The MM6 school tabs had MM7 hit boxes that stopped short of the right edge of the tab pictures.
    auto pageTape = charTapes.custom(2, [](const Character &character) { return character.lastOpenedSpellbookPage; });
    auto screenTape = tapes.screen();
    enterFirstMapFromNewGame(game);
    pParty->pCharacters[2].pActiveSkills[SKILL_AIR] = CombinedSkillValue::novice();
    test.startTaping();

    game.pressAndReleaseKey(characterKey(2)); // The Sorcerer, a Fire caster.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {455, 60}); // The right edge of the Air tab picture.
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_SPELL_BOOK));
    EXPECT_EQ(pageTape, tape(MAGIC_SCHOOL_FIRE, MAGIC_SCHOOL_AIR));
}

GAME_TEST(MM6Spellbook, QuickSpellTabAndHeaderPicture) {
    // The MM6 spellbook had no TABSPELL or header picture buttons, the quick spell could only be set from MM7's button
    // below the book.
    auto screenTape = tapes.screen();
    auto quickSpellTape = charTapes.quickSpell(1);
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseKey(characterKey(1)); // The Cleric.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, MM6_CURE_WEAKNESS_PICTURE);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {328, 340}); // TABSPELL sets the quick spell and closes the book.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {110, 58}); // The Body header picture removes the quick spell.
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_SPELL_BOOK, SCREEN_GAME, SCREEN_SPELL_BOOK));
    EXPECT_EQ(quickSpellTape, tape(SPELL_NONE, SPELL_BODY_CURE_WEAKNESS, SPELL_NONE));
}

GAME_TEST(MM6Spellbook, KnightClosesBookWithExitTab) {
    // A Knight's MM6 spellbook had no page and no TABEXIT button.
    auto screenTape = tapes.screen();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_C); // The Knight is active after creation.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {387, 340}); // TABEXIT.
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_SPELL_BOOK, SCREEN_GAME));
}

GAME_TEST(MM6Spellbook, SpellPopupHasThreeMasteries) {
    // The spell popup drew a fourth mastery line, "Get" with the Master text again, and left out the spell's picture. MM6
    // has Normal, Expert and Master.
    auto textTape = tapes.drawnTexts();
    auto lineTape = tapes.titleLines();
    auto quadTape = tapes.hudQuads();
    enterFirstMapFromNewGame(game);
    pParty->pCharacters[2].bHaveSpell[mm6::SPELL_FLAME_ARROW] = true; // The Sorcerer, a Normal Fire caster.
    pParty->pCharacters[3].pActiveSkills[SKILL_FIRE] = CombinedSkillValue(4, MASTERY_EXPERT); // The Druid.
    pParty->pCharacters[3].bHaveSpell[mm6::SPELL_FLAME_ARROW] = true;
    test.startTaping();

    for (int characterIndex : {2, 3}) {
        game.pressAndReleaseKey(characterKey(characterIndex));
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_C);
        game.tick();
        game.pressAndReleaseButton(BUTTON_LEFT, MM6_FIRE_TAB);
        game.tick();
        game.pressButton(BUTTON_RIGHT, MM6_FLAME_ARROW_PICTURE);
        game.tick();
        game.releaseButton(BUTTON_RIGHT);
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }

    auto popupTexts = textTape.flatten().map([](const DrawnText &text) { return text.text; }).filter([](const std::string &text) {
        return text.starts_with("Creates and fires");
    });
    EXPECT_EQ(popupTexts.unique(), tape(std::string(
        "Creates and fires a single flaming arrow.  Unlike most spells, this spell is notoriously inaccurate and has the same chance of hitting "
        "as a regular arrow (using your Fire skill as a bonus to hit).  The arrow does 1-8 points of damage when it hits.\n\n"
        "Normal\t048:\t055Casting cost 2 spell points\t000\n"
        "Expert\t048:\t055Casting cost 1spell point.  Quicker recovery rate\t000\n"
        "Master\t048:\t055Casting cost 0 spell points.  Quickest recovery rate")));
    EXPECT_EQ(lineTape.map(popupCost).unique(), tape(std::string(), std::string("2"), std::string(), std::string("1"), std::string()));
    EXPECT_CONTAINS(lineTape.flatten(), (DrawnLine{"SP Cost", {123, 240}, colorTable.White}));
    EXPECT_CONTAINS(quadTape.flatten(), (DrawnQuad{"cornr_ur", {517, 90}}));
    EXPECT_CONTAINS(quadTape.flatten(), (DrawnQuad{"fire002", {107, 107}}));
}

GAME_TEST(MM6Spellbook, SpellPopupCostIsSpellsTxts) {
    // Without fix_spells_cost the spell popup showed the cost MM6.exe charges, 2 for Expert Static Charge. MM6.exe's popup
    // shows spells.txt's 1.
    for (bool fix : {true, false}) {
        SCOPED_TRACE(fmt::format("fix={}", fix));
        test.prepareForNextTest();
        engine->config->gameplay.FixSpellsCost.setValue(fix);
        auto lineTape = tapes.titleLines();
        enterFirstMapFromNewGame(game);
        pParty->pCharacters[2].pActiveSkills[SKILL_AIR] = CombinedSkillValue(4, MASTERY_EXPERT); // The Sorcerer.
        pParty->pCharacters[2].bHaveSpell[mm6::SPELL_STATIC_CHARGE] = true;
        test.startTaping();

        game.pressAndReleaseKey(characterKey(2));
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_C);
        game.tick();
        game.pressAndReleaseButton(BUTTON_LEFT, MM6_AIR_TAB);
        game.tick();
        game.pressButton(BUTTON_RIGHT, MM6_STATIC_CHARGE_PICTURE);
        game.tick();
        game.releaseButton(BUTTON_RIGHT);
        game.tick();

        EXPECT_EQ(lineTape.map(popupCost).unique(), tape(std::string(), std::string("1"), std::string()));
    }
}

GAME_TEST(MM6OffscreenFrame, DISABLED_SpellbookCapture) {
    // Hidden capture of every default character's spellbook, and of the right-click popup of the first spell on the page
    // for a character who knows it.
    ASSERT_NO_FATAL_FAILURE(requireOffscreenCapture());
    enterFirstMapFromNewGame(game);
    for (int characterIndex = 0; characterIndex < 4; ++characterIndex) {
        if (pParty->activeCharacterIndex() != characterIndex) { // The active character's key opens the character screen.
            game.pressAndReleaseKey(characterKey(characterIndex));
            game.tick();
        }
        game.pressAndReleaseKey(PlatformKey::KEY_C);
        game.tick(3); // Books open over a couple of frames.
        const std::string name = fmt::format("{}-{}", characterIndex, localization->className(pParty->pCharacters[characterIndex].classType));
        writeOffscreenFrame(game, name);
        const Character &character = pParty->pCharacters[characterIndex];
        const SpellId spell = spellsForMagicSchool(character.lastOpenedSpellbookPage)[0];
        if (character.bHaveSpell[spell]) {
            auto button = std::ranges::find(pGUIWindow_CurrentMenu->vButtons, std::string("SpellBook_Spell0"), &GUIButton::id);
            ASSERT_NE(button, pGUIWindow_CurrentMenu->vButtons.end());
            game.pressButton(BUTTON_RIGHT, (*button)->rect.center());
            game.tick();
            game.runGameRoutine([spell] { dword_507B00_spell_info_to_draw_in_popup = spell; }); // The frame cleared it after drawing the popup.
            writeOffscreenFrame(game, name + "-popup");
            game.releaseButton(BUTTON_RIGHT);
            game.tick();
        }
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick(3);
        ASSERT_EQ(current_screen_type, SCREEN_GAME) << characterIndex;
    }
}

GAME_TEST(MM6OffscreenFrame, DISABLED_SpellbookSchoolsCapture) {
    // Hidden capture of all nine spellbook pages with every spell known, the first spell selected and the second Fire
    // spell as the quick spell.
    ASSERT_NO_FATAL_FAILURE(requireOffscreenCapture());
    enterFirstMapFromNewGame(game);
    engine->config->debug.AllMagic.setValue(true);
    pParty->activeCharacter().uQuickSpell = static_cast<SpellId>(2);
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick(3); // Books open over a couple of frames.
    for (MagicSchool school : allMagicSchools()) {
        game.pressGuiButton(fmt::format("SpellBook_School{}", std::to_underlying(school)));
        game.tick();
        game.pressGuiButton("SpellBook_Spell0");
        game.tick();
        writeOffscreenFrame(game, fmt::format("school{}", std::to_underlying(school)));
    }
}
